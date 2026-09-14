#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "TimerManager.h"
#include "WildBoundTreatmentFeedbackSubsystem.generated.h"

class SWidget;

UCLASS()
class WILDBOUND_API UWildBoundTreatmentFeedbackSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	virtual void OnWorldBeginPlay(UWorld& InWorld) override;
	virtual void Deinitialize() override;

private:
	FTimerHandle TreatmentFeedbackSetupTimer;
	TSharedPtr<SWidget> TreatmentViewportRoot;

	void EnsureTreatmentFeedback();
	void RemoveTreatmentFeedback();
};
