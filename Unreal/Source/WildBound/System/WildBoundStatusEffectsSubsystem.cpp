#include "WildBoundStatusEffectsSubsystem.h"

#include "../Survival/WildBoundInjuryComponent.h"
#include "../Survival/WildBoundStatusEffectComponent.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"

void UWildBoundStatusEffectsSubsystem::OnWorldBeginPlay(UWorld& InWorld)
{
	Super::OnWorldBeginPlay(InWorld);

	if (!InWorld.IsGameWorld())
	{
		return;
	}

	EnsureStatusEffectsSetup();
	InWorld.GetTimerManager().SetTimer(
		StatusEffectsSetupTimer,
		this,
		&UWildBoundStatusEffectsSubsystem::EnsureStatusEffectsSetup,
		0.5f,
		true,
		0.10f);
}

void UWildBoundStatusEffectsSubsystem::Deinitialize()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(StatusEffectsSetupTimer);
	}

	Super::Deinitialize();
}

void UWildBoundStatusEffectsSubsystem::EnsureStatusEffectsSetup()
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

	if (!Pawn->FindComponentByClass<UWildBoundInjuryComponent>())
	{
		UWildBoundInjuryComponent* Injuries = NewObject<UWildBoundInjuryComponent>(Pawn, TEXT("WildBoundInjuries"));
		Pawn->AddInstanceComponent(Injuries);
		Injuries->RegisterComponent();
		UE_LOG(LogTemp, Log, TEXT("WildBound injuries: trauma and treatment system attached to player."));
	}

	if (!Pawn->FindComponentByClass<UWildBoundStatusEffectComponent>())
	{
		UWildBoundStatusEffectComponent* StatusEffects = NewObject<UWildBoundStatusEffectComponent>(Pawn, TEXT("WildBoundStatusEffects"));
		Pawn->AddInstanceComponent(StatusEffects);
		StatusEffects->RegisterComponent();
		UE_LOG(LogTemp, Log, TEXT("WildBound status effects: unified player condition layer attached."));
	}

	World->GetTimerManager().ClearTimer(StatusEffectsSetupTimer);
}
