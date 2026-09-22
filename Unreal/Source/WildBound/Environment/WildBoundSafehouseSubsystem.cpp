#include "WildBoundSafehouseSubsystem.h"

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
	const FName SafehouseSetTag(TEXT("WildBoundSafehouse"));
	const FName InteractableTag(TEXT("WBInteractable"));
	const FName ContainerTag(TEXT("WBTypeContainer"));
	const FName SearchedContainerTag(TEXT("WBContainerSearched"));
	const FName CrateContainerTag(TEXT("WBContainerCrate"));
	const FName SafehouseStashTag(TEXT("WBSafehouseStash"));
	const FName SafehouseBedTag(TEXT("WBSafehouseBed"));
	const FName SafehouseSaveTag(TEXT("WBSafehouseSavePoint"));
	const FName SafehouseSpawnAnchorTag(TEXT("WBSafehouseSpawnAnchor"));
	const FName WorkbenchTag(TEXT("WBWorkbench"));

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

	AStaticMeshActor* SpawnPiece(
		UWorld& World,
		UStaticMesh* StaticMesh,
		const FVector& Location,
		const FVector& Scale,
		const FRotator& Rotation,
		const FLinearColor& Color,
		const TCHAR* Label,
		bool bCollidable = true,
		float Roughness = 0.9f,
		float Metallic = 0.0f)
	{
		if (!StaticMesh)
		{
			return nullptr;
		}

		AStaticMeshActor* Actor = World.SpawnActor<AStaticMeshActor>(Location, Rotation);
		if (!Actor)
		{
			return nullptr;
		}

		Actor->Tags.AddUnique(SafehouseSetTag);
#if WITH_EDITOR
		Actor->SetActorLabel(Label);
#endif

		UStaticMeshComponent* Mesh = Actor->GetStaticMeshComponent();
		Mesh->SetMobility(EComponentMobility::Movable);
		Mesh->SetStaticMesh(StaticMesh);
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

	void SpawnSign(UWorld& World, const FVector& Location)
	{
		ATextRenderActor* TextActor = World.SpawnActor<ATextRenderActor>(
			Location,
			FRotator(0.0f, 0.0f, 0.0f));
		if (!TextActor)
		{
			return;
		}

		TextActor->Tags.AddUnique(SafehouseSetTag);
#if WITH_EDITOR
		TextActor->SetActorLabel(TEXT("WB_Safehouse_Sign"));
#endif

		if (UTextRenderComponent* Text = TextActor->GetTextRender())
		{
			Text->SetText(FText::FromString(TEXT("FIELD SHELTER\nKEEP THIS PLACE CLEAN")));
			Text->SetTextRenderColor(FColor(165, 154, 112));
			Text->SetWorldSize(22.0f);
			Text->SetHorizontalAlignment(EHTA_Center);
			Text->SetCastShadow(false);
		}
	}

	void SpawnSafehouse(UWorld& World, const FVector& TownOrigin)
	{
		const FVector Base = TownOrigin + FVector(0.0f, -3375.0f, 0.0f);

		const FLinearColor Concrete(0.245f, 0.235f, 0.205f, 1.0f);
		const FLinearColor ScrapSteel(0.12f, 0.125f, 0.115f, 1.0f);
		const FLinearColor Rust(0.235f, 0.105f, 0.052f, 1.0f);
		const FLinearColor Wood(0.235f, 0.16f, 0.085f, 1.0f);
		const FLinearColor DarkWood(0.105f, 0.070f, 0.042f, 1.0f);
		const FLinearColor Canvas(0.255f, 0.255f, 0.195f, 1.0f);
		const FLinearColor Mattress(0.235f, 0.245f, 0.22f, 1.0f);
		const FLinearColor Olive(0.17f, 0.19f, 0.145f, 1.0f);
		const FLinearColor Charcoal(0.045f, 0.050f, 0.047f, 1.0f);

		// Raised slab and three-sided scrap shelter. Front faces north toward town.
		AStaticMeshActor* Floor = SpawnPiece(World, GetCube(), Base + FVector(0.0f, 0.0f, 12.0f), FVector(7.0f, 5.2f, 0.12f), FRotator::ZeroRotator, Concrete, TEXT("WB_Safehouse_Floor"), true, 0.98f);
		if (Floor)
		{
			Floor->Tags.AddUnique(SafehouseSpawnAnchorTag);
		}
		SpawnPiece(World, GetCube(), Base + FVector(0.0f, -255.0f, 205.0f), FVector(7.0f, 0.15f, 4.1f), FRotator::ZeroRotator, ScrapSteel, TEXT("WB_Safehouse_BackWall"), true, 0.78f, 0.28f);
		SpawnPiece(World, GetCube(), Base + FVector(-345.0f, -20.0f, 205.0f), FVector(0.15f, 4.7f, 4.1f), FRotator::ZeroRotator, ScrapSteel, TEXT("WB_Safehouse_LeftWall"), true, 0.78f, 0.28f);
		SpawnPiece(World, GetCube(), Base + FVector(345.0f, -20.0f, 205.0f), FVector(0.15f, 4.7f, 4.1f), FRotator::ZeroRotator, ScrapSteel, TEXT("WB_Safehouse_RightWall"), true, 0.78f, 0.28f);
		SpawnPiece(World, GetCube(), Base + FVector(0.0f, -40.0f, 425.0f), FVector(7.15f, 5.0f, 0.10f), FRotator(0.0f, 0.0f, -2.0f), Canvas, TEXT("WB_Safehouse_Roof"), true, 0.96f);

		// Bent support posts and a rough windbreak make it feel repaired, not pristine.
		for (float X : {-315.0f, 315.0f})
		{
			SpawnPiece(World, GetCube(), Base + FVector(X, 210.0f, 205.0f), FVector(0.11f, 0.11f, 4.1f), FRotator(0.0f, X < 0.0f ? -3.0f : 4.0f, X < 0.0f ? 2.0f : -2.0f), Rust, TEXT("WB_Safehouse_FrontPost"), true, 0.72f, 0.48f);
		}
		SpawnPiece(World, GetCube(), Base + FVector(-225.0f, 245.0f, 92.0f), FVector(2.3f, 0.10f, 1.7f), FRotator(0.0f, 4.0f, 0.0f), Wood, TEXT("WB_Safehouse_Windbreak"), true, 0.91f);

		// Bed / rest point.
		SpawnPiece(World, GetCube(), Base + FVector(-175.0f, -125.0f, 48.0f), FVector(2.35f, 0.92f, 0.18f), FRotator::ZeroRotator, DarkWood, TEXT("WB_Safehouse_BedFrame"), true, 0.88f);
		AStaticMeshActor* Bed = SpawnPiece(World, GetCube(), Base + FVector(-175.0f, -125.0f, 72.0f), FVector(2.18f, 0.82f, 0.20f), FRotator::ZeroRotator, Mattress, TEXT("WB_Safehouse_Cot"), true, 0.96f);
		if (Bed)
		{
			Bed->Tags.AddUnique(InteractableTag);
			Bed->Tags.AddUnique(SafehouseBedTag);
		}
		SpawnPiece(World, GetCube(), Base + FVector(-285.0f, -125.0f, 92.0f), FVector(0.42f, 0.78f, 0.16f), FRotator(0.0f, 3.0f, 0.0f), Olive, TEXT("WB_Safehouse_RolledBlanket"), false, 0.96f);

		// Permanent empty stash using the normal two-way storage UI.
		AStaticMeshActor* Stash = SpawnPiece(World, GetCube(), Base + FVector(205.0f, -150.0f, 55.0f), FVector(1.35f, 0.92f, 0.55f), FRotator::ZeroRotator, Olive, TEXT("WB_Safehouse_Stash"), true, 0.84f, 0.22f);
		if (Stash)
		{
			Stash->Tags.AddUnique(InteractableTag);
			Stash->Tags.AddUnique(ContainerTag);
			Stash->Tags.AddUnique(SearchedContainerTag);
			Stash->Tags.AddUnique(CrateContainerTag);
			Stash->Tags.AddUnique(SafehouseStashTag);
		}
		SpawnPiece(World, GetCube(), Base + FVector(205.0f, -150.0f, 104.0f), FVector(1.42f, 0.98f, 0.08f), FRotator::ZeroRotator, ScrapSteel, TEXT("WB_Safehouse_StashLid"), false, 0.72f, 0.38f);

		// Compact workbench. The top carries the canonical WBWorkbench tag.
		SpawnPiece(World, GetCube(), Base + FVector(170.0f, 70.0f, 78.0f), FVector(1.75f, 0.62f, 0.11f), FRotator::ZeroRotator, Wood, TEXT("WB_Safehouse_WorkbenchTop"), true, 0.88f);
		AStaticMeshActor* BenchSurface = SpawnPiece(World, GetCube(), Base + FVector(170.0f, 70.0f, 92.0f), FVector(1.72f, 0.60f, 0.05f), FRotator::ZeroRotator, DarkWood, TEXT("WB_Safehouse_WorkbenchSurface"), true, 0.84f);
		if (BenchSurface)
		{
			BenchSurface->Tags.AddUnique(WorkbenchTag);
		}
		for (float X : {60.0f, 280.0f})
		{
			SpawnPiece(World, GetCube(), Base + FVector(X, 70.0f, 38.0f), FVector(0.10f, 0.10f, 0.76f), FRotator::ZeroRotator, ScrapSteel, TEXT("WB_Safehouse_WorkbenchLeg"), true, 0.70f, 0.45f);
		}
		SpawnPiece(World, GetCube(), Base + FVector(235.0f, 62.0f, 112.0f), FVector(0.44f, 0.08f, 0.05f), FRotator(0.0f, 18.0f, -5.0f), Rust, TEXT("WB_Safehouse_BenchTool"), false, 0.70f, 0.45f);

		// Manual save / field log station.
		SpawnPiece(World, GetCube(), Base + FVector(-40.0f, -235.0f, 155.0f), FVector(1.10f, 0.12f, 0.75f), FRotator::ZeroRotator, Charcoal, TEXT("WB_Safehouse_LogBoard"), true, 0.80f, 0.12f);
		AStaticMeshActor* SavePoint = SpawnPiece(World, GetCube(), Base + FVector(-40.0f, -220.0f, 160.0f), FVector(0.72f, 0.08f, 0.48f), FRotator::ZeroRotator, Canvas, TEXT("WB_Safehouse_FieldLog"), true, 0.96f);
		if (SavePoint)
		{
			SavePoint->Tags.AddUnique(InteractableTag);
			SavePoint->Tags.AddUnique(SafehouseSaveTag);
		}

		// Low-tech warmth/details without a neon "safe zone" look.
		SpawnPiece(World, GetCylinder(), Base + FVector(-35.0f, 110.0f, 44.0f), FVector(0.42f, 0.42f, 0.44f), FRotator::ZeroRotator, Rust, TEXT("WB_Safehouse_Brazier"), true, 0.80f, 0.42f);
		SpawnPiece(World, GetCube(), Base + FVector(-35.0f, 110.0f, 92.0f), FVector(0.60f, 0.60f, 0.06f), FRotator(0.0f, 9.0f, 0.0f), Charcoal, TEXT("WB_Safehouse_BrazierGrate"), false, 0.86f, 0.32f);

		SpawnSign(World, Base + FVector(0.0f, -275.0f, 325.0f));
	}
}

void UWildBoundSafehouseSubsystem::OnWorldBeginPlay(UWorld& InWorld)
{
	Super::OnWorldBeginPlay(InWorld);

	if (!InWorld.IsGameWorld())
	{
		return;
	}

	TrySpawnSafehouse();
	InWorld.GetTimerManager().SetTimer(
		SafehouseSpawnTimer,
		this,
		&UWildBoundSafehouseSubsystem::TrySpawnSafehouse,
		0.5f,
		true,
		0.15f);
}

void UWildBoundSafehouseSubsystem::Deinitialize()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(SafehouseSpawnTimer);
	}

	Super::Deinitialize();
}

void UWildBoundSafehouseSubsystem::TrySpawnSafehouse()
{
	if (bSafehouseSpawned)
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
		if (It->ActorHasTag(SafehouseSetTag))
		{
			bSafehouseSpawned = true;
			World->GetTimerManager().ClearTimer(SafehouseSpawnTimer);
			return;
		}
	}

	FVector TownOrigin;
	if (!FindTownOrigin(*World, TownOrigin))
	{
		return;
	}

	SpawnSafehouse(*World, TownOrigin);
	bSafehouseSpawned = true;
	World->GetTimerManager().ClearTimer(SafehouseSpawnTimer);
	UE_LOG(LogTemp, Log, TEXT("WildBound safehouse: south field shelter spawned with stash, cot, workbench, and field log."));
}
