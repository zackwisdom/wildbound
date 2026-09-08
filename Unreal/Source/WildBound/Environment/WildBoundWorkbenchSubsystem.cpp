#include "WildBoundWorkbenchSubsystem.h"

#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/StaticMeshActor.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "UObject/UObjectGlobals.h"

namespace
{
	const FName TownTag(TEXT("WildBoundTownBlockout"));
	const FName WorkbenchTag(TEXT("WBWorkbench"));
	const FName WorkbenchSetTag(TEXT("WildBoundWorkbenchSet"));

	UStaticMesh* GetCube()
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

	AStaticMeshActor* SpawnBenchPiece(
		UWorld& World,
		const FVector& Location,
		const FVector& Scale,
		const FRotator& Rotation,
		const FLinearColor& Color,
		const TCHAR* Label,
		bool bWorkbenchSurface = false,
		float Roughness = 0.84f,
		float Metallic = 0.15f)
	{
		UStaticMesh* Cube = GetCube();
		if (!Cube)
		{
			return nullptr;
		}

		AStaticMeshActor* Actor = World.SpawnActor<AStaticMeshActor>(Location, Rotation);
		if (!Actor)
		{
			return nullptr;
		}

		Actor->Tags.AddUnique(WorkbenchSetTag);
		if (bWorkbenchSurface)
		{
			Actor->Tags.AddUnique(WorkbenchTag);
		}

#if WITH_EDITOR
		Actor->SetActorLabel(Label);
#endif

		UStaticMeshComponent* Mesh = Actor->GetStaticMeshComponent();
		Mesh->SetMobility(EComponentMobility::Movable);
		Mesh->SetStaticMesh(Cube);
		Mesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
		Mesh->SetCollisionResponseToAllChannels(ECR_Block);
		Mesh->SetCastShadow(true);
		ApplyMaterial(*Mesh, Color, Roughness, Metallic);
		Actor->SetActorScale3D(Scale);
		return Actor;
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

	void SpawnWorkbench(UWorld& World, const FVector& BaseLocation, float Yaw, const TCHAR* LabelStem)
	{
		const FRotator Rotation(0.0f, Yaw, 0.0f);
		const FLinearColor Wood(0.22f, 0.145f, 0.075f, 1.0f);
		const FLinearColor DarkWood(0.12f, 0.075f, 0.04f, 1.0f);
		const FLinearColor Steel(0.16f, 0.17f, 0.17f, 1.0f);
		const FLinearColor Rust(0.22f, 0.09f, 0.045f, 1.0f);
		const FLinearColor Pegboard(0.19f, 0.18f, 0.15f, 1.0f);

		const FVector Forward = Rotation.RotateVector(FVector(1.0f, 0.0f, 0.0f));
		const FVector Right = Rotation.RotateVector(FVector(0.0f, 1.0f, 0.0f));

		SpawnBenchPiece(
			World,
			BaseLocation + FVector(0.0f, 0.0f, 92.0f),
			FVector(1.80f, 0.70f, 0.12f),
			Rotation,
			Wood,
			LabelStem,
			true,
			0.88f,
			0.02f);

		for (int32 XSign : {-1, 1})
		{
			for (int32 YSign : {-1, 1})
			{
				const FVector Offset = Forward * (XSign * 145.0f) + Right * (YSign * 45.0f);
				SpawnBenchPiece(
					World,
					BaseLocation + Offset + FVector(0.0f, 0.0f, 43.0f),
					FVector(0.11f, 0.11f, 0.86f),
					Rotation,
					Steel,
					TEXT("WB_Workbench_Leg"),
					false,
					0.68f,
					0.50f);
			}
		}

		SpawnBenchPiece(
			World,
			BaseLocation + FVector(0.0f, 0.0f, 38.0f),
			FVector(1.45f, 0.52f, 0.08f),
			Rotation,
			DarkWood,
			TEXT("WB_Workbench_LowerShelf"),
			false,
			0.90f,
			0.02f);

		const FVector BackOffset = Right * 61.0f;
		SpawnBenchPiece(
			World,
			BaseLocation + BackOffset + FVector(0.0f, 0.0f, 160.0f),
			FVector(1.65f, 0.07f, 0.72f),
			Rotation,
			Pegboard,
			TEXT("WB_Workbench_Pegboard"),
			false,
			0.92f,
			0.03f);

		SpawnBenchPiece(
			World,
			BaseLocation + Forward * 105.0f - Right * 12.0f + FVector(0.0f, 0.0f, 116.0f),
			FVector(0.34f, 0.28f, 0.18f),
			Rotation,
			Steel,
			TEXT("WB_Workbench_Vise"),
			false,
			0.60f,
			0.62f);

		SpawnBenchPiece(
			World,
			BaseLocation - Forward * 65.0f - Right * 18.0f + FVector(0.0f, 0.0f, 113.0f),
			FVector(0.56f, 0.08f, 0.06f),
			FRotator(0.0f, Yaw + 18.0f, -5.0f),
			Rust,
			TEXT("WB_Workbench_Tool"),
			false,
			0.72f,
			0.48f);
	}
}

void UWildBoundWorkbenchSubsystem::OnWorldBeginPlay(UWorld& InWorld)
{
	Super::OnWorldBeginPlay(InWorld);

	if (!InWorld.IsGameWorld())
	{
		return;
	}

	TrySpawnWorkbenches();
	InWorld.GetTimerManager().SetTimer(
		WorkbenchSpawnTimer,
		this,
		&UWildBoundWorkbenchSubsystem::TrySpawnWorkbenches,
		0.5f,
		true,
		0.15f);
}

void UWildBoundWorkbenchSubsystem::Deinitialize()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(WorkbenchSpawnTimer);
	}

	Super::Deinitialize();
}

void UWildBoundWorkbenchSubsystem::TrySpawnWorkbenches()
{
	if (bWorkbenchesSpawned)
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
		if (It->ActorHasTag(WorkbenchSetTag))
		{
			bWorkbenchesSpawned = true;
			World->GetTimerManager().ClearTimer(WorkbenchSpawnTimer);
			return;
		}
	}

	FVector TownOrigin;
	if (!FindTownOrigin(*World, TownOrigin))
	{
		return;
	}

	// Service garage: primary early-game crafting station.
	SpawnWorkbench(
		*World,
		TownOrigin + FVector(3450.0f, -6325.0f, 0.0f),
		0.0f,
		TEXT("WB_Workbench_ServiceGarage"));

	// Warehouse loading yard: industrial crafting stop on the south-east side.
	SpawnWorkbench(
		*World,
		TownOrigin + FVector(8350.0f, -5050.0f, 0.0f),
		90.0f,
		TEXT("WB_Workbench_Warehouse"));

	// Commercial back alley maintenance bench gives the east side another useful station.
	SpawnWorkbench(
		*World,
		TownOrigin + FVector(4700.0f, 3400.0f, 0.0f),
		90.0f,
		TEXT("WB_Workbench_MarketMaintenance"));

	bWorkbenchesSpawned = true;
	World->GetTimerManager().ClearTimer(WorkbenchSpawnTimer);
	UE_LOG(LogTemp, Log, TEXT("WildBound crafting: three physical town workbenches placed and ready."));
}
