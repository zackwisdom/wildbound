#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "WildBoundBackpackComponent.generated.h"

class SWidget;
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
	TSharedPtr<SWidget> EncumbranceViewportRoot;
	bool bBackpackOpen = false;

	void EnsureBackpackWidget();
	void EnsureEncumbranceWarning();
	void ToggleBackpack();
	void RemoveBackpackWidget();
};
