#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "TimerManager.h"
#include "WildBoundCraftingSubsystem.generated.h"

UCLASS()
class WILDBOUND_API UWildBoundCraftingSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	virtual void OnWorldBeginPlay(UWorld& InWorld) override;
	virtual void Deinitialize() override;

private:
	void EnsureCraftingSetup();

	FTimerHandle CraftingSetupTimer;
};
