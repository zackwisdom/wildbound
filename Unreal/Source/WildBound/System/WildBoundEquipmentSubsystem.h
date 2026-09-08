#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "TimerManager.h"
#include "WildBoundEquipmentSubsystem.generated.h"

UCLASS()
class WILDBOUND_API UWildBoundEquipmentSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	virtual void OnWorldBeginPlay(UWorld& InWorld) override;
	virtual void Deinitialize() override;

private:
	void EnsureEquipmentSetup();

	FTimerHandle EquipmentSetupTimer;
};
