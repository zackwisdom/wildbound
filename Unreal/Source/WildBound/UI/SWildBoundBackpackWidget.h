#pragma once

#include "CoreMinimal.h"
#include "Styling/SlateColor.h"
#include "Widgets/SCompoundWidget.h"

class SVerticalBox;
class SWidget;
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
	virtual void Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime) override;
	virtual FReply OnDrop(const FGeometry& MyGeometry, const FDragDropEvent& DragDropEvent) override;

private:
	TWeakObjectPtr<UWildBoundBackpackComponent> BackpackComponent;
	TSharedPtr<SVerticalBox> InventoryRowsBox;
	TSharedPtr<SWidget> InventoryPanel;
	uint32 CachedInventorySignature = 0;

	const UWildBoundInventoryComponent* GetInventory() const;
	void RebuildInventoryRows();
	uint32 CalculateInventorySignature() const;
	TOptional<float> GetWeightPercent() const;
	FText GetWeightText() const;
	FText GetStatusText() const;
	FSlateColor GetStatusColor() const;
	FText GetSelectedItemText() const;
};
