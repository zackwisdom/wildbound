#include "WildBoundInteractionSubsystem.h"

#include "../Player/WildBoundInteractionComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMeshActor.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"

namespace
{
	const FName StoryTag(TEXT("WildBoundStoreStory"));
	const FName TownTag(TEXT("WildBoundTownBlockout"));
	const FName InteractableTag(TEXT("WBInteractable"));
	const FName SupplyTypeTag(TEXT("WBTypeSupply"));
	const FName ClueTypeTag(TEXT("WBTypeClue"));

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

	AStaticMeshActor* FindStoryActorNear(UWorld& World, const FVector& Location, float MaxDistance)
	{
		AStaticMeshActor* BestActor = nullptr;
		float BestDistanceSq = FMath::Square(MaxDistance);

		for (TActorIterator<AStaticMeshActor> It(&World); It; ++It)
		{
			AStaticMeshActor* Actor = *It;
			if (!Actor || !Actor->ActorHasTag(StoryTag))
			{
				continue;
			}

			const float DistanceSq = FVector::DistSquared(Actor->GetActorLocation(), Location);
			if (DistanceSq <= BestDistanceSq)
			{
				BestDistanceSq = DistanceSq;
				BestActor = Actor;
			}
		}

		return BestActor;
	}

	void AddGroupTagNear(UWorld& World, const FVector& Location, float Radius, const FName& GroupTag)
	{
		const float RadiusSq = FMath::Square(Radius);
		for (TActorIterator<AStaticMeshActor> It(&World); It; ++It)
		{
			AStaticMeshActor* Actor = *It;
			if (!Actor || !Actor->ActorHasTag(StoryTag))
			{
				continue;
			}

			if (FVector::DistSquared(Actor->GetActorLocation(), Location) <= RadiusSq)
			{
				Actor->Tags.AddUnique(GroupTag);
			}
		}
	}

	void ConfigureTraceTarget(AStaticMeshActor& Actor)
	{
		UStaticMeshComponent* Mesh = Actor.GetStaticMeshComponent();
		if (!Mesh)
		{
			return;
		}

		Mesh->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
		Mesh->SetCollisionResponseToAllChannels(ECR_Ignore);
		Mesh->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
		Mesh->SetGenerateOverlapEvents(false);
	}

	bool ConfigureSupply(
		UWorld& World,
		const FVector& Location,
		const FName& ItemTag,
		const FName& GroupTag)
	{
		AStaticMeshActor* Actor = FindStoryActorNear(World, Location, 12.0f);
		if (!Actor)
		{
			return false;
		}

		Actor->Tags.AddUnique(InteractableTag);
		Actor->Tags.AddUnique(SupplyTypeTag);
		Actor->Tags.AddUnique(ItemTag);
		Actor->Tags.AddUnique(GroupTag);
		ConfigureTraceTarget(*Actor);
		return true;
	}

	bool ConfigureClue(UWorld& World, const FVector& Location)
	{
		AStaticMeshActor* Actor = FindStoryActorNear(World, Location, 12.0f);
		if (!Actor)
		{
			return false;
		}

		Actor->Tags.AddUnique(InteractableTag);
		Actor->Tags.AddUnique(ClueTypeTag);
		Actor->Tags.AddUnique(TEXT("WBClueC17"));
		ConfigureTraceTarget(*Actor);
		return true;
	}
}

void UWildBoundInteractionSubsystem::OnWorldBeginPlay(UWorld& InWorld)
{
	Super::OnWorldBeginPlay(InWorld);

	if (!InWorld.IsGameWorld())
	{
		return;
	}

	EnsureInteractionSetup();
	InWorld.GetTimerManager().SetTimer(
		InteractionSetupTimer,
		this,
		&UWildBoundInteractionSubsystem::EnsureInteractionSetup,
		0.5f,
		true,
		0.15f);
}

void UWildBoundInteractionSubsystem::Deinitialize()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(InteractionSetupTimer);
	}

	Super::Deinitialize();
}

void UWildBoundInteractionSubsystem::EnsureInteractionSetup()
{
	UWorld* World = GetWorld();
	if (!World || !World->IsGameWorld())
	{
		return;
	}

	APlayerController* PlayerController = World->GetFirstPlayerController();
	APawn* Pawn = PlayerController ? PlayerController->GetPawn() : nullptr;
	if (Pawn && !Pawn->FindComponentByClass<UWildBoundInteractionComponent>())
	{
		UWildBoundInteractionComponent* Interaction = NewObject<UWildBoundInteractionComponent>(Pawn, TEXT("WildBoundInteraction"));
		Pawn->AddInstanceComponent(Interaction);
		Interaction->RegisterComponent();
		UE_LOG(LogTemp, Log, TEXT("WildBound interaction: player interaction component attached."));
	}

	if (bStoryInteractablesReady)
	{
		return;
	}

	FVector TownOrigin;
	if (!FindTownOrigin(*World, TownOrigin))
	{
		return;
	}

	const FVector Store = TownOrigin + FVector(1950.0f, 1800.0f, 0.0f);

	const FName WaterGroup(TEXT("WBGroupWater"));
	const FName MedicalGroup(TEXT("WBGroupMedical"));
	const FName FoodGroup(TEXT("WBGroupFood"));

	const FVector WaterCase = Store + FVector(-575.0f, 5.0f, 150.0f);
	const FVector MedKit = Store + FVector(40.0f, -115.0f, 170.0f);
	const FVector FoodCarton = Store + FVector(-650.0f, 650.0f, 62.0f);
	const FVector SurveyBoard = Store + FVector(360.0f, 850.0f, 570.0f);

	const bool bWaterReady = ConfigureSupply(*World, WaterCase, TEXT("WBItemWater"), WaterGroup);
	const bool bMedicalReady = ConfigureSupply(*World, MedKit, TEXT("WBItemMedical"), MedicalGroup);
	const bool bFoodReady = ConfigureSupply(*World, FoodCarton, TEXT("WBItemFood"), FoodGroup);
	const bool bClueReady = ConfigureClue(*World, SurveyBoard);

	if (bWaterReady)
	{
		AddGroupTagNear(*World, WaterCase, 20.0f, WaterGroup);
		for (int32 BottleIndex = 0; BottleIndex < 4; ++BottleIndex)
		{
			AddGroupTagNear(
				*World,
				Store + FVector(-690.0f + BottleIndex * 75.0f, -5.0f, 205.0f),
				8.0f,
				WaterGroup);
		}
	}

	if (bMedicalReady)
	{
		AddGroupTagNear(*World, MedKit, 20.0f, MedicalGroup);
		AddGroupTagNear(*World, Store + FVector(40.0f, -190.0f, 171.0f), 12.0f, MedicalGroup);
	}

	if (bFoodReady)
	{
		AddGroupTagNear(*World, FoodCarton, 20.0f, FoodGroup);
		AddGroupTagNear(*World, Store + FVector(-510.0f, 635.0f, 128.0f), 20.0f, FoodGroup);
	}

	bStoryInteractablesReady = bWaterReady && bMedicalReady && bFoodReady && bClueReady;
	if (bStoryInteractablesReady)
	{
		UE_LOG(LogTemp, Log, TEXT("WildBound interaction: storefront supplies and C-17 clue are interactable."));
	}
}
