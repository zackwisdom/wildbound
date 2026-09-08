#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "WildBoundBackpackComponent.generated.h"

class AActor;
class APlayerController;
class SWidget;
class SWildBoundBackpackWidget;
class UWildBoundInventoryComponent;

UCLASS(ClassGroup=(WildBound), meta=(BlueprintSpawnableComponent))
class WILDBOUND_API UWildBoundBackpackComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UWildBoundBackpackComponent();

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	UFUNCTION(BlueprintPure, Category="WildBound|Backpack")
	bool IsBackpackOpen() const { return bBackpackOpen; }

	UFUNCTION(BlueprintPure, Category="WildBound|Backpack")
	int32 GetSelectedStackIndex() const { return SelectedStackIndex; }

	UFUNCTION(BlueprintPure, Category="WildBound|Backpack")
	FName GetSelectedItemId() const;

	UFUNCTION(BlueprintPure, Category="WildBound|Backpack")
	int32 GetSelectedItemQuantity() const;

	UWildBoundInventoryComponent* GetInventoryComponent() const { return InventoryComponent.Get(); }

	void SelectStackIndex(int32 StackIndex);
	bool ReorderStackFromMouse(int32 SourceIndex, int32 TargetIndex);
	bool AssignItemToHotbarFromMouse(FName ItemId, int32 SlotIndex);
	void ClearHotbarSlotFromMouse(int32 SlotIndex);
	bool DropStackFromMouse(int32 StackIndex, bool bDropWholeStack = true);

private:
	TWeakObjectPtr<UWildBoundInventoryComponent> InventoryComponent;
	TSharedPtr<SWildBoundBackpackWidget> BackpackWidget;
	TSharedPtr<SWidget> BackpackViewportRoot;
	TSharedPtr<SWidget> EncumbranceViewportRoot;
	bool bBackpackOpen = false;
	int32 SelectedStackIndex = 0;

	void EnsureBackpackWidget();
	void EnsureEncumbranceWarning();
	void ToggleBackpack();
	void SetBackpackOpen(bool bOpen);
	void HandleBackpackInput(APlayerController& PlayerController);
	void MoveSelection(int32 Direction);
	void ClampSelection();
	void AssignSelectedToHotbar(int32 SlotIndex);
	void RemoveSelectedFromHotbar();
	void DropSelectedItem(bool bDropWholeStack);
	AActor* SpawnDroppedItem(FName ItemId, int32 Quantity) const;
	void RemoveBackpackWidget();
};
