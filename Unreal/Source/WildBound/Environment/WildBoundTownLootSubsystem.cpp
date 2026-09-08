#include "WildBoundTownLootSubsystem.h"

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
	const FName TownLootTag(TEXT("WildBoundTownLoot"));
	const FName InteractableTag(TEXT("WBInteractable"));
	const FName ContainerTypeTag(TEXT("WBTypeContainer"));

	const FName MedicalPoolTag(TEXT("WBLootMedical"));
	const FName MarketPoolTag(TEXT("WBLootMarket"));
	const FName ResidentialPoolTag(TEXT("WBLootResidential"));
	const FName IndustrialPoolTag(TEXT("WBLootIndustrial"));
	const FName CivicPoolTag(TEXT("WBLootCivic"));
	const FName GeneralPoolTag(TEXT("WBLootGeneral"));

	const FName CrateTag(TEXT("WBContainerCrate"));
	const FName CabinetTag(TEXT("WBContainerCabinet"));
	const FName LockerTag(TEXT("WBContainerLocker"));
	const FName DumpsterTag(TEXT("WBContainerDumpster"));
	const FName ToolboxTag(TEXT("WBContainerToolbox"));
	const FName CoolerTag(TEXT("WBContainerCooler"));

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

	AStaticMeshActor* SpawnLootBox(
		UWorld& World,
		const FVector& Location,
		const FVector& Scale,
		const FRotator& Rotation,
		const FLinearColor& Color,
		const TCHAR* Label,
		bool bCollidable,
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

		Actor->Tags.AddUnique(TownLootTag);
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

	void MarkContainer(AStaticMeshActor& Actor, const FName& LootPoolTag, const FName& ContainerTag)
	{
		Actor.Tags.AddUnique(InteractableTag);
		Actor.Tags.AddUnique(ContainerTypeTag);
		Actor.Tags.AddUnique(LootPoolTag);
		Actor.Tags.AddUnique(ContainerTag);
	}

	void SpawnCrate(UWorld& World, const FVector& Location, const FRotator& Rotation, const FName& PoolTag, const TCHAR* Label)
	{
		const FLinearColor Wood(0.22f, 0.155f, 0.085f, 1.0f);
		const FLinearColor Band(0.10f, 0.075f, 0.045f, 1.0f);

		AStaticMeshActor* Body = SpawnLootBox(World, Location + FVector(0.0f, 0.0f, 55.0f), FVector(1.7f, 1.35f, 1.05f), Rotation, Wood, Label, true, 0.95f);
		if (!Body)
		{
			return;
		}
		MarkContainer(*Body, PoolTag, CrateTag);

		SpawnLootBox(World, Location + FVector(0.0f, 0.0f, 112.0f), FVector(1.82f, 1.47f, 0.12f), Rotation, Band, TEXT("WB_Loot_CrateLid"), false, 0.86f, 0.12f);
		SpawnLootBox(World, Location + FVector(-70.0f, 0.0f, 55.0f), FVector(0.10f, 1.42f, 1.08f), Rotation, Band, TEXT("WB_Loot_CrateBandL"), false, 0.86f, 0.10f);
		SpawnLootBox(World, Location + FVector(70.0f, 0.0f, 55.0f), FVector(0.10f, 1.42f, 1.08f), Rotation, Band, TEXT("WB_Loot_CrateBandR"), false, 0.86f, 0.10f);
	}

	void SpawnCabinet(UWorld& World, const FVector& Location, const FRotator& Rotation, const FName& PoolTag, const TCHAR* Label)
	{
		const FLinearColor Paint(0.34f, 0.34f, 0.30f, 1.0f);
		const FLinearColor DarkMetal(0.075f, 0.075f, 0.068f, 1.0f);

		AStaticMeshActor* Body = SpawnLootBox(World, Location + FVector(0.0f, 0.0f, 105.0f), FVector(1.25f, 0.65f, 2.1f), Rotation, Paint, Label, true, 0.82f, 0.22f);
		if (!Body)
		{
			return;
		}
		MarkContainer(*Body, PoolTag, CabinetTag);

		SpawnLootBox(World, Location + FVector(0.0f, -68.0f, 105.0f), FVector(1.05f, 0.05f, 1.9f), Rotation, Paint * 0.88f, TEXT("WB_Loot_CabinetDoor"), false, 0.80f, 0.20f);
		SpawnLootBox(World, Location + FVector(62.0f, -75.0f, 105.0f), FVector(0.05f, 0.05f, 0.28f), Rotation, DarkMetal, TEXT("WB_Loot_CabinetHandle"), false, 0.62f, 0.52f);
	}

	void SpawnLocker(UWorld& World, const FVector& Location, const FRotator& Rotation, const FName& PoolTag, const TCHAR* Label)
	{
		const FLinearColor Steel(0.225f, 0.235f, 0.22f, 1.0f);
		const FLinearColor Slot(0.065f, 0.068f, 0.062f, 1.0f);

		AStaticMeshActor* Body = SpawnLootBox(World, Location + FVector(0.0f, 0.0f, 120.0f), FVector(0.90f, 0.72f, 2.4f), Rotation, Steel, Label, true, 0.80f, 0.28f);
		if (!Body)
		{
			return;
		}
		MarkContainer(*Body, PoolTag, LockerTag);

		for (int32 SlotIndex = 0; SlotIndex < 3; ++SlotIndex)
		{
			SpawnLootBox(World, Location + FVector(0.0f, -76.0f, 170.0f - SlotIndex * 28.0f), FVector(0.45f, 0.045f, 0.035f), Rotation, Slot, TEXT("WB_Loot_LockerVent"), false, 0.76f, 0.35f);
		}
	}

	void SpawnDumpster(UWorld& World, const FVector& Location, const FRotator& Rotation, const FName& PoolTag, const TCHAR* Label)
	{
		const FLinearColor GreenMetal(0.12f, 0.18f, 0.135f, 1.0f);
		const FLinearColor Lid(0.055f, 0.065f, 0.057f, 1.0f);

		AStaticMeshActor* Body = SpawnLootBox(World, Location + FVector(0.0f, 0.0f, 75.0f), FVector(2.4f, 1.35f, 1.35f), Rotation, GreenMetal, Label, true, 0.94f, 0.18f);
		if (!Body)
		{
			return;
		}
		MarkContainer(*Body, PoolTag, DumpsterTag);

		SpawnLootBox(World, Location + FVector(0.0f, 0.0f, 148.0f), FVector(2.55f, 1.47f, 0.10f), FRotator(Rotation.Pitch + 6.0f, Rotation.Yaw, Rotation.Roll), Lid, TEXT("WB_Loot_DumpsterLid"), false, 0.88f, 0.28f);
	}

	void SpawnToolbox(UWorld& World, const FVector& Location, const FRotator& Rotation, const FName& PoolTag, const TCHAR* Label)
	{
		const FLinearColor RedPaint(0.36f, 0.075f, 0.055f, 1.0f);
		const FLinearColor Handle(0.07f, 0.065f, 0.060f, 1.0f);

		AStaticMeshActor* Body = SpawnLootBox(World, Location + FVector(0.0f, 0.0f, 36.0f), FVector(1.45f, 0.70f, 0.60f), Rotation, RedPaint, Label, true, 0.78f, 0.24f);
		if (!Body)
		{
			return;
		}
		MarkContainer(*Body, PoolTag, ToolboxTag);
		SpawnLootBox(World, Location + FVector(0.0f, 0.0f, 78.0f), FVector(0.60f, 0.10f, 0.08f), Rotation, Handle, TEXT("WB_Loot_ToolboxHandle"), false, 0.65f, 0.55f);
	}

	void SpawnCooler(UWorld& World, const FVector& Location, const FRotator& Rotation, const FName& PoolTag, const TCHAR* Label)
	{
		const FLinearColor Cooler(0.46f, 0.48f, 0.43f, 1.0f);
		const FLinearColor Lid(0.23f, 0.275f, 0.29f, 1.0f);

		AStaticMeshActor* Body = SpawnLootBox(World, Location + FVector(0.0f, 0.0f, 40.0f), FVector(1.35f, 0.90f, 0.72f), Rotation, Cooler, Label, true, 0.90f, 0.05f);
		if (!Body)
		{
			return;
		}
		MarkContainer(*Body, PoolTag, CoolerTag);
		SpawnLootBox(World, Location + FVector(0.0f, 0.0f, 82.0f), FVector(1.43f, 0.98f, 0.10f), Rotation, Lid, TEXT("WB_Loot_CoolerLid"), false, 0.82f, 0.04f);
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

	void BuildTownLoot(UWorld& World, const FVector& Origin)
	{
		// Original ruined storefront: a second scavenging beat deeper inside the building.
		SpawnCrate(World, Origin + FVector(2450.0f, 1180.0f, 0.0f), FRotator(0.0f, -7.0f, 0.0f), GeneralPoolTag, TEXT("WB_Loot_Store_BackCrate"));

		// Commercial strip. Markets bias toward food/water, while the pharmacy biases medical.
		SpawnCrate(World, Origin + FVector(6100.0f, 900.0f, 0.0f), FRotator(0.0f, -4.0f, 0.0f), MarketPoolTag, TEXT("WB_Loot_Commercial_Crate"));
		SpawnDumpster(World, Origin + FVector(4250.0f, 3400.0f, 0.0f), FRotator(0.0f, 3.0f, 0.0f), GeneralPoolTag, TEXT("WB_Loot_Commercial_Dumpster"));
		SpawnCooler(World, Origin + FVector(8500.0f, 6000.0f, 0.0f), FRotator(0.0f, 5.0f, 0.0f), MarketPoolTag, TEXT("WB_Loot_Market_Cooler"));
		SpawnCabinet(World, Origin + FVector(3250.0f, 6850.0f, 0.0f), FRotator(0.0f, 0.0f, 0.0f), MedicalPoolTag, TEXT("WB_Loot_Clinic_MedCabinet"));
		SpawnCrate(World, Origin + FVector(5200.0f, 5900.0f, 0.0f), FRotator(0.0f, 8.0f, 0.0f), MedicalPoolTag, TEXT("WB_Loot_Pharmacy_SupplyCrate"));

		// Civic/municipal grounds: office electronics, batteries, wire, and occasional medical stock.
		SpawnLocker(World, Origin + FVector(7000.0f, 6750.0f, 0.0f), FRotator(0.0f, 0.0f, 0.0f), CivicPoolTag, TEXT("WB_Loot_Municipal_Locker"));
		SpawnCrate(World, Origin + FVector(8600.0f, 6750.0f, 0.0f), FRotator(0.0f, -5.0f, 0.0f), CivicPoolTag, TEXT("WB_Loot_Municipal_RecordsCrate"));

		// Residential edge: porches/yards give low-risk household scavenging with occasional useful tools.
		SpawnCooler(World, Origin + FVector(-8200.0f, 2600.0f, 0.0f), FRotator(0.0f, 2.0f, 0.0f), ResidentialPoolTag, TEXT("WB_Loot_House01_Cooler"));
		SpawnCrate(World, Origin + FVector(-8200.0f, 5200.0f, 0.0f), FRotator(0.0f, -3.0f, 0.0f), ResidentialPoolTag, TEXT("WB_Loot_House02_PorchBox"));
		SpawnToolbox(World, Origin + FVector(-8200.0f, 8000.0f, 0.0f), FRotator(0.0f, 4.0f, 0.0f), ResidentialPoolTag, TEXT("WB_Loot_House03_Toolbox"));
		SpawnCabinet(World, Origin + FVector(-5400.0f, 8250.0f, 0.0f), FRotator(0.0f, 0.0f, 0.0f), ResidentialPoolTag, TEXT("WB_Loot_House04_YardCabinet"));
		SpawnCrate(World, Origin + FVector(-3000.0f, 8250.0f, 0.0f), FRotator(0.0f, 6.0f, 0.0f), ResidentialPoolTag, TEXT("WB_Loot_House05_PorchCrate"));

		// Industrial/service district: highest concentration of crafting materials and tools.
		SpawnToolbox(World, Origin + FVector(2500.0f, -6450.0f, 0.0f), FRotator(0.0f, 3.0f, 0.0f), IndustrialPoolTag, TEXT("WB_Loot_Garage_Toolbox"));
		SpawnLocker(World, Origin + FVector(3700.0f, -6400.0f, 0.0f), FRotator(0.0f, -2.0f, 0.0f), IndustrialPoolTag, TEXT("WB_Loot_Garage_Locker"));
		SpawnCrate(World, Origin + FVector(6500.0f, -5900.0f, 0.0f), FRotator(0.0f, 7.0f, 0.0f), IndustrialPoolTag, TEXT("WB_Loot_Warehouse_Crate01"));
		SpawnCrate(World, Origin + FVector(8500.0f, -5900.0f, 0.0f), FRotator(0.0f, -5.0f, 0.0f), IndustrialPoolTag, TEXT("WB_Loot_Warehouse_Crate02"));
		SpawnDumpster(World, Origin + FVector(8750.0f, -4050.0f, 0.0f), FRotator(0.0f, 11.0f, 0.0f), IndustrialPoolTag, TEXT("WB_Loot_LoadingYard_Dumpster"));

		// Route-control landmarks reward players who investigate beyond obvious storefronts.
		SpawnToolbox(World, Origin + FVector(9000.0f, -900.0f, 0.0f), FRotator(0.0f, 14.0f, 0.0f), CivicPoolTag, TEXT("WB_Loot_EvacCheckpoint_FieldBox"));
		SpawnLocker(World, Origin + FVector(-9550.0f, 9600.0f, 0.0f), FRotator(0.0f, 45.0f, 0.0f), IndustrialPoolTag, TEXT("WB_Loot_WaterTower_MaintenanceLocker"));
	}
}

void UWildBoundTownLootSubsystem::OnWorldBeginPlay(UWorld& InWorld)
{
	Super::OnWorldBeginPlay(InWorld);

	if (!InWorld.IsGameWorld())
	{
		return;
	}

	TryBuildTownLoot();
	InWorld.GetTimerManager().SetTimer(
		LootBuildTimer,
		this,
		&UWildBoundTownLootSubsystem::TryBuildTownLoot,
		0.5f,
		true,
		0.15f);
}

void UWildBoundTownLootSubsystem::Deinitialize()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(LootBuildTimer);
	}
	Super::Deinitialize();
}

void UWildBoundTownLootSubsystem::TryBuildTownLoot()
{
	if (bLootBuilt)
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
		if (It->ActorHasTag(TownLootTag))
		{
			bLootBuilt = true;
			World->GetTimerManager().ClearTimer(LootBuildTimer);
			return;
		}
	}

	FVector TownOrigin;
	if (!FindTownOrigin(*World, TownOrigin))
	{
		return;
	}

	BuildTownLoot(*World, TownOrigin);
	bLootBuilt = true;
	World->GetTimerManager().ClearTimer(LootBuildTimer);
	UE_LOG(LogTemp, Log, TEXT("WildBound loot: town-wide searchable containers populated with location-specific loot pools."));
}
