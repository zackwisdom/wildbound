#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "TimerManager.h"
#include "WildBoundDeathSubsystem.generated.h"

class SWidget;
class UWildBoundSurvivalComponent;
class UWorld;

UCLASS()
class WILDBOUND_API UWildBoundDeathSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	virtual void OnWorldBeginPlay(UWorld& InWorld) override;
	virtual void Deinitialize() override;

	UFUNCTION(BlueprintPure, Category="WildBound|Death")
	bool IsGameOverOpen() const { return bGameOverOpen; }

	bool IsRestartConfirmationOpen() const { return bRestartConfirmationOpen; }
	FText GetDeathCauseText() const;

	void LoadLastSave();
	void RequestRestartFresh();
	void ConfirmRestartFresh();
	void CancelRestartFresh();
	void ReturnToTitle();

private:
	FTimerHandle DeathCheckTimer;
	TSharedPtr<SWidget> DeathViewportRoot;
	TWeakObjectPtr<UWildBoundSurvivalComponent> SurvivalComponent;
	bool bGameOverOpen = false;
	bool bRestartConfirmationOpen = false;
	FString DeathCause = TEXT("CRITICAL INJURIES");

	void CheckPlayerState();
	void ShowGameOver(UWildBoundSurvivalComponent& Survival);
	void EnsureDeathWidget();
	void RemoveDeathWidget();
	void ApplyDeathInputState(bool bActive);
};
