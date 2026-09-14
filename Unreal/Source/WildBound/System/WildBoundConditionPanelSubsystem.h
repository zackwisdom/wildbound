#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "TimerManager.h"
#include "WildBoundConditionPanelSubsystem.generated.h"

class SWidget;
class UWildBoundBackpackComponent;

UCLASS()
class WILDBOUND_API UWildBoundConditionPanelSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	virtual void OnWorldBeginPlay(UWorld& InWorld) override;
	virtual void Deinitialize() override;

private:
	TWeakObjectPtr<UWildBoundBackpackComponent> BackpackComponent;
	TSharedPtr<SWidget> ConditionViewportRoot;
	FTimerHandle ConditionPanelSetupTimer;

	void EnsureConditionPanel();
	void RemoveConditionPanel();
};
