#include "SWildBoundLootWidget.h"

#include "../Inventory/WildBoundInventoryComponent.h"
#include "../Player/WildBoundInteractionComponent.h"
#include "DragAndDrop/DecoratedDragDropOp.h"
#include "InputCoreTypes.h"
#include "Styling/CoreStyle.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Layout/SSeparator.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Text/STextBlock.h"

namespace
{
	FSlateColor GetLootRaritySlateColor(const UWildBoundInventoryComponent* Inventory, FName ItemId)
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

	class FWildBoundLootDragDropOp : public FDecoratedDragDropOp
	{
	public:
		DRAG_DROP_OPERATOR_TYPE(FWildBoundLootDragDropOp, FDecoratedDragDropOp)

		int32 EntryIndex = INDEX_NONE;

		static TSharedRef<FWildBoundLootDragDropOp> New(int32 InEntryIndex, const FString& Label)
		{
			TSharedRef<FWildBoundLootDragDropOp> Operation = MakeShared<FWildBoundLootDragDropOp>();
			Operation->EntryIndex = InEntryIndex;
			Operation->DefaultHoverText = FText::FromString(Label);
			Operation->Construct();
			return Operation;
		}
	};

	class FWildBoundBackpackStashDragDropOp : public FDecoratedDragDropOp
	{
	public:
		DRAG_DROP_OPERATOR_TYPE(FWildBoundBackpackStashDragDropOp, FDecoratedDragDropOp)

		int32 StackIndex = INDEX_NONE;

		static TSharedRef<FWildBoundBackpackStashDragDropOp> New(int32 InStackIndex, const FString& Label)
		{
			TSharedRef<FWildBoundBackpackStashDragDropOp> Operation = MakeShared<FWildBoundBackpackStashDragDropOp>();
			Operation->StackIndex = InStackIndex;
			Operation->DefaultHoverText = FText::FromString(Label);
			Operation->Construct();
			return Operation;
		}
	};

	class SWildBoundLootRow : public SBorder
	{
	public:
		SLATE_BEGIN_ARGS(SWildBoundLootRow) {}
			SLATE_ARGUMENT(TWeakObjectPtr<UWildBoundInteractionComponent>, InteractionComponent)
			SLATE_ARGUMENT(int32, EntryIndex)
		SLATE_END_ARGS()

		void Construct(const FArguments& InArgs)
		{
			InteractionComponent = InArgs._InteractionComponent;
			EntryIndex = InArgs._EntryIndex;

			SBorder::Construct(
				SBorder::FArguments()
				.Padding(FMargin(10.0f, 8.0f))
				.BorderBackgroundColor(FLinearColor(0.045f, 0.055f, 0.050f, 0.96f))
				.ToolTipText(this, &SWildBoundLootRow::GetTooltipText)
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot().FillWidth(1.0f).VAlign(VAlign_Center)
					[
						SNew(SVerticalBox)
						+ SVerticalBox::Slot().AutoHeight()
						[
							SNew(STextBlock)
							.Text(this, &SWildBoundLootRow::GetRowText)
							.ColorAndOpacity(this, &SWildBoundLootRow::GetRowColor)
							.Font(FCoreStyle::GetDefaultFontStyle("Bold", 10))
						]
						+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 3.0f, 0.0f, 0.0f)
						[
							SNew(STextBlock)
							.Text(this, &SWildBoundLootRow::GetProjectedWeightText)
							.ColorAndOpacity(this, &SWildBoundLootRow::GetProjectedWeightColor)
							.Font(FCoreStyle::GetDefaultFontStyle("Regular", 8))
						]
					]
					+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(8.0f, 0.0f, 4.0f, 0.0f)
					[
						SNew(SButton)
						.Text(FText::FromString(TEXT("TAKE 1")))
						.OnClicked(this, &SWildBoundLootRow::HandleTakeOne)
					]
					+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
					[
						SNew(SButton)
						.Text(FText::FromString(TEXT("TAKE STACK")))
						.OnClicked(this, &SWildBoundLootRow::HandleTakeStack)
					]
				]);
		}

		virtual FReply OnMouseButtonDown(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override
		{
			if (MouseEvent.GetEffectingButton() == EKeys::RightMouseButton)
			{
				if (UWildBoundInteractionComponent* Interaction = InteractionComponent.Get())
				{
					Interaction->TakeLootEntry(EntryIndex, false);
					return FReply::Handled();
				}
			}

			if (MouseEvent.GetEffectingButton() == EKeys::LeftMouseButton)
			{
				return FReply::Handled().DetectDrag(SharedThis(this), EKeys::LeftMouseButton);
			}

			return SBorder::OnMouseButtonDown(MyGeometry, MouseEvent);
		}

		virtual FReply OnDragDetected(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override
		{
			const UWildBoundInteractionComponent* Interaction = InteractionComponent.Get();
			const UWildBoundInventoryComponent* Inventory = Interaction ? Interaction->GetInventoryComponent() : nullptr;
			const FWildBoundContainerLootEntry* Entry = GetEntry();
			if (!Inventory || !Entry)
			{
				return FReply::Unhandled();
			}

			const FString Label = FString::Printf(
				TEXT("TAKE  [%s] %s x%d"),
				*Inventory->GetItemRarityName(Entry->ItemId),
				*Inventory->GetItemDisplayName(Entry->ItemId),
				Entry->Quantity);

			return FReply::Handled().BeginDragDrop(
				FWildBoundLootDragDropOp::New(EntryIndex, Label));
		}

	private:
		TWeakObjectPtr<UWildBoundInteractionComponent> InteractionComponent;
		int32 EntryIndex = INDEX_NONE;

		const FWildBoundContainerLootEntry* GetEntry() const
		{
			const UWildBoundInteractionComponent* Interaction = InteractionComponent.Get();
			const TArray<FWildBoundContainerLootEntry>* Loot = Interaction ? Interaction->GetOpenContainerLoot() : nullptr;
			return Loot && Loot->IsValidIndex(EntryIndex) ? &(*Loot)[EntryIndex] : nullptr;
		}

		FReply HandleTakeOne()
		{
			if (UWildBoundInteractionComponent* Interaction = InteractionComponent.Get())
			{
				Interaction->TakeLootEntry(EntryIndex, false);
			}
			return FReply::Handled();
		}

		FReply HandleTakeStack()
		{
			if (UWildBoundInteractionComponent* Interaction = InteractionComponent.Get())
			{
				Interaction->TakeLootEntry(EntryIndex, true);
			}
			return FReply::Handled();
		}

		FText GetRowText() const
		{
			const UWildBoundInteractionComponent* Interaction = InteractionComponent.Get();
			const UWildBoundInventoryComponent* Inventory = Interaction ? Interaction->GetInventoryComponent() : nullptr;
			const FWildBoundContainerLootEntry* Entry = GetEntry();
			if (!Inventory || !Entry)
			{
				return FText::GetEmpty();
			}

			const float Weight = Inventory->GetItemUnitWeight(Entry->ItemId) * static_cast<float>(Entry->Quantity);
			return FText::FromString(FString::Printf(
				TEXT("[%s]  %s\n%s   x%d   %.2f kg"),
				*Inventory->GetItemRarityName(Entry->ItemId),
				*Inventory->GetItemDisplayName(Entry->ItemId),
				*Inventory->GetItemCategoryName(Entry->ItemId),
				Entry->Quantity,
				Weight));
		}

		FText GetProjectedWeightText() const
		{
			const UWildBoundInteractionComponent* Interaction = InteractionComponent.Get();
			const UWildBoundInventoryComponent* Inventory = Interaction ? Interaction->GetInventoryComponent() : nullptr;
			const FWildBoundContainerLootEntry* Entry = GetEntry();
			if (!Inventory || !Entry)
			{
				return FText::GetEmpty();
			}

			const float StackWeight = Inventory->GetItemUnitWeight(Entry->ItemId) * static_cast<float>(Entry->Quantity);
			const float Projected = Inventory->GetTotalWeight() + StackWeight;
			const bool bWouldOverEncumber = Projected > Inventory->MaxCarryWeight + KINDA_SMALL_NUMBER;
			return FText::FromString(FString::Printf(
				bWouldOverEncumber
					? TEXT("TAKE STACK -> %.2f / %.2f kg   OVER ENCUMBERED")
					: TEXT("TAKE STACK -> %.2f / %.2f kg"),
				Projected,
				Inventory->MaxCarryWeight));
		}

		FSlateColor GetProjectedWeightColor() const
		{
			const UWildBoundInteractionComponent* Interaction = InteractionComponent.Get();
			const UWildBoundInventoryComponent* Inventory = Interaction ? Interaction->GetInventoryComponent() : nullptr;
			const FWildBoundContainerLootEntry* Entry = GetEntry();
			if (!Inventory || !Entry)
			{
				return FSlateColor(FLinearColor(0.62f, 0.65f, 0.60f, 1.0f));
			}

			const float Projected = Inventory->GetTotalWeight()
				+ Inventory->GetItemUnitWeight(Entry->ItemId) * static_cast<float>(Entry->Quantity);
			return Projected > Inventory->MaxCarryWeight + KINDA_SMALL_NUMBER
				? FSlateColor(FLinearColor(0.95f, 0.42f, 0.20f, 1.0f))
				: FSlateColor(FLinearColor(0.58f, 0.68f, 0.57f, 1.0f));
		}

		FText GetTooltipText() const
		{
			const UWildBoundInteractionComponent* Interaction = InteractionComponent.Get();
			const UWildBoundInventoryComponent* Inventory = Interaction ? Interaction->GetInventoryComponent() : nullptr;
			const FWildBoundContainerLootEntry* Entry = GetEntry();
			if (!Inventory || !Entry)
			{
				return FText::GetEmpty();
			}

			return FText::FromString(FString::Printf(
				TEXT("%s\n%s  •  %s\n%s\n\nStack: %d\nUnit weight: %.2f kg\nStack weight: %.2f kg\n\nDrag to BACKPACK to take the full stack.\nRight-click takes one."),
				*Inventory->GetItemDisplayName(Entry->ItemId),
				*Inventory->GetItemRarityName(Entry->ItemId),
				*Inventory->GetItemCategoryName(Entry->ItemId),
				*Inventory->GetItemDescription(Entry->ItemId),
				Entry->Quantity,
				Inventory->GetItemUnitWeight(Entry->ItemId),
				Inventory->GetItemUnitWeight(Entry->ItemId) * static_cast<float>(Entry->Quantity)));
		}

		FSlateColor GetRowColor() const
		{
			const UWildBoundInteractionComponent* Interaction = InteractionComponent.Get();
			const UWildBoundInventoryComponent* Inventory = Interaction ? Interaction->GetInventoryComponent() : nullptr;
			const FWildBoundContainerLootEntry* Entry = GetEntry();
			return Inventory && Entry
				? GetLootRaritySlateColor(Inventory, Entry->ItemId)
				: FSlateColor(FLinearColor::White);
		}
	};

	class SWildBoundStashBackpackRow : public SBorder
	{
	public:
		SLATE_BEGIN_ARGS(SWildBoundStashBackpackRow) {}
			SLATE_ARGUMENT(TWeakObjectPtr<UWildBoundInteractionComponent>, InteractionComponent)
			SLATE_ARGUMENT(int32, StackIndex)
		SLATE_END_ARGS()

		void Construct(const FArguments& InArgs)
		{
			InteractionComponent = InArgs._InteractionComponent;
			StackIndex = InArgs._StackIndex;

			SBorder::Construct(
				SBorder::FArguments()
				.Padding(FMargin(10.0f, 8.0f))
				.BorderBackgroundColor(FLinearColor(0.035f, 0.045f, 0.041f, 0.96f))
				.ToolTipText(this, &SWildBoundStashBackpackRow::GetTooltipText)
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot().FillWidth(1.0f).VAlign(VAlign_Center)
					[
						SNew(STextBlock)
						.Text(this, &SWildBoundStashBackpackRow::GetRowText)
						.ColorAndOpacity(this, &SWildBoundStashBackpackRow::GetRowColor)
						.Font(FCoreStyle::GetDefaultFontStyle("Bold", 10))
					]
					+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(8.0f, 0.0f, 4.0f, 0.0f)
					[
						SNew(SButton)
						.Text(FText::FromString(TEXT("STORE 1")))
						.OnClicked(this, &SWildBoundStashBackpackRow::HandleStoreOne)
					]
					+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
					[
						SNew(SButton)
						.Text(FText::FromString(TEXT("STORE STACK")))
						.OnClicked(this, &SWildBoundStashBackpackRow::HandleStoreStack)
					]
				]);
		}

		virtual FReply OnMouseButtonDown(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override
		{
			if (MouseEvent.GetEffectingButton() == EKeys::RightMouseButton)
			{
				if (UWildBoundInteractionComponent* Interaction = InteractionComponent.Get())
				{
					Interaction->StoreInventoryStackInOpenContainer(StackIndex, false);
					return FReply::Handled();
				}
			}

			if (MouseEvent.GetEffectingButton() == EKeys::LeftMouseButton)
			{
				return FReply::Handled().DetectDrag(SharedThis(this), EKeys::LeftMouseButton);
			}

			return SBorder::OnMouseButtonDown(MyGeometry, MouseEvent);
		}

		virtual FReply OnDragDetected(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override
		{
			const UWildBoundInteractionComponent* Interaction = InteractionComponent.Get();
			const UWildBoundInventoryComponent* Inventory = Interaction ? Interaction->GetInventoryComponent() : nullptr;
			if (!Inventory || !Inventory->Stacks.IsValidIndex(StackIndex))
			{
				return FReply::Unhandled();
			}

			const FWildBoundInventoryStack& Stack = Inventory->Stacks[StackIndex];
			const FString Label = FString::Printf(
				TEXT("STORE  [%s] %s x%d"),
				*Inventory->GetItemRarityName(Stack.ItemId),
				*Inventory->GetItemDisplayName(Stack.ItemId),
				Stack.Quantity);

			return FReply::Handled().BeginDragDrop(
				FWildBoundBackpackStashDragDropOp::New(StackIndex, Label));
		}

	private:
		TWeakObjectPtr<UWildBoundInteractionComponent> InteractionComponent;
		int32 StackIndex = INDEX_NONE;

		FReply HandleStoreOne()
		{
			if (UWildBoundInteractionComponent* Interaction = InteractionComponent.Get())
			{
				Interaction->StoreInventoryStackInOpenContainer(StackIndex, false);
			}
			return FReply::Handled();
		}

		FReply HandleStoreStack()
		{
			if (UWildBoundInteractionComponent* Interaction = InteractionComponent.Get())
			{
				Interaction->StoreInventoryStackInOpenContainer(StackIndex, true);
			}
			return FReply::Handled();
		}

		FText GetRowText() const
		{
			const UWildBoundInteractionComponent* Interaction = InteractionComponent.Get();
			const UWildBoundInventoryComponent* Inventory = Interaction ? Interaction->GetInventoryComponent() : nullptr;
			if (!Inventory || !Inventory->Stacks.IsValidIndex(StackIndex))
			{
				return FText::GetEmpty();
			}

			const FWildBoundInventoryStack& Stack = Inventory->Stacks[StackIndex];
			int32 HotbarSlot = INDEX_NONE;
			const bool bInHotbar = Inventory->IsItemInHotbar(Stack.ItemId, HotbarSlot);
			const FString HotbarText = bInHotbar
				? FString::Printf(TEXT("  •  HOTBAR %d"), HotbarSlot + 1)
				: FString();
			const float StackWeight = Inventory->GetItemUnitWeight(Stack.ItemId) * static_cast<float>(Stack.Quantity);

			return FText::FromString(FString::Printf(
				TEXT("[%s]  %s\n%s   x%d   %.2f kg%s"),
				*Inventory->GetItemRarityName(Stack.ItemId),
				*Inventory->GetItemDisplayName(Stack.ItemId),
				*Inventory->GetItemCategoryName(Stack.ItemId),
				Stack.Quantity,
				StackWeight,
				*HotbarText));
		}

		FText GetTooltipText() const
		{
			const UWildBoundInteractionComponent* Interaction = InteractionComponent.Get();
			const UWildBoundInventoryComponent* Inventory = Interaction ? Interaction->GetInventoryComponent() : nullptr;
			if (!Inventory || !Inventory->Stacks.IsValidIndex(StackIndex))
			{
				return FText::GetEmpty();
			}

			const FWildBoundInventoryStack& Stack = Inventory->Stacks[StackIndex];
			return FText::FromString(FString::Printf(
				TEXT("%s\n%s  •  %s\n%s\n\nStack: %d\nUnit weight: %.2f kg\nStack weight: %.2f kg\n\nDrag to CONTAINER to store the full stack.\nRight-click stores one.\nPassive gear bonuses stop while that gear is stored."),
				*Inventory->GetItemDisplayName(Stack.ItemId),
				*Inventory->GetItemRarityName(Stack.ItemId),
				*Inventory->GetItemCategoryName(Stack.ItemId),
				*Inventory->GetItemDescription(Stack.ItemId),
				Stack.Quantity,
				Inventory->GetItemUnitWeight(Stack.ItemId),
				Inventory->GetItemUnitWeight(Stack.ItemId) * static_cast<float>(Stack.Quantity)));
		}

		FSlateColor GetRowColor() const
		{
			const UWildBoundInteractionComponent* Interaction = InteractionComponent.Get();
			const UWildBoundInventoryComponent* Inventory = Interaction ? Interaction->GetInventoryComponent() : nullptr;
			return Inventory && Inventory->Stacks.IsValidIndex(StackIndex)
				? GetLootRaritySlateColor(Inventory, Inventory->Stacks[StackIndex].ItemId)
				: FSlateColor(FLinearColor::White);
		}
	};

	class SWildBoundBackpackLootDropTarget : public SBorder
	{
	public:
		SLATE_BEGIN_ARGS(SWildBoundBackpackLootDropTarget) {}
			SLATE_ARGUMENT(TWeakObjectPtr<UWildBoundInteractionComponent>, InteractionComponent)
		SLATE_END_ARGS()

		void Construct(const FArguments& InArgs)
		{
			InteractionComponent = InArgs._InteractionComponent;
			SBorder::Construct(
				SBorder::FArguments()
				.Padding(FMargin(10.0f, 7.0f))
				.BorderBackgroundColor(FLinearColor(0.030f, 0.060f, 0.040f, 0.98f))
				[
					SNew(STextBlock)
					.Text(FText::FromString(TEXT("DROP CONTAINER LOOT HERE  →  TAKE STACK")))
					.Font(FCoreStyle::GetDefaultFontStyle("Bold", 9))
					.ColorAndOpacity(FLinearColor(0.65f, 0.83f, 0.64f, 1.0f))
				]);
		}

		virtual FReply OnDrop(const FGeometry& MyGeometry, const FDragDropEvent& DragDropEvent) override
		{
			const TSharedPtr<FWildBoundLootDragDropOp> Operation = DragDropEvent.GetOperationAs<FWildBoundLootDragDropOp>();
			UWildBoundInteractionComponent* Interaction = InteractionComponent.Get();
			if (Operation.IsValid() && Interaction && Interaction->TakeLootEntry(Operation->EntryIndex, true))
			{
				return FReply::Handled();
			}
			return FReply::Unhandled();
		}

	private:
		TWeakObjectPtr<UWildBoundInteractionComponent> InteractionComponent;
	};

	class SWildBoundContainerStashDropTarget : public SBorder
	{
	public:
		SLATE_BEGIN_ARGS(SWildBoundContainerStashDropTarget) {}
			SLATE_ARGUMENT(TWeakObjectPtr<UWildBoundInteractionComponent>, InteractionComponent)
		SLATE_END_ARGS()

		void Construct(const FArguments& InArgs)
		{
			InteractionComponent = InArgs._InteractionComponent;
			SBorder::Construct(
				SBorder::FArguments()
				.Padding(FMargin(10.0f, 7.0f))
				.BorderBackgroundColor(FLinearColor(0.065f, 0.050f, 0.025f, 0.98f))
				[
					SNew(STextBlock)
					.Text(FText::FromString(TEXT("DROP BACKPACK ITEM HERE  →  STORE STACK")))
					.Font(FCoreStyle::GetDefaultFontStyle("Bold", 9))
					.ColorAndOpacity(FLinearColor(0.86f, 0.72f, 0.46f, 1.0f))
				]);
		}

		virtual FReply OnDrop(const FGeometry& MyGeometry, const FDragDropEvent& DragDropEvent) override
		{
			const TSharedPtr<FWildBoundBackpackStashDragDropOp> Operation = DragDropEvent.GetOperationAs<FWildBoundBackpackStashDragDropOp>();
			UWildBoundInteractionComponent* Interaction = InteractionComponent.Get();
			if (Operation.IsValid() && Interaction
				&& Interaction->StoreInventoryStackInOpenContainer(Operation->StackIndex, true))
			{
				return FReply::Handled();
			}
			return FReply::Unhandled();
		}

	private:
		TWeakObjectPtr<UWildBoundInteractionComponent> InteractionComponent;
	};
}

void SWildBoundLootWidget::Construct(const FArguments& InArgs)
{
	InteractionComponent = InArgs._InteractionComponent;

	ChildSlot
	[
		SNew(SBorder)
		.Padding(FMargin(22.0f, 18.0f))
		.BorderBackgroundColor(FLinearColor(0.010f, 0.014f, 0.013f, 0.985f))
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot().AutoHeight()
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().FillWidth(1.0f)
				[
					SNew(STextBlock)
					.Text(this, &SWildBoundLootWidget::GetHeaderText)
					.Font(FCoreStyle::GetDefaultFontStyle("Bold", 18))
				]
				+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
				[
					SNew(SButton)
					.Text(FText::FromString(TEXT("CLOSE")))
					.OnClicked(this, &SWildBoundLootWidget::HandleClose)
				]
			]
			+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 6.0f, 0.0f, 0.0f)
			[
				SNew(STextBlock)
				.Text(this, &SWildBoundLootWidget::GetCarryText)
				.Font(FCoreStyle::GetDefaultFontStyle("Regular", 9))
				.ColorAndOpacity(FLinearColor(0.64f, 0.68f, 0.62f, 1.0f))
			]
			+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 8.0f)
			[
				SNew(SSeparator)
			]
			+ SVerticalBox::Slot().AutoHeight()
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().FillWidth(0.50f).Padding(0.0f, 0.0f, 8.0f, 0.0f)
				[
					SNew(SBorder)
					.Padding(FMargin(12.0f, 10.0f))
					.BorderBackgroundColor(FLinearColor(0.030f, 0.035f, 0.032f, 0.96f))
					[
						SNew(SVerticalBox)
						+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 0.0f, 0.0f, 7.0f)
						[
							SNew(STextBlock)
							.Text(FText::FromString(TEXT("CONTAINER")))
							.Font(FCoreStyle::GetDefaultFontStyle("Bold", 12))
						]
						+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 0.0f, 0.0f, 7.0f)
						[
							SNew(SWildBoundContainerStashDropTarget)
							.InteractionComponent(InteractionComponent)
						]
						+ SVerticalBox::Slot().AutoHeight()
						[
							SNew(SBox)
							.HeightOverride(390.0f)
							[
								SNew(SScrollBox)
								+ SScrollBox::Slot()
								[
									SAssignNew(LootRowsBox, SVerticalBox)
								]
							]
						]
					]
				]
				+ SHorizontalBox::Slot().FillWidth(0.50f).Padding(8.0f, 0.0f, 0.0f, 0.0f)
				[
					SNew(SBorder)
					.Padding(FMargin(12.0f, 10.0f))
					.BorderBackgroundColor(FLinearColor(0.025f, 0.040f, 0.033f, 0.96f))
					[
						SNew(SVerticalBox)
						+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 0.0f, 0.0f, 7.0f)
						[
							SNew(STextBlock)
							.Text(FText::FromString(TEXT("BACKPACK")))
							.Font(FCoreStyle::GetDefaultFontStyle("Bold", 12))
						]
						+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 0.0f, 0.0f, 7.0f)
						[
							SNew(SWildBoundBackpackLootDropTarget)
							.InteractionComponent(InteractionComponent)
						]
						+ SVerticalBox::Slot().AutoHeight()
						[
							SNew(SBox)
							.HeightOverride(390.0f)
							[
								SNew(SScrollBox)
								+ SScrollBox::Slot()
								[
									SAssignNew(BackpackRowsBox, SVerticalBox)
								]
							]
						]
					]
				]
			]
			+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 12.0f, 0.0f, 0.0f)
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().FillWidth(1.0f).VAlign(VAlign_Center)
				[
					SNew(STextBlock)
					.Text(FText::FromString(TEXT("Right-click = move 1. Drag = move full stack. Stored items remain in this exact container. Passive gear bonuses stop while stored.")))
					.Font(FCoreStyle::GetDefaultFontStyle("Regular", 9))
					.ColorAndOpacity(FLinearColor(0.58f, 0.61f, 0.57f, 1.0f))
					.AutoWrapText(true)
				]
				+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(10.0f, 0.0f, 0.0f, 0.0f)
				[
					SNew(SButton)
					.Text(FText::FromString(TEXT("TAKE ALL")))
					.OnClicked(this, &SWildBoundLootWidget::HandleTakeAll)
				]
			]
		]
	];

	RebuildRows();
	CachedStateSignature = CalculateStateSignature();
}

void SWildBoundLootWidget::SetInteractionComponent(UWildBoundInteractionComponent* InInteractionComponent)
{
	InteractionComponent = InInteractionComponent;
}

void SWildBoundLootWidget::Tick(
	const FGeometry& AllottedGeometry,
	const double InCurrentTime,
	const float InDeltaTime)
{
	SCompoundWidget::Tick(AllottedGeometry, InCurrentTime, InDeltaTime);

	const uint32 NewSignature = CalculateStateSignature();
	if (NewSignature != CachedStateSignature)
	{
		CachedStateSignature = NewSignature;
		RebuildRows();
	}
}

void SWildBoundLootWidget::RebuildRows()
{
	RebuildLootRows();
	RebuildBackpackRows();
}

void SWildBoundLootWidget::RebuildLootRows()
{
	if (!LootRowsBox.IsValid())
	{
		return;
	}

	LootRowsBox->ClearChildren();
	const UWildBoundInteractionComponent* Interaction = InteractionComponent.Get();
	const TArray<FWildBoundContainerLootEntry>* Loot = Interaction ? Interaction->GetOpenContainerLoot() : nullptr;
	if (!Loot || Loot->IsEmpty())
	{
		LootRowsBox->AddSlot().AutoHeight().Padding(0.0f, 4.0f)
		[
			SNew(STextBlock)
			.Text(FText::FromString(TEXT("— EMPTY —\nStore items here to use this container as a stash.")))
			.Font(FCoreStyle::GetDefaultFontStyle("Bold", 11))
			.ColorAndOpacity(FLinearColor(0.54f, 0.56f, 0.52f, 1.0f))
		];
		return;
	}

	for (int32 Index = 0; Index < Loot->Num(); ++Index)
	{
		LootRowsBox->AddSlot().AutoHeight().Padding(0.0f, 3.0f)
		[
			SNew(SWildBoundLootRow)
			.InteractionComponent(InteractionComponent)
			.EntryIndex(Index)
		];
	}
}

void SWildBoundLootWidget::RebuildBackpackRows()
{
	if (!BackpackRowsBox.IsValid())
	{
		return;
	}

	BackpackRowsBox->ClearChildren();
	const UWildBoundInteractionComponent* Interaction = InteractionComponent.Get();
	const UWildBoundInventoryComponent* Inventory = Interaction ? Interaction->GetInventoryComponent() : nullptr;
	if (!Inventory || Inventory->Stacks.IsEmpty())
	{
		BackpackRowsBox->AddSlot().AutoHeight().Padding(0.0f, 4.0f)
		[
			SNew(STextBlock)
			.Text(FText::FromString(TEXT("— BACKPACK EMPTY —")))
			.Font(FCoreStyle::GetDefaultFontStyle("Bold", 11))
			.ColorAndOpacity(FLinearColor(0.54f, 0.56f, 0.52f, 1.0f))
		];
		return;
	}

	for (int32 Index = 0; Index < Inventory->Stacks.Num(); ++Index)
	{
		BackpackRowsBox->AddSlot().AutoHeight().Padding(0.0f, 3.0f)
		[
			SNew(SWildBoundStashBackpackRow)
			.InteractionComponent(InteractionComponent)
			.StackIndex(Index)
		];
	}
}

uint32 SWildBoundLootWidget::CalculateStateSignature() const
{
	const UWildBoundInteractionComponent* Interaction = InteractionComponent.Get();
	const UWildBoundInventoryComponent* Inventory = Interaction ? Interaction->GetInventoryComponent() : nullptr;
	const TArray<FWildBoundContainerLootEntry>* Loot = Interaction ? Interaction->GetOpenContainerLoot() : nullptr;

	uint32 Signature = 0;
	if (Loot)
	{
		Signature = HashCombine(Signature, GetTypeHash(Loot->Num()));
		for (const FWildBoundContainerLootEntry& Entry : *Loot)
		{
			Signature = HashCombine(Signature, GetTypeHash(Entry.ItemId));
			Signature = HashCombine(Signature, GetTypeHash(Entry.Quantity));
		}
	}

	if (Inventory)
	{
		Signature = HashCombine(Signature, GetTypeHash(Inventory->Stacks.Num()));
		for (const FWildBoundInventoryStack& Stack : Inventory->Stacks)
		{
			Signature = HashCombine(Signature, GetTypeHash(Stack.ItemId));
			Signature = HashCombine(Signature, GetTypeHash(Stack.Quantity));
		}
		for (const FName& HotbarItem : Inventory->HotbarSlots)
		{
			Signature = HashCombine(Signature, GetTypeHash(HotbarItem));
		}
		Signature = HashCombine(Signature, GetTypeHash(Inventory->MaxSlots));
		Signature = HashCombine(Signature, GetTypeHash(Inventory->MaxCarryWeight));
	}
	return Signature;
}

FText SWildBoundLootWidget::GetHeaderText() const
{
	const UWildBoundInteractionComponent* Interaction = InteractionComponent.Get();
	return Interaction
		? FText::FromString(FString::Printf(
			TEXT("[%s] %s  —  STORAGE"),
			*Interaction->GetOpenContainerQualityName(),
			*Interaction->GetOpenContainerName()))
		: FText::FromString(TEXT("CONTAINER STORAGE"));
}

FText SWildBoundLootWidget::GetCarryText() const
{
	const UWildBoundInteractionComponent* Interaction = InteractionComponent.Get();
	const UWildBoundInventoryComponent* Inventory = Interaction ? Interaction->GetInventoryComponent() : nullptr;
	const TArray<FWildBoundContainerLootEntry>* Loot = Interaction ? Interaction->GetOpenContainerLoot() : nullptr;
	if (!Inventory)
	{
		return FText::GetEmpty();
	}

	float ContainerWeight = 0.0f;
	if (Loot)
	{
		for (const FWildBoundContainerLootEntry& Entry : *Loot)
		{
			ContainerWeight += Inventory->GetItemUnitWeight(Entry.ItemId) * static_cast<float>(Entry.Quantity);
		}
	}

	const float CurrentWeight = Inventory->GetTotalWeight();
	const float ProjectedAllWeight = CurrentWeight + ContainerWeight;
	const bool bTakeAllWouldOverEncumber = ProjectedAllWeight > Inventory->MaxCarryWeight + KINDA_SMALL_NUMBER;

	return FText::FromString(FString::Printf(
		bTakeAllWouldOverEncumber
			? TEXT("BACKPACK %.2f / %.2f kg   •   SLOTS %d / %d   •   CONTAINER %.2f kg   •   TAKE ALL -> %.2f kg  [OVER ENCUMBERED]")
			: TEXT("BACKPACK %.2f / %.2f kg   •   SLOTS %d / %d   •   CONTAINER %.2f kg   •   TAKE ALL -> %.2f kg"),
		CurrentWeight,
		Inventory->MaxCarryWeight,
		Inventory->Stacks.Num(),
		Inventory->MaxSlots,
		ContainerWeight,
		ProjectedAllWeight));
}

FReply SWildBoundLootWidget::HandleTakeAll()
{
	if (UWildBoundInteractionComponent* Interaction = InteractionComponent.Get())
	{
		Interaction->TakeAllContainerLoot();
	}
	return FReply::Handled();
}

FReply SWildBoundLootWidget::HandleClose()
{
	if (UWildBoundInteractionComponent* Interaction = InteractionComponent.Get())
	{
		Interaction->CloseLootWindow();
	}
	return FReply::Handled();
}
