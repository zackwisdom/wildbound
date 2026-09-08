#include "WildBoundGameInstance.h"

#include "Engine/Engine.h"
#include "Engine/GameViewportClient.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "Widgets/Layout/SOverlay.h"
#include "Widgets/SWeakWidget.h"
#include "../Inventory/WildBoundInventoryComponent.h"
#include "../Player/WildBoundSprintComponent.h"
#include "../Survival/WildBoundSurvivalComponent.h"
#include "../UI/SWildBoundHUDWidget.h"

void UWildBoundGameInstance::OnStart()
{
	Super::OnStart();

	EnsureWildBoundPlayerSetup();

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimer(
			PlayerSetupTimer,
			this,
			&UWildBoundGameInstance::EnsureWildBoundPlayerSetup,
			0.5f,
			true,
			0.25f);
	}
}

void UWildBoundGameInstance::Shutdown()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(PlayerSetupTimer);
	}

	RemoveHUD();
	Super::Shutdown();
}

void UWildBoundGameInstance::EnsureWildBoundPlayerSetup()
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

	UWildBoundSurvivalComponent* Survival = Pawn->FindComponentByClass<UWildBoundSurvivalComponent>();
	if (!Survival)
	{
		Survival = NewObject<UWildBoundSurvivalComponent>(Pawn, TEXT("WildBoundSurvival"));
		Pawn->AddInstanceComponent(Survival);
		Survival->RegisterComponent();
	}

	UWildBoundInventoryComponent* Inventory = Pawn->FindComponentByClass<UWildBoundInventoryComponent>();
	if (!Inventory)
	{
		Inventory = NewObject<UWildBoundInventoryComponent>(Pawn, TEXT("WildBoundInventory"));
		Pawn->AddInstanceComponent(Inventory);
		Inventory->RegisterComponent();
	}

	UWildBoundSprintComponent* Sprint = Pawn->FindComponentByClass<UWildBoundSprintComponent>();
	if (!Sprint)
	{
		Sprint = NewObject<UWildBoundSprintComponent>(Pawn, TEXT("WildBoundSprint"));
		Pawn->AddInstanceComponent(Sprint);
		Sprint->RegisterComponent();
	}

	if (ActiveSurvivalComponent.Get() != Survival)
	{
		ActiveSurvivalComponent = Survival;
		EnsureHUD(Survival);
	}
	else if (!HUDViewportWidget.IsValid())
	{
		EnsureHUD(Survival);
	}
}

void UWildBoundGameInstance::EnsureHUD(UWildBoundSurvivalComponent* SurvivalComponent)
{
	if (!SurvivalComponent || !GEngine || !GEngine->GameViewport)
	{
		return;
	}

	if (HUDWidget.IsValid())
	{
		HUDWidget->SetSurvivalComponent(SurvivalComponent);
		return;
	}

	TSharedPtr<SOverlay> Overlay;
	SAssignNew(Overlay, SOverlay)
	+ SOverlay::Slot()
	.HAlign(HAlign_Left)
	.VAlign(VAlign_Bottom)
	.Padding(FMargin(28.0f, 28.0f, 0.0f, 30.0f))
	[
		SAssignNew(HUDWidget, SWildBoundHUDWidget)
		.SurvivalComponent(SurvivalComponent)
	];

	HUDViewportWidget = Overlay;
	GEngine->GameViewport->AddViewportWidgetContent(HUDViewportWidget.ToSharedRef(), 50);
}

void UWildBoundGameInstance::RemoveHUD()
{
	if (HUDViewportWidget.IsValid() && GEngine && GEngine->GameViewport)
	{
		GEngine->GameViewport->RemoveViewportWidgetContent(HUDViewportWidget.ToSharedRef());
	}

	HUDWidget.Reset();
	HUDViewportWidget.Reset();
	ActiveSurvivalComponent.Reset();
}
