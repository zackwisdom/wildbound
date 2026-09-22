#include "WildBoundSurvivalComponent.h"

#include "../Inventory/WildBoundInventoryComponent.h"
#include "WildBoundRadiationComponent.h"
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
	LastDamageCause = NAME_None;
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
	if (Owner)
	{
		const UWildBoundRadiationComponent* Radiation = Owner->FindComponentByClass<UWildBoundRadiationComponent>();
		if (Radiation)
		{
			RegenMultiplier *= Radiation->GetRadiationStaminaRegenMultiplier();
		}
	}
	Stamina = FMath::Clamp(
		Stamina + (StaminaRegenPerSecond * RegenMultiplier * DeltaTime),
		0.0f,
		MaxStamina);

	const float HungerDamageSeverity = GetCriticalDamageSeverity(GetHungerPercent());
	const float ThirstDamageSeverity = GetCriticalDamageSeverity(GetThirstPercent());
	const float StarvationDamage = StarvationDamagePerSecond * FMath::Square(HungerDamageSeverity) * DeltaTime;
	const float DehydrationDamage = DehydrationDamagePerSecond * FMath::Square(ThirstDamageSeverity) * DeltaTime;
	const float SurvivalDamage = StarvationDamage + DehydrationDamage;

	if (SurvivalDamage > 0.0f)
	{
		ApplySurvivalDamageFromCause(
			SurvivalDamage,
			StarvationDamage > DehydrationDamage ? FName(TEXT("Starvation")) : FName(TEXT("Dehydration")));
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
	ApplySurvivalDamageFromCause(Amount, FName(TEXT("CriticalInjuries")));
}

void UWildBoundSurvivalComponent::ApplySurvivalDamageFromCause(float Amount, FName DamageCause)
{
	if (Amount <= 0.0f || !IsAlive())
	{
		return;
	}

	LastDamageCause = DamageCause.IsNone() ? FName(TEXT("CriticalInjuries")) : DamageCause;
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


FString UWildBoundSurvivalComponent::GetDeathCauseText() const
{
	if (LastDamageCause == FName(TEXT("Starvation"))) return TEXT("STARVATION");
	if (LastDamageCause == FName(TEXT("Dehydration"))) return TEXT("DEHYDRATION");
	if (LastDamageCause == FName(TEXT("Bleeding"))) return TEXT("BLOOD LOSS");
	if (LastDamageCause == FName(TEXT("Radiation"))) return TEXT("RADIATION SICKNESS");
	if (LastDamageCause == FName(TEXT("Fall"))) return TEXT("TRAUMATIC FALL");
	return TEXT("CRITICAL INJURIES");
}

void UWildBoundSurvivalComponent::RestorePersistentVitals(
	float SavedHealth,
	float SavedHunger,
	float SavedThirst,
	float SavedStamina)
{
	Health = FMath::Clamp(SavedHealth, 0.0f, MaxHealth);
	Hunger = FMath::Clamp(SavedHunger, 0.0f, MaxHunger);
	Thirst = FMath::Clamp(SavedThirst, 0.0f, MaxThirst);
	Stamina = FMath::Clamp(SavedStamina, 0.0f, MaxStamina);
	bDeathBroadcast = Health <= 0.0f;
	LastDamageCause = Health <= 0.0f ? FName(TEXT("CriticalInjuries")) : NAME_None;
	BroadcastStatsChanged();
}
