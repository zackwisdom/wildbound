#include "WildBoundOutskirtsExpansionSubsystem.h"

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
	const FName OutskirtsTag(TEXT("WildBoundOutskirtsExpansion"));

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

		Actor->Tags.AddUnique(OutskirtsTag);
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

	void SpawnWorldText(UWorld& World, const FVector& Location, const FRotator& Rotation, const FString& Text, float Size = 34.0f)
	{
		ATextRenderActor* Actor = World.SpawnActor<ATextRenderActor>(Location, Rotation);
		if (!Actor)
		{
			return;
		}

		Actor->Tags.AddUnique(OutskirtsTag);
#if WITH_EDITOR
		Actor->SetActorLabel(TEXT("WB_Outskirts_Wayfinding"));
#endif
		if (UTextRenderComponent* Render = Actor->GetTextRender())
		{
			Render->SetText(FText::FromString(Text));
			Render->SetWorldSize(Size);
			Render->SetTextRenderColor(FColor(148, 139, 115));
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

	void SpawnOuterGround(UWorld& World, const FVector& Origin)
	{
		const FLinearColor DryGround(0.135f, 0.122f, 0.102f, 1.0f);
		const FLinearColor HighwayGround(0.125f, 0.117f, 0.102f, 1.0f);

		// One complete 80 m ring around the existing 240 m district creates a 400 m town footprint.
		for (int32 X = -2; X <= 2; ++X)
		{
			for (int32 Y = -2; Y <= 2; ++Y)
			{
				if (FMath::Abs(X) <= 1 && FMath::Abs(Y) <= 1)
				{
					continue;
				}

				SpawnBox(
					World,
					Origin + FVector(X * 8000.0f, Y * 8000.0f, -25.0f),
					FVector(80.0f, 80.0f, 0.50f),
					FRotator::ZeroRotator,
					DryGround,
					TEXT("WB_Outskirts_GroundRing"));
			}
		}

		// Two more tiles carry the eastern evacuation highway beyond the town edge.
		SpawnBox(World, Origin + FVector(24000.0f, 0.0f, -25.0f), FVector(80.0f, 80.0f, 0.50f), FRotator::ZeroRotator, HighwayGround, TEXT("WB_Outskirts_HighwayGround_01"));
		SpawnBox(World, Origin + FVector(32000.0f, 0.0f, -25.0f), FVector(80.0f, 80.0f, 0.50f), FRotator::ZeroRotator, HighwayGround, TEXT("WB_Outskirts_HighwayGround_02"));
	}

	void SpawnRoadConnections(UWorld& World, const FVector& Origin)
	{
		const FLinearColor Asphalt(0.041f, 0.044f, 0.046f, 1.0f);
		const FLinearColor OldAsphalt(0.055f, 0.055f, 0.052f, 1.0f);
		const FLinearColor DirtTrack(0.17f, 0.145f, 0.105f, 1.0f);

		// Main highway continues seamlessly from the district's east road all the way to the evacuation edge.
		SpawnBox(World, Origin + FVector(23500.0f, 0.0f, 6.0f), FVector(235.0f, 9.0f, 0.11f), FRotator::ZeroRotator, Asphalt, TEXT("WB_Outskirts_EvacHighway"));

		// Western residential road narrows into a county road instead of remaining urban forever.
		SpawnBox(World, Origin + FVector(-16000.0f, 0.0f, 7.0f), FVector(80.0f, 6.5f, 0.10f), FRotator::ZeroRotator, OldAsphalt, TEXT("WB_Outskirts_WestCountyRoad"));

		// North and south spines continue just far enough to establish future expansion directions.
		SpawnBox(World, Origin + FVector(0.0f, 16000.0f, 7.0f), FVector(7.0f, 80.0f, 0.10f), FRotator::ZeroRotator, OldAsphalt, TEXT("WB_Outskirts_NorthUtilityRoad"));
		SpawnBox(World, Origin + FVector(0.0f, -16000.0f, 7.0f), FVector(7.0f, 80.0f, 0.10f), FRotator::ZeroRotator, OldAsphalt, TEXT("WB_Outskirts_SouthServiceRoad"));

		// Smaller roads make the outskirts loopable instead of four long dead ends.
		SpawnBox(World, Origin + FVector(13500.0f, 10500.0f, 9.0f), FVector(65.0f, 5.0f, 0.09f), FRotator::ZeroRotator, OldAsphalt, TEXT("WB_Outskirts_NorthConnector"));
		SpawnBox(World, Origin + FVector(12500.0f, -11500.0f, 9.0f), FVector(55.0f, 5.0f, 0.09f), FRotator::ZeroRotator, OldAsphalt, TEXT("WB_Outskirts_SouthConnector"));
		SpawnBox(World, Origin + FVector(-13500.0f, 9500.0f, 10.0f), FVector(5.0f, 48.0f, 0.08f), FRotator::ZeroRotator, OldAsphalt, TEXT("WB_Outskirts_HomeLoop"));
		SpawnBox(World, Origin + FVector(-15000.0f, -10500.0f, 10.0f), FVector(45.0f, 4.0f, 0.08f), FRotator(0.0f, -5.0f, 0.0f), DirtTrack, TEXT("WB_Outskirts_DirtAccess"));
	}

	void SpawnEasternHighwayStrip(UWorld& World, const FVector& Origin)
	{
		const FLinearColor Concrete(0.28f, 0.27f, 0.245f, 1.0f);
		const FLinearColor FadedCream(0.37f, 0.35f, 0.29f, 1.0f);
		const FLinearColor Rust(0.22f, 0.105f, 0.055f, 1.0f);
		const FLinearColor Roof(0.14f, 0.145f, 0.14f, 1.0f);
		const FLinearColor AsphaltLot(0.055f, 0.057f, 0.056f, 1.0f);
		const FLinearColor SignBlue(0.12f, 0.22f, 0.255f, 1.0f);

		// Old service station at the point where downtown gives way to the highway.
		SpawnBox(World, Origin + FVector(16500.0f, -4700.0f, 12.0f), FVector(32.0f, 24.0f, 0.08f), FRotator::ZeroRotator, AsphaltLot, TEXT("WB_Outskirts_GasLot"), false);
		SpawnBox(World, Origin + FVector(17600.0f, -6000.0f, 420.0f), FVector(15.0f, 9.0f, 8.0f), FRotator::ZeroRotator, FadedCream, TEXT("WB_Outskirts_ServiceStation"));
		SpawnBox(World, Origin + FVector(15500.0f, -3900.0f, 380.0f), FVector(18.0f, 11.0f, 0.28f), FRotator::ZeroRotator, Roof, TEXT("WB_Outskirts_GasCanopy"));
		for (int32 Pump = 0; Pump < 3; ++Pump)
		{
			SpawnBox(World, Origin + FVector(14500.0f + Pump * 950.0f, -3900.0f, 95.0f), FVector(0.65f, 0.65f, 1.9f), FRotator::ZeroRotator, Rust, TEXT("WB_Outskirts_FuelPump"));
		}

		// Roadside motel provides a recognizable, eventually enterable destination farther from town.
		SpawnBox(World, Origin + FVector(24700.0f, 5800.0f, 12.0f), FVector(42.0f, 27.0f, 0.07f), FRotator::ZeroRotator, AsphaltLot, TEXT("WB_Outskirts_MotelLot"), false);
		SpawnBox(World, Origin + FVector(25500.0f, 7200.0f, 380.0f), FVector(35.0f, 7.0f, 7.0f), FRotator::ZeroRotator, FadedCream * 0.88f, TEXT("WB_Outskirts_MotelWing"));
		SpawnBox(World, Origin + FVector(23000.0f, 5100.0f, 320.0f), FVector(8.0f, 7.0f, 6.0f), FRotator::ZeroRotator, Concrete, TEXT("WB_Outskirts_MotelOffice"));
		SpawnBox(World, Origin + FVector(22000.0f, 3500.0f, 450.0f), FVector(0.18f, 0.18f, 9.0f), FRotator::ZeroRotator, Rust, TEXT("WB_Outskirts_MotelSignPole"));
		SpawnBox(World, Origin + FVector(22000.0f, 3500.0f, 950.0f), FVector(4.5f, 0.25f, 2.2f), FRotator::ZeroRotator, SignBlue, TEXT("WB_Outskirts_MotelSign"), false);
		SpawnWorldText(World, Origin + FVector(21600.0f, 3465.0f, 1020.0f), FRotator(0.0f, -90.0f, 0.0f), TEXT("MOTOR LODGE"), 30.0f);

		// Billboard becomes a long-range silhouette along the highway.
		SpawnBox(World, Origin + FVector(20500.0f, 9300.0f, 650.0f), FVector(0.22f, 0.22f, 13.0f), FRotator::ZeroRotator, Rust, TEXT("WB_Outskirts_BillboardPost"));
		SpawnBox(World, Origin + FVector(20500.0f, 9300.0f, 1450.0f), FVector(9.0f, 0.35f, 4.0f), FRotator(0.0f, -4.0f, 0.0f), FadedCream * 0.75f, TEXT("WB_Outskirts_Billboard"), false);

		// Final highway checkpoint clearly marks the current authored edge without an invisible wall.
		const FVector Checkpoint = Origin + FVector(34200.0f, 0.0f, 0.0f);
		SpawnBox(World, Checkpoint + FVector(0.0f, 0.0f, 75.0f), FVector(1.4f, 11.0f, 1.4f), FRotator(0.0f, 7.0f, 0.0f), Concrete, TEXT("WB_Outskirts_HighwayBarrier"));
		SpawnBox(World, Checkpoint + FVector(-550.0f, 360.0f, 100.0f), FVector(4.5f, 2.0f, 0.8f), FRotator(0.0f, 18.0f, 0.0f), Rust, TEXT("WB_Outskirts_AbandonedVehicle_01"));
		SpawnBox(World, Checkpoint + FVector(250.0f, -350.0f, 110.0f), FVector(5.3f, 2.2f, 0.9f), FRotator(0.0f, -13.0f, 0.0f), Roof, TEXT("WB_Outskirts_AbandonedVehicle_02"));
		SpawnBox(World, Checkpoint + FVector(-900.0f, -850.0f, 360.0f), FVector(0.18f, 0.18f, 7.0f), FRotator::ZeroRotator, Rust, TEXT("WB_Outskirts_RouteSignPole"));
		SpawnBox(World, Checkpoint + FVector(-900.0f, -850.0f, 760.0f), FVector(5.5f, 0.25f, 2.0f), FRotator::ZeroRotator, SignBlue, TEXT("WB_Outskirts_RouteSign"), false);
		SpawnWorldText(World, Checkpoint + FVector(-1370.0f, -885.0f, 825.0f), FRotator(0.0f, -90.0f, 0.0f), TEXT("STATE ROUTE 17\nEVACUATION EAST"), 27.0f);
	}

	void SpawnWesternResidentialFringe(UWorld& World, const FVector& Origin)
	{
		const FLinearColor HouseA(0.31f, 0.295f, 0.255f, 1.0f);
		const FLinearColor HouseB(0.265f, 0.285f, 0.285f, 1.0f);
		const FLinearColor HouseC(0.34f, 0.275f, 0.22f, 1.0f);
		const FLinearColor Roof(0.15f, 0.145f, 0.135f, 1.0f);
		const FLinearColor DeadGrass(0.18f, 0.155f, 0.105f, 1.0f);

		// Density drops sharply here: individual houses have breathing room and open lots between them.
		SpawnBox(World, Origin + FVector(-14600.0f, 5100.0f, 330.0f), FVector(10.0f, 8.0f, 6.0f), FRotator(0.0f, 3.0f, 0.0f), HouseA, TEXT("WB_Outskirts_Home_01"));
		SpawnBox(World, Origin + FVector(-17600.0f, 8200.0f, 300.0f), FVector(9.0f, 7.0f, 5.5f), FRotator(0.0f, -5.0f, 0.0f), HouseB, TEXT("WB_Outskirts_Home_02"));
		SpawnBox(World, Origin + FVector(-15000.0f, 13200.0f, 350.0f), FVector(11.0f, 8.0f, 6.5f), FRotator(0.0f, 2.0f, 0.0f), HouseC, TEXT("WB_Outskirts_Home_03"));
		SpawnBox(World, Origin + FVector(-17800.0f, -6500.0f, 310.0f), FVector(12.0f, 6.0f, 5.8f), FRotator(0.0f, -7.0f, 0.0f), HouseA * 0.88f, TEXT("WB_Outskirts_Farmhouse"));
		SpawnBox(World, Origin + FVector(-14800.0f, -12800.0f, 220.0f), FVector(13.0f, 4.5f, 4.0f), FRotator(0.0f, 8.0f, 0.0f), HouseB * 0.82f, TEXT("WB_Outskirts_TrailerHome"));

		// Small detached sheds and abandoned yards keep the space useful for later scavenging.
		SpawnBox(World, Origin + FVector(-16600.0f, 4700.0f, 150.0f), FVector(4.0f, 4.0f, 2.8f), FRotator::ZeroRotator, Roof, TEXT("WB_Outskirts_Shed_01"));
		SpawnBox(World, Origin + FVector(-18400.0f, -8100.0f, 145.0f), FVector(4.5f, 3.5f, 2.7f), FRotator(0.0f, 5.0f, 0.0f), Roof, TEXT("WB_Outskirts_Shed_02"));
		SpawnBox(World, Origin + FVector(-15100.0f, 9400.0f, 8.0f), FVector(24.0f, 18.0f, 0.05f), FRotator::ZeroRotator, DeadGrass, TEXT("WB_Outskirts_DeadYard_01"), false);
		SpawnBox(World, Origin + FVector(-16400.0f, -10300.0f, 8.0f), FVector(28.0f, 20.0f, 0.05f), FRotator::ZeroRotator, DeadGrass * 0.92f, TEXT("WB_Outskirts_DeadYard_02"), false);

		// County road is physically closed at the current western edge by a washout/rubble field.
		SpawnBox(World, Origin + FVector(-19800.0f, -120.0f, 85.0f), FVector(2.2f, 8.5f, 1.4f), FRotator(10.0f, 12.0f, 5.0f), Roof, TEXT("WB_Outskirts_WestWashout_01"));
		SpawnBox(World, Origin + FVector(-19450.0f, 300.0f, 60.0f), FVector(3.8f, 2.8f, 0.8f), FRotator(-8.0f, -20.0f, 6.0f), HouseC * 0.72f, TEXT("WB_Outskirts_WestWashout_02"));
	}

	void SpawnNorthernUtilityCorridor(UWorld& World, const FVector& Origin)
	{
		const FLinearColor Channel(0.075f, 0.075f, 0.068f, 1.0f);
		const FLinearColor Concrete(0.26f, 0.255f, 0.235f, 1.0f);
		const FLinearColor Steel(0.13f, 0.14f, 0.135f, 1.0f);
		const FLinearColor Rust(0.22f, 0.115f, 0.065f, 1.0f);

		// Broad drainage channel cuts across the north side and gives the map a strong non-building feature.
		SpawnBox(World, Origin + FVector(0.0f, 17100.0f, 4.0f), FVector(190.0f, 12.0f, 0.06f), FRotator::ZeroRotator, Channel, TEXT("WB_Outskirts_DrainageChannel"), false);
		SpawnBox(World, Origin + FVector(0.0f, 15800.0f, 125.0f), FVector(190.0f, 0.7f, 2.5f), FRotator::ZeroRotator, Concrete, TEXT("WB_Outskirts_ChannelWall_South"));
		SpawnBox(World, Origin + FVector(0.0f, 18400.0f, 125.0f), FVector(190.0f, 0.7f, 2.5f), FRotator::ZeroRotator, Concrete, TEXT("WB_Outskirts_ChannelWall_North"));

		// One bridge preserves the north-south route while the channel breaks up east-west movement.
		SpawnBox(World, Origin + FVector(0.0f, 17100.0f, 70.0f), FVector(9.0f, 31.0f, 0.45f), FRotator::ZeroRotator, Concrete * 0.85f, TEXT("WB_Outskirts_ChannelBridge"));

		// Compact substation to the east of the bridge.
		SpawnBox(World, Origin + FVector(7800.0f, 15100.0f, 12.0f), FVector(28.0f, 22.0f, 0.08f), FRotator::ZeroRotator, Channel * 1.15f, TEXT("WB_Outskirts_SubstationPad"), false);
		for (int32 Unit = 0; Unit < 3; ++Unit)
		{
			SpawnBox(World, Origin + FVector(6500.0f + Unit * 1300.0f, 15100.0f, 180.0f), FVector(4.0f, 5.0f, 3.2f), FRotator::ZeroRotator, Steel, TEXT("WB_Outskirts_Transformer"), true, 0.72f, 0.42f);
		}

		// Tall radio/utility mast acts as an orientation landmark from most of the district.
		const FVector Mast = Origin + FVector(-7600.0f, 17600.0f, 0.0f);
		SpawnBox(World, Mast + FVector(0.0f, 0.0f, 2800.0f), FVector(0.32f, 0.32f, 56.0f), FRotator::ZeroRotator, Rust, TEXT("WB_Outskirts_RadioMast"), true, 0.70f, 0.55f);
		SpawnBox(World, Mast + FVector(0.0f, 0.0f, 3600.0f), FVector(5.0f, 0.18f, 0.18f), FRotator::ZeroRotator, Steel, TEXT("WB_Outskirts_MastCrossbar_01"), false, 0.70f, 0.55f);
		SpawnBox(World, Mast + FVector(0.0f, 0.0f, 4400.0f), FVector(3.4f, 0.18f, 0.18f), FRotator::ZeroRotator, Steel, TEXT("WB_Outskirts_MastCrossbar_02"), false, 0.70f, 0.55f);
	}

	void SpawnSouthernServiceZone(UWorld& World, const FVector& Origin)
	{
		const FLinearColor Yard(0.064f, 0.062f, 0.057f, 1.0f);
		const FLinearColor Warehouse(0.275f, 0.265f, 0.235f, 1.0f);
		const FLinearColor Rust(0.23f, 0.11f, 0.06f, 1.0f);
		const FLinearColor Tank(0.19f, 0.205f, 0.195f, 1.0f);
		const FLinearColor Concrete(0.28f, 0.265f, 0.235f, 1.0f);

		// Salvage/service yard gives the southern edge a low, industrial silhouette.
		SpawnBox(World, Origin + FVector(8500.0f, -15700.0f, 10.0f), FVector(50.0f, 30.0f, 0.08f), FRotator::ZeroRotator, Yard, TEXT("WB_Outskirts_SalvageYard"), false);
		SpawnBox(World, Origin + FVector(11600.0f, -17400.0f, 430.0f), FVector(22.0f, 12.0f, 8.0f), FRotator::ZeroRotator, Warehouse, TEXT("WB_Outskirts_ServiceDepot"));
		for (int32 Stack = 0; Stack < 4; ++Stack)
		{
			SpawnBox(World, Origin + FVector(5800.0f + Stack * 1200.0f, -16000.0f, 120.0f + (Stack % 2) * 45.0f), FVector(4.5f, 2.2f, 1.0f), FRotator(0.0f, Stack * 11.0f, 0.0f), Rust * (0.86f + Stack * 0.03f), TEXT("WB_Outskirts_ScrapStack"));
		}

		// Water-treatment tanks create a second recognizable utility landmark west of the road.
		const FVector Treatment = Origin + FVector(-7600.0f, -16600.0f, 0.0f);
		SpawnBox(World, Treatment + FVector(0.0f, 0.0f, 8.0f), FVector(36.0f, 30.0f, 0.06f), FRotator::ZeroRotator, Concrete * 0.75f, TEXT("WB_Outskirts_TreatmentPad"), false);
		SpawnProp(World, GetCylinder(), Treatment + FVector(-1100.0f, 0.0f, 310.0f), FVector(5.0f, 5.0f, 6.0f), FRotator::ZeroRotator, Tank, TEXT("WB_Outskirts_TreatmentTank_01"), true, 0.78f, 0.28f);
		SpawnProp(World, GetCylinder(), Treatment + FVector(1100.0f, 0.0f, 310.0f), FVector(5.0f, 5.0f, 6.0f), FRotator::ZeroRotator, Tank * 0.90f, TEXT("WB_Outskirts_TreatmentTank_02"), true, 0.80f, 0.26f);

		// A fallen service barrier marks the current southern boundary but leaves an obvious future route.
		SpawnBox(World, Origin + FVector(0.0f, -19800.0f, 70.0f), FVector(8.0f, 1.2f, 1.2f), FRotator(0.0f, 7.0f, 6.0f), Rust, TEXT("WB_Outskirts_SouthBarrier"));
	}

	void BuildOutskirts(UWorld& World, const FVector& Origin)
	{
		SpawnOuterGround(World, Origin);
		SpawnRoadConnections(World, Origin);
		SpawnEasternHighwayStrip(World, Origin);
		SpawnWesternResidentialFringe(World, Origin);
		SpawnNorthernUtilityCorridor(World, Origin);
		SpawnSouthernServiceZone(World, Origin);
	}
}

void UWildBoundOutskirtsExpansionSubsystem::OnWorldBeginPlay(UWorld& InWorld)
{
	Super::OnWorldBeginPlay(InWorld);

	if (!InWorld.IsGameWorld())
	{
		return;
	}

	TryBuildOutskirts();
	InWorld.GetTimerManager().SetTimer(
		OutskirtsBuildTimer,
		this,
		&UWildBoundOutskirtsExpansionSubsystem::TryBuildOutskirts,
		0.6f,
		true,
		0.20f);
}

void UWildBoundOutskirtsExpansionSubsystem::Deinitialize()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(OutskirtsBuildTimer);
	}
	Super::Deinitialize();
}

void UWildBoundOutskirtsExpansionSubsystem::TryBuildOutskirts()
{
	if (bOutskirtsBuilt)
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
		if (It->ActorHasTag(OutskirtsTag))
		{
			bOutskirtsBuilt = true;
			World->GetTimerManager().ClearTimer(OutskirtsBuildTimer);
			return;
		}
	}

	FVector TownOrigin;
	if (!FindTownOrigin(*World, TownOrigin))
	{
		return;
	}

	BuildOutskirts(*World, TownOrigin);
	bOutskirtsBuilt = true;
	World->GetTimerManager().ClearTimer(OutskirtsBuildTimer);
	UE_LOG(LogTemp, Log, TEXT("WildBound world: outskirts expanded around the town with highway, residential, utility, and service zones."));
}
