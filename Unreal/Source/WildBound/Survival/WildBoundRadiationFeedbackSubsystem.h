#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "TimerManager.h"
#include "WildBoundRadiationFeedbackSubsystem.generated.h"

class APawn;
class SWidget;
class UWildBoundRadiationComponent;
class UWorld;

UCLASS()
class WILDBOUND_API UWildBoundRadiationFeedbackSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	virtual void OnWorldBeginPlay(UWorld& InWorld) override;
	virtual void Deinitialize() override;

private:
	FTimerHandle FeedbackTimer;
	TSharedPtr<SWidget> RadiationViewportRoot;
	TWeakObjectPtr<APawn> FeedbackPawn;
	TWeakObjectPtr<UWildBoundRadiationComponent> RadiationComponent;
	float NextSicknessCueTime = 0.0f;

	void UpdateFeedback();
	void EnsureVisualFeedback(APawn& Pawn, UWildBoundRadiationComponent& Radiation);
	void RemoveVisualFeedback();
	void PlaySicknessCue(UWorld& World, APawn& Pawn, float Severity);
};
