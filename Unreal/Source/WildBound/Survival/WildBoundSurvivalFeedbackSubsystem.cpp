#include "WildBoundSurvivalFeedbackSubsystem.h"

#include "WildBoundSurvivalComponent.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundWaveProcedural.h"

namespace
{
	constexpr int32 BodyFeedbackSampleRate = 22050;

	float GetLowStaminaSeverity(const UWildBoundSurvivalComponent& Survival)
	{
		constexpr float FeedbackThreshold = 0.35f;
		return FMath::Clamp((FeedbackThreshold - Survival.GetStaminaPercent()) / FeedbackThreshold, 0.0f, 1.0f);
	}

	float GetCriticalNutritionSeverity(const UWildBoundSurvivalComponent& Survival)
	{
		const float LowestNutrition = FMath::Min(Survival.GetHungerPercent(), Survival.GetThirstPercent());
		const float Threshold = FMath::Max(Survival.CriticalNutritionThreshold, KINDA_SMALL_NUMBER);
		return FMath::Clamp((Threshold - LowestNutrition) / Threshold, 0.0f, 1.0f);
	}

	float GetLowHealthSeverity(const UWildBoundSurvivalComponent& Survival)
	{
		constexpr float FeedbackThreshold = 0.35f;
		return FMath::Clamp((FeedbackThreshold - Survival.GetHealthPercent()) / FeedbackThreshold, 0.0f, 1.0f);
	}

	USoundWaveProcedural* BuildBodyFeedbackWave(UObject* Outer, bool bHeartbeat, float Severity)
	{
		USoundWaveProcedural* Wave = NewObject<USoundWaveProcedural>(Outer);
		if (!Wave)
		{
			return nullptr;
		}

		const float ClampedSeverity = FMath::Clamp(Severity, 0.0f, 1.0f);
		const float Duration = bHeartbeat ? 0.24f : 0.62f;
		Wave->NumChannels = 1;
		Wave->SetSampleRate(BodyFeedbackSampleRate);
		Wave->Duration = Duration;
		Wave->bLooping = false;
		Wave->SoundGroup = SOUNDGROUP_Default;

		const int32 SampleCount = FMath::RoundToInt(BodyFeedbackSampleRate * Duration);
		TArray<int16> Samples;
		Samples.SetNumZeroed(SampleCount);

		float SmoothedNoise = 0.0f;
		for (int32 SampleIndex = 0; SampleIndex < SampleCount; ++SampleIndex)
		{
			const float TimeSeconds = static_cast<float>(SampleIndex) / static_cast<float>(BodyFeedbackSampleRate);
			const float NormalizedTime = TimeSeconds / Duration;
			float Signal = 0.0f;

			if (bHeartbeat)
			{
				const float Frequency = 58.0f + (ClampedSeverity * 10.0f);
				const float PulseA = FMath::Exp(-FMath::Square((TimeSeconds - 0.045f) / 0.022f));
				const float PulseB = FMath::Exp(-FMath::Square((TimeSeconds - 0.145f) / 0.030f));
				const float Tone = FMath::Sin(2.0f * PI * Frequency * TimeSeconds);
				Signal = Tone * ((PulseA * 0.82f) + (PulseB * 0.50f));
			}
			else
			{
				const float RawNoise = FMath::FRandRange(-1.0f, 1.0f);
				SmoothedNoise = FMath::Lerp(SmoothedNoise, RawNoise, 0.075f);
				const float Envelope = FMath::Sin(PI * FMath::Clamp(NormalizedTime, 0.0f, 1.0f));
				const float AirTone = FMath::Sin(2.0f * PI * 115.0f * TimeSeconds) * 0.08f;
				Signal = (SmoothedNoise * 0.70f + AirTone) * Envelope;
			}

			const float Gain = bHeartbeat
				? FMath::Lerp(0.28f, 0.46f, ClampedSeverity)
				: FMath::Lerp(0.20f, 0.34f, ClampedSeverity);
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

	void PlayBodyFeedbackSound(UWorld& World, APawn& Pawn, bool bHeartbeat, float Severity)
	{
		USoundWaveProcedural* Wave = BuildBodyFeedbackWave(&World, bHeartbeat, Severity);
		if (!Wave)
		{
			return;
		}

		const float Volume = bHeartbeat
			? FMath::Lerp(0.16f, 0.32f, Severity)
			: FMath::Lerp(0.10f, 0.22f, Severity);

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

void UWildBoundSurvivalFeedbackSubsystem::OnWorldBeginPlay(UWorld& InWorld)
{
	Super::OnWorldBeginPlay(InWorld);

	if (!InWorld.IsGameWorld())
	{
		return;
	}

	const float Now = InWorld.GetTimeSeconds();
	NextHeartbeatTime = Now + 0.8f;
	NextBreathTime = Now + 1.2f;
	InWorld.GetTimerManager().SetTimer(
		FeedbackTimer,
		this,
		&UWildBoundSurvivalFeedbackSubsystem::UpdateFeedback,
		0.08f,
		true,
		0.25f);
}

void UWildBoundSurvivalFeedbackSubsystem::Deinitialize()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(FeedbackTimer);
	}

	NextHeartbeatTime = 0.0f;
	NextBreathTime = 0.0f;
	Super::Deinitialize();
}

void UWildBoundSurvivalFeedbackSubsystem::UpdateFeedback()
{
	UWorld* World = GetWorld();
	APlayerController* PlayerController = World ? World->GetFirstPlayerController() : nullptr;
	APawn* Pawn = PlayerController ? PlayerController->GetPawn() : nullptr;
	UWildBoundSurvivalComponent* Survival = Pawn ? Pawn->FindComponentByClass<UWildBoundSurvivalComponent>() : nullptr;
	if (!World || !Pawn || !Survival || !Survival->IsAlive())
	{
		return;
	}

	const float Now = World->GetTimeSeconds();
	const float StaminaSeverity = GetLowStaminaSeverity(*Survival);
	const float NutritionSeverity = GetCriticalNutritionSeverity(*Survival);
	const float HealthSeverity = GetLowHealthSeverity(*Survival);
	const bool bMoving = Pawn->GetVelocity().SizeSquared2D() > FMath::Square(120.0f);

	const float HeartbeatSeverity = FMath::Max3(
		HealthSeverity,
		NutritionSeverity * 0.90f,
		StaminaSeverity * 0.70f);
	if (HeartbeatSeverity >= 0.18f && Now >= NextHeartbeatTime)
	{
		PlayHeartbeat(*World, *Pawn, HeartbeatSeverity);
		NextHeartbeatTime = Now + FMath::Lerp(1.08f, 0.50f, HeartbeatSeverity);
	}

	const float MovementScale = bMoving ? 1.0f : 0.60f;
	const float BreathSeverity = FMath::Clamp(
		FMath::Max(StaminaSeverity, NutritionSeverity * 0.68f) * MovementScale,
		0.0f,
		1.0f);
	if (BreathSeverity >= 0.20f && Now >= NextBreathTime)
	{
		PlayBreath(*World, *Pawn, BreathSeverity);
		NextBreathTime = Now + FMath::Lerp(2.20f, 1.05f, BreathSeverity);
	}
}

void UWildBoundSurvivalFeedbackSubsystem::PlayHeartbeat(UWorld& World, APawn& Pawn, float Severity)
{
	PlayBodyFeedbackSound(World, Pawn, true, FMath::Clamp(Severity, 0.0f, 1.0f));
}

void UWildBoundSurvivalFeedbackSubsystem::PlayBreath(UWorld& World, APawn& Pawn, float Severity)
{
	PlayBodyFeedbackSound(World, Pawn, false, FMath::Clamp(Severity, 0.0f, 1.0f));
}
