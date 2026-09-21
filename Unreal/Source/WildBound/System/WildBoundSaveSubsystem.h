#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "TimerManager.h"
#include "WildBoundSaveSubsystem.generated.h"

class UWildBoundSaveGame;
class UWorld;

UCLASS()
class WILDBOUND_API UWildBoundSaveSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	virtual void OnWorldBeginPlay(UWorld& InWorld) override;
	virtual void Deinitialize() override;

	UFUNCTION(BlueprintCallable, Category="WildBound|Save")
	bool SaveNow(bool bShowMessage = false);

	UFUNCTION(BlueprintCallable, Category="WildBound|Save")
	bool LoadNow(bool bShowMessage = true);

	UFUNCTION(BlueprintPure, Category="WildBound|Save")
	bool HasSaveGame() const;

	UFUNCTION(BlueprintCallable, Category="WildBound|Save")
	bool ReloadLastSave();

	UFUNCTION(BlueprintCallable, Category="WildBound|Save")
	bool StartNewGame();

	UFUNCTION(BlueprintPure, Category="WildBound|Save")
	bool HasStartedSession() const { return bSessionStarted; }

	bool IsPersistenceReady() const { return bInitialized; }

private:
	FTimerHandle StartupTimer;
	FTimerHandle AutosaveTimer;
	bool bInitialized = false;
	bool bApplyingLoad = false;
	bool bSuppressExitSave = false;
	bool bSessionStarted = false;

	const FString SaveSlotName = TEXT("WildBound_Autosave");
	static constexpr int32 SaveUserIndex = 0;
	static constexpr float AutosaveIntervalSeconds = 30.0f;

	void TryInitializePersistence();
	void PerformAutosave();
	bool CaptureSave(UWildBoundSaveGame& SaveGame) const;
	bool ApplySave(const UWildBoundSaveGame& SaveGame);
	bool ArePersistenceTargetsReady() const;
	bool HasActorsWithTag(FName Tag) const;
	void DestroyActorsWithTag(FName Tag) const;
};
