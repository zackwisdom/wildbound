#include "WildBoundBuildingSubsystem.h"

#include "../Inventory/WildBoundInventoryComponent.h"
#include "CollisionQueryParams.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/Engine.h"
#include "Engine/OverlapResult.h"
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

	const FName FloorType(TEXT("BuildFloor"));
	const FName WallType(TEXT("BuildWall"));
	const FName DoorwayType(TEXT("BuildDoorway"));
	const FName RoofType(TEXT("BuildRoof"));
	const FName BarricadeType(TEXT("BuildBarricade"));
	const FName StorageType(TEXT("BuildStorage"));
	const FName CotType(TEXT("BuildCot"));
	const FName WorkbenchType(TEXT("BuildWorkbench"));

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

	bool IsStructuralBuild(FName BuildTypeId)
	{
		return BuildTypeId == FloorType
			|| BuildTypeId == WallType
			|| BuildTypeId == DoorwayType
			|| BuildTypeId == RoofType;
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
}

void UWildBoundBuildingSubsystem::Deinitialize()
{
	CancelPlacement(false);

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(BuildingUpdateTimer);
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
		: (bPlacementValid ? TEXT("VALID") : TEXT("OBSTRUCTED"));

	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(
			91700,
			0.12f,
			StatusColor,
			FString::Printf(
				TEXT("BUILD  %s   |   %s   |   LMB PLACE   R ROTATE   SHIFT+R 90 DEG   G GRID %s   RMB/ESC CANCEL"),
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
	FVector ProbePoint = AimPoint;
	if (bGridSnapEnabled)
	{
		ProbePoint = SnapLocation(ProbePoint);
	}

	FCollisionQueryParams GroundParams(SCENE_QUERY_STAT(WildBoundBuildGround), false, Pawn);
	GroundParams.AddIgnoredActor(Preview);
	FHitResult GroundHit;
	const FVector GroundStart(ProbePoint.X, ProbePoint.Y, ProbePoint.Z + 550.0f);
	const FVector GroundEnd(ProbePoint.X, ProbePoint.Y, ProbePoint.Z - 1600.0f);
	const bool bFoundGround = World->LineTraceSingleByChannel(
		GroundHit,
		GroundStart,
		GroundEnd,
		ECC_Visibility,
		GroundParams);

	if (!bFoundGround)
	{
		bPlacementValid = false;
		UpdatePreviewMaterial();
		return;
	}

	PreviewLocation = GroundHit.ImpactPoint;
	if (bGridSnapEnabled)
	{
		PreviewLocation = SnapLocation(PreviewLocation);
		PreviewLocation.Z = GroundHit.ImpactPoint.Z;
	}
	PreviewRotation = FRotator(0.0f, CurrentYaw, 0.0f);

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

	FWildBoundPlacedBuildState State;
	State.BuildTypeId = ActiveBuildTypeId;
	State.Location = PreviewLocation;
	State.Rotation = PreviewRotation;
	PlacedBuilds.Add(State);

	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(
			91702,
			2.0f,
			FColor(150, 218, 140),
			FString::Printf(TEXT("PLACED   |   %s"), *ActiveDisplayName));
	}

	// Stay in build mode while materials remain, making repeated walls/floors practical.
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

	bPlacementActive = false;
	bPlacementValid = false;
	ActiveBuildTypeId = NAME_None;
	ActiveDisplayName.Reset();
	ActiveMaterialCosts.Reset();
	DestroyPreviewActor();

	if (bShowMessage && GEngine)
	{
		GEngine->AddOnScreenDebugMessage(
			91700,
			1.4f,
			FColor(185, 190, 178),
			TEXT("BUILD MODE CANCELLED   |   no materials consumed"));
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

	for (const FWildBoundPlacedBuildState& Saved : SavedBuilds)
	{
		if (Saved.BuildTypeId.IsNone())
		{
			continue;
		}

		if (SpawnPlacedBuild(Saved.BuildTypeId, Saved.Location, Saved.Rotation, nullptr))
		{
			PlacedBuilds.Add(Saved);
		}
	}
}

void UWildBoundBuildingSubsystem::DestroyAllPlacedBuildActors()
{
	UWorld* World = GetWorld();
	if (!World)
	{
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
