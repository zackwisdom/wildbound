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

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category="WildBound|Inventory")
	TArray<FWildBoundInventoryStack> Stacks;

	UFUNCTION(BlueprintCallable, Category="WildBound|Inventory")
	bool AddItem(FName ItemId, int32 Quantity);

	UFUNCTION(BlueprintCallable, Category="WildBound|Inventory")
	bool RemoveItem(FName ItemId, int32 Quantity);

	UFUNCTION(BlueprintPure, Category="WildBound|Inventory")
	int32 GetItemCount(FName ItemId) const;

	UFUNCTION(BlueprintPure, Category="WildBound|Inventory")
	bool HasItem(FName ItemId, int32 Quantity = 1) const;

	UFUNCTION(BlueprintCallable, Category="WildBound|Inventory")
	void ClearInventory();
};
