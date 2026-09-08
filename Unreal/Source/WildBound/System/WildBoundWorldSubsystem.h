#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "TimerManager.h"
#include "WildBoundWorldSubsystem.generated.h"

class SWildBoundHUDWidget;
class SWidget;
class UWildBoundSurvivalComponent;

UCLASS()
class WILDBOUND_API UWildBoundWorldSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;
	virtual void OnWorldBeginPlay(UWorld& InWorld) override;

private:
	FTimerHandle PlayerSetupTimer;
	TSharedPtr<SWildBoundHUDWidget> HUDWidget;
	TSharedPtr<SWidget> HUDViewportWidget;
	TWeakObjectPtr<UWildBoundSurvivalComponent> ActiveSurvivalComponent;

	void EnsureWildBoundPlayerSetup();
	void EnsureHUD(UWildBoundSurvivalComponent* SurvivalComponent);
	void RemoveHUD();
};
