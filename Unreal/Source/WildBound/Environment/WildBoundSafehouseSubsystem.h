#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "TimerManager.h"
#include "WildBoundSafehouseSubsystem.generated.h"

UCLASS()
class WILDBOUND_API UWildBoundSafehouseSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	virtual void OnWorldBeginPlay(UWorld& InWorld) override;
	virtual void Deinitialize() override;

private:
	FTimerHandle SafehouseSpawnTimer;
	bool bSafehouseSpawned = false;

	void TrySpawnSafehouse();
};
