#include "WildBoundBackpackComponent.h"

#include "../Inventory/WildBoundInventoryComponent.h"
#include "../UI/SWildBoundBackpackWidget.h"
#include "Engine/Engine.h"
#include "Engine/GameViewportClient.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "InputCoreTypes.h"
#include "Styling/CoreStyle.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/SOverlay.h"
#include "Widgets/Text/STextBlock.h"

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

	EnsureBackpackWidget();
	EnsureEncumbranceWarning();

	APawn* Pawn = Cast<APawn>(GetOwner());
	APlayerController* PlayerController = Pawn ? Cast<APlayerController>(Pawn->GetController()) : nullptr;
	if (!PlayerController)
	{
		return;
	}

	if (PlayerController->WasInputKeyJustPressed(EKeys::I)
		|| PlayerController->WasInputKeyJustPressed(EKeys::Tab))
	{
		ToggleBackpack();
	}
}

void UWildBoundBackpackComponent::EnsureBackpackWidget()
{
	if (BackpackViewportRoot.IsValid())
	{
		if (BackpackWidget.IsValid())
		{
			BackpackWidget->SetInventoryComponent(InventoryComponent.Get());
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
	.HAlign(HAlign_Center)
	.VAlign(VAlign_Center)
	.Padding(FMargin(24.0f))
	[
		SAssignNew(BackpackWidget, SWildBoundBackpackWidget)
		.InventoryComponent(InventoryComponent)
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
	EnsureBackpackWidget();
	if (!BackpackViewportRoot.IsValid())
	{
		return;
	}

	bBackpackOpen = !bBackpackOpen;
	BackpackViewportRoot->SetVisibility(
		bBackpackOpen ? EVisibility::HitTestInvisible : EVisibility::Collapsed);
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
