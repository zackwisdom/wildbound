#include "WildBoundEquipmentComponent.h"

#include "../Inventory/WildBoundInventoryComponent.h"
#include "Camera/CameraComponent.h"
#include "Components/SceneComponent.h"
#include "Components/SpotLightComponent.h"
#include "Engine/Engine.h"
#include "GameFramework/Actor.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "InputCoreTypes.h"

namespace
{
	const FName FlashlightItemId(TEXT("Flashlight"));
}

UWildBoundEquipmentComponent::UWildBoundEquipmentComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = true;
	PrimaryComponentTick.TickGroup = TG_PostUpdateWork;
}

void UWildBoundEquipmentComponent::BeginPlay()
{
	Super::BeginPlay();

	InventoryComponent = GetOwner()
		? GetOwner()->FindComponentByClass<UWildBoundInventoryComponent>()
		: nullptr;

	EnsureFlashlightLight();
}

void UWildBoundEquipmentComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	SetFlashlightOn(false);
	Super::EndPlay(EndPlayReason);
}

void UWildBoundEquipmentComponent::TickComponent(
	float DeltaTime,
	ELevelTick TickType,
	FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (!InventoryComponent.IsValid() && GetOwner())
	{
		InventoryComponent = GetOwner()->FindComponentByClass<UWildBoundInventoryComponent>();
	}

	EnsureFlashlightLight();

	const UWildBoundInventoryComponent* Inventory = InventoryComponent.Get();
	if (bFlashlightOn && (!Inventory || !Inventory->HasItem(FlashlightItemId, 1)))
	{
		SetFlashlightOn(false);
	}

	APawn* Pawn = Cast<APawn>(GetOwner());
	APlayerController* PlayerController = Pawn ? Cast<APlayerController>(Pawn->GetController()) : nullptr;
	if (PlayerController && PlayerController->WasInputKeyJustPressed(EKeys::F))
	{
		ToggleFlashlight();
	}
}

void UWildBoundEquipmentComponent::EnsureFlashlightLight()
{
	if (FlashlightLight || !GetOwner())
	{
		return;
	}

	AActor* Owner = GetOwner();
	USceneComponent* AttachParent = Owner->FindComponentByClass<UCameraComponent>();
	if (!AttachParent)
	{
		AttachParent = Owner->GetRootComponent();
	}
	if (!AttachParent)
	{
		return;
	}

	FlashlightLight = NewObject<USpotLightComponent>(Owner, TEXT("WildBoundFlashlightLight"));
	if (!FlashlightLight)
	{
		return;
	}

	Owner->AddInstanceComponent(FlashlightLight);
	FlashlightLight->SetupAttachment(AttachParent);
	FlashlightLight->SetRelativeLocation(FVector(8.0f, 0.0f, -4.0f));
	FlashlightLight->SetRelativeRotation(FRotator::ZeroRotator);
	FlashlightLight->SetIntensity(8500.0f);
	FlashlightLight->SetAttenuationRadius(2400.0f);
	FlashlightLight->SetInnerConeAngle(18.0f);
	FlashlightLight->SetOuterConeAngle(36.0f);
	FlashlightLight->SetLightColor(FLinearColor(1.0f, 0.93f, 0.79f));
	FlashlightLight->SetCastShadows(true);
	FlashlightLight->SetVisibility(false);
	FlashlightLight->RegisterComponent();
}

void UWildBoundEquipmentComponent::ToggleFlashlight()
{
	const UWildBoundInventoryComponent* Inventory = InventoryComponent.Get();
	if (!Inventory || !Inventory->HasItem(FlashlightItemId, 1))
	{
		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(
				91040,
				1.8f,
				FColor(185, 185, 170),
				TEXT("You do not have a working flashlight."));
		}
		return;
	}

	SetFlashlightOn(!bFlashlightOn);

	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(
			91040,
			1.4f,
			bFlashlightOn ? FColor(235, 220, 160) : FColor(150, 150, 140),
			bFlashlightOn ? TEXT("FLASHLIGHT ON  [F]") : TEXT("FLASHLIGHT OFF  [F]"));
	}
}

void UWildBoundEquipmentComponent::SetFlashlightOn(bool bNewOn)
{
	bFlashlightOn = bNewOn;
	if (FlashlightLight)
	{
		FlashlightLight->SetVisibility(bFlashlightOn);
	}
}
