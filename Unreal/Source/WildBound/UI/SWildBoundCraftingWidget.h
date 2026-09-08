#pragma once

#include "CoreMinimal.h"
#include "Styling/SlateColor.h"
#include "Widgets/SCompoundWidget.h"

class UWildBoundCraftingComponent;

class SWildBoundCraftingWidget : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SWildBoundCraftingWidget) {}
		SLATE_ARGUMENT(TWeakObjectPtr<UWildBoundCraftingComponent>, CraftingComponent)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);
	void SetCraftingComponent(UWildBoundCraftingComponent* InCraftingComponent);

private:
	TWeakObjectPtr<UWildBoundCraftingComponent> CraftingComponent;

	FSlateColor GetRecipeBackground(int32 RecipeIndex) const;
	FSlateColor GetRecipeTextColor(int32 RecipeIndex) const;
	FText GetRecipeTitle(int32 RecipeIndex) const;
	FText GetSelectedRecipeName() const;
	FText GetSelectedRecipeDescription() const;
	FText GetSelectedRecipeOutput() const;
	FText GetSelectedRecipeRequirements() const;
	FText GetSelectedRecipeWeightChange() const;
	FText GetCraftStatusText() const;
	FSlateColor GetCraftStatusColor() const;
};
