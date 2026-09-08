#include "WildBoundTownBlockout.h"

#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/StaticMeshActor.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "TimerManager.h"
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

	void SpawnRuinedStorefront(UWorld& World, const FVector& Origin)
	{
		// South-east corner store. Its street-facing wall points toward the intersection (-Y).
		// It is intentionally assembled from individual structural pieces instead of one solid box,
		// giving us a genuinely enterable interior that can later receive final meshes/materials.
		const FVector Store = Origin + FVector(1950.0f, 1800.0f, 0.0f);

		// Structural shell: solid floor, rear/side walls, and an irregular surviving roof line.
		SpawnBox(World, Store + FVector(0.0f, 0.0f, 20.0f), FVector(18.0f, 18.0f, 0.18f), FRotator::ZeroRotator, TEXT("WB_Store_Floor"));
		SpawnBox(World, Store + FVector(0.0f, 880.0f, 500.0f), FVector(18.0f, 0.35f, 10.0f), FRotator::ZeroRotator, TEXT("WB_Store_BackWall"));
		SpawnBox(World, Store + FVector(-880.0f, 0.0f, 500.0f), FVector(0.35f, 18.0f, 10.0f), FRotator::ZeroRotator, TEXT("WB_Store_LeftWall"));
		SpawnBox(World, Store + FVector(880.0f, 0.0f, 500.0f), FVector(0.35f, 18.0f, 10.0f), FRotator::ZeroRotator, TEXT("WB_Store_RightWall"));
		SpawnBox(World, Store + FVector(-350.0f, 430.0f, 990.0f), FVector(11.0f, 9.0f, 0.22f), FRotator::ZeroRotator, TEXT("WB_Store_RoofRearLeft"));
		SpawnBox(World, Store + FVector(600.0f, 540.0f, 965.0f), FVector(6.5f, 6.5f, 0.22f), FRotator(3.0f, -2.0f, 0.0f), TEXT("WB_Store_RoofRearRight"));

		// Exposed roof ribs make the missing front roof section feel torn open rather than unfinished.
		SpawnBox(World, Store + FVector(-600.0f, -280.0f, 970.0f), FVector(0.22f, 11.0f, 0.22f), FRotator(0.0f, 2.0f, 0.0f), TEXT("WB_Store_RoofRib_01"));
		SpawnBox(World, Store + FVector(-50.0f, -260.0f, 955.0f), FVector(0.22f, 10.5f, 0.22f), FRotator(-2.0f, -1.0f, 0.0f), TEXT("WB_Store_RoofRib_02"));
		SpawnBox(World, Store + FVector(500.0f, -230.0f, 930.0f), FVector(0.22f, 10.0f, 0.22f), FRotator(-5.0f, 3.0f, 0.0f), TEXT("WB_Store_RoofRib_03"));

		// Blown-out storefront facade. The large center opening is wide enough to walk straight in,
		// while the remaining masonry and bent frame pieces create a readable ruined silhouette.
		SpawnBox(World, Store + FVector(-700.0f, -880.0f, 500.0f), FVector(3.6f, 0.35f, 10.0f), FRotator::ZeroRotator, TEXT("WB_Store_FacadeLeft"));
		SpawnBox(World, Store + FVector(720.0f, -880.0f, 500.0f), FVector(3.2f, 0.35f, 10.0f), FRotator::ZeroRotator, TEXT("WB_Store_FacadeRight"));
		SpawnBox(World, Store + FVector(0.0f, -880.0f, 900.0f), FVector(10.5f, 0.35f, 2.0f), FRotator(0.0f, -1.5f, -1.5f), TEXT("WB_Store_FacadeHeader"));
		SpawnBox(World, Store + FVector(-410.0f, -895.0f, 420.0f), FVector(0.18f, 0.22f, 4.8f), FRotator(-6.0f, 0.0f, 0.0f), TEXT("WB_Store_BentWindowFrameLeft"));
		SpawnBox(World, Store + FVector(405.0f, -895.0f, 390.0f), FVector(0.18f, 0.22f, 4.1f), FRotator(8.0f, 0.0f, 0.0f), TEXT("WB_Store_BentWindowFrameRight"));
		SpawnBox(World, Store + FVector(-30.0f, -925.0f, 735.0f), FVector(5.0f, 0.28f, 0.75f), FRotator(0.0f, -6.0f, -4.0f), TEXT("WB_Store_BrokenSignBand"));

		// Threshold rubble pulls the eye toward the entrance while leaving a clean walkable channel.
		SpawnBox(World, Store + FVector(-500.0f, -790.0f, 50.0f), FVector(1.7f, 1.0f, 0.35f), FRotator(12.0f, 18.0f, 5.0f), TEXT("WB_Store_EntryRubble_01"));
		SpawnBox(World, Store + FVector(520.0f, -760.0f, 42.0f), FVector(1.3f, 0.8f, 0.28f), FRotator(-8.0f, -27.0f, 3.0f), TEXT("WB_Store_EntryRubble_02"));
		SpawnBox(World, Store + FVector(690.0f, -620.0f, 70.0f), FVector(2.0f, 0.65f, 0.42f), FRotator(15.0f, 11.0f, 8.0f), TEXT("WB_Store_EntryRubble_03"));

		// Checkout counter creates a natural first loot/search point after entering.
		SpawnBox(World, Store + FVector(-220.0f, -120.0f, 62.0f), FVector(6.0f, 0.75f, 0.85f), FRotator(0.0f, -4.0f, 0.0f), TEXT("WB_Store_CounterBase"));
		SpawnBox(World, Store + FVector(-220.0f, -120.0f, 125.0f), FVector(6.4f, 1.15f, 0.18f), FRotator(0.0f, -4.0f, 0.0f), TEXT("WB_Store_CounterTop"));
		SpawnBox(World, Store + FVector(390.0f, -80.0f, 82.0f), FVector(1.5f, 1.0f, 0.32f), FRotator(9.0f, 17.0f, 11.0f), TEXT("WB_Store_BrokenCounterEnd"));

		// Rear wall shelving gives the room depth and a strong scavenging destination.
		for (int32 ShelfIndex = 0; ShelfIndex < 3; ++ShelfIndex)
		{
			const float ShelfZ = 110.0f + ShelfIndex * 180.0f;
			SpawnBox(World, Store + FVector(-430.0f, 760.0f, ShelfZ), FVector(5.2f, 1.1f, 0.16f), FRotator::ZeroRotator,
				ShelfIndex == 0 ? TEXT("WB_Store_BackShelf_01") : ShelfIndex == 1 ? TEXT("WB_Store_BackShelf_02") : TEXT("WB_Store_BackShelf_03"));
		}
		SpawnBox(World, Store + FVector(-900.0f + 500.0f, 760.0f, 340.0f), FVector(0.16f, 1.1f, 5.4f), FRotator::ZeroRotator, TEXT("WB_Store_ShelfPostLeft"));
		SpawnBox(World, Store + FVector(40.0f, 760.0f, 340.0f), FVector(0.16f, 1.1f, 5.4f), FRotator::ZeroRotator, TEXT("WB_Store_ShelfPostRight"));

		// A toppled aisle shelf forces a slight path choice and breaks up the rectangular interior.
		SpawnBox(World, Store + FVector(430.0f, 310.0f, 95.0f), FVector(5.0f, 0.7f, 0.18f), FRotator(9.0f, 27.0f, 6.0f), TEXT("WB_Store_ToppledShelfBase"));
		SpawnBox(World, Store + FVector(410.0f, 315.0f, 190.0f), FVector(4.6f, 0.18f, 1.8f), FRotator(13.0f, 27.0f, 3.0f), TEXT("WB_Store_ToppledShelfBack"));

		// Small rear stock-room partition, with an off-center doorway leading to a second search pocket.
		SpawnBox(World, Store + FVector(-560.0f, 470.0f, 420.0f), FVector(5.0f, 0.22f, 8.0f), FRotator::ZeroRotator, TEXT("WB_Store_PartitionLeft"));
		SpawnBox(World, Store + FVector(600.0f, 470.0f, 420.0f), FVector(3.2f, 0.22f, 8.0f), FRotator::ZeroRotator, TEXT("WB_Store_PartitionRight"));
		SpawnBox(World, Store + FVector(150.0f, 470.0f, 760.0f), FVector(2.2f, 0.22f, 1.2f), FRotator::ZeroRotator, TEXT("WB_Store_PartitionHeader"));

		// Interior collapse tells a simple story: part of the roof failed inward toward the stock room.
		SpawnBox(World, Store + FVector(560.0f, 600.0f, 500.0f), FVector(4.5f, 2.8f, 0.20f), FRotator(24.0f, -11.0f, 8.0f), TEXT("WB_Store_CollapsedRoofSlab"));
		SpawnBox(World, Store + FVector(700.0f, 530.0f, 240.0f), FVector(0.25f, 0.25f, 4.8f), FRotator(18.0f, -10.0f, 4.0f), TEXT("WB_Store_FallenSupport"));

		// Fine debris clusters make the floor feel damaged without blocking traversal.
		SpawnBox(World, Store + FVector(-610.0f, 260.0f, 38.0f), FVector(1.2f, 0.8f, 0.26f), FRotator(8.0f, 31.0f, 3.0f), TEXT("WB_Store_Debris_01"));
		SpawnBox(World, Store + FVector(-470.0f, 350.0f, 52.0f), FVector(0.9f, 0.7f, 0.38f), FRotator(-11.0f, -15.0f, 9.0f), TEXT("WB_Store_Debris_02"));
		SpawnBox(World, Store + FVector(220.0f, 650.0f, 34.0f), FVector(1.4f, 0.6f, 0.22f), FRotator(6.0f, 44.0f, -4.0f), TEXT("WB_Store_Debris_03"));
		SpawnBox(World, Store + FVector(340.0f, 710.0f, 45.0f), FVector(0.8f, 0.8f, 0.32f), FRotator(14.0f, -9.0f, 6.0f), TEXT("WB_Store_Debris_04"));
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

	APlayerController* PlayerController = World.GetFirstPlayerController();
	APawn* Pawn = PlayerController ? PlayerController->GetPawn() : nullptr;
	if (!Pawn)
	{
		// World subsystems can begin before the template pawn is possessed.
		// Retry next tick instead of guessing a fixed world-space spawn location.
		TWeakObjectPtr<UWorld> WeakWorld(&World);
		World.GetTimerManager().SetTimerForNextTick(FTimerDelegate::CreateLambda([WeakWorld]()
		{
			if (UWorld* RetryWorld = WeakWorld.Get())
			{
				WildBoundTownBlockout::Spawn(*RetryWorld);
			}
		}));
		return;
	}

	// Put the WildBound prototype roughly 1.4 km diagonally away from Epic's
	// First Person sample arena. This keeps the sample geometry completely out
	// of sight while still letting us reuse the map's sky/atmosphere setup.
	const FVector Origin = Pawn->GetActorLocation() + FVector(100000.0f, 100000.0f, 0.0f);

	SpawnBox(World, Origin + FVector(0.0f, 0.0f, -25.0f), FVector(80.0f, 80.0f, 0.50f), FRotator::ZeroRotator, TEXT("WB_TownFoundation"));

	// Main crossroad: broad enough for vehicles, debris, and future encounters.
	SpawnBox(World, Origin + FVector(0.0f, 0.0f, 5.0f), FVector(60.0f, 8.0f, 0.10f), FRotator::ZeroRotator, TEXT("WB_Road_EastWest"));
	SpawnBox(World, Origin + FVector(0.0f, 0.0f, 7.0f), FVector(8.0f, 60.0f, 0.10f), FRotator::ZeroRotator, TEXT("WB_Road_NorthSouth"));

	// Sidewalk edges give the intersection readable pedestrian scale.
	SpawnBox(World, Origin + FVector(0.0f, -600.0f, 16.0f), FVector(60.0f, 2.0f, 0.18f), FRotator::ZeroRotator, TEXT("WB_Sidewalk_South"));
	SpawnBox(World, Origin + FVector(0.0f, 600.0f, 16.0f), FVector(60.0f, 2.0f, 0.18f), FRotator::ZeroRotator, TEXT("WB_Sidewalk_North"));
	SpawnBox(World, Origin + FVector(-600.0f, 0.0f, 18.0f), FVector(2.0f, 60.0f, 0.18f), FRotator::ZeroRotator, TEXT("WB_Sidewalk_West"));
	SpawnBox(World, Origin + FVector(600.0f, 0.0f, 18.0f), FVector(2.0f, 60.0f, 0.18f), FRotator::ZeroRotator, TEXT("WB_Sidewalk_East"));

	// Three distant masses plus one detailed, enterable corner storefront establish the first block.
	SpawnBox(World, Origin + FVector(-1900.0f, -1750.0f, 600.0f), FVector(20.0f, 17.0f, 12.0f), FRotator(0.0f, 2.0f, 0.0f), TEXT("WB_Building_NW"));
	SpawnBox(World, Origin + FVector(1900.0f, -1900.0f, 900.0f), FVector(16.0f, 20.0f, 18.0f), FRotator(0.0f, -3.0f, 0.0f), TEXT("WB_Building_NE"));
	SpawnBox(World, Origin + FVector(-2050.0f, 1900.0f, 700.0f), FVector(22.0f, 16.0f, 14.0f), FRotator(0.0f, -2.0f, 0.0f), TEXT("WB_Building_SW"));
	SpawnRuinedStorefront(World, Origin);

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

	// Put the player directly on the new street so there is no ambiguity about which
	// environment is being tested.
	Pawn->SetActorLocation(Origin + FVector(0.0f, 0.0f, 140.0f), false, nullptr, ETeleportType::TeleportPhysics);

	UE_LOG(LogTemp, Log, TEXT("WildBound environment: player moved to isolated abandoned town at %s."), *Origin.ToCompactString());
}
