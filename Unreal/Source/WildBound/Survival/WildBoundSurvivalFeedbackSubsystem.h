#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "TimerManager.h"
#include "WildBoundSurvivalFeedbackSubsystem.generated.h"

class APawn;
class UWorld;

UCLASS()
class WILDBOUND_API UWildBoundSurvivalFeedbackSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	virtual void OnWorldBeginPlay(UWorld& InWorld) override;
	virtual void Deinitialize() override;

private:
	FTimerHandle FeedbackTimer;
	float NextHeartbeatTime = 0.0f;
	float NextBreathTime = 0.0f;

	void UpdateFeedback();
	void PlayHeartbeat(UWorld& World, APawn& Pawn, float Severity);
	void PlayBreath(UWorld& World, APawn& Pawn, float Severity);
};
