#include "WildBoundRadiationComponent.h"

#include "WildBoundSurvivalComponent.h"
#include "Engine/World.h"
#include "EngineUtils.h"

namespace
{
	const FName RadiationHotspotTag(TEXT("WBRadiationHotspot"));
}

UWildBoundRadiationComponent::UWildBoundRadiationComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.TickInterval = 0.05f;
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

		const float Distance = FVector::Dist(OwnerLocation, Source->GetActorLocation());
		if (Distance >= SafeOuterRadius)
		{
			continue;
		}

		float Strength = 1.0f;
		if (Distance > InnerRadius)
		{
			const float Alpha = (Distance - InnerRadius) / (SafeOuterRadius - InnerRadius);
			Strength = 1.0f - FMath::Clamp(Alpha, 0.0f, 1.0f);
			Strength = FMath::Pow(Strength, 1.35f);
		}

		TargetExposure = FMath::Max(TargetExposure, Strength * 100.0f);
	}

	CurrentExposure = FMath::FInterpTo(CurrentExposure, TargetExposure, DeltaTime, 6.0f);
	if (CurrentExposure < 0.05f)
	{
		CurrentExposure = 0.0f;
	}

	if (CurrentExposure > 0.0f && AccumulatedDose < MaxDose)
	{
		const float DoseGain = (CurrentExposure / 100.0f) * FullExposureDosePerSecond * DeltaTime;
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
}

float UWildBoundRadiationComponent::GetExposurePercent() const
{
	return FMath::Clamp(CurrentExposure / 100.0f, 0.0f, 1.0f);
}

float UWildBoundRadiationComponent::GetDosePercent() const
{
	return MaxDose > 0.0f ? FMath::Clamp(AccumulatedDose / MaxDose, 0.0f, 1.0f) : 0.0f;
}
