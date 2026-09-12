#include "SWildBoundCraftingWidget.h"

#include "../Crafting/WildBoundCraftingComponent.h"
#include "../Inventory/WildBoundInventoryComponent.h"
#include "Styling/CoreStyle.h"
#include "Widgets/Input/SButton.h"
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

	FString CleanRecipeName(const FString& DisplayName)
	{
		FString Result = DisplayName;
		Result.RemoveFromStart(TEXT("HAND: "));
		Result.RemoveFromStart(TEXT("BENCH: "));
		return Result;
	}

	FLinearColor GetRarityColor(const UWildBoundInventoryComponent* Inventory, FName ItemId)
	{
		const int32 Tier = Inventory ? Inventory->GetItemRarityTier(ItemId) : 0;
		switch (Tier)
		{
		case 3: return FLinearColor(0.84f, 0.48f, 0.96f, 1.0f);
		case 2: return FLinearColor(0.38f, 0.67f, 0.96f, 1.0f);
		case 1: return FLinearColor(0.46f, 0.82f, 0.50f, 1.0f);
		default: return FLinearColor(0.80f, 0.82f, 0.77f, 1.0f);
		}
	}

	FLinearColor GetRarityBackground(const UWildBoundInventoryComponent* Inventory, FName ItemId)
	{
		const int32 Tier = Inventory ? Inventory->GetItemRarityTier(ItemId) : 0;
		switch (Tier)
		{
		case 3: return FLinearColor(0.18f, 0.075f, 0.23f, 0.98f);
		case 2: return FLinearColor(0.055f, 0.105f, 0.19f, 0.98f);
		case 1: return FLinearColor(0.055f, 0.145f, 0.070f, 0.98f);
		default: return FLinearColor(0.075f, 0.086f, 0.080f, 0.98f);
		}
	}

	FString GetItemIconCode(FName ItemId, const UWildBoundInventoryComponent* Inventory)
	{
		const FString Id = ItemId.ToString();
		if (Id == TEXT("Water")) return TEXT("H2O");
		if (Id == TEXT("Food")) return TEXT("FOOD");
		if (Id == TEXT("MedicalSupplies")) return TEXT("+");
		if (Id == TEXT("ScrapMetal")) return TEXT("FE");
		if (Id == TEXT("Cloth")) return TEXT("CL");
		if (Id == TEXT("Wood")) return TEXT("WD");
		if (Id == TEXT("Plastic")) return TEXT("PL");
		if (Id == TEXT("Electronics")) return TEXT("PCB");
		if (Id == TEXT("Chemicals")) return TEXT("CHEM");
		if (Id == TEXT("Adhesive")) return TEXT("ADH");
		if (Id == TEXT("Wire")) return TEXT("WIRE");
		if (Id == TEXT("Battery")) return TEXT("BAT");
		if (Id == TEXT("MechanicalParts")) return TEXT("MEC");
		if (Id == TEXT("Flashlight")) return TEXT("LUX");
		if (Id == TEXT("Crowbar")) return TEXT("PRY");
		if (Id == TEXT("ReinforcedBackpack")) return TEXT("PACK");
		if (Id == TEXT("FilterMask")) return TEXT("MASK");
		if (Id == TEXT("Canteen")) return TEXT("CAN");
		if (Id == TEXT("TraumaKit")) return TEXT("AID");
		if (Id == TEXT("RadTreatment")) return TEXT("RAD");
		if (Id == TEXT("UtilityBelt")) return TEXT("BELT");

		const FString Category = Inventory ? Inventory->GetItemCategoryName(ItemId) : FString();
		return Category.IsEmpty() ? TEXT("ITEM") : Category.Left(4);
	}
}

void SWildBoundCraftingWidget::Construct(const FArguments& InArgs)
{
	CraftingComponent = InArgs._CraftingComponent;

	auto MakeRecipeButton = [this](int32 RecipeIndex) -> TSharedRef<SWidget>
	{
		return SNew(SButton)
			.Visibility_Lambda([this, RecipeIndex]()
			{
				const UWildBoundCraftingComponent* Crafting = CraftingComponent.Get();
				return Crafting && Crafting->GetRecipes().IsValidIndex(RecipeIndex)
					? EVisibility::Visible
					: EVisibility::Collapsed;
			})
			.ContentPadding(FMargin(0.0f))
			.OnClicked_Lambda([this, RecipeIndex]()
			{
				if (UWildBoundCraftingComponent* Crafting = CraftingComponent.Get())
				{
					Crafting->SelectRecipeFromMouse(RecipeIndex);
				}
				return FReply::Handled();
			})
			[
				SNew(SBorder)
				.Padding(FMargin(9.0f, 7.0f))
				.BorderBackgroundColor_Lambda([this, RecipeIndex]() { return GetRecipeBackground(RecipeIndex); })
				[
					SNew(STextBlock)
					.Text_Lambda([this, RecipeIndex]() { return GetRecipeTitle(RecipeIndex); })
					.ColorAndOpacity_Lambda([this, RecipeIndex]() { return GetRecipeTextColor(RecipeIndex); })
					.Font(FCoreStyle::GetDefaultFontStyle("Bold", 9))
				]
			];
	};

	ChildSlot
	[
		SNew(SBox)
		.WidthOverride(940.0f)
		[
			SNew(SBorder)
			.Padding(FMargin(22.0f, 18.0f))
			.BorderBackgroundColor(FLinearColor(0.009f, 0.013f, 0.012f, 0.992f))
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot().AutoHeight()
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot().FillWidth(1.0f)
					[
						SNew(SVerticalBox)
						+ SVerticalBox::Slot().AutoHeight()
						[
							SNew(STextBlock)
							.Text_Lambda([this]()
							{
								const UWildBoundCraftingComponent* Crafting = CraftingComponent.Get();
								return FText::FromString(Crafting && Crafting->IsWorkbenchMode()
									? TEXT("WORKBENCH CRAFTING")
									: TEXT("FIELD CRAFTING"));
							})
							.Font(FCoreStyle::GetDefaultFontStyle("Bold", 20))
							.ColorAndOpacity_Lambda([this]()
							{
								const UWildBoundCraftingComponent* Crafting = CraftingComponent.Get();
								return Crafting && Crafting->IsWorkbenchMode()
									? FSlateColor(FLinearColor(0.90f, 0.72f, 0.38f, 1.0f))
									: FSlateColor(FLinearColor(0.60f, 0.82f, 0.58f, 1.0f));
							})
						]
						+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 3.0f, 0.0f, 0.0f)
						[
							SNew(STextBlock)
							.Text_Lambda([this]()
							{
								const UWildBoundCraftingComponent* Crafting = CraftingComponent.Get();
								return FText::FromString(Crafting && Crafting->IsWorkbenchMode()
									? TEXT("ADVANCED ASSEMBLY  /  TOOLS  /  PASSIVE SURVIVAL GEAR")
									: TEXT("IMPROVISED FIELD ASSEMBLY  /  MEDICAL  /  EMERGENCY USE"));
							})
							.Font(FCoreStyle::GetDefaultFontStyle("Bold", 8))
							.ColorAndOpacity(FLinearColor(0.50f, 0.58f, 0.51f, 1.0f))
						]
					]
					+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
					[
						SNew(SVerticalBox)
						+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Right)
						[
							SNew(STextBlock)
							.Text_Lambda([this]()
							{
								const UWildBoundCraftingComponent* Crafting = CraftingComponent.Get();
								const int32 Count = Crafting ? Crafting->GetRecipes().Num() : 0;
								return FText::FromString(FString::Printf(TEXT("%d RECIPES"), Count));
							})
							.Font(FCoreStyle::GetDefaultFontStyle("Bold", 9))
							.ColorAndOpacity(FLinearColor(0.72f, 0.74f, 0.68f, 1.0f))
						]
						+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 3.0f, 0.0f, 0.0f).HAlign(HAlign_Right)
						[
							SNew(STextBlock)
							.Text(FText::FromString(TEXT("CLICK RECIPE  /  C OR ESC CLOSE")))
							.Font(FCoreStyle::GetDefaultFontStyle("Regular", 8))
							.ColorAndOpacity(FLinearColor(0.53f, 0.56f, 0.52f, 1.0f))
						]
					]
				]
				+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 9.0f, 0.0f, 12.0f)
				[
					SNew(SSeparator)
				]
				+ SVerticalBox::Slot().AutoHeight()
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot().FillWidth(0.45f).Padding(0.0f, 0.0f, 14.0f, 0.0f)
					[
						SNew(SBorder)
						.Padding(FMargin(11.0f, 10.0f))
						.BorderBackgroundColor(FLinearColor(0.022f, 0.030f, 0.027f, 0.98f))
						[
							SNew(SVerticalBox)
							+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 0.0f, 0.0f, 7.0f)
							[
								SNew(SHorizontalBox)
								+ SHorizontalBox::Slot().FillWidth(1.0f)
								[
									SNew(STextBlock)
									.Text(FText::FromString(TEXT("BLUEPRINTS")))
									.Font(FCoreStyle::GetDefaultFontStyle("Bold", 10))
								]
								+ SHorizontalBox::Slot().AutoWidth()
								[
									SNew(STextBlock)
									.Text(FText::FromString(TEXT("CLICK TO SELECT")))
									.Font(FCoreStyle::GetDefaultFontStyle("Regular", 7))
									.ColorAndOpacity(FLinearColor(0.50f, 0.54f, 0.50f, 1.0f))
								]
							]
							+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 2.0f)[MakeRecipeButton(0)]
							+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 2.0f)[MakeRecipeButton(1)]
							+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 2.0f)[MakeRecipeButton(2)]
							+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 2.0f)[MakeRecipeButton(3)]
							+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 2.0f)[MakeRecipeButton(4)]
							+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 2.0f)[MakeRecipeButton(5)]
						]
					]
					+ SHorizontalBox::Slot().FillWidth(0.55f)
					[
						SNew(SBorder)
						.Padding(FMargin(15.0f, 12.0f))
						.BorderBackgroundColor(FLinearColor(0.027f, 0.037f, 0.033f, 0.98f))
						[
							SNew(SVerticalBox)
							+ SVerticalBox::Slot().AutoHeight()
							[
								SNew(STextBlock)
								.Text(this, &SWildBoundCraftingWidget::GetSelectedRecipeName)
								.Font(FCoreStyle::GetDefaultFontStyle("Bold", 15))
								.ColorAndOpacity(FLinearColor(0.92f, 0.94f, 0.89f, 1.0f))
							]
							+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 4.0f, 0.0f, 10.0f)
							[
								SNew(STextBlock)
								.Text(this, &SWildBoundCraftingWidget::GetSelectedRecipeDescription)
								.AutoWrapText(true)
								.Font(FCoreStyle::GetDefaultFontStyle("Regular", 9))
								.ColorAndOpacity(FLinearColor(0.69f, 0.73f, 0.68f, 1.0f))
							]
							+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 1.0f, 0.0f, 10.0f)
							[
								SNew(SBorder)
								.Padding(FMargin(10.0f, 9.0f))
								.BorderBackgroundColor_Lambda([this]()
								{
									const UWildBoundCraftingComponent* Crafting = CraftingComponent.Get();
									const FWildBoundCraftingRecipe* Recipe = GetSelectedRecipe(Crafting);
									const UWildBoundInventoryComponent* Inventory = Crafting ? Crafting->GetInventoryComponent() : nullptr;
									return Recipe ? FSlateColor(GetRarityBackground(Inventory, Recipe->OutputItemId)) : FSlateColor(FLinearColor(0.05f, 0.06f, 0.055f, 1.0f));
								})
								[
									SNew(STextBlock)
									.Text(this, &SWildBoundCraftingWidget::GetSelectedRecipeOutput)
									.Font(FCoreStyle::GetDefaultFontStyle("Bold", 10))
									.ColorAndOpacity_Lambda([this]()
									{
										const UWildBoundCraftingComponent* Crafting = CraftingComponent.Get();
										const FWildBoundCraftingRecipe* Recipe = GetSelectedRecipe(Crafting);
										const UWildBoundInventoryComponent* Inventory = Crafting ? Crafting->GetInventoryComponent() : nullptr;
										return Recipe ? FSlateColor(GetRarityColor(Inventory, Recipe->OutputItemId)) : FSlateColor(FLinearColor::White);
									})
								]
							]
							+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 1.0f, 0.0f, 8.0f)
							[
								SNew(SSeparator)
							]
							+ SVerticalBox::Slot().AutoHeight()
							[
								SNew(STextBlock)
								.Text(FText::FromString(TEXT("MATERIAL REQUIREMENTS")))
								.Font(FCoreStyle::GetDefaultFontStyle("Bold", 9))
								.ColorAndOpacity(FLinearColor(0.78f, 0.81f, 0.76f, 1.0f))
							]
							+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 6.0f, 0.0f, 8.0f)
							[
								SNew(SBorder)
								.Padding(FMargin(10.0f, 8.0f))
								.BorderBackgroundColor(FLinearColor(0.018f, 0.024f, 0.022f, 0.98f))
								[
									SNew(STextBlock)
									.Text(this, &SWildBoundCraftingWidget::GetSelectedRecipeRequirements)
									.Font(FCoreStyle::GetDefaultFontStyle("Mono", 9))
									.ColorAndOpacity(FLinearColor(0.82f, 0.85f, 0.79f, 1.0f))
								]
							]
							+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 0.0f, 0.0f, 10.0f)
							[
								SNew(STextBlock)
								.Text(this, &SWildBoundCraftingWidget::GetSelectedRecipeWeightChange)
								.Font(FCoreStyle::GetDefaultFontStyle("Regular", 8))
								.ColorAndOpacity(FLinearColor(0.60f, 0.64f, 0.59f, 1.0f))
							]
							+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 0.0f, 0.0f, 8.0f)
							[
								SNew(SBorder)
								.Padding(FMargin(12.0f, 9.0f))
								.BorderBackgroundColor(FLinearColor(0.018f, 0.025f, 0.021f, 0.99f))
								[
									SNew(STextBlock)
									.Text(this, &SWildBoundCraftingWidget::GetCraftStatusText)
									.ColorAndOpacity(this, &SWildBoundCraftingWidget::GetCraftStatusColor)
									.Font(FCoreStyle::GetDefaultFontStyle("Bold", 10))
									.Justification(ETextJustify::Center)
								]
							]
							+ SVerticalBox::Slot().AutoHeight()
							[
								SNew(SButton)
								.IsEnabled_Lambda([this]()
								{
									const UWildBoundCraftingComponent* Crafting = CraftingComponent.Get();
									return Crafting && Crafting->CanCraftRecipe(Crafting->GetSelectedRecipeIndex());
								})
								.ContentPadding(FMargin(14.0f, 10.0f))
								.OnClicked_Lambda([this]()
								{
									if (UWildBoundCraftingComponent* Crafting = CraftingComponent.Get())
									{
										Crafting->CraftSelectedRecipeFromMouse();
									}
									return FReply::Handled();
								})
								[
									SNew(STextBlock)
									.Text(FText::FromString(TEXT("CRAFT ITEM")))
									.Font(FCoreStyle::GetDefaultFontStyle("Bold", 11))
									.Justification(ETextJustify::Center)
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
	if (!Crafting || !Crafting->GetRecipes().IsValidIndex(RecipeIndex))
	{
		return FSlateColor(FLinearColor(0.025f, 0.030f, 0.028f, 1.0f));
	}

	if (Crafting->GetSelectedRecipeIndex() == RecipeIndex)
	{
		return Crafting->IsWorkbenchMode()
			? FSlateColor(FLinearColor(0.145f, 0.105f, 0.045f, 1.0f))
			: FSlateColor(FLinearColor(0.070f, 0.145f, 0.075f, 1.0f));
	}

	if (IsRecipeOwned(Crafting, RecipeIndex))
	{
		return FSlateColor(FLinearColor(0.040f, 0.075f, 0.095f, 1.0f));
	}

	return Crafting->CanCraftRecipe(RecipeIndex)
		? FSlateColor(FLinearColor(0.034f, 0.065f, 0.040f, 1.0f))
		: FSlateColor(FLinearColor(0.030f, 0.035f, 0.032f, 1.0f));
}

FSlateColor SWildBoundCraftingWidget::GetRecipeTextColor(int32 RecipeIndex) const
{
	const UWildBoundCraftingComponent* Crafting = CraftingComponent.Get();
	if (!Crafting || !Crafting->GetRecipes().IsValidIndex(RecipeIndex))
	{
		return FSlateColor(FLinearColor(0.30f, 0.32f, 0.30f, 1.0f));
	}

	const FWildBoundCraftingRecipe& Recipe = Crafting->GetRecipes()[RecipeIndex];
	const UWildBoundInventoryComponent* Inventory = Crafting->GetInventoryComponent();
	if (Crafting->GetSelectedRecipeIndex() == RecipeIndex)
	{
		return FSlateColor(GetRarityColor(Inventory, Recipe.OutputItemId));
	}
	if (IsRecipeOwned(Crafting, RecipeIndex))
	{
		return FSlateColor(FLinearColor(0.48f, 0.68f, 0.76f, 1.0f));
	}
	return Crafting->CanCraftRecipe(RecipeIndex)
		? FSlateColor(FLinearColor(0.66f, 0.80f, 0.60f, 1.0f))
		: FSlateColor(FLinearColor(0.53f, 0.55f, 0.51f, 1.0f));
}

FText SWildBoundCraftingWidget::GetRecipeTitle(int32 RecipeIndex) const
{
	const UWildBoundCraftingComponent* Crafting = CraftingComponent.Get();
	if (!Crafting || !Crafting->GetRecipes().IsValidIndex(RecipeIndex))
	{
		return FText::GetEmpty();
	}

	const FWildBoundCraftingRecipe& Recipe = Crafting->GetRecipes()[RecipeIndex];
	const UWildBoundInventoryComponent* Inventory = Crafting->GetInventoryComponent();
	const FString Marker = Crafting->GetSelectedRecipeIndex() == RecipeIndex ? TEXT(">") : TEXT(" ");
	const FString Status = IsRecipeOwned(Crafting, RecipeIndex)
		? TEXT("OWNED")
		: (Crafting->CanCraftRecipe(RecipeIndex) ? TEXT("READY") : TEXT("MISSING MATERIALS"));
	const FString Rarity = Inventory ? Inventory->GetItemRarityName(Recipe.OutputItemId) : TEXT("COMMON");

	return FText::FromString(FString::Printf(
		TEXT("%s  [%s]  %s\n     %s"),
		*Marker,
		*Rarity,
		*CleanRecipeName(Recipe.DisplayName),
		*Status));
}

FText SWildBoundCraftingWidget::GetSelectedRecipeName() const
{
	const FWildBoundCraftingRecipe* Recipe = GetSelectedRecipe(CraftingComponent.Get());
	return FText::FromString(Recipe ? CleanRecipeName(Recipe->DisplayName) : TEXT("NO RECIPE"));
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
		return FText::FromString(TEXT("OUTPUT  --"));
	}

	const float OutputWeight = Inventory->GetItemUnitWeight(Recipe->OutputItemId) * static_cast<float>(Recipe->OutputQuantity);
	return FText::FromString(FString::Printf(
		TEXT("OUTPUT   [%s]   %s\n%s   x%d   |   %s   |   %.2f kg"),
		*Inventory->GetItemRarityName(Recipe->OutputItemId),
		*GetItemIconCode(Recipe->OutputItemId, Inventory),
		*Inventory->GetItemDisplayName(Recipe->OutputItemId),
		Recipe->OutputQuantity,
		*Inventory->GetItemCategoryName(Recipe->OutputItemId),
		OutputWeight));
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
		if (Owned >= Requirement.Quantity)
		{
			Result += FString::Printf(
				TEXT("[READY]  %-18s  %d / %d\n"),
				*Inventory->GetItemDisplayName(Requirement.ItemId),
				Owned,
				Requirement.Quantity);
		}
		else
		{
			const int32 Missing = Requirement.Quantity - Owned;
			Result += FString::Printf(
				TEXT("[NEED %d] %-18s  %d / %d\n"),
				Missing,
				*Inventory->GetItemDisplayName(Requirement.ItemId),
				Owned,
				Requirement.Quantity);
		}
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
		IngredientWeight += Inventory->GetItemUnitWeight(Requirement.ItemId) * static_cast<float>(Requirement.Quantity);
	}

	const float OutputWeight = Inventory->GetItemUnitWeight(Recipe->OutputItemId) * static_cast<float>(Recipe->OutputQuantity);
	const float Delta = OutputWeight - IngredientWeight;
	return FText::FromString(FString::Printf(
		TEXT("PACK WEIGHT   %+.2f kg after craft   |   CURRENT %.2f / %.2f kg"),
		Delta,
		Inventory->GetTotalWeight(),
		Inventory->MaxCarryWeight));
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
		return FText::FromString(TEXT("OWNED   |   UNIQUE GEAR ALREADY IN BACKPACK"));
	}

	if (Crafting->CanCraftRecipe(Index))
	{
		return FText::FromString(Crafting->IsWorkbenchMode()
			? TEXT("READY   |   CLICK CRAFT ITEM OR PRESS ENTER")
			: TEXT("READY   |   CLICK CRAFT ITEM OR PRESS ENTER"));
	}

	return FText::FromString(TEXT("MISSING MATERIALS   |   SCAVENGE REQUIRED"));
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
		: FSlateColor(FLinearColor(0.91f, 0.42f, 0.22f, 1.0f));
}
