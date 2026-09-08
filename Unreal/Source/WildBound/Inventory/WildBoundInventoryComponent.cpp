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

float UWildBoundInventoryComponent::GetItemUnitWeight(FName ItemId) const
{
	if (ItemId == TEXT("Water")) return 0.75f;
	if (ItemId == TEXT("Food")) return 0.45f;
	if (ItemId == TEXT("MedicalSupplies")) return 1.20f;
	if (ItemId == TEXT("ScrapMetal")) return 0.65f;
	if (ItemId == TEXT("Cloth")) return 0.15f;
	if (ItemId == TEXT("Wood")) return 0.80f;
	if (ItemId == TEXT("Plastic")) return 0.25f;
	if (ItemId == TEXT("Electronics")) return 0.55f;
	if (ItemId == TEXT("Chemicals")) return 0.75f;
	if (ItemId == TEXT("Adhesive")) return 0.35f;
	if (ItemId == TEXT("Wire")) return 0.25f;
	if (ItemId == TEXT("Battery")) return 0.45f;
	if (ItemId == TEXT("MechanicalParts")) return 0.85f;
	if (ItemId == TEXT("Flashlight")) return 0.70f;
	if (ItemId == TEXT("Crowbar")) return 2.00f;

	// Unknown future items still carry mass so newly-added loot never bypasses encumbrance.
	return 0.50f;
}

float UWildBoundInventoryComponent::GetTotalWeight() const
{
	float TotalWeight = 0.0f;
	for (const FWildBoundInventoryStack& Stack : Stacks)
	{
		if (!Stack.ItemId.IsNone() && Stack.Quantity > 0)
		{
			TotalWeight += GetItemUnitWeight(Stack.ItemId) * static_cast<float>(Stack.Quantity);
		}
	}
	return TotalWeight;
}

float UWildBoundInventoryComponent::GetCarryWeightRatio() const
{
	return MaxCarryWeight > 0.0f ? GetTotalWeight() / MaxCarryWeight : 0.0f;
}

bool UWildBoundInventoryComponent::IsOverEncumbered() const
{
	return GetTotalWeight() > MaxCarryWeight + KINDA_SMALL_NUMBER;
}

FString UWildBoundInventoryComponent::GetItemDisplayName(FName ItemId) const
{
	if (ItemId == TEXT("Water")) return TEXT("Bottled Water");
	if (ItemId == TEXT("Food")) return TEXT("Preserved Food");
	if (ItemId == TEXT("MedicalSupplies")) return TEXT("Medical Supplies");
	if (ItemId == TEXT("ScrapMetal")) return TEXT("Scrap Metal");
	if (ItemId == TEXT("Cloth")) return TEXT("Cloth");
	if (ItemId == TEXT("Wood")) return TEXT("Wood");
	if (ItemId == TEXT("Plastic")) return TEXT("Plastic");
	if (ItemId == TEXT("Electronics")) return TEXT("Electronics");
	if (ItemId == TEXT("Chemicals")) return TEXT("Chemicals");
	if (ItemId == TEXT("Adhesive")) return TEXT("Adhesive");
	if (ItemId == TEXT("Wire")) return TEXT("Wire");
	if (ItemId == TEXT("Battery")) return TEXT("Battery");
	if (ItemId == TEXT("MechanicalParts")) return TEXT("Mechanical Parts");
	if (ItemId == TEXT("Flashlight")) return TEXT("Flashlight");
	if (ItemId == TEXT("Crowbar")) return TEXT("Crowbar");
	return ItemId.ToString();
}
