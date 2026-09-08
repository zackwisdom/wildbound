#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "TimerManager.h"
#include "WildBoundStoreStorySubsystem.generated.h"

UCLASS()
class WILDBOUND_API UWildBoundStoreStorySubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	virtual void OnWorldBeginPlay(UWorld& InWorld) override;
	virtual void Deinitialize() override;

private:
	void TrySpawnStoryPass();

	FTimerHandle StorySpawnTimer;
};
