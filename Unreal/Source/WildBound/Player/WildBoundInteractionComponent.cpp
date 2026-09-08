#include "WildBoundInteractionComponent.h"

#include "../Inventory/WildBoundInventoryComponent.h"
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
	const FName InspectedTag(TEXT("WBInspected"));

	const FName WaterTag(TEXT("WBItemWater"));
	const FName MedicalTag(TEXT("WBItemMedical"));
	const FName FoodTag(TEXT("WBItemFood"));
	const FName C17ClueTag(TEXT("WBClueC17"));

	const FName WaterGroupTag(TEXT("WBGroupWater"));
	const FName MedicalGroupTag(TEXT("WBGroupMedical"));
	const FName FoodGroupTag(TEXT("WBGroupFood"));
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

	FVector ViewLocation;
	FRotator ViewRotation;
	PlayerController->GetPlayerViewPoint(ViewLocation, ViewRotation);

	const FVector TraceEnd = ViewLocation + ViewRotation.Vector() * InteractionDistance;
	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(WildBoundInteractionTrace), false, Pawn);

	FHitResult Hit;
	const bool bHit = World->LineTraceSingleByChannel(Hit, ViewLocation, TraceEnd, ECC_Visibility, QueryParams);
	AActor* TargetActor = bHit ? Hit.GetActor() : nullptr;
	if (!TargetActor || !TargetActor->ActorHasTag(InteractableTag))
	{
		return;
	}

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
}

FString UWildBoundInteractionComponent::GetInteractionPrompt(const AActor* TargetActor) const
{
	if (!TargetActor)
	{
		return TEXT("Interact");
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
			ItemId = TEXT("Water");
			GroupTag = WaterGroupTag;
			Quantity = 4;
			PickupMessage = TEXT("Collected 4 bottled waters");
		}
		else if (TargetActor->ActorHasTag(MedicalTag))
		{
			ItemId = TEXT("MedicalSupplies");
			GroupTag = MedicalGroupTag;
			Quantity = 1;
			PickupMessage = TEXT("Collected first-aid kit");
		}
		else if (TargetActor->ActorHasTag(FoodTag))
		{
			ItemId = TEXT("Food");
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
