#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "TimerManager.h"
#include "WildBoundPauseMenuSubsystem.generated.h"

class SWidget;
class UWorld;

UCLASS()
class WILDBOUND_API UWildBoundPauseMenuSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	virtual void OnWorldBeginPlay(UWorld& InWorld) override;
	virtual void Deinitialize() override;

	UFUNCTION(BlueprintPure, Category="WildBound|Pause")
	bool IsPauseMenuOpen() const { return bPauseMenuOpen; }

	void ContinueGame();
	void SaveGame();
	void LoadGame();
	void OpenSettings();
	void CloseSettings();
	void QuitGame();

	void ChangeGraphicsQuality(int32 Delta);
	void ToggleVSync();
	void CycleFrameRateLimit();

	FText GetGraphicsQualityText() const;
	FText GetVSyncText() const;
	FText GetFrameRateLimitText() const;
	bool HasSaveGame() const;

private:
	TSharedPtr<SWidget> PauseViewportRoot;
	FTimerHandle PauseInputTimer;
	bool bPauseMenuOpen = false;
	bool bSettingsOpen = false;
	bool bBlockingUIWasOpenLastCheck = false;

	void UpdatePauseInput();
	void EnsurePauseWidget();
	void OpenPauseMenu();
	void RemovePauseWidget();
	bool HasBlockingUIOpen() const;
	void ApplyPauseState(bool bPaused);
};
