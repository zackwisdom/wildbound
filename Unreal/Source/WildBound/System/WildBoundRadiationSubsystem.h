#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "TimerManager.h"
#include "WildBoundRadiationSubsystem.generated.h"

UCLASS()
class WILDBOUND_API UWildBoundRadiationSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	virtual void OnWorldBeginPlay(UWorld& InWorld) override;
	virtual void Deinitialize() override;

private:
	void TrySpawnHotspot();

	FTimerHandle HotspotSpawnTimer;
	bool bHotspotSpawned = false;
};
