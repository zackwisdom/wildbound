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
						SNew(STextBlock)
						.Text(this, &SWildBoundLootRow::GetRowText)
						.ColorAndOpacity(this, &SWildBoundLootRow::GetRowColor)
						.Font(FCoreStyle::GetDefaultFontStyle("Bold", 10))
					]
					+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
					[
						SNew(STextBlock)
						.Text(FText::FromString(TEXT("CLICK TAKE  •  RIGHT-CLICK TAKE 1  •  DRAG TO BACKPACK")))
						.Font(FCoreStyle::GetDefaultFontStyle("Regular", 8))
						.ColorAndOpacity(FLinearColor(0.58f, 0.62f, 0.57f, 1.0f))
					]
				]);
		}

		virtual FReply OnMouseButtonDown(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override
		{
			UWildBoundInteractionComponent* Interaction = InteractionComponent.Get();
			if (!Interaction)
			{
				return FReply::Unhandled();
			}

			if (MouseEvent.GetEffectingButton() == EKeys::RightMouseButton)
			{
				Interaction->TakeLootEntry(EntryIndex, false);
				return FReply::Handled();
			}

			if (MouseEvent.GetEffectingButton() == EKeys::LeftMouseButton)
			{
				return FReply::Handled().DetectDrag(SharedThis(this), EKeys::LeftMouseButton);
			}

			return SBorder::OnMouseButtonDown(MyGeometry, MouseEvent);
		}

		virtual FReply OnMouseButtonUp(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override
		{
			if (MouseEvent.GetEffectingButton() == EKeys::LeftMouseButton)
			{
				if (UWildBoundInteractionComponent* Interaction = InteractionComponent.Get())
				{
					Interaction->TakeLootEntry(EntryIndex, true);
					return FReply::Handled();
				}
			}
			return SBorder::OnMouseButtonUp(MyGeometry, MouseEvent);
		}

		virtual FReply OnDragDetected(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override
		{
			const UWildBoundInteractionComponent* Interaction = InteractionComponent.Get();
			const UWildBoundInventoryComponent* Inventory = Interaction ? Interaction->GetInventoryComponent() : nullptr;
			const TArray<FWildBoundContainerLootEntry>* Loot = Interaction ? Interaction->GetOpenContainerLoot() : nullptr;
			if (!Inventory || !Loot || !Loot->IsValidIndex(EntryIndex))
			{
				return FReply::Unhandled();
			}

			const FWildBoundContainerLootEntry& Entry = (*Loot)[EntryIndex];
			const FString Label = FString::Printf(TEXT("[%s] %s x%d"),
				*Inventory->GetItemRarityName(Entry.ItemId),
				*Inventory->GetItemDisplayName(Entry.ItemId),
				Entry.Quantity);
			return FReply::Handled().BeginDragDrop(FWildBoundLootDragDropOp::New(EntryIndex, Label));
		}

	private:
		TWeakObjectPtr<UWildBoundInteractionComponent> InteractionComponent;
		int32 EntryIndex = INDEX_NONE;

		FText GetRowText() const
		{
			const UWildBoundInteractionComponent* Interaction = InteractionComponent.Get();
			const UWildBoundInventoryComponent* Inventory = Interaction ? Interaction->GetInventoryComponent() : nullptr;
			const TArray<FWildBoundContainerLootEntry>* Loot = Interaction ? Interaction->GetOpenContainerLoot() : nullptr;
			if (!Inventory || !Loot || !Loot->IsValidIndex(EntryIndex))
			{
				return FText::GetEmpty();
			}

			const FWildBoundContainerLootEntry& Entry = (*Loot)[EntryIndex];
			const float Weight = Inventory->GetItemUnitWeight(Entry.ItemId) * static_cast<float>(Entry.Quantity);
			return FText::FromString(FString::Printf(TEXT("[%s]  %s   x%d   %.2f kg"),
				*Inventory->GetItemRarityName(Entry.ItemId),
				*Inventory->GetItemDisplayName(Entry.ItemId),
				Entry.Quantity,
				Weight));
		}

		FText GetTooltipText() const
		{
			const UWildBoundInteractionComponent* Interaction = InteractionComponent.Get();
			const UWildBoundInventoryComponent* Inventory = Interaction ? Interaction->GetInventoryComponent() : nullptr;
			const TArray<FWildBoundContainerLootEntry>* Loot = Interaction ? Interaction->GetOpenContainerLoot() : nullptr;
			if (!Inventory || !Loot || !Loot->IsValidIndex(EntryIndex))
			{
				return FText::GetEmpty();
			}

			const FWildBoundContainerLootEntry& Entry = (*Loot)[EntryIndex];
			return FText::FromString(FString::Printf(TEXT("%s\n%s\n%s\n\nStack: %d\nUnit weight: %.2f kg\nStack weight: %.2f kg"),
				*Inventory->GetItemDisplayName(Entry.ItemId),
				*Inventory->GetItemRarityName(Entry.ItemId),
				*Inventory->GetItemDescription(Entry.ItemId),
				Entry.Quantity,
				Inventory->GetItemUnitWeight(Entry.ItemId),
				Inventory->GetItemUnitWeight(Entry.ItemId) * static_cast<float>(Entry.Quantity)));
		}

		FSlateColor GetRowColor() const
		{
			const UWildBoundInteractionComponent* Interaction = InteractionComponent.Get();
			const UWildBoundInventoryComponent* Inventory = Interaction ? Interaction->GetInventoryComponent() : nullptr;
			const TArray<FWildBoundContainerLootEntry>* Loot = Interaction ? Interaction->GetOpenContainerLoot() : nullptr;
			return Inventory && Loot && Loot->IsValidIndex(EntryIndex)
				? GetLootRaritySlateColor(Inventory, (*Loot)[EntryIndex].ItemId)
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
				.Padding(FMargin(14.0f, 12.0f))
				.BorderBackgroundColor(FLinearColor(0.030f, 0.045f, 0.038f, 0.97f))
				[
					SNew(STextBlock)
					.Text(FText::FromString(TEXT("BACKPACK\n\nDRAG LOOT HERE\n\nClicking an item also takes its full stack.\nRight-click takes one item.")))
					.Font(FCoreStyle::GetDefaultFontStyle("Bold", 11))
					.ColorAndOpacity(FLinearColor(0.72f, 0.82f, 0.70f, 1.0f))
					.AutoWrapText(true)
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
				+ SHorizontalBox::Slot().FillWidth(0.66f).Padding(0.0f, 0.0f, 12.0f, 0.0f)
				[
					SNew(SBox)
					.HeightOverride(360.0f)
					[
						SNew(SScrollBox)
						+ SScrollBox::Slot()
						[
							SAssignNew(LootRowsBox, SVerticalBox)
						]
					]
				]
				+ SHorizontalBox::Slot().FillWidth(0.34f)
				[
					SNew(SWildBoundBackpackLootDropTarget)
					.InteractionComponent(InteractionComponent)
				]
			]
			+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 12.0f, 0.0f, 0.0f)
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().FillWidth(1.0f)
				[
					SNew(STextBlock)
					.Text(FText::FromString(TEXT("Leave anything you do not want. You can reopen this container later.")))
					.Font(FCoreStyle::GetDefaultFontStyle("Regular", 9))
					.ColorAndOpacity(FLinearColor(0.58f, 0.61f, 0.57f, 1.0f))
				]
				+ SHorizontalBox::Slot().AutoWidth()
				[
					SNew(SButton)
					.Text(FText::FromString(TEXT("TAKE ALL")))
					.OnClicked(this, &SWildBoundLootWidget::HandleTakeAll)
				]
			]
		]
	];

	RebuildLootRows();
	CachedLootSignature = CalculateLootSignature();
}

void SWildBoundLootWidget::SetInteractionComponent(UWildBoundInteractionComponent* InInteractionComponent)
{
	InteractionComponent = InInteractionComponent;
}

void SWildBoundLootWidget::Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime)
{
	SCompoundWidget::Tick(AllottedGeometry, InCurrentTime, InDeltaTime);
	const uint32 NewSignature = CalculateLootSignature();
	if (NewSignature != CachedLootSignature)
	{
		CachedLootSignature = NewSignature;
		RebuildLootRows();
	}
}

void SWildBoundLootWidget::RebuildLootRows()
{
	if (!LootRowsBox.IsValid()) return;
	LootRowsBox->ClearChildren();

	const UWildBoundInteractionComponent* Interaction = InteractionComponent.Get();
	const TArray<FWildBoundContainerLootEntry>* Loot = Interaction ? Interaction->GetOpenContainerLoot() : nullptr;
	if (!Loot || Loot->IsEmpty())
	{
		LootRowsBox->AddSlot().AutoHeight().Padding(0.0f, 4.0f)
		[
			SNew(STextBlock)
			.Text(FText::FromString(TEXT("— EMPTY —")))
			.Font(FCoreStyle::GetDefaultFontStyle("Bold", 12))
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

uint32 SWildBoundLootWidget::CalculateLootSignature() const
{
	const UWildBoundInteractionComponent* Interaction = InteractionComponent.Get();
	const TArray<FWildBoundContainerLootEntry>* Loot = Interaction ? Interaction->GetOpenContainerLoot() : nullptr;
	if (!Loot) return 0;

	uint32 Signature = GetTypeHash(Loot->Num());
	for (const FWildBoundContainerLootEntry& Entry : *Loot)
	{
		Signature = HashCombine(Signature, GetTypeHash(Entry.ItemId));
		Signature = HashCombine(Signature, GetTypeHash(Entry.Quantity));
	}
	return Signature;
}

FText SWildBoundLootWidget::GetHeaderText() const
{
	const UWildBoundInteractionComponent* Interaction = InteractionComponent.Get();
	return Interaction
		? FText::FromString(FString::Printf(TEXT("[%s] %s"), *Interaction->GetOpenContainerQualityName(), *Interaction->GetOpenContainerName()))
		: FText::FromString(TEXT("CONTAINER"));
}

FText SWildBoundLootWidget::GetCarryText() const
{
	const UWildBoundInteractionComponent* Interaction = InteractionComponent.Get();
	const UWildBoundInventoryComponent* Inventory = Interaction ? Interaction->GetInventoryComponent() : nullptr;
	return Inventory
		? FText::FromString(FString::Printf(TEXT("BACKPACK   %.2f / %.2f kg      SLOTS   %d / %d"),
			Inventory->GetTotalWeight(), Inventory->MaxCarryWeight, Inventory->Stacks.Num(), Inventory->MaxSlots))
		: FText::GetEmpty();
}

FReply SWildBoundLootWidget::HandleTakeAll()
{
	if (UWildBoundInteractionComponent* Interaction = InteractionComponent.Get()) Interaction->TakeAllContainerLoot();
	return FReply::Handled();
}

FReply SWildBoundLootWidget::HandleClose()
{
	if (UWildBoundInteractionComponent* Interaction = InteractionComponent.Get()) Interaction->CloseLootWindow();
	return FReply::Handled();
}
