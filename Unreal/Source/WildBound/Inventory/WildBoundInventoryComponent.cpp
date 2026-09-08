#include "WildBoundInventoryComponent.h"

namespace
{
	const FName WaterItem(TEXT("Water"));
	const FName FoodItem(TEXT("Food"));
	const FName MedicalItem(TEXT("MedicalSupplies"));
	const FName ScrapItem(TEXT("ScrapMetal"));
	const FName ClothItem(TEXT("Cloth"));
	const FName WoodItem(TEXT("Wood"));
	const FName PlasticItem(TEXT("Plastic"));
	const FName ElectronicsItem(TEXT("Electronics"));
	const FName ChemicalsItem(TEXT("Chemicals"));
	const FName AdhesiveItem(TEXT("Adhesive"));
	const FName WireItem(TEXT("Wire"));
	const FName BatteryItem(TEXT("Battery"));
	const FName MechanicalPartsItem(TEXT("MechanicalParts"));
	const FName FlashlightItem(TEXT("Flashlight"));
	const FName CrowbarItem(TEXT("Crowbar"));
	const FName ReinforcedBackpackItem(TEXT("ReinforcedBackpack"));
	const FName FilterMaskItem(TEXT("FilterMask"));
	const FName CanteenItem(TEXT("Canteen"));
	const FName TraumaKitItem(TEXT("TraumaKit"));
	const FName RadTreatmentItem(TEXT("RadTreatment"));
	const FName UtilityBeltItem(TEXT("UtilityBelt"));
}

UWildBoundInventoryComponent::UWildBoundInventoryComponent()
{
	PrimaryComponentTick.bCanEverTick = false;

	HotbarSlots.SetNum(3);
	HotbarSlots[0] = WaterItem;
	HotbarSlots[1] = FoodItem;
	HotbarSlots[2] = MedicalItem;
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

	if (!HasItem(ItemId, 1))
	{
		for (FName& HotbarItem : HotbarSlots)
		{
			if (HotbarItem == ItemId)
			{
				HotbarItem = NAME_None;
			}
		}
	}

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
	if (Stacks.IsEmpty() && HotbarSlots.IsEmpty())
	{
		return;
	}

	Stacks.Reset();
	for (FName& HotbarItem : HotbarSlots)
	{
		HotbarItem = NAME_None;
	}
	OnInventoryChanged.Broadcast();
}

float UWildBoundInventoryComponent::GetItemUnitWeight(FName ItemId) const
{
	if (ItemId == WaterItem) return 0.75f;
	if (ItemId == FoodItem) return 0.45f;
	if (ItemId == MedicalItem) return 1.20f;
	if (ItemId == ScrapItem) return 0.65f;
	if (ItemId == ClothItem) return 0.15f;
	if (ItemId == WoodItem) return 0.80f;
	if (ItemId == PlasticItem) return 0.25f;
	if (ItemId == ElectronicsItem) return 0.55f;
	if (ItemId == ChemicalsItem) return 0.75f;
	if (ItemId == AdhesiveItem) return 0.35f;
	if (ItemId == WireItem) return 0.25f;
	if (ItemId == BatteryItem) return 0.45f;
	if (ItemId == MechanicalPartsItem) return 0.85f;
	if (ItemId == FlashlightItem) return 0.70f;
	if (ItemId == CrowbarItem) return 2.00f;
	if (ItemId == ReinforcedBackpackItem) return 3.20f;
	if (ItemId == FilterMaskItem) return 0.90f;
	if (ItemId == CanteenItem) return 1.00f;
	if (ItemId == TraumaKitItem) return 1.80f;
	if (ItemId == RadTreatmentItem) return 0.35f;
	if (ItemId == UtilityBeltItem) return 1.40f;

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
	if (ItemId == WaterItem) return TEXT("Bottled Water");
	if (ItemId == FoodItem) return TEXT("Preserved Food");
	if (ItemId == MedicalItem) return TEXT("Medical Supplies");
	if (ItemId == ScrapItem) return TEXT("Scrap Metal");
	if (ItemId == ClothItem) return TEXT("Cloth");
	if (ItemId == WoodItem) return TEXT("Wood");
	if (ItemId == PlasticItem) return TEXT("Plastic");
	if (ItemId == ElectronicsItem) return TEXT("Electronics");
	if (ItemId == ChemicalsItem) return TEXT("Chemicals");
	if (ItemId == AdhesiveItem) return TEXT("Adhesive");
	if (ItemId == WireItem) return TEXT("Wire");
	if (ItemId == BatteryItem) return TEXT("Battery");
	if (ItemId == MechanicalPartsItem) return TEXT("Mechanical Parts");
	if (ItemId == FlashlightItem) return TEXT("Flashlight");
	if (ItemId == CrowbarItem) return TEXT("Crowbar");
	if (ItemId == ReinforcedBackpackItem) return TEXT("Reinforced Backpack");
	if (ItemId == FilterMaskItem) return TEXT("Filter Mask");
	if (ItemId == CanteenItem) return TEXT("Canteen");
	if (ItemId == TraumaKitItem) return TEXT("Field Trauma Kit");
	if (ItemId == RadTreatmentItem) return TEXT("Radiation Treatment");
	if (ItemId == UtilityBeltItem) return TEXT("Utility Belt");
	return ItemId.ToString();
}

FName UWildBoundInventoryComponent::GetHotbarItemId(int32 SlotIndex) const
{
	if (!HotbarSlots.IsValidIndex(SlotIndex))
	{
		return NAME_None;
	}

	const FName ItemId = HotbarSlots[SlotIndex];
	return !ItemId.IsNone() && HasItem(ItemId, 1) ? ItemId : NAME_None;
}

bool UWildBoundInventoryComponent::AssignHotbarSlot(int32 SlotIndex, FName ItemId)
{
	if (!HotbarSlots.IsValidIndex(SlotIndex) || ItemId.IsNone() || !HasItem(ItemId, 1))
	{
		return false;
	}

	for (int32 Index = 0; Index < HotbarSlots.Num(); ++Index)
	{
		if (Index != SlotIndex && HotbarSlots[Index] == ItemId)
		{
			HotbarSlots[Index] = NAME_None;
		}
	}

	HotbarSlots[SlotIndex] = ItemId;
	OnInventoryChanged.Broadcast();
	return true;
}

void UWildBoundInventoryComponent::ClearHotbarSlot(int32 SlotIndex)
{
	if (!HotbarSlots.IsValidIndex(SlotIndex) || HotbarSlots[SlotIndex].IsNone())
	{
		return;
	}

	HotbarSlots[SlotIndex] = NAME_None;
	OnInventoryChanged.Broadcast();
}

void UWildBoundInventoryComponent::ClearItemFromHotbar(FName ItemId)
{
	bool bChanged = false;
	for (FName& HotbarItem : HotbarSlots)
	{
		if (HotbarItem == ItemId)
		{
			HotbarItem = NAME_None;
			bChanged = true;
		}
	}

	if (bChanged)
	{
		OnInventoryChanged.Broadcast();
	}
}

bool UWildBoundInventoryComponent::IsItemInHotbar(FName ItemId, int32& OutSlotIndex) const
{
	OutSlotIndex = INDEX_NONE;
	for (int32 Index = 0; Index < HotbarSlots.Num(); ++Index)
	{
		if (HotbarSlots[Index] == ItemId && HasItem(ItemId, 1))
		{
			OutSlotIndex = Index;
			return true;
		}
	}
	return false;
}
