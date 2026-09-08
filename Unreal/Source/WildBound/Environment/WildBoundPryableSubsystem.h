#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "TimerManager.h"
#include "WildBoundPryableSubsystem.generated.h"

UCLASS()
class WILDBOUND_API UWildBoundPryableSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	virtual void OnWorldBeginPlay(UWorld& InWorld) override;
	virtual void Deinitialize() override;

private:
	void TryBuildPryables();

	FTimerHandle PryableBuildTimer;
	bool bPryablesBuilt = false;
};
