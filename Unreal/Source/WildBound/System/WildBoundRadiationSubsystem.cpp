#include "WildBoundRadiationSubsystem.h"

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
	const FName RadiationSetpieceTag(TEXT("WildBoundRadiationSetpiece"));
	const FName RadiationHotspotTag(TEXT("WBRadiationHotspot"));
	const FName RadiationWeakTag(TEXT("WBRadiationWeak"));
	const FName RadiationStrongTag(TEXT("WBRadiationStrong"));
	const FName RadiationExtremeTag(TEXT("WBRadiationExtreme"));

	const FName InteractableTag(TEXT("WBInteractable"));
	const FName ContainerTag(TEXT("WBTypeContainer"));
	const FName QualityRareTag(TEXT("WBLootQualityRare"));
	const FName QualityEpicTag(TEXT("WBLootQualityEpic"));
	const FName MedicalPoolTag(TEXT("WBLootMedical"));
	const FName IndustrialPoolTag(TEXT("WBLootIndustrial"));
	const FName CivicPoolTag(TEXT("WBLootCivic"));
	const FName CrateContainerTag(TEXT("WBContainerCrate"));
	const FName CabinetContainerTag(TEXT("WBContainerCabinet"));

	UStaticMesh* GetCubeMesh()
	{
		static TWeakObjectPtr<UStaticMesh> Mesh;
		if (!Mesh.IsValid())
		{
			Mesh = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube"));
		}
		return Mesh.Get();
	}

	UStaticMesh* GetCylinderMesh()
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
		UStaticMesh* StaticMesh,
		const FVector& Location,
		const FVector& Scale,
		const FRotator& Rotation,
		const FLinearColor& Color,
		const TCHAR* Label,
		bool bCollidable,
		bool bRadiationSource = false,
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

		Actor->Tags.AddUnique(RadiationSetpieceTag);
		if (bRadiationSource)
		{
			Actor->Tags.AddUnique(RadiationHotspotTag);
		}
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

	void SetRadiationIntensity(AActor* Actor, const FName& IntensityTag)
	{
		if (!Actor || IntensityTag.IsNone())
		{
			return;
		}
		Actor->Tags.AddUnique(IntensityTag);
	}

	void SpawnWarningText(
		UWorld& World,
		const FVector& Location,
		const FString& Message,
		const FRotator& Rotation = FRotator(0.0f, -90.0f, 0.0f),
		float WorldSize = 26.0f)
	{
		ATextRenderActor* Actor = World.SpawnActor<ATextRenderActor>(Location, Rotation);
		if (!Actor)
		{
			return;
		}

		Actor->Tags.AddUnique(RadiationSetpieceTag);
#if WITH_EDITOR
		Actor->SetActorLabel(TEXT("WB_Radiation_WarningText"));
#endif

		if (UTextRenderComponent* Text = Actor->GetTextRender())
		{
			Text->SetText(FText::FromString(Message));
			Text->SetTextRenderColor(FColor(38, 34, 25));
			Text->SetWorldSize(WorldSize);
			Text->SetCastShadow(false);
		}
	}

	void SpawnHazardSign(
		UWorld& World,
		const FVector& BaseLocation,
		const FString& Message,
		float YawDegrees)
	{
		const FLinearColor FadedYellow(0.55f, 0.39f, 0.075f, 1.0f);
		const FLinearColor Charcoal(0.045f, 0.047f, 0.044f, 1.0f);
		const FRotator Rotation(0.0f, YawDegrees, 0.0f);

		SpawnProp(World, GetCubeMesh(), BaseLocation + FVector(0.0f, 0.0f, 130.0f), FVector(0.12f, 0.12f, 2.6f), Rotation, Charcoal, TEXT("WB_Radiation_SignPost"), true, false, 0.78f, 0.35f);
		SpawnProp(World, GetCubeMesh(), BaseLocation + FVector(0.0f, 0.0f, 285.0f), FVector(2.0f, 0.10f, 1.15f), Rotation, FadedYellow, TEXT("WB_Radiation_SignBoard"), false, false, 0.92f);

		for (int32 Stripe = 0; Stripe < 3; ++Stripe)
		{
			const FVector LocalOffset(-85.0f + Stripe * 85.0f, -12.0f, 285.0f);
			SpawnProp(
				World,
				GetCubeMesh(),
				BaseLocation + Rotation.RotateVector(LocalOffset),
				FVector(0.13f, 0.11f, 1.0f),
				FRotator(30.0f, YawDegrees, 0.0f),
				Charcoal,
				TEXT("WB_Radiation_SignStripe"),
				false,
				false,
				0.82f,
				0.15f);
		}

		SpawnWarningText(
			World,
			BaseLocation + Rotation.RotateVector(FVector(-135.0f, -24.0f, 330.0f)),
			Message,
			FRotator(0.0f, YawDegrees - 90.0f, 0.0f));
	}

	void SpawnHotZoneContainer(
		UWorld& World,
		const FVector& Location,
		const FRotator& Rotation,
		const FName& PoolTag,
		bool bEpic,
		bool bCabinet,
		const TCHAR* Label)
	{
		const FLinearColor BodyColor = bEpic
			? FLinearColor(0.18f, 0.145f, 0.075f, 1.0f)
			: FLinearColor(0.135f, 0.145f, 0.13f, 1.0f);
		const FLinearColor LidColor = bEpic
			? FLinearColor(0.34f, 0.255f, 0.09f, 1.0f)
			: FLinearColor(0.22f, 0.24f, 0.205f, 1.0f);

		AStaticMeshActor* Body = SpawnProp(
			World,
			GetCubeMesh(),
			Location + FVector(0.0f, 0.0f, 55.0f),
			bCabinet ? FVector(1.0f, 0.65f, 1.35f) : FVector(1.55f, 1.05f, 0.65f),
			Rotation,
			BodyColor,
			Label,
			true,
			false,
			0.78f,
			0.30f);

		if (Body)
		{
			Body->Tags.AddUnique(InteractableTag);
			Body->Tags.AddUnique(ContainerTag);
			Body->Tags.AddUnique(PoolTag);
			Body->Tags.AddUnique(bEpic ? QualityEpicTag : QualityRareTag);
			Body->Tags.AddUnique(bCabinet ? CabinetContainerTag : CrateContainerTag);
		}

		SpawnProp(
			World,
			GetCubeMesh(),
			Location + FVector(0.0f, 0.0f, bCabinet ? 125.0f : 108.0f),
			bCabinet ? FVector(1.04f, 0.68f, 0.10f) : FVector(1.62f, 1.10f, 0.10f),
			Rotation,
			LidColor,
			TEXT("WB_Radiation_HotZoneCache_Lid"),
			false,
			false,
			0.70f,
			0.40f);
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

	void SpawnTruckHotspot(UWorld& World, const FVector& TownOrigin)
	{
		const FVector Center = TownOrigin + FVector(360.0f, 1580.0f, 0.0f);
		const FLinearColor AsphaltStain(0.065f, 0.058f, 0.050f, 1.0f);
		const FLinearColor RustedSteel(0.24f, 0.105f, 0.055f, 1.0f);
		const FLinearColor Charcoal(0.045f, 0.047f, 0.044f, 1.0f);
		const FLinearColor ConcreteDust(0.28f, 0.255f, 0.215f, 1.0f);

		SpawnProp(World, GetCubeMesh(), Center + FVector(0.0f, 0.0f, 9.0f), FVector(3.8f, 2.8f, 0.06f), FRotator(0.0f, -7.0f, 0.0f), AsphaltStain, TEXT("WB_Radiation_Spill"), false, true, 0.99f);
		SpawnProp(World, GetCubeMesh(), Center + FVector(-210.0f, 85.0f, 13.0f), FVector(1.5f, 1.0f, 0.05f), FRotator(0.0f, 18.0f, 0.0f), AsphaltStain * 0.82f, TEXT("WB_Radiation_Stain_02"), false, false, 0.99f);

		SpawnProp(World, GetCylinderMesh(), Center + FVector(-70.0f, 35.0f, 75.0f), FVector(0.48f, 0.48f, 0.75f), FRotator::ZeroRotator, RustedSteel, TEXT("WB_Radiation_Drum_01"), true, true, 0.76f, 0.42f);
		SpawnProp(World, GetCylinderMesh(), Center + FVector(70.0f, 85.0f, 75.0f), FVector(0.48f, 0.48f, 0.75f), FRotator(0.0f, 8.0f, 0.0f), RustedSteel * 0.82f, TEXT("WB_Radiation_Drum_02"), true, false, 0.78f, 0.40f);
		SpawnProp(World, GetCylinderMesh(), Center + FVector(155.0f, -55.0f, 55.0f), FVector(0.48f, 0.48f, 0.75f), FRotator(90.0f, -13.0f, 0.0f), RustedSteel * 0.72f, TEXT("WB_Radiation_Drum_Fallen"), true, true, 0.82f, 0.36f);

		SpawnProp(World, GetCubeMesh(), Center + FVector(-300.0f, -80.0f, 42.0f), FVector(1.4f, 0.8f, 0.32f), FRotator(12.0f, 21.0f, 6.0f), ConcreteDust, TEXT("WB_Radiation_Debris_01"), true, false, 0.98f);
		SpawnProp(World, GetCylinderMesh(), Center + FVector(250.0f, 120.0f, 22.0f), FVector(0.55f, 0.55f, 0.08f), FRotator(16.0f, 30.0f, 5.0f), Charcoal, TEXT("WB_Radiation_DrumLid"), false, false, 0.70f, 0.50f);

		SpawnHazardSign(
			World,
			Center + FVector(-320.0f, -360.0f, 0.0f),
			TEXT("CIVIL DEFENSE\nHOT ZONE - KEEP OUT"),
			0.0f);
	}

	void SpawnClinicHotspot(UWorld& World, const FVector& TownOrigin)
	{
		const FVector Center = TownOrigin + FVector(2600.0f, 7350.0f, 0.0f);
		const FLinearColor Stain(0.070f, 0.064f, 0.050f, 1.0f);
		const FLinearColor MedicalWhite(0.34f, 0.35f, 0.315f, 1.0f);
		const FLinearColor Rust(0.21f, 0.11f, 0.065f, 1.0f);
		const FLinearColor DarkRubber(0.055f, 0.058f, 0.052f, 1.0f);

		AStaticMeshActor* Source = SpawnProp(
			World,
			GetCubeMesh(),
			Center + FVector(160.0f, 40.0f, 8.0f),
			FVector(4.0f, 2.4f, 0.05f),
			FRotator(0.0f, 6.0f, 0.0f),
			Stain,
			TEXT("WB_Radiation_Clinic_DeconSpill"),
			false,
			true,
			0.99f);
		SetRadiationIntensity(Source, RadiationStrongTag);

		// Collapsed decontamination station at the clinic approach.
		SpawnProp(World, GetCubeMesh(), Center + FVector(-290.0f, 120.0f, 115.0f), FVector(1.6f, 0.20f, 2.3f), FRotator(0.0f, 8.0f, 6.0f), MedicalWhite, TEXT("WB_Radiation_Clinic_DeconPanel"), true, false, 0.88f);
		SpawnProp(World, GetCylinderMesh(), Center + FVector(40.0f, -40.0f, 72.0f), FVector(0.42f, 0.42f, 0.72f), FRotator::ZeroRotator, Rust, TEXT("WB_Radiation_Clinic_WasteDrum"), true, false, 0.80f, 0.28f);
		SpawnProp(World, GetCubeMesh(), Center + FVector(360.0f, 160.0f, 40.0f), FVector(1.5f, 0.9f, 0.35f), FRotator(5.0f, -12.0f, 3.0f), DarkRubber, TEXT("WB_Radiation_Clinic_BagPile"), true, false, 0.98f);

		SpawnHotZoneContainer(
			World,
			Center + FVector(-430.0f, -120.0f, 0.0f),
			FRotator(0.0f, 10.0f, 0.0f),
			MedicalPoolTag,
			false,
			true,
			TEXT("WB_Radiation_Clinic_RareMedicalCabinet"));

		SpawnHazardSign(
			World,
			Center + FVector(-690.0f, -430.0f, 0.0f),
			TEXT("CLINIC QUARANTINE\nCONTAMINATED - MASK ADVISED"),
			8.0f);
	}

	void SpawnWarehouseHotspot(UWorld& World, const FVector& TownOrigin)
	{
		const FVector Center = TownOrigin + FVector(8650.0f, -5000.0f, 0.0f);
		const FLinearColor Sludge(0.060f, 0.055f, 0.042f, 1.0f);
		const FLinearColor Rust(0.245f, 0.115f, 0.055f, 1.0f);
		const FLinearColor Steel(0.12f, 0.115f, 0.10f, 1.0f);
		const FLinearColor Concrete(0.265f, 0.25f, 0.22f, 1.0f);

		AStaticMeshActor* Source = SpawnProp(
			World,
			GetCylinderMesh(),
			Center + FVector(0.0f, 0.0f, 68.0f),
			FVector(0.62f, 0.62f, 0.68f),
			FRotator(90.0f, -14.0f, 0.0f),
			Rust,
			TEXT("WB_Radiation_Warehouse_RupturedDrum"),
			true,
			true,
			0.82f,
			0.34f);
		SetRadiationIntensity(Source, RadiationStrongTag);

		SpawnProp(World, GetCubeMesh(), Center + FVector(100.0f, 40.0f, 8.0f), FVector(5.0f, 3.4f, 0.05f), FRotator(0.0f, -10.0f, 0.0f), Sludge, TEXT("WB_Radiation_Warehouse_Sludge"), false, false, 0.99f);
		SpawnProp(World, GetCubeMesh(), Center + FVector(-320.0f, 210.0f, 62.0f), FVector(2.2f, 1.0f, 0.55f), FRotator(8.0f, 16.0f, 5.0f), Steel, TEXT("WB_Radiation_Warehouse_PalletDebris"), true, false, 0.82f, 0.26f);
		SpawnProp(World, GetCubeMesh(), Center + FVector(410.0f, -170.0f, 55.0f), FVector(1.5f, 1.1f, 0.50f), FRotator(-5.0f, -12.0f, 4.0f), Concrete, TEXT("WB_Radiation_Warehouse_BrokenBarrier"), true, false, 0.96f);

		SpawnHotZoneContainer(
			World,
			Center + FVector(520.0f, 220.0f, 0.0f),
			FRotator(0.0f, -8.0f, 0.0f),
			IndustrialPoolTag,
			false,
			false,
			TEXT("WB_Radiation_Warehouse_RareIndustrialCache"));

		SpawnHazardSign(
			World,
			Center + FVector(-620.0f, 520.0f, 0.0f),
			TEXT("LOADING YARD CLOSED\nRADIOLOGICAL MATERIAL"),
			-12.0f);
	}

	void SpawnDrainageHotspot(UWorld& World, const FVector& TownOrigin)
	{
		const FLinearColor Runoff(0.052f, 0.056f, 0.044f, 1.0f);
		const FLinearColor Silt(0.095f, 0.083f, 0.060f, 1.0f);
		const FVector Sources[] =
		{
			TownOrigin + FVector(-3600.0f, 17100.0f, 0.0f),
			TownOrigin + FVector(-2200.0f, 17100.0f, 0.0f),
			TownOrigin + FVector(-800.0f, 17100.0f, 0.0f)
		};

		for (int32 Index = 0; Index < 3; ++Index)
		{
			AStaticMeshActor* Source = SpawnProp(
				World,
				GetCubeMesh(),
				Sources[Index] + FVector(0.0f, 0.0f, 9.0f),
				FVector(7.0f, 5.0f, 0.035f),
				FRotator(0.0f, Index % 2 == 0 ? -5.0f : 7.0f, 0.0f),
				Runoff * (0.92f + Index * 0.035f),
				TEXT("WB_Radiation_Drainage_Runoff"),
				false,
				true,
				0.99f);
			SetRadiationIntensity(Source, RadiationWeakTag);

			SpawnProp(
				World,
				GetCubeMesh(),
				Sources[Index] + FVector(180.0f, -120.0f, 18.0f),
				FVector(2.0f, 1.4f, 0.06f),
				FRotator(0.0f, 12.0f, 0.0f),
				Silt,
				TEXT("WB_Radiation_Drainage_Silt"),
				false,
				false,
				0.99f);
		}

		SpawnHotZoneContainer(
			World,
			TownOrigin + FVector(-2050.0f, 16820.0f, 0.0f),
			FRotator(0.0f, 4.0f, 0.0f),
			CivicPoolTag,
			false,
			false,
			TEXT("WB_Radiation_Drainage_RareSurveyCache"));

		SpawnHazardSign(
			World,
			TownOrigin + FVector(-3100.0f, 15720.0f, 0.0f),
			TEXT("RUNOFF CONTAMINATED\nDO NOT ENTER CHANNEL"),
			0.0f);
	}

	void SpawnTreatmentPlantHotspot(UWorld& World, const FVector& TownOrigin)
	{
		const FVector Center = TownOrigin + FVector(-7600.0f, -16600.0f, 0.0f);
		const FLinearColor Basin(0.050f, 0.052f, 0.038f, 1.0f);
		const FLinearColor Oxide(0.22f, 0.105f, 0.052f, 1.0f);
		const FLinearColor Concrete(0.24f, 0.235f, 0.205f, 1.0f);
		const FLinearColor DarkMetal(0.095f, 0.09f, 0.075f, 1.0f);

		AStaticMeshActor* Source = SpawnProp(
			World,
			GetCubeMesh(),
			Center + FVector(0.0f, 0.0f, 10.0f),
			FVector(8.0f, 6.5f, 0.05f),
			FRotator(0.0f, 4.0f, 0.0f),
			Basin,
			TEXT("WB_Radiation_TreatmentPlant_ContaminatedBasin"),
			false,
			true,
			0.99f);
		SetRadiationIntensity(Source, RadiationExtremeTag);

		AStaticMeshActor* SecondarySource = SpawnProp(
			World,
			GetCylinderMesh(),
			Center + FVector(640.0f, -260.0f, 85.0f),
			FVector(0.58f, 0.58f, 0.85f),
			FRotator(82.0f, -18.0f, 4.0f),
			Oxide,
			TEXT("WB_Radiation_TreatmentPlant_RupturedFilter"),
			true,
			true,
			0.78f,
			0.38f);
		SetRadiationIntensity(SecondarySource, RadiationExtremeTag);

		SpawnProp(World, GetCubeMesh(), Center + FVector(-480.0f, 300.0f, 60.0f), FVector(2.4f, 0.35f, 1.2f), FRotator(4.0f, -8.0f, 3.0f), Concrete, TEXT("WB_Radiation_TreatmentPlant_CollapsedWall"), true, false, 0.98f);
		SpawnProp(World, GetCubeMesh(), Center + FVector(410.0f, 360.0f, 58.0f), FVector(1.8f, 1.1f, 0.50f), FRotator(-6.0f, 14.0f, 5.0f), DarkMetal, TEXT("WB_Radiation_TreatmentPlant_FilterDebris"), true, false, 0.82f, 0.26f);

		SpawnHotZoneContainer(
			World,
			Center + FVector(-520.0f, -360.0f, 0.0f),
			FRotator(0.0f, -10.0f, 0.0f),
			MedicalPoolTag,
			true,
			true,
			TEXT("WB_Radiation_TreatmentPlant_EpicEmergencyCabinet"));

		SpawnHazardSign(
			World,
			Center + FVector(0.0f, 1050.0f, 0.0f),
			TEXT("WATER AUTHORITY\nEXTREME RADIOLOGICAL HAZARD"),
			0.0f);
		SpawnHazardSign(
			World,
			Center + FVector(1100.0f, 0.0f, 0.0f),
			TEXT("RESPIRATORY PROTECTION\nMANDATORY BEYOND THIS POINT"),
			90.0f);
	}

	void SpawnRadiationZones(UWorld& World, const FVector& TownOrigin)
	{
		SpawnTruckHotspot(World, TownOrigin);
		SpawnClinicHotspot(World, TownOrigin);
		SpawnWarehouseHotspot(World, TownOrigin);
		SpawnDrainageHotspot(World, TownOrigin);
		SpawnTreatmentPlantHotspot(World, TownOrigin);
	}
}

void UWildBoundRadiationSubsystem::OnWorldBeginPlay(UWorld& InWorld)
{
	Super::OnWorldBeginPlay(InWorld);

	if (!InWorld.IsGameWorld())
	{
		return;
	}

	TrySpawnHotspot();
	InWorld.GetTimerManager().SetTimer(
		HotspotSpawnTimer,
		this,
		&UWildBoundRadiationSubsystem::TrySpawnHotspot,
		0.5f,
		true,
		0.10f);
}

void UWildBoundRadiationSubsystem::Deinitialize()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(HotspotSpawnTimer);
	}
	Super::Deinitialize();
}

void UWildBoundRadiationSubsystem::TrySpawnHotspot()
{
	if (bHotspotSpawned)
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
		if (It->ActorHasTag(RadiationSetpieceTag))
		{
			bHotspotSpawned = true;
			World->GetTimerManager().ClearTimer(HotspotSpawnTimer);
			return;
		}
	}

	FVector TownOrigin;
	if (!FindTownOrigin(*World, TownOrigin))
	{
		return;
	}

	SpawnRadiationZones(*World, TownOrigin);
	bHotspotSpawned = true;
	World->GetTimerManager().ClearTimer(HotspotSpawnTimer);
	UE_LOG(LogTemp, Log, TEXT("WildBound radiation: five localized risk/reward contamination zones spawned."));
}
