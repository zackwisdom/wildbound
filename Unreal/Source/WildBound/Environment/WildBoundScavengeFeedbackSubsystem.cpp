#include "WildBoundScavengeFeedbackSubsystem.h"

#include "../Player/WildBoundInteractionComponent.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundWaveProcedural.h"

namespace
{
	const FName InteractableTag(TEXT("WBInteractable"));
	const FName ContainerTag(TEXT("WBTypeContainer"));
	const FName SearchedTag(TEXT("WBContainerSearched"));
	const FName EmptyTag(TEXT("WBContainerEmpty"));
	const FName OpenTag(TEXT("WBContainerOpen"));
	const FName PryLockedTag(TEXT("WBPryLocked"));

	const FName QualityUncommonTag(TEXT("WBLootQualityUncommon"));
	const FName QualityRareTag(TEXT("WBLootQualityRare"));
	const FName QualityEpicTag(TEXT("WBLootQualityEpic"));

	const FName CrateTag(TEXT("WBContainerCrate"));
	const FName CabinetTag(TEXT("WBContainerCabinet"));
	const FName LockerTag(TEXT("WBContainerLocker"));
	const FName DumpsterTag(TEXT("WBContainerDumpster"));
	const FName ToolboxTag(TEXT("WBContainerToolbox"));
	const FName CoolerTag(TEXT("WBContainerCooler"));

	constexpr float ScavengeTraceDistance = 450.0f;
	constexpr int32 TransitionSampleRate = 22050;

	FString GetContainerQualityName(const AActor& Container)
	{
		if (Container.ActorHasTag(QualityEpicTag)) return TEXT("EPIC");
		if (Container.ActorHasTag(QualityRareTag)) return TEXT("RARE");
		if (Container.ActorHasTag(QualityUncommonTag)) return TEXT("UNCOMMON");
		return TEXT("COMMON");
	}

	FString GetContainerTypeName(const AActor& Container)
	{
		if (Container.ActorHasTag(ToolboxTag)) return TEXT("TOOLBOX");
		if (Container.ActorHasTag(LockerTag)) return TEXT("LOCKER");
		if (Container.ActorHasTag(DumpsterTag)) return TEXT("DUMPSTER");
		if (Container.ActorHasTag(CoolerTag)) return TEXT("COOLER");
		if (Container.ActorHasTag(CabinetTag)) return TEXT("CABINET");
		if (Container.ActorHasTag(CrateTag)) return TEXT("CRATE");
		return TEXT("CONTAINER");
	}

	USoundWaveProcedural* BuildTransitionWave(UObject* Outer, bool bOpening)
	{
		USoundWaveProcedural* Wave = NewObject<USoundWaveProcedural>(Outer);
		if (!Wave)
		{
			return nullptr;
		}

		const float Duration = bOpening ? 0.14f : 0.10f;
		Wave->NumChannels = 1;
		Wave->SetSampleRate(TransitionSampleRate);
		Wave->Duration = Duration;
		Wave->bLooping = false;
		Wave->SoundGroup = SOUNDGROUP_Default;

		const int32 SampleCount = FMath::RoundToInt(static_cast<float>(TransitionSampleRate) * Duration);
		TArray<int16> Samples;
		Samples.SetNumZeroed(SampleCount);

		const float BaseFrequency = bOpening ? 190.0f : 145.0f;
		const float AccentFrequency = bOpening ? 330.0f : 255.0f;
		for (int32 Index = 0; Index < SampleCount; ++Index)
		{
			const float Time = static_cast<float>(Index) / static_cast<float>(TransitionSampleRate);
			const float Normalized = Time / Duration;
			const float Envelope = FMath::Pow(FMath::Clamp(1.0f - Normalized, 0.0f, 1.0f), 2.0f);
			const float Impact = FMath::Sin(2.0f * PI * BaseFrequency * Time) * 0.58f;
			const float Mechanism = FMath::Sin(2.0f * PI * AccentFrequency * Time) * 0.24f;
			const float Click = Normalized < 0.22f
				? FMath::Sin(2.0f * PI * 760.0f * Time) * 0.18f
				: 0.0f;
			const float Signal = (Impact + Mechanism + Click) * Envelope * 0.42f;
			Samples[Index] = static_cast<int16>(FMath::Clamp(FMath::RoundToInt(Signal * 32767.0f), -32768, 32767));
		}

		Wave->QueueAudio(reinterpret_cast<const uint8*>(Samples.GetData()), Samples.Num() * sizeof(int16));
		return Wave;
	}
}

void UWildBoundScavengeFeedbackSubsystem::OnWorldBeginPlay(UWorld& InWorld)
{
	Super::OnWorldBeginPlay(InWorld);
	if (!InWorld.IsGameWorld())
	{
		return;
	}

	InWorld.GetTimerManager().SetTimer(
		FeedbackTimer,
		this,
		&UWildBoundScavengeFeedbackSubsystem::UpdateScavengeFeedback,
		0.06f,
		true,
		0.10f);
}

void UWildBoundScavengeFeedbackSubsystem::Deinitialize()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(FeedbackTimer);
	}

	if (OpenContainer.IsValid() && bOpenPoseApplied)
	{
		RestoreContainerOpenPose(*OpenContainer.Get());
	}

	LastTargetedContainer.Reset();
	OpenContainer.Reset();
	bOpenPoseApplied = false;
	Super::Deinitialize();
}

void UWildBoundScavengeFeedbackSubsystem::UpdateScavengeFeedback()
{
	UWorld* World = GetWorld();
	if (!World || !World->IsGameWorld())
	{
		return;
	}

	APawn* Pawn = UGameplayStatics::GetPlayerPawn(World, 0);
	UWildBoundInteractionComponent* Interaction = Pawn
		? Pawn->FindComponentByClass<UWildBoundInteractionComponent>()
		: nullptr;
	if (!Interaction)
	{
		return;
	}

	RefreshSearchedContainerStates();
	UpdateContextPrompt(*Interaction);
	UpdateOpenContainerState(*Interaction);
}

void UWildBoundScavengeFeedbackSubsystem::RefreshSearchedContainerStates()
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	for (TActorIterator<AActor> It(World); It; ++It)
	{
		AActor* Container = *It;
		if (!Container || !Container->ActorHasTag(ContainerTag) || !Container->ActorHasTag(SearchedTag))
		{
			continue;
		}

		// Searched containers remain usable as persistent field storage, even when empty.
		Container->Tags.AddUnique(InteractableTag);
	}
}

void UWildBoundScavengeFeedbackSubsystem::UpdateContextPrompt(UWildBoundInteractionComponent& Interaction)
{
	UWorld* World = GetWorld();
	APawn* Pawn = UGameplayStatics::GetPlayerPawn(World, 0);
	APlayerController* PlayerController = Pawn ? Cast<APlayerController>(Pawn->GetController()) : nullptr;
	if (!World || !Pawn || !PlayerController || Interaction.IsLootWindowOpen())
	{
		return;
	}

	FVector ViewLocation;
	FRotator ViewRotation;
	PlayerController->GetPlayerViewPoint(ViewLocation, ViewRotation);
	const FVector TraceEnd = ViewLocation + ViewRotation.Vector() * ScavengeTraceDistance;
	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(WildBoundScavengeFeedbackTrace), false, Pawn);
	FHitResult Hit;
	if (!World->LineTraceSingleByChannel(Hit, ViewLocation, TraceEnd, ECC_Visibility, QueryParams))
	{
		return;
	}

	AActor* Target = Hit.GetActor();
	if (!Target || !Target->ActorHasTag(ContainerTag) || Target->ActorHasTag(PryLockedTag))
	{
		return;
	}

	LastTargetedContainer = Target;
	const bool bSearched = Target->ActorHasTag(SearchedTag);
	const bool bEmpty = Target->ActorHasTag(EmptyTag);
	const FString Quality = GetContainerQualityName(*Target);
	const FString Type = GetContainerTypeName(*Target);

	FString Prompt;
	if (!bSearched)
	{
		Prompt = FString::Printf(TEXT("[E] SEARCH  |  [%s] %s"), *Quality, *Type);
	}
	else if (bEmpty)
	{
		Prompt = FString::Printf(TEXT("[E] OPEN  |  EMPTY  |  [%s] %s"), *Quality, *Type);
	}
	else
	{
		Prompt = FString::Printf(TEXT("[E] OPEN  |  SEARCHED  |  [%s] %s"), *Quality, *Type);
	}

	Interaction.SetContextPrompt(Prompt, 80);
}

void UWildBoundScavengeFeedbackSubsystem::UpdateOpenContainerState(UWildBoundInteractionComponent& Interaction)
{
	const bool bLootOpen = Interaction.IsLootWindowOpen();
	if (bLootOpen)
	{
		if (!OpenContainer.IsValid())
		{
			AActor* Candidate = LastTargetedContainer.Get();
			if (Candidate && Candidate->ActorHasTag(ContainerTag) && Candidate->ActorHasTag(SearchedTag))
			{
				OpenContainer = Candidate;
				Candidate->Tags.AddUnique(OpenTag);
				Candidate->Tags.AddUnique(InteractableTag);
				ApplyContainerOpenPose(*Candidate);
				PlayContainerTransitionSound(*Candidate, true);
			}
		}

		if (AActor* Container = OpenContainer.Get())
		{
			const TArray<FWildBoundContainerLootEntry>* Loot = Interaction.GetOpenContainerLoot();
			if (Loot && Loot->IsEmpty())
			{
				Container->Tags.AddUnique(EmptyTag);
			}
			else if (Loot)
			{
				Container->Tags.Remove(EmptyTag);
			}
			Container->Tags.AddUnique(InteractableTag);
		}
		return;
	}

	if (AActor* Container = OpenContainer.Get())
	{
		Container->Tags.Remove(OpenTag);
		Container->Tags.AddUnique(InteractableTag);
		RestoreContainerOpenPose(*Container);
		PlayContainerTransitionSound(*Container, false);
	}
	else
	{
		bOpenPoseApplied = false;
		AppliedOpenRotation = FRotator::ZeroRotator;
		AppliedOpenOffset = FVector::ZeroVector;
	}

	OpenContainer.Reset();
}

void UWildBoundScavengeFeedbackSubsystem::ApplyContainerOpenPose(AActor& Container)
{
	if (bOpenPoseApplied)
	{
		return;
	}

	AppliedOpenRotation = FRotator::ZeroRotator;
	AppliedOpenOffset = FVector::ZeroVector;

	if (Container.ActorHasTag(CrateTag) || Container.ActorHasTag(CoolerTag) || Container.ActorHasTag(ToolboxTag))
	{
		AppliedOpenRotation.Pitch = -6.0f;
		AppliedOpenOffset.Z = 4.0f;
	}
	else if (Container.ActorHasTag(CabinetTag) || Container.ActorHasTag(LockerTag))
	{
		AppliedOpenRotation.Yaw = 9.0f;
		AppliedOpenOffset = Container.GetActorForwardVector() * 5.0f;
	}
	else if (Container.ActorHasTag(DumpsterTag))
	{
		AppliedOpenRotation.Pitch = 7.0f;
		AppliedOpenOffset.Z = 3.0f;
	}
	else
	{
		AppliedOpenRotation.Roll = 2.5f;
		AppliedOpenOffset.Z = 2.0f;
	}

	Container.AddActorLocalRotation(AppliedOpenRotation);
	Container.AddActorWorldOffset(AppliedOpenOffset, false);
	bOpenPoseApplied = true;
}

void UWildBoundScavengeFeedbackSubsystem::RestoreContainerOpenPose(AActor& Container)
{
	if (!bOpenPoseApplied)
	{
		return;
	}

	Container.AddActorWorldOffset(-AppliedOpenOffset, false);
	Container.AddActorLocalRotation(-AppliedOpenRotation);
	AppliedOpenRotation = FRotator::ZeroRotator;
	AppliedOpenOffset = FVector::ZeroVector;
	bOpenPoseApplied = false;
}

void UWildBoundScavengeFeedbackSubsystem::PlayContainerTransitionSound(const AActor& Container, bool bOpening) const
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	USoundWaveProcedural* Wave = BuildTransitionWave(World, bOpening);
	if (!Wave)
	{
		return;
	}

	UGameplayStatics::SpawnSoundAtLocation(
		World,
		Wave,
		Container.GetActorLocation(),
		FRotator::ZeroRotator,
		bOpening ? 0.52f : 0.38f,
		bOpening ? 1.0f : 0.92f,
		0.0f,
		nullptr,
		nullptr,
		true);
}
