#include "WildBoundInteractionComponent.h"

#include "../Inventory/WildBoundInventoryComponent.h"
#include "WildBoundGearComponent.h"
#include "Engine/Engine.h"
#include "GameFramework/Actor.h"

namespace
{
	const FName UtilityBeltItemId(TEXT("UtilityBelt"));

	FColor GetStorageRarityColor(int32 Tier)
	{
		switch (Tier)
		{
		case 3: return FColor(205, 145, 235);
		case 2: return FColor(120, 175, 235);
		case 1: return FColor(145, 205, 150);
		default: return FColor(205, 220, 190);
		}
	}
}

bool UWildBoundInteractionComponent::StoreInventoryStackInOpenContainer(
	int32 StackIndex,
	bool bStoreWholeStack)
{
	UWildBoundInventoryComponent* Inventory = GetInventoryComponent();
	TArray<FWildBoundContainerLootEntry>* Loot = OpenContainerActor.IsValid()
		? ContainerLootByActor.Find(OpenContainerActor)
		: nullptr;

	if (!Inventory || !Loot || !Inventory->Stacks.IsValidIndex(StackIndex))
	{
		return false;
	}

	const FWildBoundInventoryStack Stack = Inventory->Stacks[StackIndex];
	if (Stack.ItemId.IsNone() || Stack.Quantity <= 0)
	{
		return false;
	}

	const int32 QuantityToStore = bStoreWholeStack ? Stack.Quantity : 1;

	// The Utility Belt grants extra inventory slots while carried. Do not let the
	// player stash their last belt if doing so would leave more stacks than the
	// base backpack can legally hold.
	if (Stack.ItemId == UtilityBeltItemId
		&& Inventory->GetItemCount(UtilityBeltItemId) - QuantityToStore <= 0)
	{
		const UWildBoundGearComponent* Gear = GetOwner()
			? GetOwner()->FindComponentByClass<UWildBoundGearComponent>()
			: nullptr;

		if (Gear)
		{
			const int32 ProjectedStackCount = Inventory->Stacks.Num()
				- (QuantityToStore >= Stack.Quantity ? 1 : 0);
			const int32 BaseSlotCount = FMath::Max(1, Inventory->MaxSlots - Gear->UtilityBeltSlotBonus);

			if (ProjectedStackCount > BaseSlotCount)
			{
				if (GEngine)
				{
					GEngine->AddOnScreenDebugMessage(
						91053,
						2.0f,
						FColor(225, 155, 105),
						TEXT("Free up backpack slots before storing your Utility Belt."));
				}
				return false;
			}
		}
	}

	if (!Inventory->RemoveFromStack(StackIndex, QuantityToStore))
	{
		return false;
	}

	FWildBoundContainerLootEntry* ExistingEntry = Loot->FindByPredicate(
		[&Stack](const FWildBoundContainerLootEntry& Entry)
		{
			return Entry.ItemId == Stack.ItemId;
		});

	if (ExistingEntry)
	{
		ExistingEntry->Quantity += QuantityToStore;
	}
	else
	{
		FWildBoundContainerLootEntry NewEntry;
		NewEntry.ItemId = Stack.ItemId;
		NewEntry.Quantity = QuantityToStore;
		Loot->Add(NewEntry);
	}

	Loot->Sort([Inventory](const FWildBoundContainerLootEntry& A, const FWildBoundContainerLootEntry& B)
	{
		const int32 RarityA = Inventory->GetItemRarityTier(A.ItemId);
		const int32 RarityB = Inventory->GetItemRarityTier(B.ItemId);
		if (RarityA != RarityB)
		{
			return RarityA > RarityB;
		}
		return Inventory->GetItemDisplayName(A.ItemId) < Inventory->GetItemDisplayName(B.ItemId);
	});

	RefreshContainerInteractableState(OpenContainerActor.Get());

	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(
			91053,
			1.7f,
			GetStorageRarityColor(Inventory->GetItemRarityTier(Stack.ItemId)),
			FString::Printf(
				TEXT("Stored [%s] %s x%d"),
				*Inventory->GetItemRarityName(Stack.ItemId),
				*Inventory->GetItemDisplayName(Stack.ItemId),
				QuantityToStore));
	}

	return true;
}
