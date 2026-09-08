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

	void SpawnWarningText(UWorld& World, const FVector& Location)
	{
		ATextRenderActor* Actor = World.SpawnActor<ATextRenderActor>(Location, FRotator(0.0f, -90.0f, 0.0f));
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
			Text->SetText(FText::FromString(TEXT("CIVIL DEFENSE\nHOT ZONE - KEEP OUT")));
			Text->SetTextRenderColor(FColor(38, 34, 25));
			Text->SetWorldSize(26.0f);
			Text->SetCastShadow(false);
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

	void SpawnHotspotSetpiece(UWorld& World, const FVector& TownOrigin)
	{
		// The first hotspot sits beside the abandoned truck. It is close enough to discover
		// naturally, but its 12 m radiation field does not reach the player spawn at the intersection.
		const FVector Center = TownOrigin + FVector(360.0f, 1580.0f, 0.0f);

		const FLinearColor AsphaltStain(0.065f, 0.058f, 0.050f, 1.0f);
		const FLinearColor RustedSteel(0.24f, 0.105f, 0.055f, 1.0f);
		const FLinearColor FadedYellow(0.55f, 0.39f, 0.075f, 1.0f);
		const FLinearColor Charcoal(0.045f, 0.047f, 0.044f, 1.0f);
		const FLinearColor ConcreteDust(0.28f, 0.255f, 0.215f, 1.0f);

		// Dark contaminated spill. It is intentionally brown/black instead of glowing green.
		SpawnProp(World, GetCubeMesh(), Center + FVector(0.0f, 0.0f, 9.0f), FVector(3.8f, 2.8f, 0.06f), FRotator(0.0f, -7.0f, 0.0f), AsphaltStain, TEXT("WB_Radiation_Spill"), false, true, 0.99f);
		SpawnProp(World, GetCubeMesh(), Center + FVector(-210.0f, 85.0f, 13.0f), FVector(1.5f, 1.0f, 0.05f), FRotator(0.0f, 18.0f, 0.0f), AsphaltStain * 0.82f, TEXT("WB_Radiation_Stain_02"), false, false, 0.99f);

		// Ruptured drums form the readable source of the danger.
		SpawnProp(World, GetCylinderMesh(), Center + FVector(-70.0f, 35.0f, 75.0f), FVector(0.48f, 0.48f, 0.75f), FRotator::ZeroRotator, RustedSteel, TEXT("WB_Radiation_Drum_01"), true, true, 0.76f, 0.42f);
		SpawnProp(World, GetCylinderMesh(), Center + FVector(70.0f, 85.0f, 75.0f), FVector(0.48f, 0.48f, 0.75f), FRotator(0.0f, 8.0f, 0.0f), RustedSteel * 0.82f, TEXT("WB_Radiation_Drum_02"), true, false, 0.78f, 0.40f);
		SpawnProp(World, GetCylinderMesh(), Center + FVector(155.0f, -55.0f, 55.0f), FVector(0.48f, 0.48f, 0.75f), FRotator(90.0f, -13.0f, 0.0f), RustedSteel * 0.72f, TEXT("WB_Radiation_Drum_Fallen"), true, true, 0.82f, 0.36f);

		// Broken concrete and a tossed containment lid make the scene feel abandoned in a hurry.
		SpawnProp(World, GetCubeMesh(), Center + FVector(-300.0f, -80.0f, 42.0f), FVector(1.4f, 0.8f, 0.32f), FRotator(12.0f, 21.0f, 6.0f), ConcreteDust, TEXT("WB_Radiation_Debris_01"), true, false, 0.98f);
		SpawnProp(World, GetCylinderMesh(), Center + FVector(250.0f, 120.0f, 22.0f), FVector(0.55f, 0.55f, 0.08f), FRotator(16.0f, 30.0f, 5.0f), Charcoal, TEXT("WB_Radiation_DrumLid"), false, false, 0.70f, 0.50f);

		// A battered official warning sign communicates danger before the meter spikes.
		const FVector Sign = Center + FVector(-320.0f, -360.0f, 0.0f);
		SpawnProp(World, GetCubeMesh(), Sign + FVector(0.0f, 0.0f, 130.0f), FVector(0.12f, 0.12f, 2.6f), FRotator::ZeroRotator, Charcoal, TEXT("WB_Radiation_SignPost"), true, false, 0.78f, 0.35f);
		SpawnProp(World, GetCubeMesh(), Sign + FVector(0.0f, 0.0f, 285.0f), FVector(2.0f, 0.10f, 1.15f), FRotator::ZeroRotator, FadedYellow, TEXT("WB_Radiation_SignBoard"), false, false, 0.92f);
		for (int32 Stripe = 0; Stripe < 3; ++Stripe)
		{
			SpawnProp(World, GetCubeMesh(), Sign + FVector(-85.0f + Stripe * 85.0f, -12.0f, 285.0f), FVector(0.13f, 0.11f, 1.0f), FRotator(30.0f, 0.0f, 0.0f), Charcoal, TEXT("WB_Radiation_SignStripe"), false, false, 0.82f, 0.15f);
		}
		SpawnWarningText(World, Sign + FVector(-135.0f, -24.0f, 330.0f));
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

	SpawnHotspotSetpiece(*World, TownOrigin);
	bHotspotSpawned = true;
	World->GetTimerManager().ClearTimer(HotspotSpawnTimer);
	UE_LOG(LogTemp, Log, TEXT("WildBound radiation: localized truck-side hotspot spawned without global color grading."));
}
