#include "WildBoundTownBlockout.h"

#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/StaticMeshActor.h"
#include "Engine/World.h"
#include "UObject/UObjectGlobals.h"

namespace
{
	UStaticMesh* GetBlockoutCube()
	{
		static TWeakObjectPtr<UStaticMesh> CubeMesh;
		if (!CubeMesh.IsValid())
		{
			CubeMesh = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube"));
		}

		return CubeMesh.Get();
	}

	AStaticMeshActor* SpawnBox(
		UWorld& World,
		const FVector& Location,
		const FVector& Scale,
		const FRotator& Rotation,
		const TCHAR* Label)
	{
		UStaticMesh* CubeMesh = GetBlockoutCube();
		if (!CubeMesh)
		{
			return nullptr;
		}

		AStaticMeshActor* Actor = World.SpawnActor<AStaticMeshActor>(Location, Rotation);
		if (!Actor)
		{
			return nullptr;
		}

		Actor->Tags.AddUnique(TEXT("WildBoundTownBlockout"));
#if WITH_EDITOR
		Actor->SetActorLabel(Label);
#endif

		UStaticMeshComponent* Mesh = Actor->GetStaticMeshComponent();
		Mesh->SetMobility(EComponentMobility::Movable);
		Mesh->SetStaticMesh(CubeMesh);
		Mesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
		Mesh->SetCollisionResponseToAllChannels(ECR_Block);
		Mesh->SetCastShadow(true);
		Actor->SetActorScale3D(Scale);

		return Actor;
	}
}

void WildBoundTownBlockout::Spawn(UWorld& World)
{
	for (TActorIterator<AStaticMeshActor> It(&World); It; ++It)
	{
		if (It->ActorHasTag(TEXT("WildBoundTownBlockout")))
		{
			return;
		}
	}

	const FVector Origin(3500.0f, 0.0f, 0.0f);

	// Main crossroad: broad enough for vehicles, debris, and future encounters.
	SpawnBox(World, Origin + FVector(0.0f, 0.0f, 5.0f), FVector(60.0f, 8.0f, 0.10f), FRotator::ZeroRotator, TEXT("WB_Road_EastWest"));
	SpawnBox(World, Origin + FVector(0.0f, 0.0f, 7.0f), FVector(8.0f, 60.0f, 0.10f), FRotator::ZeroRotator, TEXT("WB_Road_NorthSouth"));

	// Sidewalk edges give the intersection readable pedestrian scale.
	SpawnBox(World, Origin + FVector(0.0f, -600.0f, 16.0f), FVector(60.0f, 2.0f, 0.18f), FRotator::ZeroRotator, TEXT("WB_Sidewalk_South"));
	SpawnBox(World, Origin + FVector(0.0f, 600.0f, 16.0f), FVector(60.0f, 2.0f, 0.18f), FRotator::ZeroRotator, TEXT("WB_Sidewalk_North"));
	SpawnBox(World, Origin + FVector(-600.0f, 0.0f, 18.0f), FVector(2.0f, 60.0f, 0.18f), FRotator::ZeroRotator, TEXT("WB_Sidewalk_West"));
	SpawnBox(World, Origin + FVector(600.0f, 0.0f, 18.0f), FVector(2.0f, 60.0f, 0.18f), FRotator::ZeroRotator, TEXT("WB_Sidewalk_East"));

	// Four distinct building masses establish the first abandoned town intersection.
	SpawnBox(World, Origin + FVector(-1900.0f, -1750.0f, 600.0f), FVector(20.0f, 17.0f, 12.0f), FRotator(0.0f, 2.0f, 0.0f), TEXT("WB_Building_NW"));
	SpawnBox(World, Origin + FVector(1900.0f, -1900.0f, 900.0f), FVector(16.0f, 20.0f, 18.0f), FRotator(0.0f, -3.0f, 0.0f), TEXT("WB_Building_NE"));
	SpawnBox(World, Origin + FVector(-2050.0f, 1900.0f, 700.0f), FVector(22.0f, 16.0f, 14.0f), FRotator(0.0f, -2.0f, 0.0f), TEXT("WB_Building_SW"));
	SpawnBox(World, Origin + FVector(1950.0f, 1800.0f, 500.0f), FVector(18.0f, 18.0f, 10.0f), FRotator(0.0f, 4.0f, 0.0f), TEXT("WB_Building_SE"));

	// Abandoned vehicles break the clean road lines and create immediate cover/obstacles.
	SpawnBox(World, Origin + FVector(-900.0f, -180.0f, 80.0f), FVector(4.3f, 2.0f, 0.75f), FRotator(0.0f, 8.0f, 0.0f), TEXT("WB_WreckedCar_01"));
	SpawnBox(World, Origin + FVector(1250.0f, 230.0f, 90.0f), FVector(5.0f, 2.2f, 0.85f), FRotator(0.0f, -17.0f, 0.0f), TEXT("WB_WreckedCar_02"));
	SpawnBox(World, Origin + FVector(220.0f, 1450.0f, 115.0f), FVector(6.0f, 2.4f, 1.05f), FRotator(0.0f, 83.0f, 0.0f), TEXT("WB_AbandonedTruck"));

	// Crude barricade and scattered collapse sell abandonment even before final art assets arrive.
	SpawnBox(World, Origin + FVector(-150.0f, -1050.0f, 55.0f), FVector(3.5f, 0.45f, 0.55f), FRotator(0.0f, 28.0f, 0.0f), TEXT("WB_Barricade_01"));
	SpawnBox(World, Origin + FVector(220.0f, -1120.0f, 65.0f), FVector(3.0f, 0.50f, 0.65f), FRotator(4.0f, -18.0f, 2.0f), TEXT("WB_Barricade_02"));
	SpawnBox(World, Origin + FVector(760.0f, 1180.0f, 45.0f), FVector(1.8f, 1.0f, 0.35f), FRotator(12.0f, 21.0f, 5.0f), TEXT("WB_Debris_01"));
	SpawnBox(World, Origin + FVector(980.0f, 1320.0f, 70.0f), FVector(2.4f, 0.9f, 0.55f), FRotator(-8.0f, -11.0f, 9.0f), TEXT("WB_Debris_02"));
	SpawnBox(World, Origin + FVector(-1280.0f, 980.0f, 55.0f), FVector(2.0f, 1.3f, 0.45f), FRotator(9.0f, 31.0f, -6.0f), TEXT("WB_Debris_03"));

	// Sparse poles preserve vertical rhythm and will later become lights, signs, and utility props.
	SpawnBox(World, Origin + FVector(-720.0f, -720.0f, 300.0f), FVector(0.18f, 0.18f, 6.0f), FRotator::ZeroRotator, TEXT("WB_UtilityPole_01"));
	SpawnBox(World, Origin + FVector(720.0f, 720.0f, 300.0f), FVector(0.18f, 0.18f, 6.0f), FRotator(0.0f, 3.0f, 0.0f), TEXT("WB_UtilityPole_02"));

	UE_LOG(LogTemp, Log, TEXT("WildBound environment: first abandoned town intersection spawned."));
}
