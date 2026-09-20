#include "WildBoundRadiationFeedbackSubsystem.h"

#include "WildBoundRadiationComponent.h"
#include "Engine/Engine.h"
#include "Engine/GameViewportClient.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundWaveProcedural.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/SOverlay.h"

namespace
{
	constexpr int32 RadiationFeedbackSampleRate = 22050;

	USoundWaveProcedural* BuildRadiationSicknessWave(UObject* Outer, float Severity)
	{
		USoundWaveProcedural* Wave = NewObject<USoundWaveProcedural>(Outer);
		if (!Wave)
		{
			return nullptr;
		}

		const float ClampedSeverity = FMath::Clamp(Severity, 0.0f, 1.0f);
		const float Duration = 0.72f;
		Wave->NumChannels = 1;
		Wave->SetSampleRate(RadiationFeedbackSampleRate);
		Wave->Duration = Duration;
		Wave->bLooping = false;
		Wave->SoundGroup = SOUNDGROUP_Default;

		const int32 SampleCount = FMath::RoundToInt(RadiationFeedbackSampleRate * Duration);
		TArray<int16> Samples;
		Samples.SetNumZeroed(SampleCount);
		float SmoothedNoise = 0.0f;

		for (int32 SampleIndex = 0; SampleIndex < SampleCount; ++SampleIndex)
		{
			const float TimeSeconds = static_cast<float>(SampleIndex) / static_cast<float>(RadiationFeedbackSampleRate);
			const float NormalizedTime = TimeSeconds / Duration;
			const float Envelope = FMath::Sin(PI * FMath::Clamp(NormalizedTime, 0.0f, 1.0f));
			const float RawNoise = FMath::FRandRange(-1.0f, 1.0f);
			SmoothedNoise = FMath::Lerp(SmoothedNoise, RawNoise, 0.035f);

			const float LowRumble = FMath::Sin(2.0f * PI * (46.0f + ClampedSeverity * 8.0f) * TimeSeconds);
			const float ToneA = FMath::Sin(2.0f * PI * 382.0f * TimeSeconds);
			const float ToneB = FMath::Sin(2.0f * PI * 397.0f * TimeSeconds);
			const float BeatTone = ToneA * 0.34f + ToneB * 0.28f;
			const float Signal = (LowRumble * 0.28f + BeatTone * 0.18f + SmoothedNoise * 0.24f) * Envelope;
			const float Gain = FMath::Lerp(0.055f, 0.13f, ClampedSeverity);

			const int32 PCM = FMath::Clamp(
				FMath::RoundToInt(Signal * Gain * 32767.0f),
				-32768,
				32767);
			Samples[SampleIndex] = static_cast<int16>(PCM);
		}

		Wave->QueueAudio(
			reinterpret_cast<const uint8*>(Samples.GetData()),
			Samples.Num() * sizeof(int16));
		return Wave;
	}
}

void UWildBoundRadiationFeedbackSubsystem::OnWorldBeginPlay(UWorld& InWorld)
{
	Super::OnWorldBeginPlay(InWorld);

	if (!InWorld.IsGameWorld())
	{
		return;
	}

	NextSicknessCueTime = InWorld.GetTimeSeconds() + 2.0f;
	InWorld.GetTimerManager().SetTimer(
		FeedbackTimer,
		this,
		&UWildBoundRadiationFeedbackSubsystem::UpdateFeedback,
		0.10f,
		true,
		0.35f);
}

void UWildBoundRadiationFeedbackSubsystem::Deinitialize()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(FeedbackTimer);
	}

	RemoveVisualFeedback();
	NextSicknessCueTime = 0.0f;
	Super::Deinitialize();
}

void UWildBoundRadiationFeedbackSubsystem::UpdateFeedback()
{
	UWorld* World = GetWorld();
	APlayerController* PlayerController = World ? World->GetFirstPlayerController() : nullptr;
	APawn* Pawn = PlayerController ? PlayerController->GetPawn() : nullptr;
	UWildBoundRadiationComponent* Radiation = Pawn ? Pawn->FindComponentByClass<UWildBoundRadiationComponent>() : nullptr;

	if (!World || !Pawn || !Radiation)
	{
		RemoveVisualFeedback();
		return;
	}

	if (FeedbackPawn.Get() != Pawn || RadiationComponent.Get() != Radiation || !RadiationViewportRoot.IsValid())
	{
		RemoveVisualFeedback();
		EnsureVisualFeedback(*Pawn, *Radiation);
	}

	const float DosePercent = Radiation->GetDosePercent();
	if (DosePercent < 0.35f)
	{
		return;
	}

	const float Severity = FMath::Clamp((DosePercent - 0.35f) / 0.65f, 0.0f, 1.0f);
	const float Now = World->GetTimeSeconds();
	if (Now >= NextSicknessCueTime)
	{
		PlaySicknessCue(*World, *Pawn, Severity);
		NextSicknessCueTime = Now + FMath::Lerp(6.5f, 2.6f, Severity);
	}
}

void UWildBoundRadiationFeedbackSubsystem::EnsureVisualFeedback(APawn& Pawn, UWildBoundRadiationComponent& Radiation)
{
	if (RadiationViewportRoot.IsValid() || !GEngine || !GEngine->GameViewport)
	{
		return;
	}

	FeedbackPawn = &Pawn;
	RadiationComponent = &Radiation;
	const TWeakObjectPtr<UWildBoundRadiationComponent> WeakRadiation = &Radiation;

	TSharedPtr<SOverlay> Overlay;
	SAssignNew(Overlay, SOverlay)

	+ SOverlay::Slot()
	.HAlign(HAlign_Fill)
	.VAlign(VAlign_Fill)
	[
		SNew(SBorder)
		.Padding(0.0f)
		.Visibility_Lambda([WeakRadiation]()
		{
			const UWildBoundRadiationComponent* CurrentRadiation = WeakRadiation.Get();
			if (!CurrentRadiation)
			{
				return EVisibility::Collapsed;
		}
			return CurrentRadiation->CurrentExposure >= 8.0f || CurrentRadiation->GetDosePercent() >= 0.20f
				? EVisibility::HitTestInvisible
				: EVisibility::Collapsed;
		})
		.BorderBackgroundColor_Lambda([WeakRadiation]()
		{
			const UWildBoundRadiationComponent* CurrentRadiation = WeakRadiation.Get();
			const UWorld* World = CurrentRadiation ? CurrentRadiation->GetWorld() : nullptr;
			if (!CurrentRadiation)
			{
				return FSlateColor(FLinearColor::Transparent);
			}

			const float Exposure = FMath::Clamp(CurrentRadiation->CurrentExposure / 100.0f, 0.0f, 1.0f);
			const float Dose = CurrentRadiation->GetDosePercent();
			const float DoseSeverity = FMath::Clamp((Dose - 0.20f) / 0.80f, 0.0f, 1.0f);
			const float TimeSeconds = World ? World->GetTimeSeconds() : 0.0f;
			const float ExposureFlicker = 0.72f + 0.28f * (0.5f + 0.5f * FMath::Sin(TimeSeconds * 8.4f));
			const float SicknessDrift = 0.75f + 0.25f * (0.5f + 0.5f * FMath::Sin(TimeSeconds * 1.05f));
			const float Alpha = FMath::Clamp(
				Exposure * 0.026f * ExposureFlicker + DoseSeverity * 0.040f * SicknessDrift,
				0.0f,
				0.062f);

			return FSlateColor(FLinearColor(0.13f, 0.145f, 0.040f, Alpha));
		})
	];

	RadiationViewportRoot = Overlay;
	GEngine->GameViewport->AddViewportWidgetContent(RadiationViewportRoot.ToSharedRef(), 111);
}

void UWildBoundRadiationFeedbackSubsystem::RemoveVisualFeedback()
{
	if (RadiationViewportRoot.IsValid() && GEngine && GEngine->GameViewport)
	{
		GEngine->GameViewport->RemoveViewportWidgetContent(RadiationViewportRoot.ToSharedRef());
	}

	RadiationViewportRoot.Reset();
	FeedbackPawn.Reset();
	RadiationComponent.Reset();
}

void UWildBoundRadiationFeedbackSubsystem::PlaySicknessCue(UWorld& World, APawn& Pawn, float Severity)
{
	USoundWaveProcedural* Wave = BuildRadiationSicknessWave(&World, Severity);
	if (!Wave)
	{
		return;
	}

	UGameplayStatics::SpawnSoundAtLocation(
		&World,
		Wave,
		Pawn.GetActorLocation() + FVector(0.0f, 0.0f, 70.0f),
		FRotator::ZeroRotator,
		FMath::Lerp(0.055f, 0.12f, FMath::Clamp(Severity, 0.0f, 1.0f)),
		1.0f,
		0.0f,
		nullptr,
		nullptr,
		true);
}
