#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "WildBoundBackpackComponent.generated.h"

class SWildBoundBackpackWidget;
class UWildBoundInventoryComponent;

UCLASS(ClassGroup=(WildBound), meta=(BlueprintSpawnableComponent))
class WILDBOUND_API UWildBoundBackpackComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UWildBoundBackpackComponent();

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	UFUNCTION(BlueprintPure, Category="WildBound|Backpack")
	bool IsBackpackOpen() const { return bBackpackOpen; }

private:
	TWeakObjectPtr<UWildBoundInventoryComponent> InventoryComponent;
	TSharedPtr<SWildBoundBackpackWidget> BackpackWidget;
	TSharedPtr<SWidget> BackpackViewportRoot;
	bool bBackpackOpen = false;

	void EnsureBackpackWidget();
	void ToggleBackpack();
	void RemoveBackpackWidget();
};
