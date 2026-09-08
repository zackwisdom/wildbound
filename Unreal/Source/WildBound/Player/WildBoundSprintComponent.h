#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "WildBoundSprintComponent.generated.h"

class ACharacter;
class UWildBoundInventoryComponent;
class UWildBoundSurvivalComponent;

UCLASS(ClassGroup=(WildBound), meta=(BlueprintSpawnableComponent))
class WILDBOUND_API UWildBoundSprintComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UWildBoundSprintComponent();

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="WildBound|Movement", meta=(ClampMin="1.0"))
	float SprintSpeed = 900.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="WildBound|Movement", meta=(ClampMin="0.0"))
	float SprintStaminaDrainPerSecond = 34.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="WildBound|Movement", meta=(ClampMin="0.0"))
	float MinimumStaminaToStartSprint = 10.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="WildBound|Movement|Encumbrance", meta=(ClampMin="0.1", ClampMax="1.0"))
	float MinimumEncumberedSpeedMultiplier = 0.55f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="WildBound|Movement|Encumbrance", meta=(ClampMin="1.0"))
	float MaximumEncumberedSprintDrainMultiplier = 2.25f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="WildBound|Movement|Encumbrance", meta=(ClampMin="0.0"))
	float EncumberedWalkingStaminaDrainPerSecond = 8.0f;

	UFUNCTION(BlueprintPure, Category="WildBound|Movement")
	bool IsSprinting() const { return bIsSprinting; }

private:
	TWeakObjectPtr<ACharacter> CharacterOwner;
	TWeakObjectPtr<UWildBoundSurvivalComponent> SurvivalComponent;
	TWeakObjectPtr<UWildBoundInventoryComponent> InventoryComponent;
	float BaseWalkSpeed = 600.0f;
	bool bIsSprinting = false;

	float GetEncumbranceSeverity() const;
	float GetCurrentSpeedMultiplier() const;
	void ApplyMovementSpeed();
	void SetSprinting(bool bNewSprinting);
};
