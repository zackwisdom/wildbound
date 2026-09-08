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

	void SetContextPrompt(const FString& Prompt, int32 Priority = 0);
	FString GetContextPrompt() const;

private:
	float InteractionDistance = 450.0f;
	int32 SelectedHotbarSlot = 0;
	FString ContextPrompt;
	float ContextPromptExpiresAt = -1.0f;
	int32 ContextPromptPriority = MIN_int32;

	void TryInteract(AActor* TargetActor);
	void TryPryTarget(AActor* TargetActor);
	void SearchLootContainer(AActor* TargetActor);
	FString GetInteractionPrompt(const AActor* TargetActor) const;
	void DestroyInteractionGroup(const FName& GroupTag);
	void HandleHotbarSelection(class APlayerController& PlayerController);
	void TryUseSelectedHotbarItem();
	void TryUseInventoryItem(FName ItemId);
};
