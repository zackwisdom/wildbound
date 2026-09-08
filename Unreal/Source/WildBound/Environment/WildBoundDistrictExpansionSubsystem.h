#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "TimerManager.h"
#include "WildBoundDistrictExpansionSubsystem.generated.h"

UCLASS()
class WILDBOUND_API UWildBoundDistrictExpansionSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	virtual void OnWorldBeginPlay(UWorld& InWorld) override;
	virtual void Deinitialize() override;

private:
	void TryBuildDistrict();

	FTimerHandle DistrictBuildTimer;
	bool bDistrictBuilt = false;
};
