#include "WildBoundPryableSubsystem.h"

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
	const FName PryableSetTag(TEXT("WildBoundPryables"));
	const FName InteractableTag(TEXT("WBInteractable"));
	const FName PryLockedTag(TEXT("WBPryLocked"));
	const FName PryContainerTag(TEXT("WBPryContainer"));
	const FName PryAccessTag(TEXT("WBPryAccess"));
	const FName IndustrialPoolTag(TEXT("WBLootIndustrial"));
	const FName CivicPoolTag(TEXT("WBLootCivic"));
	const FName CrateTag(TEXT("WBContainerCrate"));
	const FName LockerTag(TEXT("WBContainerLocker"));
	const FName CommercialGateGroupTag(TEXT("WBPryGroupCommercialGate"));

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

	AStaticMeshActor* SpawnBox(
		UWorld& World,
		const FVector& Location,
		const FVector& Scale,
		const FRotator& Rotation,
		const FLinearColor& Color,
		const TCHAR* Label,
		bool bCollidable = true,
		float Roughness = 0.90f,
		float Metallic = 0.0f)
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

		Actor->Tags.AddUnique(PryableSetTag);
#if WITH_EDITOR
		Actor->SetActorLabel(Label);
#endif

		UStaticMeshComponent* Mesh = Actor->GetStaticMeshComponent();
		Mesh->SetMobility(EComponentMobility::Movable);
		Mesh->SetStaticMesh(Cube);
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

	void MarkLockedContainer(AStaticMeshActor& Actor, const FName& PoolTag, const FName& ContainerTag)
	{
		Actor.Tags.AddUnique(InteractableTag);
		Actor.Tags.AddUnique(PryLockedTag);
		Actor.Tags.AddUnique(PryContainerTag);
		Actor.Tags.AddUnique(PoolTag);
		Actor.Tags.AddUnique(ContainerTag);
	}

	void SpawnSealedCrate(UWorld& World, const FVector& Location)
	{
		const FLinearColor Wood(0.19f, 0.125f, 0.065f, 1.0f);
		const FLinearColor Steel(0.11f, 0.095f, 0.075f, 1.0f);
		const FLinearColor Rust(0.28f, 0.095f, 0.035f, 1.0f);

		AStaticMeshActor* Body = SpawnBox(World, Location + FVector(0.0f, 0.0f, 62.0f), FVector(1.9f, 1.45f, 1.18f), FRotator(0.0f, -8.0f, 0.0f), Wood, TEXT("WB_Pry_ServiceGarage_SealedCrate"), true, 0.96f);
		if (!Body)
		{
			return;
		}
		MarkLockedContainer(*Body, IndustrialPoolTag, CrateTag);

		SpawnBox(World, Location + FVector(0.0f, 0.0f, 126.0f), FVector(2.02f, 1.57f, 0.13f), FRotator(0.0f, -8.0f, 0.0f), Steel, TEXT("WB_Pry_SealedCrate_Lid"), false, 0.78f, 0.38f);
		SpawnBox(World, Location + FVector(0.0f, -150.0f, 82.0f), FVector(0.42f, 0.10f, 0.72f), FRotator(0.0f, -8.0f, 0.0f), Rust, TEXT("WB_Pry_SealedCrate_Lock"), false, 0.78f, 0.55f);
	}

	void SpawnSealedLocker(UWorld& World, const FVector& Location)
	{
		const FLinearColor Steel(0.20f, 0.215f, 0.205f, 1.0f);
		const FLinearColor Dark(0.055f, 0.060f, 0.057f, 1.0f);
		const FLinearColor Rust(0.26f, 0.085f, 0.035f, 1.0f);

		AStaticMeshActor* Body = SpawnBox(World, Location + FVector(0.0f, 0.0f, 125.0f), FVector(1.0f, 0.78f, 2.5f), FRotator(0.0f, 4.0f, 0.0f), Steel, TEXT("WB_Pry_Municipal_SealedLocker"), true, 0.82f, 0.30f);
		if (!Body)
		{
			return;
		}
		MarkLockedContainer(*Body, CivicPoolTag, LockerTag);

		for (int32 Index = 0; Index < 3; ++Index)
		{
			SpawnBox(World, Location + FVector(0.0f, -82.0f, 175.0f - Index * 30.0f), FVector(0.48f, 0.045f, 0.035f), FRotator(0.0f, 4.0f, 0.0f), Dark, TEXT("WB_Pry_SealedLocker_Vent"), false, 0.72f, 0.35f);
		}
		SpawnBox(World, Location + FVector(58.0f, -86.0f, 105.0f), FVector(0.09f, 0.08f, 0.42f), FRotator(0.0f, 4.0f, 0.0f), Rust, TEXT("WB_Pry_SealedLocker_Lock"), false, 0.74f, 0.50f);
	}

	void SpawnCommercialMaintenanceGate(UWorld& World, const FVector& Location)
	{
		const FLinearColor Steel(0.105f, 0.095f, 0.080f, 1.0f);
		const FLinearColor Rust(0.24f, 0.075f, 0.025f, 1.0f);

		AStaticMeshActor* MainBar = SpawnBox(World, Location + FVector(0.0f, 0.0f, 115.0f), FVector(3.15f, 0.20f, 0.24f), FRotator(0.0f, 0.0f, 6.0f), Rust, TEXT("WB_Pry_CommercialGate_MainBar"), true, 0.84f, 0.46f);
		if (!MainBar)
		{
			return;
		}

		MainBar->Tags.AddUnique(InteractableTag);
		MainBar->Tags.AddUnique(PryLockedTag);
		MainBar->Tags.AddUnique(PryAccessTag);
		MainBar->Tags.AddUnique(CommercialGateGroupTag);

		for (int32 Index = -2; Index <= 2; ++Index)
		{
			AStaticMeshActor* Slat = SpawnBox(
				World,
				Location + FVector(Index * 120.0f, 0.0f, 110.0f),
				FVector(0.13f, 0.18f, 2.2f),
				FRotator(0.0f, 0.0f, Index * 2.0f),
				Steel,
				TEXT("WB_Pry_CommercialGate_Slat"),
				true,
				0.82f,
				0.42f);
			if (Slat)
			{
				Slat->Tags.AddUnique(CommercialGateGroupTag);
			}
		}

		AStaticMeshActor* CrossBrace = SpawnBox(World, Location + FVector(0.0f, 0.0f, 205.0f), FVector(3.0f, 0.17f, 0.15f), FRotator(0.0f, 0.0f, -5.0f), Steel, TEXT("WB_Pry_CommercialGate_CrossBrace"), true, 0.82f, 0.42f);
		if (CrossBrace)
		{
			CrossBrace->Tags.AddUnique(CommercialGateGroupTag);
		}
	}

	void BuildPryables(UWorld& World, const FVector& Origin)
	{
		SpawnSealedCrate(World, Origin + FVector(3950.0f, -7100.0f, 0.0f));
		SpawnSealedLocker(World, Origin + FVector(8350.0f, 6750.0f, 0.0f));
		SpawnCommercialMaintenanceGate(World, Origin + FVector(4100.0f, 5200.0f, 0.0f));
	}
}

void UWildBoundPryableSubsystem::OnWorldBeginPlay(UWorld& InWorld)
{
	Super::OnWorldBeginPlay(InWorld);

	if (!InWorld.IsGameWorld())
	{
		return;
	}

	TryBuildPryables();
	InWorld.GetTimerManager().SetTimer(
		PryableBuildTimer,
		this,
		&UWildBoundPryableSubsystem::TryBuildPryables,
		0.5f,
		true,
		0.10f);
}

void UWildBoundPryableSubsystem::Deinitialize()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(PryableBuildTimer);
	}

	Super::Deinitialize();
}

void UWildBoundPryableSubsystem::TryBuildPryables()
{
	if (bPryablesBuilt)
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
		if (It->ActorHasTag(PryableSetTag))
		{
			bPryablesBuilt = true;
			World->GetTimerManager().ClearTimer(PryableBuildTimer);
			return;
		}
	}

	FVector TownOrigin;
	if (!FindTownOrigin(*World, TownOrigin))
	{
		return;
	}

	BuildPryables(*World, TownOrigin);
	bPryablesBuilt = true;
	World->GetTimerManager().ClearTimer(PryableBuildTimer);
	UE_LOG(LogTemp, Log, TEXT("WildBound crowbar: sealed containers and maintenance gate placed around town."));
}
