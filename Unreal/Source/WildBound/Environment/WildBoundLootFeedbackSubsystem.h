#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "TimerManager.h"
#include "WildBoundLootFeedbackSubsystem.generated.h"

class AActor;

UCLASS()
class WILDBOUND_API UWildBoundLootFeedbackSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	virtual void OnWorldBeginPlay(UWorld& InWorld) override;
	virtual void Deinitialize() override;

private:
	FTimerHandle FeedbackTimer;
	TSet<TWeakObjectPtr<AActor>> ProcessedContainers;
	TMap<FName, int32> PreviousInventoryCounts;
	bool bInventorySnapshotInitialized = false;

	void UpdateLootFeedback();
	void ProcessOpenedContainers();
	void ProcessInventoryPickupFeedback();
};
