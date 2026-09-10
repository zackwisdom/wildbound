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
	FLinearColor GetRarityColorLinear(const UWildBoundInventoryComponent* Inventory, FName ItemId)
	{
		const int32 Tier = Inventory ? Inventory->GetItemRarityTier(ItemId) : 0;
		switch (Tier)
		{
		case 3: return FLinearColor(0.84f, 0.48f, 0.96f, 1.0f);
		case 2: return FLinearColor(0.38f, 0.67f, 0.96f, 1.0f);
		case 1: return FLinearColor(0.46f, 0.82f, 0.50f, 1.0f);
		default: return FLinearColor(0.78f, 0.81f, 0.76f, 1.0f);
		}
	}

	FSlateColor GetRaritySlateColor(const UWildBoundInventoryComponent* Inventory, FName ItemId)
	{
		return FSlateColor(GetRarityColorLinear(Inventory, ItemId));
	}

	FLinearColor GetRarityBackground(const UWildBoundInventoryComponent* Inventory, FName ItemId)
	{
		const int32 Tier = Inventory ? Inventory->GetItemRarityTier(ItemId) : 0;
		switch (Tier)
		{
		case 3: return FLinearColor(0.18f, 0.075f, 0.23f, 0.98f);
		case 2: return FLinearColor(0.055f, 0.105f, 0.19f, 0.98f);
		case 1: return FLinearColor(0.055f, 0.145f, 0.070f, 0.98f);
		default: return FLinearColor(0.085f, 0.095f, 0.090f, 0.98f);
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
				.Padding(FMargin(7.0f, 6.0f))
				.BorderBackgroundColor(FLinearColor(0.032f, 0.040f, 0.036f, 0.98f))
				.ToolTipText(this, &SWildBoundLootRow::GetTooltipText)
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0.0f,0.0f,8.0f,0.0f)
					[
						SNew(SBox).WidthOverride(42.0f).HeightOverride(42.0f)
						[
							SNew(SBorder)
							.Padding(FMargin(2.0f))
							.BorderBackgroundColor(this, &SWildBoundLootRow::GetRarityColor)
							[
								SNew(SBorder)
								.Padding(FMargin(3.0f))
								.BorderBackgroundColor(this, &SWildBoundLootRow::GetIconBackground)
								[
									SNew(STextBlock)
									.Text(this, &SWildBoundLootRow::GetIconText)
									.Font(FCoreStyle::GetDefaultFontStyle("Bold", 8))
									.ColorAndOpacity(this, &SWildBoundLootRow::GetRarityColor)
									.Justification(ETextJustify::Center)
								]
							]
						]
					]
					+ SHorizontalBox::Slot().FillWidth(1.0f).VAlign(VAlign_Center)
					[
						SNew(SVerticalBox)
						+ SVerticalBox::Slot().AutoHeight()
						[
							SNew(SHorizontalBox)
							+ SHorizontalBox::Slot().FillWidth(1.0f)
							[
								SNew(STextBlock)
								.Text(this, &SWildBoundLootRow::GetItemNameText)
								.Font(FCoreStyle::GetDefaultFontStyle("Bold", 9))
								.ColorAndOpacity(FLinearColor(0.90f,0.93f,0.88f,1.0f))
							]
							+ SHorizontalBox::Slot().AutoWidth().Padding(5.0f,0.0f,0.0f,0.0f)
							[
								SNew(SBorder)
								.Padding(FMargin(5.0f,1.0f))
								.BorderBackgroundColor(this, &SWildBoundLootRow::GetIconBackground)
								[
									SNew(STextBlock)
									.Text(this, &SWildBoundLootRow::GetRarityText)
									.Font(FCoreStyle::GetDefaultFontStyle("Bold", 6))
									.ColorAndOpacity(this, &SWildBoundLootRow::GetRarityColor)
								]
							]
						]
						+ SVerticalBox::Slot().AutoHeight().Padding(0.0f,2.0f,0.0f,0.0f)
						[
							SNew(STextBlock)
							.Text(this, &SWildBoundLootRow::GetMetadataText)
							.Font(FCoreStyle::GetDefaultFontStyle("Regular", 7))
							.ColorAndOpacity(FLinearColor(0.56f,0.62f,0.57f,1.0f))
						]
					]
					+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(7.0f,0.0f,0.0f,0.0f)
					[
						SNew(SButton)
						.Text(FText::FromString(TEXT("TAKE")))
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

		virtual FReply OnMouseButtonDoubleClick(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override
		{
			if (MouseEvent.GetEffectingButton() == EKeys::LeftMouseButton)
			{
				if (UWildBoundInteractionComponent* Interaction = InteractionComponent.Get())
				{
					Interaction->TakeLootEntry(EntryIndex, true);
					return FReply::Handled();
				}
			}
			return SBorder::OnMouseButtonDoubleClick(MyGeometry, MouseEvent);
		}

		virtual FReply OnDragDetected(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override
		{
			const UWildBoundInventoryComponent* Inventory = GetInventory();
			const FWildBoundContainerLootEntry* Entry = GetEntry();
			if (!Inventory || !Entry) return FReply::Unhandled();

			return FReply::Handled().BeginDragDrop(FWildBoundLootDragDropOp::New(
				EntryIndex,
				FString::Printf(TEXT("TAKE  [%s] %s x%d"),
					*Inventory->GetItemRarityName(Entry->ItemId),
					*Inventory->GetItemDisplayName(Entry->ItemId),
					Entry->Quantity)));
		}

	private:
		TWeakObjectPtr<UWildBoundInteractionComponent> InteractionComponent;
		int32 EntryIndex = INDEX_NONE;

		const UWildBoundInventoryComponent* GetInventory() const
		{
			const UWildBoundInteractionComponent* Interaction = InteractionComponent.Get();
			return Interaction ? Interaction->GetInventoryComponent() : nullptr;
		}

		const FWildBoundContainerLootEntry* GetEntry() const
		{
			const UWildBoundInteractionComponent* Interaction = InteractionComponent.Get();
			const TArray<FWildBoundContainerLootEntry>* Loot = Interaction ? Interaction->GetOpenContainerLoot() : nullptr;
			return Loot && Loot->IsValidIndex(EntryIndex) ? &(*Loot)[EntryIndex] : nullptr;
		}

		FSlateColor GetRarityColor() const
		{
			const UWildBoundInventoryComponent* Inventory = GetInventory();
			const FWildBoundContainerLootEntry* Entry = GetEntry();
			return Entry ? GetRaritySlateColor(Inventory, Entry->ItemId) : FSlateColor(FLinearColor::White);
		}

		FSlateColor GetIconBackground() const
		{
			const UWildBoundInventoryComponent* Inventory = GetInventory();
			const FWildBoundContainerLootEntry* Entry = GetEntry();
			return FSlateColor(Entry ? GetRarityBackground(Inventory, Entry->ItemId) : FLinearColor(0.08f,0.08f,0.08f,1.0f));
		}

		FText GetIconText() const
		{
			const UWildBoundInventoryComponent* Inventory = GetInventory();
			const FWildBoundContainerLootEntry* Entry = GetEntry();
			return Entry ? FText::FromString(GetItemIconCode(Entry->ItemId, Inventory)) : FText::GetEmpty();
		}

		FText GetItemNameText() const
		{
			const UWildBoundInventoryComponent* Inventory = GetInventory();
			const FWildBoundContainerLootEntry* Entry = GetEntry();
			return Inventory && Entry ? FText::FromString(Inventory->GetItemDisplayName(Entry->ItemId)) : FText::GetEmpty();
		}

		FText GetRarityText() const
		{
			const UWildBoundInventoryComponent* Inventory = GetInventory();
			const FWildBoundContainerLootEntry* Entry = GetEntry();
			return Inventory && Entry ? FText::FromString(Inventory->GetItemRarityName(Entry->ItemId)) : FText::GetEmpty();
		}

		FText GetMetadataText() const
		{
			const UWildBoundInventoryComponent* Inventory = GetInventory();
			const FWildBoundContainerLootEntry* Entry = GetEntry();
			if (!Inventory || !Entry) return FText::GetEmpty();
			const float Weight = Inventory->GetItemUnitWeight(Entry->ItemId) * static_cast<float>(Entry->Quantity);
			return FText::FromString(FString::Printf(
				TEXT("%s   |   x%d   |   %.2f kg"),
				*Inventory->GetItemCategoryName(Entry->ItemId),
				Entry->Quantity,
				Weight));
		}

		FText GetTooltipText() const
		{
			const UWildBoundInventoryComponent* Inventory = GetInventory();
			const FWildBoundContainerLootEntry* Entry = GetEntry();
			if (!Inventory || !Entry) return FText::GetEmpty();
			const float UnitWeight = Inventory->GetItemUnitWeight(Entry->ItemId);
			return FText::FromString(FString::Printf(
				TEXT("[%s] %s\n%s\n\n%s\n\nStack: %d   Unit: %.2f kg   Total: %.2f kg\nDouble-click: take stack   Right-click: take one   Drag: take stack"),
				*Inventory->GetItemRarityName(Entry->ItemId),
				*Inventory->GetItemDisplayName(Entry->ItemId),
				*Inventory->GetItemCategoryName(Entry->ItemId),
				*Inventory->GetItemDescription(Entry->ItemId),
				Entry->Quantity,
				UnitWeight,
				UnitWeight * static_cast<float>(Entry->Quantity)));
		}

		FReply HandleTakeStack()
		{
			if (UWildBoundInteractionComponent* Interaction = InteractionComponent.Get()) Interaction->TakeLootEntry(EntryIndex, true);
			return FReply::Handled();
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
				.Padding(FMargin(7.0f, 6.0f))
				.BorderBackgroundColor(FLinearColor(0.028f, 0.039f, 0.033f, 0.98f))
				.ToolTipText(this, &SWildBoundStashBackpackRow::GetTooltipText)
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0.0f,0.0f,8.0f,0.0f)
					[
						SNew(SBox).WidthOverride(42.0f).HeightOverride(42.0f)
						[
							SNew(SBorder)
							.Padding(FMargin(2.0f))
							.BorderBackgroundColor(this, &SWildBoundStashBackpackRow::GetRarityColor)
							[
								SNew(SBorder)
								.Padding(FMargin(3.0f))
								.BorderBackgroundColor(this, &SWildBoundStashBackpackRow::GetIconBackground)
								[
									SNew(STextBlock)
									.Text(this, &SWildBoundStashBackpackRow::GetIconText)
									.Font(FCoreStyle::GetDefaultFontStyle("Bold", 8))
									.ColorAndOpacity(this, &SWildBoundStashBackpackRow::GetRarityColor)
									.Justification(ETextJustify::Center)
								]
							]
						]
					]
					+ SHorizontalBox::Slot().FillWidth(1.0f).VAlign(VAlign_Center)
					[
						SNew(SVerticalBox)
						+ SVerticalBox::Slot().AutoHeight()
						[
							SNew(SHorizontalBox)
							+ SHorizontalBox::Slot().FillWidth(1.0f)
							[
								SNew(STextBlock)
								.Text(this, &SWildBoundStashBackpackRow::GetItemNameText)
								.Font(FCoreStyle::GetDefaultFontStyle("Bold", 9))
								.ColorAndOpacity(FLinearColor(0.90f,0.93f,0.88f,1.0f))
							]
							+ SHorizontalBox::Slot().AutoWidth().Padding(5.0f,0.0f,0.0f,0.0f)
							[
								SNew(SBorder)
								.Padding(FMargin(5.0f,1.0f))
								.BorderBackgroundColor(this, &SWildBoundStashBackpackRow::GetIconBackground)
								[
									SNew(STextBlock)
									.Text(this, &SWildBoundStashBackpackRow::GetRarityText)
									.Font(FCoreStyle::GetDefaultFontStyle("Bold", 6))
									.ColorAndOpacity(this, &SWildBoundStashBackpackRow::GetRarityColor)
								]
							]
						]
						+ SVerticalBox::Slot().AutoHeight().Padding(0.0f,2.0f,0.0f,0.0f)
						[
							SNew(STextBlock)
							.Text(this, &SWildBoundStashBackpackRow::GetMetadataText)
							.Font(FCoreStyle::GetDefaultFontStyle("Regular", 7))
							.ColorAndOpacity(FLinearColor(0.56f,0.62f,0.57f,1.0f))
						]
					]
					+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(7.0f,0.0f,0.0f,0.0f)
					[
						SNew(SButton)
						.Text(FText::FromString(TEXT("STORE")))
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

		virtual FReply OnMouseButtonDoubleClick(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override
		{
			if (MouseEvent.GetEffectingButton() == EKeys::LeftMouseButton)
			{
				if (UWildBoundInteractionComponent* Interaction = InteractionComponent.Get())
				{
					Interaction->StoreInventoryStackInOpenContainer(StackIndex, true);
					return FReply::Handled();
				}
			}
			return SBorder::OnMouseButtonDoubleClick(MyGeometry, MouseEvent);
		}

		virtual FReply OnDragDetected(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override
		{
			const UWildBoundInventoryComponent* Inventory = GetInventory();
			if (!Inventory || !Inventory->Stacks.IsValidIndex(StackIndex)) return FReply::Unhandled();

			const FWildBoundInventoryStack& Stack = Inventory->Stacks[StackIndex];
			return FReply::Handled().BeginDragDrop(FWildBoundBackpackStashDragDropOp::New(
				StackIndex,
				FString::Printf(TEXT("STORE  [%s] %s x%d"),
					*Inventory->GetItemRarityName(Stack.ItemId),
					*Inventory->GetItemDisplayName(Stack.ItemId),
					Stack.Quantity)));
		}

	private:
		TWeakObjectPtr<UWildBoundInteractionComponent> InteractionComponent;
		int32 StackIndex = INDEX_NONE;

		const UWildBoundInventoryComponent* GetInventory() const
		{
			const UWildBoundInteractionComponent* Interaction = InteractionComponent.Get();
			return Interaction ? Interaction->GetInventoryComponent() : nullptr;
		}

		const FWildBoundInventoryStack* GetStack() const
		{
			const UWildBoundInventoryComponent* Inventory = GetInventory();
			return Inventory && Inventory->Stacks.IsValidIndex(StackIndex) ? &Inventory->Stacks[StackIndex] : nullptr;
		}

		FSlateColor GetRarityColor() const
		{
			const UWildBoundInventoryComponent* Inventory = GetInventory();
			const FWildBoundInventoryStack* Stack = GetStack();
			return Stack ? GetRaritySlateColor(Inventory, Stack->ItemId) : FSlateColor(FLinearColor::White);
		}

		FSlateColor GetIconBackground() const
		{
			const UWildBoundInventoryComponent* Inventory = GetInventory();
			const FWildBoundInventoryStack* Stack = GetStack();
			return FSlateColor(Stack ? GetRarityBackground(Inventory, Stack->ItemId) : FLinearColor(0.08f,0.08f,0.08f,1.0f));
		}

		FText GetIconText() const
		{
			const UWildBoundInventoryComponent* Inventory = GetInventory();
			const FWildBoundInventoryStack* Stack = GetStack();
			return Stack ? FText::FromString(GetItemIconCode(Stack->ItemId, Inventory)) : FText::GetEmpty();
		}

		FText GetItemNameText() const
		{
			const UWildBoundInventoryComponent* Inventory = GetInventory();
			const FWildBoundInventoryStack* Stack = GetStack();
			return Inventory && Stack ? FText::FromString(Inventory->GetItemDisplayName(Stack->ItemId)) : FText::GetEmpty();
		}

		FText GetRarityText() const
		{
			const UWildBoundInventoryComponent* Inventory = GetInventory();
			const FWildBoundInventoryStack* Stack = GetStack();
			return Inventory && Stack ? FText::FromString(Inventory->GetItemRarityName(Stack->ItemId)) : FText::GetEmpty();
		}

		FText GetMetadataText() const
		{
			const UWildBoundInventoryComponent* Inventory = GetInventory();
			const FWildBoundInventoryStack* Stack = GetStack();
			if (!Inventory || !Stack) return FText::GetEmpty();

			int32 HotbarSlot = INDEX_NONE;
			const bool bInHotbar = Inventory->IsItemInHotbar(Stack->ItemId, HotbarSlot);
			const float Weight = Inventory->GetItemUnitWeight(Stack->ItemId) * static_cast<float>(Stack->Quantity);
			return FText::FromString(FString::Printf(
				TEXT("%s   |   x%d   |   %.2f kg%s"),
				*Inventory->GetItemCategoryName(Stack->ItemId),
				Stack->Quantity,
				Weight,
				bInHotbar ? *FString::Printf(TEXT("   |   H%d"), HotbarSlot + 1) : TEXT("")));
		}

		FText GetTooltipText() const
		{
			const UWildBoundInventoryComponent* Inventory = GetInventory();
			const FWildBoundInventoryStack* Stack = GetStack();
			if (!Inventory || !Stack) return FText::GetEmpty();
			const float UnitWeight = Inventory->GetItemUnitWeight(Stack->ItemId);
			return FText::FromString(FString::Printf(
				TEXT("[%s] %s\n%s\n\n%s\n\nStack: %d   Unit: %.2f kg   Total: %.2f kg\nDouble-click: store stack   Right-click: store one   Drag: store stack"),
				*Inventory->GetItemRarityName(Stack->ItemId),
				*Inventory->GetItemDisplayName(Stack->ItemId),
				*Inventory->GetItemCategoryName(Stack->ItemId),
				*Inventory->GetItemDescription(Stack->ItemId),
				Stack->Quantity,
				UnitWeight,
				UnitWeight * static_cast<float>(Stack->Quantity)));
		}

		FReply HandleStoreStack()
		{
			if (UWildBoundInteractionComponent* Interaction = InteractionComponent.Get()) Interaction->StoreInventoryStackInOpenContainer(StackIndex, true);
			return FReply::Handled();
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
				.Padding(FMargin(8.0f,5.0f))
				.BorderBackgroundColor(FLinearColor(0.030f,0.065f,0.040f,0.98f))
				[
					SNew(STextBlock)
					.Text(FText::FromString(TEXT("DROP HERE TO TAKE STACK")))
					.Font(FCoreStyle::GetDefaultFontStyle("Bold", 7))
					.ColorAndOpacity(FLinearColor(0.62f,0.83f,0.62f,1.0f))
				]);
		}

		virtual FReply OnDrop(const FGeometry& MyGeometry, const FDragDropEvent& DragDropEvent) override
		{
			const TSharedPtr<FWildBoundLootDragDropOp> Operation = DragDropEvent.GetOperationAs<FWildBoundLootDragDropOp>();
			UWildBoundInteractionComponent* Interaction = InteractionComponent.Get();
			return Operation.IsValid() && Interaction && Interaction->TakeLootEntry(Operation->EntryIndex, true)
				? FReply::Handled()
				: FReply::Unhandled();
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
				.Padding(FMargin(8.0f,5.0f))
				.BorderBackgroundColor(FLinearColor(0.070f,0.052f,0.025f,0.98f))
				[
					SNew(STextBlock)
					.Text(FText::FromString(TEXT("DROP HERE TO STORE STACK")))
					.Font(FCoreStyle::GetDefaultFontStyle("Bold", 7))
					.ColorAndOpacity(FLinearColor(0.86f,0.72f,0.45f,1.0f))
				]);
		}

		virtual FReply OnDrop(const FGeometry& MyGeometry, const FDragDropEvent& DragDropEvent) override
		{
			const TSharedPtr<FWildBoundBackpackStashDragDropOp> Operation = DragDropEvent.GetOperationAs<FWildBoundBackpackStashDragDropOp>();
			UWildBoundInteractionComponent* Interaction = InteractionComponent.Get();
			return Operation.IsValid() && Interaction
				&& Interaction->StoreInventoryStackInOpenContainer(Operation->StackIndex, true)
				? FReply::Handled()
				: FReply::Unhandled();
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
		.Padding(FMargin(18.0f, 16.0f))
		.BorderBackgroundColor(FLinearColor(0.009f,0.013f,0.012f,0.994f))
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
						.Text(this, &SWildBoundLootWidget::GetHeaderText)
						.Font(FCoreStyle::GetDefaultFontStyle("Bold", 17))
						.ColorAndOpacity(FLinearColor(0.93f,0.95f,0.91f,1.0f))
					]
					+ SVerticalBox::Slot().AutoHeight().Padding(0.0f,3.0f,0.0f,0.0f)
					[
						SNew(STextBlock)
						.Text(FText::FromString(TEXT("CONTAINER TRANSFER / FIELD STORAGE")))
						.Font(FCoreStyle::GetDefaultFontStyle("Bold", 7))
						.ColorAndOpacity(FLinearColor(0.50f,0.60f,0.51f,1.0f))
					]
				]
				+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(5.0f,0.0f)
				[
					SNew(SButton)
					.Text(this, &SWildBoundLootWidget::GetSortButtonText)
					.OnClicked(this, &SWildBoundLootWidget::HandleCycleSort)
				]
				+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(5.0f,0.0f)
				[
					SNew(SButton)
					.Text(this, &SWildBoundLootWidget::GetFilterButtonText)
					.OnClicked(this, &SWildBoundLootWidget::HandleCycleFilter)
				]
				+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(5.0f,0.0f,0.0f,0.0f)
				[
					SNew(SButton)
					.Text(FText::FromString(TEXT("CLOSE")))
					.OnClicked(this, &SWildBoundLootWidget::HandleClose)
				]
			]
			+ SVerticalBox::Slot().AutoHeight().Padding(0.0f,7.0f,0.0f,0.0f)
			[
				SNew(STextBlock)
				.Text(this, &SWildBoundLootWidget::GetCarryText)
				.Font(FCoreStyle::GetDefaultFontStyle("Regular", 8))
				.ColorAndOpacity(FLinearColor(0.61f,0.66f,0.61f,1.0f))
			]
			+ SVerticalBox::Slot().AutoHeight().Padding(0.0f,8.0f)
			[
				SNew(SSeparator)
			]
			+ SVerticalBox::Slot().AutoHeight()
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().FillWidth(0.50f).Padding(0.0f,0.0f,7.0f,0.0f)
				[
					SNew(SBorder)
					.Padding(FMargin(10.0f,9.0f))
					.BorderBackgroundColor(FLinearColor(0.023f,0.030f,0.027f,0.98f))
					[
						SNew(SVerticalBox)
						+ SVerticalBox::Slot().AutoHeight()
						[
							SNew(SHorizontalBox)
							+ SHorizontalBox::Slot().FillWidth(1.0f)
							[
								SNew(STextBlock)
								.Text(FText::FromString(TEXT("CONTAINER")))
								.Font(FCoreStyle::GetDefaultFontStyle("Bold", 10))
							]
							+ SHorizontalBox::Slot().AutoWidth()
							[
								SNew(STextBlock)
								.Text(FText::FromString(TEXT("DBL-CLICK TAKE")))
								.Font(FCoreStyle::GetDefaultFontStyle("Regular", 6))
								.ColorAndOpacity(FLinearColor(0.47f,0.52f,0.48f,1.0f))
							]
						]
						+ SVerticalBox::Slot().AutoHeight().Padding(0.0f,5.0f)
						[
							SNew(SWildBoundContainerStashDropTarget).InteractionComponent(InteractionComponent)
						]
						+ SVerticalBox::Slot().AutoHeight()
						[
							SNew(SBox).HeightOverride(390.0f)
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
				+ SHorizontalBox::Slot().FillWidth(0.50f).Padding(7.0f,0.0f,0.0f,0.0f)
				[
					SNew(SBorder)
					.Padding(FMargin(10.0f,9.0f))
					.BorderBackgroundColor(FLinearColor(0.021f,0.034f,0.027f,0.98f))
					[
						SNew(SVerticalBox)
						+ SVerticalBox::Slot().AutoHeight()
						[
							SNew(SHorizontalBox)
							+ SHorizontalBox::Slot().FillWidth(1.0f)
							[
								SNew(STextBlock)
								.Text(FText::FromString(TEXT("BACKPACK")))
								.Font(FCoreStyle::GetDefaultFontStyle("Bold", 10))
							]
							+ SHorizontalBox::Slot().AutoWidth()
							[
								SNew(STextBlock)
								.Text(FText::FromString(TEXT("DBL-CLICK STORE")))
								.Font(FCoreStyle::GetDefaultFontStyle("Regular", 6))
								.ColorAndOpacity(FLinearColor(0.47f,0.52f,0.48f,1.0f))
							]
						]
						+ SVerticalBox::Slot().AutoHeight().Padding(0.0f,5.0f)
						[
							SNew(SWildBoundBackpackLootDropTarget).InteractionComponent(InteractionComponent)
						]
						+ SVerticalBox::Slot().AutoHeight()
						[
							SNew(SBox).HeightOverride(390.0f)
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
			+ SVerticalBox::Slot().AutoHeight().Padding(0.0f,10.0f,0.0f,0.0f)
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().FillWidth(1.0f).VAlign(VAlign_Center)
				[
					SNew(STextBlock)
					.Text(FText::FromString(TEXT("Right-click moves 1   |   Drag or double-click moves stack   |   Stored items remain here")))
					.Font(FCoreStyle::GetDefaultFontStyle("Regular", 7))
					.ColorAndOpacity(FLinearColor(0.52f,0.57f,0.53f,1.0f))
				]
				+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(8.0f,0.0f,0.0f,0.0f)
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

void SWildBoundLootWidget::Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime)
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
	if (!LootRowsBox.IsValid()) return;
	LootRowsBox->ClearChildren();

	const UWildBoundInteractionComponent* Interaction = InteractionComponent.Get();
	const UWildBoundInventoryComponent* Inventory = Interaction ? Interaction->GetInventoryComponent() : nullptr;
	const TArray<FWildBoundContainerLootEntry>* Loot = Interaction ? Interaction->GetOpenContainerLoot() : nullptr;
	if (!Inventory || !Loot)
	{
		return;
	}

	TArray<int32> Indices;
	for (int32 Index = 0; Index < Loot->Num(); ++Index)
	{
		if (PassesFilter((*Loot)[Index].ItemId)) Indices.Add(Index);
	}

	Indices.Sort([this, Inventory, Loot](int32 A, int32 B)
	{
		const FWildBoundContainerLootEntry& Left = (*Loot)[A];
		const FWildBoundContainerLootEntry& Right = (*Loot)[B];
		if (SortMode == 1)
		{
			return Inventory->GetItemDisplayName(Left.ItemId) < Inventory->GetItemDisplayName(Right.ItemId);
		}
		if (SortMode == 2)
		{
			const float LeftWeight = Inventory->GetItemUnitWeight(Left.ItemId) * static_cast<float>(Left.Quantity);
			const float RightWeight = Inventory->GetItemUnitWeight(Right.ItemId) * static_cast<float>(Right.Quantity);
			return LeftWeight > RightWeight;
		}
		const int32 LeftRarity = Inventory->GetItemRarityTier(Left.ItemId);
		const int32 RightRarity = Inventory->GetItemRarityTier(Right.ItemId);
		return LeftRarity == RightRarity
			? Inventory->GetItemDisplayName(Left.ItemId) < Inventory->GetItemDisplayName(Right.ItemId)
			: LeftRarity > RightRarity;
	});

	if (Indices.IsEmpty())
	{
		LootRowsBox->AddSlot().AutoHeight().Padding(0.0f,4.0f)
		[
			SNew(STextBlock)
			.Text(FText::FromString(Loot->IsEmpty() ? TEXT("-- EMPTY --") : TEXT("-- NO ITEMS MATCH FILTER --")))
			.Font(FCoreStyle::GetDefaultFontStyle("Bold", 9))
			.ColorAndOpacity(FLinearColor(0.48f,0.52f,0.49f,1.0f))
		];
		return;
	}

	for (const int32 Index : Indices)
	{
		LootRowsBox->AddSlot().AutoHeight().Padding(0.0f,2.0f)
		[
			SNew(SWildBoundLootRow).InteractionComponent(InteractionComponent).EntryIndex(Index)
		];
	}
}

void SWildBoundLootWidget::RebuildBackpackRows()
{
	if (!BackpackRowsBox.IsValid()) return;
	BackpackRowsBox->ClearChildren();

	const UWildBoundInteractionComponent* Interaction = InteractionComponent.Get();
	const UWildBoundInventoryComponent* Inventory = Interaction ? Interaction->GetInventoryComponent() : nullptr;
	if (!Inventory) return;

	TArray<int32> Indices;
	for (int32 Index = 0; Index < Inventory->Stacks.Num(); ++Index)
	{
		if (PassesFilter(Inventory->Stacks[Index].ItemId)) Indices.Add(Index);
	}

	Indices.Sort([this, Inventory](int32 A, int32 B)
	{
		const FWildBoundInventoryStack& Left = Inventory->Stacks[A];
		const FWildBoundInventoryStack& Right = Inventory->Stacks[B];
		if (SortMode == 1)
		{
			return Inventory->GetItemDisplayName(Left.ItemId) < Inventory->GetItemDisplayName(Right.ItemId);
		}
		if (SortMode == 2)
		{
			const float LeftWeight = Inventory->GetItemUnitWeight(Left.ItemId) * static_cast<float>(Left.Quantity);
			const float RightWeight = Inventory->GetItemUnitWeight(Right.ItemId) * static_cast<float>(Right.Quantity);
			return LeftWeight > RightWeight;
		}
		const int32 LeftRarity = Inventory->GetItemRarityTier(Left.ItemId);
		const int32 RightRarity = Inventory->GetItemRarityTier(Right.ItemId);
		return LeftRarity == RightRarity
			? Inventory->GetItemDisplayName(Left.ItemId) < Inventory->GetItemDisplayName(Right.ItemId)
			: LeftRarity > RightRarity;
	});

	if (Indices.IsEmpty())
	{
		BackpackRowsBox->AddSlot().AutoHeight().Padding(0.0f,4.0f)
		[
			SNew(STextBlock)
			.Text(FText::FromString(Inventory->Stacks.IsEmpty() ? TEXT("-- BACKPACK EMPTY --") : TEXT("-- NO ITEMS MATCH FILTER --")))
			.Font(FCoreStyle::GetDefaultFontStyle("Bold", 9))
			.ColorAndOpacity(FLinearColor(0.48f,0.52f,0.49f,1.0f))
		];
		return;
	}

	for (const int32 Index : Indices)
	{
		BackpackRowsBox->AddSlot().AutoHeight().Padding(0.0f,2.0f)
		[
			SNew(SWildBoundStashBackpackRow).InteractionComponent(InteractionComponent).StackIndex(Index)
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

bool SWildBoundLootWidget::PassesFilter(FName ItemId) const
{
	if (FilterMode == 0) return true;
	const UWildBoundInteractionComponent* Interaction = InteractionComponent.Get();
	const UWildBoundInventoryComponent* Inventory = Interaction ? Interaction->GetInventoryComponent() : nullptr;
	if (!Inventory) return true;

	const FString Category = Inventory->GetItemCategoryName(ItemId);
	switch (FilterMode)
	{
	case 1: return Category == TEXT("CONSUMABLE");
	case 2: return Category == TEXT("TOOL");
	case 3: return Category == TEXT("PASSIVE GEAR");
	case 4: return Category == TEXT("CRAFTING MATERIAL");
	default: return true;
	}
}

FText SWildBoundLootWidget::GetHeaderText() const
{
	const UWildBoundInteractionComponent* Interaction = InteractionComponent.Get();
	return Interaction
		? FText::FromString(FString::Printf(TEXT("[%s] %s"),
			*Interaction->GetOpenContainerQualityName(),
			*Interaction->GetOpenContainerName()))
		: FText::FromString(TEXT("CONTAINER STORAGE"));
}

FText SWildBoundLootWidget::GetCarryText() const
{
	const UWildBoundInteractionComponent* Interaction = InteractionComponent.Get();
	const UWildBoundInventoryComponent* Inventory = Interaction ? Interaction->GetInventoryComponent() : nullptr;
	const TArray<FWildBoundContainerLootEntry>* Loot = Interaction ? Interaction->GetOpenContainerLoot() : nullptr;
	if (!Inventory) return FText::GetEmpty();

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
	const bool bOver = ProjectedAllWeight > Inventory->MaxCarryWeight + KINDA_SMALL_NUMBER;
	return FText::FromString(FString::Printf(
		bOver
			? TEXT("BACKPACK %.2f / %.2f kg   |   SLOTS %d / %d   |   CONTAINER %.2f kg   |   TAKE ALL %.2f kg  [OVER ENCUMBERED]")
			: TEXT("BACKPACK %.2f / %.2f kg   |   SLOTS %d / %d   |   CONTAINER %.2f kg   |   TAKE ALL %.2f kg"),
		CurrentWeight,
		Inventory->MaxCarryWeight,
		Inventory->Stacks.Num(),
		Inventory->MaxSlots,
		ContainerWeight,
		ProjectedAllWeight));
}

FText SWildBoundLootWidget::GetSortButtonText() const
{
	switch (SortMode)
	{
	case 1: return FText::FromString(TEXT("SORT: NAME"));
	case 2: return FText::FromString(TEXT("SORT: WEIGHT"));
	default: return FText::FromString(TEXT("SORT: RARITY"));
	}
}

FText SWildBoundLootWidget::GetFilterButtonText() const
{
	switch (FilterMode)
	{
	case 1: return FText::FromString(TEXT("FILTER: CONSUMABLE"));
	case 2: return FText::FromString(TEXT("FILTER: TOOL"));
	case 3: return FText::FromString(TEXT("FILTER: GEAR"));
	case 4: return FText::FromString(TEXT("FILTER: MATERIAL"));
	default: return FText::FromString(TEXT("FILTER: ALL"));
	}
}

FReply SWildBoundLootWidget::HandleCycleSort()
{
	SortMode = (SortMode + 1) % 3;
	RebuildRows();
	return FReply::Handled();
}

FReply SWildBoundLootWidget::HandleCycleFilter()
{
	FilterMode = (FilterMode + 1) % 5;
	RebuildRows();
	return FReply::Handled();
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