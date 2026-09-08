#include "WildBoundEquipmentSubsystem.h"

#include "../Player/WildBoundEquipmentComponent.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"

void UWildBoundEquipmentSubsystem::OnWorldBeginPlay(UWorld& InWorld)
{
	Super::OnWorldBeginPlay(InWorld);

	if (!InWorld.IsGameWorld())
	{
		return;
	}

	EnsureEquipmentSetup();
	InWorld.GetTimerManager().SetTimer(
		EquipmentSetupTimer,
		this,
		&UWildBoundEquipmentSubsystem::EnsureEquipmentSetup,
		0.5f,
		true,
		0.10f);
}

void UWildBoundEquipmentSubsystem::Deinitialize()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(EquipmentSetupTimer);
	}

	Super::Deinitialize();
}

void UWildBoundEquipmentSubsystem::EnsureEquipmentSetup()
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

	if (!Pawn->FindComponentByClass<UWildBoundEquipmentComponent>())
	{
		UWildBoundEquipmentComponent* Equipment = NewObject<UWildBoundEquipmentComponent>(Pawn, TEXT("WildBoundEquipment"));
		Pawn->AddInstanceComponent(Equipment);
		Equipment->RegisterComponent();
		UE_LOG(LogTemp, Log, TEXT("WildBound equipment: flashlight utility attached to player."));
	}

	World->GetTimerManager().ClearTimer(EquipmentSetupTimer);
}
