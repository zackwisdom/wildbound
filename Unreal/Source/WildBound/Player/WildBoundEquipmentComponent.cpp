#include "WildBoundEquipmentComponent.h"

#include "../Inventory/WildBoundInventoryComponent.h"
#include "Camera/CameraComponent.h"
#include "Components/SceneComponent.h"
#include "Components/SpotLightComponent.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/Actor.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "InputCoreTypes.h"

namespace
{
	const FName FlashlightItemId(TEXT("Flashlight"));
	const FName CrowbarItemId(TEXT("Crowbar"));
	const FName PryLockedTag(TEXT("WBPryLocked"));
	const FName PryContainerTag(TEXT("WBPryContainer"));
	const FName PryAccessTag(TEXT("WBPryAccess"));
	const FName ContainerTypeTag(TEXT("WBTypeContainer"));
	const FName CommercialGateGroupTag(TEXT("WBPryGroupCommercialGate"));
}

UWildBoundEquipmentComponent::UWildBoundEquipmentComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = true;
	PrimaryComponentTick.TickGroup = TG_LastDemotable;
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
	if (!PlayerController)
	{
		return;
	}

	if (PlayerController->WasInputKeyJustPressed(EKeys::F))
	{
		ToggleFlashlight();
	}

	HandleCrowbarInteraction(*PlayerController);
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

AActor* UWildBoundEquipmentComponent::GetPryTarget(APlayerController& PlayerController) const
{
	UWorld* World = GetWorld();
	APawn* Pawn = Cast<APawn>(GetOwner());
	if (!World || !Pawn)
	{
		return nullptr;
	}

	FVector ViewLocation;
	FRotator ViewRotation;
	PlayerController.GetPlayerViewPoint(ViewLocation, ViewRotation);

	const FVector TraceEnd = ViewLocation + ViewRotation.Vector() * ToolInteractionDistance;
	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(WildBoundPryTrace), false, Pawn);

	FHitResult Hit;
	if (!World->LineTraceSingleByChannel(Hit, ViewLocation, TraceEnd, ECC_Visibility, QueryParams))
	{
		return nullptr;
	}

	AActor* Target = Hit.GetActor();
	return Target && Target->ActorHasTag(PryLockedTag) ? Target : nullptr;
}

void UWildBoundEquipmentComponent::HandleCrowbarInteraction(APlayerController& PlayerController)
{
	AActor* Target = GetPryTarget(PlayerController);
	if (!Target)
	{
		return;
	}

	const UWildBoundInventoryComponent* Inventory = InventoryComponent.Get();
	const bool bHasCrowbar = Inventory && Inventory->HasItem(CrowbarItemId, 1);
	const bool bAccessGate = Target->ActorHasTag(PryAccessTag);

	if (GEngine)
	{
		const FString Prompt = bHasCrowbar
			? (bAccessGate ? TEXT("[E] Pry open maintenance gate") : TEXT("[E] Pry open sealed container"))
			: (bAccessGate ? TEXT("[E] Locked gate - crowbar required") : TEXT("[E] Sealed - crowbar required"));

		// Reuse the normal interaction prompt id so the tool-specific prompt replaces generic "Interact".
		GEngine->AddOnScreenDebugMessage(91001, 0.08f, bHasCrowbar ? FColor::White : FColor(215, 155, 105), Prompt);
	}

	if (!PlayerController.WasInputKeyJustPressed(EKeys::E))
	{
		return;
	}

	if (!bHasCrowbar)
	{
		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(91041, 1.8f, FColor(220, 145, 100), TEXT("You need a crowbar to force this open."));
		}
		return;
	}

	PryTarget(*Target);
}

void UWildBoundEquipmentComponent::PryTarget(AActor& TargetActor)
{
	if (TargetActor.ActorHasTag(PryAccessTag))
	{
		DestroyPryGroup(CommercialGateGroupTag);
		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(91041, 2.2f, FColor(185, 205, 165), TEXT("Maintenance gate forced open."));
		}
		return;
	}

	if (TargetActor.ActorHasTag(PryContainerTag))
	{
		TargetActor.Tags.Remove(PryLockedTag);
		TargetActor.Tags.Remove(PryContainerTag);
		TargetActor.Tags.AddUnique(ContainerTypeTag);

		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(91041, 2.2f, FColor(185, 205, 165), TEXT("Seal forced open. Search the container."));
		}
	}
}

void UWildBoundEquipmentComponent::DestroyPryGroup(const FName& GroupTag)
{
	UWorld* World = GetWorld();
	if (!World || GroupTag.IsNone())
	{
		return;
	}

	TArray<AActor*> ActorsToDestroy;
	for (TActorIterator<AActor> It(World); It; ++It)
	{
		AActor* Actor = *It;
		if (Actor && Actor->ActorHasTag(GroupTag))
		{
			ActorsToDestroy.Add(Actor);
		}
	}

	for (AActor* Actor : ActorsToDestroy)
	{
		if (IsValid(Actor))
		{
			Actor->Destroy();
		}
	}
}
