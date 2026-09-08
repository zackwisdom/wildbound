#include "SWildBoundBackpackWidget.h"

#include "../Inventory/WildBoundInventoryComponent.h"
#include "../Player/WildBoundBackpackComponent.h"
#include "Styling/CoreStyle.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SSeparator.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Notifications/SProgressBar.h"
#include "Widgets/Text/STextBlock.h"

void SWildBoundBackpackWidget::Construct(const FArguments& InArgs)
{
	BackpackComponent = InArgs._BackpackComponent;

	ChildSlot
	[
		SNew(SBox)
		.WidthOverride(840.0f)
		[
			SNew(SBorder)
			.Padding(FMargin(22.0f, 18.0f))
			.BorderBackgroundColor(FLinearColor(0.012f, 0.016f, 0.015f, 0.97f))
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot().AutoHeight()
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot().FillWidth(1.0f)
					[
						SNew(STextBlock)
						.Text(FText::FromString(TEXT("BACKPACK / LOADOUT")))
						.Font(FCoreStyle::GetDefaultFontStyle("Bold", 18))
					]
					+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
					[
						SNew(STextBlock)
						.Text(FText::FromString(TEXT("TAB / I / ESC   CLOSE")))
						.Font(FCoreStyle::GetDefaultFontStyle("Regular", 9))
						.ColorAndOpacity(FLinearColor(0.58f, 0.61f, 0.57f, 1.0f))
					]
				]
				+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 7.0f, 0.0f, 0.0f)
				[
					SNew(STextBlock)
					.Text(FText::FromString(TEXT("RARITY   COMMON  •  UNCOMMON  •  RARE  •  EPIC")))
					.Font(FCoreStyle::GetDefaultFontStyle("Regular", 9))
					.ColorAndOpacity(FLinearColor(0.62f, 0.67f, 0.62f, 1.0f))
				]
				+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 8.0f, 0.0f, 10.0f)
				[
					SNew(SSeparator)
				]
				+ SVerticalBox::Slot().AutoHeight()
				[
					SNew(STextBlock)
					.Text(this, &SWildBoundBackpackWidget::GetWeightText)
					.Font(FCoreStyle::GetDefaultFontStyle("Bold", 11))
				]
				+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 4.0f)
				[
					SNew(SProgressBar)
					.Percent(this, &SWildBoundBackpackWidget::GetWeightPercent)
					.FillColorAndOpacity(FLinearColor(0.70f, 0.55f, 0.18f, 1.0f))
				]
				+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 0.0f, 0.0f, 12.0f)
				[
					SNew(STextBlock)
					.Text(this, &SWildBoundBackpackWidget::GetStatusText)
					.ColorAndOpacity(this, &SWildBoundBackpackWidget::GetStatusColor)
					.Font(FCoreStyle::GetDefaultFontStyle("Bold", 10))
				]
				+ SVerticalBox::Slot().AutoHeight()
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot().FillWidth(0.66f).Padding(0.0f, 0.0f, 14.0f, 0.0f)
					[
						SNew(SBorder)
						.Padding(FMargin(14.0f, 12.0f))
						.BorderBackgroundColor(FLinearColor(0.035f, 0.045f, 0.041f, 0.90f))
						[
							SNew(SVerticalBox)
							+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 0.0f, 0.0f, 8.0f)
							[
								SNew(STextBlock)
								.Text(FText::FromString(TEXT("INVENTORY")))
								.Font(FCoreStyle::GetDefaultFontStyle("Bold", 11))
							]
							+ SVerticalBox::Slot().AutoHeight()
							[
								SNew(STextBlock)
								.Text(this, &SWildBoundBackpackWidget::GetInventoryListText)
								.Font(FCoreStyle::GetDefaultFontStyle("Mono", 10))
								.ColorAndOpacity(FLinearColor(0.87f, 0.89f, 0.84f, 1.0f))
							]
						]
					]
					+ SHorizontalBox::Slot().FillWidth(0.34f)
					[
						SNew(SVerticalBox)
						+ SVerticalBox::Slot().AutoHeight()
						[
							SNew(SBorder)
							.Padding(FMargin(12.0f, 10.0f))
							.BorderBackgroundColor(FLinearColor(0.045f, 0.055f, 0.050f, 0.95f))
							[
								SNew(SVerticalBox)
								+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 0.0f, 0.0f, 7.0f)
								[
									SNew(STextBlock)
									.Text(FText::FromString(TEXT("HOTBAR")))
									.Font(FCoreStyle::GetDefaultFontStyle("Bold", 11))
								]
								+ SVerticalBox::Slot().AutoHeight()
								[
									SNew(STextBlock)
									.Text(this, &SWildBoundBackpackWidget::GetHotbarText)
									.Font(FCoreStyle::GetDefaultFontStyle("Mono", 9))
									.ColorAndOpacity(FLinearColor(0.91f, 0.84f, 0.62f, 1.0f))
								]
							]
						]
						+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 12.0f, 0.0f, 0.0f)
						[
							SNew(SBorder)
							.Padding(FMargin(12.0f, 10.0f))
							.BorderBackgroundColor(FLinearColor(0.028f, 0.035f, 0.032f, 0.95f))
							[
								SNew(SVerticalBox)
								+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 0.0f, 0.0f, 7.0f)
								[
									SNew(STextBlock)
									.Text(this, &SWildBoundBackpackWidget::GetSelectedItemText)
									.Font(FCoreStyle::GetDefaultFontStyle("Bold", 10))
								]
								+ SVerticalBox::Slot().AutoHeight()
								[
									SNew(STextBlock)
									.Text(FText::FromString(TEXT(
										"UP / DOWN   SELECT\n"
										"1 / 2 / 3   MOVE TO HOTBAR\n"
										"R           REMOVE FROM HOTBAR\n"
										"D           DROP ONE\n"
										"SHIFT + D   DROP STACK")))
									.Font(FCoreStyle::GetDefaultFontStyle("Regular", 9))
									.ColorAndOpacity(FLinearColor(0.66f, 0.69f, 0.64f, 1.0f))
								]
							]
						]
					]
				]
			]
		]
	];
}

void SWildBoundBackpackWidget::SetBackpackComponent(UWildBoundBackpackComponent* InBackpackComponent)
{
	BackpackComponent = InBackpackComponent;
}

const UWildBoundInventoryComponent* SWildBoundBackpackWidget::GetInventory() const
{
	const UWildBoundBackpackComponent* Backpack = BackpackComponent.Get();
	return Backpack ? Backpack->GetInventoryComponent() : nullptr;
}

TOptional<float> SWildBoundBackpackWidget::GetWeightPercent() const
{
	const UWildBoundInventoryComponent* Inventory = GetInventory();
	if (!Inventory || Inventory->MaxCarryWeight <= 0.0f)
	{
		return TOptional<float>(0.0f);
	}
	return TOptional<float>(FMath::Clamp(Inventory->GetTotalWeight() / Inventory->MaxCarryWeight, 0.0f, 1.0f));
}

FText SWildBoundBackpackWidget::GetWeightText() const
{
	const UWildBoundInventoryComponent* Inventory = GetInventory();
	const float Current = Inventory ? Inventory->GetTotalWeight() : 0.0f;
	const float Max = Inventory ? Inventory->MaxCarryWeight : 0.0f;
	const int32 SlotsUsed = Inventory ? Inventory->Stacks.Num() : 0;
	const int32 SlotsMax = Inventory ? Inventory->MaxSlots : 0;
	return FText::FromString(FString::Printf(
		TEXT("CARRY WEIGHT   %.2f / %.2f kg      SLOTS   %d / %d"),
		Current,
		Max,
		SlotsUsed,
		SlotsMax));
}

FText SWildBoundBackpackWidget::GetStatusText() const
{
	const UWildBoundInventoryComponent* Inventory = GetInventory();
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
	const UWildBoundInventoryComponent* Inventory = GetInventory();
	return Inventory && Inventory->IsOverEncumbered()
		? FSlateColor(FLinearColor(0.95f, 0.30f, 0.12f, 1.0f))
		: FSlateColor(FLinearColor(0.50f, 0.70f, 0.46f, 1.0f));
}

FText SWildBoundBackpackWidget::GetHotbarText() const
{
	const UWildBoundInventoryComponent* Inventory = GetInventory();
	if (!Inventory)
	{
		return FText::FromString(TEXT("1  —\n2  —\n3  —"));
	}

	FString Result;
	for (int32 Slot = 0; Slot < 3; ++Slot)
	{
		const FName ItemId = Inventory->GetHotbarItemId(Slot);
		if (ItemId.IsNone())
		{
			Result += FString::Printf(TEXT("%d  — EMPTY —\n"), Slot + 1);
			continue;
		}

		Result += FString::Printf(
			TEXT("%d  [%s] %s x%d\n"),
			Slot + 1,
			*Inventory->GetItemRarityName(ItemId),
			*Inventory->GetItemDisplayName(ItemId),
			Inventory->GetItemCount(ItemId));
	}
	return FText::FromString(Result);
}

FText SWildBoundBackpackWidget::GetInventoryListText() const
{
	const UWildBoundInventoryComponent* Inventory = GetInventory();
	const UWildBoundBackpackComponent* Backpack = BackpackComponent.Get();
	if (!Inventory || Inventory->Stacks.IsEmpty())
	{
		return FText::FromString(TEXT("— backpack empty —"));
	}

	FString Result;
	for (int32 Index = 0; Index < Inventory->Stacks.Num(); ++Index)
	{
		const FWildBoundInventoryStack& Stack = Inventory->Stacks[Index];
		const bool bSelected = Backpack && Backpack->GetSelectedStackIndex() == Index;
		int32 HotbarSlot = INDEX_NONE;
		const bool bInHotbar = Inventory->IsItemInHotbar(Stack.ItemId, HotbarSlot);
		const FString Hotbar = bInHotbar ? FString::Printf(TEXT("H%d"), HotbarSlot + 1) : TEXT("  ");
		const float StackWeight = Inventory->GetItemUnitWeight(Stack.ItemId) * static_cast<float>(Stack.Quantity);

		Result += FString::Printf(
			TEXT("%s [%s] [%-8s] %-22s x%-3d %5.2f kg\n"),
			bSelected ? TEXT(">") : TEXT(" "),
			*Hotbar,
			*Inventory->GetItemRarityName(Stack.ItemId),
			*Inventory->GetItemDisplayName(Stack.ItemId),
			Stack.Quantity,
			StackWeight);
	}
	return FText::FromString(Result);
}

FText SWildBoundBackpackWidget::GetSelectedItemText() const
{
	const UWildBoundInventoryComponent* Inventory = GetInventory();
	const UWildBoundBackpackComponent* Backpack = BackpackComponent.Get();
	if (!Inventory || !Backpack)
	{
		return FText::FromString(TEXT("SELECTED  —"));
	}

	const FName ItemId = Backpack->GetSelectedItemId();
	const int32 Quantity = Backpack->GetSelectedItemQuantity();
	if (ItemId.IsNone() || Quantity <= 0)
	{
		return FText::FromString(TEXT("SELECTED  —"));
	}

	return FText::FromString(FString::Printf(
		TEXT("SELECTED\n[%s] %s x%d\n%.2f kg each"),
		*Inventory->GetItemRarityName(ItemId),
		*Inventory->GetItemDisplayName(ItemId),
		Quantity,
		Inventory->GetItemUnitWeight(ItemId)));
}
