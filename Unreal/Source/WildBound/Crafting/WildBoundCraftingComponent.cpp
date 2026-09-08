#include "WildBoundCraftingComponent.h"

#include "../Inventory/WildBoundInventoryComponent.h"
#include "../Player/WildBoundBackpackComponent.h"
#include "../UI/SWildBoundCraftingWidget.h"
#include "Engine/Engine.h"
#include "Engine/GameViewportClient.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "InputCoreTypes.h"
#include "Widgets/SOverlay.h"

namespace
{
	const FName WorkbenchTag(TEXT("WBWorkbench"));

	FWildBoundCraftingIngredient Ingredient(const TCHAR* ItemId, int32 Quantity)
	{
		FWildBoundCraftingIngredient Result;
		Result.ItemId = FName(ItemId);
		Result.Quantity = Quantity;
		return Result;
	}
}

UWildBoundCraftingComponent::UWildBoundCraftingComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = true;
	PrimaryComponentTick.TickGroup = TG_PostUpdateWork;
}

void UWildBoundCraftingComponent::BeginPlay()
{
	Super::BeginPlay();
	InventoryComponent = GetOwner()
		? GetOwner()->FindComponentByClass<UWildBoundInventoryComponent>()
		: nullptr;
	bWorkbenchMode = false;
	BuildRecipesForCurrentMode();
	EnsureCraftingWidget();
}

void UWildBoundCraftingComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	SetCraftingOpen(false);
	RemoveCraftingWidget();
	Super::EndPlay(EndPlayReason);
}

void UWildBoundCraftingComponent::TickComponent(
	float DeltaTime,
	ELevelTick TickType,
	FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (!InventoryComponent.IsValid() && GetOwner())
	{
		InventoryComponent = GetOwner()->FindComponentByClass<UWildBoundInventoryComponent>();
	}

	EnsureCraftingWidget();

	APawn* Pawn = Cast<APawn>(GetOwner());
	APlayerController* PlayerController = Pawn ? Cast<APlayerController>(Pawn->GetController()) : nullptr;
	if (!PlayerController)
	{
		return;
	}

	if (PlayerController->WasInputKeyJustPressed(EKeys::C))
	{
		ToggleCrafting();
		return;
	}

	if (!bCraftingOpen)
	{
		return;
	}

	if (PlayerController->WasInputKeyJustPressed(EKeys::Escape)
		|| PlayerController->WasInputKeyJustPressed(EKeys::I)
		|| PlayerController->WasInputKeyJustPressed(EKeys::Tab))
	{
		SetCraftingOpen(false);
		return;
	}

	if (PlayerController->WasInputKeyJustPressed(EKeys::Up))
	{
		MoveSelection(-1);
	}
	else if (PlayerController->WasInputKeyJustPressed(EKeys::Down))
	{
		MoveSelection(1);
	}
	else if (PlayerController->WasInputKeyJustPressed(EKeys::Enter))
	{
		CraftSelectedRecipe();
	}
}

bool UWildBoundCraftingComponent::IsNearWorkbench() const
{
	const UWorld* World = GetWorld();
	const AActor* Owner = GetOwner();
	if (!World || !Owner)
	{
		return false;
	}

	const float RadiusSq = FMath::Square(WorkbenchUseRadius);
	for (TActorIterator<AActor> It(const_cast<UWorld*>(World)); It; ++It)
	{
		const AActor* Actor = *It;
		if (Actor
			&& Actor->ActorHasTag(WorkbenchTag)
			&& FVector::DistSquared(Owner->GetActorLocation(), Actor->GetActorLocation()) <= RadiusSq)
		{
			return true;
		}
	}
	return false;
}

void UWildBoundCraftingComponent::BuildRecipesForCurrentMode()
{
	Recipes.Reset();

	if (!bWorkbenchMode)
	{
		FWildBoundCraftingRecipe FirstAid;
		FirstAid.RecipeId = TEXT("FirstAidKit");
		FirstAid.DisplayName = TEXT("HAND: FIRST-AID KIT");
		FirstAid.Description = TEXT("Hand-craft a usable field medical kit from scavenged cloth, disinfectant chemicals, and adhesive.");
		FirstAid.OutputItemId = TEXT("MedicalSupplies");
		FirstAid.OutputQuantity = 1;
		FirstAid.Ingredients =
		{
			Ingredient(TEXT("Cloth"), 3),
			Ingredient(TEXT("Chemicals"), 1),
			Ingredient(TEXT("Adhesive"), 1)
		};
		Recipes.Add(FirstAid);
	}
	else
	{
		FWildBoundCraftingRecipe Flashlight;
		Flashlight.RecipeId = TEXT("Flashlight");
		Flashlight.DisplayName = TEXT("BENCH: FLASHLIGHT");
		Flashlight.Description = TEXT("Use the workbench to assemble a working handheld light from electronics, batteries, plastic housing, and wire.");
		Flashlight.OutputItemId = TEXT("Flashlight");
		Flashlight.OutputQuantity = 1;
		Flashlight.Ingredients =
		{
			Ingredient(TEXT("Electronics"), 1),
			Ingredient(TEXT("Battery"), 2),
			Ingredient(TEXT("Plastic"), 1),
			Ingredient(TEXT("Wire"), 1)
		};
		Recipes.Add(Flashlight);

		FWildBoundCraftingRecipe Crowbar;
		Crowbar.RecipeId = TEXT("Crowbar");
		Crowbar.DisplayName = TEXT("BENCH: IMPROVISED CROWBAR");
		Crowbar.Description = TEXT("Use the vice and bench tools to shape and reinforce salvaged metal into a heavy pry tool.");
		Crowbar.OutputItemId = TEXT("Crowbar");
		Crowbar.OutputQuantity = 1;
		Crowbar.Ingredients =
		{
			Ingredient(TEXT("ScrapMetal"), 5),
			Ingredient(TEXT("MechanicalParts"), 2),
			Ingredient(TEXT("Cloth"), 1)
		};
		Recipes.Add(Crowbar);
	}

	SelectedRecipeIndex = 0;
}

void UWildBoundCraftingComponent::EnsureCraftingWidget()
{
	if (CraftingViewportRoot.IsValid())
	{
		if (CraftingWidget.IsValid())
		{
			CraftingWidget->SetCraftingComponent(this);
		}
		return;
	}

	if (!InventoryComponent.IsValid() || !GEngine || !GEngine->GameViewport)
	{
		return;
	}

	TSharedPtr<SOverlay> Overlay;
	SAssignNew(Overlay, SOverlay)
	+ SOverlay::Slot()
	.HAlign(HAlign_Center)
	.VAlign(VAlign_Center)
	.Padding(FMargin(24.0f))
	[
		SAssignNew(CraftingWidget, SWildBoundCraftingWidget)
		.CraftingComponent(this)
	];

	CraftingViewportRoot = Overlay;
	CraftingViewportRoot->SetVisibility(EVisibility::Collapsed);
	GEngine->GameViewport->AddViewportWidgetContent(CraftingViewportRoot.ToSharedRef(), 140);
}

void UWildBoundCraftingComponent::ToggleCrafting()
{
	if (bCraftingOpen)
	{
		SetCraftingOpen(false);
		return;
	}
	OpenCrafting(IsNearWorkbench());
}

void UWildBoundCraftingComponent::OpenCrafting(bool bUseWorkbench)
{
	const UWildBoundBackpackComponent* Backpack = GetOwner()
		? GetOwner()->FindComponentByClass<UWildBoundBackpackComponent>()
		: nullptr;
	if (Backpack && Backpack->IsBackpackOpen())
	{
		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(91020, 1.8f, FColor(190, 190, 175), TEXT("Close the backpack before opening crafting."));
		}
		return;
	}

	bWorkbenchMode = bUseWorkbench;
	BuildRecipesForCurrentMode();
	SetCraftingOpen(true);

	if (bCraftingOpen && GEngine)
	{
		GEngine->AddOnScreenDebugMessage(
			91023,
			1.6f,
			bWorkbenchMode ? FColor(205, 190, 135) : FColor(175, 205, 165),
			bWorkbenchMode
				? TEXT("WORKBENCH CRAFTING — advanced recipes available")
				: TEXT("HAND CRAFTING — find a workbench for advanced recipes"));
	}
}

void UWildBoundCraftingComponent::SetCraftingOpen(bool bOpen)
{
	if (bCraftingOpen == bOpen && (!bOpen || CraftingViewportRoot.IsValid()))
	{
		return;
	}

	EnsureCraftingWidget();
	bCraftingOpen = bOpen && CraftingViewportRoot.IsValid();

	if (CraftingViewportRoot.IsValid())
	{
		CraftingViewportRoot->SetVisibility(
			bCraftingOpen ? EVisibility::HitTestInvisible : EVisibility::Collapsed);
	}

	APawn* Pawn = Cast<APawn>(GetOwner());
	APlayerController* PlayerController = Pawn ? Cast<APlayerController>(Pawn->GetController()) : nullptr;
	if (PlayerController)
	{
		PlayerController->SetIgnoreMoveInput(bCraftingOpen);
		PlayerController->SetIgnoreLookInput(bCraftingOpen);
	}
}

void UWildBoundCraftingComponent::MoveSelection(int32 Direction)
{
	if (Recipes.IsEmpty() || Direction == 0)
	{
		return;
	}

	SelectedRecipeIndex = (SelectedRecipeIndex + Direction) % Recipes.Num();
	if (SelectedRecipeIndex < 0)
	{
		SelectedRecipeIndex += Recipes.Num();
	}
}

bool UWildBoundCraftingComponent::CanCraftRecipe(int32 RecipeIndex) const
{
	const UWildBoundInventoryComponent* Inventory = InventoryComponent.Get();
	if (!Inventory || !Recipes.IsValidIndex(RecipeIndex))
	{
		return false;
	}

	for (const FWildBoundCraftingIngredient& Requirement : Recipes[RecipeIndex].Ingredients)
	{
		if (!Inventory->HasItem(Requirement.ItemId, Requirement.Quantity))
		{
			return false;
		}
	}
	return true;
}

void UWildBoundCraftingComponent::CraftSelectedRecipe()
{
	UWildBoundInventoryComponent* Inventory = InventoryComponent.Get();
	if (!Inventory || !Recipes.IsValidIndex(SelectedRecipeIndex))
	{
		return;
	}

	const FWildBoundCraftingRecipe& Recipe = Recipes[SelectedRecipeIndex];
	if (!CanCraftRecipe(SelectedRecipeIndex))
	{
		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(91021, 1.8f, FColor(220, 145, 110), TEXT("Missing crafting materials."));
		}
		return;
	}

	for (const FWildBoundCraftingIngredient& Requirement : Recipe.Ingredients)
	{
		if (!Inventory->RemoveItem(Requirement.ItemId, Requirement.Quantity))
		{
			return;
		}
	}

	if (!Inventory->AddItem(Recipe.OutputItemId, Recipe.OutputQuantity))
	{
		for (const FWildBoundCraftingIngredient& Requirement : Recipe.Ingredients)
		{
			Inventory->AddItem(Requirement.ItemId, Requirement.Quantity);
		}

		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(91021, 2.0f, FColor(220, 145, 110), TEXT("Not enough inventory space for crafted item."));
		}
		return;
	}

	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(
			91021,
			2.2f,
			FColor(170, 215, 155),
			FString::Printf(TEXT("Crafted: %s x%d"), *Recipe.DisplayName, Recipe.OutputQuantity));
	}
}

void UWildBoundCraftingComponent::RemoveCraftingWidget()
{
	if (CraftingViewportRoot.IsValid() && GEngine && GEngine->GameViewport)
	{
		GEngine->GameViewport->RemoveViewportWidgetContent(CraftingViewportRoot.ToSharedRef());
	}

	CraftingWidget.Reset();
	CraftingViewportRoot.Reset();
	bCraftingOpen = false;
}
