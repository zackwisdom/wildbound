#pragma once

#include "CoreMinimal.h"
#include "Styling/SlateColor.h"
#include "Widgets/SCompoundWidget.h"

class UWildBoundInventoryComponent;

class SWildBoundBackpackWidget : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SWildBoundBackpackWidget) {}
		SLATE_ARGUMENT(TWeakObjectPtr<UWildBoundInventoryComponent>, InventoryComponent)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);
	void SetInventoryComponent(UWildBoundInventoryComponent* InInventoryComponent);

private:
	TWeakObjectPtr<UWildBoundInventoryComponent> InventoryComponent;

	TOptional<float> GetWeightPercent() const;
	FText GetWeightText() const;
	FText GetStatusText() const;
	FSlateColor GetStatusColor() const;
	FText GetSuppliesText() const;
	FText GetMaterialsText() const;
};
