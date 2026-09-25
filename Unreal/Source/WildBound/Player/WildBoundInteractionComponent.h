#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "WildBoundInteractionComponent.generated.h"

class AActor;
class SWidget;
class SWildBoundLootWidget;
class UWildBoundInventoryComponent;

USTRUCT()
struct FWildBoundContainerLootEntry
{
	GENERATED_BODY()

	UPROPERTY()
	FName ItemId = NAME_None;

	UPROPERTY()
	int32 Quantity = 0;
};

USTRUCT()
struct FWildBoundPersistentContainerState
{
	GENERATED_BODY()

	UPROPERTY()
	FVector Location = FVector::ZeroVector;

	UPROPERTY()
	bool bSearched = false;

	UPROPERTY()
	bool bPryUnlocked = false;

	UPROPERTY()
	TArray<FWildBoundContainerLootEntry> Loot;
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

	UFUNCTION(BlueprintPure, Category="WildBound|Treatment")
	bool IsTreatmentInProgress() const { return bTreatmentInProgress; }

	UFUNCTION(BlueprintPure, Category="WildBound|Treatment")
	float GetTreatmentProgress() const;

	UFUNCTION(BlueprintPure, Category="WildBound|Treatment")
	FString GetTreatmentLabel() const { return TreatmentActionLabel; }

	UFUNCTION(BlueprintPure, Category="WildBound|Consumable")
	bool IsConsumableActionInProgress() const { return bTreatmentInProgress || bQuickUseInProgress; }

	UFUNCTION(BlueprintPure, Category="WildBound|Consumable")
	float GetConsumableActionProgress() const;

	UFUNCTION(BlueprintPure, Category="WildBound|Consumable")
	FString GetConsumableActionLabel() const;

	UFUNCTION(BlueprintPure, Category="WildBound|Consumable")
	bool IsConsumableFeedbackVisible() const;

	UFUNCTION(BlueprintPure, Category="WildBound|Consumable")
	FString GetConsumableResultText() const { return ConsumableResultText; }

	FLinearColor GetConsumableFeedbackColor() const;

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
	bool StoreInventoryStackInOpenContainer(int32 StackIndex, bool bStoreWholeStack = true);
	bool HasStoredItemsForActor(const AActor* ContainerActor) const;
	void CloseLootWindow();

	void BuildPersistentContainerStates(TArray<FWildBoundPersistentContainerState>& OutStates) const;
	void RestorePersistentContainerStates(const TArray<FWildBoundPersistentContainerState>& SavedStates);

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

	bool bTreatmentInProgress = false;
	FName PendingTreatmentItemId = NAME_None;
	float TreatmentElapsedSeconds = 0.0f;
	float TreatmentDurationSeconds = 0.0f;
	FString TreatmentActionLabel;

	bool bQuickUseInProgress = false;
	FName PendingQuickUseItemId = NAME_None;
	float QuickUseElapsedSeconds = 0.0f;
	float QuickUseDurationSeconds = 0.0f;
	FString QuickUseActionLabel;
	FLinearColor QuickUseActionColor = FLinearColor(0.70f, 0.75f, 0.68f, 1.0f);

	FString ConsumableResultText;
	FLinearColor ConsumableResultColor = FLinearColor(0.70f, 0.75f, 0.68f, 1.0f);
	float ConsumableResultExpiresAt = -1.0f;

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
	void StartQuickUse(FName ItemId);
	void UpdateQuickUse(float DeltaTime, class APlayerController& PlayerController);
	void CompleteQuickUse();
	void CancelQuickUse(bool bShowMessage = true);
	void SetConsumableResult(const FString& Message, const FLinearColor& Color, float DurationSeconds = 2.6f);
	void StartTreatment(FName ItemId);
	void UpdateTreatment(float DeltaTime, class APlayerController& PlayerController);
	void CompleteTreatment();
	void CancelTreatment(bool bShowMessage = true);
	void SetTreatmentInputLock(bool bLocked);
};
