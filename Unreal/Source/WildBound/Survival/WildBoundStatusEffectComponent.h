#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "WildBoundStatusEffectComponent.generated.h"

class UWildBoundGearComponent;
class UWildBoundInventoryComponent;
class UWildBoundRadiationComponent;
class UWildBoundSurvivalComponent;

UENUM(BlueprintType)
enum class EWildBoundStatusSeverity : uint8
{
	Positive,
	Notice,
	Warning,
	Critical
};

USTRUCT(BlueprintType)
struct FWildBoundStatusEffect
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category="WildBound|Status")
	FName EffectId = NAME_None;

	UPROPERTY(BlueprintReadOnly, Category="WildBound|Status")
	FString DisplayName;

	UPROPERTY(BlueprintReadOnly, Category="WildBound|Status")
	FString Detail;

	UPROPERTY(BlueprintReadOnly, Category="WildBound|Status")
	EWildBoundStatusSeverity Severity = EWildBoundStatusSeverity::Notice;

	UPROPERTY(BlueprintReadOnly, Category="WildBound|Status")
	float RemainingSeconds = -1.0f;

	UPROPERTY(BlueprintReadOnly, Category="WildBound|Status")
	bool bBeneficial = false;
};

UCLASS(ClassGroup=(WildBound), meta=(BlueprintSpawnableComponent))
class WILDBOUND_API UWildBoundStatusEffectComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UWildBoundStatusEffectComponent();

	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="WildBound|Status|Thresholds", meta=(ClampMin="0.05", ClampMax="0.95"))
	float InjuryWarningThreshold = 0.55f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="WildBound|Status|Thresholds", meta=(ClampMin="0.01", ClampMax="0.75"))
	float InjuryCriticalThreshold = 0.25f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="WildBound|Status|Thresholds", meta=(ClampMin="0.05", ClampMax="0.95"))
	float RadiationSicknessThreshold = 0.35f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="WildBound|Status|Thresholds", meta=(ClampMin="0.10", ClampMax="1.0"))
	float SevereRadiationThreshold = 0.70f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="WildBound|Status|Treatment", meta=(ClampMin="1.0"))
	float DefaultMedicalTreatmentDuration = 35.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="WildBound|Status|Treatment", meta=(ClampMin="1.0"))
	float DefaultRadiationTreatmentDuration = 60.0f;

	UFUNCTION(BlueprintCallable, Category="WildBound|Status")
	void RegisterMedicalTreatment(float DurationSeconds = -1.0f);

	UFUNCTION(BlueprintCallable, Category="WildBound|Status")
	void RegisterRadiationTreatment(float DurationSeconds = -1.0f);

	UFUNCTION(BlueprintPure, Category="WildBound|Status")
	TArray<FWildBoundStatusEffect> GetActiveEffects() const;

	UFUNCTION(BlueprintPure, Category="WildBound|Status")
	bool HasEffect(FName EffectId) const;

	UFUNCTION(BlueprintPure, Category="WildBound|Status")
	float GetEffectTimeRemaining(FName EffectId) const;

	UFUNCTION(BlueprintPure, Category="WildBound|Status")
	FString GetCompactStatusText() const;

	UFUNCTION(BlueprintPure, Category="WildBound|Status")
	EWildBoundStatusSeverity GetHighestSeverity() const;

private:
	TWeakObjectPtr<UWildBoundSurvivalComponent> SurvivalComponent;
	TWeakObjectPtr<UWildBoundRadiationComponent> RadiationComponent;
	TWeakObjectPtr<UWildBoundGearComponent> GearComponent;
	TWeakObjectPtr<UWildBoundInventoryComponent> InventoryComponent;

	float MedicalTreatmentRemaining = 0.0f;
	float RadiationTreatmentRemaining = 0.0f;
	float PreviousHealth = 0.0f;
	float PreviousRadiationDose = 0.0f;
	bool bTreatmentSnapshotInitialized = false;

	void RefreshComponentReferences();
	void DetectTreatmentEvents();
	void AddEffect(
		TArray<FWildBoundStatusEffect>& Effects,
		FName EffectId,
		const TCHAR* DisplayName,
		const TCHAR* Detail,
		EWildBoundStatusSeverity Severity,
		bool bBeneficial,
		float RemainingSeconds = -1.0f) const;
};
