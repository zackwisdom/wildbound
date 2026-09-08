#include "SWildBoundCraftingWidget.h"

#include "../Crafting/WildBoundCraftingComponent.h"
#include "../Inventory/WildBoundInventoryComponent.h"
#include "Styling/CoreStyle.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SSeparator.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Text/STextBlock.h"

namespace
{
	const FName ReinforcedBackpackItemId(TEXT("ReinforcedBackpack"));
	const FName FilterMaskItemId(TEXT("FilterMask"));
	const FName CanteenItemId(TEXT("Canteen"));
	const FName UtilityBeltItemId(TEXT("UtilityBelt"));

	const FWildBoundCraftingRecipe* GetSelectedRecipe(const UWildBoundCraftingComponent* Crafting)
	{
		if (!Crafting)
		{
			return nullptr;
		}
		const TArray<FWildBoundCraftingRecipe>& Recipes = Crafting->GetRecipes();
		const int32 Index = Crafting->GetSelectedRecipeIndex();
		return Recipes.IsValidIndex(Index) ? &Recipes[Index] : nullptr;
	}

	bool IsUniqueGearItem(const FName& ItemId)
	{
		return ItemId == ReinforcedBackpackItemId
			|| ItemId == FilterMaskItemId
			|| ItemId == CanteenItemId
			|| ItemId == UtilityBeltItemId;
	}

	bool IsRecipeOwned(const UWildBoundCraftingComponent* Crafting, int32 RecipeIndex)
	{
		if (!Crafting || !Crafting->GetRecipes().IsValidIndex(RecipeIndex))
		{
			return false;
		}
		const UWildBoundInventoryComponent* Inventory = Crafting->GetInventoryComponent();
		const FName OutputItemId = Crafting->GetRecipes()[RecipeIndex].OutputItemId;
		return Inventory && IsUniqueGearItem(OutputItemId) && Inventory->HasItem(OutputItemId, 1);
	}
}

void SWildBoundCraftingWidget::Construct(const FArguments& InArgs)
{
	CraftingComponent = InArgs._CraftingComponent;

	ChildSlot
	[
		SNew(SBox)
		.WidthOverride(820.0f)
		[
			SNew(SBorder)
			.Padding(FMargin(24.0f, 20.0f))
			.BorderBackgroundColor(FLinearColor(0.012f, 0.016f, 0.015f, 0.97f))
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot().AutoHeight()
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot().FillWidth(1.0f)
					[
						SNew(STextBlock)
						.Text(FText::FromString(TEXT("CRAFTING")))
						.Font(FCoreStyle::GetDefaultFontStyle("Bold", 18))
					]
					+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
					[
						SNew(STextBlock)
						.Text(FText::FromString(TEXT("C / ESC CLOSE")))
						.Font(FCoreStyle::GetDefaultFontStyle("Regular", 9))
						.ColorAndOpacity(FLinearColor(0.58f, 0.61f, 0.57f, 1.0f))
					]
				]
				+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 8.0f, 0.0f, 14.0f)
				[
					SNew(SSeparator)
				]
				+ SVerticalBox::Slot().AutoHeight()
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot().FillWidth(0.42f).Padding(0.0f, 0.0f, 16.0f, 0.0f)
					[
						SNew(SBorder)
						.Padding(FMargin(14.0f, 12.0f))
						.BorderBackgroundColor(FLinearColor(0.032f, 0.041f, 0.038f, 0.90f))
						[
							SNew(SVerticalBox)
							+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 0.0f, 0.0f, 10.0f)
							[
								SNew(STextBlock).Text(FText::FromString(TEXT("RECIPES"))).Font(FCoreStyle::GetDefaultFontStyle("Bold", 11))
							]
							+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 3.0f)
							[
								SNew(STextBlock).Text_Lambda([this]() { return GetRecipeTitle(0); }).ColorAndOpacity_Lambda([this]() { return GetRecipeTextColor(0); }).Font(FCoreStyle::GetDefaultFontStyle("Bold", 10))
							]
							+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 3.0f)
							[
								SNew(STextBlock).Text_Lambda([this]() { return GetRecipeTitle(1); }).ColorAndOpacity_Lambda([this]() { return GetRecipeTextColor(1); }).Font(FCoreStyle::GetDefaultFontStyle("Bold", 10))
							]
							+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 3.0f)
							[
								SNew(STextBlock).Text_Lambda([this]() { return GetRecipeTitle(2); }).ColorAndOpacity_Lambda([this]() { return GetRecipeTextColor(2); }).Font(FCoreStyle::GetDefaultFontStyle("Bold", 10))
							]
							+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 3.0f)
							[
								SNew(STextBlock).Text_Lambda([this]() { return GetRecipeTitle(3); }).ColorAndOpacity_Lambda([this]() { return GetRecipeTextColor(3); }).Font(FCoreStyle::GetDefaultFontStyle("Bold", 10))
							]
							+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 3.0f)
							[
								SNew(STextBlock).Text_Lambda([this]() { return GetRecipeTitle(4); }).ColorAndOpacity_Lambda([this]() { return GetRecipeTextColor(4); }).Font(FCoreStyle::GetDefaultFontStyle("Bold", 10))
							]
							+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 3.0f)
							[
								SNew(STextBlock).Text_Lambda([this]() { return GetRecipeTitle(5); }).ColorAndOpacity_Lambda([this]() { return GetRecipeTextColor(5); }).Font(FCoreStyle::GetDefaultFontStyle("Bold", 10))
							]
							+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 16.0f, 0.0f, 0.0f)
							[
								SNew(STextBlock).Text(FText::FromString(TEXT("UP / DOWN   SELECT"))).Font(FCoreStyle::GetDefaultFontStyle("Regular", 9)).ColorAndOpacity(FLinearColor(0.60f, 0.63f, 0.58f, 1.0f))
							]
						]
					]
					+ SHorizontalBox::Slot().FillWidth(0.58f)
					[
						SNew(SBorder)
						.Padding(FMargin(18.0f, 14.0f))
						.BorderBackgroundColor(FLinearColor(0.035f, 0.045f, 0.041f, 0.90f))
						[
							SNew(SVerticalBox)
							+ SVerticalBox::Slot().AutoHeight()
							[
								SNew(STextBlock).Text(this, &SWildBoundCraftingWidget::GetSelectedRecipeName).Font(FCoreStyle::GetDefaultFontStyle("Bold", 15))
							]
							+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 5.0f, 0.0f, 12.0f)
							[
								SNew(STextBlock).Text(this, &SWildBoundCraftingWidget::GetSelectedRecipeDescription).AutoWrapText(true).Font(FCoreStyle::GetDefaultFontStyle("Regular", 10)).ColorAndOpacity(FLinearColor(0.72f, 0.74f, 0.69f, 1.0f))
							]
							+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 0.0f, 0.0f, 8.0f)
							[
								SNew(STextBlock).Text(this, &SWildBoundCraftingWidget::GetSelectedRecipeOutput).Font(FCoreStyle::GetDefaultFontStyle("Bold", 11))
							]
							+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 4.0f, 0.0f, 10.0f)
							[
								SNew(SSeparator)
							]
							+ SVerticalBox::Slot().AutoHeight()
							[
								SNew(STextBlock).Text(FText::FromString(TEXT("REQUIRED MATERIALS"))).Font(FCoreStyle::GetDefaultFontStyle("Bold", 10))
							]
							+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 5.0f, 0.0f, 8.0f)
							[
								SNew(STextBlock).Text(this, &SWildBoundCraftingWidget::GetSelectedRecipeRequirements).Font(FCoreStyle::GetDefaultFontStyle("Mono", 10)).ColorAndOpacity(FLinearColor(0.84f, 0.87f, 0.80f, 1.0f))
							]
							+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 0.0f, 0.0f, 12.0f)
							[
								SNew(STextBlock).Text(this, &SWildBoundCraftingWidget::GetSelectedRecipeWeightChange).Font(FCoreStyle::GetDefaultFontStyle("Regular", 9)).ColorAndOpacity(FLinearColor(0.63f, 0.66f, 0.61f, 1.0f))
							]
							+ SVerticalBox::Slot().AutoHeight()
							[
								SNew(SBorder)
								.Padding(FMargin(12.0f, 9.0f))
								.BorderBackgroundColor(FLinearColor(0.025f, 0.032f, 0.029f, 0.95f))
								[
									SNew(STextBlock).Text(this, &SWildBoundCraftingWidget::GetCraftStatusText).ColorAndOpacity(this, &SWildBoundCraftingWidget::GetCraftStatusColor).Font(FCoreStyle::GetDefaultFontStyle("Bold", 11))
								]
							]
						]
					]
				]
			]
		]
	];
}

void SWildBoundCraftingWidget::SetCraftingComponent(UWildBoundCraftingComponent* InCraftingComponent)
{
	CraftingComponent = InCraftingComponent;
}

FSlateColor SWildBoundCraftingWidget::GetRecipeBackground(int32 RecipeIndex) const
{
	const UWildBoundCraftingComponent* Crafting = CraftingComponent.Get();
	return FSlateColor(Crafting && Crafting->GetSelectedRecipeIndex() == RecipeIndex
		? FLinearColor(0.14f, 0.19f, 0.15f, 1.0f)
		: FLinearColor(0.03f, 0.04f, 0.037f, 1.0f));
}

FSlateColor SWildBoundCraftingWidget::GetRecipeTextColor(int32 RecipeIndex) const
{
	const UWildBoundCraftingComponent* Crafting = CraftingComponent.Get();
	if (!Crafting || !Crafting->GetRecipes().IsValidIndex(RecipeIndex))
	{
		return FSlateColor(FLinearColor(0.30f, 0.32f, 0.30f, 1.0f));
	}
	if (Crafting->GetSelectedRecipeIndex() == RecipeIndex)
	{
		return FSlateColor(FLinearColor(0.94f, 0.90f, 0.70f, 1.0f));
	}
	if (IsRecipeOwned(Crafting, RecipeIndex))
	{
		return FSlateColor(FLinearColor(0.48f, 0.68f, 0.76f, 1.0f));
	}
	return Crafting->CanCraftRecipe(RecipeIndex)
		? FSlateColor(FLinearColor(0.66f, 0.80f, 0.60f, 1.0f))
		: FSlateColor(FLinearColor(0.58f, 0.59f, 0.55f, 1.0f));
}

FText SWildBoundCraftingWidget::GetRecipeTitle(int32 RecipeIndex) const
{
	const UWildBoundCraftingComponent* Crafting = CraftingComponent.Get();
	if (!Crafting || !Crafting->GetRecipes().IsValidIndex(RecipeIndex))
	{
		return FText::GetEmpty();
	}

	const FWildBoundCraftingRecipe& Recipe = Crafting->GetRecipes()[RecipeIndex];
	const TCHAR* Marker = Crafting->GetSelectedRecipeIndex() == RecipeIndex ? TEXT(">") : TEXT(" ");
	const TCHAR* Status = IsRecipeOwned(Crafting, RecipeIndex)
		? TEXT("OWNED")
		: (Crafting->CanCraftRecipe(RecipeIndex) ? TEXT("READY") : TEXT("MISSING"));
	return FText::FromString(FString::Printf(TEXT("%s %-28s %s"), Marker, *Recipe.DisplayName, Status));
}

FText SWildBoundCraftingWidget::GetSelectedRecipeName() const
{
	const FWildBoundCraftingRecipe* Recipe = GetSelectedRecipe(CraftingComponent.Get());
	return FText::FromString(Recipe ? Recipe->DisplayName : TEXT("NO RECIPE"));
}

FText SWildBoundCraftingWidget::GetSelectedRecipeDescription() const
{
	const FWildBoundCraftingRecipe* Recipe = GetSelectedRecipe(CraftingComponent.Get());
	return FText::FromString(Recipe ? Recipe->Description : TEXT(""));
}

FText SWildBoundCraftingWidget::GetSelectedRecipeOutput() const
{
	const UWildBoundCraftingComponent* Crafting = CraftingComponent.Get();
	const FWildBoundCraftingRecipe* Recipe = GetSelectedRecipe(Crafting);
	const UWildBoundInventoryComponent* Inventory = Crafting ? Crafting->GetInventoryComponent() : nullptr;
	if (!Recipe || !Inventory)
	{
		return FText::FromString(TEXT("OUTPUT  —"));
	}
	return FText::FromString(FString::Printf(TEXT("OUTPUT  %s x%d"), *Inventory->GetItemDisplayName(Recipe->OutputItemId), Recipe->OutputQuantity));
}

FText SWildBoundCraftingWidget::GetSelectedRecipeRequirements() const
{
	const UWildBoundCraftingComponent* Crafting = CraftingComponent.Get();
	const FWildBoundCraftingRecipe* Recipe = GetSelectedRecipe(Crafting);
	const UWildBoundInventoryComponent* Inventory = Crafting ? Crafting->GetInventoryComponent() : nullptr;
	if (!Recipe || !Inventory)
	{
		return FText::FromString(TEXT("No inventory data"));
	}

	FString Result;
	for (const FWildBoundCraftingIngredient& Requirement : Recipe->Ingredients)
	{
		const int32 Owned = Inventory->GetItemCount(Requirement.ItemId);
		const TCHAR* Marker = Owned >= Requirement.Quantity ? TEXT("[OK]") : TEXT("[  ]");
		Result += FString::Printf(TEXT("%s %-19s  %d / %d\n"), Marker, *Inventory->GetItemDisplayName(Requirement.ItemId), Owned, Requirement.Quantity);
	}
	return FText::FromString(Result);
}

FText SWildBoundCraftingWidget::GetSelectedRecipeWeightChange() const
{
	const UWildBoundCraftingComponent* Crafting = CraftingComponent.Get();
	const FWildBoundCraftingRecipe* Recipe = GetSelectedRecipe(Crafting);
	const UWildBoundInventoryComponent* Inventory = Crafting ? Crafting->GetInventoryComponent() : nullptr;
	if (!Recipe || !Inventory)
	{
		return FText::GetEmpty();
	}

	float IngredientWeight = 0.0f;
	for (const FWildBoundCraftingIngredient& Requirement : Recipe->Ingredients)
	{
		IngredientWeight += Inventory->GetItemUnitWeight(Requirement.ItemId) * Requirement.Quantity;
	}
	const float OutputWeight = Inventory->GetItemUnitWeight(Recipe->OutputItemId) * Recipe->OutputQuantity;
	const float Delta = OutputWeight - IngredientWeight;
	return FText::FromString(FString::Printf(TEXT("PACK WEIGHT CHANGE  %+.2f kg   |   CURRENT %.2f / %.2f kg"), Delta, Inventory->GetTotalWeight(), Inventory->MaxCarryWeight));
}

FText SWildBoundCraftingWidget::GetCraftStatusText() const
{
	const UWildBoundCraftingComponent* Crafting = CraftingComponent.Get();
	if (!Crafting)
	{
		return FText::FromString(TEXT("CRAFTING UNAVAILABLE"));
	}
	const int32 Index = Crafting->GetSelectedRecipeIndex();
	if (IsRecipeOwned(Crafting, Index))
	{
		return FText::FromString(TEXT("OWNED — PASSIVE GEAR ALREADY ACTIVE"));
	}
	return Crafting->CanCraftRecipe(Index)
		? FText::FromString(TEXT("ENTER   CRAFT ITEM"))
		: FText::FromString(TEXT("MISSING MATERIALS"));
}

FSlateColor SWildBoundCraftingWidget::GetCraftStatusColor() const
{
	const UWildBoundCraftingComponent* Crafting = CraftingComponent.Get();
	if (Crafting && IsRecipeOwned(Crafting, Crafting->GetSelectedRecipeIndex()))
	{
		return FSlateColor(FLinearColor(0.48f, 0.68f, 0.76f, 1.0f));
	}
	const bool bReady = Crafting && Crafting->CanCraftRecipe(Crafting->GetSelectedRecipeIndex());
	return bReady
		? FSlateColor(FLinearColor(0.57f, 0.83f, 0.49f, 1.0f))
		: FSlateColor(FLinearColor(0.88f, 0.42f, 0.22f, 1.0f));
}
