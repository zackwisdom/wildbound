#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "WildBoundInteractionComponent.generated.h"

class AActor;
class SWidget;
class SWildBoundLootWidget;
class UWildBoundInventoryComponent;

struct FWildBoundContainerLootEntry
{
	FName ItemId = NAME_None;
	int32 Quantity = 0;
};

UCLASS(ClassGroup=(WildBound), meta=(BlueprintSpawnableComponent))
class WILDBOUND_API UWildBoundInteractionComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UWildBoundInteractionComponent();

	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	UFUNCTION(BlueprintPure, Category="WildBound|Hotbar")
	int32 GetSelectedHotbarSlot() const { return SelectedHotbarSlot; }

	void SetContextPrompt(const FString& Prompt, int32 Priority = 0);
	FString GetContextPrompt() const;

	bool IsLootWindowOpen() const { return bLootWindowOpen; }
	const TArray<FWildBoundContainerLootEntry>* GetOpenContainerLoot() const;
	UWildBoundInventoryComponent* GetInventoryComponent() const;
	FString GetOpenContainerName() const;
	FString GetOpenContainerQualityName() const;
	int32 GetOpenContainerQualityTier() const;
	bool TakeLootEntry(int32 EntryIndex, bool bTakeWholeStack = true);
	void TakeAllContainerLoot();
	void CloseLootWindow();

private:
	float InteractionDistance = 450.0f;
	int32 SelectedHotbarSlot = 0;
	FString ContextPrompt;
	float ContextPromptExpiresAt = -1.0f;
	int32 ContextPromptPriority = MIN_int32;

	TMap<TWeakObjectPtr<AActor>, TArray<FWildBoundContainerLootEntry>> ContainerLootByActor;
	TWeakObjectPtr<AActor> OpenContainerActor;
	TSharedPtr<SWildBoundLootWidget> LootWidget;
	TSharedPtr<SWidget> LootViewportRoot;
	bool bLootWindowOpen = false;

	void TryInteract(AActor* TargetActor);
	void TryPryTarget(AActor* TargetActor);
	void SearchLootContainer(AActor* TargetActor);
	void GenerateContainerLoot(AActor& TargetActor, TArray<FWildBoundContainerLootEntry>& OutLoot);
	void OpenLootWindow(AActor& TargetActor);
	void EnsureLootWidget();
	void RemoveLootWidget();
	void RefreshContainerInteractableState(AActor* TargetActor);
	FString GetInteractionPrompt(const AActor* TargetActor) const;
	void DestroyInteractionGroup(const FName& GroupTag);
	void HandleHotbarSelection(class APlayerController& PlayerController);
	void TryUseSelectedHotbarItem();
	void TryUseInventoryItem(FName ItemId);
};
