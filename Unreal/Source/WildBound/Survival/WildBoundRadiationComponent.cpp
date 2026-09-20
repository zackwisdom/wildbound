#include "WildBoundRadiationComponent.h"

#include "WildBoundSurvivalComponent.h"
#include "Components/AudioComponent.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Sound/SoundWaveProcedural.h"

namespace
{
	const FName RadiationHotspotTag(TEXT("WBRadiationHotspot"));
	const FName RadiationWeakTag(TEXT("WBRadiationWeak"));
	const FName RadiationStrongTag(TEXT("WBRadiationStrong"));
	const FName RadiationExtremeTag(TEXT("WBRadiationExtreme"));
	constexpr int32 GeigerSampleRate = 44100;
	constexpr float GeigerFrameSeconds = 0.05f;
}

UWildBoundRadiationComponent::UWildBoundRadiationComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.TickInterval = GeigerFrameSeconds;
}

void UWildBoundRadiationComponent::BeginPlay()
{
	Super::BeginPlay();
	InitializeGeigerAudio();
}

void UWildBoundRadiationComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (GeigerAudioComponent)
	{
		GeigerAudioComponent->Stop();
	}

	GeigerAudioComponent = nullptr;
	GeigerSoundWave = nullptr;
	Super::EndPlay(EndPlayReason);
}

void UWildBoundRadiationComponent::TickComponent(
	float DeltaTime,
	ELevelTick TickType,
	FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	AActor* Owner = GetOwner();
	UWorld* World = GetWorld();
	if (!Owner || !World)
	{
		return;
	}

	float TargetExposure = 0.0f;
	const FVector OwnerLocation = Owner->GetActorLocation();
	const float SafeOuterRadius = FMath::Max(OuterRadius, InnerRadius + 1.0f);

	for (TActorIterator<AActor> It(World); It; ++It)
	{
		AActor* Source = *It;
		if (!Source || !Source->ActorHasTag(RadiationHotspotTag))
		{
			continue;
		}

		float SourceIntensity = 1.0f;
		float SourceOuterRadius = SafeOuterRadius;
		float SourceInnerRadius = InnerRadius;

		if (Source->ActorHasTag(RadiationWeakTag))
		{
			// Diffuse runoff: lower peak danger, but readable from farther away.
			SourceIntensity = 0.48f;
			SourceOuterRadius *= 1.28f;
			SourceInnerRadius *= 0.80f;
		}
		else if (Source->ActorHasTag(RadiationExtremeTag))
		{
			SourceIntensity = 1.75f;
			SourceOuterRadius *= 1.32f;
			SourceInnerRadius *= 1.25f;
		}
		else if (Source->ActorHasTag(RadiationStrongTag))
		{
			SourceIntensity = 1.32f;
			SourceOuterRadius *= 1.12f;
			SourceInnerRadius *= 1.10f;
		}

		SourceOuterRadius = FMath::Max(SourceOuterRadius, SourceInnerRadius + 1.0f);
		const float Distance = FVector::Dist(OwnerLocation, Source->GetActorLocation());
		if (Distance >= SourceOuterRadius)
		{
			continue;
		}

		float Strength = 1.0f;
		if (Distance > SourceInnerRadius)
		{
			const float Alpha = (Distance - SourceInnerRadius) / (SourceOuterRadius - SourceInnerRadius);
			Strength = 1.0f - FMath::Clamp(Alpha, 0.0f, 1.0f);
			Strength = FMath::Pow(Strength, 1.35f);
		}

		const float SourceExposure = FMath::Clamp(Strength * SourceIntensity * 100.0f, 0.0f, 100.0f);
		TargetExposure = FMath::Max(TargetExposure, SourceExposure);
	}

	CurrentExposure = FMath::FInterpTo(CurrentExposure, TargetExposure, DeltaTime, 6.0f);
	if (CurrentExposure < 0.05f)
	{
		CurrentExposure = 0.0f;
	}

	TreatmentProtectionRemaining = FMath::Max(0.0f, TreatmentProtectionRemaining - DeltaTime);

	if (CurrentExposure > 0.0f && AccumulatedDose < MaxDose)
	{
		const float DoseGain = GetCurrentDoseRatePerSecond() * DeltaTime;
		AccumulatedDose = FMath::Clamp(AccumulatedDose + DoseGain, 0.0f, MaxDose);
	}

	if (AccumulatedDose > HighDoseDamageThreshold)
	{
		UWildBoundSurvivalComponent* Survival = Owner->FindComponentByClass<UWildBoundSurvivalComponent>();
		if (Survival)
		{
			const float DamageRange = FMath::Max(MaxDose - HighDoseDamageThreshold, 1.0f);
			const float DamageScale = FMath::Clamp((AccumulatedDose - HighDoseDamageThreshold) / DamageRange, 0.0f, 1.0f);
			Survival->ApplySurvivalDamage(HighDoseDamagePerSecond * DamageScale * DeltaTime);
		}
	}

	UpdateGeigerAudio(DeltaTime);
}

void UWildBoundRadiationComponent::InitializeGeigerAudio()
{
	if (GeigerAudioComponent || GeigerSoundWave)
	{
		return;
	}

	AActor* Owner = GetOwner();
	if (!Owner)
	{
		return;
	}

	GeigerSoundWave = NewObject<USoundWaveProcedural>(this, TEXT("WildBoundGeigerProceduralWave"));
	if (!GeigerSoundWave)
	{
		return;
	}

	GeigerSoundWave->NumChannels = 1;
	GeigerSoundWave->SetSampleRate(GeigerSampleRate);
	GeigerSoundWave->Duration = INDEFINITELY_LOOPING_DURATION;
	GeigerSoundWave->bLooping = false;
	GeigerSoundWave->SoundGroup = SOUNDGROUP_Default;

	GeigerAudioComponent = NewObject<UAudioComponent>(Owner, TEXT("WildBoundGeigerAudio"));
	if (!GeigerAudioComponent)
	{
		GeigerSoundWave = nullptr;
		return;
	}

	Owner->AddInstanceComponent(GeigerAudioComponent);
	GeigerAudioComponent->bAutoActivate = false;
	GeigerAudioComponent->SetSound(GeigerSoundWave);
	GeigerAudioComponent->SetVolumeMultiplier(GeigerVolume);
	GeigerAudioComponent->RegisterComponent();

	QueueGeigerAudioFrame(false, 0.0f);
	QueueGeigerAudioFrame(false, 0.0f);
	GeigerAudioComponent->Play();

	UE_LOG(LogTemp, Log, TEXT("WildBound radiation: procedural Geiger audio initialized."));
}

void UWildBoundRadiationComponent::UpdateGeigerAudio(float DeltaTime)
{
	if (!GeigerAudioComponent || !GeigerSoundWave)
	{
		InitializeGeigerAudio();
		if (!GeigerAudioComponent || !GeigerSoundWave)
		{
			return;
		}
	}

	GeigerAudioComponent->SetVolumeMultiplier(GeigerVolume);

	const float ExposurePercent = FMath::Clamp(CurrentExposure, 0.0f, 100.0f);
	if (ExposurePercent < 0.5f)
	{
		SecondsUntilNextGeigerClick = 0.0f;
		QueueGeigerAudioFrame(false, 0.0f);
		return;
	}

	SecondsUntilNextGeigerClick -= DeltaTime;
	bool bEmitClick = false;
	if (SecondsUntilNextGeigerClick <= 0.0f)
	{
		bEmitClick = true;
		const float BaseInterval = GetGeigerClickInterval(ExposurePercent);
		SecondsUntilNextGeigerClick = BaseInterval * FMath::FRandRange(0.82f, 1.18f);
	}

	QueueGeigerAudioFrame(bEmitClick, ExposurePercent);
}

float UWildBoundRadiationComponent::GetGeigerClickInterval(float ExposurePercent) const
{
	const float ExposureAlpha = FMath::Clamp(ExposurePercent / 100.0f, 0.0f, 1.0f);
	const float ShapedExposure = FMath::Pow(ExposureAlpha, 1.20f);
	const float ClicksPerSecond = FMath::Lerp(0.70f, 14.0f, ShapedExposure);
	return 1.0f / FMath::Max(ClicksPerSecond, 0.1f);
}

void UWildBoundRadiationComponent::QueueGeigerAudioFrame(bool bEmitClick, float ExposurePercent)
{
	if (!GeigerSoundWave)
	{
		return;
	}

	const int32 FrameSampleCount = FMath::RoundToInt(GeigerSampleRate * GeigerFrameSeconds);
	TArray<int16> Samples;
	Samples.SetNumZeroed(FrameSampleCount);

	if (bEmitClick)
	{
		const float ExposureAlpha = FMath::Clamp(ExposurePercent / 100.0f, 0.0f, 1.0f);
		const float ClickAmplitude = FMath::Lerp(0.24f, 0.82f, FMath::Pow(ExposureAlpha, 0.65f));
		const float ClickFrequency = FMath::Lerp(1900.0f, 3250.0f, ExposureAlpha);
		const int32 ClickStartSample = 45;
		const int32 ClickSampleCount = 145;

		for (int32 ClickSample = 0; ClickSample < ClickSampleCount; ++ClickSample)
		{
			const int32 SampleIndex = ClickStartSample + ClickSample;
			if (!Samples.IsValidIndex(SampleIndex))
			{
				break;
			}

			const float TimeSeconds = static_cast<float>(ClickSample) / static_cast<float>(GeigerSampleRate);
			const float Envelope = FMath::Exp(-1500.0f * TimeSeconds);
			const float Ring = FMath::Sin(2.0f * PI * ClickFrequency * TimeSeconds);
			const float Noise = FMath::FRandRange(-1.0f, 1.0f);
			float Signal = (Ring * 0.34f + Noise * 0.66f) * Envelope * ClickAmplitude;

			if (ClickSample < 4)
			{
				Signal += (1.0f - static_cast<float>(ClickSample) / 4.0f) * 0.32f * ClickAmplitude;
			}

			const int32 PCMValue = FMath::Clamp(FMath::RoundToInt(Signal * 32767.0f), -32768, 32767);
			Samples[SampleIndex] = static_cast<int16>(PCMValue);
		}
	}

	GeigerSoundWave->QueueAudio(
		reinterpret_cast<const uint8*>(Samples.GetData()),
		Samples.Num() * sizeof(int16));
}

float UWildBoundRadiationComponent::GetExposurePercent() const
{
	return FMath::Clamp(CurrentExposure / 100.0f, 0.0f, 1.0f);
}

float UWildBoundRadiationComponent::GetDosePercent() const
{
	return MaxDose > 0.0f ? FMath::Clamp(AccumulatedDose / MaxDose, 0.0f, 1.0f) : 0.0f;
}

void UWildBoundRadiationComponent::ReduceDose(float Amount)
{
	if (Amount <= 0.0f)
	{
		return;
	}
	AccumulatedDose = FMath::Clamp(AccumulatedDose - Amount, 0.0f, MaxDose);
}

void UWildBoundRadiationComponent::ApplyTreatment(float DoseReduction)
{
	ReduceDose(DoseReduction);
	TreatmentProtectionRemaining = FMath::Max(TreatmentProtectionRemaining, PostTreatmentProtectionDuration);
}

void UWildBoundRadiationComponent::SetDoseIntakeMultiplier(float Multiplier)
{
	DoseIntakeMultiplier = FMath::Clamp(Multiplier, 0.05f, 1.0f);
}

float UWildBoundRadiationComponent::GetEffectiveDoseIntakeMultiplier() const
{
	const float TreatmentMultiplier = TreatmentProtectionRemaining > 0.0f
		? PostTreatmentDoseMultiplier
		: 1.0f;
	return FMath::Clamp(DoseIntakeMultiplier * TreatmentMultiplier, 0.05f, 1.0f);
}

float UWildBoundRadiationComponent::GetDoseProtectionPercent() const
{
	return FMath::Clamp(1.0f - GetEffectiveDoseIntakeMultiplier(), 0.0f, 0.95f);
}

float UWildBoundRadiationComponent::GetCurrentDoseRatePerSecond() const
{
	const float ExposureAlpha = FMath::Clamp(CurrentExposure / 100.0f, 0.0f, 1.0f);
	return ExposureAlpha * FullExposureDosePerSecond * GetEffectiveDoseIntakeMultiplier();
}

float UWildBoundRadiationComponent::GetDosePenaltySeverity(float ThresholdDose) const
{
	const float SafeMaxDose = FMath::Max(MaxDose, 1.0f);
	const float ClampedThreshold = FMath::Clamp(ThresholdDose, 0.0f, SafeMaxDose - KINDA_SMALL_NUMBER);
	const float Range = FMath::Max(SafeMaxDose - ClampedThreshold, 1.0f);
	return FMath::Clamp((AccumulatedDose - ClampedThreshold) / Range, 0.0f, 1.0f);
}

float UWildBoundRadiationComponent::GetRadiationStaminaRegenMultiplier() const
{
	const float Severity = GetDosePenaltySeverity(FatigueDoseThreshold);
	return FMath::Lerp(1.0f, MinimumRadiationStaminaRegenMultiplier, Severity);
}

float UWildBoundRadiationComponent::GetRadiationMoveSpeedMultiplier() const
{
	const float Severity = GetDosePenaltySeverity(MovementPenaltyDoseThreshold);
	return FMath::Lerp(1.0f, MinimumRadiationMoveSpeedMultiplier, Severity);
}

float UWildBoundRadiationComponent::GetRadiationSprintDrainMultiplier() const
{
	const float Severity = GetDosePenaltySeverity(FatigueDoseThreshold);
	return FMath::Lerp(1.0f, MaximumRadiationSprintDrainMultiplier, Severity);
}
