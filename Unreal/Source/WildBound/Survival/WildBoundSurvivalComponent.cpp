#include "WildBoundSurvivalComponent.h"

#include "../Inventory/WildBoundInventoryComponent.h"
#include "GameFramework/Actor.h"

UWildBoundSurvivalComponent::UWildBoundSurvivalComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
}

void UWildBoundSurvivalComponent::BeginPlay()
{
	Super::BeginPlay();

	Health = MaxHealth;
	Hunger = MaxHunger;
	Thirst = MaxThirst;
	Stamina = MaxStamina;
	bDeathBroadcast = false;
	BroadcastStatsChanged();
}

void UWildBoundSurvivalComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (!IsAlive())
	{
		return;
	}

	const float PreviousHealth = Health;
	const float PreviousHunger = Hunger;
	const float PreviousThirst = Thirst;
	const float PreviousStamina = Stamina;

	Hunger = FMath::Clamp(Hunger - (HungerDrainPerSecond * DeltaTime), 0.0f, MaxHunger);
	Thirst = FMath::Clamp(Thirst - (ThirstDrainPerSecond * DeltaTime), 0.0f, MaxThirst);

	const AActor* Owner = GetOwner();
	const float Speed2D = Owner ? Owner->GetVelocity().Size2D() : StationaryVelocityThreshold + 1.0f;
	const bool bCompletelyStill = Speed2D <= StationaryVelocityThreshold;

	float RegenMultiplier = bCompletelyStill ? StationaryStaminaRegenMultiplier : 1.0f;
	if (!bCompletelyStill && Owner)
	{
		const UWildBoundInventoryComponent* Inventory = Owner->FindComponentByClass<UWildBoundInventoryComponent>();
		if (Inventory && Inventory->IsOverEncumbered())
		{
			RegenMultiplier *= EncumberedMovingStaminaRegenMultiplier;
		}
	}

	RegenMultiplier *= GetNutritionStaminaRegenMultiplier();
	Stamina = FMath::Clamp(
		Stamina + (StaminaRegenPerSecond * RegenMultiplier * DeltaTime),
		0.0f,
		MaxStamina);

	const float HungerDamageSeverity = GetCriticalDamageSeverity(GetHungerPercent());
	const float ThirstDamageSeverity = GetCriticalDamageSeverity(GetThirstPercent());
	const float SurvivalDamage =
		(StarvationDamagePerSecond * FMath::Square(HungerDamageSeverity)
			+ DehydrationDamagePerSecond * FMath::Square(ThirstDamageSeverity)) * DeltaTime;

	if (SurvivalDamage > 0.0f)
	{
		ApplySurvivalDamage(SurvivalDamage);
	}

	if (!FMath::IsNearlyEqual(PreviousHealth, Health)
		|| !FMath::IsNearlyEqual(PreviousHunger, Hunger)
		|| !FMath::IsNearlyEqual(PreviousThirst, Thirst)
		|| !FMath::IsNearlyEqual(PreviousStamina, Stamina))
	{
		BroadcastStatsChanged();
	}
}

bool UWildBoundSurvivalComponent::ConsumeStamina(float Amount)
{
	if (Amount <= 0.0f)
	{
		return true;
	}

	if (Stamina < Amount)
	{
		return false;
	}

	Stamina = FMath::Clamp(Stamina - Amount, 0.0f, MaxStamina);
	BroadcastStatsChanged();
	return true;
}

void UWildBoundSurvivalComponent::AddHunger(float Amount)
{
	Hunger = FMath::Clamp(Hunger + Amount, 0.0f, MaxHunger);
	BroadcastStatsChanged();
}

void UWildBoundSurvivalComponent::AddThirst(float Amount)
{
	Thirst = FMath::Clamp(Thirst + Amount, 0.0f, MaxThirst);
	BroadcastStatsChanged();
}

void UWildBoundSurvivalComponent::RestoreStamina(float Amount)
{
	Stamina = FMath::Clamp(Stamina + Amount, 0.0f, MaxStamina);
	BroadcastStatsChanged();
}

void UWildBoundSurvivalComponent::ApplySurvivalDamage(float Amount)
{
	if (Amount <= 0.0f || !IsAlive())
	{
		return;
	}

	Health = FMath::Clamp(Health - Amount, 0.0f, MaxHealth);
	BroadcastStatsChanged();

	if (Health <= 0.0f && !bDeathBroadcast)
	{
		bDeathBroadcast = true;
		OnPlayerDied.Broadcast();
	}
}

void UWildBoundSurvivalComponent::Heal(float Amount)
{
	if (Amount <= 0.0f || !IsAlive())
	{
		return;
	}

	Health = FMath::Clamp(Health + Amount, 0.0f, MaxHealth);
	BroadcastStatsChanged();
}

float UWildBoundSurvivalComponent::GetHealthPercent() const
{
	return MaxHealth > 0.0f ? Health / MaxHealth : 0.0f;
}

float UWildBoundSurvivalComponent::GetHungerPercent() const
{
	return MaxHunger > 0.0f ? Hunger / MaxHunger : 0.0f;
}

float UWildBoundSurvivalComponent::GetThirstPercent() const
{
	return MaxThirst > 0.0f ? Thirst / MaxThirst : 0.0f;
}

float UWildBoundSurvivalComponent::GetStaminaPercent() const
{
	return MaxStamina > 0.0f ? Stamina / MaxStamina : 0.0f;
}

float UWildBoundSurvivalComponent::GetNutritionSeverity(float Percent) const
{
	const float Threshold = FMath::Max(LowNutritionThreshold, KINDA_SMALL_NUMBER);
	return FMath::Clamp((Threshold - Percent) / Threshold, 0.0f, 1.0f);
}

float UWildBoundSurvivalComponent::GetCriticalDamageSeverity(float Percent) const
{
	const float Threshold = FMath::Max(HealthDamageThreshold, KINDA_SMALL_NUMBER);
	return FMath::Clamp((Threshold - Percent) / Threshold, 0.0f, 1.0f);
}

float UWildBoundSurvivalComponent::GetNutritionStaminaRegenMultiplier() const
{
	const float HungerSeverity = GetNutritionSeverity(GetHungerPercent());
	const float ThirstSeverity = GetNutritionSeverity(GetThirstPercent());
	const float CombinedSeverity = 1.0f - ((1.0f - HungerSeverity) * (1.0f - ThirstSeverity));
	return FMath::Lerp(1.0f, MinimumNutritionStaminaRegenMultiplier, CombinedSeverity);
}

float UWildBoundSurvivalComponent::GetNutritionMoveSpeedMultiplier() const
{
	const float HungerSeverity = GetNutritionSeverity(GetHungerPercent()) * 0.65f;
	const float ThirstSeverity = GetNutritionSeverity(GetThirstPercent());
	const float CombinedSeverity = FMath::Clamp(
		1.0f - ((1.0f - HungerSeverity) * (1.0f - ThirstSeverity)),
		0.0f,
		1.0f);
	return FMath::Lerp(1.0f, MinimumNutritionMoveSpeedMultiplier, CombinedSeverity);
}

float UWildBoundSurvivalComponent::GetNutritionSprintDrainMultiplier() const
{
	const float HungerSeverity = GetNutritionSeverity(GetHungerPercent()) * 0.75f;
	const float ThirstSeverity = GetNutritionSeverity(GetThirstPercent());
	const float CombinedSeverity = FMath::Clamp(
		1.0f - ((1.0f - HungerSeverity) * (1.0f - ThirstSeverity)),
		0.0f,
		1.0f);
	return FMath::Lerp(1.0f, MaximumNutritionSprintDrainMultiplier, CombinedSeverity);
}

bool UWildBoundSurvivalComponent::IsNutritionLow() const
{
	return GetHungerPercent() <= LowNutritionThreshold || GetThirstPercent() <= LowNutritionThreshold;
}

bool UWildBoundSurvivalComponent::IsNutritionCritical() const
{
	return GetHungerPercent() <= CriticalNutritionThreshold || GetThirstPercent() <= CriticalNutritionThreshold;
}

void UWildBoundSurvivalComponent::BroadcastStatsChanged()
{
	OnStatsChanged.Broadcast();
}
