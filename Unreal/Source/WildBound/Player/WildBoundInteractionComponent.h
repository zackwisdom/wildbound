#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "WildBoundInteractionComponent.generated.h"

UCLASS(ClassGroup=(WildBound), meta=(BlueprintSpawnableComponent))
class WILDBOUND_API UWildBoundInteractionComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UWildBoundInteractionComponent();

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	UFUNCTION(BlueprintPure, Category="WildBound|Hotbar")
	int32 GetSelectedHotbarSlot() const { return SelectedHotbarSlot; }

private:
	float InteractionDistance = 450.0f;
	int32 SelectedHotbarSlot = 0;

	void TryInteract(AActor* TargetActor);
	FString GetInteractionPrompt(const AActor* TargetActor) const;
	void DestroyInteractionGroup(const FName& GroupTag);
	void HandleHotbarSelection(class APlayerController& PlayerController);
	void TryUseSelectedHotbarItem();
	void TryUseInventoryItem(FName ItemId);
};
