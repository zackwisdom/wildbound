#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "TimerManager.h"
#include "WildBoundLootRaritySubsystem.generated.h"

UCLASS()
class WILDBOUND_API UWildBoundLootRaritySubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	virtual void OnWorldBeginPlay(UWorld& InWorld) override;
	virtual void Deinitialize() override;

private:
	FTimerHandle LootRaritySetupTimer;
	bool bLootRaritySetupComplete = false;

	void TrySetupLootRarity();
};
