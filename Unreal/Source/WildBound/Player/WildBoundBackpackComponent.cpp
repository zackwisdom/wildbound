#include "WildBoundBackpackComponent.h"

#include "../Inventory/WildBoundInventoryComponent.h"
#include "../UI/SWildBoundBackpackWidget.h"
#include "Engine/Engine.h"
#include "Engine/GameViewportClient.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "InputCoreTypes.h"
#include "Widgets/SOverlay.h"

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
	if (BackpackViewportRoot.IsValid() && GEngine && GEngine->GameViewport)
	{
		GEngine->GameViewport->RemoveViewportWidgetContent(BackpackViewportRoot.ToSharedRef());
	}

	BackpackWidget.Reset();
	BackpackViewportRoot.Reset();
	bBackpackOpen = false;
}
