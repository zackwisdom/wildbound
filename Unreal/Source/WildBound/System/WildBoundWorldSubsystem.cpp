#include "WildBoundWorldSubsystem.h"

#include "Components/DirectionalLightComponent.h"
#include "Components/ExponentialHeightFogComponent.h"
#include "Components/SkyAtmosphereComponent.h"
#include "Components/SkyLightComponent.h"
#include "Engine/Engine.h"
#include "Engine/GameViewportClient.h"
#include "Engine/PostProcessVolume.h"
#include "Engine/World.h"
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

			// Fourth mood pass: a barely perceptible sickly cast in distant airborne dust only.
			// The 1000 cm start distance keeps the player's immediate surroundings neutral.
			Fog->SetFogInscatteringColor(
				FLinearColor::FromSRGBColor(FColor(184, 188, 166)));

			// Very light volumetric body so the warmer sun can catch suspended dust.
			Fog->SetVolumetricFog(true);
			Fog->SetVolumetricFogScatteringDistribution(0.20f);
			Fog->SetVolumetricFogExtinctionScale(0.35f);
			Fog->SetVolumetricFogAlbedo(FColor(198, 202, 184));
			Fog->SetVolumetricFogDistance(9000.0f);

			UE_LOG(LogTemp, Log, TEXT("WildBound atmosphere: subtle contaminated distance haze applied."));
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
			SkyAtmosphere->SetMieScatteringScale(1.08f);
			SkyAtmosphere->SetSkyLuminanceFactor(FLinearColor(0.96f, 0.97f, 0.94f, 1.0f));

			UE_LOG(LogTemp, Log, TEXT("WildBound atmosphere: muted post-disaster sky applied."));
			break;
		}

		// Fifth mood pass: slightly harsher direct sun against cooler, weaker ambient fill.
		// This adds depth and makes the abandoned town feel more exposed without crushing visibility.
		for (TActorIterator<AActor> It(&World); It; ++It)
		{
			UDirectionalLightComponent* Sun = It->FindComponentByClass<UDirectionalLightComponent>();
			if (!Sun)
			{
				continue;
			}

			Sun->SetIntensity(5.15f);
			Sun->SetUseTemperature(true);
			Sun->SetTemperature(5750.0f);
			Sun->SetIndirectLightingIntensity(0.92f);
			Sun->SetVolumetricScatteringIntensity(1.30f);

			UE_LOG(LogTemp, Log, TEXT("WildBound lighting: exposed sunlight applied."));
			break;
		}

		for (TActorIterator<AActor> It(&World); It; ++It)
		{
			USkyLightComponent* SkyLight = It->FindComponentByClass<USkyLightComponent>();
			if (!SkyLight)
			{
				continue;
			}

			SkyLight->SetIntensity(0.78f);
			SkyLight->SetLightColor(FLinearColor(0.92f, 0.95f, 1.0f, 1.0f));
			SkyLight->SetIndirectLightingIntensity(0.85f);

			UE_LOG(LogTemp, Log, TEXT("WildBound lighting: cooler ambient sky fill applied."));
			break;
		}

		// Third mood pass: a restrained global grade. Keep it natural and readable,
		// but remove some of the pristine template color and cool the deepest shadows.
		APostProcessVolume* PostProcessVolume = World.SpawnActor<APostProcessVolume>();
		if (PostProcessVolume)
		{
			PostProcessVolume->SetActorLabel(TEXT("WildBound_RuntimePostProcess"));
			PostProcessVolume->bUnbound = true;
			PostProcessVolume->bEnabled = true;
			PostProcessVolume->Priority = 50.0f;
			PostProcessVolume->BlendWeight = 1.0f;

			FPostProcessSettings& Settings = PostProcessVolume->Settings;

			Settings.bOverride_ColorSaturation = true;
			Settings.ColorSaturation = FVector4(0.94f, 0.94f, 0.94f, 1.0f);

			Settings.bOverride_ColorContrast = true;
			Settings.ColorContrast = FVector4(1.04f, 1.04f, 1.04f, 1.0f);

			Settings.bOverride_ColorGainShadows = true;
			Settings.ColorGainShadows = FVector4(0.97f, 0.99f, 1.03f, 1.0f);

			// Sixth mood pass: restrained ambient occlusion to ground objects where they meet surfaces.
			// Keep the radius tight and intensity moderate so it adds contact depth without dirty halos.
			Settings.bOverride_AmbientOcclusionIntensity = true;
			Settings.AmbientOcclusionIntensity = 0.65f;
			Settings.bOverride_AmbientOcclusionRadius = true;
			Settings.AmbientOcclusionRadius = 120.0f;
			Settings.bOverride_AmbientOcclusionPower = true;
			Settings.AmbientOcclusionPower = 1.15f;

			UE_LOG(LogTemp, Log, TEXT("WildBound atmosphere: subtle post-process and contact shadow grade applied."));
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
