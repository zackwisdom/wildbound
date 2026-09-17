#include "WildBoundInjuryFeedbackSubsystem.h"

#include "WildBoundInjuryComponent.h"
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
	constexpr int32 InjuryFeedbackSampleRate = 22050;

	enum class EWildBoundInjuryCueType : uint8
	{
		Bleeding,
		Fracture,
		Pain
	};

	USoundWaveProcedural* BuildInjuryCueWave(UObject* Outer, EWildBoundInjuryCueType CueType, float Severity)
	{
		USoundWaveProcedural* Wave = NewObject<USoundWaveProcedural>(Outer);
		if (!Wave)
		{
			return nullptr;
		}

		const float ClampedSeverity = FMath::Clamp(Severity, 0.0f, 1.0f);
		float Duration = 0.24f;
		if (CueType == EWildBoundInjuryCueType::Fracture)
		{
			Duration = 0.18f;
		}
		else if (CueType == EWildBoundInjuryCueType::Pain)
		{
			Duration = 0.46f;
		}

		Wave->NumChannels = 1;
		Wave->SetSampleRate(InjuryFeedbackSampleRate);
		Wave->Duration = Duration;
		Wave->bLooping = false;
		Wave->SoundGroup = SOUNDGROUP_Default;

		const int32 SampleCount = FMath::RoundToInt(InjuryFeedbackSampleRate * Duration);
		TArray<int16> Samples;
		Samples.SetNumZeroed(SampleCount);
		float SmoothedNoise = 0.0f;

		for (int32 SampleIndex = 0; SampleIndex < SampleCount; ++SampleIndex)
		{
			const float TimeSeconds = static_cast<float>(SampleIndex) / static_cast<float>(InjuryFeedbackSampleRate);
			const float NormalizedTime = TimeSeconds / Duration;
			float Signal = 0.0f;

			if (CueType == EWildBoundInjuryCueType::Bleeding)
			{
				const float Frequency = 62.0f + ClampedSeverity * 8.0f;
				const float PulseA = FMath::Exp(-FMath::Square((TimeSeconds - 0.045f) / 0.024f));
				const float PulseB = FMath::Exp(-FMath::Square((TimeSeconds - 0.145f) / 0.035f));
				const float Tone = FMath::Sin(2.0f * PI * Frequency * TimeSeconds);
				Signal = Tone * (PulseA * 0.62f + PulseB * 0.34f);
			}
			else if (CueType == EWildBoundInjuryCueType::Fracture)
			{
				const float RawNoise = FMath::FRandRange(-1.0f, 1.0f);
				SmoothedNoise = FMath::Lerp(SmoothedNoise, RawNoise, 0.16f);
				const float Envelope = FMath::Pow(FMath::Clamp(1.0f - NormalizedTime, 0.0f, 1.0f), 3.0f);
				const float LowTone = FMath::Sin(2.0f * PI * 92.0f * TimeSeconds);
				const float MidTone = FMath::Sin(2.0f * PI * 178.0f * TimeSeconds);
				Signal = (LowTone * 0.58f + MidTone * 0.20f + SmoothedNoise * 0.22f) * Envelope;
			}
			else
			{
				const float Envelope = FMath::Sin(PI * FMath::Clamp(NormalizedTime, 0.0f, 1.0f));
				const float ToneA = FMath::Sin(2.0f * PI * 690.0f * TimeSeconds);
				const float ToneB = FMath::Sin(2.0f * PI * 735.0f * TimeSeconds);
				Signal = (ToneA * 0.60f + ToneB * 0.30f) * Envelope;
			}

			float Gain = FMath::Lerp(0.10f, 0.22f, ClampedSeverity);
			if (CueType == EWildBoundInjuryCueType::Fracture)
			{
				Gain = FMath::Lerp(0.13f, 0.28f, ClampedSeverity);
			}
			else if (CueType == EWildBoundInjuryCueType::Pain)
			{
				Gain = FMath::Lerp(0.035f, 0.085f, ClampedSeverity);
			}

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

	void PlayInjuryCue(UWorld& World, APawn& Pawn, EWildBoundInjuryCueType CueType, float Severity)
	{
		USoundWaveProcedural* Wave = BuildInjuryCueWave(&World, CueType, Severity);
		if (!Wave)
		{
			return;
		}

		float Volume = FMath::Lerp(0.11f, 0.24f, Severity);
		if (CueType == EWildBoundInjuryCueType::Pain)
		{
			Volume = FMath::Lerp(0.045f, 0.10f, Severity);
		}

		UGameplayStatics::SpawnSoundAtLocation(
			&World,
			Wave,
			Pawn.GetActorLocation() + FVector(0.0f, 0.0f, 70.0f),
			FRotator::ZeroRotator,
			Volume,
			1.0f,
			0.0f,
			nullptr,
			nullptr,
			true);
	}
}

void UWildBoundInjuryFeedbackSubsystem::OnWorldBeginPlay(UWorld& InWorld)
{
	Super::OnWorldBeginPlay(InWorld);

	if (!InWorld.IsGameWorld())
	{
		return;
	}

	const float Now = InWorld.GetTimeSeconds();
	NextBleedingCueTime = Now + 1.0f;
	NextFractureCueTime = Now + 0.8f;
	NextPainCueTime = Now + 1.8f;
	InWorld.GetTimerManager().SetTimer(
		FeedbackTimer,
		this,
		&UWildBoundInjuryFeedbackSubsystem::UpdateFeedback,
		0.08f,
		true,
		0.30f);
}

void UWildBoundInjuryFeedbackSubsystem::Deinitialize()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(FeedbackTimer);
	}

	RemoveVisualFeedback();
	FeedbackPawn.Reset();
	InjuryComponent.Reset();
	NextBleedingCueTime = 0.0f;
	NextFractureCueTime = 0.0f;
	NextPainCueTime = 0.0f;
	Super::Deinitialize();
}

void UWildBoundInjuryFeedbackSubsystem::UpdateFeedback()
{
	UWorld* World = GetWorld();
	APlayerController* PlayerController = World ? World->GetFirstPlayerController() : nullptr;
	APawn* Pawn = PlayerController ? PlayerController->GetPawn() : nullptr;
	UWildBoundInjuryComponent* Injury = Pawn ? Pawn->FindComponentByClass<UWildBoundInjuryComponent>() : nullptr;
	if (!World || !Pawn || !Injury)
	{
		return;
	}

	if (FeedbackPawn.Get() != Pawn || InjuryComponent.Get() != Injury || !InjuryViewportRoot.IsValid())
	{
		RemoveVisualFeedback();
		EnsureVisualFeedback(*Pawn, *Injury);
	}

	const float Now = World->GetTimeSeconds();
	const float BleedingSeverity = FMath::Clamp(Injury->BleedingSeverity, 0.0f, 1.0f);
	const float FractureSeverity = FMath::Clamp(Injury->FractureSeverity, 0.0f, 1.0f);
	const float PainSeverity = FMath::Clamp(Injury->PainSeverity, 0.0f, 1.0f);
	const bool bMoving = Pawn->GetVelocity().SizeSquared2D() > FMath::Square(85.0f);

	if (BleedingSeverity >= 0.12f && Now >= NextBleedingCueTime)
	{
		PlayBleedingCue(*World, *Pawn, BleedingSeverity);
		NextBleedingCueTime = Now + FMath::Lerp(1.85f, 0.68f, BleedingSeverity);
	}

	if (FractureSeverity >= 0.24f && bMoving && Now >= NextFractureCueTime)
	{
		PlayFractureCue(*World, *Pawn, FractureSeverity);
		NextFractureCueTime = Now + FMath::Lerp(1.30f, 0.58f, FractureSeverity);
	}

	if (PainSeverity >= 0.45f && Now >= NextPainCueTime)
	{
		PlayPainCue(*World, *Pawn, PainSeverity);
		NextPainCueTime = Now + FMath::Lerp(3.60f, 1.75f, PainSeverity);
	}
}

void UWildBoundInjuryFeedbackSubsystem::EnsureVisualFeedback(APawn& Pawn, UWildBoundInjuryComponent& Injury)
{
	if (InjuryViewportRoot.IsValid() || !GEngine || !GEngine->GameViewport)
	{
		return;
	}

	FeedbackPawn = &Pawn;
	InjuryComponent = &Injury;
	const TWeakObjectPtr<UWildBoundInjuryComponent> WeakInjury = &Injury;
	const TWeakObjectPtr<APawn> WeakPawn = &Pawn;

	TSharedPtr<SOverlay> Overlay;
	SAssignNew(Overlay, SOverlay)

	+ SOverlay::Slot()
	.HAlign(HAlign_Fill)
	.VAlign(VAlign_Fill)
	[
		SNew(SBorder)
		.Padding(0.0f)
		.Visibility_Lambda([WeakInjury]()
		{
			const UWildBoundInjuryComponent* CurrentInjury = WeakInjury.Get();
			return CurrentInjury && CurrentInjury->BleedingSeverity > 0.03f
				? EVisibility::HitTestInvisible
				: EVisibility::Collapsed;
		})
		.BorderBackgroundColor_Lambda([WeakInjury]()
		{
			const UWildBoundInjuryComponent* CurrentInjury = WeakInjury.Get();
			const UWorld* World = CurrentInjury ? CurrentInjury->GetWorld() : nullptr;
			const float Severity = CurrentInjury ? FMath::Clamp(CurrentInjury->BleedingSeverity, 0.0f, 1.0f) : 0.0f;
			const float TimeSeconds = World ? World->GetTimeSeconds() : 0.0f;
			const float PulseRate = FMath::Lerp(2.8f, 5.2f, Severity);
			const float Pulse = 0.45f + 0.55f * FMath::Square(0.5f + 0.5f * FMath::Sin(TimeSeconds * PulseRate));
			const float Alpha = FMath::Clamp((0.012f + 0.052f * Severity) * Pulse, 0.0f, 0.064f);
			return FSlateColor(FLinearColor(0.34f, 0.012f, 0.008f, Alpha));
		})
	]

	+ SOverlay::Slot()
	.HAlign(HAlign_Fill)
	.VAlign(VAlign_Fill)
	[
		SNew(SBorder)
		.Padding(0.0f)
		.Visibility_Lambda([WeakInjury, WeakPawn]()
		{
			const UWildBoundInjuryComponent* CurrentInjury = WeakInjury.Get();
			const APawn* CurrentPawn = WeakPawn.Get();
			const bool bMoving = CurrentPawn && CurrentPawn->GetVelocity().SizeSquared2D() > FMath::Square(85.0f);
			return CurrentInjury && CurrentInjury->FractureSeverity > 0.08f && bMoving
				? EVisibility::HitTestInvisible
				: EVisibility::Collapsed;
		})
		.BorderBackgroundColor_Lambda([WeakInjury]()
		{
			const UWildBoundInjuryComponent* CurrentInjury = WeakInjury.Get();
			const UWorld* World = CurrentInjury ? CurrentInjury->GetWorld() : nullptr;
			const float Severity = CurrentInjury ? FMath::Clamp(CurrentInjury->FractureSeverity, 0.0f, 1.0f) : 0.0f;
			const float TimeSeconds = World ? World->GetTimeSeconds() : 0.0f;
			const float StepPulse = FMath::Pow(FMath::Abs(FMath::Sin(TimeSeconds * 4.1f)), 8.0f);
			const float Alpha = FMath::Clamp(StepPulse * Severity * 0.030f, 0.0f, 0.030f);
			return FSlateColor(FLinearColor(0.42f, 0.19f, 0.045f, Alpha));
		})
	]

	+ SOverlay::Slot()
	.HAlign(HAlign_Fill)
	.VAlign(VAlign_Fill)
	[
		SNew(SBorder)
		.Padding(0.0f)
		.Visibility_Lambda([WeakInjury]()
		{
			const UWildBoundInjuryComponent* CurrentInjury = WeakInjury.Get();
			return CurrentInjury && CurrentInjury->PainSeverity > 0.18f
				? EVisibility::HitTestInvisible
				: EVisibility::Collapsed;
		})
		.BorderBackgroundColor_Lambda([WeakInjury]()
		{
			const UWildBoundInjuryComponent* CurrentInjury = WeakInjury.Get();
			const UWorld* World = CurrentInjury ? CurrentInjury->GetWorld() : nullptr;
			const float Severity = CurrentInjury ? FMath::Clamp(CurrentInjury->PainSeverity, 0.0f, 1.0f) : 0.0f;
			const float TimeSeconds = World ? World->GetTimeSeconds() : 0.0f;
			const float Drift = 0.72f + 0.28f * (0.5f + 0.5f * FMath::Sin(TimeSeconds * 1.35f));
			const float Alpha = FMath::Clamp(Severity * 0.020f * Drift, 0.0f, 0.020f);
			return FSlateColor(FLinearColor(0.17f, 0.105f, 0.055f, Alpha));
		})
	];

	InjuryViewportRoot = Overlay;
	GEngine->GameViewport->AddViewportWidgetContent(InjuryViewportRoot.ToSharedRef(), 112);
}

void UWildBoundInjuryFeedbackSubsystem::RemoveVisualFeedback()
{
	if (InjuryViewportRoot.IsValid() && GEngine && GEngine->GameViewport)
	{
		GEngine->GameViewport->RemoveViewportWidgetContent(InjuryViewportRoot.ToSharedRef());
	}
	InjuryViewportRoot.Reset();
	FeedbackPawn.Reset();
	InjuryComponent.Reset();
}

void UWildBoundInjuryFeedbackSubsystem::PlayBleedingCue(UWorld& World, APawn& Pawn, float Severity)
{
	PlayInjuryCue(World, Pawn, EWildBoundInjuryCueType::Bleeding, FMath::Clamp(Severity, 0.0f, 1.0f));
}

void UWildBoundInjuryFeedbackSubsystem::PlayFractureCue(UWorld& World, APawn& Pawn, float Severity)
{
	PlayInjuryCue(World, Pawn, EWildBoundInjuryCueType::Fracture, FMath::Clamp(Severity, 0.0f, 1.0f));
}

void UWildBoundInjuryFeedbackSubsystem::PlayPainCue(UWorld& World, APawn& Pawn, float Severity)
{
	PlayInjuryCue(World, Pawn, EWildBoundInjuryCueType::Pain, FMath::Clamp(Severity, 0.0f, 1.0f));
}
