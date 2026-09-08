#include "WildBoundCraftingSubsystem.h"

#include "../Crafting/WildBoundCraftingComponent.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"

void UWildBoundCraftingSubsystem::OnWorldBeginPlay(UWorld& InWorld)
{
	Super::OnWorldBeginPlay(InWorld);

	if (!InWorld.IsGameWorld())
	{
		return;
	}

	EnsureCraftingSetup();
	InWorld.GetTimerManager().SetTimer(
		CraftingSetupTimer,
		this,
		&UWildBoundCraftingSubsystem::EnsureCraftingSetup,
		0.5f,
		true,
		0.10f);
}

void UWildBoundCraftingSubsystem::Deinitialize()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(CraftingSetupTimer);
	}

	Super::Deinitialize();
}

void UWildBoundCraftingSubsystem::EnsureCraftingSetup()
{
	UWorld* World = GetWorld();
	if (!World || !World->IsGameWorld())
	{
		return;
	}

	APlayerController* PlayerController = World->GetFirstPlayerController();
	APawn* Pawn = PlayerController ? PlayerController->GetPawn() : nullptr;
	if (!Pawn)
	{
		return;
	}

	if (!Pawn->FindComponentByClass<UWildBoundCraftingComponent>())
	{
		UWildBoundCraftingComponent* Crafting = NewObject<UWildBoundCraftingComponent>(Pawn, TEXT("WildBoundCrafting"));
		Pawn->AddInstanceComponent(Crafting);
		Crafting->RegisterComponent();
		UE_LOG(LogTemp, Log, TEXT("WildBound crafting: starter recipe system attached to player."));
	}

	World->GetTimerManager().ClearTimer(CraftingSetupTimer);
}
