#include "WildBoundInjuryComponent.h"

#include "WildBoundSurvivalComponent.h"
#include "../Inventory/WildBoundInventoryComponent.h"
#include "Engine/Engine.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"

namespace
{
	const FName MedicalSuppliesItemId(TEXT("MedicalSupplies"));
	const FName TraumaKitItemId(TEXT("TraumaKit"));
}

UWildBoundInjuryComponent::UWildBoundInjuryComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = true;
}

void UWildBoundInjuryComponent::BeginPlay()
{
	Super::BeginPlay();
	RefreshReferences();
	DetectConsumedMedicalTreatment();

	if (ACharacter* Character = CharacterOwner.Get())
	{
		if (const UCharacterMovementComponent* Movement = Character->GetCharacterMovement())
		{
			bWasFalling = Movement->IsFalling();
		}
	}
}

void UWildBoundInjuryComponent::TickComponent(
	float DeltaTime,
	ELevelTick TickType,
	FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	RefreshReferences();
	DetectConsumedMedicalTreatment();
	UpdateFallTracking();
	UpdateOngoingInjuries(DeltaTime);
}

void UWildBoundInjuryComponent::RefreshReferences()
{
	AActor* Owner = GetOwner();
	if (!Owner)
	{
		return;
	}

	if (!CharacterOwner.IsValid())
	{
		CharacterOwner = Cast<ACharacter>(Owner);
	}
	if (!SurvivalComponent.IsValid())
	{
		SurvivalComponent = Owner->FindComponentByClass<UWildBoundSurvivalComponent>();
	}
	if (!InventoryComponent.IsValid())
	{
		InventoryComponent = Owner->FindComponentByClass<UWildBoundInventoryComponent>();
	}
}

void UWildBoundInjuryComponent::DetectConsumedMedicalTreatment()
{
	const UWildBoundSurvivalComponent* Survival = SurvivalComponent.Get();
	const UWildBoundInventoryComponent* Inventory = InventoryComponent.Get();
	if (!Survival || !Inventory)
	{
		return;
	}

	const float CurrentHealth = Survival->Health;
	const int32 CurrentMedicalSupplies = Inventory->GetItemCount(MedicalSuppliesItemId);
	const int32 CurrentTraumaKits = Inventory->GetItemCount(TraumaKitItemId);

	if (!bTreatmentSnapshotInitialized)
	{
		PreviousObservedHealth = CurrentHealth;
		PreviousMedicalSuppliesCount = CurrentMedicalSupplies;
		PreviousTraumaKitCount = CurrentTraumaKits;
		bTreatmentSnapshotInitialized = true;
		return;
	}

	const float HealthGain = CurrentHealth - PreviousObservedHealth;
	const bool bTraumaKitConsumed = CurrentTraumaKits < PreviousTraumaKitCount;
	const bool bMedicalSuppliesConsumed = CurrentMedicalSupplies < PreviousMedicalSuppliesCount;

	if (HealthGain > 0.5f)
	{
		if (bTraumaKitConsumed)
		{
			TreatWithMedicalSupplies(true);
		}
		else if (bMedicalSuppliesConsumed)
		{
			TreatWithMedicalSupplies(false);
		}
	}

	PreviousObservedHealth = CurrentHealth;
	PreviousMedicalSuppliesCount = CurrentMedicalSupplies;
	PreviousTraumaKitCount = CurrentTraumaKits;
}

void UWildBoundInjuryComponent::UpdateFallTracking()
{
	ACharacter* Character = CharacterOwner.Get();
	const UCharacterMovementComponent* Movement = Character ? Character->GetCharacterMovement() : nullptr;
	if (!Character || !Movement)
	{
		return;
	}

	const bool bIsFalling = Movement->IsFalling();
	if (bIsFalling)
	{
		PeakDownwardFallSpeed = FMath::Max(PeakDownwardFallSpeed, FMath::Max(0.0f, -Character->GetVelocity().Z));
	}
	else if (bWasFalling)
	{
		HandleLandingImpact(PeakDownwardFallSpeed);
		PeakDownwardFallSpeed = 0.0f;
	}

	bWasFalling = bIsFalling;
}

void UWildBoundInjuryComponent::HandleLandingImpact(float ImpactSpeed)
{
	if (ImpactSpeed < FallPainThreshold)
	{
		return;
	}

	const float SafeReferenceRange = FMath::Max(MaximumReferenceFallSpeed - FallPainThreshold, 1.0f);
	const float ImpactSeverity = FMath::Clamp((ImpactSpeed - FallPainThreshold) / SafeReferenceRange, 0.0f, 1.0f);
	const float HealthDamage = FMath::Lerp(2.0f, MaximumFallDamage, FMath::Square(ImpactSeverity));
	const float PainAmount = FMath::Clamp(0.18f + ImpactSeverity * 0.82f, 0.0f, 1.0f);

	float BleedingAmount = 0.0f;
	if (ImpactSpeed >= FallBleedingThreshold)
	{
		const float Range = FMath::Max(MaximumReferenceFallSpeed - FallBleedingThreshold, 1.0f);
		const float Severity = FMath::Clamp((ImpactSpeed - FallBleedingThreshold) / Range, 0.0f, 1.0f);
		BleedingAmount = 0.14f + Severity * 0.72f;
	}

	float FractureAmount = 0.0f;
	if (ImpactSpeed >= FallFractureThreshold)
	{
		const float Range = FMath::Max(MaximumReferenceFallSpeed - FallFractureThreshold, 1.0f);
		const float Severity = FMath::Clamp((ImpactSpeed - FallFractureThreshold) / Range, 0.0f, 1.0f);
		FractureAmount = 0.30f + Severity * 0.70f;
	}

	ApplyTrauma(HealthDamage, BleedingAmount, FractureAmount, PainAmount);

	if (GEngine)
	{
		if (FractureAmount > 0.0f)
		{
			GEngine->AddOnScreenDebugMessage(91320, 3.2f, FColor(235, 110, 85), TEXT("HARD FALL  |  FRACTURE  |  TRAUMA KIT REQUIRED"));
		}
		else if (BleedingAmount > 0.0f)
		{
			GEngine->AddOnScreenDebugMessage(91320, 3.0f, FColor(225, 125, 95), TEXT("HARD FALL  |  BLEEDING  |  MEDICAL SUPPLIES REQUIRED"));
		}
		else
		{
			GEngine->AddOnScreenDebugMessage(91320, 2.4f, FColor(215, 165, 115), TEXT("HARD LANDING  |  PAIN"));
		}
	}
}

void UWildBoundInjuryComponent::UpdateOngoingInjuries(float DeltaTime)
{
	UWildBoundSurvivalComponent* Survival = SurvivalComponent.Get();
	ACharacter* Character = CharacterOwner.Get();
	if (!Survival || !Survival->IsAlive())
	{
		return;
	}

	const bool bMoving = Character && Character->GetVelocity().SizeSquared2D() > FMath::Square(60.0f);
	if (HasBleeding())
	{
		const float MovementMultiplier = bMoving ? MovingBleedingDamageMultiplier : 1.0f;
		Survival->ApplySurvivalDamage(BleedingDamagePerSecond * BleedingSeverity * MovementMultiplier * DeltaTime);
	}

	const float PainFloor = FMath::Clamp(FMath::Max(BleedingSeverity * 0.35f, FractureSeverity * 0.68f), 0.0f, 1.0f);
	PainSeverity = FMath::Max(PainFloor, FMath::Max(0.0f, PainSeverity - PainRecoveryPerSecond * DeltaTime));

	if (bMoving && (HasFracture() || PainSeverity > 0.35f))
	{
		const float InjurySeverity = FMath::Clamp(FMath::Max(FractureSeverity, PainSeverity * 0.65f), 0.0f, 1.0f);
		const float Drain = InjuredMovementStaminaDrainPerSecond * InjurySeverity * DeltaTime;
		if (Drain > 0.0f && Survival->Stamina > 0.0f)
		{
			Survival->ConsumeStamina(FMath::Min(Drain, Survival->Stamina));
		}
	}
}

float UWildBoundInjuryComponent::CombineSeverity(float CurrentSeverity, float AddedSeverity) const
{
	const float Current = FMath::Clamp(CurrentSeverity, 0.0f, 1.0f);
	const float Added = FMath::Clamp(AddedSeverity, 0.0f, 1.0f);
	return FMath::Clamp(1.0f - ((1.0f - Current) * (1.0f - Added)), 0.0f, 1.0f);
}

void UWildBoundInjuryComponent::ApplyTrauma(float HealthDamage, float BleedingAmount, float FractureAmount, float PainAmount)
{
	RefreshReferences();

	if (UWildBoundSurvivalComponent* Survival = SurvivalComponent.Get())
	{
		if (HealthDamage > 0.0f)
		{
			Survival->ApplySurvivalDamage(HealthDamage);
		}
	}

	BleedingSeverity = CombineSeverity(BleedingSeverity, BleedingAmount);
	FractureSeverity = CombineSeverity(FractureSeverity, FractureAmount);
	PainSeverity = CombineSeverity(PainSeverity, PainAmount);
}

bool UWildBoundInjuryComponent::CanUseBasicMedicalTreatment() const
{
	return HasBleeding() || PainSeverity > 0.15f;
}

bool UWildBoundInjuryComponent::CanUseTraumaKit() const
{
	return HasAnyInjury();
}

bool UWildBoundInjuryComponent::TreatWithMedicalSupplies(bool bTraumaKit)
{
	const bool bCanTreat = bTraumaKit ? CanUseTraumaKit() : CanUseBasicMedicalTreatment();
	if (!bCanTreat)
	{
		return false;
	}

	if (HasBleeding())
	{
		BleedingSeverity = 0.0f;
	}

	if (bTraumaKit && HasFracture())
	{
		FractureSeverity = 0.0f;
	}

	const float PainReduction = bTraumaKit ? 0.82f : 0.36f;
	PainSeverity = FMath::Clamp(PainSeverity - PainReduction, 0.0f, 1.0f);
	const float PainFloor = FMath::Clamp(FMath::Max(BleedingSeverity * 0.35f, FractureSeverity * 0.68f), 0.0f, 1.0f);
	PainSeverity = FMath::Max(PainSeverity, PainFloor);
	return true;
}

float UWildBoundInjuryComponent::GetMovementSpeedMultiplier() const
{
	const float FractureMultiplier = FMath::Lerp(1.0f, MinimumFractureMoveSpeedMultiplier, FMath::Clamp(FractureSeverity, 0.0f, 1.0f));
	const float PainMultiplier = FMath::Lerp(1.0f, 0.88f, FMath::Clamp(PainSeverity, 0.0f, 1.0f));
	return FMath::Clamp(FractureMultiplier * PainMultiplier, 0.55f, 1.0f);
}

float UWildBoundInjuryComponent::GetAccelerationMultiplier() const
{
	const float FractureMultiplier = FMath::Lerp(1.0f, MinimumFractureAccelerationMultiplier, FMath::Clamp(FractureSeverity, 0.0f, 1.0f));
	const float PainMultiplier = FMath::Lerp(1.0f, 0.82f, FMath::Clamp(PainSeverity, 0.0f, 1.0f));
	return FMath::Clamp(FractureMultiplier * PainMultiplier, 0.50f, 1.0f);
}

float UWildBoundInjuryComponent::GetSprintDrainMultiplier() const
{
	const float Severity = FMath::Clamp(FMath::Max(FractureSeverity, PainSeverity * 0.75f), 0.0f, 1.0f);
	return FMath::Lerp(1.0f, MaximumInjurySprintDrainMultiplier, Severity);
}

FString UWildBoundInjuryComponent::GetTreatmentRequirementText() const
{
	if (HasFracture() && HasBleeding())
	{
		return TEXT("TRAUMA KIT RECOMMENDED | splint fracture + stop bleeding");
	}
	if (HasFracture())
	{
		return TEXT("TRAUMA KIT REQUIRED | splint fracture");
	}
	if (HasBleeding())
	{
		return TEXT("MEDICAL SUPPLIES REQUIRED | stop bleeding");
	}
	if (PainSeverity > 0.15f)
	{
		return TEXT("MEDICAL SUPPLIES | reduce pain");
	}
	return TEXT("NO INJURY TREATMENT REQUIRED");
}
