#include "WildBoundDistrictExpansionSubsystem.h"

#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/StaticMeshActor.h"
#include "Engine/TextRenderActor.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "UObject/UObjectGlobals.h"

namespace
{
	const FName TownTag(TEXT("WildBoundTownBlockout"));
	const FName DistrictTag(TEXT("WildBoundDistrictExpansion"));

	UStaticMesh* GetCube()
	{
		static TWeakObjectPtr<UStaticMesh> Mesh;
		if (!Mesh.IsValid())
		{
			Mesh = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube"));
		}
		return Mesh.Get();
	}

	UStaticMesh* GetCylinder()
	{
		static TWeakObjectPtr<UStaticMesh> Mesh;
		if (!Mesh.IsValid())
		{
			Mesh = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
		}
		return Mesh.Get();
	}

	UMaterialInterface* GetBaseMaterial()
	{
		static TWeakObjectPtr<UMaterialInterface> Material;
		if (!Material.IsValid())
		{
			Material = LoadObject<UMaterialInterface>(nullptr, TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));
		}
		return Material.Get();
	}

	void ApplyMaterial(UStaticMeshComponent& Mesh, const FLinearColor& Color, float Roughness, float Metallic)
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

	AStaticMeshActor* SpawnProp(
		UWorld& World,
		UStaticMesh* MeshAsset,
		const FVector& Location,
		const FVector& Scale,
		const FRotator& Rotation,
		const FLinearColor& Color,
		const TCHAR* Label,
		bool bCollidable = true,
		float Roughness = 0.94f,
		float Metallic = 0.0f)
	{
		if (!MeshAsset)
		{
			return nullptr;
		}

		AStaticMeshActor* Actor = World.SpawnActor<AStaticMeshActor>(Location, Rotation);
		if (!Actor)
		{
			return nullptr;
		}

		Actor->Tags.AddUnique(DistrictTag);
#if WITH_EDITOR
		Actor->SetActorLabel(Label);
#endif

		UStaticMeshComponent* Mesh = Actor->GetStaticMeshComponent();
		Mesh->SetMobility(EComponentMobility::Movable);
		Mesh->SetStaticMesh(MeshAsset);
		Mesh->SetCastShadow(true);
		Mesh->SetCollisionEnabled(bCollidable ? ECollisionEnabled::QueryAndPhysics : ECollisionEnabled::NoCollision);
		if (bCollidable)
		{
			Mesh->SetCollisionResponseToAllChannels(ECR_Block);
		}
		ApplyMaterial(*Mesh, Color, Roughness, Metallic);
		Actor->SetActorScale3D(Scale);
		return Actor;
	}

	AStaticMeshActor* SpawnBox(
		UWorld& World,
		const FVector& Location,
		const FVector& Scale,
		const FRotator& Rotation,
		const FLinearColor& Color,
		const TCHAR* Label,
		bool bCollidable = true,
		float Roughness = 0.94f,
		float Metallic = 0.0f)
	{
		return SpawnProp(World, GetCube(), Location, Scale, Rotation, Color, Label, bCollidable, Roughness, Metallic);
	}

	void SpawnDistrictText(UWorld& World, const FVector& Location, const FRotator& Rotation, const FString& Text)
	{
		ATextRenderActor* Actor = World.SpawnActor<ATextRenderActor>(Location, Rotation);
		if (!Actor)
		{
			return;
		}

		Actor->Tags.AddUnique(DistrictTag);
#if WITH_EDITOR
		Actor->SetActorLabel(TEXT("WB_District_Wayfinding"));
#endif
		if (UTextRenderComponent* Render = Actor->GetTextRender())
		{
			Render->SetText(FText::FromString(Text));
			Render->SetWorldSize(34.0f);
			Render->SetTextRenderColor(FColor(155, 146, 123));
			Render->SetCastShadow(false);
		}
	}

	bool FindTownOrigin(UWorld& World, FVector& OutOrigin)
	{
		for (TActorIterator<AStaticMeshActor> It(&World); It; ++It)
		{
			AStaticMeshActor* Actor = *It;
			if (!Actor || !Actor->ActorHasTag(TownTag))
			{
				continue;
			}

			if (Actor->GetActorScale3D().Equals(FVector(80.0f, 80.0f, 0.50f), 0.05f))
			{
				OutOrigin = Actor->GetActorLocation() + FVector(0.0f, 0.0f, 25.0f);
				return true;
			}
		}

		return false;
	}

	void SpawnGroundAndRoadNetwork(UWorld& World, const FVector& Origin)
	{
		const FLinearColor Ground(0.145f, 0.132f, 0.112f, 1.0f);
		const FLinearColor Asphalt(0.043f, 0.047f, 0.050f, 1.0f);
		const FLinearColor Sidewalk(0.29f, 0.278f, 0.25f, 1.0f);
		const FLinearColor Alley(0.075f, 0.072f, 0.066f, 1.0f);

		// Eight additional 80 m ground tiles turn the original intersection into a 240 m district.
		for (int32 X = -1; X <= 1; ++X)
		{
			for (int32 Y = -1; Y <= 1; ++Y)
			{
				if (X == 0 && Y == 0)
				{
					continue;
				}

				SpawnBox(
					World,
					Origin + FVector(X * 8000.0f, Y * 8000.0f, -25.0f),
					FVector(80.0f, 80.0f, 0.50f),
					FRotator::ZeroRotator,
					Ground,
					TEXT("WB_District_GroundTile"));
			}
		}

		// Extend the original main roads through the district.
		SpawnBox(World, Origin + FVector(6000.0f, 0.0f, 5.0f), FVector(60.0f, 8.0f, 0.10f), FRotator::ZeroRotator, Asphalt, TEXT("WB_District_MainRoad_East"));
		SpawnBox(World, Origin + FVector(-6000.0f, 0.0f, 5.0f), FVector(60.0f, 8.0f, 0.10f), FRotator::ZeroRotator, Asphalt, TEXT("WB_District_MainRoad_West"));
		SpawnBox(World, Origin + FVector(0.0f, 6000.0f, 7.0f), FVector(8.0f, 60.0f, 0.10f), FRotator::ZeroRotator, Asphalt, TEXT("WB_District_MainRoad_North"));
		SpawnBox(World, Origin + FVector(0.0f, -6000.0f, 7.0f), FVector(8.0f, 60.0f, 0.10f), FRotator::ZeroRotator, Asphalt, TEXT("WB_District_MainRoad_South"));

		// Secondary east-west avenue across the northern half.
		SpawnBox(World, Origin + FVector(-4500.0f, 6500.0f, 6.0f), FVector(75.0f, 7.0f, 0.10f), FRotator::ZeroRotator, Asphalt, TEXT("WB_District_NorthAvenue_West"));
		SpawnBox(World, Origin + FVector(4500.0f, 6500.0f, 6.0f), FVector(75.0f, 7.0f, 0.10f), FRotator::ZeroRotator, Asphalt, TEXT("WB_District_NorthAvenue_East"));

		// Western residential cross street and eastern service road make loops instead of dead corridors.
		SpawnBox(World, Origin + FVector(-6500.0f, 2500.0f, 7.0f), FVector(7.0f, 80.0f, 0.10f), FRotator::ZeroRotator, Asphalt, TEXT("WB_District_ResidentialStreet"));
		SpawnBox(World, Origin + FVector(6500.0f, -3000.0f, 7.0f), FVector(7.0f, 70.0f, 0.10f), FRotator::ZeroRotator, Asphalt, TEXT("WB_District_ServiceRoad"));

		// Narrow alleys provide shortcuts and future stealth/scavenging routes.
		SpawnBox(World, Origin + FVector(4100.0f, 3300.0f, 11.0f), FVector(3.0f, 55.0f, 0.08f), FRotator::ZeroRotator, Alley, TEXT("WB_District_CommercialAlley"));
		SpawnBox(World, Origin + FVector(-3400.0f, -5700.0f, 11.0f), FVector(42.0f, 3.0f, 0.08f), FRotator::ZeroRotator, Alley, TEXT("WB_District_BackLane"));

		// Sidewalk strips only where they help read the major pedestrian routes.
		SpawnBox(World, Origin + FVector(0.0f, 900.0f, 15.0f), FVector(115.0f, 2.2f, 0.15f), FRotator::ZeroRotator, Sidewalk, TEXT("WB_District_MainSidewalk_N"));
		SpawnBox(World, Origin + FVector(0.0f, -900.0f, 15.0f), FVector(115.0f, 2.2f, 0.15f), FRotator::ZeroRotator, Sidewalk, TEXT("WB_District_MainSidewalk_S"));
		SpawnBox(World, Origin + FVector(-6500.0f, 6500.0f, 15.0f), FVector(2.0f, 42.0f, 0.15f), FRotator::ZeroRotator, Sidewalk, TEXT("WB_District_ResidentialWalk"));
	}

	void SpawnCommercialStrip(UWorld& World, const FVector& Origin)
	{
		const FLinearColor FadedBrick(0.34f, 0.255f, 0.205f, 1.0f);
		const FLinearColor DustyPlaster(0.37f, 0.35f, 0.30f, 1.0f);
		const FLinearColor Pharmacy(0.24f, 0.31f, 0.285f, 1.0f);
		const FLinearColor Roof(0.18f, 0.17f, 0.155f, 1.0f);

		// East side: recognizable storefront rhythm with intentional gaps for future interiors.
		SpawnBox(World, Origin + FVector(5200.0f, 2200.0f, 520.0f), FVector(16.0f, 12.0f, 10.0f), FRotator(0.0f, -2.0f, 0.0f), FadedBrick, TEXT("WB_District_Commercial_01"));
		SpawnBox(World, Origin + FVector(8200.0f, 2150.0f, 700.0f), FVector(20.0f, 13.0f, 14.0f), FRotator(0.0f, 1.5f, 0.0f), DustyPlaster, TEXT("WB_District_Commercial_02"));
		SpawnBox(World, Origin + FVector(5200.0f, 4700.0f, 420.0f), FVector(13.0f, 10.0f, 8.0f), FRotator(0.0f, 1.0f, 0.0f), Pharmacy, TEXT("WB_District_PharmacyShell"));
		SpawnBox(World, Origin + FVector(8300.0f, 4750.0f, 560.0f), FVector(20.0f, 11.0f, 11.0f), FRotator(0.0f, -1.0f, 0.0f), FadedBrick * 0.88f, TEXT("WB_District_MarketShell"));

		// Roof setbacks break up the primitive-block skyline.
		SpawnBox(World, Origin + FVector(8300.0f, 2150.0f, 1450.0f), FVector(7.0f, 6.0f, 1.4f), FRotator::ZeroRotator, Roof, TEXT("WB_District_RooftopPlant"));
		SpawnBox(World, Origin + FVector(5200.0f, 4700.0f, 860.0f), FVector(4.5f, 4.0f, 0.9f), FRotator::ZeroRotator, Roof, TEXT("WB_District_PharmacyRoofUnit"));
	}

	void SpawnCivicAndMedicalZone(UWorld& World, const FVector& Origin)
	{
		const FLinearColor CivicConcrete(0.305f, 0.31f, 0.30f, 1.0f);
		const FLinearColor Clinic(0.34f, 0.355f, 0.33f, 1.0f);
		const FLinearColor DarkGlassMass(0.095f, 0.105f, 0.105f, 1.0f);
		const FLinearColor Rust(0.17f, 0.095f, 0.055f, 1.0f);

		// North-east civic/medical zone becomes a future objective cluster.
		SpawnBox(World, Origin + FVector(2600.0f, 8600.0f, 640.0f), FVector(24.0f, 16.0f, 12.0f), FRotator(0.0f, -2.0f, 0.0f), Clinic, TEXT("WB_District_ClinicShell"));
		SpawnBox(World, Origin + FVector(7600.0f, 8600.0f, 920.0f), FVector(26.0f, 17.0f, 18.0f), FRotator(0.0f, 1.0f, 0.0f), CivicConcrete, TEXT("WB_District_MunicipalShell"));
		SpawnBox(World, Origin + FVector(7600.0f, 7700.0f, 710.0f), FVector(13.0f, 0.35f, 5.0f), FRotator::ZeroRotator, DarkGlassMass, TEXT("WB_District_MunicipalWindowBand"), false, 0.55f, 0.10f);

		// A damaged pedestrian canopy creates a strong threshold into the clinic grounds.
		SpawnBox(World, Origin + FVector(2200.0f, 7000.0f, 300.0f), FVector(8.0f, 2.0f, 0.25f), FRotator(8.0f, 0.0f, 3.0f), Rust, TEXT("WB_District_ClinicCanopy"), true, 0.78f, 0.44f);
		SpawnBox(World, Origin + FVector(1500.0f, 7000.0f, 145.0f), FVector(0.22f, 0.22f, 3.0f), FRotator(4.0f, 0.0f, 2.0f), Rust, TEXT("WB_District_ClinicCanopyPostL"), true, 0.78f, 0.44f);
		SpawnBox(World, Origin + FVector(2850.0f, 7000.0f, 145.0f), FVector(0.22f, 0.22f, 3.0f), FRotator(-5.0f, 0.0f, -2.0f), Rust, TEXT("WB_District_ClinicCanopyPostR"), true, 0.78f, 0.44f);
	}

	void SpawnResidentialEdge(UWorld& World, const FVector& Origin)
	{
		const FLinearColor HouseA(0.30f, 0.285f, 0.255f, 1.0f);
		const FLinearColor HouseB(0.255f, 0.285f, 0.30f, 1.0f);
		const FLinearColor HouseC(0.315f, 0.265f, 0.225f, 1.0f);
		const FLinearColor Fence(0.17f, 0.135f, 0.095f, 1.0f);

		// West side transitions into smaller residential scale.
		const FVector HouseLocations[] =
		{
			Origin + FVector(-9000.0f, 2600.0f, 360.0f),
			Origin + FVector(-9000.0f, 5200.0f, 410.0f),
			Origin + FVector(-9000.0f, 8000.0f, 360.0f),
			Origin + FVector(-6200.0f, 9000.0f, 390.0f),
			Origin + FVector(-3500.0f, 9000.0f, 430.0f)
		};

		for (int32 Index = 0; Index < 5; ++Index)
		{
			const FLinearColor Color = Index % 3 == 0 ? HouseA : Index % 3 == 1 ? HouseB : HouseC;
			SpawnBox(World, HouseLocations[Index], FVector(11.0f, 9.0f, 7.0f + (Index % 2)), FRotator(0.0f, Index % 2 == 0 ? -2.0f : 2.0f, 0.0f), Color, TEXT("WB_District_ResidentialShell"));
			SpawnBox(World, HouseLocations[Index] + FVector(0.0f, 0.0f, 415.0f), FVector(11.5f, 9.5f, 0.45f), FRotator(0.0f, 0.0f, Index % 2 == 0 ? 3.0f : -3.0f), HouseA * 0.60f, TEXT("WB_District_ResidentialRoof"));
		}

		// Sparse fence fragments imply yards without enclosing the player in maze geometry.
		SpawnBox(World, Origin + FVector(-7600.0f, 3900.0f, 70.0f), FVector(0.16f, 10.0f, 1.4f), FRotator::ZeroRotator, Fence, TEXT("WB_District_YardFence_01"), true, 0.82f, 0.15f);
		SpawnBox(World, Origin + FVector(-7600.0f, 7500.0f, 70.0f), FVector(0.16f, 8.0f, 1.4f), FRotator(0.0f, 3.0f, 0.0f), Fence, TEXT("WB_District_YardFence_02"), true, 0.82f, 0.15f);
	}

	void SpawnIndustrialSouth(UWorld& World, const FVector& Origin)
	{
		const FLinearColor Warehouse(0.24f, 0.245f, 0.235f, 1.0f);
		const FLinearColor ServiceBrick(0.31f, 0.235f, 0.19f, 1.0f);
		const FLinearColor Metal(0.13f, 0.105f, 0.08f, 1.0f);
		const FLinearColor Concrete(0.255f, 0.245f, 0.225f, 1.0f);

		SpawnBox(World, Origin + FVector(7600.0f, -7200.0f, 650.0f), FVector(29.0f, 20.0f, 12.0f), FRotator(0.0f, -1.5f, 0.0f), Warehouse, TEXT("WB_District_WarehouseShell"));
		SpawnBox(World, Origin + FVector(3000.0f, -8000.0f, 420.0f), FVector(18.0f, 13.0f, 8.0f), FRotator(0.0f, 2.0f, 0.0f), ServiceBrick, TEXT("WB_District_ServiceGarage"));
		SpawnBox(World, Origin + FVector(3000.0f, -6750.0f, 260.0f), FVector(14.0f, 0.30f, 5.0f), FRotator::ZeroRotator, Metal, TEXT("WB_District_GarageDoor"), true, 0.72f, 0.42f);

		// Loading-yard clutter establishes cover and future loot locations.
		SpawnBox(World, Origin + FVector(8700.0f, -4300.0f, 70.0f), FVector(3.0f, 2.0f, 0.7f), FRotator(0.0f, 13.0f, 0.0f), Concrete, TEXT("WB_District_LoadingBarrier_01"));
		SpawnBox(World, Origin + FVector(9300.0f, -4800.0f, 95.0f), FVector(4.2f, 1.7f, 0.95f), FRotator(5.0f, -8.0f, 2.0f), Metal, TEXT("WB_District_LoadingDebris_02"), true, 0.82f, 0.30f);
	}

	void SpawnLandmarksAndRouteControl(UWorld& World, const FVector& Origin)
	{
		const FLinearColor TowerMetal(0.15f, 0.095f, 0.06f, 1.0f);
		const FLinearColor Tank(0.215f, 0.225f, 0.215f, 1.0f);
		const FLinearColor Barricade(0.31f, 0.245f, 0.15f, 1.0f);
		const FLinearColor Debris(0.22f, 0.205f, 0.185f, 1.0f);
		const FLinearColor Wreck(0.18f, 0.17f, 0.15f, 1.0f);

		// Water tower anchors the far north-west skyline and gives the district a navigation landmark.
		const FVector TowerBase = Origin + FVector(-10200.0f, 10100.0f, 0.0f);
		for (int32 Corner = 0; Corner < 4; ++Corner)
		{
			const float X = Corner < 2 ? -180.0f : 180.0f;
			const float Y = Corner % 2 == 0 ? -180.0f : 180.0f;
			SpawnBox(World, TowerBase + FVector(X, Y, 750.0f), FVector(0.18f, 0.18f, 15.0f), FRotator(Corner % 2 == 0 ? 2.0f : -2.0f, 0.0f, 0.0f), TowerMetal, TEXT("WB_District_WaterTowerLeg"), true, 0.72f, 0.55f);
		}
		SpawnProp(World, GetCylinder(), TowerBase + FVector(0.0f, 0.0f, 1650.0f), FVector(3.4f, 3.4f, 2.4f), FRotator::ZeroRotator, Tank, TEXT("WB_District_WaterTowerTank"), true, 0.80f, 0.35f);
		SpawnBox(World, TowerBase + FVector(0.0f, 0.0f, 1280.0f), FVector(4.0f, 0.14f, 0.12f), FRotator(0.0f, 45.0f, 0.0f), TowerMetal, TEXT("WB_District_WaterTowerBrace_01"), false, 0.76f, 0.52f);
		SpawnBox(World, TowerBase + FVector(0.0f, 0.0f, 1280.0f), FVector(4.0f, 0.14f, 0.12f), FRotator(0.0f, -45.0f, 0.0f), TowerMetal, TEXT("WB_District_WaterTowerBrace_02"), false, 0.76f, 0.52f);

		// East road is partially blocked by an evacuation choke point, but a side route remains open.
		SpawnBox(World, Origin + FVector(9300.0f, -350.0f, 70.0f), FVector(5.0f, 0.55f, 0.70f), FRotator(0.0f, 18.0f, 0.0f), Barricade, TEXT("WB_District_EvacBarrier_01"));
		SpawnBox(World, Origin + FVector(9800.0f, 260.0f, 82.0f), FVector(4.6f, 0.55f, 0.82f), FRotator(0.0f, -14.0f, 0.0f), Barricade * 0.85f, TEXT("WB_District_EvacBarrier_02"));
		SpawnBox(World, Origin + FVector(10400.0f, 50.0f, 120.0f), FVector(6.0f, 2.4f, 1.15f), FRotator(0.0f, 22.0f, 4.0f), Wreck, TEXT("WB_District_EvacWreck"), true, 0.84f, 0.26f);

		// North road collapse encourages the player to use the new side street/alley network.
		SpawnBox(World, Origin + FVector(-250.0f, 10300.0f, 105.0f), FVector(4.0f, 2.0f, 0.85f), FRotator(12.0f, 9.0f, 7.0f), Debris, TEXT("WB_District_RoadCollapse_01"));
		SpawnBox(World, Origin + FVector(500.0f, 10600.0f, 135.0f), FVector(5.0f, 2.6f, 1.1f), FRotator(-9.0f, -13.0f, 5.0f), Debris * 0.88f, TEXT("WB_District_RoadCollapse_02"));
		SpawnBox(World, Origin + FVector(-850.0f, 10900.0f, 80.0f), FVector(2.5f, 1.6f, 0.55f), FRotator(6.0f, 30.0f, -3.0f), Debris * 0.76f, TEXT("WB_District_RoadCollapse_03"));

		// Minimal wayfinding helps testing the new layout without committing to final signage art.
		SpawnDistrictText(World, Origin + FVector(6200.0f, 900.0f, 360.0f), FRotator(0.0f, -90.0f, 0.0f), TEXT("EAST MARKET  ->"));
		SpawnDistrictText(World, Origin + FVector(-6100.0f, 6150.0f, 360.0f), FRotator(0.0f, 0.0f, 0.0f), TEXT("RESIDENTIAL / WATER TOWER"));
	}

	void BuildDistrict(UWorld& World, const FVector& Origin)
	{
		SpawnGroundAndRoadNetwork(World, Origin);
		SpawnCommercialStrip(World, Origin);
		SpawnCivicAndMedicalZone(World, Origin);
		SpawnResidentialEdge(World, Origin);
		SpawnIndustrialSouth(World, Origin);
		SpawnLandmarksAndRouteControl(World, Origin);
	}
}

void UWildBoundDistrictExpansionSubsystem::OnWorldBeginPlay(UWorld& InWorld)
{
	Super::OnWorldBeginPlay(InWorld);

	if (!InWorld.IsGameWorld())
	{
		return;
	}

	TryBuildDistrict();
	InWorld.GetTimerManager().SetTimer(
		DistrictBuildTimer,
		this,
		&UWildBoundDistrictExpansionSubsystem::TryBuildDistrict,
		0.5f,
		true,
		0.10f);
}

void UWildBoundDistrictExpansionSubsystem::Deinitialize()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(DistrictBuildTimer);
	}

	Super::Deinitialize();
}

void UWildBoundDistrictExpansionSubsystem::TryBuildDistrict()
{
	if (bDistrictBuilt)
	{
		return;
	}

	UWorld* World = GetWorld();
	if (!World || !World->IsGameWorld())
	{
		return;
	}

	for (TActorIterator<AActor> It(World); It; ++It)
	{
		if (It->ActorHasTag(DistrictTag))
		{
			bDistrictBuilt = true;
			World->GetTimerManager().ClearTimer(DistrictBuildTimer);
			return;
		}
	}

	FVector TownOrigin;
	if (!FindTownOrigin(*World, TownOrigin))
	{
		return;
	}

	BuildDistrict(*World, TownOrigin);
	bDistrictBuilt = true;
	World->GetTimerManager().ClearTimer(DistrictBuildTimer);
	UE_LOG(LogTemp, Log, TEXT("WildBound world: first 240m explorable town district built around the original intersection."));
}
