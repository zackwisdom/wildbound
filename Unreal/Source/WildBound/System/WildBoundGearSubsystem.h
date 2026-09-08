#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "TimerManager.h"
#include "WildBoundGearSubsystem.generated.h"

UCLASS()
class WILDBOUND_API UWildBoundGearSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	virtual void OnWorldBeginPlay(UWorld& InWorld) override;
	virtual void Deinitialize() override;

private:
	void EnsureGearSetup();
	FTimerHandle GearSetupTimer;
};
