#include "WildBoundInventoryComponent.h"

UWildBoundInventoryComponent::UWildBoundInventoryComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

bool UWildBoundInventoryComponent::AddItem(FName ItemId, int32 Quantity)
{
	if (ItemId.IsNone() || Quantity <= 0)
	{
		return false;
	}

	int32 Remaining = Quantity;

	for (FWildBoundInventoryStack& Stack : Stacks)
	{
		if (Stack.ItemId != ItemId || Stack.Quantity >= DefaultMaxStackSize)
		{
			continue;
		}

		const int32 Space = DefaultMaxStackSize - Stack.Quantity;
		const int32 ToAdd = FMath::Min(Space, Remaining);
		Stack.Quantity += ToAdd;
		Remaining -= ToAdd;

		if (Remaining <= 0)
		{
			OnInventoryChanged.Broadcast();
			return true;
		}
	}

	while (Remaining > 0 && Stacks.Num() < MaxSlots)
	{
		FWildBoundInventoryStack NewStack;
		NewStack.ItemId = ItemId;
		NewStack.Quantity = FMath::Min(DefaultMaxStackSize, Remaining);
		Remaining -= NewStack.Quantity;
		Stacks.Add(NewStack);
	}

	if (Remaining != Quantity)
	{
		OnInventoryChanged.Broadcast();
	}

	return Remaining == 0;
}

bool UWildBoundInventoryComponent::RemoveItem(FName ItemId, int32 Quantity)
{
	if (ItemId.IsNone() || Quantity <= 0 || !HasItem(ItemId, Quantity))
	{
		return false;
	}

	int32 Remaining = Quantity;
	for (FWildBoundInventoryStack& Stack : Stacks)
	{
		if (Stack.ItemId != ItemId)
		{
			continue;
		}

		const int32 ToRemove = FMath::Min(Stack.Quantity, Remaining);
		Stack.Quantity -= ToRemove;
		Remaining -= ToRemove;

		if (Remaining <= 0)
		{
			break;
		}
	}

	Stacks.RemoveAll([](const FWildBoundInventoryStack& Stack)
	{
		return Stack.Quantity <= 0 || Stack.ItemId.IsNone();
	});

	OnInventoryChanged.Broadcast();
	return true;
}

int32 UWildBoundInventoryComponent::GetItemCount(FName ItemId) const
{
	int32 Total = 0;
	for (const FWildBoundInventoryStack& Stack : Stacks)
	{
		if (Stack.ItemId == ItemId)
		{
			Total += Stack.Quantity;
		}
	}
	return Total;
}

bool UWildBoundInventoryComponent::HasItem(FName ItemId, int32 Quantity) const
{
	return Quantity <= 0 || GetItemCount(ItemId) >= Quantity;
}

void UWildBoundInventoryComponent::ClearInventory()
{
	if (Stacks.IsEmpty())
	{
		return;
	}

	Stacks.Reset();
	OnInventoryChanged.Broadcast();
}
