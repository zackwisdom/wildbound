#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "TimerManager.h"
#include "WildBoundScavengeFeedbackSubsystem.generated.h"

class AActor;
class UWildBoundInteractionComponent;

UCLASS()
class WILDBOUND_API UWildBoundScavengeFeedbackSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	virtual void OnWorldBeginPlay(UWorld& InWorld) override;
	virtual void Deinitialize() override;

private:
	FTimerHandle FeedbackTimer;
	TWeakObjectPtr<AActor> LastTargetedContainer;
	TWeakObjectPtr<AActor> OpenContainer;
	FRotator AppliedOpenRotation = FRotator::ZeroRotator;
	FVector AppliedOpenOffset = FVector::ZeroVector;
	bool bOpenPoseApplied = false;

	void UpdateScavengeFeedback();
	void RefreshSearchedContainerStates();
	void UpdateContextPrompt(UWildBoundInteractionComponent& Interaction);
	void UpdateOpenContainerState(UWildBoundInteractionComponent& Interaction);
	void ApplyContainerOpenPose(AActor& Container);
	void RestoreContainerOpenPose(AActor& Container);
	void PlayContainerTransitionSound(const AActor& Container, bool bOpening) const;
};
