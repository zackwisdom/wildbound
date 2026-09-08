#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"

class UWildBoundSurvivalComponent;

class SWildBoundInteractionPromptWidget : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SWildBoundInteractionPromptWidget) {}
		SLATE_ARGUMENT(TWeakObjectPtr<UWildBoundSurvivalComponent>, SurvivalComponent)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

private:
	TWeakObjectPtr<UWildBoundSurvivalComponent> SurvivalComponent;

	FText GetPromptText() const;
	EVisibility GetPromptVisibility() const;
};
