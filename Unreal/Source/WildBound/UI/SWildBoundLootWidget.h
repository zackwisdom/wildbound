#pragma once

#include "CoreMinimal.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/SCompoundWidget.h"

class SVerticalBox;
class UWildBoundInteractionComponent;

class SWildBoundLootWidget : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SWildBoundLootWidget) {}
		SLATE_ARGUMENT(TWeakObjectPtr<UWildBoundInteractionComponent>, InteractionComponent)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);
	void SetInteractionComponent(UWildBoundInteractionComponent* InInteractionComponent);
	virtual void Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime) override;

private:
	TWeakObjectPtr<UWildBoundInteractionComponent> InteractionComponent;
	TSharedPtr<SVerticalBox> LootRowsBox;
	TSharedPtr<SVerticalBox> BackpackRowsBox;
	uint32 CachedStateSignature = 0;

	void RebuildRows();
	void RebuildLootRows();
	void RebuildBackpackRows();
	uint32 CalculateStateSignature() const;
	FText GetHeaderText() const;
	FText GetCarryText() const;
	FReply HandleTakeAll();
	FReply HandleClose();
};
