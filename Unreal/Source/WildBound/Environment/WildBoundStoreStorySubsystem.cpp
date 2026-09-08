#include "WildBoundStoreStorySubsystem.h"

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
	const FName StoryTag(TEXT("WildBoundStoreStory"));
	const FName TownTag(TEXT("WildBoundTownBlockout"));

	UStaticMesh* GetStoryCube()
	{
		static TWeakObjectPtr<UStaticMesh> CubeMesh;
		if (!CubeMesh.IsValid())
		{
			CubeMesh = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube"));
		}
		return CubeMesh.Get();
	}

	UMaterialInterface* GetStoryMaterialBase()
	{
		static TWeakObjectPtr<UMaterialInterface> BaseMaterial;
		if (!BaseMaterial.IsValid())
		{
			BaseMaterial = LoadObject<UMaterialInterface>(nullptr, TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));
		}
		return BaseMaterial.Get();
	}

	void ApplyStoryMaterial(UStaticMeshComponent& Mesh, const FLinearColor& Color, float Roughness, float Metallic)
	{
		UMaterialInterface* BaseMaterial = GetStoryMaterialBase();
		if (!BaseMaterial)
		{
			return;
		}

		UMaterialInstanceDynamic* Material = UMaterialInstanceDynamic::Create(BaseMaterial, &Mesh);
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

	AStaticMeshActor* SpawnStoryBox(
		UWorld& World,
		const FVector& Location,
		const FVector& Scale,
		const FRotator& Rotation,
		const FLinearColor& Color,
		const TCHAR* Label,
		float Roughness = 0.9f,
		float Metallic = 0.0f,
		bool bCollidable = false)
	{
		UStaticMesh* CubeMesh = GetStoryCube();
		if (!CubeMesh)
		{
			return nullptr;
		}

		AStaticMeshActor* Actor = World.SpawnActor<AStaticMeshActor>(Location, Rotation);
		if (!Actor)
		{
			return nullptr;
		}

		Actor->Tags.AddUnique(StoryTag);
#if WITH_EDITOR
		Actor->SetActorLabel(Label);
#endif

		UStaticMeshComponent* Mesh = Actor->GetStaticMeshComponent();
		Mesh->SetMobility(EComponentMobility::Movable);
		Mesh->SetStaticMesh(CubeMesh);
		Mesh->SetCastShadow(true);
		Mesh->SetCollisionEnabled(bCollidable ? ECollisionEnabled::QueryAndPhysics : ECollisionEnabled::NoCollision);
		if (bCollidable)
		{
			Mesh->SetCollisionResponseToAllChannels(ECR_Block);
		}
		ApplyStoryMaterial(*Mesh, Color, Roughness, Metallic);
		Actor->SetActorScale3D(Scale);
		return Actor;
	}

	void SpawnStoryText(
		UWorld& World,
		const FVector& Location,
		const FRotator& Rotation,
		const FString& Text,
		const FColor& Color,
		float WorldSize,
		const TCHAR* Label)
	{
		ATextRenderActor* Actor = World.SpawnActor<ATextRenderActor>(Location, Rotation);
		if (!Actor)
		{
			return;
		}

		Actor->Tags.AddUnique(StoryTag);
#if WITH_EDITOR
		Actor->SetActorLabel(Label);
#endif

		if (UTextRenderComponent* TextRender = Actor->GetTextRender())
		{
			TextRender->SetText(FText::FromString(Text));
			TextRender->SetTextRenderColor(Color);
			TextRender->SetWorldSize(WorldSize);
			TextRender->SetCastShadow(false);
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

			// The town foundation is uniquely scaled 80 x 80 x 0.5 in the current blockout.
			// Finding it means this pass does not depend on player position or subsystem order.
			if (Actor->GetActorScale3D().Equals(FVector(80.0f, 80.0f, 0.50f), 0.05f))
			{
				OutOrigin = Actor->GetActorLocation() + FVector(0.0f, 0.0f, 25.0f);
				return true;
			}
		}

		return false;
	}

	void SpawnStoreStoryPass(UWorld& World, const FVector& TownOrigin)
	{
		const FVector Store = TownOrigin + FVector(1950.0f, 1800.0f, 0.0f);

		const FLinearColor CrateBrown(0.16f, 0.11f, 0.065f, 1.0f);
		const FLinearColor WaterBlue(0.18f, 0.29f, 0.33f, 1.0f);
		const FLinearColor MedicalRed(0.42f, 0.075f, 0.055f, 1.0f);
		const FLinearColor MedicalWhite(0.68f, 0.63f, 0.53f, 1.0f);
		const FLinearColor FoodOlive(0.25f, 0.255f, 0.13f, 1.0f);
		const FLinearColor WarningYellow(0.62f, 0.43f, 0.07f, 1.0f);
		const FLinearColor Charcoal(0.055f, 0.060f, 0.058f, 1.0f);
		const FLinearColor SurveySteel(0.18f, 0.19f, 0.18f, 1.0f);
		const FLinearColor BoardCream(0.46f, 0.43f, 0.35f, 1.0f);
		const FLinearColor MarkerRed(0.44f, 0.075f, 0.055f, 1.0f);

		// Scavenging cluster behind the checkout counter: supplies survived because
		// they were shoved into the employee side when the storefront was abandoned.
		SpawnStoryBox(World, Store + FVector(-610.0f, 10.0f, 58.0f), FVector(2.2f, 1.6f, 0.58f), FRotator(0.0f, 5.0f, 0.0f), CrateBrown, TEXT("WB_Story_SupplyCrate_01"), 0.92f, 0.0f, true);
		SpawnStoryBox(World, Store + FVector(-575.0f, 5.0f, 150.0f), FVector(1.8f, 1.45f, 0.45f), FRotator(0.0f, -4.0f, 0.0f), WaterBlue, TEXT("WB_Story_WaterCase"));
		for (int32 BottleIndex = 0; BottleIndex < 4; ++BottleIndex)
		{
			SpawnStoryBox(World, Store + FVector(-690.0f + BottleIndex * 75.0f, -5.0f, 205.0f), FVector(0.20f, 0.20f, 0.42f), FRotator::ZeroRotator, WaterBlue * 1.25f, TEXT("WB_Story_WaterBottle"), 0.55f);
		}

		// A battered first-aid kit on the counter is deliberately easy to spot from the entrance.
		SpawnStoryBox(World, Store + FVector(40.0f, -115.0f, 170.0f), FVector(1.25f, 0.75f, 0.35f), FRotator(0.0f, -7.0f, 0.0f), MedicalRed, TEXT("WB_Story_MedKit"), 0.78f);
		SpawnStoryBox(World, Store + FVector(40.0f, -190.0f, 171.0f), FVector(0.18f, 0.08f, 0.24f), FRotator::ZeroRotator, MedicalWhite, TEXT("WB_Story_MedCrossVertical"), 0.85f);
		SpawnStoryBox(World, Store + FVector(40.0f, -191.0f, 171.0f), FVector(0.42f, 0.08f, 0.10f), FRotator::ZeroRotator, MedicalWhite, TEXT("WB_Story_MedCrossHorizontal"), 0.85f);

		// Remaining food cartons are pushed against the rear shelving rather than filling the aisle.
		SpawnStoryBox(World, Store + FVector(-650.0f, 650.0f, 62.0f), FVector(1.5f, 1.15f, 0.55f), FRotator(0.0f, 8.0f, 0.0f), FoodOlive, TEXT("WB_Story_FoodCarton_01"), 0.94f);
		SpawnStoryBox(World, Store + FVector(-510.0f, 635.0f, 128.0f), FVector(1.2f, 1.0f, 0.42f), FRotator(0.0f, -6.0f, 0.0f), FoodOlive * 0.82f, TEXT("WB_Story_FoodCarton_02"), 0.94f);

		// Warning placard beside the stock-room opening. The diagonal dark bars make it
		// read as official emergency signage even before final authored textures arrive.
		SpawnStoryBox(World, Store + FVector(470.0f, 445.0f, 560.0f), FVector(1.65f, 0.08f, 1.20f), FRotator::ZeroRotator, WarningYellow, TEXT("WB_Story_EmergencyPlacard"), 0.90f);
		for (int32 StripeIndex = 0; StripeIndex < 3; ++StripeIndex)
		{
			SpawnStoryBox(World, Store + FVector(390.0f + StripeIndex * 80.0f, 436.0f, 560.0f), FVector(0.13f, 0.09f, 1.05f), FRotator(34.0f, 0.0f, 0.0f), Charcoal, TEXT("WB_Story_WarningStripe"), 0.78f, 0.15f);
		}
		SpawnStoryText(World, Store + FVector(325.0f, 420.0f, 690.0f), FRotator(0.0f, -90.0f, 0.0f), TEXT("EMERGENCY STOCK\nAUTHORIZED ONLY"), FColor(38, 34, 26), 25.0f, TEXT("WB_Story_EmergencyText"));

		// Back-room survey station: a surviving workbench gives the room a clear focal point
		// and hints that somebody was measuring or cataloging contamination here.
		SpawnStoryBox(World, Store + FVector(360.0f, 710.0f, 145.0f), FVector(4.8f, 1.7f, 0.18f), FRotator::ZeroRotator, SurveySteel, TEXT("WB_Story_SurveyBenchTop"), 0.72f, 0.35f, true);
		SpawnStoryBox(World, Store + FVector(20.0f, 710.0f, 75.0f), FVector(0.22f, 1.3f, 1.35f), FRotator::ZeroRotator, Charcoal, TEXT("WB_Story_SurveyBenchLegL"), 0.72f, 0.45f, true);
		SpawnStoryBox(World, Store + FVector(700.0f, 710.0f, 75.0f), FVector(0.22f, 1.3f, 1.35f), FRotator::ZeroRotator, Charcoal, TEXT("WB_Story_SurveyBenchLegR"), 0.72f, 0.45f, true);

		// Government-style field case, meter, probe, and sample canisters.
		SpawnStoryBox(World, Store + FVector(470.0f, 705.0f, 205.0f), FVector(1.75f, 1.0f, 0.40f), FRotator(0.0f, -3.0f, 0.0f), Charcoal, TEXT("WB_Story_SurveyCase"), 0.66f, 0.48f);
		SpawnStoryBox(World, Store + FVector(470.0f, 605.0f, 207.0f), FVector(1.45f, 0.08f, 0.08f), FRotator::ZeroRotator, WarningYellow, TEXT("WB_Story_SurveyCaseStripe"), 0.82f);
		SpawnStoryBox(World, Store + FVector(170.0f, 705.0f, 215.0f), FVector(0.65f, 0.50f, 0.42f), FRotator(0.0f, 5.0f, 0.0f), SurveySteel, TEXT("WB_Story_SurveyMeterBody"), 0.68f, 0.35f);
		SpawnStoryBox(World, Store + FVector(170.0f, 690.0f, 275.0f), FVector(0.08f, 0.08f, 0.78f), FRotator(-12.0f, 0.0f, 0.0f), Charcoal, TEXT("WB_Story_SurveyProbe"), 0.58f, 0.55f);
		for (int32 CanisterIndex = 0; CanisterIndex < 3; ++CanisterIndex)
		{
			SpawnStoryBox(World, Store + FVector(260.0f + CanisterIndex * 68.0f, 730.0f, 212.0f), FVector(0.18f, 0.18f, 0.38f), FRotator::ZeroRotator, SurveySteel * (0.9f + CanisterIndex * 0.08f), TEXT("WB_Story_SampleCanister"), 0.48f, 0.62f);
		}

		// The first mystery clue: a faded civil-defense survey board with an anomalous
		// sector marked in red. It implies organized sampling before the town was lost,
		// without explaining who ordered it or what they found.
		SpawnStoryBox(World, Store + FVector(360.0f, 850.0f, 570.0f), FVector(3.8f, 0.08f, 2.25f), FRotator::ZeroRotator, BoardCream, TEXT("WB_Story_SurveyBoard"), 0.96f);
		SpawnStoryBox(World, Store + FVector(170.0f, 840.0f, 620.0f), FVector(0.08f, 0.09f, 1.15f), FRotator(26.0f, 0.0f, 0.0f), Charcoal, TEXT("WB_Story_MapLine_01"), 0.92f);
		SpawnStoryBox(World, Store + FVector(380.0f, 839.0f, 515.0f), FVector(1.2f, 0.09f, 0.07f), FRotator::ZeroRotator, Charcoal, TEXT("WB_Story_MapLine_02"), 0.92f);
		SpawnStoryBox(World, Store + FVector(540.0f, 838.0f, 650.0f), FVector(0.42f, 0.10f, 0.42f), FRotator::ZeroRotator, MarkerRed, TEXT("WB_Story_SectorMarker"), 0.88f);
		SpawnStoryText(World, Store + FVector(60.0f, 828.0f, 790.0f), FRotator(0.0f, -90.0f, 0.0f), TEXT("CIVIL DEFENSE SURVEY\nSECTOR C-17 / HOLD"), FColor(96, 42, 34), 24.0f, TEXT("WB_Story_SurveyBoardText"));

		// A thin paper record left beside the meter makes the station feel recently used
		// relative to the rest of the ruined store and gives us a natural future pickup.
		SpawnStoryBox(World, Store + FVector(-60.0f, 680.0f, 176.0f), FVector(0.85f, 0.60f, 0.025f), FRotator(0.0f, 7.0f, 0.0f), BoardCream * 1.15f, TEXT("WB_Story_SurveyRecord"), 0.98f);
		SpawnStoryBox(World, Store + FVector(-35.0f, 676.0f, 180.0f), FVector(0.10f, 0.32f, 0.025f), FRotator(0.0f, 7.0f, 0.0f), MarkerRed, TEXT("WB_Story_SurveyRecordMark"), 0.90f);
	}
}

void UWildBoundStoreStorySubsystem::OnWorldBeginPlay(UWorld& InWorld)
{
	Super::OnWorldBeginPlay(InWorld);

	if (!InWorld.IsGameWorld())
	{
		return;
	}

	TrySpawnStoryPass();
	InWorld.GetTimerManager().SetTimer(
		StorySpawnTimer,
		this,
		&UWildBoundStoreStorySubsystem::TrySpawnStoryPass,
		0.20f,
		true,
		0.05f);
}

void UWildBoundStoreStorySubsystem::Deinitialize()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(StorySpawnTimer);
	}
	Super::Deinitialize();
}

void UWildBoundStoreStorySubsystem::TrySpawnStoryPass()
{
	UWorld* World = GetWorld();
	if (!World || !World->IsGameWorld())
	{
		return;
	}

	for (TActorIterator<AActor> It(World); It; ++It)
	{
		if (It->ActorHasTag(StoryTag))
		{
			World->GetTimerManager().ClearTimer(StorySpawnTimer);
			return;
		}
	}

	FVector TownOrigin;
	if (!FindTownOrigin(*World, TownOrigin))
	{
		return;
	}

	SpawnStoreStoryPass(*World, TownOrigin);
	World->GetTimerManager().ClearTimer(StorySpawnTimer);

	UE_LOG(LogTemp, Log, TEXT("WildBound environment: storefront storytelling pass spawned at %s."), *TownOrigin.ToCompactString());
}
