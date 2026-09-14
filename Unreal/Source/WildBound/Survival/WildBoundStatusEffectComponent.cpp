#include "WildBoundStatusEffectComponent.h"

#include "WildBoundInjuryComponent.h"
#include "WildBoundRadiationComponent.h"
#include "WildBoundSurvivalComponent.h"
#include "../Inventory/WildBoundInventoryComponent.h"
#include "../Player/WildBoundGearComponent.h"
#include "GameFramework/Actor.h"

namespace
{
	const FName EffectSevereInjury(TEXT("SevereInjury"));
	const FName EffectInjured(TEXT("Injured"));
	const FName EffectBleeding(TEXT("Bleeding"));
	const FName EffectFracture(TEXT("Fracture"));
	const FName EffectPain(TEXT("Pain"));
	const FName EffectStarving(TEXT("Starving"));
	const FName EffectHungry(TEXT("Hungry"));
	const FName EffectDehydrated(TEXT("Dehydrated"));
	const FName EffectThirsty(TEXT("Thirsty"));
	const FName EffectSevereRadiation(TEXT("SevereRadiation"));
	const FName EffectRadiationSickness(TEXT("RadiationSickness"));
	const FName EffectRadiationExposure(TEXT("RadiationExposure"));
	const FName EffectMedicalTreatment(TEXT("MedicalTreatment"));
	const FName EffectRadiationTreatment(TEXT("RadiationTreatment"));
	const FName EffectFilterMask(TEXT("FilterMask"));
	const FName EffectReinforcedBackpack(TEXT("ReinforcedBackpack"));
	const FName EffectUtilityBelt(TEXT("UtilityBelt"));
}

UWildBoundStatusEffectComponent::UWildBoundStatusEffectComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = true;
	PrimaryComponentTick.TickInterval = 0.20f;
}

void UWildBoundStatusEffectComponent::BeginPlay()
{
	Super::BeginPlay();
	RefreshComponentReferences();
	DetectTreatmentEvents();
}

void UWildBoundStatusEffectComponent::TickComponent(
	float DeltaTime,
	ELevelTick TickType,
	FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	RefreshComponentReferences();
	DetectTreatmentEvents();

	MedicalTreatmentRemaining = FMath::Max(0.0f, MedicalTreatmentRemaining - DeltaTime);
	RadiationTreatmentRemaining = FMath::Max(0.0f, RadiationTreatmentRemaining - DeltaTime);
}

void UWildBoundStatusEffectComponent::RefreshComponentReferences()
{
	AActor* Owner = GetOwner();
	if (!Owner)
	{
		return;
	}

	if (!SurvivalComponent.IsValid())
	{
		SurvivalComponent = Owner->FindComponentByClass<UWildBoundSurvivalComponent>();
	}
	if (!RadiationComponent.IsValid())
	{
		RadiationComponent = Owner->FindComponentByClass<UWildBoundRadiationComponent>();
	}
	if (!GearComponent.IsValid())
	{
		GearComponent = Owner->FindComponentByClass<UWildBoundGearComponent>();
	}
	if (!InventoryComponent.IsValid())
	{
		InventoryComponent = Owner->FindComponentByClass<UWildBoundInventoryComponent>();
	}
	if (!InjuryComponent.IsValid())
	{
		InjuryComponent = Owner->FindComponentByClass<UWildBoundInjuryComponent>();
	}
}

void UWildBoundStatusEffectComponent::DetectTreatmentEvents()
{
	const UWildBoundSurvivalComponent* Survival = SurvivalComponent.Get();
	const UWildBoundRadiationComponent* Radiation = RadiationComponent.Get();
	const float CurrentHealth = Survival ? Survival->Health : PreviousHealth;
	const float CurrentRadiationDose = Radiation ? Radiation->AccumulatedDose : PreviousRadiationDose;

	if (!bTreatmentSnapshotInitialized)
	{
		PreviousHealth = CurrentHealth;
		PreviousRadiationDose = CurrentRadiationDose;
		bTreatmentSnapshotInitialized = Survival || Radiation;
		return;
	}

	if (Survival)
	{
		const float HealthGain = CurrentHealth - PreviousHealth;
		if (HealthGain >= 20.0f)
		{
			RegisterMedicalTreatment(HealthGain >= 70.0f ? 60.0f : DefaultMedicalTreatmentDuration);
		}
		PreviousHealth = CurrentHealth;
	}

	if (Radiation)
	{
		const float DoseReduction = PreviousRadiationDose - CurrentRadiationDose;
		if (DoseReduction >= 10.0f)
		{
			RegisterRadiationTreatment();
		}
		PreviousRadiationDose = CurrentRadiationDose;
	}
}

void UWildBoundStatusEffectComponent::RegisterMedicalTreatment(float DurationSeconds)
{
	const float Duration = DurationSeconds > 0.0f ? DurationSeconds : DefaultMedicalTreatmentDuration;
	MedicalTreatmentRemaining = FMath::Max(MedicalTreatmentRemaining, Duration);
}

void UWildBoundStatusEffectComponent::RegisterRadiationTreatment(float DurationSeconds)
{
	const float Duration = DurationSeconds > 0.0f ? DurationSeconds : DefaultRadiationTreatmentDuration;
	RadiationTreatmentRemaining = FMath::Max(RadiationTreatmentRemaining, Duration);
}

void UWildBoundStatusEffectComponent::AddEffect(
	TArray<FWildBoundStatusEffect>& Effects,
	FName EffectId,
	const TCHAR* DisplayName,
	const TCHAR* Detail,
	EWildBoundStatusSeverity Severity,
	bool bBeneficial,
	float RemainingSeconds) const
{
	FWildBoundStatusEffect Effect;
	Effect.EffectId = EffectId;
	Effect.DisplayName = DisplayName;
	Effect.Detail = Detail;
	Effect.Severity = Severity;
	Effect.bBeneficial = bBeneficial;
	Effect.RemainingSeconds = RemainingSeconds;
	Effects.Add(MoveTemp(Effect));
}

TArray<FWildBoundStatusEffect> UWildBoundStatusEffectComponent::GetActiveEffects() const
{
	TArray<FWildBoundStatusEffect> Effects;

	const UWildBoundInjuryComponent* Injuries = InjuryComponent.Get();
	if (Injuries)
	{
		if (Injuries->HasBleeding())
		{
			if (Injuries->BleedingSeverity >= 0.65f)
			{
				AddEffect(Effects, EffectBleeding, TEXT("SEVERE BLEEDING"), TEXT("Rapid blood loss | medical supplies or trauma kit"), EWildBoundStatusSeverity::Critical, false);
			}
			else
			{
				AddEffect(Effects, EffectBleeding, TEXT("BLEEDING"), TEXT("Ongoing blood loss | medical supplies or trauma kit"), EWildBoundStatusSeverity::Warning, false);
			}
		}

		if (Injuries->HasFracture())
		{
			if (Injuries->FractureSeverity >= 0.65f)
			{
				AddEffect(Effects, EffectFracture, TEXT("SEVERE FRACTURE"), TEXT("Movement heavily impaired | trauma kit required"), EWildBoundStatusSeverity::Critical, false);
			}
			else
			{
				AddEffect(Effects, EffectFracture, TEXT("FRACTURE"), TEXT("Movement impaired | trauma kit required"), EWildBoundStatusSeverity::Warning, false);
			}
		}

		if (Injuries->PainSeverity >= 0.72f)
		{
			AddEffect(Effects, EffectPain, TEXT("SEVERE PAIN"), TEXT("Acceleration and stamina compromised"), EWildBoundStatusSeverity::Warning, false);
		}
		else if (Injuries->PainSeverity >= 0.30f)
		{
			AddEffect(Effects, EffectPain, TEXT("PAIN"), TEXT("Physical performance reduced"), EWildBoundStatusSeverity::Notice, false);
		}
	}

	const UWildBoundSurvivalComponent* Survival = SurvivalComponent.Get();
	if (Survival)
	{
		const float HealthPercent = Survival->GetHealthPercent();
		if (HealthPercent <= InjuryCriticalThreshold)
		{
			AddEffect(Effects, EffectSevereInjury, TEXT("CRITICAL CONDITION"), TEXT("Health critically compromised"), EWildBoundStatusSeverity::Critical, false);
		}
		else if (HealthPercent <= InjuryWarningThreshold)
		{
			AddEffect(Effects, EffectInjured, TEXT("WOUNDED"), TEXT("Health below safe operating level"), EWildBoundStatusSeverity::Warning, false);
		}

		const float HungerPercent = Survival->GetHungerPercent();
		if (HungerPercent <= Survival->HealthDamageThreshold)
		{
			AddEffect(Effects, EffectStarving, TEXT("STARVING"), TEXT("Ongoing health damage"), EWildBoundStatusSeverity::Critical, false);
		}
		else if (HungerPercent <= Survival->LowNutritionThreshold)
		{
			AddEffect(Effects, EffectHungry, TEXT("HUNGRY"), TEXT("Stamina recovery reduced"), EWildBoundStatusSeverity::Warning, false);
		}

		const float ThirstPercent = Survival->GetThirstPercent();
		if (ThirstPercent <= Survival->HealthDamageThreshold)
		{
			AddEffect(Effects, EffectDehydrated, TEXT("SEVERE DEHYDRATION"), TEXT("Ongoing health damage"), EWildBoundStatusSeverity::Critical, false);
		}
		else if (ThirstPercent <= Survival->LowNutritionThreshold)
		{
			AddEffect(Effects, EffectThirsty, TEXT("THIRSTY"), TEXT("Movement and stamina impaired"), EWildBoundStatusSeverity::Warning, false);
		}
	}

	const UWildBoundRadiationComponent* Radiation = RadiationComponent.Get();
	if (Radiation)
	{
		const float DosePercent = Radiation->GetDosePercent();
		if (DosePercent >= SevereRadiationThreshold)
		{
			AddEffect(Effects, EffectSevereRadiation, TEXT("SEVERE RADIATION SICKNESS"), TEXT("Radiation dose causing health damage"), EWildBoundStatusSeverity::Critical, false);
		}
		else if (DosePercent >= RadiationSicknessThreshold)
		{
			AddEffect(Effects, EffectRadiationSickness, TEXT("RADIATION SICKNESS"), TEXT("Accumulated radiation dose elevated"), EWildBoundStatusSeverity::Warning, false);
		}
		else if (Radiation->IsExposed())
		{
			AddEffect(Effects, EffectRadiationExposure, TEXT("RADIATION EXPOSURE"), TEXT("Receiving active radiation dose"), EWildBoundStatusSeverity::Notice, false);
		}
	}

	if (MedicalTreatmentRemaining > 0.0f)
	{
		AddEffect(Effects, EffectMedicalTreatment, TEXT("MEDICALLY STABILIZED"), TEXT("Recent medical treatment"), EWildBoundStatusSeverity::Positive, true, MedicalTreatmentRemaining);
	}
	if (RadiationTreatmentRemaining > 0.0f)
	{
		AddEffect(Effects, EffectRadiationTreatment, TEXT("RAD TREATMENT ACTIVE"), TEXT("Recent radiation treatment"), EWildBoundStatusSeverity::Positive, true, RadiationTreatmentRemaining);
	}

	const UWildBoundGearComponent* Gear = GearComponent.Get();
	if (Gear)
	{
		if (Gear->HasFilterMask())
		{
			AddEffect(Effects, EffectFilterMask, TEXT("FILTER MASK"), TEXT("Radiation dose intake reduced"), EWildBoundStatusSeverity::Positive, true);
		}
		if (Gear->HasReinforcedBackpack())
		{
			AddEffect(Effects, EffectReinforcedBackpack, TEXT("REINFORCED PACK"), TEXT("Carry capacity increased"), EWildBoundStatusSeverity::Positive, true);
		}
		if (Gear->HasUtilityBelt())
		{
			AddEffect(Effects, EffectUtilityBelt, TEXT("UTILITY BELT"), TEXT("Inventory slot capacity increased"), EWildBoundStatusSeverity::Positive, true);
		}
	}

	Effects.Sort([](const FWildBoundStatusEffect& A, const FWildBoundStatusEffect& B)
	{
		if (A.Severity != B.Severity)
		{
			return static_cast<uint8>(A.Severity) > static_cast<uint8>(B.Severity);
		}
		if (A.bBeneficial != B.bBeneficial)
		{
			return !A.bBeneficial;
		}
		return A.DisplayName < B.DisplayName;
	});

	return Effects;
}

bool UWildBoundStatusEffectComponent::HasEffect(FName EffectId) const
{
	const TArray<FWildBoundStatusEffect> Effects = GetActiveEffects();
	return Effects.ContainsByPredicate([EffectId](const FWildBoundStatusEffect& Effect)
	{
		return Effect.EffectId == EffectId;
	});
}

float UWildBoundStatusEffectComponent::GetEffectTimeRemaining(FName EffectId) const
{
	const TArray<FWildBoundStatusEffect> Effects = GetActiveEffects();
	for (const FWildBoundStatusEffect& Effect : Effects)
	{
		if (Effect.EffectId == EffectId)
		{
			return Effect.RemainingSeconds;
		}
	}
	return 0.0f;
}

EWildBoundStatusSeverity UWildBoundStatusEffectComponent::GetHighestSeverity() const
{
	const TArray<FWildBoundStatusEffect> Effects = GetActiveEffects();
	for (const FWildBoundStatusEffect& Effect : Effects)
	{
		if (!Effect.bBeneficial)
		{
			return Effect.Severity;
		}
	}
	return Effects.IsEmpty() ? EWildBoundStatusSeverity::Notice : EWildBoundStatusSeverity::Positive;
}

FString UWildBoundStatusEffectComponent::GetCompactStatusText() const
{
	const TArray<FWildBoundStatusEffect> Effects = GetActiveEffects();
	if (Effects.IsEmpty())
	{
		return TEXT("STABLE");
	}

	TArray<FString> Labels;
	for (const FWildBoundStatusEffect& Effect : Effects)
	{
		if (!Effect.bBeneficial)
		{
			Labels.Add(Effect.DisplayName);
			if (Labels.Num() >= 3)
			{
				break;
			}
		}
	}

	if (Labels.IsEmpty())
	{
		for (const FWildBoundStatusEffect& Effect : Effects)
		{
			if (Effect.bBeneficial)
			{
				Labels.Add(Effect.DisplayName);
				if (Labels.Num() >= 2)
				{
					break;
				}
			}
		}
	}

	return Labels.IsEmpty() ? TEXT("STABLE") : FString::Join(Labels, TEXT("  |  "));
}
