#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "TimerManager.h"
#include "WildBoundMainMenuSubsystem.generated.h"

class SWidget;
class UWorld;

UCLASS()
class WILDBOUND_API UWildBoundMainMenuSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	virtual void OnWorldBeginPlay(UWorld& InWorld) override;
	virtual void Deinitialize() override;

	static void SuppressNextWorldMenuOnce();

	UFUNCTION(BlueprintPure, Category="WildBound|MainMenu")
	bool IsMainMenuOpen() const { return bMainMenuOpen; }

	bool IsSettingsPageOpen() const { return bSettingsPageOpen; }
	bool IsLoadPageOpen() const { return bLoadPageOpen; }
	bool IsNewGameConfirmationOpen() const { return bNewGameConfirmationOpen; }

	void ContinueLastGame();
	void RequestNewGame();
	void ConfirmNewGame();
	void CancelNewGame();
	void OpenLoadPage();
	void OpenSettingsPage();
	void BackToRoot();
	void LoadSelectedSave();
	void QuitGame();

	void ChangeGraphicsQuality(int32 Delta);
	void ToggleVSync();
	void CycleFrameRateLimit();

	FText GetGraphicsQualityText() const;
	FText GetVSyncText() const;
	FText GetFrameRateLimitText() const;
	bool HasSaveGame() const;

private:
	TSharedPtr<SWidget> MainMenuViewportRoot;
	FTimerHandle StartupTimer;
	bool bMainMenuOpen = false;
	bool bSettingsPageOpen = false;
	bool bLoadPageOpen = false;
	bool bNewGameConfirmationOpen = false;

	void TryOpenMainMenu();
	void OpenMainMenu();
	void CloseMainMenu();
	void EnsureMainMenuWidget();
	void RemoveMainMenuWidget();
	void ApplyMenuState(bool bOpen);
};
