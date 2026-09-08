#include "WildBoundLootFeedbackSubsystem.h"

#include "../Inventory/WildBoundInventoryComponent.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundWaveProcedural.h"

namespace
{
	const FName FeedbackSearchedTag(TEXT("WBContainerSearched"));
	const FName FeedbackQualityRareTag(TEXT("WBLootQualityRare"));
	const FName FeedbackQualityEpicTag(TEXT("WBLootQualityEpic"));
	const FName FeedbackQualityUncommonTag(TEXT("WBLootQualityUncommon"));
	const FName FeedbackRarityVisualTag(TEXT("WildBoundLootRarityVisual"));
	const FName FeedbackCrateTag(TEXT("WBContainerCrate"));
	const FName FeedbackCoolerTag(TEXT("WBContainerCooler"));
	const FName FeedbackToolboxTag(TEXT("WBContainerToolbox"));
	const FName FeedbackCabinetTag(TEXT("WBContainerCabinet"));
	const FName FeedbackLockerTag(TEXT("WBContainerLocker"));
	const FName FeedbackDumpsterTag(TEXT("WBContainerDumpster"));

	constexpr int32 FeedbackSampleRate = 22050;
	constexpr float FeedbackSoundDuration = 0.18f;

	int32 GetFeedbackQualityTier(const AActor& Actor)
	{
		if (Actor.ActorHasTag(FeedbackQualityEpicTag)) return 3;
		if (Actor.ActorHasTag(FeedbackQualityRareTag)) return 2;
		if (Actor.ActorHasTag(FeedbackQualityUncommonTag)) return 1;
		return 0;
	}

	void ApplyOpenedContainerPose(AActor& Actor)
	{
		FRotator RotationDelta = FRotator::ZeroRotator;
		FVector WorldOffset = FVector::ZeroVector;

		if (Actor.ActorHasTag(FeedbackCrateTag)
			|| Actor.ActorHasTag(FeedbackCoolerTag)
			|| Actor.ActorHasTag(FeedbackToolboxTag))
		{
			RotationDelta.Pitch = -8.0f;
			WorldOffset.Z = -7.0f;
		}
		else if (Actor.ActorHasTag(FeedbackCabinetTag)
			|| Actor.ActorHasTag(FeedbackLockerTag))
		{
			RotationDelta.Yaw = 7.0f;
			WorldOffset += Actor.GetActorForwardVector() * 8.0f;
		}
		else if (Actor.ActorHasTag(FeedbackDumpsterTag))
		{
			RotationDelta.Pitch = 6.0f;
			WorldOffset.Z = -5.0f;
		}
		else
		{
			RotationDelta.Roll = 3.0f;
			WorldOffset.Z = -4.0f;
		}

		Actor.AddActorLocalRotation(RotationDelta);
		Actor.AddActorWorldOffset(WorldOffset, false);
	}

	void RemoveNearbyQualityMarker(UWorld& World, const AActor& Container)
	{
		TArray<AActor*> MarkersToRemove;
		const FVector ContainerLocation = Container.GetActorLocation();
		const float MaxDistanceSquared = FMath::Square(260.0f);

		for (TActorIterator<AActor> It(&World); It; ++It)
		{
			AActor* Candidate = *It;
			if (Candidate
				&& Candidate->ActorHasTag(FeedbackRarityVisualTag)
				&& FVector::DistSquared(Candidate->GetActorLocation(), ContainerLocation) <= MaxDistanceSquared)
			{
				MarkersToRemove.Add(Candidate);
			}
		}

		for (AActor* Marker : MarkersToRemove)
		{
			if (IsValid(Marker))
			{
				Marker->Destroy();
			}
		}
	}

	USoundWaveProcedural* BuildFeedbackWave(UObject* Outer, int32 Tier, bool bContainerSound)
	{
		USoundWaveProcedural* Wave = NewObject<USoundWaveProcedural>(Outer);
		if (!Wave)
		{
			return nullptr;
		}

		Wave->NumChannels = 1;
		Wave->SetSampleRate(FeedbackSampleRate);
		Wave->Duration = FeedbackSoundDuration;
		Wave->bLooping = false;
		Wave->SoundGroup = SOUNDGROUP_Default;

		const int32 SampleCount = FMath::RoundToInt(FeedbackSampleRate * FeedbackSoundDuration);
		TArray<int16> Samples;
		Samples.SetNumZeroed(SampleCount);

		const float BaseFrequency = bContainerSound
			? (280.0f + static_cast<float>(Tier) * 55.0f)
			: (620.0f + static_cast<float>(Tier) * 145.0f);
		const float AccentFrequency = bContainerSound
			? (560.0f + static_cast<float>(Tier) * 85.0f)
			: (BaseFrequency * 1.55f);
		const float TierGain = 0.34f + static_cast<float>(Tier) * 0.08f;

		for (int32 SampleIndex = 0; SampleIndex < SampleCount; ++SampleIndex)
		{
			const float TimeSeconds = static_cast<float>(SampleIndex) / static_cast<float>(FeedbackSampleRate);
			const float NormalizedTime = TimeSeconds / FeedbackSoundDuration;
			const float Envelope = FMath::Pow(FMath::Clamp(1.0f - NormalizedTime, 0.0f, 1.0f), 2.2f);
			const float ToneA = FMath::Sin(2.0f * PI * BaseFrequency * TimeSeconds);
			const float ToneB = FMath::Sin(2.0f * PI * AccentFrequency * TimeSeconds);
			const float Noise = FMath::FRandRange(-1.0f, 1.0f);

			float Signal = bContainerSound
				? (ToneA * 0.34f + ToneB * 0.18f + Noise * 0.30f)
				: (ToneA * 0.54f + ToneB * 0.28f + Noise * 0.08f);

			if (Tier >= 2 && NormalizedTime > 0.38f && NormalizedTime < 0.58f)
			{
				Signal += FMath::Sin(2.0f * PI * AccentFrequency * 1.65f * TimeSeconds) * 0.22f;
			}

			const int32 PCM = FMath::Clamp(
				FMath::RoundToInt(Signal * Envelope * TierGain * 32767.0f),
				-32768,
				32767);
			Samples[SampleIndex] = static_cast<int16>(PCM);
		}

		Wave->QueueAudio(
			reinterpret_cast<const uint8*>(Samples.GetData()),
			Samples.Num() * sizeof(int16));
		return Wave;
	}

	void PlayFeedbackSound(UWorld& World, const FVector& Location, int32 Tier, bool bContainerSound)
	{
		USoundWaveProcedural* Wave = BuildFeedbackWave(&World, FMath::Clamp(Tier, 0, 3), bContainerSound);
		if (!Wave)
		{
			return;
		}

		UGameplayStatics::SpawnSoundAtLocation(
			&World,
			Wave,
			Location,
			FRotator::ZeroRotator,
			0.72f,
			1.0f,
			0.0f,
			nullptr,
			nullptr,
			true);
	}
}

void UWildBoundLootFeedbackSubsystem::OnWorldBeginPlay(UWorld& InWorld)
{
	Super::OnWorldBeginPlay(InWorld);

	if (!InWorld.IsGameWorld())
	{
		return;
	}

	UpdateLootFeedback();
	InWorld.GetTimerManager().SetTimer(
		FeedbackTimer,
		this,
		&UWildBoundLootFeedbackSubsystem::UpdateLootFeedback,
		0.14f,
		true,
		0.20f);
}

void UWildBoundLootFeedbackSubsystem::Deinitialize()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(FeedbackTimer);
	}

	ProcessedContainers.Reset();
	PreviousInventoryCounts.Reset();
	bInventorySnapshotInitialized = false;
	Super::Deinitialize();
}

void UWildBoundLootFeedbackSubsystem::UpdateLootFeedback()
{
	UWorld* World = GetWorld();
	if (!World || !World->IsGameWorld())
	{
		return;
	}

	ProcessOpenedContainers();
	ProcessInventoryPickupFeedback();
}

void UWildBoundLootFeedbackSubsystem::ProcessOpenedContainers()
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	for (auto It = ProcessedContainers.CreateIterator(); It; ++It)
	{
		if (!It->IsValid())
		{
			It.RemoveCurrent();
		}
	}

	for (TActorIterator<AActor> It(World); It; ++It)
	{
		AActor* Actor = *It;
		if (!Actor || !Actor->ActorHasTag(FeedbackSearchedTag) || ProcessedContainers.Contains(Actor))
		{
			continue;
		}

		ProcessedContainers.Add(Actor);
		const int32 QualityTier = GetFeedbackQualityTier(*Actor);
		ApplyOpenedContainerPose(*Actor);
		RemoveNearbyQualityMarker(*World, *Actor);
		PlayFeedbackSound(*World, Actor->GetActorLocation(), QualityTier, true);
	}
}

void UWildBoundLootFeedbackSubsystem::ProcessInventoryPickupFeedback()
{
	UWorld* World = GetWorld();
	APlayerController* PlayerController = World ? World->GetFirstPlayerController() : nullptr;
	APawn* Pawn = PlayerController ? PlayerController->GetPawn() : nullptr;
	UWildBoundInventoryComponent* Inventory = Pawn ? Pawn->FindComponentByClass<UWildBoundInventoryComponent>() : nullptr;
	if (!World || !Pawn || !Inventory)
	{
		return;
	}

	TMap<FName, int32> CurrentCounts;
	int32 HighestNewRarityTier = INDEX_NONE;

	for (const FWildBoundInventoryStack& Stack : Inventory->Stacks)
	{
		if (Stack.ItemId.IsNone() || Stack.Quantity <= 0)
		{
			continue;
		}

		CurrentCounts.FindOrAdd(Stack.ItemId) += Stack.Quantity;
	}

	if (bInventorySnapshotInitialized)
	{
		for (const TPair<FName, int32>& Pair : CurrentCounts)
		{
			const int32 PreviousCount = PreviousInventoryCounts.FindRef(Pair.Key);
			if (Pair.Value > PreviousCount)
			{
				HighestNewRarityTier = FMath::Max(
					HighestNewRarityTier,
					Inventory->GetItemRarityTier(Pair.Key));
			}
		}
	}

	PreviousInventoryCounts = MoveTemp(CurrentCounts);

	if (!bInventorySnapshotInitialized)
	{
		bInventorySnapshotInitialized = true;
		return;
	}

	if (HighestNewRarityTier >= 0)
	{
		PlayFeedbackSound(
			*World,
			Pawn->GetActorLocation() + FVector(0.0f, 0.0f, 70.0f),
			HighestNewRarityTier,
			false);
	}
}
