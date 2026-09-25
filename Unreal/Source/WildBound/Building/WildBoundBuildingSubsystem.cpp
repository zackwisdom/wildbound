#include "WildBoundBuildingSubsystem.h"

#include "../Inventory/WildBoundInventoryComponent.h"
#include "../Player/WildBoundInteractionComponent.h"
#include "CollisionQueryParams.h"
#include "Components/PointLightComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/Engine.h"
#include "Engine/OverlapResult.h"
#include "Engine/PointLight.h"
#include "Engine/StaticMesh.h"
#include "Engine/StaticMeshActor.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "InputCoreTypes.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "UObject/UObjectGlobals.h"

namespace
{
	const FName PlayerBuildTag(TEXT("WildBoundPlayerBuild"));
	const FName PlayerBuildPreviewTag(TEXT("WildBoundBuildPreview"));
	const FName InteractableTag(TEXT("WBInteractable"));
	const FName ContainerTag(TEXT("WBTypeContainer"));
	const FName SearchedContainerTag(TEXT("WBContainerSearched"));
	const FName CrateContainerTag(TEXT("WBContainerCrate"));
	const FName StorageTag(TEXT("WBSafehouseStash"));
	const FName BedTag(TEXT("WBSafehouseBed"));
	const FName WorkbenchTag(TEXT("WBWorkbench"));
	const FName RainCollectorTag(TEXT("WBRainCollector"));
	const FName PowerBankTag(TEXT("WBPowerBank"));
	const FName PoweredLightTag(TEXT("WBPoweredLight"));
	const FName ReinforcedBuildTag(TEXT("WBReinforcedBuild"));
	const FName GeneratorTag(TEXT("WBGenerator"));
	const FName PowerLinkVisualTag(TEXT("WildBoundPowerLinkVisual"));
	const FName SafehouseSpawnAnchorTag(TEXT("WBSafehouseSpawnAnchor"));
	const FName PurifierTag(TEXT("WBWaterPurifier"));
	const FName HeaterTag(TEXT("WBHeater"));
	const FName ToolStationTag(TEXT("WBPoweredToolStation"));

	const FName FloorType(TEXT("BuildFloor"));
	const FName WallType(TEXT("BuildWall"));
	const FName DoorwayType(TEXT("BuildDoorway"));
	const FName RoofType(TEXT("BuildRoof"));
	const FName BarricadeType(TEXT("BuildBarricade"));
	const FName StorageType(TEXT("BuildStorage"));
	const FName CotType(TEXT("BuildCot"));
	const FName WorkbenchType(TEXT("BuildWorkbench"));
	const FName RainCollectorType(TEXT("BuildRainCollector"));
	const FName PowerBankType(TEXT("BuildPowerBank"));
	const FName PoweredLightType(TEXT("BuildPoweredLight"));
	const FName ReinforcedFloorType(TEXT("BuildReinforcedFloor"));
	const FName ReinforcedWallType(TEXT("BuildReinforcedWall"));
	const FName GeneratorType(TEXT("BuildGenerator"));
	const FName PurifierType(TEXT("BuildWaterPurifier"));
	const FName HeaterType(TEXT("BuildHeater"));
	const FName ToolStationType(TEXT("BuildPoweredToolStation"));

	constexpr float BuildManagementDistance = 475.0f;
	constexpr float DismantleHoldDuration = 0.75f;
	constexpr float PieceSnapDistance = 155.0f;
	constexpr float UtilityPowerRadius = 1400.0f;
	constexpr float ShelterUpgradeRadius = 1500.0f;
	constexpr float RainWaterSecondsPerUnit = 90.0f;
	constexpr int32 RainCollectorCapacity = 4;
	constexpr float GeneratorFuelSecondsPerCan = 240.0f;
	constexpr float GeneratorChargePerSecond = 2.2f;
	constexpr float BatteryCapacity = 100.0f;
	constexpr float PoweredLightLoadPerSecond = 0.18f;
	constexpr float PurifierLoadPerSecond = 0.55f;
	constexpr float HeaterLoadPerSecond = 0.75f;
	constexpr float ToolStationLoadPerSecond = 0.65f;
	constexpr float PurifierSecondsPerUnit = 45.0f;
	constexpr int32 PurifierCapacity = 4;

	UStaticMesh* GetCubeMesh()
	{
		static TWeakObjectPtr<UStaticMesh> Mesh;
		if (!Mesh.IsValid())
		{
			Mesh = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube"));
		}
		return Mesh.Get();
	}

	UMaterialInterface* GetBaseMaterial()
	{
		static TWeakObjectPtr<UMaterialInterface> Material;
		if (!Material.IsValid())
		{
			Material = LoadObject<UMaterialInterface>(
				nullptr,
				TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));
		}
		return Material.Get();
	}

	void ApplyPieceMaterial(
		UStaticMeshComponent& Mesh,
		const FLinearColor& Color,
		float Roughness,
		float Metallic)
	{
		UMaterialInterface* Base = GetBaseMaterial();
		if (!Base)
		{
			return;
		}

		UMaterialInstanceDynamic* Material = UMaterialInstanceDynamic::Create(Base, &Mesh);
		if (!Material)
		{
			return;
		}

		Material->SetVectorParameterValue(TEXT("Color"), Color);
		Material->SetVectorParameterValue(TEXT("BaseColor"), Color);
		Material->SetScalarParameterValue(TEXT("Roughness"), Roughness);
		Material->SetScalarParameterValue(TEXT("Metallic"), Metallic);
		Mesh.SetMaterial(0, Material);
	}

	AStaticMeshActor* SpawnBuildPiece(
		UWorld& World,
		const FVector& Location,
		const FVector& Scale,
		const FRotator& Rotation,
		const FLinearColor& Color,
		const TCHAR* Label,
		TArray<AActor*>* OutSpawnedActors,
		float Roughness = 0.90f,
		float Metallic = 0.02f)
	{
		UStaticMesh* Cube = GetCubeMesh();
		if (!Cube)
		{
			return nullptr;
		}

		AStaticMeshActor* Actor = World.SpawnActor<AStaticMeshActor>(Location, Rotation);
		if (!Actor)
		{
			return nullptr;
		}

		Actor->Tags.AddUnique(PlayerBuildTag);
#if WITH_EDITOR
		Actor->SetActorLabel(Label);
#endif

		UStaticMeshComponent* Mesh = Actor->GetStaticMeshComponent();
		Mesh->SetMobility(EComponentMobility::Movable);
		Mesh->SetStaticMesh(Cube);
		Mesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
		Mesh->SetCollisionResponseToAllChannels(ECR_Block);
		Mesh->SetCastShadow(true);
		ApplyPieceMaterial(*Mesh, Color, Roughness, Metallic);
		Actor->SetActorScale3D(Scale);

		if (OutSpawnedActors)
		{
			OutSpawnedActors->Add(Actor);
		}
		return Actor;
	}


	AStaticMeshActor* SpawnPowerLinkVisual(
		UWorld& World,
		const FVector& Start,
		const FVector& End)
	{
		UStaticMesh* Cube = GetCubeMesh();
		if (!Cube)
		{
			return nullptr;
		}

		const FVector Delta = End - Start;
		const float Distance = Delta.Size();
		if (Distance < KINDA_SMALL_NUMBER)
		{
			return nullptr;
		}

		AStaticMeshActor* Actor = World.SpawnActor<AStaticMeshActor>(
			(Start + End) * 0.5f,
			Delta.Rotation());
		if (!Actor)
		{
			return nullptr;
		}

		Actor->Tags.AddUnique(PowerLinkVisualTag);
#if WITH_EDITOR
		Actor->SetActorLabel(TEXT("WB_Power_Link"));
#endif

		UStaticMeshComponent* Mesh = Actor->GetStaticMeshComponent();
		Mesh->SetMobility(EComponentMobility::Movable);
		Mesh->SetStaticMesh(Cube);
		Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Mesh->SetCastShadow(false);
		ApplyPieceMaterial(
			*Mesh,
			FLinearColor(0.055f, 0.060f, 0.050f, 1.0f),
			0.70f,
			0.42f);
		Actor->SetActorScale3D(FVector(Distance / 100.0f, 0.035f, 0.035f));
		return Actor;
	}

	bool IsStructuralBuild(FName BuildTypeId)
	{
		return BuildTypeId == FloorType
			|| BuildTypeId == WallType
			|| BuildTypeId == DoorwayType
			|| BuildTypeId == RoofType
			|| BuildTypeId == ReinforcedFloorType
			|| BuildTypeId == ReinforcedWallType;
	}
}

void UWildBoundBuildingSubsystem::OnWorldBeginPlay(UWorld& InWorld)
{
	Super::OnWorldBeginPlay(InWorld);

	if (!InWorld.IsGameWorld())
	{
		return;
	}

	InWorld.GetTimerManager().SetTimer(
		BuildingUpdateTimer,
		this,
		&UWildBoundBuildingSubsystem::UpdateBuildingMode,
		0.035f,
		true,
		0.20f);

	InWorld.GetTimerManager().SetTimer(
		UtilityUpdateTimer,
		this,
		&UWildBoundBuildingSubsystem::UpdateUtilities,
		1.0f,
		true,
		1.0f);
}

void UWildBoundBuildingSubsystem::Deinitialize()
{
	CancelPlacement(false);

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(BuildingUpdateTimer);
		World->GetTimerManager().ClearTimer(UtilityUpdateTimer);
	}

	Super::Deinitialize();
}

bool UWildBoundBuildingSubsystem::BeginPlacement(
	FName BuildTypeId,
	const FString& DisplayName,
	const TMap<FName, int32>& MaterialCosts)
{
	if (BuildTypeId.IsNone() || MaterialCosts.IsEmpty())
	{
		return false;
	}

	UWildBoundInventoryComponent* Inventory = GetPlayerInventory();
	if (!Inventory)
	{
		return false;
	}

	for (const TPair<FName, int32>& Cost : MaterialCosts)
	{
		if (Cost.Key.IsNone() || Cost.Value <= 0 || !Inventory->HasItem(Cost.Key, Cost.Value))
		{
			return false;
		}
	}

	CancelPlacement(false);
	ActiveBuildTypeId = BuildTypeId;
	ActiveDisplayName = DisplayName;
	ActiveMaterialCosts = MaterialCosts;
	CurrentYaw = 0.0f;
	bGridSnapEnabled = true;
	bPlacementActive = true;
	bPlacementValid = false;

	if (UWorld* World = GetWorld())
	{
		PlacementStartedAt = World->GetTimeSeconds();
	}

	EnsurePreviewActor();
	UpdatePreviewTransform();

	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(
			91701,
			2.8f,
			FColor(178, 205, 154),
			TEXT("BUILD MODE   |   materials are only consumed after successful placement"));
	}

	return PreviewActor.IsValid();
}

void UWildBoundBuildingSubsystem::UpdateBuildingMode()
{
	if (!bPlacementActive)
	{
		UpdateManagementMode();
		return;
	}

	UWorld* World = GetWorld();
	APlayerController* PlayerController = World ? World->GetFirstPlayerController() : nullptr;
	if (!World || !PlayerController)
	{
		CancelPlacement(false);
		return;
	}

	UpdatePreviewTransform();

	if (PlayerController->WasInputKeyJustPressed(EKeys::R))
	{
		const float Step = PlayerController->IsInputKeyDown(EKeys::LeftShift)
			|| PlayerController->IsInputKeyDown(EKeys::RightShift)
			? 90.0f
			: 15.0f;
		CurrentYaw = FMath::Fmod(CurrentYaw + Step, 360.0f);
		PreviewRotation = FRotator(0.0f, CurrentYaw, 0.0f);
		UpdatePreviewTransform();
	}

	if (PlayerController->WasInputKeyJustPressed(EKeys::G))
	{
		bGridSnapEnabled = !bGridSnapEnabled;
		UpdatePreviewTransform();
	}

	const float Elapsed = World->GetTimeSeconds() - PlacementStartedAt;
	if (Elapsed > 0.18f && PlayerController->WasInputKeyJustPressed(EKeys::LeftMouseButton))
	{
		TryPlaceActiveBuild();
		return;
	}

	if (PlayerController->WasInputKeyJustPressed(EKeys::Escape)
		|| PlayerController->WasInputKeyJustPressed(EKeys::RightMouseButton))
	{
		CancelPlacement(true);
		return;
	}

	const bool bAffordable = CanAffordActiveBuild();
	const FColor StatusColor = bPlacementValid && bAffordable
		? FColor(132, 220, 128)
		: FColor(230, 118, 92);
	const FString Status = !bAffordable
		? TEXT("MATERIALS CHANGED")
		: (bPlacementValid
			? (bPieceSnapped ? TEXT("SNAPPED") : TEXT("VALID"))
			: TEXT("OBSTRUCTED"));
	const FString ModeLabel = bRelocatingBuild ? TEXT("MOVE") : TEXT("BUILD");

	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(
			91700,
			0.12f,
			StatusColor,
			FString::Printf(
				TEXT("%s  %s   |   %s   |   LMB PLACE   R ROTATE   SHIFT+R 90 DEG   G SNAP %s   RMB/ESC CANCEL"),
				*ModeLabel,
				*ActiveDisplayName,
				*Status,
				bGridSnapEnabled ? TEXT("ON") : TEXT("OFF")));
	}
}

void UWildBoundBuildingSubsystem::UpdatePreviewTransform()
{
	UWorld* World = GetWorld();
	APlayerController* PlayerController = World ? World->GetFirstPlayerController() : nullptr;
	APawn* Pawn = PlayerController ? PlayerController->GetPawn() : nullptr;
	if (!World || !PlayerController || !Pawn)
	{
		bPlacementValid = false;
		UpdatePreviewMaterial();
		return;
	}

	EnsurePreviewActor();
	AStaticMeshActor* Preview = PreviewActor.Get();
	if (!Preview)
	{
		bPlacementValid = false;
		return;
	}

	FVector ViewLocation;
	FRotator ViewRotation;
	PlayerController->GetPlayerViewPoint(ViewLocation, ViewRotation);

	const FVector AimPoint = ViewLocation + ViewRotation.Vector() * 650.0f;
	FCollisionQueryParams GroundParams(SCENE_QUERY_STAT(WildBoundBuildGround), false, Pawn);
	GroundParams.AddIgnoredActor(Preview);
	FHitResult GroundHit;
	const FVector GroundStart(AimPoint.X, AimPoint.Y, AimPoint.Z + 550.0f);
	const FVector GroundEnd(AimPoint.X, AimPoint.Y, AimPoint.Z - 1600.0f);
	const bool bFoundGround = World->LineTraceSingleByChannel(
		GroundHit,
		GroundStart,
		GroundEnd,
		ECC_Visibility,
		GroundParams);

	if (!bFoundGround)
	{
		bPieceSnapped = false;
		bPlacementValid = false;
		UpdatePreviewMaterial();
		return;
	}

	PreviewLocation = GroundHit.ImpactPoint;
	PreviewRotation = FRotator(0.0f, CurrentYaw, 0.0f);
	bPieceSnapped = false;

	if (bGridSnapEnabled)
	{
		bPieceSnapped = TryApplyPieceSnap(PreviewLocation, PreviewRotation);
		if (!bPieceSnapped)
		{
			PreviewLocation = SnapLocation(PreviewLocation);
			PreviewLocation.Z = GroundHit.ImpactPoint.Z;
		}
	}


	const FVector HalfExtents = GetBuildHalfExtents(ActiveBuildTypeId);
	const float VerticalOffset = GetBuildVerticalOffset(ActiveBuildTypeId);
	const FVector PreviewCenter = PreviewLocation + FVector(
		0.0f,
		0.0f,
		VerticalOffset + HalfExtents.Z);

	Preview->SetActorLocationAndRotation(
		PreviewCenter,
		PreviewRotation,
		false,
		nullptr,
		ETeleportType::TeleportPhysics);
	Preview->SetActorScale3D(FVector(
		FMath::Max(HalfExtents.X / 50.0f, 0.05f),
		FMath::Max(HalfExtents.Y / 50.0f, 0.05f),
		FMath::Max(HalfExtents.Z / 50.0f, 0.05f)));

	bPlacementValid = ValidatePlacement(
		PreviewLocation,
		PreviewRotation,
		GroundHit.GetActor());
	UpdatePreviewMaterial();
}

void UWildBoundBuildingSubsystem::TryPlaceActiveBuild()
{
	if (!bPlacementActive || !bPlacementValid)
	{
		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(
				91702,
				1.4f,
				FColor(230, 118, 92),
				TEXT("Cannot place here."));
		}
		return;
	}

	if (!CanAffordActiveBuild())
	{
		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(
				91702,
				1.8f,
				FColor(230, 145, 105),
				TEXT("Required materials are no longer in your inventory."));
		}
		return;
	}

	const int32 BuildId = bRelocatingBuild ? RelocatingBuildId : NextBuildId;
	TArray<AActor*> SpawnedActors;
	if (!SpawnPlacedBuild(
		ActiveBuildTypeId,
		PreviewLocation,
		PreviewRotation,
		&SpawnedActors))
	{
		for (AActor* Actor : SpawnedActors)
		{
			if (IsValid(Actor))
			{
				Actor->Destroy();
			}
		}
		return;
	}

	if (!ConsumeActiveBuildMaterials())
	{
		for (AActor* Actor : SpawnedActors)
		{
			if (IsValid(Actor))
			{
				Actor->Destroy();
			}
		}
		return;
	}

	RegisterBuildActors(BuildId, SpawnedActors);

	if (bRelocatingBuild)
	{
		if (FWildBoundPlacedBuildState* Existing = FindBuildState(BuildId))
		{
			Existing->Location = PreviewLocation;
			Existing->Rotation = PreviewRotation;
		}

		const FString MovedName = ActiveDisplayName;
		bRelocatingBuild = false;
		RelocatingBuildId = 0;
		CancelPlacement(false);

		RefreshPowerLinkVisuals();
		RefreshPoweredLights();
		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(
				91702,
				2.0f,
				FColor(150, 218, 140),
				FString::Printf(TEXT("RELOCATED   |   %s"), *MovedName));
		}
		return;
	}

	FWildBoundPlacedBuildState State;
	State.BuildId = BuildId;
	State.BuildTypeId = ActiveBuildTypeId;
	State.Location = PreviewLocation;
	State.Rotation = PreviewRotation;
	if (State.BuildTypeId == GeneratorType)
	{
		State.bUtilityEnabled = false;
		State.FuelSecondsRemaining = 0.0f;
	}
	else if (State.BuildTypeId == PowerBankType)
	{
		State.bUtilityEnabled = true;
		State.StoredPower = 0.0f;
	}
	PlacedBuilds.Add(State);
	NextBuildId = FMath::Max(NextBuildId, BuildId + 1);
	RefreshPowerLinkVisuals();
	RefreshPoweredLights();

	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(
			91702,
			2.0f,
			FColor(150, 218, 140),
			FString::Printf(TEXT("PLACED   |   %s"), *ActiveDisplayName));
	}

	if (!CanAffordActiveBuild())
	{
		CancelPlacement(false);
	}
}

void UWildBoundBuildingSubsystem::CancelPlacement(bool bShowMessage)
{
	if (!bPlacementActive && !PreviewActor.IsValid())
	{
		return;
	}

	const bool bWasRelocating = bRelocatingBuild;
	const FWildBoundPlacedBuildState OriginalState = RelocationOriginalState;
	const FString CancelledName = ActiveDisplayName;

	bPlacementActive = false;
	bPlacementValid = false;
	bPieceSnapped = false;
	bRelocatingBuild = false;
	RelocatingBuildId = 0;
	ActiveBuildTypeId = NAME_None;
	ActiveDisplayName.Reset();
	ActiveMaterialCosts.Reset();
	DestroyPreviewActor();

	if (bWasRelocating && OriginalState.BuildId > 0 && !OriginalState.BuildTypeId.IsNone())
	{
		TArray<AActor*> RestoredActors;
		if (SpawnPlacedBuild(
			OriginalState.BuildTypeId,
			OriginalState.Location,
			OriginalState.Rotation,
			&RestoredActors))
		{
			RegisterBuildActors(OriginalState.BuildId, RestoredActors);
			RefreshPowerLinkVisuals();
			RefreshPoweredLights();
		}
	}

	if (bShowMessage && GEngine)
	{
		GEngine->AddOnScreenDebugMessage(
			91700,
			1.6f,
			FColor(185, 190, 178),
			bWasRelocating
				? FString::Printf(TEXT("MOVE CANCELLED   |   %s restored"), *CancelledName)
				: TEXT("BUILD MODE CANCELLED   |   no materials consumed"));
	}
}

void UWildBoundBuildingSubsystem::EnsurePreviewActor()
{
	if (PreviewActor.IsValid())
	{
		return;
	}

	UWorld* World = GetWorld();
	UStaticMesh* Cube = GetCubeMesh();
	if (!World || !Cube)
	{
		return;
	}

	AStaticMeshActor* Preview = World->SpawnActor<AStaticMeshActor>(
		FVector::ZeroVector,
		FRotator::ZeroRotator);
	if (!Preview)
	{
		return;
	}

	Preview->Tags.AddUnique(PlayerBuildPreviewTag);
#if WITH_EDITOR
	Preview->SetActorLabel(TEXT("WB_BuildPlacementPreview"));
#endif

	UStaticMeshComponent* Mesh = Preview->GetStaticMeshComponent();
	Mesh->SetMobility(EComponentMobility::Movable);
	Mesh->SetStaticMesh(Cube);
	Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Mesh->SetCastShadow(false);

	if (UMaterialInterface* Base = GetBaseMaterial())
	{
		UMaterialInstanceDynamic* Material = UMaterialInstanceDynamic::Create(Base, Mesh);
		if (Material)
		{
			Material->SetScalarParameterValue(TEXT("Roughness"), 0.55f);
			Material->SetScalarParameterValue(TEXT("Metallic"), 0.0f);
			Material->SetScalarParameterValue(TEXT("Opacity"), 0.36f);
			Mesh->SetMaterial(0, Material);
			PreviewMaterial = Material;
		}
	}

	PreviewActor = Preview;
}

void UWildBoundBuildingSubsystem::DestroyPreviewActor()
{
	if (AStaticMeshActor* Preview = PreviewActor.Get())
	{
		Preview->Destroy();
	}

	PreviewActor.Reset();
	PreviewMaterial.Reset();
}

void UWildBoundBuildingSubsystem::UpdatePreviewMaterial()
{
	UMaterialInstanceDynamic* Material = PreviewMaterial.Get();
	if (!Material)
	{
		return;
	}

	const bool bReady = bPlacementValid && CanAffordActiveBuild();
	const FLinearColor Color = bReady
		? FLinearColor(0.10f, 0.68f, 0.16f, 0.36f)
		: FLinearColor(0.78f, 0.10f, 0.055f, 0.36f);
	Material->SetVectorParameterValue(TEXT("Color"), Color);
	Material->SetVectorParameterValue(TEXT("BaseColor"), Color);
}

bool UWildBoundBuildingSubsystem::CanAffordActiveBuild() const
{
	if (bRelocatingBuild)
	{
		return true;
	}

	const UWildBoundInventoryComponent* Inventory = GetPlayerInventory();
	if (!Inventory || ActiveMaterialCosts.IsEmpty())
	{
		return false;
	}

	for (const TPair<FName, int32>& Cost : ActiveMaterialCosts)
	{
		if (!Inventory->HasItem(Cost.Key, Cost.Value))
		{
			return false;
		}
	}
	return true;
}

bool UWildBoundBuildingSubsystem::ConsumeActiveBuildMaterials()
{
	if (bRelocatingBuild)
	{
		return true;
	}

	UWildBoundInventoryComponent* Inventory = GetPlayerInventory();
	if (!Inventory || !CanAffordActiveBuild())
	{
		return false;
	}

	TArray<TPair<FName, int32>> Removed;
	for (const TPair<FName, int32>& Cost : ActiveMaterialCosts)
	{
		if (!Inventory->RemoveItem(Cost.Key, Cost.Value))
		{
			for (const TPair<FName, int32>& Refund : Removed)
			{
				Inventory->AddItem(Refund.Key, Refund.Value);
			}
			return false;
		}
		Removed.Add(Cost);
	}
	return true;
}

bool UWildBoundBuildingSubsystem::ValidatePlacement(
	const FVector& Location,
	const FRotator& Rotation,
	AActor* GroundActor) const
{
	UWorld* World = GetWorld();
	APlayerController* PlayerController = World ? World->GetFirstPlayerController() : nullptr;
	APawn* Pawn = PlayerController ? PlayerController->GetPawn() : nullptr;
	if (!World || !Pawn || FVector::DistSquared2D(Pawn->GetActorLocation(), Location) > FMath::Square(850.0f))
	{
		return false;
	}

	if (IsDuplicatePlacement(
		Location,
		ActiveBuildTypeId,
		bRelocatingBuild ? RelocatingBuildId : 0))
	{
		return false;
	}

	const FVector HalfExtents = GetBuildHalfExtents(ActiveBuildTypeId);
	const FVector Center = Location + FVector(
		0.0f,
		0.0f,
		GetBuildVerticalOffset(ActiveBuildTypeId) + HalfExtents.Z);

	FCollisionQueryParams Params(SCENE_QUERY_STAT(WildBoundBuildOverlap), false, Pawn);
	if (GroundActor)
	{
		Params.AddIgnoredActor(GroundActor);
	}
	if (PreviewActor.IsValid())
	{
		Params.AddIgnoredActor(PreviewActor.Get());
	}

	FCollisionObjectQueryParams ObjectParams;
	ObjectParams.AddObjectTypesToQuery(ECC_WorldStatic);
	ObjectParams.AddObjectTypesToQuery(ECC_WorldDynamic);
	ObjectParams.AddObjectTypesToQuery(ECC_PhysicsBody);

	TArray<FOverlapResult> Overlaps;
	World->OverlapMultiByObjectType(
		Overlaps,
		Center,
		Rotation.Quaternion(),
		ObjectParams,
		FCollisionShape::MakeBox(HalfExtents * 0.92f),
		Params);

	for (const FOverlapResult& Overlap : Overlaps)
	{
		AActor* Other = Overlap.GetActor();
		if (!Other || Other == GroundActor || Other == Pawn || Other->ActorHasTag(PlayerBuildPreviewTag))
		{
			continue;
		}

		if (IsStructuralBuild(ActiveBuildTypeId) && Other->ActorHasTag(PlayerBuildTag))
		{
			continue;
		}

		return false;
	}

	return true;
}

bool UWildBoundBuildingSubsystem::SpawnPlacedBuild(
	FName BuildTypeId,
	const FVector& Location,
	const FRotator& Rotation,
	TArray<AActor*>* OutSpawnedActors)
{
	UWorld* World = GetWorld();
	if (!World || BuildTypeId.IsNone())
	{
		return false;
	}

	const FLinearColor Wood(0.265f, 0.175f, 0.088f, 1.0f);
	const FLinearColor DarkWood(0.125f, 0.078f, 0.040f, 1.0f);
	const FLinearColor Scrap(0.145f, 0.145f, 0.125f, 1.0f);
	const FLinearColor Rust(0.245f, 0.105f, 0.050f, 1.0f);
	const FLinearColor Canvas(0.235f, 0.240f, 0.205f, 1.0f);
	const FVector Forward = Rotation.RotateVector(FVector(1.0f, 0.0f, 0.0f));

	if (BuildTypeId == FloorType)
	{
		return SpawnBuildPiece(
			*World,
			Location + FVector(0.0f, 0.0f, 10.0f),
			FVector(4.0f, 4.0f, 0.20f),
			Rotation,
			Wood,
			TEXT("WB_PlayerBuild_Floor"),
			OutSpawnedActors) != nullptr;
	}

	if (BuildTypeId == WallType)
	{
		return SpawnBuildPiece(
			*World,
			Location + FVector(0.0f, 0.0f, 125.0f),
			FVector(4.0f, 0.25f, 2.50f),
			Rotation,
			Wood,
			TEXT("WB_PlayerBuild_Wall"),
			OutSpawnedActors) != nullptr;
	}

	if (BuildTypeId == DoorwayType)
	{
		const FVector Right = Rotation.RotateVector(FVector(0.0f, 1.0f, 0.0f));
		bool bSpawned = false;
		bSpawned |= SpawnBuildPiece(
			*World,
			Location - Right * 150.0f + FVector(0.0f, 0.0f, 125.0f),
			FVector(1.0f, 0.25f, 2.50f),
			Rotation,
			Wood,
			TEXT("WB_PlayerBuild_DoorwayLeft"),
			OutSpawnedActors) != nullptr;
		bSpawned |= SpawnBuildPiece(
			*World,
			Location + Right * 150.0f + FVector(0.0f, 0.0f, 125.0f),
			FVector(1.0f, 0.25f, 2.50f),
			Rotation,
			Wood,
			TEXT("WB_PlayerBuild_DoorwayRight"),
			OutSpawnedActors) != nullptr;
		bSpawned |= SpawnBuildPiece(
			*World,
			Location + FVector(0.0f, 0.0f, 225.0f),
			FVector(2.0f, 0.25f, 0.50f),
			Rotation,
			DarkWood,
			TEXT("WB_PlayerBuild_DoorwayLintel"),
			OutSpawnedActors) != nullptr;
		return bSpawned;
	}

	if (BuildTypeId == RoofType)
	{
		return SpawnBuildPiece(
			*World,
			Location + FVector(0.0f, 0.0f, 260.0f),
			FVector(4.10f, 4.10f, 0.20f),
			Rotation,
			Canvas,
			TEXT("WB_PlayerBuild_Roof"),
			OutSpawnedActors,
			0.96f) != nullptr;
	}

	if (BuildTypeId == BarricadeType)
	{
		const FVector Right = Rotation.RotateVector(FVector(0.0f, 1.0f, 0.0f));
		bool bSpawned = false;
		for (float Z : {45.0f, 90.0f, 135.0f})
		{
			bSpawned |= SpawnBuildPiece(
				*World,
				Location + FVector(0.0f, 0.0f, Z),
				FVector(3.0f, 0.22f, 0.22f),
				FRotator(0.0f, Rotation.Yaw, Z == 90.0f ? 5.0f : -4.0f),
				Wood,
				TEXT("WB_PlayerBuild_BarricadeBeam"),
				OutSpawnedActors) != nullptr;
		}
		for (float Side : {-120.0f, 120.0f})
		{
			bSpawned |= SpawnBuildPiece(
				*World,
				Location + Right * Side + FVector(0.0f, 0.0f, 82.0f),
				FVector(0.18f, 0.18f, 1.65f),
				Rotation,
				Scrap,
				TEXT("WB_PlayerBuild_BarricadePost"),
				OutSpawnedActors,
				0.76f,
				0.35f) != nullptr;
		}
		return bSpawned;
	}

	if (BuildTypeId == StorageType)
	{
		AStaticMeshActor* Body = SpawnBuildPiece(
			*World,
			Location + FVector(0.0f, 0.0f, 42.0f),
			FVector(1.40f, 1.0f, 0.84f),
			Rotation,
			FLinearColor(0.16f, 0.18f, 0.13f, 1.0f),
			TEXT("WB_PlayerBuild_Storage"),
			OutSpawnedActors,
			0.84f,
			0.22f);
		if (!Body)
		{
			return false;
		}
		Body->Tags.AddUnique(InteractableTag);
		Body->Tags.AddUnique(ContainerTag);
		Body->Tags.AddUnique(SearchedContainerTag);
		Body->Tags.AddUnique(CrateContainerTag);
		Body->Tags.AddUnique(StorageTag);
		SpawnBuildPiece(
			*World,
			Location + FVector(0.0f, 0.0f, 88.0f),
			FVector(1.46f, 1.05f, 0.08f),
			Rotation,
			Scrap,
			TEXT("WB_PlayerBuild_StorageLid"),
			OutSpawnedActors,
			0.72f,
			0.36f);
		return true;
	}

	if (BuildTypeId == CotType)
	{
		SpawnBuildPiece(
			*World,
			Location + FVector(0.0f, 0.0f, 28.0f),
			FVector(2.25f, 0.88f, 0.18f),
			Rotation,
			DarkWood,
			TEXT("WB_PlayerBuild_CotFrame"),
			OutSpawnedActors);
		AStaticMeshActor* Mattress = SpawnBuildPiece(
			*World,
			Location + FVector(0.0f, 0.0f, 48.0f),
			FVector(2.10f, 0.80f, 0.20f),
			Rotation,
			Canvas,
			TEXT("WB_PlayerBuild_Cot"),
			OutSpawnedActors,
			0.96f);
		if (!Mattress)
		{
			return false;
		}
		Mattress->Tags.AddUnique(InteractableTag);
		Mattress->Tags.AddUnique(BedTag);
		return true;
	}

	if (BuildTypeId == ReinforcedFloorType)
	{
		AStaticMeshActor* Base = SpawnBuildPiece(
			*World,
			Location + FVector(0.0f, 0.0f, 12.0f),
			FVector(4.0f, 4.0f, 0.24f),
			Rotation,
			DarkWood,
			TEXT("WB_PlayerBuild_ReinforcedFloor"),
			OutSpawnedActors,
			0.84f);
		if (!Base)
		{
			return false;
		}
		Base->Tags.AddUnique(ReinforcedBuildTag);
		const FVector Right = Rotation.RotateVector(FVector(0.0f, 1.0f, 0.0f));
		for (float Offset : {-165.0f, 165.0f})
		{
			AStaticMeshActor* Brace = SpawnBuildPiece(
				*World,
				Location + Right * Offset + FVector(0.0f, 0.0f, 27.0f),
				FVector(3.7f, 0.10f, 0.08f),
				Rotation,
				Scrap,
				TEXT("WB_PlayerBuild_ReinforcedFloorBrace"),
				OutSpawnedActors,
				0.68f,
				0.46f);
			if (Brace)
			{
				Brace->Tags.AddUnique(ReinforcedBuildTag);
			}
		}
		return true;
	}

	if (BuildTypeId == ReinforcedWallType)
	{
		AStaticMeshActor* Base = SpawnBuildPiece(
			*World,
			Location + FVector(0.0f, 0.0f, 125.0f),
			FVector(4.0f, 0.28f, 2.50f),
			Rotation,
			DarkWood,
			TEXT("WB_PlayerBuild_ReinforcedWall"),
			OutSpawnedActors,
			0.82f);
		if (!Base)
		{
			return false;
		}
		Base->Tags.AddUnique(ReinforcedBuildTag);
		const FVector Right = Rotation.RotateVector(FVector(0.0f, 1.0f, 0.0f));
		for (float Side : {-145.0f, 145.0f})
		{
			AStaticMeshActor* Brace = SpawnBuildPiece(
				*World,
				Location + Right * Side + FVector(0.0f, 0.0f, 125.0f),
				FVector(0.12f, 0.36f, 2.35f),
				Rotation,
				Scrap,
				TEXT("WB_PlayerBuild_ReinforcedWallBrace"),
				OutSpawnedActors,
				0.68f,
				0.52f);
			if (Brace)
			{
				Brace->Tags.AddUnique(ReinforcedBuildTag);
			}
		}
		return true;
	}

	if (BuildTypeId == RainCollectorType)
	{
		const FVector Right = Rotation.RotateVector(FVector(0.0f, 1.0f, 0.0f));
		AStaticMeshActor* Tank = SpawnBuildPiece(
			*World,
			Location + FVector(0.0f, 0.0f, 70.0f),
			FVector(1.10f, 0.90f, 1.35f),
			Rotation,
			FLinearColor(0.11f, 0.14f, 0.12f, 1.0f),
			TEXT("WB_PlayerBuild_RainCollectorTank"),
			OutSpawnedActors,
			0.86f,
			0.18f);
		if (!Tank)
		{
			return false;
		}
		Tank->Tags.AddUnique(InteractableTag);
		Tank->Tags.AddUnique(RainCollectorTag);

		SpawnBuildPiece(
			*World,
			Location + FVector(0.0f, 0.0f, 175.0f),
			FVector(1.75f, 1.45f, 0.08f),
			FRotator(Rotation.Pitch + 7.0f, Rotation.Yaw, Rotation.Roll),
			Scrap,
			TEXT("WB_PlayerBuild_RainCollectorCatch"),
			OutSpawnedActors,
			0.74f,
			0.36f);
		for (float Side : {-78.0f, 78.0f})
		{
			SpawnBuildPiece(
				*World,
				Location + Right * Side + FVector(0.0f, 0.0f, 125.0f),
				FVector(0.08f, 0.08f, 1.55f),
				Rotation,
				Rust,
				TEXT("WB_PlayerBuild_RainCollectorPost"),
				OutSpawnedActors,
				0.72f,
				0.40f);
		}
		return true;
	}

	if (BuildTypeId == PowerBankType)
	{
		AStaticMeshActor* Cabinet = SpawnBuildPiece(
			*World,
			Location + FVector(0.0f, 0.0f, 70.0f),
			FVector(1.20f, 0.78f, 1.35f),
			Rotation,
			FLinearColor(0.10f, 0.115f, 0.10f, 1.0f),
			TEXT("WB_PlayerBuild_PowerBank"),
			OutSpawnedActors,
			0.70f,
			0.48f);
		if (!Cabinet)
		{
			return false;
		}
		Cabinet->Tags.AddUnique(InteractableTag);
		Cabinet->Tags.AddUnique(PowerBankTag);

		for (float Z : {42.0f, 82.0f, 122.0f})
		{
			SpawnBuildPiece(
				*World,
				Location + Rotation.RotateVector(FVector(0.0f, -42.0f, Z)),
				FVector(0.82f, 0.16f, 0.25f),
				Rotation,
				FLinearColor(0.17f, 0.18f, 0.12f, 1.0f),
				TEXT("WB_PlayerBuild_PowerCell"),
				OutSpawnedActors,
				0.72f,
				0.26f);
		}
		return true;
	}

	if (BuildTypeId == PoweredLightType)
	{
		AStaticMeshActor* Pole = SpawnBuildPiece(
			*World,
			Location + FVector(0.0f, 0.0f, 150.0f),
			FVector(0.12f, 0.12f, 3.0f),
			Rotation,
			Scrap,
			TEXT("WB_PlayerBuild_LightPole"),
			OutSpawnedActors,
			0.68f,
			0.52f);
		if (!Pole)
		{
			return false;
		}
		Pole->Tags.AddUnique(PoweredLightTag);

		SpawnBuildPiece(
			*World,
			Location + FVector(0.0f, 0.0f, 302.0f),
			FVector(0.42f, 0.30f, 0.18f),
			Rotation,
			FLinearColor(0.18f, 0.18f, 0.16f, 1.0f),
			TEXT("WB_PlayerBuild_LightFixture"),
			OutSpawnedActors,
			0.68f,
			0.48f);

		APointLight* Light = World->SpawnActor<APointLight>(
			Location + FVector(0.0f, 0.0f, 295.0f),
			Rotation);
		if (Light)
		{
			Light->Tags.AddUnique(PlayerBuildTag);
			Light->Tags.AddUnique(PoweredLightTag);
#if WITH_EDITOR
			Light->SetActorLabel(TEXT("WB_PlayerBuild_PoweredLight"));
#endif
			if (UPointLightComponent* LightComponent = Cast<UPointLightComponent>(Light->GetLightComponent()))
			{
				LightComponent->SetIntensity(3200.0f);
				LightComponent->SetAttenuationRadius(1050.0f);
				LightComponent->SetLightColor(FLinearColor(0.92f, 0.78f, 0.55f));
				LightComponent->SetCastShadows(true);
				LightComponent->SetVisibility(false);
			}
			if (OutSpawnedActors)
			{
				OutSpawnedActors->Add(Light);
			}
		}
		return true;
	}

	if (BuildTypeId == PurifierType)
	{
		AStaticMeshActor* Body = SpawnBuildPiece(
			*World,
			Location + FVector(0.0f, 0.0f, 74.0f),
			FVector(1.05f, 0.78f, 1.38f),
			Rotation,
			FLinearColor(0.12f, 0.145f, 0.14f, 1.0f),
			TEXT("WB_PlayerBuild_WaterPurifier"),
			OutSpawnedActors,
			0.76f,
			0.30f);
		if (!Body)
		{
			return false;
		}
		Body->Tags.AddUnique(InteractableTag);
		Body->Tags.AddUnique(PurifierTag);

		SpawnBuildPiece(
			*World,
			Location + Rotation.RotateVector(FVector(0.0f, -48.0f, 94.0f)),
			FVector(0.68f, 0.18f, 0.86f),
			Rotation,
			FLinearColor(0.18f, 0.20f, 0.16f, 1.0f),
			TEXT("WB_PlayerBuild_PurifierFilter"),
			OutSpawnedActors,
			0.82f,
			0.16f);
		SpawnBuildPiece(
			*World,
			Location + FVector(0.0f, 0.0f, 151.0f),
			FVector(0.88f, 0.62f, 0.12f),
			Rotation,
			Scrap,
			TEXT("WB_PlayerBuild_PurifierTop"),
			OutSpawnedActors,
			0.72f,
			0.42f);
		return true;
	}

	if (BuildTypeId == HeaterType)
	{
		AStaticMeshActor* Body = SpawnBuildPiece(
			*World,
			Location + FVector(0.0f, 0.0f, 52.0f),
			FVector(0.90f, 0.58f, 1.00f),
			Rotation,
			FLinearColor(0.135f, 0.12f, 0.095f, 1.0f),
			TEXT("WB_PlayerBuild_Heater"),
			OutSpawnedActors,
			0.70f,
			0.50f);
		if (!Body)
		{
			return false;
		}
		Body->Tags.AddUnique(InteractableTag);
		Body->Tags.AddUnique(HeaterTag);

		for (float Z : {26.0f, 52.0f, 78.0f})
		{
			SpawnBuildPiece(
				*World,
				Location + Rotation.RotateVector(FVector(0.0f, -34.0f, Z)),
				FVector(0.62f, 0.08f, 0.08f),
				Rotation,
				Rust,
				TEXT("WB_PlayerBuild_HeaterElement"),
				OutSpawnedActors,
				0.64f,
				0.48f);
		}
		return true;
	}

	if (BuildTypeId == ToolStationType)
	{
		AStaticMeshActor* Bench = SpawnBuildPiece(
			*World,
			Location + FVector(0.0f, 0.0f, 78.0f),
			FVector(1.55f, 0.72f, 0.14f),
			Rotation,
			DarkWood,
			TEXT("WB_PlayerBuild_PoweredToolStation"),
			OutSpawnedActors,
			0.80f,
			0.22f);
		if (!Bench)
		{
			return false;
		}
		Bench->Tags.AddUnique(InteractableTag);
		Bench->Tags.AddUnique(ToolStationTag);

		const FVector Right = Rotation.RotateVector(FVector(0.0f, 1.0f, 0.0f));
		SpawnBuildPiece(
			*World,
			Location + Right * 58.0f + FVector(0.0f, 0.0f, 122.0f),
			FVector(0.84f, 0.16f, 0.72f),
			Rotation,
			FLinearColor(0.13f, 0.14f, 0.12f, 1.0f),
			TEXT("WB_PlayerBuild_ToolStationMotor"),
			OutSpawnedActors,
			0.72f,
			0.44f);
		SpawnBuildPiece(
			*World,
			Location - Right * 72.0f + FVector(0.0f, 0.0f, 116.0f),
			FVector(0.56f, 0.20f, 0.54f),
			Rotation,
			Scrap,
			TEXT("WB_PlayerBuild_ToolStationPress"),
			OutSpawnedActors,
			0.68f,
			0.48f);
		return true;
	}

	if (BuildTypeId == GeneratorType)
	{
		AStaticMeshActor* Body = SpawnBuildPiece(
			*World,
			Location + FVector(0.0f, 0.0f, 62.0f),
			FVector(1.35f, 0.85f, 1.15f),
			Rotation,
			FLinearColor(0.12f, 0.13f, 0.105f, 1.0f),
			TEXT("WB_PlayerBuild_Generator"),
			OutSpawnedActors,
			0.74f,
			0.46f);
		if (!Body)
		{
			return false;
		}
		Body->Tags.AddUnique(InteractableTag);
		Body->Tags.AddUnique(GeneratorTag);

		SpawnBuildPiece(
			*World,
			Location + Rotation.RotateVector(FVector(-72.0f, 0.0f, 64.0f)),
			FVector(0.42f, 0.66f, 0.74f),
			Rotation,
			Rust,
			TEXT("WB_PlayerBuild_GeneratorFuelTank"),
			OutSpawnedActors,
			0.76f,
			0.36f);

		SpawnBuildPiece(
			*World,
			Location + Rotation.RotateVector(FVector(48.0f, 0.0f, 126.0f)),
			FVector(0.12f, 0.12f, 0.88f),
			Rotation,
			Scrap,
			TEXT("WB_PlayerBuild_GeneratorExhaust"),
			OutSpawnedActors,
			0.70f,
			0.52f);
		return true;
	}

	if (BuildTypeId == WorkbenchType)
	{
		AStaticMeshActor* Top = SpawnBuildPiece(
			*World,
			Location + FVector(0.0f, 0.0f, 82.0f),
			FVector(1.80f, 0.70f, 0.12f),
			Rotation,
			Wood,
			TEXT("WB_PlayerBuild_WorkbenchTop"),
			OutSpawnedActors);
		if (!Top)
		{
			return false;
		}
		Top->Tags.AddUnique(WorkbenchTag);

		const FVector Right = Rotation.RotateVector(FVector(0.0f, 1.0f, 0.0f));
		for (float XSide : {-135.0f, 135.0f})
		{
			for (float YSide : {-42.0f, 42.0f})
			{
				SpawnBuildPiece(
					*World,
					Location + Forward * XSide + Right * YSide + FVector(0.0f, 0.0f, 38.0f),
					FVector(0.11f, 0.11f, 0.76f),
					Rotation,
					Scrap,
					TEXT("WB_PlayerBuild_WorkbenchLeg"),
					OutSpawnedActors,
					0.70f,
					0.44f);
			}
		}
		SpawnBuildPiece(
			*World,
			Location + FVector(0.0f, 0.0f, 40.0f),
			FVector(1.48f, 0.50f, 0.08f),
			Rotation,
			DarkWood,
			TEXT("WB_PlayerBuild_WorkbenchShelf"),
			OutSpawnedActors);
		SpawnBuildPiece(
			*World,
			Location + Right * 62.0f + FVector(0.0f, 0.0f, 150.0f),
			FVector(1.65f, 0.07f, 0.62f),
			Rotation,
			Rust,
			TEXT("WB_PlayerBuild_WorkbenchBack"),
			OutSpawnedActors,
			0.78f,
			0.26f);
		return true;
	}

	return false;
}

void UWildBoundBuildingSubsystem::RestorePlacedBuilds(
	const TArray<FWildBoundPlacedBuildState>& SavedBuilds)
{
	CancelPlacement(false);
	DestroyAllPlacedBuildActors();
	PlacedBuilds.Reset();
	BuildIdByActor.Reset();
	NextBuildId = 1;

	for (const FWildBoundPlacedBuildState& Saved : SavedBuilds)
	{
		if (Saved.BuildTypeId.IsNone())
		{
			continue;
		}

		FWildBoundPlacedBuildState Restored = Saved;
		if (Restored.BuildId <= 0)
		{
			Restored.BuildId = NextBuildId;
		}

		TArray<AActor*> SpawnedActors;
		if (SpawnPlacedBuild(
			Restored.BuildTypeId,
			Restored.Location,
			Restored.Rotation,
			&SpawnedActors))
		{
			RegisterBuildActors(Restored.BuildId, SpawnedActors);
			PlacedBuilds.Add(Restored);
			NextBuildId = FMath::Max(NextBuildId, Restored.BuildId + 1);
		}
	}
	RefreshPowerLinkVisuals();
	RefreshPoweredLights();
}

void UWildBoundBuildingSubsystem::RegisterBuildActors(
	int32 BuildId,
	const TArray<AActor*>& Actors)
{
	if (BuildId <= 0)
	{
		return;
	}

	for (AActor* Actor : Actors)
	{
		if (IsValid(Actor))
		{
			BuildIdByActor.Add(TWeakObjectPtr<AActor>(Actor), BuildId);
		}
	}
}

void UWildBoundBuildingSubsystem::DestroyBuildActors(int32 BuildId)
{
	if (BuildId <= 0)
	{
		return;
	}

	TArray<TWeakObjectPtr<AActor>> KeysToRemove;
	TArray<AActor*> ActorsToDestroy;
	for (const TPair<TWeakObjectPtr<AActor>, int32>& Pair : BuildIdByActor)
	{
		if (Pair.Value != BuildId)
		{
			continue;
		}

		KeysToRemove.Add(Pair.Key);
		if (AActor* Actor = Pair.Key.Get())
		{
			ActorsToDestroy.AddUnique(Actor);
		}
	}

	for (AActor* Actor : ActorsToDestroy)
	{
		if (IsValid(Actor))
		{
			Actor->Destroy();
		}
	}

	for (const TWeakObjectPtr<AActor>& Key : KeysToRemove)
	{
		BuildIdByActor.Remove(Key);
	}
}

void UWildBoundBuildingSubsystem::DestroyAllPlacedBuildActors()
{
	UWorld* World = GetWorld();
	if (!World)
	{
		BuildIdByActor.Reset();
		return;
	}

	TArray<AActor*> ToDestroy;
	for (TActorIterator<AActor> It(World); It; ++It)
	{
		AActor* Actor = *It;
		if (Actor && Actor->ActorHasTag(PlayerBuildTag))
		{
			ToDestroy.Add(Actor);
		}
	}

	for (AActor* Actor : ToDestroy)
	{
		if (IsValid(Actor))
		{
			Actor->Destroy();
		}
	}

	TArray<AActor*> LinkVisuals;
	for (TActorIterator<AActor> It(World); It; ++It)
	{
		AActor* Actor = *It;
		if (Actor && Actor->ActorHasTag(PowerLinkVisualTag))
		{
			LinkVisuals.Add(Actor);
		}
	}
	for (AActor* Actor : LinkVisuals)
	{
		if (IsValid(Actor))
		{
			Actor->Destroy();
		}
	}

	BuildIdByActor.Reset();
}

int32 UWildBoundBuildingSubsystem::FindBuildIdForActor(const AActor* Actor) const
{
	if (!Actor)
	{
		return 0;
	}

	if (const int32* BuildId = BuildIdByActor.Find(
		TWeakObjectPtr<AActor>(const_cast<AActor*>(Actor))))
	{
		return *BuildId;
	}

	return 0;
}

FWildBoundPlacedBuildState* UWildBoundBuildingSubsystem::FindBuildState(int32 BuildId)
{
	return PlacedBuilds.FindByPredicate(
		[BuildId](const FWildBoundPlacedBuildState& State)
		{
			return State.BuildId == BuildId;
		});
}

const FWildBoundPlacedBuildState* UWildBoundBuildingSubsystem::FindBuildState(int32 BuildId) const
{
	return PlacedBuilds.FindByPredicate(
		[BuildId](const FWildBoundPlacedBuildState& State)
		{
			return State.BuildId == BuildId;
		});
}

void UWildBoundBuildingSubsystem::UpdateUtilities()
{
	bool bUtilityStateChanged = false;

	for (FWildBoundPlacedBuildState& State : PlacedBuilds)
	{
		if (State.BuildTypeId == RainCollectorType
			&& State.bUtilityEnabled
			&& State.StoredUtilityUnits < RainCollectorCapacity)
		{
			State.UtilityProgress += 1.0f / RainWaterSecondsPerUnit;
			while (State.UtilityProgress >= 1.0f
				&& State.StoredUtilityUnits < RainCollectorCapacity)
			{
				State.UtilityProgress -= 1.0f;
				++State.StoredUtilityUnits;
				bUtilityStateChanged = true;
			}
			if (State.StoredUtilityUnits >= RainCollectorCapacity)
			{
				State.UtilityProgress = 0.0f;
			}
		}

		if (State.BuildTypeId == GeneratorType
			&& State.bUtilityEnabled
			&& State.FuelSecondsRemaining > 0.0f)
		{
			State.FuelSecondsRemaining = FMath::Max(0.0f, State.FuelSecondsRemaining - 1.0f);
			if (State.FuelSecondsRemaining <= 0.0f)
			{
				State.bUtilityEnabled = false;
				bUtilityStateChanged = true;
			}

			for (int32 LinkedId : State.LinkedBuildIds)
			{
				FWildBoundPlacedBuildState* Battery = FindBuildState(LinkedId);
				if (!Battery || Battery->BuildTypeId != PowerBankType)
				{
					continue;
				}
				Battery->StoredPower = FMath::Clamp(
					Battery->StoredPower + GeneratorChargePerSecond,
					0.0f,
					BatteryCapacity);
			}
		}
	}

	for (FWildBoundPlacedBuildState& State : PlacedBuilds)
	{
		if (State.BuildTypeId != PurifierType
			|| !State.bUtilityEnabled
			|| State.StoredUtilityUnits >= PurifierCapacity
			|| !IsBuildPowered(State.BuildId))
		{
			continue;
		}

		FWildBoundPlacedBuildState* SourceCollector = nullptr;
		for (FWildBoundPlacedBuildState& Candidate : PlacedBuilds)
		{
			if (Candidate.BuildTypeId == RainCollectorType
				&& Candidate.StoredUtilityUnits > 0
				&& FVector::DistSquared2D(Candidate.Location, State.Location) <= FMath::Square(ShelterUpgradeRadius))
			{
				SourceCollector = &Candidate;
				break;
			}
		}

		if (!SourceCollector)
		{
			State.UtilityProgress = 0.0f;
			continue;
		}

		State.UtilityProgress += 1.0f / PurifierSecondsPerUnit;
		if (State.UtilityProgress >= 1.0f)
		{
			State.UtilityProgress = 0.0f;
			--SourceCollector->StoredUtilityUnits;
			++State.StoredUtilityUnits;
			bUtilityStateChanged = true;
		}
	}

	for (FWildBoundPlacedBuildState& State : PlacedBuilds)
	{
		if (State.BuildTypeId != PowerBankType
			|| !State.bUtilityEnabled
			|| State.StoredPower <= 0.0f)
		{
			continue;
		}

		const float Load = GetConnectedLoadForBattery(State.BuildId);
		if (Load > 0.0f)
		{
			State.StoredPower = FMath::Max(0.0f, State.StoredPower - Load);
		}
	}

	RefreshPoweredLights();

	if (bUtilityStateChanged && GEngine)
	{
		GEngine->AddOnScreenDebugMessage(
			91708,
			1.4f,
			FColor(125, 175, 205),
			TEXT("BASE UTILITIES UPDATED"));
	}
}

void UWildBoundBuildingSubsystem::RefreshPoweredLights()
{
	for (const TPair<TWeakObjectPtr<AActor>, int32>& Pair : BuildIdByActor)
	{
		APointLight* Light = Cast<APointLight>(Pair.Key.Get());
		if (!Light || !Light->ActorHasTag(PoweredLightTag))
		{
			continue;
		}

		const FWildBoundPlacedBuildState* State = FindBuildState(Pair.Value);
		const bool bPowered = State && IsPowerAvailableAt(State->Location);
		if (UPointLightComponent* LightComponent = Cast<UPointLightComponent>(Light->GetLightComponent()))
		{
			LightComponent->SetVisibility(bPowered);
		}
	}
}

void UWildBoundBuildingSubsystem::RefreshPowerLinkVisuals()
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	TArray<AActor*> OldLinks;
	for (TActorIterator<AActor> It(World); It; ++It)
	{
		AActor* Actor = *It;
		if (Actor && Actor->ActorHasTag(PowerLinkVisualTag))
		{
			OldLinks.Add(Actor);
		}
	}
	for (AActor* Actor : OldLinks)
	{
		if (IsValid(Actor))
		{
			Actor->Destroy();
		}
	}

	for (const FWildBoundPlacedBuildState& Source : PlacedBuilds)
	{
		if (!IsElectricalBuild(Source.BuildTypeId))
		{
			continue;
		}

		for (int32 TargetId : Source.LinkedBuildIds)
		{
			const FWildBoundPlacedBuildState* Target = FindBuildState(TargetId);
			if (!Target)
			{
				continue;
			}

			SpawnPowerLinkVisual(
				*World,
				Source.Location + FVector(0.0f, 0.0f, 95.0f),
				Target->Location + FVector(0.0f, 0.0f, 95.0f));
		}
	}
}

bool UWildBoundBuildingSubsystem::IsElectricalBuild(FName BuildTypeId) const
{
	return BuildTypeId == GeneratorType
		|| BuildTypeId == PowerBankType
		|| BuildTypeId == PoweredLightType
		|| BuildTypeId == PurifierType
		|| BuildTypeId == HeaterType
		|| BuildTypeId == ToolStationType;
}

bool UWildBoundBuildingSubsystem::CanLinkPowerBuilds(int32 SourceBuildId, int32 TargetBuildId) const
{
	const FWildBoundPlacedBuildState* Source = FindBuildState(SourceBuildId);
	const FWildBoundPlacedBuildState* Target = FindBuildState(TargetBuildId);
	if (!Source || !Target || SourceBuildId == TargetBuildId)
	{
		return false;
	}

	if (FVector::DistSquared2D(Source->Location, Target->Location) > FMath::Square(UtilityPowerRadius))
	{
		return false;
	}

	const bool bBatteryLoad =
		Target->BuildTypeId == PoweredLightType
		|| Target->BuildTypeId == PurifierType
		|| Target->BuildTypeId == HeaterType
		|| Target->BuildTypeId == ToolStationType;

	return (Source->BuildTypeId == GeneratorType && Target->BuildTypeId == PowerBankType)
		|| (Source->BuildTypeId == PowerBankType && bBatteryLoad);
}

void UWildBoundBuildingSubsystem::TogglePowerLink(int32 SourceBuildId, int32 TargetBuildId)
{
	FWildBoundPlacedBuildState* Source = FindBuildState(SourceBuildId);
	if (!Source || !CanLinkPowerBuilds(SourceBuildId, TargetBuildId))
	{
		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(
				91712,
				2.0f,
				FColor(225, 145, 105),
				TEXT("Invalid power link. Generator -> Battery Bank -> powered utility."));
		}
		return;
	}

	if (Source->LinkedBuildIds.Contains(TargetBuildId))
	{
		Source->LinkedBuildIds.Remove(TargetBuildId);
	}
	else
	{
		Source->LinkedBuildIds.AddUnique(TargetBuildId);
	}

	RefreshPowerLinkVisuals();
	RefreshPoweredLights();
}

float UWildBoundBuildingSubsystem::GetConnectedLoadForBattery(int32 BatteryBuildId) const
{
	const FWildBoundPlacedBuildState* Battery = FindBuildState(BatteryBuildId);
	if (!Battery || Battery->BuildTypeId != PowerBankType)
	{
		return 0.0f;
	}

	float Load = 0.0f;
	for (int32 LinkedId : Battery->LinkedBuildIds)
	{
		const FWildBoundPlacedBuildState* Target = FindBuildState(LinkedId);
		if (!Target || !Target->bUtilityEnabled)
		{
			continue;
		}

		if (Target->BuildTypeId == PoweredLightType)
		{
			Load += PoweredLightLoadPerSecond;
		}
		else if (Target->BuildTypeId == PurifierType)
		{
			Load += PurifierLoadPerSecond;
		}
		else if (Target->BuildTypeId == HeaterType)
		{
			Load += HeaterLoadPerSecond;
		}
		else if (Target->BuildTypeId == ToolStationType)
		{
			Load += ToolStationLoadPerSecond;
		}
	}
	return Load;
}

bool UWildBoundBuildingSubsystem::IsBuildPowered(int32 BuildId) const
{
	const FWildBoundPlacedBuildState* Target = FindBuildState(BuildId);
	if (!Target || !Target->bUtilityEnabled)
	{
		return false;
	}

	for (const FWildBoundPlacedBuildState& Battery : PlacedBuilds)
	{
		if (Battery.BuildTypeId == PowerBankType
			&& Battery.bUtilityEnabled
			&& Battery.StoredPower > 0.0f
			&& Battery.LinkedBuildIds.Contains(BuildId))
		{
			return true;
		}
	}
	return false;
}

bool UWildBoundBuildingSubsystem::IsPowerAvailableAt(const FVector& Location) const
{
	for (const FWildBoundPlacedBuildState& State : PlacedBuilds)
	{
		if (!IsElectricalBuild(State.BuildTypeId)
			|| State.BuildTypeId == GeneratorType
			|| State.BuildTypeId == PowerBankType)
		{
			continue;
		}

		if (FVector::DistSquared2D(State.Location, Location) <= FMath::Square(80.0f)
			&& IsBuildPowered(State.BuildId))
		{
			return true;
		}
	}
	return false;
}

FString UWildBoundBuildingSubsystem::GetUtilityInteractionPrompt(const AActor* Actor) const
{
	const int32 BuildId = FindBuildIdForActor(Actor);
	const FWildBoundPlacedBuildState* State = FindBuildState(BuildId);
	if (!State)
	{
		return FString();
	}

	if (State->BuildTypeId == RainCollectorType)
	{
		if (State->StoredUtilityUnits > 0)
		{
			return FString::Printf(TEXT("Collect rainwater x%d"), State->StoredUtilityUnits);
		}
		return FString::Printf(
			TEXT("Rain collector - %d%%"),
			FMath::RoundToInt(FMath::Clamp(State->UtilityProgress, 0.0f, 1.0f) * 100.0f));
	}

	if (State->BuildTypeId == GeneratorType)
	{
		return FString::Printf(
			TEXT("Generator %s - fuel %.0fs | Shift+E refuel"),
			State->bUtilityEnabled ? TEXT("ON") : TEXT("OFF"),
			State->FuelSecondsRemaining);
	}

	if (State->BuildTypeId == PowerBankType)
	{
		return FString::Printf(
			TEXT("Battery %s - %.0f%% - load %.2f/s"),
			State->bUtilityEnabled ? TEXT("ON") : TEXT("OFF"),
			State->StoredPower,
			GetConnectedLoadForBattery(State->BuildId));
	}

	if (State->BuildTypeId == PurifierType)
	{
		if (State->StoredUtilityUnits > 0)
		{
			return FString::Printf(TEXT("Collect purified water x%d | Shift+E toggle"), State->StoredUtilityUnits);
		}
		return FString::Printf(
			TEXT("Purifier %s - %d%% | Shift+E toggle"),
			State->bUtilityEnabled ? TEXT("ON") : TEXT("OFF"),
			FMath::RoundToInt(FMath::Clamp(State->UtilityProgress, 0.0f, 1.0f) * 100.0f));
	}

	if (State->BuildTypeId == HeaterType)
	{
		return FString::Printf(
			TEXT("Heater %s - %s"),
			State->bUtilityEnabled ? TEXT("ON") : TEXT("OFF"),
			IsBuildPowered(State->BuildId) ? TEXT("POWERED") : TEXT("NO POWER"));
	}

	if (State->BuildTypeId == ToolStationType)
	{
		return FString::Printf(
			TEXT("Powered tools %s - %s"),
			State->bUtilityEnabled ? TEXT("ON") : TEXT("OFF"),
			IsBuildPowered(State->BuildId) ? TEXT("POWERED") : TEXT("NO POWER"));
	}

	return FString();
}

bool UWildBoundBuildingSubsystem::TryUseUtility(AActor* Actor)
{
	const int32 BuildId = FindBuildIdForActor(Actor);
	FWildBoundPlacedBuildState* State = FindBuildState(BuildId);
	if (!State)
	{
		return false;
	}

	if (State->BuildTypeId == RainCollectorType)
	{
		UWildBoundInventoryComponent* Inventory = GetPlayerInventory();
		if (!Inventory)
		{
			return true;
		}

		if (State->StoredUtilityUnits <= 0)
		{
			if (GEngine)
			{
				GEngine->AddOnScreenDebugMessage(91709, 1.8f, FColor(135, 175, 200), TEXT("Rain collector is still filling."));
			}
			return true;
		}

		int32 Collected = 0;
		while (State->StoredUtilityUnits > 0)
		{
			if (!Inventory->AddItem(FName(TEXT("Water")), 1))
			{
				break;
			}
			--State->StoredUtilityUnits;
			++Collected;
		}

		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(
				91709,
				2.0f,
				Collected > 0 ? FColor(135, 190, 220) : FColor(220, 155, 105),
				Collected > 0 ? FString::Printf(TEXT("COLLECTED WATER x%d"), Collected)
					: TEXT("Not enough backpack capacity for collected water."));
		}
		return true;
	}

	if (State->BuildTypeId == GeneratorType)
	{
		UWildBoundInventoryComponent* Inventory = GetPlayerInventory();
		APlayerController* PlayerController = GetWorld() ? GetWorld()->GetFirstPlayerController() : nullptr;
		const bool bRefuel = PlayerController
			&& (PlayerController->IsInputKeyDown(EKeys::LeftShift)
				|| PlayerController->IsInputKeyDown(EKeys::RightShift));

		if (bRefuel)
		{
			if (Inventory && Inventory->RemoveItem(FName(TEXT("Fuel")), 1))
			{
				State->FuelSecondsRemaining += GeneratorFuelSecondsPerCan;
				State->bUtilityEnabled = true;
				if (GEngine)
				{
					GEngine->AddOnScreenDebugMessage(91709, 2.0f, FColor(195, 190, 120), TEXT("GENERATOR REFUELED"));
				}
			}
			else if (GEngine)
			{
				GEngine->AddOnScreenDebugMessage(91709, 2.0f, FColor(225, 145, 105), TEXT("You need a Fuel Can."));
			}
			return true;
		}

		if (State->FuelSecondsRemaining <= 0.0f)
		{
			if (GEngine)
			{
				GEngine->AddOnScreenDebugMessage(91709, 2.0f, FColor(225, 145, 105), TEXT("Generator is empty. Hold Shift and press E with a Fuel Can."));
			}
			return true;
		}

		State->bUtilityEnabled = !State->bUtilityEnabled;
		RefreshPoweredLights();
		return true;
	}

	if (State->BuildTypeId == PowerBankType)
	{
		State->bUtilityEnabled = !State->bUtilityEnabled;
		RefreshPoweredLights();
		return true;
	}

	return false;
}

int32 UWildBoundBuildingSubsystem::GetShelterProgressionTierAt(const FVector& Location) const
{
	bool bHasWater = false;
	bool bHasPower = false;
	bool bHasPoweredLight = false;
	int32 ReinforcedPieces = 0;

	for (const FWildBoundPlacedBuildState& State : PlacedBuilds)
	{
		if (FVector::DistSquared2D(State.Location, Location) > FMath::Square(ShelterUpgradeRadius))
		{
			continue;
		}

		if (State.BuildTypeId == RainCollectorType)
		{
			bHasWater = true;
		}
		else if (State.BuildTypeId == PowerBankType && State.bUtilityEnabled && State.StoredPower > 0.0f)
		{
			bHasPower = true;
		}
		else if (State.BuildTypeId == PoweredLightType && IsPowerAvailableAt(State.Location))
		{
			bHasPoweredLight = true;
		}
		else if (State.BuildTypeId == ReinforcedFloorType || State.BuildTypeId == ReinforcedWallType)
		{
			++ReinforcedPieces;
		}
	}

	int32 Tier = 0;
	if (bHasWater)
	{
		Tier = 1;
	}
	if (Tier >= 1 && bHasPower && bHasPoweredLight)
	{
		Tier = 2;
	}
	if (Tier >= 2 && ReinforcedPieces >= 2)
	{
		Tier = 3;
	}
	return Tier;
}

float UWildBoundBuildingSubsystem::GetRestHealthRecoveryAt(const FVector& Location) const
{
	return 12.0f + static_cast<float>(GetShelterProgressionTierAt(Location)) * 4.0f;
}

FString UWildBoundBuildingSubsystem::GetShelterProgressionNameAt(const FVector& Location) const
{
	switch (GetShelterProgressionTierAt(Location))
	{
	case 3: return TEXT("REINFORCED SAFEHOUSE");
	case 2: return TEXT("POWERED SHELTER");
	case 1: return TEXT("WATER-SECURED SHELTER");
	default: return TEXT("FIELD SHELTER");
	}
}

FVector UWildBoundBuildingSubsystem::GetSafehouseAnchorLocation() const
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return FVector::ZeroVector;
	}

	for (TActorIterator<AActor> It(World); It; ++It)
	{
		AActor* Actor = *It;
		if (Actor && Actor->ActorHasTag(SafehouseSpawnAnchorTag))
		{
			return Actor->GetActorLocation();
		}
	}
	return FVector::ZeroVector;
}

bool UWildBoundBuildingSubsystem::IsPlayerNearSafehouseStatus() const
{
	UWorld* World = GetWorld();
	APlayerController* PlayerController = World ? World->GetFirstPlayerController() : nullptr;
	APawn* Pawn = PlayerController ? PlayerController->GetPawn() : nullptr;
	const FVector Anchor = GetSafehouseAnchorLocation();
	return Pawn
		&& !Anchor.IsNearlyZero()
		&& FVector::DistSquared2D(Pawn->GetActorLocation(), Anchor) <= FMath::Square(1200.0f);
}

FString UWildBoundBuildingSubsystem::GetSafehouseStatusText() const
{
	const FVector Anchor = GetSafehouseAnchorLocation();
	int32 StoredWater = 0;
	int32 ReinforcedPieces = 0;
	float BatteryCharge = 0.0f;
	int32 BatteryCount = 0;
	float TotalLoad = 0.0f;
	float GeneratorFuel = 0.0f;
	bool bGeneratorRunning = false;
	int32 PoweredLights = 0;

	for (const FWildBoundPlacedBuildState& State : PlacedBuilds)
	{
		if (!Anchor.IsNearlyZero()
			&& FVector::DistSquared2D(State.Location, Anchor) > FMath::Square(ShelterUpgradeRadius))
		{
			continue;
		}

		if (State.BuildTypeId == RainCollectorType)
		{
			StoredWater += State.StoredUtilityUnits;
		}
		else if (State.BuildTypeId == ReinforcedFloorType || State.BuildTypeId == ReinforcedWallType)
		{
			++ReinforcedPieces;
		}
		else if (State.BuildTypeId == PowerBankType)
		{
			BatteryCharge += State.StoredPower;
			++BatteryCount;
			TotalLoad += State.bUtilityEnabled ? GetConnectedLoadForBattery(State.BuildId) : 0.0f;
		}
		else if (State.BuildTypeId == GeneratorType)
		{
			GeneratorFuel += State.FuelSecondsRemaining;
			bGeneratorRunning |= State.bUtilityEnabled && State.FuelSecondsRemaining > 0.0f;
		}
		else if (State.BuildTypeId == PoweredLightType && IsPowerAvailableAt(State.Location))
		{
			++PoweredLights;
		}
	}

	return FString::Printf(
		TEXT("%s\nWATER  %d STORED\nPOWER  %.0f / %.0f  LOAD %.2f/s\nGENERATOR  %s  FUEL %.0fs\nLIGHTS  %d ONLINE\nREINFORCEMENT  %d PIECES"),
		*GetShelterProgressionNameAt(Anchor),
		StoredWater,
		BatteryCharge,
		static_cast<float>(BatteryCount) * BatteryCapacity,
		TotalLoad,
		bGeneratorRunning ? TEXT("RUNNING") : TEXT("OFF"),
		GeneratorFuel,
		PoweredLights,
		ReinforcedPieces);
}

void UWildBoundBuildingSubsystem::UpdateManagementMode()
{
	UWorld* World = GetWorld();
	APlayerController* PlayerController = World ? World->GetFirstPlayerController() : nullptr;
	APawn* Pawn = PlayerController ? PlayerController->GetPawn() : nullptr;
	if (!World || !PlayerController || !Pawn || PlayerController->bShowMouseCursor)
	{
		PendingDismantleBuildId = 0;
		DismantleHoldStartedAt = -1.0f;
		return;
	}

	FVector ViewLocation;
	FRotator ViewRotation;
	PlayerController->GetPlayerViewPoint(ViewLocation, ViewRotation);

	FCollisionQueryParams Params(SCENE_QUERY_STAT(WildBoundBuildManageTrace), false, Pawn);
	FHitResult Hit;
	const bool bHit = World->LineTraceSingleByChannel(
		Hit,
		ViewLocation,
		ViewLocation + ViewRotation.Vector() * BuildManagementDistance,
		ECC_Visibility,
		Params);

	AActor* TargetActor = bHit ? Hit.GetActor() : nullptr;
	const int32 BuildId = FindBuildIdForActor(TargetActor);
	const FWildBoundPlacedBuildState* State = FindBuildState(BuildId);
	if (!State)
	{
		PendingDismantleBuildId = 0;
		DismantleHoldStartedAt = -1.0f;
		return;
	}

	UWildBoundInteractionComponent* Interaction =
		Pawn->FindComponentByClass<UWildBoundInteractionComponent>();
	const FString BuildName = GetBuildDisplayName(State->BuildTypeId);

	FString UseHint;
	const FString UtilityPrompt = GetUtilityInteractionPrompt(TargetActor);
	if (!UtilityPrompt.IsEmpty())
	{
		UseHint = FString::Printf(TEXT("[E] %s   |   "), *UtilityPrompt);
	}
	else if (State->BuildTypeId == StorageType || State->BuildTypeId == CotType)
	{
		UseHint = TEXT("[E] USE   |   ");
	}
	else if (State->BuildTypeId == WorkbenchType)
	{
		UseHint = TEXT("[C] CRAFT/BUILD   |   ");
	}

	FString LinkHint;
	if (IsElectricalBuild(State->BuildTypeId))
	{
		if (PendingPowerLinkSourceBuildId == 0)
		{
			if (State->BuildTypeId == GeneratorType || State->BuildTypeId == PowerBankType)
			{
				LinkHint = TEXT("[L] START LINK  [SHIFT+L] CLEAR LINKS   |   ");
			}
		}
		else
		{
			const FWildBoundPlacedBuildState* Source = FindBuildState(PendingPowerLinkSourceBuildId);
			if (Source)
			{
				LinkHint = FString::Printf(
					TEXT("[L] LINK FROM %s   |   "),
					*GetBuildDisplayName(Source->BuildTypeId));
			}
		}
	}

	float HoldProgress = 0.0f;
	if (PendingDismantleBuildId == BuildId
		&& DismantleHoldStartedAt >= 0.0f
		&& PlayerController->IsInputKeyDown(EKeys::X))
	{
		HoldProgress = FMath::Clamp(
			(World->GetTimeSeconds() - DismantleHoldStartedAt) / DismantleHoldDuration,
			0.0f,
			1.0f);
	}

	if (Interaction)
	{
		const FString DismantleHint = HoldProgress > 0.0f
			? FString::Printf(
				TEXT("HOLD [X] DISMANTLE %d%%"),
				FMath::RoundToInt(HoldProgress * 100.0f))
			: TEXT("HOLD [X] DISMANTLE / 60% REFUND");

		Interaction->SetContextPrompt(
			FString::Printf(
				TEXT("%s%s%s   |   [R] RELOCATE   |   %s"),
				*UseHint,
				*LinkHint,
				*BuildName,
				*DismantleHint),
			35);
	}

	if (PlayerController->WasInputKeyJustPressed(EKeys::L) && IsElectricalBuild(State->BuildTypeId))
	{
		const bool bShift = PlayerController->IsInputKeyDown(EKeys::LeftShift)
			|| PlayerController->IsInputKeyDown(EKeys::RightShift);

		if (bShift)
		{
			if (FWildBoundPlacedBuildState* Mutable = FindBuildState(BuildId))
			{
				Mutable->LinkedBuildIds.Reset();
				PendingPowerLinkSourceBuildId = 0;
				RefreshPowerLinkVisuals();
				RefreshPoweredLights();
			}
			return;
		}

		if (PendingPowerLinkSourceBuildId == 0)
		{
			if (State->BuildTypeId == GeneratorType || State->BuildTypeId == PowerBankType)
			{
				PendingPowerLinkSourceBuildId = BuildId;
				if (GEngine)
				{
					GEngine->AddOnScreenDebugMessage(
						91712,
						2.0f,
						FColor(175, 205, 150),
						TEXT("POWER LINK STARTED   |   aim at a compatible target and press L"));
				}
			}
		}
		else if (PendingPowerLinkSourceBuildId == BuildId)
		{
			PendingPowerLinkSourceBuildId = 0;
		}
		else
		{
			const int32 SourceId = PendingPowerLinkSourceBuildId;
			PendingPowerLinkSourceBuildId = 0;
			TogglePowerLink(SourceId, BuildId);
		}
		return;
	}

	if (PlayerController->WasInputKeyJustPressed(EKeys::R))
	{
		PendingDismantleBuildId = 0;
		DismantleHoldStartedAt = -1.0f;
		BeginRelocation(BuildId);
		return;
	}

	if (PlayerController->WasInputKeyJustPressed(EKeys::X))
	{
		PendingDismantleBuildId = BuildId;
		DismantleHoldStartedAt = World->GetTimeSeconds();
	}

	if (PendingDismantleBuildId == BuildId)
	{
		if (!PlayerController->IsInputKeyDown(EKeys::X))
		{
			PendingDismantleBuildId = 0;
			DismantleHoldStartedAt = -1.0f;
		}
		else if (DismantleHoldStartedAt >= 0.0f
			&& World->GetTimeSeconds() - DismantleHoldStartedAt >= DismantleHoldDuration)
		{
			PendingDismantleBuildId = 0;
			DismantleHoldStartedAt = -1.0f;
			DismantleBuild(BuildId);
		}
	}
	else if (PendingDismantleBuildId != 0)
	{
		PendingDismantleBuildId = 0;
		DismantleHoldStartedAt = -1.0f;
	}
}

bool UWildBoundBuildingSubsystem::CanManageBuild(
	int32 BuildId,
	bool bShowMessage) const
{
	const FWildBoundPlacedBuildState* State = FindBuildState(BuildId);
	if (!State)
	{
		return false;
	}

	if (State->BuildTypeId != StorageType)
	{
		return true;
	}

	UWorld* World = GetWorld();
	APlayerController* PlayerController = World ? World->GetFirstPlayerController() : nullptr;
	APawn* Pawn = PlayerController ? PlayerController->GetPawn() : nullptr;
	const UWildBoundInteractionComponent* Interaction =
		Pawn ? Pawn->FindComponentByClass<UWildBoundInteractionComponent>() : nullptr;
	if (!Interaction)
	{
		return false;
	}

	for (const TPair<TWeakObjectPtr<AActor>, int32>& Pair : BuildIdByActor)
	{
		AActor* Actor = Pair.Key.Get();
		if (Pair.Value == BuildId
			&& Actor
			&& Actor->ActorHasTag(StorageTag)
			&& Interaction->HasStoredItemsForActor(Actor))
		{
			if (bShowMessage && GEngine)
			{
				GEngine->AddOnScreenDebugMessage(
					91706,
					2.3f,
					FColor(225, 155, 105),
					TEXT("Empty this storage crate before moving or dismantling it."));
			}
			return false;
		}
	}

	return true;
}

void UWildBoundBuildingSubsystem::BeginRelocation(int32 BuildId)
{
	if (bPlacementActive || !CanManageBuild(BuildId, true))
	{
		return;
	}

	FWildBoundPlacedBuildState* State = FindBuildState(BuildId);
	if (!State)
	{
		return;
	}

	RelocationOriginalState = *State;
	RelocatingBuildId = BuildId;
	bRelocatingBuild = true;
	ActiveBuildTypeId = State->BuildTypeId;
	ActiveDisplayName = GetBuildDisplayName(State->BuildTypeId);
	ActiveMaterialCosts = GetMaterialCostsForBuild(State->BuildTypeId);
	CurrentYaw = State->Rotation.Yaw;
	bGridSnapEnabled = true;
	bPieceSnapped = false;
	bPlacementActive = true;
	bPlacementValid = false;
	PlacementStartedAt = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0f;

	DestroyBuildActors(BuildId);
	EnsurePreviewActor();
	UpdatePreviewTransform();

	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(
			91706,
			2.0f,
			FColor(180, 205, 160),
			TEXT("RELOCATE MODE   |   cancel restores the original structure"));
	}
}

void UWildBoundBuildingSubsystem::DismantleBuild(int32 BuildId)
{
	if (bPlacementActive || !CanManageBuild(BuildId, true))
	{
		return;
	}

	const FWildBoundPlacedBuildState* State = FindBuildState(BuildId);
	UWildBoundInventoryComponent* Inventory = GetPlayerInventory();
	if (!State || !Inventory)
	{
		return;
	}

	const FName BuildTypeId = State->BuildTypeId;
	const FString BuildName = GetBuildDisplayName(BuildTypeId);

	if (BuildTypeId == RainCollectorType && State->StoredUtilityUnits > 0)
	{
		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(
				91706,
				2.2f,
				FColor(135, 180, 205),
				TEXT("Collect the stored rainwater before dismantling this collector."));
		}
		return;
	}

	const TMap<FName, int32> FullCosts = GetMaterialCostsForBuild(BuildTypeId);

	TArray<TPair<FName, int32>> Refunds;
	for (const TPair<FName, int32>& Cost : FullCosts)
	{
		const int32 RefundQuantity = FMath::Max(
			1,
			FMath::FloorToInt(static_cast<float>(Cost.Value) * 0.60f));
		Refunds.Emplace(Cost.Key, RefundQuantity);
	}

	TArray<TPair<FName, int32>> Granted;
	for (const TPair<FName, int32>& Refund : Refunds)
	{
		if (!Inventory->AddItem(Refund.Key, Refund.Value))
		{
			for (const TPair<FName, int32>& Rollback : Granted)
			{
				Inventory->RemoveItem(Rollback.Key, Rollback.Value);
			}
			if (GEngine)
			{
				GEngine->AddOnScreenDebugMessage(
					91706,
					2.2f,
					FColor(225, 155, 105),
					TEXT("Not enough backpack space for the dismantle refund."));
			}
			return;
		}
		Granted.Add(Refund);
	}

	DestroyBuildActors(BuildId);
	for (FWildBoundPlacedBuildState& Existing : PlacedBuilds)
	{
		Existing.LinkedBuildIds.Remove(BuildId);
	}
	PlacedBuilds.RemoveAll(
		[BuildId](const FWildBoundPlacedBuildState& Existing)
		{
			return Existing.BuildId == BuildId;
		});
	if (PendingPowerLinkSourceBuildId == BuildId)
	{
		PendingPowerLinkSourceBuildId = 0;
	}
	RefreshPowerLinkVisuals();
	RefreshPoweredLights();

	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(
			91706,
			2.3f,
			FColor(190, 205, 150),
			FString::Printf(
				TEXT("DISMANTLED   |   %s   |   60%% MATERIALS RECOVERED"),
				*BuildName));
	}
}

TMap<FName, int32> UWildBoundBuildingSubsystem::GetMaterialCostsForBuild(
	FName BuildTypeId) const
{
	TMap<FName, int32> Costs;
	auto Add = [&Costs](const TCHAR* ItemId, int32 Quantity)
	{
		Costs.Add(FName(ItemId), Quantity);
	};

	if (BuildTypeId == FloorType)
	{
		Add(TEXT("Wood"), 6); Add(TEXT("ScrapMetal"), 1);
	}
	else if (BuildTypeId == WallType || BuildTypeId == DoorwayType)
	{
		Add(TEXT("Wood"), 5); Add(TEXT("ScrapMetal"), 2);
	}
	else if (BuildTypeId == RoofType)
	{
		Add(TEXT("Wood"), 4); Add(TEXT("Cloth"), 4); Add(TEXT("ScrapMetal"), 1);
	}
	else if (BuildTypeId == BarricadeType)
	{
		Add(TEXT("Wood"), 4); Add(TEXT("ScrapMetal"), 3);
	}
	else if (BuildTypeId == StorageType)
	{
		Add(TEXT("Wood"), 4); Add(TEXT("ScrapMetal"), 2); Add(TEXT("MechanicalParts"), 1);
	}
	else if (BuildTypeId == CotType)
	{
		Add(TEXT("Wood"), 3); Add(TEXT("Cloth"), 5); Add(TEXT("ScrapMetal"), 1);
	}
	else if (BuildTypeId == WorkbenchType)
	{
		Add(TEXT("Wood"), 6); Add(TEXT("ScrapMetal"), 5); Add(TEXT("MechanicalParts"), 3);
	}
	else if (BuildTypeId == RainCollectorType)
	{
		Add(TEXT("ScrapMetal"), 4); Add(TEXT("Plastic"), 4); Add(TEXT("Cloth"), 2); Add(TEXT("Adhesive"), 2);
	}
	else if (BuildTypeId == PowerBankType)
	{
		Add(TEXT("ScrapMetal"), 6); Add(TEXT("Battery"), 4); Add(TEXT("Electronics"), 2); Add(TEXT("Wire"), 3); Add(TEXT("MechanicalParts"), 2);
	}
	else if (BuildTypeId == PoweredLightType)
	{
		Add(TEXT("ScrapMetal"), 3); Add(TEXT("Electronics"), 1); Add(TEXT("Wire"), 2); Add(TEXT("Plastic"), 1);
	}
	else if (BuildTypeId == ReinforcedFloorType)
	{
		Add(TEXT("Wood"), 8); Add(TEXT("ScrapMetal"), 5); Add(TEXT("MechanicalParts"), 1);
	}
	else if (BuildTypeId == ReinforcedWallType)
	{
		Add(TEXT("Wood"), 7); Add(TEXT("ScrapMetal"), 6); Add(TEXT("MechanicalParts"), 2);
	}
	else if (BuildTypeId == GeneratorType)
	{
		Add(TEXT("ScrapMetal"), 8); Add(TEXT("MechanicalParts"), 5); Add(TEXT("Electronics"), 3); Add(TEXT("Wire"), 4);
	}

	return Costs;
}

FString UWildBoundBuildingSubsystem::GetBuildDisplayName(FName BuildTypeId) const
{
	if (BuildTypeId == FloorType) return TEXT("WOOD FLOOR");
	if (BuildTypeId == WallType) return TEXT("WOOD WALL");
	if (BuildTypeId == DoorwayType) return TEXT("DOORWAY FRAME");
	if (BuildTypeId == RoofType) return TEXT("SHELTER ROOF");
	if (BuildTypeId == BarricadeType) return TEXT("BARRICADE");
	if (BuildTypeId == StorageType) return TEXT("STORAGE CRATE");
	if (BuildTypeId == CotType) return TEXT("FIELD COT");
	if (BuildTypeId == WorkbenchType) return TEXT("WORKBENCH");
	if (BuildTypeId == RainCollectorType) return TEXT("RAIN COLLECTOR");
	if (BuildTypeId == PowerBankType) return TEXT("BATTERY BANK");
	if (BuildTypeId == PoweredLightType) return TEXT("POWERED LIGHT");
	if (BuildTypeId == ReinforcedFloorType) return TEXT("REINFORCED FLOOR");
	if (BuildTypeId == ReinforcedWallType) return TEXT("REINFORCED WALL");
	if (BuildTypeId == GeneratorType) return TEXT("FUEL GENERATOR");
	return TEXT("STRUCTURE");
}

bool UWildBoundBuildingSubsystem::TryApplyPieceSnap(
	FVector& InOutLocation,
	FRotator& InOutRotation) const
{
	if (!bGridSnapEnabled)
	{
		return false;
	}

	bool bFound = false;
	float BestDistanceSq = FMath::Square(PieceSnapDistance);
	FVector BestLocation = InOutLocation;
	FRotator BestRotation = InOutRotation;

	auto Consider = [&](
		const FVector& CandidateLocation,
		const FRotator& CandidateRotation)
	{
		if (FMath::Abs(CandidateLocation.Z - InOutLocation.Z) > 140.0f)
		{
			return;
		}

		const float DistanceSq = FVector::DistSquared2D(
			CandidateLocation,
			InOutLocation);
		if (DistanceSq <= BestDistanceSq)
		{
			BestDistanceSq = DistanceSq;
			BestLocation = CandidateLocation;
			BestRotation = CandidateRotation;
			bFound = true;
		}
	};

	for (const FWildBoundPlacedBuildState& State : PlacedBuilds)
	{
		if (State.BuildId == RelocatingBuildId)
		{
			continue;
		}

		const FVector Forward = State.Rotation.RotateVector(FVector(1.0f, 0.0f, 0.0f));
		const FVector Right = State.Rotation.RotateVector(FVector(0.0f, 1.0f, 0.0f));

		const bool bActiveFloorLike = ActiveBuildTypeId == FloorType || ActiveBuildTypeId == ReinforcedFloorType;
		const bool bExistingFloorLike = State.BuildTypeId == FloorType || State.BuildTypeId == ReinforcedFloorType;
		const bool bActiveWallLike = ActiveBuildTypeId == WallType || ActiveBuildTypeId == DoorwayType || ActiveBuildTypeId == ReinforcedWallType;
		const bool bExistingWallLike = State.BuildTypeId == WallType || State.BuildTypeId == DoorwayType || State.BuildTypeId == ReinforcedWallType;

		if (bActiveFloorLike && bExistingFloorLike)
		{
			Consider(State.Location + Forward * 400.0f, State.Rotation);
			Consider(State.Location - Forward * 400.0f, State.Rotation);
			Consider(State.Location + Right * 400.0f, State.Rotation);
			Consider(State.Location - Right * 400.0f, State.Rotation);
		}
		else if (ActiveBuildTypeId == RoofType)
		{
			if (bExistingFloorLike)
			{
				Consider(State.Location, State.Rotation);
			}
			else if (State.BuildTypeId == RoofType)
			{
				Consider(State.Location + Forward * 400.0f, State.Rotation);
				Consider(State.Location - Forward * 400.0f, State.Rotation);
				Consider(State.Location + Right * 400.0f, State.Rotation);
				Consider(State.Location - Right * 400.0f, State.Rotation);
			}
		}
		else if (bActiveWallLike)
		{
			if (bExistingFloorLike)
			{
				Consider(
					State.Location + Right * 200.0f,
					State.Rotation);
				Consider(
					State.Location - Right * 200.0f,
					State.Rotation);
				Consider(
					State.Location + Forward * 200.0f,
					FRotator(0.0f, State.Rotation.Yaw + 90.0f, 0.0f));
				Consider(
					State.Location - Forward * 200.0f,
					FRotator(0.0f, State.Rotation.Yaw + 90.0f, 0.0f));
			}
			else if (bExistingWallLike)
			{
				Consider(State.Location + Forward * 400.0f, State.Rotation);
				Consider(State.Location - Forward * 400.0f, State.Rotation);
			}
		}
		else if (ActiveBuildTypeId == BarricadeType && State.BuildTypeId == BarricadeType)
		{
			Consider(State.Location + Forward * 300.0f, State.Rotation);
			Consider(State.Location - Forward * 300.0f, State.Rotation);
		}
	}

	if (bFound)
	{
		InOutLocation = BestLocation;
		InOutRotation = BestRotation;
	}
	return bFound;
}

bool UWildBoundBuildingSubsystem::IsDuplicatePlacement(
	const FVector& Location,
	FName BuildTypeId,
	int32 IgnoreBuildId) const
{
	const bool bNewWallLike = BuildTypeId == WallType || BuildTypeId == DoorwayType || BuildTypeId == ReinforcedWallType;
	const bool bNewFloorLike = BuildTypeId == FloorType || BuildTypeId == ReinforcedFloorType;

	for (const FWildBoundPlacedBuildState& State : PlacedBuilds)
	{
		if (State.BuildId == IgnoreBuildId)
		{
			continue;
		}

		if (FMath::Abs(State.Location.Z - Location.Z) > 90.0f
			|| FVector::DistSquared2D(State.Location, Location) > FMath::Square(70.0f))
		{
			continue;
		}

		const bool bExistingWallLike =
			State.BuildTypeId == WallType || State.BuildTypeId == DoorwayType || State.BuildTypeId == ReinforcedWallType;
		const bool bExistingFloorLike =
			State.BuildTypeId == FloorType || State.BuildTypeId == ReinforcedFloorType;
		if (State.BuildTypeId == BuildTypeId
			|| (bNewWallLike && bExistingWallLike)
			|| (bNewFloorLike && bExistingFloorLike))
		{
			return true;
		}
	}

	return false;
}

UWildBoundInventoryComponent* UWildBoundBuildingSubsystem::GetPlayerInventory() const
{
	UWorld* World = GetWorld();
	APlayerController* PlayerController = World ? World->GetFirstPlayerController() : nullptr;
	APawn* Pawn = PlayerController ? PlayerController->GetPawn() : nullptr;
	return Pawn ? Pawn->FindComponentByClass<UWildBoundInventoryComponent>() : nullptr;
}

FVector UWildBoundBuildingSubsystem::GetBuildHalfExtents(FName BuildTypeId) const
{
	if (BuildTypeId == FloorType) return FVector(200.0f, 200.0f, 10.0f);
	if (BuildTypeId == WallType || BuildTypeId == DoorwayType) return FVector(200.0f, 16.0f, 125.0f);
	if (BuildTypeId == RoofType) return FVector(205.0f, 205.0f, 10.0f);
	if (BuildTypeId == BarricadeType) return FVector(150.0f, 28.0f, 82.0f);
	if (BuildTypeId == StorageType) return FVector(72.0f, 52.0f, 48.0f);
	if (BuildTypeId == CotType) return FVector(112.0f, 45.0f, 34.0f);
	if (BuildTypeId == WorkbenchType) return FVector(92.0f, 45.0f, 112.0f);
	if (BuildTypeId == RainCollectorType) return FVector(92.0f, 78.0f, 180.0f);
	if (BuildTypeId == PowerBankType) return FVector(65.0f, 48.0f, 70.0f);
	if (BuildTypeId == PoweredLightType) return FVector(35.0f, 35.0f, 155.0f);
	if (BuildTypeId == ReinforcedFloorType) return FVector(200.0f, 200.0f, 14.0f);
	if (BuildTypeId == ReinforcedWallType) return FVector(200.0f, 20.0f, 125.0f);
	if (BuildTypeId == GeneratorType) return FVector(78.0f, 52.0f, 135.0f);
	return FVector(50.0f, 50.0f, 50.0f);
}

float UWildBoundBuildingSubsystem::GetBuildVerticalOffset(FName BuildTypeId) const
{
	return BuildTypeId == RoofType ? 250.0f : 0.0f;
}

FVector UWildBoundBuildingSubsystem::SnapLocation(const FVector& Location) const
{
	if (!bGridSnapEnabled)
	{
		return Location;
	}

	return FVector(
		FMath::GridSnap(Location.X, 100.0f),
		FMath::GridSnap(Location.Y, 100.0f),
		Location.Z);
}
