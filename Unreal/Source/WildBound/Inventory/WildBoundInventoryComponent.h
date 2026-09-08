#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "WildBoundInventoryComponent.generated.h"

USTRUCT(BlueprintType)
struct FWildBoundInventoryStack
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="WildBound|Inventory")
	FName ItemId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="WildBound|Inventory", meta=(ClampMin="0"))
	int32 Quantity = 0;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FWildBoundInventoryChanged);

UCLASS(ClassGroup=(WildBound), meta=(BlueprintSpawnableComponent))
class WILDBOUND_API UWildBoundInventoryComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UWildBoundInventoryComponent();

	UPROPERTY(BlueprintAssignable, Category="WildBound|Inventory")
	FWildBoundInventoryChanged OnInventoryChanged;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="WildBound|Inventory", meta=(ClampMin="1"))
	int32 MaxSlots = 24;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="WildBound|Inventory", meta=(ClampMin="1"))
	int32 DefaultMaxStackSize = 99;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="WildBound|Inventory|Weight", meta=(ClampMin="1.0"))
	float MaxCarryWeight = 32.0f;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category="WildBound|Inventory")
	TArray<FWildBoundInventoryStack> Stacks;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category="WildBound|Inventory|Hotbar")
	TArray<FName> HotbarSlots;

	UFUNCTION(BlueprintCallable, Category="WildBound|Inventory")
	bool AddItem(FName ItemId, int32 Quantity);

	UFUNCTION(BlueprintCallable, Category="WildBound|Inventory")
	bool RemoveItem(FName ItemId, int32 Quantity);

	UFUNCTION(BlueprintCallable, Category="WildBound|Inventory")
	bool RemoveFromStack(int32 StackIndex, int32 Quantity);

	UFUNCTION(BlueprintPure, Category="WildBound|Inventory")
	int32 GetItemCount(FName ItemId) const;

	UFUNCTION(BlueprintPure, Category="WildBound|Inventory")
	bool HasItem(FName ItemId, int32 Quantity = 1) const;

	UFUNCTION(BlueprintCallable, Category="WildBound|Inventory")
	void ClearInventory();

	UFUNCTION(BlueprintCallable, Category="WildBound|Inventory")
	bool MoveStack(int32 SourceIndex, int32 TargetIndex);

	UFUNCTION(BlueprintCallable, Category="WildBound|Inventory")
	bool SplitStack(int32 StackIndex, int32 SplitQuantity);

	UFUNCTION(BlueprintPure, Category="WildBound|Inventory|Weight")
	float GetItemUnitWeight(FName ItemId) const;

	UFUNCTION(BlueprintPure, Category="WildBound|Inventory|Weight")
	float GetTotalWeight() const;

	UFUNCTION(BlueprintPure, Category="WildBound|Inventory|Weight")
	float GetCarryWeightRatio() const;

	UFUNCTION(BlueprintPure, Category="WildBound|Inventory|Weight")
	bool IsOverEncumbered() const;

	UFUNCTION(BlueprintPure, Category="WildBound|Inventory")
	FString GetItemDisplayName(FName ItemId) const;

	UFUNCTION(BlueprintPure, Category="WildBound|Inventory")
	FString GetItemCategoryName(FName ItemId) const;

	UFUNCTION(BlueprintPure, Category="WildBound|Inventory")
	FString GetItemDescription(FName ItemId) const;

	UFUNCTION(BlueprintPure, Category="WildBound|Inventory|Rarity")
	int32 GetItemRarityTier(FName ItemId) const;

	UFUNCTION(BlueprintPure, Category="WildBound|Inventory|Rarity")
	FString GetItemRarityName(FName ItemId) const;

	UFUNCTION(BlueprintPure, Category="WildBound|Inventory|Hotbar")
	int32 GetHotbarSlotCount() const { return HotbarSlots.Num(); }

	UFUNCTION(BlueprintPure, Category="WildBound|Inventory|Hotbar")
	FName GetHotbarItemId(int32 SlotIndex) const;

	UFUNCTION(BlueprintCallable, Category="WildBound|Inventory|Hotbar")
	bool AssignHotbarSlot(int32 SlotIndex, FName ItemId);

	UFUNCTION(BlueprintCallable, Category="WildBound|Inventory|Hotbar")
	void ClearHotbarSlot(int32 SlotIndex);

	UFUNCTION(BlueprintCallable, Category="WildBound|Inventory|Hotbar")
	void ClearItemFromHotbar(FName ItemId);

	bool IsItemInHotbar(FName ItemId, int32& OutSlotIndex) const;
};
