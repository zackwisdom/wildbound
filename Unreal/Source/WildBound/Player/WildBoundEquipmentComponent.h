#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "WildBoundEquipmentComponent.generated.h"

class AActor;
class APlayerController;
class UCameraComponent;
class USpotLightComponent;
class UWildBoundInventoryComponent;

UCLASS(ClassGroup=(WildBound), meta=(BlueprintSpawnableComponent))
class WILDBOUND_API UWildBoundEquipmentComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UWildBoundEquipmentComponent();

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	UFUNCTION(BlueprintPure, Category="WildBound|Equipment")
	bool IsFlashlightOn() const { return bFlashlightOn; }

private:
	TWeakObjectPtr<UWildBoundInventoryComponent> InventoryComponent;
	TObjectPtr<USpotLightComponent> FlashlightLight = nullptr;
	bool bFlashlightOn = false;
	float ToolInteractionDistance = 450.0f;

	void EnsureFlashlightLight();
	void ToggleFlashlight();
	void SetFlashlightOn(bool bNewOn);
	void HandleCrowbarInteraction(APlayerController& PlayerController);
	AActor* GetPryTarget(APlayerController& PlayerController) const;
	void PryTarget(AActor& TargetActor);
	void DestroyPryGroup(const FName& GroupTag);
};
