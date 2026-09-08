#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "TimerManager.h"
#include "WildBoundOutskirtsExpansionSubsystem.generated.h"

UCLASS()
class WILDBOUND_API UWildBoundOutskirtsExpansionSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	virtual void OnWorldBeginPlay(UWorld& InWorld) override;
	virtual void Deinitialize() override;

private:
	void TryBuildOutskirts();

	FTimerHandle OutskirtsBuildTimer;
	bool bOutskirtsBuilt = false;
};
