#include "SWildBoundBackpackWidget.h"

#include "../Inventory/WildBoundInventoryComponent.h"
#include "Styling/CoreStyle.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SSeparator.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Notifications/SProgressBar.h"
#include "Widgets/Text/STextBlock.h"

namespace
{
	FString BuildInventorySection(
		const UWildBoundInventoryComponent* Inventory,
		const TArray<FName>& ItemIds)
	{
		if (!Inventory)
		{
			return TEXT("No inventory data");
		}

		FString Result;
		for (const FName& ItemId : ItemIds)
		{
			const int32 Count = Inventory->GetItemCount(ItemId);
			if (Count <= 0)
			{
				continue;
			}

			const float UnitWeight = Inventory->GetItemUnitWeight(ItemId);
			const float StackWeight = UnitWeight * static_cast<float>(Count);
			Result += FString::Printf(
				TEXT("%-20s  x%-3d   %5.2f kg\n"),
				*Inventory->GetItemDisplayName(ItemId),
				Count,
				StackWeight);
		}

		return Result.IsEmpty() ? TEXT("— empty —") : Result;
	}
}

void SWildBoundBackpackWidget::Construct(const FArguments& InArgs)
{
	InventoryComponent = InArgs._InventoryComponent;

	ChildSlot
	[
		SNew(SBox)
		.WidthOverride(680.0f)
		[
			SNew(SBorder)
			.Padding(FMargin(22.0f, 18.0f))
			.BorderBackgroundColor(FLinearColor(0.012f, 0.016f, 0.015f, 0.95f))
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot()
				.AutoHeight()
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot()
					.FillWidth(1.0f)
					[
						SNew(STextBlock)
						.Text(FText::FromString(TEXT("BACKPACK")))
						.Font(FCoreStyle::GetDefaultFontStyle("Bold", 18))
					]
					+ SHorizontalBox::Slot()
					.AutoWidth()
					.VAlign(VAlign_Center)
					[
						SNew(STextBlock)
						.Text(FText::FromString(TEXT("I / TAB  CLOSE")))
						.Font(FCoreStyle::GetDefaultFontStyle("Regular", 9))
						.ColorAndOpacity(FLinearColor(0.58f, 0.61f, 0.57f, 1.0f))
					]
				]
				+ SVerticalBox::Slot()
				.AutoHeight()
				.Padding(0.0f, 8.0f, 0.0f, 10.0f)
				[
					SNew(SSeparator)
				]
				+ SVerticalBox::Slot()
				.AutoHeight()
				[
					SNew(STextBlock)
					.Text(this, &SWildBoundBackpackWidget::GetWeightText)
					.Font(FCoreStyle::GetDefaultFontStyle("Bold", 11))
				]
				+ SVerticalBox::Slot()
				.AutoHeight()
				.Padding(0.0f, 4.0f, 0.0f, 4.0f)
				[
					SNew(SProgressBar)
					.Percent(this, &SWildBoundBackpackWidget::GetWeightPercent)
					.FillColorAndOpacity(FLinearColor(0.70f, 0.55f, 0.18f, 1.0f))
				]
				+ SVerticalBox::Slot()
				.AutoHeight()
				.Padding(0.0f, 0.0f, 0.0f, 14.0f)
				[
					SNew(STextBlock)
					.Text(this, &SWildBoundBackpackWidget::GetStatusText)
					.ColorAndOpacity(this, &SWildBoundBackpackWidget::GetStatusColor)
					.Font(FCoreStyle::GetDefaultFontStyle("Bold", 10))
				]
				+ SVerticalBox::Slot()
				.AutoHeight()
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot()
					.FillWidth(0.45f)
					.Padding(0.0f, 0.0f, 16.0f, 0.0f)
					[
						SNew(SBorder)
						.Padding(FMargin(14.0f, 12.0f))
						.BorderBackgroundColor(FLinearColor(0.035f, 0.045f, 0.041f, 0.88f))
						[
							SNew(SVerticalBox)
							+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 0.0f, 0.0f, 8.0f)
							[
								SNew(STextBlock)
								.Text(FText::FromString(TEXT("SUPPLIES / GEAR")))
								.Font(FCoreStyle::GetDefaultFontStyle("Bold", 11))
							]
							+ SVerticalBox::Slot().AutoHeight()
							[
								SNew(STextBlock)
								.Text(this, &SWildBoundBackpackWidget::GetSuppliesText)
								.Font(FCoreStyle::GetDefaultFontStyle("Regular", 10))
								.ColorAndOpacity(FLinearColor(0.87f, 0.89f, 0.84f, 1.0f))
							]
						]
					]
					+ SHorizontalBox::Slot()
					.FillWidth(0.55f)
					[
						SNew(SBorder)
						.Padding(FMargin(14.0f, 12.0f))
						.BorderBackgroundColor(FLinearColor(0.035f, 0.045f, 0.041f, 0.88f))
						[
							SNew(SVerticalBox)
							+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 0.0f, 0.0f, 8.0f)
							[
								SNew(STextBlock)
								.Text(FText::FromString(TEXT("CRAFTING MATERIALS")))
								.Font(FCoreStyle::GetDefaultFontStyle("Bold", 11))
							]
							+ SVerticalBox::Slot().AutoHeight()
							[
								SNew(STextBlock)
								.Text(this, &SWildBoundBackpackWidget::GetMaterialsText)
								.Font(FCoreStyle::GetDefaultFontStyle("Regular", 10))
								.ColorAndOpacity(FLinearColor(0.87f, 0.89f, 0.84f, 1.0f))
							]
						]
					]
				]
			]
		]
	];
}

void SWildBoundBackpackWidget::SetInventoryComponent(UWildBoundInventoryComponent* InInventoryComponent)
{
	InventoryComponent = InInventoryComponent;
}

TOptional<float> SWildBoundBackpackWidget::GetWeightPercent() const
{
	const UWildBoundInventoryComponent* Inventory = InventoryComponent.Get();
	if (!Inventory || Inventory->MaxCarryWeight <= 0.0f)
	{
		return TOptional<float>(0.0f);
	}

	return TOptional<float>(FMath::Clamp(Inventory->GetTotalWeight() / Inventory->MaxCarryWeight, 0.0f, 1.0f));
}

FText SWildBoundBackpackWidget::GetWeightText() const
{
	const UWildBoundInventoryComponent* Inventory = InventoryComponent.Get();
	const float Current = Inventory ? Inventory->GetTotalWeight() : 0.0f;
	const float Max = Inventory ? Inventory->MaxCarryWeight : 0.0f;
	return FText::FromString(FString::Printf(TEXT("CARRY WEIGHT   %.2f / %.2f kg"), Current, Max));
}

FText SWildBoundBackpackWidget::GetStatusText() const
{
	const UWildBoundInventoryComponent* Inventory = InventoryComponent.Get();
	if (!Inventory)
	{
		return FText::FromString(TEXT("LOAD STATUS UNAVAILABLE"));
	}

	if (Inventory->IsOverEncumbered())
	{
		const int32 OverPercent = FMath::RoundToInt(FMath::Max(0.0f, Inventory->GetCarryWeightRatio() - 1.0f) * 100.0f);
		return FText::FromString(FString::Printf(
			TEXT("OVER ENCUMBERED  +%d%% — MOVEMENT SLOWED / STAMINA COST INCREASED"),
			OverPercent));
	}

	return FText::FromString(TEXT("LOAD STATUS NORMAL"));
}

FSlateColor SWildBoundBackpackWidget::GetStatusColor() const
{
	const UWildBoundInventoryComponent* Inventory = InventoryComponent.Get();
	return Inventory && Inventory->IsOverEncumbered()
		? FSlateColor(FLinearColor(0.95f, 0.30f, 0.12f, 1.0f))
		: FSlateColor(FLinearColor(0.50f, 0.70f, 0.46f, 1.0f));
}

FText SWildBoundBackpackWidget::GetSuppliesText() const
{
	static const TArray<FName> Supplies =
	{
		TEXT("Water"),
		TEXT("Food"),
		TEXT("MedicalSupplies"),
		TEXT("Flashlight"),
		TEXT("Crowbar")
	};

	return FText::FromString(BuildInventorySection(InventoryComponent.Get(), Supplies));
}

FText SWildBoundBackpackWidget::GetMaterialsText() const
{
	static const TArray<FName> Materials =
	{
		TEXT("ScrapMetal"),
		TEXT("Cloth"),
		TEXT("Wood"),
		TEXT("Plastic"),
		TEXT("Electronics"),
		TEXT("Chemicals"),
		TEXT("Adhesive"),
		TEXT("Wire"),
		TEXT("Battery"),
		TEXT("MechanicalParts")
	};

	return FText::FromString(BuildInventorySection(InventoryComponent.Get(), Materials));
}
