#include "WildBoundInteractionComponent.h"

#include "../Inventory/WildBoundInventoryComponent.h"
#include "../Survival/WildBoundSurvivalComponent.h"
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
	const FName SearchedContainerTag(TEXT("WBContainerSearched"));
	const FName InspectedTag(TEXT("WBInspected"));

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

	constexpr int32 HotbarSlotCount = 3;

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

	FString GetLootDisplayName(const FName& ItemId)
	{
		if (ItemId == WaterItemId) return TEXT("Water");
		if (ItemId == FoodItemId) return TEXT("Food");
		if (ItemId == MedicalItemId) return TEXT("Medical Supplies");
		if (ItemId == ScrapItemId) return TEXT("Scrap Metal");
		if (ItemId == ClothItemId) return TEXT("Cloth");
		if (ItemId == WoodItemId) return TEXT("Wood");
		if (ItemId == PlasticItemId) return TEXT("Plastic");
		if (ItemId == ElectronicsItemId) return TEXT("Electronics");
		if (ItemId == ChemicalsItemId) return TEXT("Chemicals");
		if (ItemId == AdhesiveItemId) return TEXT("Adhesive");
		if (ItemId == WireItemId) return TEXT("Wire");
		if (ItemId == BatteryItemId) return TEXT("Battery");
		if (ItemId == MechanicalPartsItemId) return TEXT("Mechanical Parts");
		if (ItemId == FlashlightItemId) return TEXT("Flashlight");
		if (ItemId == CrowbarItemId) return TEXT("Crowbar");
		return ItemId.ToString();
	}
}

UWildBoundInteractionComponent::UWildBoundInteractionComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.TickGroup = TG_PostUpdateWork;
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
		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(
				91001,
				0.08f,
				FColor::White,
				FString::Printf(TEXT("[E] %s"), *GetInteractionPrompt(TargetActor)));
		}

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
	switch (SelectedHotbarSlot)
	{
	case 0:
		TryUseInventoryItem(WaterItemId);
		break;
	case 1:
		TryUseInventoryItem(FoodItemId);
		break;
	case 2:
		TryUseInventoryItem(MedicalItemId);
		break;
	default:
		break;
	}
}

void UWildBoundInteractionComponent::TryUseInventoryItem(FName ItemId)
{
	AActor* Owner = GetOwner();
	UWildBoundInventoryComponent* Inventory = Owner ? Owner->FindComponentByClass<UWildBoundInventoryComponent>() : nullptr;
	UWildBoundSurvivalComponent* Survival = Owner ? Owner->FindComponentByClass<UWildBoundSurvivalComponent>() : nullptr;
	if (!Inventory || !Survival)
	{
		return;
	}

	if (!Inventory->HasItem(ItemId, 1))
	{
		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(91003, 1.8f, FColor(185, 185, 175), TEXT("That hotbar slot is empty."));
		}
		return;
	}

	FString UseMessage;
	FColor MessageColor(205, 220, 190);

	if (ItemId == WaterItemId)
	{
		if (Survival->Thirst >= Survival->MaxThirst - KINDA_SMALL_NUMBER)
		{
			if (GEngine)
			{
				GEngine->AddOnScreenDebugMessage(91003, 1.8f, FColor(170, 200, 220), TEXT("Thirst is already full."));
			}
			return;
		}

		if (!Inventory->RemoveItem(ItemId, 1))
		{
			return;
		}
		Survival->AddThirst(35.0f);
		UseMessage = TEXT("Drank bottled water  +35 THIRST");
		MessageColor = FColor(145, 195, 225);
	}
	else if (ItemId == FoodItemId)
	{
		if (Survival->Hunger >= Survival->MaxHunger - KINDA_SMALL_NUMBER)
		{
			if (GEngine)
			{
				GEngine->AddOnScreenDebugMessage(91003, 1.8f, FColor(215, 185, 120), TEXT("Hunger is already full."));
			}
			return;
		}

		if (!Inventory->RemoveItem(ItemId, 1))
		{
			return;
		}
		Survival->AddHunger(30.0f);
		UseMessage = TEXT("Ate preserved ration  +30 HUNGER");
		MessageColor = FColor(215, 185, 120);
	}
	else if (ItemId == MedicalItemId)
	{
		if (Survival->Health >= Survival->MaxHealth - KINDA_SMALL_NUMBER)
		{
			if (GEngine)
			{
				GEngine->AddOnScreenDebugMessage(91003, 1.8f, FColor(220, 155, 145), TEXT("Health is already full."));
			}
			return;
		}

		if (!Inventory->RemoveItem(ItemId, 1))
		{
			return;
		}
		Survival->Heal(45.0f);
		UseMessage = TEXT("Used first-aid kit  +45 HEALTH");
		MessageColor = FColor(220, 155, 145);
	}
	else
	{
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

	if (TargetActor->ActorHasTag(ContainerTag))
	{
		if (TargetActor->ActorHasTag(ToolboxContainerTag)) return TEXT("Search toolbox");
		if (TargetActor->ActorHasTag(MedicalPoolTag) && TargetActor->ActorHasTag(CabinetContainerTag)) return TEXT("Search medical cabinet");
		if (TargetActor->ActorHasTag(LockerContainerTag)) return TEXT("Search locker");
		if (TargetActor->ActorHasTag(DumpsterContainerTag)) return TEXT("Search dumpster");
		if (TargetActor->ActorHasTag(CoolerContainerTag)) return TEXT("Search cooler");
		if (TargetActor->ActorHasTag(CabinetContainerTag)) return TEXT("Search cabinet");
		if (TargetActor->ActorHasTag(CrateContainerTag)) return TEXT("Search crate");
		return TEXT("Search container");
	}
	if (TargetActor->ActorHasTag(WaterTag))
	{
		return TEXT("Take bottled water");
	}
	if (TargetActor->ActorHasTag(MedicalTag))
	{
		return TEXT("Take first-aid kit");
	}
	if (TargetActor->ActorHasTag(FoodTag))
	{
		return TEXT("Take preserved food");
	}
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

	if (TargetActor->ActorHasTag(ContainerTag))
	{
		SearchLootContainer(TargetActor);
		return;
	}

	if (TargetActor->ActorHasTag(SupplyTag))
	{
		UWildBoundInventoryComponent* Inventory = GetOwner()
			? GetOwner()->FindComponentByClass<UWildBoundInventoryComponent>()
			: nullptr;
		if (!Inventory)
		{
			return;
		}

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

		if (ItemId.IsNone() || Quantity <= 0)
		{
			return;
		}

		if (!Inventory->AddItem(ItemId, Quantity))
		{
			if (GEngine)
			{
				GEngine->AddOnScreenDebugMessage(91002, 2.0f, FColor::Red, TEXT("Inventory full"));
			}
			return;
		}

		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(91002, 2.5f, FColor(205, 220, 190), PickupMessage);
		}

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

	const FVector Location = TargetActor->GetActorLocation();
	const int32 LocationSeed = HashCombine(
		GetTypeHash(FMath::RoundToInt(Location.X)),
		HashCombine(GetTypeHash(FMath::RoundToInt(Location.Y)), GetTypeHash(FMath::RoundToInt(Location.Z))));
	FRandomStream Random(HashCombine(LocationSeed, FMath::Rand()));

	float EmptyChance = 0.18f;
	if (TargetActor->ActorHasTag(MedicalPoolTag) || TargetActor->ActorHasTag(IndustrialPoolTag))
	{
		EmptyChance = 0.10f;
	}

	if (Random.FRand() < EmptyChance)
	{
		TargetActor->Tags.AddUnique(SearchedContainerTag);
		TargetActor->Tags.Remove(InteractableTag);
		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(91002, 2.2f, FColor(155, 155, 145), TEXT("Empty. Someone got here first."));
		}
		return;
	}

	TMap<FName, int32> Grants;
	const int32 Rolls = Random.RandRange(2, 4);
	for (int32 RollIndex = 0; RollIndex < Rolls; ++RollIndex)
	{
		const FName ItemId = RollLootItem(*TargetActor, Random);
		if (!ItemId.IsNone())
		{
			Grants.FindOrAdd(ItemId) += RollLootQuantity(ItemId, Random);
		}
	}

	// Rare intact tools are deliberately tied to believable places rather than global random drops.
	if (TargetActor->ActorHasTag(IndustrialPoolTag) && Random.FRand() < 0.10f)
	{
		Grants.FindOrAdd(CrowbarItemId) += 1;
	}
	else if (TargetActor->ActorHasTag(ResidentialPoolTag) && Random.FRand() < 0.08f)
	{
		Grants.FindOrAdd(FlashlightItemId) += 1;
	}

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
		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(91002, 2.2f, FColor(220, 145, 115), TEXT("Not enough inventory space to search this container."));
		}
		return;
	}

	FString FoundText(TEXT("Found: "));
	bool bAddedAnything = false;
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
		FoundText += FString::Printf(TEXT("%s x%d"), *GetLootDisplayName(Grant.Key), Grant.Value);
		bAddedAnything = true;
	}

	TargetActor->Tags.AddUnique(SearchedContainerTag);
	TargetActor->Tags.Remove(InteractableTag);

	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(
			91002,
			3.4f,
			FColor(205, 220, 190),
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
