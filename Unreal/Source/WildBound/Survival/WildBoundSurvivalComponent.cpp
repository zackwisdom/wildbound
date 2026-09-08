#include "WildBoundSurvivalComponent.h"

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
	Stamina = FMath::Clamp(Stamina + (StaminaRegenPerSecond * DeltaTime), 0.0f, MaxStamina);

	float SurvivalDamage = 0.0f;
	if (Hunger <= 0.0f)
	{
		SurvivalDamage += StarvationDamagePerSecond * DeltaTime;
	}
	if (Thirst <= 0.0f)
	{
		SurvivalDamage += DehydrationDamagePerSecond * DeltaTime;
	}
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

void UWildBoundSurvivalComponent::BroadcastStatsChanged()
{
	OnStatsChanged.Broadcast();
}
