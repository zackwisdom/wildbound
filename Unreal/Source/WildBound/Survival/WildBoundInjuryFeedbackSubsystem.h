#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "TimerManager.h"
#include "WildBoundInjuryFeedbackSubsystem.generated.h"

class APawn;
class SWidget;
class UWildBoundInjuryComponent;
class UWorld;

UCLASS()
class WILDBOUND_API UWildBoundInjuryFeedbackSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	virtual void OnWorldBeginPlay(UWorld& InWorld) override;
	virtual void Deinitialize() override;

private:
	FTimerHandle FeedbackTimer;
	TSharedPtr<SWidget> InjuryViewportRoot;
	TWeakObjectPtr<APawn> FeedbackPawn;
	TWeakObjectPtr<UWildBoundInjuryComponent> InjuryComponent;
	float NextBleedingCueTime = 0.0f;
	float NextFractureCueTime = 0.0f;
	float NextPainCueTime = 0.0f;

	void UpdateFeedback();
	void EnsureVisualFeedback(APawn& Pawn, UWildBoundInjuryComponent& Injury);
	void RemoveVisualFeedback();
	void PlayBleedingCue(UWorld& World, APawn& Pawn, float Severity);
	void PlayFractureCue(UWorld& World, APawn& Pawn, float Severity);
	void PlayPainCue(UWorld& World, APawn& Pawn, float Severity);
};
