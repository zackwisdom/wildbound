#include "WildBoundLootRaritySubsystem.h"

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
	const FName LootRarityTownTag(TEXT("WildBoundTownBlockout"));
	const FName LootRarityTownLootTag(TEXT("WildBoundTownLoot"));
	const FName LootRarityVisualTag(TEXT("WildBoundLootRarityVisual"));
	const FName LootRarityCacheSetTag(TEXT("WildBoundRareLootCache"));
	const FName LootRarityInteractableTag(TEXT("WBInteractable"));
	const FName LootRarityContainerTag(TEXT("WBTypeContainer"));
	const FName LootRarityPryLockedTag(TEXT("WBPryLocked"));
	const FName LootRarityPryContainerTag(TEXT("WBPryContainer"));

	const FName LootQualityCommonTag(TEXT("WBLootQualityCommon"));
	const FName LootQualityUncommonTag(TEXT("WBLootQualityUncommon"));
	const FName LootQualityRareTag(TEXT("WBLootQualityRare"));
	const FName LootQualityEpicTag(TEXT("WBLootQualityEpic"));

	const FName LootRarityMedicalPoolTag(TEXT("WBLootMedical"));
	const FName LootRarityMarketPoolTag(TEXT("WBLootMarket"));
	const FName LootRarityResidentialPoolTag(TEXT("WBLootResidential"));
	const FName LootRarityIndustrialPoolTag(TEXT("WBLootIndustrial"));
	const FName LootRarityCivicPoolTag(TEXT("WBLootCivic"));

	const FName LootRarityCrateTag(TEXT("WBContainerCrate"));
	const FName LootRarityCabinetTag(TEXT("WBContainerCabinet"));
	const FName LootRarityLockerTag(TEXT("WBContainerLocker"));
	const FName LootRarityDumpsterTag(TEXT("WBContainerDumpster"));
	const FName LootRarityToolboxTag(TEXT("WBContainerToolbox"));
	const FName LootRarityCoolerTag(TEXT("WBContainerCooler"));

	UStaticMesh* GetLootRarityCube()
	{
		static TWeakObjectPtr<UStaticMesh> Mesh;
		if (!Mesh.IsValid())
		{
			Mesh = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube"));
		}
		return Mesh.Get();
	}

	UMaterialInterface* GetLootRarityBaseMaterial()
	{
		static TWeakObjectPtr<UMaterialInterface> Material;
		if (!Material.IsValid())
		{
			Material = LoadObject<UMaterialInterface>(nullptr, TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));
		}
		return Material.Get();
	}

	void ApplyLootRarityMaterial(UStaticMeshComponent& Mesh, const FLinearColor& Color, float Roughness, float Metallic)
	{
		UMaterialInterface* Base = GetLootRarityBaseMaterial();
		if (!Base)
		{
			return;
		}

		UMaterialInstanceDynamic* Dynamic = UMaterialInstanceDynamic::Create(Base, &Mesh);
		if (!Dynamic)
		{
			return;
		}

		Dynamic->SetVectorParameterValue(TEXT("Color"), Color);
		Dynamic->SetVectorParameterValue(TEXT("BaseColor"), Color);
		Dynamic->SetScalarParameterValue(TEXT("Roughness"), Roughness);
		Dynamic->SetScalarParameterValue(TEXT("Metallic"), Metallic);
		Mesh.SetMaterial(0, Dynamic);
	}

	AStaticMeshActor* SpawnLootRarityBox(
		UWorld& World,
		const FVector& Location,
		const FVector& Scale,
		const FRotator& Rotation,
		const FLinearColor& Color,
		const TCHAR* Label,
		bool bCollidable,
		float Roughness = 0.82f,
		float Metallic = 0.22f)
	{
		UStaticMesh* Cube = GetLootRarityCube();
		if (!Cube)
		{
			return nullptr;
		}

		AStaticMeshActor* Actor = World.SpawnActor<AStaticMeshActor>(Location, Rotation);
		if (!Actor)
		{
			return nullptr;
		}

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
		ApplyLootRarityMaterial(*Mesh, Color, Roughness, Metallic);
		Actor->SetActorScale3D(Scale);
		return Actor;
	}

	bool FindLootRarityTownOrigin(UWorld& World, FVector& OutOrigin)
	{
		for (TActorIterator<AStaticMeshActor> It(&World); It; ++It)
		{
			AStaticMeshActor* Actor = *It;
			if (!Actor || !Actor->ActorHasTag(LootRarityTownTag))
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

	void ClearLootQualityTags(AActor& Actor)
	{
		Actor.Tags.Remove(LootQualityCommonTag);
		Actor.Tags.Remove(LootQualityUncommonTag);
		Actor.Tags.Remove(LootQualityRareTag);
		Actor.Tags.Remove(LootQualityEpicTag);
	}

	void SetLootQualityTier(AActor& Actor, int32 Tier)
	{
		ClearLootQualityTags(Actor);
		switch (Tier)
		{
		case 3: Actor.Tags.AddUnique(LootQualityEpicTag); break;
		case 2: Actor.Tags.AddUnique(LootQualityRareTag); break;
		case 1: Actor.Tags.AddUnique(LootQualityUncommonTag); break;
		default: Actor.Tags.AddUnique(LootQualityCommonTag); break;
		}
	}

	int32 DetermineContainerQuality(const AActor& Actor)
	{
		const FVector Location = Actor.GetActorLocation();
		const uint32 LocationHash = HashCombine(
			GetTypeHash(FMath::RoundToInt(Location.X)),
			HashCombine(GetTypeHash(FMath::RoundToInt(Location.Y)), GetTypeHash(FMath::RoundToInt(Location.Z))));
		const int32 Roll = static_cast<int32>(LocationHash % 100u);

		int32 RareThreshold = 5;
		int32 UncommonThreshold = 34;

		if (Actor.ActorHasTag(LootRarityDumpsterTag))
		{
			RareThreshold = 1;
			UncommonThreshold = 12;
		}
		else if (Actor.ActorHasTag(LootRarityToolboxTag))
		{
			RareThreshold = 18;
			UncommonThreshold = 75;
		}
		else if (Actor.ActorHasTag(LootRarityLockerTag))
		{
			RareThreshold = 15;
			UncommonThreshold = 68;
		}
		else if (Actor.ActorHasTag(LootRarityCabinetTag))
		{
			RareThreshold = 13;
			UncommonThreshold = 70;
		}
		else if (Actor.ActorHasTag(LootRarityCoolerTag))
		{
			RareThreshold = 8;
			UncommonThreshold = 50;
		}
		else if (Actor.ActorHasTag(LootRarityCrateTag))
		{
			RareThreshold = 6;
			UncommonThreshold = 35;
		}

		if (Actor.ActorHasTag(LootRarityMedicalPoolTag) || Actor.ActorHasTag(LootRarityIndustrialPoolTag))
		{
			RareThreshold += 4;
			UncommonThreshold += 6;
		}
		else if (Actor.ActorHasTag(LootRarityCivicPoolTag))
		{
			RareThreshold += 2;
			UncommonThreshold += 4;
		}

		RareThreshold = FMath::Clamp(RareThreshold, 0, 28);
		UncommonThreshold = FMath::Clamp(FMath::Max(UncommonThreshold, RareThreshold + 1), 1, 90);

		if (Roll < RareThreshold)
		{
			return 2;
		}
		if (Roll < UncommonThreshold)
		{
			return 1;
		}
		return 0;
	}

	void SpawnQualityMarker(UWorld& World, const AStaticMeshActor& Container, int32 Tier)
	{
		if (Tier <= 0)
		{
			return;
		}

		const FVector Scale = Container.GetActorScale3D();
		const FRotator Rotation = Container.GetActorRotation();
		const FVector LocalOffset(0.0f, -(Scale.Y * 50.0f + 5.0f), Scale.Z * 18.0f);
		const FVector MarkerLocation = Container.GetActorLocation() + Rotation.RotateVector(LocalOffset);
		const FVector MarkerScale(FMath::Clamp(Scale.X * 0.48f, 0.28f, 1.35f), 0.045f, 0.065f);

		FLinearColor MarkerColor(0.30f, 0.36f, 0.24f, 1.0f); // worn olive for uncommon
		if (Tier == 2)
		{
			MarkerColor = FLinearColor(0.18f, 0.30f, 0.42f, 1.0f); // faded blue steel for rare
		}
		else if (Tier >= 3)
		{
			MarkerColor = FLinearColor(0.50f, 0.31f, 0.09f, 1.0f); // aged brass for epic
		}

		if (AStaticMeshActor* Marker = SpawnLootRarityBox(
			World,
			MarkerLocation,
			MarkerScale,
			Rotation,
			MarkerColor,
			TEXT("WB_LootQualityMarker"),
			false,
			0.62f,
			0.42f))
		{
			Marker->Tags.AddUnique(LootRarityVisualTag);
		}
	}

	void SpawnLockedLootCache(
		UWorld& World,
		const FVector& BaseLocation,
		const FName& PoolTag,
		const FRotator& Rotation,
		const TCHAR* LabelStem)
	{
		const FLinearColor CaseMetal(0.15f, 0.16f, 0.145f, 1.0f);
		const FLinearColor DarkMetal(0.055f, 0.060f, 0.055f, 1.0f);
		const FLinearColor Brass(0.50f, 0.31f, 0.09f, 1.0f);

		AStaticMeshActor* Body = SpawnLootRarityBox(
			World,
			BaseLocation + FVector(0.0f, 0.0f, 58.0f),
			FVector(1.85f, 1.20f, 0.82f),
			Rotation,
			CaseMetal,
			LabelStem,
			true,
			0.76f,
			0.46f);
		if (!Body)
		{
			return;
		}

		Body->Tags.AddUnique(LootRarityTownLootTag);
		Body->Tags.AddUnique(LootRarityCacheSetTag);
		Body->Tags.AddUnique(LootRarityInteractableTag);
		Body->Tags.AddUnique(LootRarityContainerTag);
		Body->Tags.AddUnique(LootRarityPryLockedTag);
		Body->Tags.AddUnique(LootRarityPryContainerTag);
		Body->Tags.AddUnique(LootRarityCrateTag);
		Body->Tags.AddUnique(PoolTag);
		Body->Tags.AddUnique(LootQualityEpicTag);

		AStaticMeshActor* Lid = SpawnLootRarityBox(
			World,
			BaseLocation + FVector(0.0f, 0.0f, 105.0f),
			FVector(1.95f, 1.30f, 0.13f),
			Rotation,
			DarkMetal,
			TEXT("WB_RareCache_Lid"),
			false,
			0.70f,
			0.54f);
		if (Lid)
		{
			Lid->Tags.AddUnique(LootRarityVisualTag);
			Lid->Tags.AddUnique(LootRarityCacheSetTag);
		}

		const FVector Forward = Rotation.RotateVector(FVector(0.0f, -1.0f, 0.0f));
		AStaticMeshActor* Latch = SpawnLootRarityBox(
			World,
			BaseLocation + Forward * 126.0f + FVector(0.0f, 0.0f, 58.0f),
			FVector(0.32f, 0.10f, 0.34f),
			Rotation,
			Brass,
			TEXT("WB_RareCache_Latch"),
			false,
			0.56f,
			0.68f);
		if (Latch)
		{
			Latch->Tags.AddUnique(LootRarityVisualTag);
			Latch->Tags.AddUnique(LootRarityCacheSetTag);
		}

		AStaticMeshActor* Stripe = SpawnLootRarityBox(
			World,
			BaseLocation + Forward * 127.0f + FVector(0.0f, 0.0f, 80.0f),
			FVector(1.05f, 0.055f, 0.055f),
			Rotation,
			Brass,
			TEXT("WB_RareCache_QualityStripe"),
			false,
			0.62f,
			0.44f);
		if (Stripe)
		{
			Stripe->Tags.AddUnique(LootRarityVisualTag);
			Stripe->Tags.AddUnique(LootRarityCacheSetTag);
		}
	}
}

void UWildBoundLootRaritySubsystem::OnWorldBeginPlay(UWorld& InWorld)
{
	Super::OnWorldBeginPlay(InWorld);

	if (!InWorld.IsGameWorld())
	{
		return;
	}

	TrySetupLootRarity();
	InWorld.GetTimerManager().SetTimer(
		LootRaritySetupTimer,
		this,
		&UWildBoundLootRaritySubsystem::TrySetupLootRarity,
		0.5f,
		true,
		0.20f);
}

void UWildBoundLootRaritySubsystem::Deinitialize()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(LootRaritySetupTimer);
	}
	Super::Deinitialize();
}

void UWildBoundLootRaritySubsystem::TrySetupLootRarity()
{
	if (bLootRaritySetupComplete)
	{
		return;
	}

	UWorld* World = GetWorld();
	if (!World || !World->IsGameWorld())
	{
		return;
	}

	FVector TownOrigin;
	if (!FindLootRarityTownOrigin(*World, TownOrigin))
	{
		return;
	}

	TArray<AStaticMeshActor*> Containers;
	bool bCachesAlreadyExist = false;
	for (TActorIterator<AStaticMeshActor> It(World); It; ++It)
	{
		AStaticMeshActor* Actor = *It;
		if (!Actor)
		{
			continue;
		}

		if (Actor->ActorHasTag(LootRarityCacheSetTag))
		{
			bCachesAlreadyExist = true;
		}

		if (Actor->ActorHasTag(LootRarityTownLootTag) && Actor->ActorHasTag(LootRarityContainerTag))
		{
			Containers.Add(Actor);
		}
	}

	// Wait until the town loot subsystem has created its searchable containers.
	if (Containers.Num() < 8)
	{
		return;
	}

	for (AStaticMeshActor* Container : Containers)
	{
		if (!Container || Container->ActorHasTag(LootRarityCacheSetTag))
		{
			continue;
		}

		const int32 Tier = DetermineContainerQuality(*Container);
		SetLootQualityTier(*Container, Tier);
		SpawnQualityMarker(*World, *Container, Tier);
	}

	if (!bCachesAlreadyExist)
	{
		// Three deliberately hidden high-value caches reward deeper town exploration.
		SpawnLockedLootCache(
			*World,
			TownOrigin + FVector(4550.0f, 7350.0f, 0.0f),
			LootRarityMedicalPoolTag,
			FRotator(0.0f, -7.0f, 0.0f),
			TEXT("WB_RareCache_ClinicEmergencyStock"));

		SpawnLockedLootCache(
			*World,
			TownOrigin + FVector(7900.0f, -6900.0f, 0.0f),
			LootRarityIndustrialPoolTag,
			FRotator(0.0f, 6.0f, 0.0f),
			TEXT("WB_RareCache_WarehouseForeman"));

		SpawnLockedLootCache(
			*World,
			TownOrigin + FVector(-9300.0f, 9000.0f, 0.0f),
			LootRarityCivicPoolTag,
			FRotator(0.0f, 35.0f, 0.0f),
			TEXT("WB_RareCache_WaterTowerCivilDefense"));
	}

	bLootRaritySetupComplete = true;
	World->GetTimerManager().ClearTimer(LootRaritySetupTimer);
	UE_LOG(LogTemp, Log, TEXT("WildBound loot rarity: container quality tiers and three locked high-value caches initialized."));
}
