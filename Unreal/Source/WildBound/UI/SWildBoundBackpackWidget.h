#pragma once

#include "CoreMinimal.h"
#include "Styling/SlateColor.h"
#include "Widgets/SCompoundWidget.h"

class UWildBoundBackpackComponent;
class UWildBoundInventoryComponent;

class SWildBoundBackpackWidget : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SWildBoundBackpackWidget) {}
		SLATE_ARGUMENT(TWeakObjectPtr<UWildBoundBackpackComponent>, BackpackComponent)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);
	void SetBackpackComponent(UWildBoundBackpackComponent* InBackpackComponent);

private:
	TWeakObjectPtr<UWildBoundBackpackComponent> BackpackComponent;

	const UWildBoundInventoryComponent* GetInventory() const;
	TOptional<float> GetWeightPercent() const;
	FText GetWeightText() const;
	FText GetStatusText() const;
	FSlateColor GetStatusColor() const;
	FText GetHotbarText() const;
	FText GetInventoryListText() const;
	FText GetSelectedItemText() const;
};
