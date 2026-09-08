#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "TimerManager.h"
#include "WildBoundWorkbenchSubsystem.generated.h"

UCLASS()
class WILDBOUND_API UWildBoundWorkbenchSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	virtual void OnWorldBeginPlay(UWorld& InWorld) override;
	virtual void Deinitialize() override;

private:
	void TrySpawnWorkbenches();

	FTimerHandle WorkbenchSpawnTimer;
	bool bWorkbenchesSpawned = false;
};
