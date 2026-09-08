#include "SWildBoundBackpackWidget.h"

#include "../Inventory/WildBoundInventoryComponent.h"
#include "../Player/WildBoundBackpackComponent.h"
#include "DragAndDrop/DecoratedDragDropOp.h"
#include "InputCoreTypes.h"
#include "Styling/CoreStyle.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Layout/SSeparator.h"
#include "Widgets/Notifications/SProgressBar.h"
#include "Widgets/SBoxPanel.h"
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
				.Padding(FMargin(10.0f, 8.0f))
				.BorderBackgroundColor(this, &SWildBoundInventoryRow::GetRowBackground)
				.ToolTipText(this, &SWildBoundInventoryRow::GetTooltipText)
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot().AutoWidth().Padding(0.0f, 0.0f, 10.0f, 0.0f)
					[
						SNew(SBox)
						.WidthOverride(6.0f)
						.HeightOverride(42.0f)
						[
							SNew(SBorder)
							.Padding(0.0f)
							.BorderBackgroundColor(this, &SWildBoundInventoryRow::GetRarityColor)
						]
					]
					+ SHorizontalBox::Slot().FillWidth(1.0f).VAlign(VAlign_Center)
					[
						SNew(SVerticalBox)
						+ SVerticalBox::Slot().AutoHeight()
						[
							SNew(STextBlock)
							.Text(this, &SWildBoundInventoryRow::GetItemNameText)
							.Font(FCoreStyle::GetDefaultFontStyle("Bold", 11))
							.ColorAndOpacity(this, &SWildBoundInventoryRow::GetRarityColor)
						]
						+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 2.0f, 0.0f, 0.0f)
						[
							SNew(STextBlock)
							.Text(this, &SWildBoundInventoryRow::GetMetadataText)
							.Font(FCoreStyle::GetDefaultFontStyle("Regular", 8))
							.ColorAndOpacity(FLinearColor(0.61f, 0.66f, 0.61f, 1.0f))
						]
					]
					+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).HAlign(HAlign_Right)
					[
						SNew(SVerticalBox)
						+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Right)
						[
							SNew(STextBlock)
							.Text(this, &SWildBoundInventoryRow::GetQuantityText)
							.Font(FCoreStyle::GetDefaultFontStyle("Bold", 11))
						]
						+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 2.0f, 0.0f, 0.0f).HAlign(HAlign_Right)
						[
							SNew(STextBlock)
							.Text(this, &SWildBoundInventoryRow::GetWeightText)
							.Font(FCoreStyle::GetDefaultFontStyle("Regular", 8))
							.ColorAndOpacity(FLinearColor(0.70f, 0.71f, 0.67f, 1.0f))
						]
					]
				]);
		}

		virtual FReply OnMouseButtonDown(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override
		{
			UWildBoundBackpackComponent* Backpack = BackpackComponent.Get();
			if (!Backpack)
			{
				return SBorder::OnMouseButtonDown(MyGeometry, MouseEvent);
			}

			if (MouseEvent.GetEffectingButton() == EKeys::LeftMouseButton)
			{
				Backpack->SelectStackIndex(StackIndex);
				return FReply::Handled().DetectDrag(SharedThis(this), EKeys::LeftMouseButton);
			}

			if (MouseEvent.GetEffectingButton() == EKeys::RightMouseButton)
			{
				Backpack->SelectStackIndex(StackIndex);
				Backpack->SplitStackFromMouse(StackIndex);
				return FReply::Handled();
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

		const FWildBoundInventoryStack* GetStack() const
		{
			const UWildBoundBackpackComponent* Backpack = BackpackComponent.Get();
			const UWildBoundInventoryComponent* Inventory = Backpack ? Backpack->GetInventoryComponent() : nullptr;
			return Inventory && Inventory->Stacks.IsValidIndex(StackIndex)
				? &Inventory->Stacks[StackIndex]
				: nullptr;
		}

		FSlateColor GetRowBackground() const
		{
			const UWildBoundBackpackComponent* Backpack = BackpackComponent.Get();
			return Backpack && Backpack->GetSelectedStackIndex() == StackIndex
				? FSlateColor(FLinearColor(0.095f, 0.115f, 0.102f, 0.98f))
				: FSlateColor(FLinearColor(0.045f, 0.054f, 0.050f, 0.96f));
		}

		FSlateColor GetRarityColor() const
		{
			const UWildBoundBackpackComponent* Backpack = BackpackComponent.Get();
			const UWildBoundInventoryComponent* Inventory = Backpack ? Backpack->GetInventoryComponent() : nullptr;
			const FWildBoundInventoryStack* Stack = GetStack();
			return Stack ? GetItemRaritySlateColor(Inventory, Stack->ItemId) : FSlateColor(FLinearColor::White);
		}

		FText GetItemNameText() const
		{
			const UWildBoundBackpackComponent* Backpack = BackpackComponent.Get();
			const UWildBoundInventoryComponent* Inventory = Backpack ? Backpack->GetInventoryComponent() : nullptr;
			const FWildBoundInventoryStack* Stack = GetStack();
			return Inventory && Stack
				? FText::FromString(Inventory->GetItemDisplayName(Stack->ItemId))
				: FText::GetEmpty();
		}

		FText GetMetadataText() const
		{
			const UWildBoundBackpackComponent* Backpack = BackpackComponent.Get();
			const UWildBoundInventoryComponent* Inventory = Backpack ? Backpack->GetInventoryComponent() : nullptr;
			const FWildBoundInventoryStack* Stack = GetStack();
			if (!Inventory || !Stack)
			{
				return FText::GetEmpty();
			}

			int32 HotbarSlot = INDEX_NONE;
			const bool bInHotbar = Inventory->IsItemInHotbar(Stack->ItemId, HotbarSlot);
			return FText::FromString(FString::Printf(
				TEXT("%s  •  %s%s"),
				*Inventory->GetItemRarityName(Stack->ItemId),
				*Inventory->GetItemCategoryName(Stack->ItemId),
				bInHotbar ? *FString::Printf(TEXT("  •  HOTBAR %d"), HotbarSlot + 1) : TEXT("")));
		}

		FText GetQuantityText() const
		{
			const FWildBoundInventoryStack* Stack = GetStack();
			return Stack ? FText::FromString(FString::Printf(TEXT("x%d"), Stack->Quantity)) : FText::GetEmpty();
		}

		FText GetWeightText() const
		{
			const UWildBoundBackpackComponent* Backpack = BackpackComponent.Get();
			const UWildBoundInventoryComponent* Inventory = Backpack ? Backpack->GetInventoryComponent() : nullptr;
			const FWildBoundInventoryStack* Stack = GetStack();
			if (!Inventory || !Stack)
			{
				return FText::GetEmpty();
			}
			return FText::FromString(FString::Printf(
				TEXT("%.2f kg"),
				Inventory->GetItemUnitWeight(Stack->ItemId) * static_cast<float>(Stack->Quantity)));
		}

		FText GetTooltipText() const
		{
			const UWildBoundBackpackComponent* Backpack = BackpackComponent.Get();
			const UWildBoundInventoryComponent* Inventory = Backpack ? Backpack->GetInventoryComponent() : nullptr;
			const FWildBoundInventoryStack* Stack = GetStack();
			if (!Inventory || !Stack)
			{
				return FText::GetEmpty();
			}

			const float UnitWeight = Inventory->GetItemUnitWeight(Stack->ItemId);
			return FText::FromString(FString::Printf(
				TEXT("[%s] %s\n%s\n\n%s\n\nStack: %d   Unit: %.2f kg   Total: %.2f kg\nLeft-drag: move / hotbar / drop   Right-click: split stack in half"),
				*Inventory->GetItemRarityName(Stack->ItemId),
				*Inventory->GetItemDisplayName(Stack->ItemId),
				*Inventory->GetItemCategoryName(Stack->ItemId),
				*Inventory->GetItemDescription(Stack->ItemId),
				Stack->Quantity,
				UnitWeight,
				UnitWeight * static_cast<float>(Stack->Quantity)));
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
				.Padding(FMargin(12.0f, 10.0f))
				.BorderBackgroundColor(FLinearColor(0.060f, 0.072f, 0.064f, 0.96f))
				.ToolTipText(FText::FromString(TEXT("Drag an inventory item here to assign it. Right-click to clear this slot.")))
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
			.BorderBackgroundColor(FLinearColor(0.0f, 0.0f, 0.0f, 0.42f))
		]
		+ SOverlay::Slot()
		.HAlign(HAlign_Center)
		.VAlign(VAlign_Center)
		.Padding(FMargin(24.0f))
		[
			SAssignNew(InventoryPanel, SBox)
			.WidthOverride(1020.0f)
			[
				SNew(SBorder)
				.Padding(FMargin(24.0f, 20.0f))
				.BorderBackgroundColor(FLinearColor(0.012f, 0.016f, 0.015f, 0.985f))
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
								.Text(FText::FromString(TEXT("BACKPACK")))
								.Font(FCoreStyle::GetDefaultFontStyle("Bold", 20))
							]
							+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 3.0f, 0.0f, 0.0f)
							[
								SNew(STextBlock)
								.Text(FText::FromString(TEXT("LOADOUT / SCAVENGED SUPPLIES")))
								.Font(FCoreStyle::GetDefaultFontStyle("Regular", 9))
								.ColorAndOpacity(FLinearColor(0.57f, 0.62f, 0.57f, 1.0f))
							]
						]
						+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
						[
							SNew(STextBlock)
							.Text(FText::FromString(TEXT("TAB / I / ESC   CLOSE")))
							.Font(FCoreStyle::GetDefaultFontStyle("Regular", 9))
							.ColorAndOpacity(FLinearColor(0.58f, 0.61f, 0.57f, 1.0f))
						]
					]
					+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 10.0f, 0.0f, 10.0f)
					[
						SNew(SSeparator)
					]
					+ SVerticalBox::Slot().AutoHeight()
					[
						SNew(SHorizontalBox)
						+ SHorizontalBox::Slot().FillWidth(1.0f)
						[
							SNew(STextBlock)
							.Text(this, &SWildBoundBackpackWidget::GetWeightText)
							.Font(FCoreStyle::GetDefaultFontStyle("Bold", 10))
						]
						+ SHorizontalBox::Slot().AutoWidth()
						[
							SNew(STextBlock)
							.Text(FText::FromString(TEXT("LEFT-DRAG MOVE   •   RIGHT-CLICK SPLIT   •   DRAG OUT TO DROP")))
							.Font(FCoreStyle::GetDefaultFontStyle("Regular", 8))
							.ColorAndOpacity(FLinearColor(0.63f, 0.67f, 0.61f, 1.0f))
						]
					]
					+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 5.0f, 0.0f, 3.0f)
					[
						SNew(SProgressBar)
						.Percent(this, &SWildBoundBackpackWidget::GetWeightPercent)
						.FillColorAndOpacity(FLinearColor(0.70f, 0.55f, 0.18f, 1.0f))
					]
					+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 0.0f, 0.0f, 14.0f)
					[
						SNew(STextBlock)
						.Text(this, &SWildBoundBackpackWidget::GetStatusText)
						.ColorAndOpacity(this, &SWildBoundBackpackWidget::GetStatusColor)
						.Font(FCoreStyle::GetDefaultFontStyle("Bold", 9))
					]
					+ SVerticalBox::Slot().AutoHeight()
					[
						SNew(SHorizontalBox)
						+ SHorizontalBox::Slot().FillWidth(0.68f).Padding(0.0f, 0.0f, 16.0f, 0.0f)
						[
							SNew(SBorder)
							.Padding(FMargin(12.0f, 10.0f))
							.BorderBackgroundColor(FLinearColor(0.028f, 0.037f, 0.033f, 0.96f))
							[
								SNew(SVerticalBox)
								+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 0.0f, 0.0f, 8.0f)
								[
									SNew(SHorizontalBox)
									+ SHorizontalBox::Slot().FillWidth(1.0f)
									[
										SNew(STextBlock)
										.Text(FText::FromString(TEXT("INVENTORY")))
										.Font(FCoreStyle::GetDefaultFontStyle("Bold", 11))
									]
									+ SHorizontalBox::Slot().AutoWidth()
									[
										SNew(STextBlock)
										.Text(FText::FromString(TEXT("HOVER FOR DETAILS")))
										.Font(FCoreStyle::GetDefaultFontStyle("Regular", 8))
										.ColorAndOpacity(FLinearColor(0.55f, 0.59f, 0.55f, 1.0f))
									]
								]
								+ SVerticalBox::Slot().AutoHeight()
								[
									SNew(SBox)
									.HeightOverride(460.0f)
									[
										SNew(SScrollBox)
										+ SScrollBox::Slot()
										[
											SAssignNew(InventoryRowsBox, SVerticalBox)
										]
									]
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
								.BorderBackgroundColor(FLinearColor(0.040f, 0.050f, 0.045f, 0.97f))
								[
									SNew(SVerticalBox)
									+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 0.0f, 0.0f, 7.0f)
									[
										SNew(STextBlock)
										.Text(FText::FromString(TEXT("QUICK ACCESS")))
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
								]
							]
							+ SVerticalBox::Slot().FillHeight(1.0f).Padding(0.0f, 12.0f, 0.0f, 0.0f)
							[
								SNew(SBorder)
								.Padding(FMargin(14.0f, 12.0f))
								.BorderBackgroundColor(FLinearColor(0.025f, 0.032f, 0.029f, 0.97f))
								[
									SNew(STextBlock)
									.Text(this, &SWildBoundBackpackWidget::GetSelectedItemText)
									.AutoWrapText(true)
									.Font(FCoreStyle::GetDefaultFontStyle("Regular", 9))
									.ColorAndOpacity(FLinearColor(0.83f, 0.85f, 0.80f, 1.0f))
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
			SNew(SBorder)
			.Padding(FMargin(16.0f, 20.0f))
			.BorderBackgroundColor(FLinearColor(0.035f, 0.042f, 0.039f, 0.90f))
			[
				SNew(STextBlock)
				.Text(FText::FromString(TEXT("Backpack empty. Search the town for supplies.")))
				.Font(FCoreStyle::GetDefaultFontStyle("Regular", 10))
				.ColorAndOpacity(FLinearColor(0.60f, 0.63f, 0.59f, 1.0f))
			]
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
		TEXT("CARRY   %.2f / %.2f kg      INVENTORY SLOTS   %d / %d"),
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
		return FText::FromString(TEXT("ITEM DETAILS\n—"));
	}

	const FName ItemId = Backpack->GetSelectedItemId();
	const int32 Quantity = Backpack->GetSelectedItemQuantity();
	if (ItemId.IsNone() || Quantity <= 0)
	{
		return FText::FromString(TEXT("ITEM DETAILS\nSelect an item to inspect it."));
	}

	int32 HotbarSlot = INDEX_NONE;
	const bool bInHotbar = Inventory->IsItemInHotbar(ItemId, HotbarSlot);
	const float UnitWeight = Inventory->GetItemUnitWeight(ItemId);

	return FText::FromString(FString::Printf(
		TEXT("ITEM DETAILS\n\n[%s]  %s\n%s\n\n%s\n\nSTACK   x%d\nUNIT WEIGHT   %.2f kg\nSTACK WEIGHT   %.2f kg%s\n\nMOUSE\nLeft-drag: reorder / hotbar / world drop\nRight-click: split stack in half"),
		*Inventory->GetItemRarityName(ItemId),
		*Inventory->GetItemDisplayName(ItemId),
		*Inventory->GetItemCategoryName(ItemId),
		*Inventory->GetItemDescription(ItemId),
		Quantity,
		UnitWeight,
		UnitWeight * static_cast<float>(Quantity),
		bInHotbar ? *FString::Printf(TEXT("\nHOTBAR   %d"), HotbarSlot + 1) : TEXT("")));
}
