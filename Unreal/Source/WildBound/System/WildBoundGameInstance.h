#pragma once

#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "TimerManager.h"
#include "WildBoundGameInstance.generated.h"

class SWildBoundHUDWidget;
class SWidget;
class UWildBoundSurvivalComponent;

UCLASS()
class WILDBOUND_API UWildBoundGameInstance : public UGameInstance
{
	GENERATED_BODY()

public:
	virtual void OnStart() override;
	virtual void Shutdown() override;

private:
	FTimerHandle PlayerSetupTimer;
	TSharedPtr<SWildBoundHUDWidget> HUDWidget;
	TSharedPtr<SWidget> HUDViewportWidget;
	TWeakObjectPtr<UWildBoundSurvivalComponent> ActiveSurvivalComponent;

	void EnsureWildBoundPlayerSetup();
	void EnsureHUD(UWildBoundSurvivalComponent* SurvivalComponent);
	void RemoveHUD();
};
