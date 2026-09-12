#include "WildBoundBackpackComponent.h"

#include "../Inventory/WildBoundInventoryComponent.h"
#include "../UI/SWildBoundBackpackWidget.h"
#include "WildBoundInteractionComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/Engine.h"
#include "Engine/GameViewportClient.h"
#include "Engine/StaticMesh.h"
#include "Engine/StaticMeshActor.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "InputCoreTypes.h"
#include "Styling/CoreStyle.h"
#include "UObject/UObjectGlobals.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/SOverlay.h"
#include "Widgets/Text/STextBlock.h"

namespace
{
	const FName InteractableTag(TEXT("WBInteractable"));
	const FName DroppedItemTag(TEXT("WBTypeDroppedItem"));
	const FName DroppedWorldTag(TEXT("WildBoundDroppedItem"));
	const FString DroppedItemPrefix(TEXT("WBDropItem_"));
	const FString DroppedQuantityPrefix(TEXT("WBDropQty_"));

	UStaticMesh* GetDropMesh()
	{
		static TWeakObjectPtr<UStaticMesh> Mesh;
		if (!Mesh.IsValid())
		{
			Mesh = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube"));
		}
		return Mesh.Get();
	}
}

UWildBoundBackpackComponent::UWildBoundBackpackComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = true;
	PrimaryComponentTick.TickGroup = TG_PostUpdateWork;
}

void UWildBoundBackpackComponent::BeginPlay()
{
	Super::BeginPlay();
	InventoryComponent = GetOwner()
		? GetOwner()->FindComponentByClass<UWildBoundInventoryComponent>()
		: nullptr;
	EnsureBackpackWidget();
	EnsureEncumbranceWarning();
}

void UWildBoundBackpackComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	SetBackpackOpen(false);
	RemoveBackpackWidget();
	Super::EndPlay(EndPlayReason);
}

void UWildBoundBackpackComponent::TickComponent(
	float DeltaTime,
	ELevelTick TickType,
	FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (!InventoryComponent.IsValid() && GetOwner())
	{
		InventoryComponent = GetOwner()->FindComponentByClass<UWildBoundInventoryComponent>();
	}

	ClampSelection();
	EnsureBackpackWidget();
	EnsureEncumbranceWarning();

	APawn* Pawn = Cast<APawn>(GetOwner());
	APlayerController* PlayerController = Pawn ? Cast<APlayerController>(Pawn->GetController()) : nullptr;
	if (!PlayerController)
	{
		return;
	}

	UWildBoundInteractionComponent* Interaction = GetOwner()
		? GetOwner()->FindComponentByClass<UWildBoundInteractionComponent>()
		: nullptr;

	if (PlayerController->WasInputKeyJustPressed(EKeys::Tab))
	{
		if (Interaction && Interaction->IsLootWindowOpen())
		{
			Interaction->CloseLootWindow();
			return;
		}

		ToggleBackpack();
		return;
	}

	if (PlayerController->WasInputKeyJustPressed(EKeys::I))
	{
		if (Interaction && Interaction->IsLootWindowOpen())
		{
			return;
		}

		ToggleBackpack();
		return;
	}

	if (bBackpackOpen)
	{
		HandleBackpackInput(*PlayerController);
	}
}

FName UWildBoundBackpackComponent::GetSelectedItemId() const
{
	const UWildBoundInventoryComponent* Inventory = InventoryComponent.Get();
	return Inventory && Inventory->Stacks.IsValidIndex(SelectedStackIndex)
		? Inventory->Stacks[SelectedStackIndex].ItemId
		: NAME_None;
}

int32 UWildBoundBackpackComponent::GetSelectedItemQuantity() const
{
	const UWildBoundInventoryComponent* Inventory = InventoryComponent.Get();
	return Inventory && Inventory->Stacks.IsValidIndex(SelectedStackIndex)
		? Inventory->Stacks[SelectedStackIndex].Quantity
		: 0;
}

void UWildBoundBackpackComponent::SelectStackIndex(int32 StackIndex)
{
	const UWildBoundInventoryComponent* Inventory = InventoryComponent.Get();
	if (!Inventory || !Inventory->Stacks.IsValidIndex(StackIndex))
	{
		return;
	}
	SelectedStackIndex = StackIndex;
}

bool UWildBoundBackpackComponent::ReorderStackFromMouse(int32 SourceIndex, int32 TargetIndex)
{
	UWildBoundInventoryComponent* Inventory = InventoryComponent.Get();
	if (!Inventory || !Inventory->MoveStack(SourceIndex, TargetIndex))
	{
		return false;
	}

	SelectedStackIndex = FMath::Clamp(TargetIndex, 0, FMath::Max(Inventory->Stacks.Num() - 1, 0));
	return true;
}

bool UWildBoundBackpackComponent::SplitStackFromMouse(int32 StackIndex)
{
	UWildBoundInventoryComponent* Inventory = InventoryComponent.Get();
	if (!Inventory || !Inventory->Stacks.IsValidIndex(StackIndex))
	{
		return false;
	}

	const int32 Quantity = Inventory->Stacks[StackIndex].Quantity;
	if (Quantity < 2)
	{
		return false;
	}

	const int32 SplitQuantity = Quantity / 2;
	if (!Inventory->SplitStack(StackIndex, SplitQuantity))
	{
		if (GEngine && Inventory->Stacks.Num() >= Inventory->MaxSlots)
		{
			GEngine->AddOnScreenDebugMessage(91032, 1.6f, FColor(220, 160, 115), TEXT("No free inventory slot to split this stack."));
		}
		return false;
	}

	SelectedStackIndex = FMath::Clamp(StackIndex + 1, 0, Inventory->Stacks.Num() - 1);
	return true;
}

bool UWildBoundBackpackComponent::AssignItemToHotbarFromMouse(FName ItemId, int32 SlotIndex)
{
	UWildBoundInventoryComponent* Inventory = InventoryComponent.Get();
	if (!Inventory || ItemId.IsNone())
	{
		return false;
	}

	return Inventory->AssignHotbarSlot(SlotIndex, ItemId);
}

void UWildBoundBackpackComponent::ClearHotbarSlotFromMouse(int32 SlotIndex)
{
	if (UWildBoundInventoryComponent* Inventory = InventoryComponent.Get())
	{
		Inventory->ClearHotbarSlot(SlotIndex);
	}
}

bool UWildBoundBackpackComponent::DropStackFromMouse(int32 StackIndex, bool bDropWholeStack)
{
	UWildBoundInventoryComponent* Inventory = InventoryComponent.Get();
	if (!Inventory || !Inventory->Stacks.IsValidIndex(StackIndex))
	{
		return false;
	}

	SelectedStackIndex = StackIndex;
	const FName ItemId = GetSelectedItemId();
	const int32 CurrentQuantity = GetSelectedItemQuantity();
	if (ItemId.IsNone() || CurrentQuantity <= 0)
	{
		return false;
	}

	const int32 DropQuantity = bDropWholeStack ? CurrentQuantity : 1;
	AActor* DroppedActor = SpawnDroppedItem(ItemId, DropQuantity);
	if (!DroppedActor)
	{
		return false;
	}

	if (!Inventory->RemoveFromStack(StackIndex, DropQuantity))
	{
		DroppedActor->Destroy();
		return false;
	}

	ClampSelection();
	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(
			91031,
			1.6f,
			FColor(185, 185, 170),
			FString::Printf(TEXT("Dropped %s x%d"), *Inventory->GetItemDisplayName(ItemId), DropQuantity));
	}
	return true;
}

void UWildBoundBackpackComponent::EnsureBackpackWidget()
{
	if (BackpackViewportRoot.IsValid())
	{
		if (BackpackWidget.IsValid())
		{
			BackpackWidget->SetBackpackComponent(this);
		}
		return;
	}

	if (!InventoryComponent.IsValid() || !GEngine || !GEngine->GameViewport)
	{
		return;
	}

	TSharedPtr<SOverlay> Overlay;
	SAssignNew(Overlay, SOverlay)
	+ SOverlay::Slot()
	.HAlign(HAlign_Fill)
	.VAlign(VAlign_Fill)
	[
		SAssignNew(BackpackWidget, SWildBoundBackpackWidget)
		.BackpackComponent(this)
	];

	BackpackViewportRoot = Overlay;
	BackpackViewportRoot->SetVisibility(EVisibility::Collapsed);
	GEngine->GameViewport->AddViewportWidgetContent(BackpackViewportRoot.ToSharedRef(), 130);
}

void UWildBoundBackpackComponent::EnsureEncumbranceWarning()
{
	if (EncumbranceViewportRoot.IsValid())
	{
		return;
	}

	if (!InventoryComponent.IsValid() || !GEngine || !GEngine->GameViewport)
	{
		return;
	}

	const TWeakObjectPtr<UWildBoundInventoryComponent> WeakInventory = InventoryComponent;

	TSharedPtr<SOverlay> Overlay;
	SAssignNew(Overlay, SOverlay)
	+ SOverlay::Slot()
	.HAlign(HAlign_Right)
	.VAlign(VAlign_Bottom)
	.Padding(FMargin(0.0f, 0.0f, 28.0f, 150.0f))
	[
		SNew(SBorder)
		.Padding(FMargin(10.0f, 6.0f))
		.BorderBackgroundColor(FLinearColor(0.22f, 0.035f, 0.02f, 0.92f))
		.Visibility_Lambda([WeakInventory]()
		{
			const UWildBoundInventoryComponent* Inventory = WeakInventory.Get();
			return Inventory && Inventory->IsOverEncumbered()
				? EVisibility::HitTestInvisible
				: EVisibility::Collapsed;
		})
		[
			SNew(STextBlock)
			.Text_Lambda([WeakInventory]()
			{
				const UWildBoundInventoryComponent* Inventory = WeakInventory.Get();
				if (!Inventory)
				{
					return FText::GetEmpty();
				}
				return FText::FromString(FString::Printf(
					TEXT("OVER ENCUMBERED   %.1f / %.1f kg"),
					Inventory->GetTotalWeight(),
					Inventory->MaxCarryWeight));
			})
			.Font(FCoreStyle::GetDefaultFontStyle("Bold", 10))
			.ColorAndOpacity(FLinearColor(1.0f, 0.72f, 0.62f, 1.0f))
		]
	];

	EncumbranceViewportRoot = Overlay;
	GEngine->GameViewport->AddViewportWidgetContent(EncumbranceViewportRoot.ToSharedRef(), 125);
}

void UWildBoundBackpackComponent::ToggleBackpack()
{
	SetBackpackOpen(!bBackpackOpen);
}

void UWildBoundBackpackComponent::SetBackpackOpen(bool bOpen)
{
	EnsureBackpackWidget();
	bBackpackOpen = bOpen && BackpackViewportRoot.IsValid();
	ClampSelection();

	if (BackpackViewportRoot.IsValid())
	{
		BackpackViewportRoot->SetVisibility(
			bBackpackOpen ? EVisibility::Visible : EVisibility::Collapsed);
	}

	APawn* Pawn = Cast<APawn>(GetOwner());
	APlayerController* PlayerController = Pawn ? Cast<APlayerController>(Pawn->GetController()) : nullptr;
	if (PlayerController)
	{
		PlayerController->SetIgnoreMoveInput(bBackpackOpen);
		PlayerController->SetIgnoreLookInput(bBackpackOpen);
		PlayerController->bShowMouseCursor = bBackpackOpen;

		if (bBackpackOpen)
		{
			FInputModeGameAndUI InputMode;
			InputMode.SetHideCursorDuringCapture(false);
			InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
			PlayerController->SetInputMode(InputMode);
		}
		else
		{
			FInputModeGameOnly InputMode;
			PlayerController->SetInputMode(InputMode);
		}
	}
}

void UWildBoundBackpackComponent::HandleBackpackInput(APlayerController& PlayerController)
{
	if (PlayerController.WasInputKeyJustPressed(EKeys::Escape))
	{
		SetBackpackOpen(false);
		return;
	}

	if (PlayerController.WasInputKeyJustPressed(EKeys::Up))
	{
		MoveSelection(-1);
		return;
	}
	if (PlayerController.WasInputKeyJustPressed(EKeys::Down))
	{
		MoveSelection(1);
		return;
	}

	if (PlayerController.WasInputKeyJustPressed(EKeys::One))
	{
		AssignSelectedToHotbar(0);
		return;
	}
	if (PlayerController.WasInputKeyJustPressed(EKeys::Two))
	{
		AssignSelectedToHotbar(1);
		return;
	}
	if (PlayerController.WasInputKeyJustPressed(EKeys::Three))
	{
		AssignSelectedToHotbar(2);
		return;
	}

	if (PlayerController.WasInputKeyJustPressed(EKeys::R))
	{
		RemoveSelectedFromHotbar();
		return;
	}

	if (PlayerController.WasInputKeyJustPressed(EKeys::D))
	{
		const bool bDropWholeStack = PlayerController.IsInputKeyDown(EKeys::LeftShift)
			|| PlayerController.IsInputKeyDown(EKeys::RightShift);
		DropSelectedItem(bDropWholeStack);
	}
}

void UWildBoundBackpackComponent::MoveSelection(int32 Direction)
{
	const UWildBoundInventoryComponent* Inventory = InventoryComponent.Get();
	if (!Inventory || Inventory->Stacks.IsEmpty() || Direction == 0)
	{
		SelectedStackIndex = 0;
		return;
	}

	SelectedStackIndex = (SelectedStackIndex + Direction) % Inventory->Stacks.Num();
	if (SelectedStackIndex < 0)
	{
		SelectedStackIndex += Inventory->Stacks.Num();
	}
}

void UWildBoundBackpackComponent::ClampSelection()
{
	const UWildBoundInventoryComponent* Inventory = InventoryComponent.Get();
	if (!Inventory || Inventory->Stacks.IsEmpty())
	{
		SelectedStackIndex = 0;
		return;
	}
	SelectedStackIndex = FMath::Clamp(SelectedStackIndex, 0, Inventory->Stacks.Num() - 1);
}

void UWildBoundBackpackComponent::AssignSelectedToHotbar(int32 SlotIndex)
{
	UWildBoundInventoryComponent* Inventory = InventoryComponent.Get();
	const FName ItemId = GetSelectedItemId();
	if (!Inventory || ItemId.IsNone())
	{
		return;
	}

	if (Inventory->GetHotbarItemId(SlotIndex) == ItemId)
	{
		Inventory->ClearHotbarSlot(SlotIndex);
		return;
	}

	Inventory->AssignHotbarSlot(SlotIndex, ItemId);
}

void UWildBoundBackpackComponent::RemoveSelectedFromHotbar()
{
	UWildBoundInventoryComponent* Inventory = InventoryComponent.Get();
	const FName ItemId = GetSelectedItemId();
	if (Inventory && !ItemId.IsNone())
	{
		Inventory->ClearItemFromHotbar(ItemId);
	}
}

void UWildBoundBackpackComponent::DropSelectedItem(bool bDropWholeStack)
{
	DropStackFromMouse(SelectedStackIndex, bDropWholeStack);
}

AActor* UWildBoundBackpackComponent::SpawnDroppedItem(FName ItemId, int32 Quantity) const
{
	UWorld* World = GetWorld();
	AActor* Owner = GetOwner();
	UStaticMesh* MeshAsset = GetDropMesh();
	if (!World || !Owner || !MeshAsset || ItemId.IsNone() || Quantity <= 0)
	{
		return nullptr;
	}

	const FVector Location = Owner->GetActorLocation()
		+ Owner->GetActorForwardVector() * 145.0f
		+ FVector(0.0f, 0.0f, -68.0f);

	AStaticMeshActor* Dropped = World->SpawnActor<AStaticMeshActor>(Location, FRotator::ZeroRotator);
	if (!Dropped)
	{
		return nullptr;
	}

	Dropped->Tags.AddUnique(InteractableTag);
	Dropped->Tags.AddUnique(DroppedItemTag);
	Dropped->Tags.AddUnique(DroppedWorldTag);
	Dropped->Tags.AddUnique(FName(*(DroppedItemPrefix + ItemId.ToString())));
	Dropped->Tags.AddUnique(FName(*(DroppedQuantityPrefix + FString::FromInt(Quantity))));

#if WITH_EDITOR
	Dropped->SetActorLabel(FString::Printf(TEXT("WB_Dropped_%s_x%d"), *ItemId.ToString(), Quantity));
#endif

	UStaticMeshComponent* Mesh = Dropped->GetStaticMeshComponent();
	Mesh->SetMobility(EComponentMobility::Movable);
	Mesh->SetStaticMesh(MeshAsset);
	Mesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	Mesh->SetCollisionResponseToAllChannels(ECR_Block);
	Mesh->SetCastShadow(true);
	Dropped->SetActorScale3D(FVector(0.34f, 0.34f, 0.24f));
	return Dropped;
}

void UWildBoundBackpackComponent::RemoveBackpackWidget()
{
	if (GEngine && GEngine->GameViewport)
	{
		if (BackpackViewportRoot.IsValid())
		{
			GEngine->GameViewport->RemoveViewportWidgetContent(BackpackViewportRoot.ToSharedRef());
		}
		if (EncumbranceViewportRoot.IsValid())
		{
			GEngine->GameViewport->RemoveViewportWidgetContent(EncumbranceViewportRoot.ToSharedRef());
		}
	}

	BackpackWidget.Reset();
	BackpackViewportRoot.Reset();
	EncumbranceViewportRoot.Reset();
	bBackpackOpen = false;
}
