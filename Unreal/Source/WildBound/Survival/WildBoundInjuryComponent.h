#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "WildBoundInjuryComponent.generated.h"

class ACharacter;
class UWildBoundSurvivalComponent;

UCLASS(ClassGroup=(WildBound), meta=(BlueprintSpawnableComponent))
class WILDBOUND_API UWildBoundInjuryComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UWildBoundInjuryComponent();

	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="WildBound|Injury|Falls", meta=(ClampMin="100.0"))
	float FallPainThreshold = 900.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="WildBound|Injury|Falls", meta=(ClampMin="100.0"))
	float FallBleedingThreshold = 1180.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="WildBound|Injury|Falls", meta=(ClampMin="100.0"))
	float FallFractureThreshold = 1450.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="WildBound|Injury|Falls", meta=(ClampMin="500.0"))
	float MaximumReferenceFallSpeed = 1900.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="WildBound|Injury|Falls", meta=(ClampMin="0.0"))
	float MaximumFallDamage = 38.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="WildBound|Injury|Bleeding", meta=(ClampMin="0.0"))
	float BleedingDamagePerSecond = 0.80f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="WildBound|Injury|Bleeding", meta=(ClampMin="1.0"))
	float MovingBleedingDamageMultiplier = 1.25f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="WildBound|Injury|Pain", meta=(ClampMin="0.0"))
	float PainRecoveryPerSecond = 0.018f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="WildBound|Injury|Movement", meta=(ClampMin="0.35", ClampMax="1.0"))
	float MinimumFractureMoveSpeedMultiplier = 0.62f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="WildBound|Injury|Movement", meta=(ClampMin="0.35", ClampMax="1.0"))
	float MinimumFractureAccelerationMultiplier = 0.60f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="WildBound|Injury|Movement", meta=(ClampMin="1.0"))
	float MaximumInjurySprintDrainMultiplier = 2.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="WildBound|Injury|Movement", meta=(ClampMin="0.0"))
	float InjuredMovementStaminaDrainPerSecond = 4.0f;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category="WildBound|Injury|State", meta=(ClampMin="0.0", ClampMax="1.0"))
	float BleedingSeverity = 0.0f;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category="WildBound|Injury|State", meta=(ClampMin="0.0", ClampMax="1.0"))
	float FractureSeverity = 0.0f;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category="WildBound|Injury|State", meta=(ClampMin="0.0", ClampMax="1.0"))
	float PainSeverity = 0.0f;

	UFUNCTION(BlueprintCallable, Category="WildBound|Injury")
	void ApplyTrauma(float HealthDamage, float BleedingAmount, float FractureAmount, float PainAmount);

	UFUNCTION(BlueprintCallable, Category="WildBound|Injury")
	bool TreatWithMedicalSupplies(bool bTraumaKit);

	UFUNCTION(BlueprintPure, Category="WildBound|Injury")
	bool HasBleeding() const { return BleedingSeverity > 0.02f; }

	UFUNCTION(BlueprintPure, Category="WildBound|Injury")
	bool HasFracture() const { return FractureSeverity > 0.02f; }

	UFUNCTION(BlueprintPure, Category="WildBound|Injury")
	bool HasPain() const { return PainSeverity > 0.10f; }

	UFUNCTION(BlueprintPure, Category="WildBound|Injury")
	bool HasAnyInjury() const { return HasBleeding() || HasFracture() || HasPain(); }

	UFUNCTION(BlueprintPure, Category="WildBound|Injury")
	bool CanUseBasicMedicalTreatment() const;

	UFUNCTION(BlueprintPure, Category="WildBound|Injury")
	bool CanUseTraumaKit() const;

	UFUNCTION(BlueprintPure, Category="WildBound|Injury")
	float GetMovementSpeedMultiplier() const;

	UFUNCTION(BlueprintPure, Category="WildBound|Injury")
	float GetAccelerationMultiplier() const;

	UFUNCTION(BlueprintPure, Category="WildBound|Injury")
	float GetSprintDrainMultiplier() const;

	UFUNCTION(BlueprintPure, Category="WildBound|Injury")
	FString GetTreatmentRequirementText() const;

private:
	TWeakObjectPtr<ACharacter> CharacterOwner;
	TWeakObjectPtr<UWildBoundSurvivalComponent> SurvivalComponent;
	bool bWasFalling = false;
	float PeakDownwardFallSpeed = 0.0f;

	void RefreshReferences();
	void UpdateFallTracking();
	void HandleLandingImpact(float ImpactSpeed);
	void UpdateOngoingInjuries(float DeltaTime);
	float CombineSeverity(float CurrentSeverity, float AddedSeverity) const;
};
