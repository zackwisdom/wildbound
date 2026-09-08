#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "TimerManager.h"
#include "WildBoundTownLootSubsystem.generated.h"

UCLASS()
class WILDBOUND_API UWildBoundTownLootSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	virtual void OnWorldBeginPlay(UWorld& InWorld) override;
	virtual void Deinitialize() override;

private:
	void TryBuildTownLoot();

	FTimerHandle LootBuildTimer;
	bool bLootBuilt = false;
};
