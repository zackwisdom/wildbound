#include "SWildBoundBackpackWidget.h"

#include "../Inventory/WildBoundInventoryComponent.h"
#include "../Player/WildBoundBackpackComponent.h"
#include "DragAndDrop/DecoratedDragDropOp.h"
#include "InputCoreTypes.h"
#include "Styling/CoreStyle.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SSeparator.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Notifications/SProgressBar.h"
#include "Widgets/SOverlay.h"
#include "Widgets/Text/STextBlock.h"

namespace
{
	FSlateColor GetItemRaritySlateColor(const UWildBoundInventoryComponent* Inventory, FName ItemId)
	{
		const int32 Tier = Inventory ? Inventory->GetItemRarityTier(ItemId) : 0;
		switch (Tier)
		{
		case 3: return FSlateColor(FLinearColor(0.83f, 0.50f, 0.95f, 1.0f));
		case 2: return FSlateColor(FLinearColor(0.45f, 0.68f, 0.94f, 1.0f));
		case 1: return FSlateColor(FLinearColor(0.52f, 0.80f, 0.54f, 1.0f));
		default: return FSlateColor(FLinearColor(0.84f, 0.86f, 0.81f, 1.0f));
		}
	}

	class FWildBoundInventoryDragDropOp : public FDecoratedDragDropOp
	{
	public:
		DRAG_DROP_OPERATOR_TYPE(FWildBoundInventoryDragDropOp, FDecoratedDragDropOp)

		int32 SourceStackIndex = INDEX_NONE;
		FName ItemId = NAME_None;
		int32 Quantity = 0;

		static TSharedRef<FWildBoundInventoryDragDropOp> New(
			int32 InSourceStackIndex,
			FName InItemId,
			int32 InQuantity,
			const FString& DisplayName,
			const FString& Rarity)
		{
			TSharedRef<FWildBoundInventoryDragDropOp> Operation = MakeShared<FWildBoundInventoryDragDropOp>();
			Operation->SourceStackIndex = InSourceStackIndex;
			Operation->ItemId = InItemId;
			Operation->Quantity = InQuantity;
			Operation->DefaultHoverText = FText::FromString(FString::Printf(
				TEXT("[%s] %s x%d"),
				*Rarity,
				*DisplayName,
				InQuantity));
			Operation->Construct();
			return Operation;
		}
	};

	class SWildBoundInventoryRow : public SBorder
	{
	public:
		SLATE_BEGIN_ARGS(SWildBoundInventoryRow) {}
			SLATE_ARGUMENT(TWeakObjectPtr<UWildBoundBackpackComponent>, BackpackComponent)
			SLATE_ARGUMENT(int32, StackIndex)
		SLATE_END_ARGS()

		void Construct(const FArguments& InArgs)
		{
			BackpackComponent = InArgs._BackpackComponent;
			StackIndex = InArgs._StackIndex;

			SBorder::Construct(
				SBorder::FArguments()
				.Padding(FMargin(8.0f, 6.0f))
				.BorderBackgroundColor(FLinearColor(0.055f, 0.065f, 0.060f, 0.94f))
				[
					SNew(STextBlock)
					.Text(this, &SWildBoundInventoryRow::GetRowText)
					.ColorAndOpacity(this, &SWildBoundInventoryRow::GetRowColor)
					.Font(FCoreStyle::GetDefaultFontStyle("Mono", 10))
				]);
		}

		virtual FReply OnMouseButtonDown(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override
		{
			if (MouseEvent.GetEffectingButton() == EKeys::LeftMouseButton)
			{
				if (UWildBoundBackpackComponent* Backpack = BackpackComponent.Get())
				{
					Backpack->SelectStackIndex(StackIndex);
				}
				return FReply::Handled().DetectDrag(SharedThis(this), EKeys::LeftMouseButton);
			}
			return SBorder::OnMouseButtonDown(MyGeometry, MouseEvent);
		}

		virtual FReply OnDragDetected(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override
		{
			UWildBoundBackpackComponent* Backpack = BackpackComponent.Get();
			UWildBoundInventoryComponent* Inventory = Backpack ? Backpack->GetInventoryComponent() : nullptr;
			if (!Inventory || !Inventory->Stacks.IsValidIndex(StackIndex))
			{
				return FReply::Unhandled();
			}

			const FWildBoundInventoryStack& Stack = Inventory->Stacks[StackIndex];
			return FReply::Handled().BeginDragDrop(FWildBoundInventoryDragDropOp::New(
				StackIndex,
				Stack.ItemId,
				Stack.Quantity,
				Inventory->GetItemDisplayName(Stack.ItemId),
				Inventory->GetItemRarityName(Stack.ItemId)));
		}

		virtual FReply OnDrop(const FGeometry& MyGeometry, const FDragDropEvent& DragDropEvent) override
		{
			const TSharedPtr<FWildBoundInventoryDragDropOp> Operation = DragDropEvent.GetOperationAs<FWildBoundInventoryDragDropOp>();
			UWildBoundBackpackComponent* Backpack = BackpackComponent.Get();
			if (Operation.IsValid() && Backpack)
			{
				Backpack->ReorderStackFromMouse(Operation->SourceStackIndex, StackIndex);
				return FReply::Handled();
			}
			return FReply::Unhandled();
		}

	private:
		TWeakObjectPtr<UWildBoundBackpackComponent> BackpackComponent;
		int32 StackIndex = INDEX_NONE;

		FText GetRowText() const
		{
			const UWildBoundBackpackComponent* Backpack = BackpackComponent.Get();
			const UWildBoundInventoryComponent* Inventory = Backpack ? Backpack->GetInventoryComponent() : nullptr;
			if (!Inventory || !Inventory->Stacks.IsValidIndex(StackIndex))
			{
				return FText::GetEmpty();
			}

			const FWildBoundInventoryStack& Stack = Inventory->Stacks[StackIndex];
			int32 HotbarSlot = INDEX_NONE;
			const bool bInHotbar = Inventory->IsItemInHotbar(Stack.ItemId, HotbarSlot);
			const bool bSelected = Backpack->GetSelectedStackIndex() == StackIndex;
			const FString HotbarText = bInHotbar ? FString::Printf(TEXT("H%d"), HotbarSlot + 1) : TEXT("--");
			const float StackWeight = Inventory->GetItemUnitWeight(Stack.ItemId) * static_cast<float>(Stack.Quantity);

			return FText::FromString(FString::Printf(
				TEXT("%s [%s] [%-8s] %-22s x%-3d %5.2f kg"),
				bSelected ? TEXT(">") : TEXT(" "),
				*HotbarText,
				*Inventory->GetItemRarityName(Stack.ItemId),
				*Inventory->GetItemDisplayName(Stack.ItemId),
				Stack.Quantity,
				StackWeight));
		}

		FSlateColor GetRowColor() const
		{
			const UWildBoundBackpackComponent* Backpack = BackpackComponent.Get();
			const UWildBoundInventoryComponent* Inventory = Backpack ? Backpack->GetInventoryComponent() : nullptr;
			if (!Inventory || !Inventory->Stacks.IsValidIndex(StackIndex))
			{
				return FSlateColor(FLinearColor::White);
			}
			return GetItemRaritySlateColor(Inventory, Inventory->Stacks[StackIndex].ItemId);
		}
	};

	class SWildBoundHotbarDropSlot : public SBorder
	{
	public:
		SLATE_BEGIN_ARGS(SWildBoundHotbarDropSlot) {}
			SLATE_ARGUMENT(TWeakObjectPtr<UWildBoundBackpackComponent>, BackpackComponent)
			SLATE_ARGUMENT(int32, SlotIndex)
		SLATE_END_ARGS()

		void Construct(const FArguments& InArgs)
		{
			BackpackComponent = InArgs._BackpackComponent;
			SlotIndex = InArgs._SlotIndex;

			SBorder::Construct(
				SBorder::FArguments()
				.Padding(FMargin(10.0f, 8.0f))
				.BorderBackgroundColor(FLinearColor(0.060f, 0.072f, 0.064f, 0.96f))
				[
					SNew(STextBlock)
					.Text(this, &SWildBoundHotbarDropSlot::GetSlotText)
					.Font(FCoreStyle::GetDefaultFontStyle("Bold", 9))
					.ColorAndOpacity(FLinearColor(0.92f, 0.85f, 0.64f, 1.0f))
				]);
		}

		virtual FReply OnDrop(const FGeometry& MyGeometry, const FDragDropEvent& DragDropEvent) override
		{
			const TSharedPtr<FWildBoundInventoryDragDropOp> Operation = DragDropEvent.GetOperationAs<FWildBoundInventoryDragDropOp>();
			UWildBoundBackpackComponent* Backpack = BackpackComponent.Get();
			if (Operation.IsValid() && Backpack && Backpack->AssignItemToHotbarFromMouse(Operation->ItemId, SlotIndex))
			{
				return FReply::Handled();
			}
			return FReply::Unhandled();
		}

		virtual FReply OnMouseButtonDown(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override
		{
			if (MouseEvent.GetEffectingButton() == EKeys::RightMouseButton)
			{
				if (UWildBoundBackpackComponent* Backpack = BackpackComponent.Get())
				{
					Backpack->ClearHotbarSlotFromMouse(SlotIndex);
					return FReply::Handled();
				}
			}
			return SBorder::OnMouseButtonDown(MyGeometry, MouseEvent);
		}

	private:
		TWeakObjectPtr<UWildBoundBackpackComponent> BackpackComponent;
		int32 SlotIndex = 0;

		FText GetSlotText() const
		{
			const UWildBoundBackpackComponent* Backpack = BackpackComponent.Get();
			const UWildBoundInventoryComponent* Inventory = Backpack ? Backpack->GetInventoryComponent() : nullptr;
			if (!Inventory)
			{
				return FText::FromString(FString::Printf(TEXT("%d   — EMPTY —"), SlotIndex + 1));
			}

			const FName ItemId = Inventory->GetHotbarItemId(SlotIndex);
			if (ItemId.IsNone())
			{
				return FText::FromString(FString::Printf(TEXT("%d   — EMPTY —\nDROP ITEM HERE"), SlotIndex + 1));
			}

			return FText::FromString(FString::Printf(
				TEXT("%d   [%s]\n%s  x%d"),
				SlotIndex + 1,
				*Inventory->GetItemRarityName(ItemId),
				*Inventory->GetItemDisplayName(ItemId),
				Inventory->GetItemCount(ItemId)));
		}
	};
}

void SWildBoundBackpackWidget::Construct(const FArguments& InArgs)
{
	BackpackComponent = InArgs._BackpackComponent;

	ChildSlot
	[
		SNew(SOverlay)
		+ SOverlay::Slot()
		.HAlign(HAlign_Fill)
		.VAlign(VAlign_Fill)
		[
			SNew(SBorder)
			.BorderBackgroundColor(FLinearColor(0.0f, 0.0f, 0.0f, 0.32f))
		]
		+ SOverlay::Slot()
		.HAlign(HAlign_Center)
		.VAlign(VAlign_Center)
		.Padding(FMargin(24.0f))
		[
			SAssignNew(InventoryPanel, SBox)
			.WidthOverride(920.0f)
			[
				SNew(SBorder)
				.Padding(FMargin(22.0f, 18.0f))
				.BorderBackgroundColor(FLinearColor(0.012f, 0.016f, 0.015f, 0.98f))
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
					+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 6.0f, 0.0f, 0.0f)
					[
						SNew(STextBlock)
						.Text(FText::FromString(TEXT("MOUSE: DRAG TO REORDER  •  DRAG TO HOTBAR  •  DRAG OUTSIDE PANEL TO DROP STACK")))
						.Font(FCoreStyle::GetDefaultFontStyle("Bold", 9))
						.ColorAndOpacity(FLinearColor(0.72f, 0.76f, 0.69f, 1.0f))
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
						+ SHorizontalBox::Slot().FillWidth(0.68f).Padding(0.0f, 0.0f, 14.0f, 0.0f)
						[
							SNew(SBorder)
							.Padding(FMargin(12.0f, 10.0f))
							.BorderBackgroundColor(FLinearColor(0.035f, 0.045f, 0.041f, 0.92f))
							[
								SNew(SVerticalBox)
								+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 0.0f, 0.0f, 8.0f)
								[
									SNew(STextBlock)
									.Text(FText::FromString(TEXT("INVENTORY — CLICK / DRAG ITEMS")))
									.Font(FCoreStyle::GetDefaultFontStyle("Bold", 11))
								]
								+ SVerticalBox::Slot().AutoHeight()
								[
									SAssignNew(InventoryRowsBox, SVerticalBox)
								]
							]
						]
						+ SHorizontalBox::Slot().FillWidth(0.32f)
						[
							SNew(SVerticalBox)
							+ SVerticalBox::Slot().AutoHeight()
							[
								SNew(SBorder)
								.Padding(FMargin(12.0f, 10.0f))
								.BorderBackgroundColor(FLinearColor(0.045f, 0.055f, 0.050f, 0.96f))
								[
									SNew(SVerticalBox)
									+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 0.0f, 0.0f, 7.0f)
									[
										SNew(STextBlock)
										.Text(FText::FromString(TEXT("HOTBAR — DROP ITEMS")))
										.Font(FCoreStyle::GetDefaultFontStyle("Bold", 11))
									]
									+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 2.0f)
									[
										SNew(SWildBoundHotbarDropSlot).BackpackComponent(BackpackComponent).SlotIndex(0)
									]
									+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 2.0f)
									[
										SNew(SWildBoundHotbarDropSlot).BackpackComponent(BackpackComponent).SlotIndex(1)
									]
									+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 2.0f)
									[
										SNew(SWildBoundHotbarDropSlot).BackpackComponent(BackpackComponent).SlotIndex(2)
									]
									+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 6.0f, 0.0f, 0.0f)
									[
										SNew(STextBlock)
										.Text(FText::FromString(TEXT("Right-click a hotbar slot to clear it.")))
										.Font(FCoreStyle::GetDefaultFontStyle("Regular", 8))
										.ColorAndOpacity(FLinearColor(0.60f, 0.63f, 0.58f, 1.0f))
									]
								]
							]
							+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 12.0f, 0.0f, 0.0f)
							[
								SNew(SBorder)
								.Padding(FMargin(12.0f, 10.0f))
								.BorderBackgroundColor(FLinearColor(0.028f, 0.035f, 0.032f, 0.96f))
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
											"KEYBOARD\n"
											"UP / DOWN   SELECT\n"
											"1 / 2 / 3   HOTBAR\n"
											"D           DROP ONE\n"
											"SHIFT + D   DROP STACK")))
										.Font(FCoreStyle::GetDefaultFontStyle("Regular", 8))
										.ColorAndOpacity(FLinearColor(0.62f, 0.65f, 0.60f, 1.0f))
									]
								]
							]
						]
					]
				]
			]
		]
	];

	RebuildInventoryRows();
	CachedInventorySignature = CalculateInventorySignature();
}

void SWildBoundBackpackWidget::SetBackpackComponent(UWildBoundBackpackComponent* InBackpackComponent)
{
	BackpackComponent = InBackpackComponent;
}

void SWildBoundBackpackWidget::Tick(
	const FGeometry& AllottedGeometry,
	const double InCurrentTime,
	const float InDeltaTime)
{
	SCompoundWidget::Tick(AllottedGeometry, InCurrentTime, InDeltaTime);

	const uint32 NewSignature = CalculateInventorySignature();
	if (NewSignature != CachedInventorySignature)
	{
		CachedInventorySignature = NewSignature;
		RebuildInventoryRows();
	}
}

FReply SWildBoundBackpackWidget::OnDrop(const FGeometry& MyGeometry, const FDragDropEvent& DragDropEvent)
{
	const TSharedPtr<FWildBoundInventoryDragDropOp> Operation = DragDropEvent.GetOperationAs<FWildBoundInventoryDragDropOp>();
	UWildBoundBackpackComponent* Backpack = BackpackComponent.Get();
	if (!Operation.IsValid() || !Backpack)
	{
		return FReply::Unhandled();
	}

	if (InventoryPanel.IsValid()
		&& InventoryPanel->GetCachedGeometry().IsUnderLocation(DragDropEvent.GetScreenSpacePosition()))
	{
		return FReply::Unhandled();
	}

	return Backpack->DropStackFromMouse(Operation->SourceStackIndex, true)
		? FReply::Handled()
		: FReply::Unhandled();
}

const UWildBoundInventoryComponent* SWildBoundBackpackWidget::GetInventory() const
{
	const UWildBoundBackpackComponent* Backpack = BackpackComponent.Get();
	return Backpack ? Backpack->GetInventoryComponent() : nullptr;
}

void SWildBoundBackpackWidget::RebuildInventoryRows()
{
	if (!InventoryRowsBox.IsValid())
	{
		return;
	}

	InventoryRowsBox->ClearChildren();
	const UWildBoundInventoryComponent* Inventory = GetInventory();
	if (!Inventory || Inventory->Stacks.IsEmpty())
	{
		InventoryRowsBox->AddSlot().AutoHeight()
		[
			SNew(STextBlock)
			.Text(FText::FromString(TEXT("— backpack empty —")))
			.Font(FCoreStyle::GetDefaultFontStyle("Regular", 10))
			.ColorAndOpacity(FLinearColor(0.60f, 0.63f, 0.59f, 1.0f))
		];
		return;
	}

	for (int32 Index = 0; Index < Inventory->Stacks.Num(); ++Index)
	{
		InventoryRowsBox->AddSlot().AutoHeight().Padding(0.0f, 2.0f)
		[
			SNew(SWildBoundInventoryRow)
			.BackpackComponent(BackpackComponent)
			.StackIndex(Index)
		];
	}
}

uint32 SWildBoundBackpackWidget::CalculateInventorySignature() const
{
	const UWildBoundInventoryComponent* Inventory = GetInventory();
	if (!Inventory)
	{
		return 0;
	}

	uint32 Signature = GetTypeHash(Inventory->Stacks.Num());
	for (const FWildBoundInventoryStack& Stack : Inventory->Stacks)
	{
		Signature = HashCombine(Signature, GetTypeHash(Stack.ItemId));
		Signature = HashCombine(Signature, GetTypeHash(Stack.Quantity));
	}
	for (const FName& HotbarItem : Inventory->HotbarSlots)
	{
		Signature = HashCombine(Signature, GetTypeHash(HotbarItem));
	}
	return Signature;
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
