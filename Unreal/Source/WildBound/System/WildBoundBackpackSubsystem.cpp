#include "WildBoundBackpackSubsystem.h"

#include "../Player/WildBoundBackpackComponent.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"

void UWildBoundBackpackSubsystem::OnWorldBeginPlay(UWorld& InWorld)
{
	Super::OnWorldBeginPlay(InWorld);

	if (!InWorld.IsGameWorld())
	{
		return;
	}

	EnsureBackpackSetup();
	InWorld.GetTimerManager().SetTimer(
		BackpackSetupTimer,
		this,
		&UWildBoundBackpackSubsystem::EnsureBackpackSetup,
		0.5f,
		true,
		0.10f);
}

void UWildBoundBackpackSubsystem::Deinitialize()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(BackpackSetupTimer);
	}

	Super::Deinitialize();
}

void UWildBoundBackpackSubsystem::EnsureBackpackSetup()
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

	if (!Pawn->FindComponentByClass<UWildBoundBackpackComponent>())
	{
		UWildBoundBackpackComponent* Backpack = NewObject<UWildBoundBackpackComponent>(Pawn, TEXT("WildBoundBackpack"));
		Pawn->AddInstanceComponent(Backpack);
		Backpack->RegisterComponent();
		UE_LOG(LogTemp, Log, TEXT("WildBound backpack: inventory screen attached to player."));
	}

	World->GetTimerManager().ClearTimer(BackpackSetupTimer);
}
