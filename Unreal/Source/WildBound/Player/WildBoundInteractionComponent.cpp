#include "WildBoundInteractionComponent.h"

#include "../Inventory/WildBoundInventoryComponent.h"
#include "../Survival/WildBoundRadiationComponent.h"
#include "../Survival/WildBoundSurvivalComponent.h"
#include "WildBoundBackpackComponent.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "InputCoreTypes.h"

namespace
{
	const FName InteractableTag(TEXT("WBInteractable"));
	const FName SupplyTag(TEXT("WBTypeSupply"));
	const FName ClueTag(TEXT("WBTypeClue"));
	const FName ContainerTag(TEXT("WBTypeContainer"));
	const FName DroppedItemTag(TEXT("WBTypeDroppedItem"));
	const FName SearchedContainerTag(TEXT("WBContainerSearched"));
	const FName InspectedTag(TEXT("WBInspected"));

	const FName PryLockedTag(TEXT("WBPryLocked"));
	const FName PryContainerTag(TEXT("WBPryContainer"));
	const FName PryAccessTag(TEXT("WBPryAccess"));
	const FName CommercialGateGroupTag(TEXT("WBPryGroupCommercialGate"));

	const FName QualityCommonTag(TEXT("WBLootQualityCommon"));
	const FName QualityUncommonTag(TEXT("WBLootQualityUncommon"));
	const FName QualityRareTag(TEXT("WBLootQualityRare"));
	const FName QualityEpicTag(TEXT("WBLootQualityEpic"));

	const FName WaterTag(TEXT("WBItemWater"));
	const FName MedicalTag(TEXT("WBItemMedical"));
	const FName FoodTag(TEXT("WBItemFood"));
	const FName C17ClueTag(TEXT("WBClueC17"));

	const FName WaterGroupTag(TEXT("WBGroupWater"));
	const FName MedicalGroupTag(TEXT("WBGroupMedical"));
	const FName FoodGroupTag(TEXT("WBGroupFood"));

	const FName WaterItemId(TEXT("Water"));
	const FName FoodItemId(TEXT("Food"));
	const FName MedicalItemId(TEXT("MedicalSupplies"));
	const FName ScrapItemId(TEXT("ScrapMetal"));
	const FName ClothItemId(TEXT("Cloth"));
	const FName WoodItemId(TEXT("Wood"));
	const FName PlasticItemId(TEXT("Plastic"));
	const FName ElectronicsItemId(TEXT("Electronics"));
	const FName ChemicalsItemId(TEXT("Chemicals"));
	const FName AdhesiveItemId(TEXT("Adhesive"));
	const FName WireItemId(TEXT("Wire"));
	const FName BatteryItemId(TEXT("Battery"));
	const FName MechanicalPartsItemId(TEXT("MechanicalParts"));
	const FName FlashlightItemId(TEXT("Flashlight"));
	const FName CrowbarItemId(TEXT("Crowbar"));
	const FName ReinforcedBackpackItemId(TEXT("ReinforcedBackpack"));
	const FName FilterMaskItemId(TEXT("FilterMask"));
	const FName CanteenItemId(TEXT("Canteen"));
	const FName TraumaKitItemId(TEXT("TraumaKit"));
	const FName RadTreatmentItemId(TEXT("RadTreatment"));
	const FName UtilityBeltItemId(TEXT("UtilityBelt"));

	const FName MedicalPoolTag(TEXT("WBLootMedical"));
	const FName MarketPoolTag(TEXT("WBLootMarket"));
	const FName ResidentialPoolTag(TEXT("WBLootResidential"));
	const FName IndustrialPoolTag(TEXT("WBLootIndustrial"));
	const FName CivicPoolTag(TEXT("WBLootCivic"));

	const FName CrateContainerTag(TEXT("WBContainerCrate"));
	const FName CabinetContainerTag(TEXT("WBContainerCabinet"));
	const FName LockerContainerTag(TEXT("WBContainerLocker"));
	const FName DumpsterContainerTag(TEXT("WBContainerDumpster"));
	const FName ToolboxContainerTag(TEXT("WBContainerToolbox"));
	const FName CoolerContainerTag(TEXT("WBContainerCooler"));

	const FString DroppedItemPrefix(TEXT("WBDropItem_"));
	const FString DroppedQuantityPrefix(TEXT("WBDropQty_"));
	constexpr int32 HotbarSlotCount = 3;

	bool ParseDroppedItem(const AActor& Actor, FName& OutItemId, int32& OutQuantity)
	{
		OutItemId = NAME_None;
		OutQuantity = 0;

		for (const FName& Tag : Actor.Tags)
		{
			const FString TagString = Tag.ToString();
			if (TagString.StartsWith(DroppedItemPrefix))
			{
				OutItemId = FName(*TagString.RightChop(DroppedItemPrefix.Len()));
			}
			else if (TagString.StartsWith(DroppedQuantityPrefix))
			{
				OutQuantity = FCString::Atoi(*TagString.RightChop(DroppedQuantityPrefix.Len()));
			}
		}

		return !OutItemId.IsNone() && OutQuantity > 0;
	}

	bool CanInventoryFit(const UWildBoundInventoryComponent& Inventory, FName ItemId, int32 Quantity)
	{
		int32 Capacity = 0;
		for (const FWildBoundInventoryStack& Stack : Inventory.Stacks)
		{
			if (Stack.ItemId == ItemId)
			{
				Capacity += FMath::Max(0, Inventory.DefaultMaxStackSize - Stack.Quantity);
			}
		}
		Capacity += FMath::Max(0, Inventory.MaxSlots - Inventory.Stacks.Num()) * Inventory.DefaultMaxStackSize;
		return Quantity <= Capacity;
	}

	int32 GetContainerQualityTier(const AActor& Container)
	{
		if (Container.ActorHasTag(QualityEpicTag)) return 3;
		if (Container.ActorHasTag(QualityRareTag)) return 2;
		if (Container.ActorHasTag(QualityUncommonTag)) return 1;
		return 0;
	}

	FString GetContainerQualityName(const AActor& Container)
	{
		switch (GetContainerQualityTier(Container))
		{
		case 3: return TEXT("EPIC");
		case 2: return TEXT("RARE");
		case 1: return TEXT("UNCOMMON");
		default: return TEXT("COMMON");
		}
	}

	FColor GetRarityColor(int32 Tier)
	{
		switch (Tier)
		{
		case 3: return FColor(205, 145, 235);
		case 2: return FColor(120, 175, 235);
		case 1: return FColor(145, 205, 150);
		default: return FColor(205, 220, 190);
		}
	}

	FName RollLootItem(const AActor& Container, FRandomStream& Random)
	{
		const int32 Roll = Random.RandRange(0, 99);

		if (Container.ActorHasTag(MedicalPoolTag))
		{
			if (Roll < 30) return MedicalItemId;
			if (Roll < 45) return ChemicalsItemId;
			if (Roll < 60) return ClothItemId;
			if (Roll < 74) return PlasticItemId;
			if (Roll < 84) return WaterItemId;
			if (Roll < 94) return AdhesiveItemId;
			return ElectronicsItemId;
		}

		if (Container.ActorHasTag(MarketPoolTag))
		{
			if (Roll < 34) return FoodItemId;
			if (Roll < 59) return WaterItemId;
			if (Roll < 73) return PlasticItemId;
			if (Roll < 83) return ClothItemId;
			if (Roll < 93) return AdhesiveItemId;
			return BatteryItemId;
		}

		if (Container.ActorHasTag(ResidentialPoolTag))
		{
			if (Roll < 18) return FoodItemId;
			if (Roll < 33) return WaterItemId;
			if (Roll < 48) return ClothItemId;
			if (Roll < 61) return WoodItemId;
			if (Roll < 72) return PlasticItemId;
			if (Roll < 81) return AdhesiveItemId;
			if (Roll < 89) return BatteryItemId;
			if (Roll < 96) return ElectronicsItemId;
			return FlashlightItemId;
		}

		if (Container.ActorHasTag(IndustrialPoolTag))
		{
			if (Roll < 29) return ScrapItemId;
			if (Roll < 49) return MechanicalPartsItemId;
			if (Roll < 64) return WireItemId;
			if (Roll < 75) return ElectronicsItemId;
			if (Roll < 85) return AdhesiveItemId;
			if (Roll < 92) return BatteryItemId;
			if (Roll < 97) return PlasticItemId;
			return CrowbarItemId;
		}

		if (Container.ActorHasTag(CivicPoolTag))
		{
			if (Roll < 25) return ElectronicsItemId;
			if (Roll < 44) return WireItemId;
			if (Roll < 59) return BatteryItemId;
			if (Roll < 73) return PlasticItemId;
			if (Roll < 84) return ClothItemId;
			if (Roll < 94) return AdhesiveItemId;
			return MedicalItemId;
		}

		if (Roll < 18) return ScrapItemId;
		if (Roll < 32) return FoodItemId;
		if (Roll < 45) return WaterItemId;
		if (Roll < 58) return ClothItemId;
		if (Roll < 70) return PlasticItemId;
		if (Roll < 80) return WoodItemId;
		if (Roll < 89) return AdhesiveItemId;
		if (Roll < 96) return WireItemId;
		return BatteryItemId;
	}

	int32 RollLootQuantity(const FName& ItemId, FRandomStream& Random)
	{
		if (ItemId == ScrapItemId) return Random.RandRange(2, 6);
		if (ItemId == ClothItemId) return Random.RandRange(1, 4);
		if (ItemId == WoodItemId) return Random.RandRange(2, 5);
		if (ItemId == PlasticItemId) return Random.RandRange(1, 4);
		if (ItemId == ElectronicsItemId) return Random.RandRange(1, 2);
		if (ItemId == ChemicalsItemId) return Random.RandRange(1, 2);
		if (ItemId == AdhesiveItemId) return Random.RandRange(1, 2);
		if (ItemId == WireItemId) return Random.RandRange(1, 3);
		if (ItemId == BatteryItemId) return Random.RandRange(1, 2);
		if (ItemId == MechanicalPartsItemId) return Random.RandRange(1, 3);
		if (ItemId == WaterItemId) return Random.RandRange(1, 2);
		if (ItemId == FoodItemId) return Random.RandRange(1, 3);
		return 1;
	}

	void AddQualityBonusLoot(const AActor& Container, int32 QualityTier, FRandomStream& Random, TMap<FName, int32>& Grants)
	{
		const float TierBonus = static_cast<float>(QualityTier) * 0.07f;

		if (Container.ActorHasTag(IndustrialPoolTag) && Random.FRand() < 0.10f + TierBonus)
		{
			Grants.FindOrAdd(CrowbarItemId) += 1;
		}
		else if (Container.ActorHasTag(ResidentialPoolTag) && Random.FRand() < 0.08f + TierBonus)
		{
			Grants.FindOrAdd(FlashlightItemId) += 1;
		}

		if (Container.ActorHasTag(MedicalPoolTag) && Random.FRand() < 0.05f + TierBonus)
		{
			Grants.FindOrAdd(RadTreatmentItemId) += 1;
		}
		if (Container.ActorHasTag(MedicalPoolTag) && Random.FRand() < 0.025f + TierBonus * 0.55f)
		{
			Grants.FindOrAdd(TraumaKitItemId) += 1;
		}
		if ((Container.ActorHasTag(ResidentialPoolTag) || Container.ActorHasTag(MarketPoolTag))
			&& Random.FRand() < 0.04f + TierBonus)
		{
			Grants.FindOrAdd(CanteenItemId) += 1;
		}
		if (Container.ActorHasTag(IndustrialPoolTag) && Random.FRand() < 0.02f + TierBonus * 0.60f)
		{
			Grants.FindOrAdd(UtilityBeltItemId) += 1;
		}

		if (QualityTier >= 3)
		{
			if (Container.ActorHasTag(MedicalPoolTag))
			{
				Grants.FindOrAdd(TraumaKitItemId) += 1;
				Grants.FindOrAdd(RadTreatmentItemId) += 2;
			}
			else if (Container.ActorHasTag(IndustrialPoolTag))
			{
				Grants.FindOrAdd(UtilityBeltItemId) += 1;
				Grants.FindOrAdd(CrowbarItemId) += 1;
			}
			else if (Container.ActorHasTag(CivicPoolTag))
			{
				Grants.FindOrAdd(FilterMaskItemId) += 1;
				Grants.FindOrAdd(ElectronicsItemId) += 2;
			}
			else if (Container.ActorHasTag(ResidentialPoolTag) || Container.ActorHasTag(MarketPoolTag))
			{
				Grants.FindOrAdd(CanteenItemId) += 1;
				Grants.FindOrAdd(FlashlightItemId) += 1;
			}
			else
			{
				Grants.FindOrAdd(ReinforcedBackpackItemId) += 1;
			}
		}
	}
}

UWildBoundInteractionComponent::UWildBoundInteractionComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = true;
	PrimaryComponentTick.TickGroup = TG_PostUpdateWork;
}

void UWildBoundInteractionComponent::SetContextPrompt(const FString& Prompt, int32 Priority)
{
	UWorld* World = GetWorld();
	if (!World || Prompt.IsEmpty())
	{
		return;
	}

	const float Now = World->GetTimeSeconds();
	if (Now > ContextPromptExpiresAt || Priority >= ContextPromptPriority)
	{
		ContextPrompt = Prompt;
		ContextPromptPriority = Priority;
		ContextPromptExpiresAt = Now + 0.12f;
	}
}

FString UWildBoundInteractionComponent::GetContextPrompt() const
{
	const UWorld* World = GetWorld();
	if (!World || World->GetTimeSeconds() > ContextPromptExpiresAt)
	{
		return FString();
	}
	return ContextPrompt;
}

void UWildBoundInteractionComponent::TickComponent(
	float DeltaTime,
	ELevelTick TickType,
	FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	APawn* Pawn = Cast<APawn>(GetOwner());
	APlayerController* PlayerController = Pawn ? Cast<APlayerController>(Pawn->GetController()) : nullptr;
	UWorld* World = GetWorld();
	if (!Pawn || !PlayerController || !World)
	{
		return;
	}

	const UWildBoundBackpackComponent* Backpack = Pawn->FindComponentByClass<UWildBoundBackpackComponent>();
	if (Backpack && Backpack->IsBackpackOpen())
	{
		return;
	}

	HandleHotbarSelection(*PlayerController);

	FVector ViewLocation;
	FRotator ViewRotation;
	PlayerController->GetPlayerViewPoint(ViewLocation, ViewRotation);
	const FVector TraceEnd = ViewLocation + ViewRotation.Vector() * InteractionDistance;
	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(WildBoundInteractionTrace), false, Pawn);

	FHitResult Hit;
	const bool bHit = World->LineTraceSingleByChannel(Hit, ViewLocation, TraceEnd, ECC_Visibility, QueryParams);
	AActor* TargetActor = bHit ? Hit.GetActor() : nullptr;
	const bool bHasWorldInteraction = TargetActor && TargetActor->ActorHasTag(InteractableTag);

	if (bHasWorldInteraction)
	{
		SetContextPrompt(FString::Printf(TEXT("[E] %s"), *GetInteractionPrompt(TargetActor)), 20);
		if (PlayerController->WasInputKeyJustPressed(EKeys::E))
		{
			TryInteract(TargetActor);
		}
		return;
	}

	if (PlayerController->WasInputKeyJustPressed(EKeys::E))
	{
		TryUseSelectedHotbarItem();
	}
}

void UWildBoundInteractionComponent::HandleHotbarSelection(APlayerController& PlayerController)
{
	if (PlayerController.WasInputKeyJustPressed(EKeys::One))
	{
		SelectedHotbarSlot = 0;
	}
	else if (PlayerController.WasInputKeyJustPressed(EKeys::Two))
	{
		SelectedHotbarSlot = 1;
	}
	else if (PlayerController.WasInputKeyJustPressed(EKeys::Three))
	{
		SelectedHotbarSlot = 2;
	}
	else if (PlayerController.WasInputKeyJustPressed(EKeys::MouseScrollDown))
	{
		SelectedHotbarSlot = (SelectedHotbarSlot + 1) % HotbarSlotCount;
	}
	else if (PlayerController.WasInputKeyJustPressed(EKeys::MouseScrollUp))
	{
		SelectedHotbarSlot = (SelectedHotbarSlot + HotbarSlotCount - 1) % HotbarSlotCount;
	}
}

void UWildBoundInteractionComponent::TryUseSelectedHotbarItem()
{
	AActor* Owner = GetOwner();
	UWildBoundInventoryComponent* Inventory = Owner ? Owner->FindComponentByClass<UWildBoundInventoryComponent>() : nullptr;
	if (!Inventory)
	{
		return;
	}

	const FName ItemId = Inventory->GetHotbarItemId(SelectedHotbarSlot);
	if (ItemId.IsNone())
	{
		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(91003, 1.5f, FColor(185, 185, 175), TEXT("That hotbar slot is empty."));
		}
		return;
	}

	TryUseInventoryItem(ItemId);
}

void UWildBoundInteractionComponent::TryUseInventoryItem(FName ItemId)
{
	AActor* Owner = GetOwner();
	UWildBoundInventoryComponent* Inventory = Owner ? Owner->FindComponentByClass<UWildBoundInventoryComponent>() : nullptr;
	UWildBoundSurvivalComponent* Survival = Owner ? Owner->FindComponentByClass<UWildBoundSurvivalComponent>() : nullptr;
	UWildBoundRadiationComponent* Radiation = Owner ? Owner->FindComponentByClass<UWildBoundRadiationComponent>() : nullptr;
	if (!Inventory || !Survival || !Inventory->HasItem(ItemId, 1))
	{
		return;
	}

	FString UseMessage;
	FColor MessageColor(205, 220, 190);

	if (ItemId == WaterItemId)
	{
		if (Survival->Thirst >= Survival->MaxThirst - KINDA_SMALL_NUMBER)
		{
			if (GEngine) GEngine->AddOnScreenDebugMessage(91003, 1.8f, FColor(170, 200, 220), TEXT("Thirst is already full."));
			return;
		}
		if (!Inventory->RemoveItem(ItemId, 1)) return;
		const float Restore = Inventory->HasItem(CanteenItemId, 1) ? 45.0f : 35.0f;
		Survival->AddThirst(Restore);
		UseMessage = FString::Printf(TEXT("Drank water  +%.0f THIRST"), Restore);
		MessageColor = FColor(145, 195, 225);
	}
	else if (ItemId == FoodItemId)
	{
		if (Survival->Hunger >= Survival->MaxHunger - KINDA_SMALL_NUMBER)
		{
			if (GEngine) GEngine->AddOnScreenDebugMessage(91003, 1.8f, FColor(215, 185, 120), TEXT("Hunger is already full."));
			return;
		}
		if (!Inventory->RemoveItem(ItemId, 1)) return;
		Survival->AddHunger(30.0f);
		UseMessage = TEXT("Ate preserved ration  +30 HUNGER");
		MessageColor = FColor(215, 185, 120);
	}
	else if (ItemId == MedicalItemId || ItemId == TraumaKitItemId)
	{
		if (Survival->Health >= Survival->MaxHealth - KINDA_SMALL_NUMBER)
		{
			if (GEngine) GEngine->AddOnScreenDebugMessage(91003, 1.8f, FColor(220, 155, 145), TEXT("Health is already full."));
			return;
		}
		if (!Inventory->RemoveItem(ItemId, 1)) return;
		const float HealAmount = ItemId == TraumaKitItemId ? 80.0f : 45.0f;
		Survival->Heal(HealAmount);
		UseMessage = FString::Printf(TEXT("Used medical treatment  +%.0f HEALTH"), HealAmount);
		MessageColor = FColor(220, 155, 145);
	}
	else if (ItemId == RadTreatmentItemId)
	{
		if (!Radiation || Radiation->AccumulatedDose <= KINDA_SMALL_NUMBER)
		{
			if (GEngine) GEngine->AddOnScreenDebugMessage(91003, 1.8f, FColor(205, 190, 145), TEXT("Radiation dose is already clear."));
			return;
		}
		if (!Inventory->RemoveItem(ItemId, 1)) return;
		Radiation->ReduceDose(30.0f);
		UseMessage = TEXT("Radiation treatment used  -30 DOSE");
		MessageColor = FColor(205, 190, 145);
	}
	else
	{
		if (GEngine)
		{
			FString Message = TEXT("This item has no direct hotbar action.");
			if (ItemId == FlashlightItemId) Message = TEXT("Flashlight: press F to toggle.");
			else if (ItemId == CrowbarItemId) Message = TEXT("Crowbar: use it on sealed targets.");
			else if (ItemId == ReinforcedBackpackItemId || ItemId == FilterMaskItemId || ItemId == CanteenItemId || ItemId == UtilityBeltItemId)
			{
				Message = TEXT("Passive gear is active while carried.");
			}
			GEngine->AddOnScreenDebugMessage(91003, 1.8f, FColor(185, 185, 175), Message);
		}
		return;
	}

	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(91003, 2.2f, MessageColor, UseMessage);
	}
}

FString UWildBoundInteractionComponent::GetInteractionPrompt(const AActor* TargetActor) const
{
	if (!TargetActor)
	{
		return TEXT("Interact");
	}

	const UWildBoundInventoryComponent* Inventory = GetOwner() ? GetOwner()->FindComponentByClass<UWildBoundInventoryComponent>() : nullptr;

	if (TargetActor->ActorHasTag(DroppedItemTag))
	{
		FName ItemId;
		int32 Quantity = 0;
		if (ParseDroppedItem(*TargetActor, ItemId, Quantity))
		{
			const FString DisplayName = Inventory ? Inventory->GetItemDisplayName(ItemId) : ItemId.ToString();
			const FString Rarity = Inventory ? Inventory->GetItemRarityName(ItemId) : TEXT("COMMON");
			return FString::Printf(TEXT("Pick up [%s] %s x%d"), *Rarity, *DisplayName, Quantity);
		}
		return TEXT("Pick up item");
	}

	if (TargetActor->ActorHasTag(PryLockedTag))
	{
		const bool bHasCrowbar = Inventory && Inventory->HasItem(CrowbarItemId, 1);
		const bool bAccessGate = TargetActor->ActorHasTag(PryAccessTag);
		if (bAccessGate)
		{
			return bHasCrowbar ? TEXT("Pry open maintenance gate") : TEXT("Locked gate - crowbar required");
		}

		const FString Quality = GetContainerQualityName(*TargetActor);
		return bHasCrowbar
			? FString::Printf(TEXT("Pry open [%s] locked cache"), *Quality)
			: FString::Printf(TEXT("[%s] cache locked - crowbar required"), *Quality);
	}

	if (TargetActor->ActorHasTag(ContainerTag))
	{
		FString ContainerName(TEXT("container"));
		if (TargetActor->ActorHasTag(ToolboxContainerTag)) ContainerName = TEXT("toolbox");
		else if (TargetActor->ActorHasTag(MedicalPoolTag) && TargetActor->ActorHasTag(CabinetContainerTag)) ContainerName = TEXT("medical cabinet");
		else if (TargetActor->ActorHasTag(LockerContainerTag)) ContainerName = TEXT("locker");
		else if (TargetActor->ActorHasTag(DumpsterContainerTag)) ContainerName = TEXT("dumpster");
		else if (TargetActor->ActorHasTag(CoolerContainerTag)) ContainerName = TEXT("cooler");
		else if (TargetActor->ActorHasTag(CabinetContainerTag)) ContainerName = TEXT("cabinet");
		else if (TargetActor->ActorHasTag(CrateContainerTag)) ContainerName = TEXT("crate");

		return FString::Printf(TEXT("[%s] Search %s"), *GetContainerQualityName(*TargetActor), *ContainerName);
	}

	if (TargetActor->ActorHasTag(WaterTag)) return TEXT("Take bottled water");
	if (TargetActor->ActorHasTag(MedicalTag)) return TEXT("Take first-aid kit");
	if (TargetActor->ActorHasTag(FoodTag)) return TEXT("Take preserved food");
	if (TargetActor->ActorHasTag(C17ClueTag))
	{
		return TargetActor->ActorHasTag(InspectedTag)
			? TEXT("Re-read Civil Defense survey")
			: TEXT("Inspect Civil Defense survey");
	}
	return TEXT("Interact");
}

void UWildBoundInteractionComponent::TryInteract(AActor* TargetActor)
{
	if (!TargetActor)
	{
		return;
	}

	if (TargetActor->ActorHasTag(DroppedItemTag))
	{
		UWildBoundInventoryComponent* Inventory = GetOwner() ? GetOwner()->FindComponentByClass<UWildBoundInventoryComponent>() : nullptr;
		FName ItemId;
		int32 Quantity = 0;
		if (!Inventory || !ParseDroppedItem(*TargetActor, ItemId, Quantity))
		{
			return;
		}
		if (!CanInventoryFit(*Inventory, ItemId, Quantity) || !Inventory->AddItem(ItemId, Quantity))
		{
			if (GEngine) GEngine->AddOnScreenDebugMessage(91002, 1.8f, FColor(220, 145, 115), TEXT("Not enough inventory space."));
			return;
		}
		if (GEngine)
		{
			const int32 RarityTier = Inventory->GetItemRarityTier(ItemId);
			GEngine->AddOnScreenDebugMessage(
				91002,
				2.1f,
				GetRarityColor(RarityTier),
				FString::Printf(TEXT("Picked up [%s] %s x%d"), *Inventory->GetItemRarityName(ItemId), *Inventory->GetItemDisplayName(ItemId), Quantity));
		}
		TargetActor->Destroy();
		return;
	}

	if (TargetActor->ActorHasTag(PryLockedTag))
	{
		TryPryTarget(TargetActor);
		return;
	}

	if (TargetActor->ActorHasTag(ContainerTag))
	{
		SearchLootContainer(TargetActor);
		return;
	}

	if (TargetActor->ActorHasTag(SupplyTag))
	{
		UWildBoundInventoryComponent* Inventory = GetOwner() ? GetOwner()->FindComponentByClass<UWildBoundInventoryComponent>() : nullptr;
		if (!Inventory) return;

		FName ItemId = NAME_None;
		FName GroupTag = NAME_None;
		int32 Quantity = 0;
		FString PickupMessage;

		if (TargetActor->ActorHasTag(WaterTag))
		{
			ItemId = WaterItemId;
			GroupTag = WaterGroupTag;
			Quantity = 4;
			PickupMessage = TEXT("Collected 4 bottled waters");
		}
		else if (TargetActor->ActorHasTag(MedicalTag))
		{
			ItemId = MedicalItemId;
			GroupTag = MedicalGroupTag;
			Quantity = 1;
			PickupMessage = TEXT("Collected first-aid kit");
		}
		else if (TargetActor->ActorHasTag(FoodTag))
		{
			ItemId = FoodItemId;
			GroupTag = FoodGroupTag;
			Quantity = 3;
			PickupMessage = TEXT("Collected 3 preserved food rations");
		}

		if (ItemId.IsNone() || Quantity <= 0 || !CanInventoryFit(*Inventory, ItemId, Quantity) || !Inventory->AddItem(ItemId, Quantity))
		{
			if (GEngine) GEngine->AddOnScreenDebugMessage(91002, 2.0f, FColor::Red, TEXT("Inventory full"));
			return;
		}

		if (GEngine) GEngine->AddOnScreenDebugMessage(91002, 2.5f, GetRarityColor(Inventory->GetItemRarityTier(ItemId)), PickupMessage);
		DestroyInteractionGroup(GroupTag);
		return;
	}

	if (TargetActor->ActorHasTag(ClueTag) && TargetActor->ActorHasTag(C17ClueTag))
	{
		TargetActor->Tags.AddUnique(InspectedTag);
		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(
				91002,
				9.0f,
				FColor(220, 194, 122),
				TEXT("CIVIL DEFENSE FIELD SURVEY - SECTOR C-17\nBackground radiation elevated BEFORE the detonation alert.\nThree samples transferred off-site. Receiving authority: [REDACTED]."));
		}
	}
}

void UWildBoundInteractionComponent::TryPryTarget(AActor* TargetActor)
{
	if (!TargetActor)
	{
		return;
	}

	const UWildBoundInventoryComponent* Inventory = GetOwner() ? GetOwner()->FindComponentByClass<UWildBoundInventoryComponent>() : nullptr;
	if (!Inventory || !Inventory->HasItem(CrowbarItemId, 1))
	{
		if (GEngine) GEngine->AddOnScreenDebugMessage(91041, 1.8f, FColor(220, 145, 100), TEXT("You need a crowbar to force this open."));
		return;
	}

	if (TargetActor->ActorHasTag(PryAccessTag))
	{
		DestroyInteractionGroup(CommercialGateGroupTag);
		if (GEngine) GEngine->AddOnScreenDebugMessage(91041, 2.2f, FColor(185, 205, 165), TEXT("Maintenance gate forced open."));
		return;
	}

	if (TargetActor->ActorHasTag(PryContainerTag))
	{
		TargetActor->Tags.Remove(PryLockedTag);
		TargetActor->Tags.Remove(PryContainerTag);
		TargetActor->Tags.AddUnique(ContainerTag);
		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(
				91041,
				2.2f,
				GetRarityColor(GetContainerQualityTier(*TargetActor)),
				FString::Printf(TEXT("[%s] cache forced open. Search it."), *GetContainerQualityName(*TargetActor)));
		}
	}
}

void UWildBoundInteractionComponent::SearchLootContainer(AActor* TargetActor)
{
	if (!TargetActor || TargetActor->ActorHasTag(SearchedContainerTag))
	{
		return;
	}

	AActor* Owner = GetOwner();
	UWildBoundInventoryComponent* Inventory = Owner ? Owner->FindComponentByClass<UWildBoundInventoryComponent>() : nullptr;
	if (!Inventory)
	{
		return;
	}

	const int32 QualityTier = GetContainerQualityTier(*TargetActor);
	const FVector Location = TargetActor->GetActorLocation();
	const int32 LocationSeed = HashCombine(
		GetTypeHash(FMath::RoundToInt(Location.X)),
		HashCombine(GetTypeHash(FMath::RoundToInt(Location.Y)), GetTypeHash(FMath::RoundToInt(Location.Z))));
	FRandomStream Random(HashCombine(LocationSeed, FMath::Rand()));

	float EmptyChance = 0.18f;
	int32 MinRolls = 2;
	int32 MaxRolls = 4;
	if (QualityTier == 1)
	{
		EmptyChance = 0.09f;
		MinRolls = 3;
		MaxRolls = 4;
	}
	else if (QualityTier == 2)
	{
		EmptyChance = 0.03f;
		MinRolls = 3;
		MaxRolls = 5;
	}
	else if (QualityTier >= 3)
	{
		EmptyChance = 0.0f;
		MinRolls = 4;
		MaxRolls = 6;
	}

	if (TargetActor->ActorHasTag(MedicalPoolTag) || TargetActor->ActorHasTag(IndustrialPoolTag))
	{
		EmptyChance = FMath::Max(0.0f, EmptyChance - 0.03f);
	}

	if (Random.FRand() < EmptyChance)
	{
		TargetActor->Tags.AddUnique(SearchedContainerTag);
		TargetActor->Tags.Remove(InteractableTag);
		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(
				91002,
				2.2f,
				FColor(155, 155, 145),
				FString::Printf(TEXT("[%s] Empty. Someone got here first."), *GetContainerQualityName(*TargetActor)));
		}
		return;
	}

	TMap<FName, int32> Grants;
	const int32 Rolls = Random.RandRange(MinRolls, MaxRolls);
	for (int32 RollIndex = 0; RollIndex < Rolls; ++RollIndex)
	{
		const FName ItemId = RollLootItem(*TargetActor, Random);
		if (!ItemId.IsNone())
		{
			int32 Quantity = RollLootQuantity(ItemId, Random);
			if (QualityTier >= 2 && Inventory->GetItemRarityTier(ItemId) <= 1 && Random.FRand() < 0.55f)
			{
				++Quantity;
			}
			Grants.FindOrAdd(ItemId) += Quantity;
		}
	}

	AddQualityBonusLoot(*TargetActor, QualityTier, Random, Grants);

	int32 NewItemTypes = 0;
	for (const TPair<FName, int32>& Grant : Grants)
	{
		if (Grant.Value > 0 && Inventory->GetItemCount(Grant.Key) <= 0)
		{
			++NewItemTypes;
		}
	}

	if (Inventory->Stacks.Num() + NewItemTypes > Inventory->MaxSlots)
	{
		if (GEngine) GEngine->AddOnScreenDebugMessage(91002, 2.2f, FColor(220, 145, 115), TEXT("Not enough inventory space to search this container."));
		return;
	}

	FString FoundText = FString::Printf(TEXT("[%s] Found: "), *GetContainerQualityName(*TargetActor));
	bool bAddedAnything = false;
	int32 BestRarityTier = 0;

	for (const TPair<FName, int32>& Grant : Grants)
	{
		if (Grant.Value <= 0 || !Inventory->AddItem(Grant.Key, Grant.Value))
		{
			continue;
		}

		if (bAddedAnything)
		{
			FoundText += TEXT("  |  ");
		}

		const int32 ItemRarityTier = Inventory->GetItemRarityTier(Grant.Key);
		BestRarityTier = FMath::Max(BestRarityTier, ItemRarityTier);
		FoundText += FString::Printf(
			TEXT("[%s] %s x%d"),
			*Inventory->GetItemRarityName(Grant.Key),
			*Inventory->GetItemDisplayName(Grant.Key),
			Grant.Value);
		bAddedAnything = true;
	}

	TargetActor->Tags.AddUnique(SearchedContainerTag);
	TargetActor->Tags.Remove(InteractableTag);

	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(
			91002,
			QualityTier >= 2 || BestRarityTier >= 2 ? 4.2f : 3.4f,
			GetRarityColor(FMath::Max(QualityTier, BestRarityTier)),
			bAddedAnything ? FoundText : TEXT("Nothing useful inside."));
	}
}

void UWildBoundInteractionComponent::DestroyInteractionGroup(const FName& GroupTag)
{
	UWorld* World = GetWorld();
	if (!World || GroupTag.IsNone())
	{
		return;
	}

	TArray<AActor*> ActorsToDestroy;
	for (TActorIterator<AActor> It(World); It; ++It)
	{
		AActor* Actor = *It;
		if (Actor && Actor->ActorHasTag(GroupTag))
		{
			ActorsToDestroy.Add(Actor);
		}
	}

	for (AActor* Actor : ActorsToDestroy)
	{
		if (IsValid(Actor))
		{
			Actor->Destroy();
		}
	}
}
