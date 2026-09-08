#include "WildBoundGearSubsystem.h"

#include "../Player/WildBoundGearComponent.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"

void UWildBoundGearSubsystem::OnWorldBeginPlay(UWorld& InWorld)
{
	Super::OnWorldBeginPlay(InWorld);

	if (!InWorld.IsGameWorld())
	{
		return;
	}

	EnsureGearSetup();
	InWorld.GetTimerManager().SetTimer(
		GearSetupTimer,
		this,
		&UWildBoundGearSubsystem::EnsureGearSetup,
		0.5f,
		true,
		0.10f);
}

void UWildBoundGearSubsystem::Deinitialize()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(GearSetupTimer);
	}

	Super::Deinitialize();
}

void UWildBoundGearSubsystem::EnsureGearSetup()
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

	if (!Pawn->FindComponentByClass<UWildBoundGearComponent>())
	{
		UWildBoundGearComponent* Gear = NewObject<UWildBoundGearComponent>(Pawn, TEXT("WildBoundGear"));
		Pawn->AddInstanceComponent(Gear);
		Gear->RegisterComponent();
		UE_LOG(LogTemp, Log, TEXT("WildBound gear: survival equipment progression attached to player."));
	}

	World->GetTimerManager().ClearTimer(GearSetupTimer);
}
