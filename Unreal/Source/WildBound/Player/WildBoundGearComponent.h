#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "WildBoundGearComponent.generated.h"

class UWildBoundInventoryComponent;
class UWildBoundRadiationComponent;

UCLASS(ClassGroup=(WildBound), meta=(BlueprintSpawnableComponent))
class WILDBOUND_API UWildBoundGearComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UWildBoundGearComponent();

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="WildBound|Gear", meta=(ClampMin="0.0"))
	float ReinforcedBackpackCapacityBonus = 12.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="WildBound|Gear", meta=(ClampMin="0.0", ClampMax="1.0"))
	float FilterMaskDoseMultiplier = 0.55f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="WildBound|Gear", meta=(ClampMin="0"))
	int32 UtilityBeltSlotBonus = 4;

	UFUNCTION(BlueprintPure, Category="WildBound|Gear")
	bool HasReinforcedBackpack() const;

	UFUNCTION(BlueprintPure, Category="WildBound|Gear")
	bool HasFilterMask() const;

	UFUNCTION(BlueprintPure, Category="WildBound|Gear")
	bool HasUtilityBelt() const;

private:
	TWeakObjectPtr<UWildBoundInventoryComponent> InventoryComponent;
	TWeakObjectPtr<UWildBoundRadiationComponent> RadiationComponent;

	float BaseCarryWeight = -1.0f;
	float BaseRadiationDosePerSecond = -1.0f;
	int32 BaseMaxSlots = -1;

	void RefreshComponentReferences();
	void ApplyGearEffects();
	void RestoreBaseValues();
};
