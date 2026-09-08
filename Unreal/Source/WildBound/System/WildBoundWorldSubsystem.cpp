#include "WildBoundWorldSubsystem.h"

#include "Components/ExponentialHeightFogComponent.h"
#include "Components/SkyAtmosphereComponent.h"
#include "Engine/Engine.h"
#include "Engine/GameViewportClient.h"
#include "EngineUtils.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "Widgets/SOverlay.h"
#include "../Inventory/WildBoundInventoryComponent.h"
#include "../Player/WildBoundSprintComponent.h"
#include "../Survival/WildBoundSurvivalComponent.h"
#include "../UI/SWildBoundHUDWidget.h"

namespace
{
	void ApplyWildBoundAtmosphere(UWorld& World)
	{
		for (TActorIterator<AActor> It(&World); It; ++It)
		{
			UExponentialHeightFogComponent* Fog = It->FindComponentByClass<UExponentialHeightFogComponent>();
			if (!Fog)
			{
				continue;
			}

			// First WildBound atmosphere pass: a restrained, dusty post-disaster haze.
			// Keep nearby visibility clean while letting distance feel dry and contaminated.
			Fog->SetFogDensity(0.012f);
			Fog->SetFogHeightFalloff(0.20f);
			Fog->SetStartDistance(1000.0f);
			Fog->SetFogMaxOpacity(0.45f);
			Fog->SetFogInscatteringColor(
				FLinearColor::FromSRGBColor(FColor(190, 184, 168)));

			// Very light volumetric body so the warmer sun can catch suspended dust.
			Fog->SetVolumetricFog(true);
			Fog->SetVolumetricFogScatteringDistribution(0.20f);
			Fog->SetVolumetricFogExtinctionScale(0.35f);
			Fog->SetVolumetricFogAlbedo(FColor(205, 200, 185));
			Fog->SetVolumetricFogDistance(9000.0f);

			UE_LOG(LogTemp, Log, TEXT("WildBound atmosphere: subtle dust haze applied."));
			break;
		}

		for (TActorIterator<AActor> It(&World); It; ++It)
		{
			USkyAtmosphereComponent* SkyAtmosphere = It->FindComponentByClass<USkyAtmosphereComponent>();
			if (!SkyAtmosphere)
			{
				continue;
			}

			// Second mood pass: make the daylight sky feel slightly dirtier and less pristine.
			// A small Mie increase adds aerosol haze, while the luminance factor gently mutes the clean blue.
			SkyAtmosphere->SetMieScatteringScale(1.08f);
			SkyAtmosphere->SetSkyLuminanceFactor(FLinearColor(0.96f, 0.97f, 0.94f, 1.0f));

			UE_LOG(LogTemp, Log, TEXT("WildBound atmosphere: muted post-disaster sky applied."));
			break;
		}
	}
}

void UWildBoundWorldSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
}

void UWildBoundWorldSubsystem::Deinitialize()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(PlayerSetupTimer);
	}

	RemoveHUD();
	Super::Deinitialize();
}

void UWildBoundWorldSubsystem::OnWorldBeginPlay(UWorld& InWorld)
{
	Super::OnWorldBeginPlay(InWorld);

	if (!InWorld.IsGameWorld())
	{
		return;
	}

	ApplyWildBoundAtmosphere(InWorld);
	EnsureWildBoundPlayerSetup();

	InWorld.GetTimerManager().SetTimer(
		PlayerSetupTimer,
		this,
		&UWildBoundWorldSubsystem::EnsureWildBoundPlayerSetup,
		0.25f,
		true,
		0.10f);
}

void UWildBoundWorldSubsystem::EnsureWildBoundPlayerSetup()
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

void UWildBoundWorldSubsystem::EnsureHUD(UWildBoundSurvivalComponent* SurvivalComponent)
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
	GEngine->GameViewport->AddViewportWidgetContent(HUDViewportWidget.ToSharedRef(), 100);

	UE_LOG(LogTemp, Log, TEXT("WildBound HUD attached to viewport."));
}

void UWildBoundWorldSubsystem::RemoveHUD()
{
	if (HUDViewportWidget.IsValid() && GEngine && GEngine->GameViewport)
	{
		GEngine->GameViewport->RemoveViewportWidgetContent(HUDViewportWidget.ToSharedRef());
	}

	HUDWidget.Reset();
	HUDViewportWidget.Reset();
	ActiveSurvivalComponent.Reset();
}
