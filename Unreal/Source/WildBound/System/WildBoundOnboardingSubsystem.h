#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "TimerManager.h"
#include "WildBoundOnboardingSubsystem.generated.h"

class SWidget;
class UWorld;

UCLASS()
class WILDBOUND_API UWildBoundOnboardingSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	virtual void OnWorldBeginPlay(UWorld& InWorld) override;
	virtual void Deinitialize() override;

	int32 GetProgressStage() const { return ProgressStage; }
	int32 GetTutorialHintFlags() const { return TutorialHintFlags; }
	void RestorePersistentState(int32 SavedProgressStage, int32 SavedHintFlags);

	FText GetObjectiveTitle() const;
	FText GetObjectiveText() const;
	FText GetControlHintText() const;
	FText GetTutorialToastText() const;
	float GetIntroOpacity() const;
	bool IsIntroVisible() const;
	bool IsObjectiveVisible() const;
	bool IsTutorialToastVisible() const;
	bool ShouldShowOnboardingUI() const;

private:
	TSharedPtr<SWidget> OnboardingViewportRoot;
	FTimerHandle UpdateTimer;
	int32 ProgressStage = 0;
	int32 TutorialHintFlags = 0;
	FVector RunStartLocation = FVector::ZeroVector;
	bool bRunStartCaptured = false;
	bool bIntroActive = false;
	float IntroStartedAt = 0.0f;
	float IntroDuration = 4.25f;
	float IntroInputLockDuration = 1.15f;
	FString TutorialToast;
	float TutorialToastExpiresAt = -1.0f;

	void UpdateOnboarding();
	void StartNewRunOnboarding();
	void AdvanceStage(int32 NewStage);
	void EvaluateObjectiveProgress();
	void EvaluateContextualTutorials();
	void ShowTutorialToast(const FString& Text, float DurationSeconds = 5.0f);
	void EnsureOnboardingWidget();
	void RemoveOnboardingWidget();
	void SetIntroInputLock(bool bLocked);
	bool HasSearchedContainer() const;
};
