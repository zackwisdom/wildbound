#include "WildBoundSaveSubsystem.h"

#include "WildBoundEvidenceLogSubsystem.h"
#include "WildBoundSaveGame.h"
#include "../Inventory/WildBoundInventoryComponent.h"
#include "../Player/WildBoundInteractionComponent.h"
#include "../Survival/WildBoundInjuryComponent.h"
#include "../Survival/WildBoundRadiationComponent.h"
#include "../Survival/WildBoundSurvivalComponent.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"

namespace
{
	const FName TownTag(TEXT("WildBoundTownBlockout"));
	const FName RadiationSetpieceTag(TEXT("WildBoundRadiationSetpiece"));
	const FName ContainerTag(TEXT("WBTypeContainer"));
	const FName ClueTag(TEXT("WBTypeClue"));
	const FName InspectedTag(TEXT("WBInspected"));
	const FName WaterGroupTag(TEXT("WBGroupWater"));
	const FName MedicalGroupTag(TEXT("WBGroupMedical"));
	const FName FoodGroupTag(TEXT("WBGroupFood"));
	const FName CommercialGateGroupTag(TEXT("WBPryGroupCommercialGate"));
}

void UWildBoundSaveSubsystem::OnWorldBeginPlay(UWorld& InWorld)
{
	Super::OnWorldBeginPlay(InWorld);

	if (!InWorld.IsGameWorld())
	{
		return;
	}

	TryInitializePersistence();
	InWorld.GetTimerManager().SetTimer(
		StartupTimer,
		this,
		&UWildBoundSaveSubsystem::TryInitializePersistence,
		0.5f,
		true,
		0.35f);
}

void UWildBoundSaveSubsystem::Deinitialize()
{
	if (bInitialized && !bApplyingLoad)
	{
		SaveNow(false);
	}

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(StartupTimer);
		World->GetTimerManager().ClearTimer(AutosaveTimer);
	}

	Super::Deinitialize();
}

bool UWildBoundSaveSubsystem::HasSaveGame() const
{
	return UGameplayStatics::DoesSaveGameExist(SaveSlotName, SaveUserIndex);
}

void UWildBoundSaveSubsystem::TryInitializePersistence()
{
	if (bInitialized || !ArePersistenceTargetsReady())
	{
		return;
	}

	if (HasSaveGame())
	{
		LoadNow(false);
	}
	else
	{
		bInitialized = true;
	}

	if (!bInitialized)
	{
		return;
	}

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(StartupTimer);
		World->GetTimerManager().SetTimer(
			AutosaveTimer,
			this,
			&UWildBoundSaveSubsystem::PerformAutosave,
			AutosaveIntervalSeconds,
			true,
			AutosaveIntervalSeconds);
	}

	UE_LOG(LogTemp, Log, TEXT("WildBound save system: persistence ready."));
}

void UWildBoundSaveSubsystem::PerformAutosave()
{
	SaveNow(false);
}

bool UWildBoundSaveSubsystem::SaveNow(bool bShowMessage)
{
	if (bApplyingLoad || !ArePersistenceTargetsReady())
	{
		return false;
	}

	UWildBoundSaveGame* SaveGame = Cast<UWildBoundSaveGame>(
		UGameplayStatics::CreateSaveGameObject(UWildBoundSaveGame::StaticClass()));
	if (!SaveGame || !CaptureSave(*SaveGame))
	{
		return false;
	}

	const bool bSaved = UGameplayStatics::SaveGameToSlot(SaveGame, SaveSlotName, SaveUserIndex);
	if (bSaved && bShowMessage && GEngine)
	{
		GEngine->AddOnScreenDebugMessage(
			91510,
			2.0f,
			FColor(170, 210, 165),
			TEXT("GAME SAVED"));
	}

	return bSaved;
}

bool UWildBoundSaveSubsystem::LoadNow(bool bShowMessage)
{
	if (bApplyingLoad || !ArePersistenceTargetsReady() || !HasSaveGame())
	{
		return false;
	}

	UWildBoundSaveGame* SaveGame = Cast<UWildBoundSaveGame>(
		UGameplayStatics::LoadGameFromSlot(SaveSlotName, SaveUserIndex));
	if (!SaveGame)
	{
		return false;
	}

	bApplyingLoad = true;
	const bool bLoaded = ApplySave(*SaveGame);
	bApplyingLoad = false;

	if (bLoaded)
	{
		bInitialized = true;
		if (bShowMessage && GEngine)
		{
			GEngine->AddOnScreenDebugMessage(
				91510,
				2.0f,
				FColor(190, 205, 155),
				TEXT("SAVE LOADED"));
		}
	}

	return bLoaded;
}

bool UWildBoundSaveSubsystem::CaptureSave(UWildBoundSaveGame& SaveGame) const
{
	UWorld* World = GetWorld();
	APlayerController* PlayerController = World ? World->GetFirstPlayerController() : nullptr;
	APawn* Pawn = PlayerController ? PlayerController->GetPawn() : nullptr;
	if (!World || !Pawn)
	{
		return false;
	}

	const UWildBoundInventoryComponent* Inventory = Pawn->FindComponentByClass<UWildBoundInventoryComponent>();
	const UWildBoundSurvivalComponent* Survival = Pawn->FindComponentByClass<UWildBoundSurvivalComponent>();
	const UWildBoundRadiationComponent* Radiation = Pawn->FindComponentByClass<UWildBoundRadiationComponent>();
	const UWildBoundInjuryComponent* Injury = Pawn->FindComponentByClass<UWildBoundInjuryComponent>();
	const UWildBoundInteractionComponent* Interaction = Pawn->FindComponentByClass<UWildBoundInteractionComponent>();
	const UWildBoundEvidenceLogSubsystem* EvidenceLog = World->GetSubsystem<UWildBoundEvidenceLogSubsystem>();

	if (!Inventory || !Survival || !Radiation || !Injury || !Interaction || !EvidenceLog)
	{
		return false;
	}

	SaveGame.PlayerLocation = Pawn->GetActorLocation();
	SaveGame.PlayerRotation = Pawn->GetActorRotation();
	SaveGame.InventoryStacks = Inventory->Stacks;
	SaveGame.HotbarSlots = Inventory->HotbarSlots;
	SaveGame.Health = Survival->Health;
	SaveGame.Hunger = Survival->Hunger;
	SaveGame.Thirst = Survival->Thirst;
	SaveGame.Stamina = Survival->Stamina;
	SaveGame.RadiationDose = Radiation->AccumulatedDose;
	SaveGame.RadiationTreatmentProtectionRemaining = Radiation->GetTreatmentProtectionRemaining();
	SaveGame.BleedingSeverity = Injury->BleedingSeverity;
	SaveGame.FractureSeverity = Injury->FractureSeverity;
	SaveGame.PainSeverity = Injury->PainSeverity;
	SaveGame.EvidenceEntries = EvidenceLog->GetEvidenceEntries();
	Interaction->BuildPersistentContainerStates(SaveGame.ContainerStates);

	SaveGame.InspectedClueLocations.Reset();
	for (TActorIterator<AActor> It(World); It; ++It)
	{
		AActor* Actor = *It;
		if (Actor && Actor->ActorHasTag(ClueTag) && Actor->ActorHasTag(InspectedTag))
		{
			SaveGame.InspectedClueLocations.Add(Actor->GetActorLocation());
		}
	}

	SaveGame.bWaterSupplyCollected = !HasActorsWithTag(WaterGroupTag);
	SaveGame.bMedicalSupplyCollected = !HasActorsWithTag(MedicalGroupTag);
	SaveGame.bFoodSupplyCollected = !HasActorsWithTag(FoodGroupTag);
	SaveGame.bCommercialGateOpened = !HasActorsWithTag(CommercialGateGroupTag);
	return true;
}

bool UWildBoundSaveSubsystem::ApplySave(const UWildBoundSaveGame& SaveGame)
{
	UWorld* World = GetWorld();
	APlayerController* PlayerController = World ? World->GetFirstPlayerController() : nullptr;
	APawn* Pawn = PlayerController ? PlayerController->GetPawn() : nullptr;
	if (!World || !Pawn)
	{
		return false;
	}

	UWildBoundInventoryComponent* Inventory = Pawn->FindComponentByClass<UWildBoundInventoryComponent>();
	UWildBoundSurvivalComponent* Survival = Pawn->FindComponentByClass<UWildBoundSurvivalComponent>();
	UWildBoundRadiationComponent* Radiation = Pawn->FindComponentByClass<UWildBoundRadiationComponent>();
	UWildBoundInjuryComponent* Injury = Pawn->FindComponentByClass<UWildBoundInjuryComponent>();
	UWildBoundInteractionComponent* Interaction = Pawn->FindComponentByClass<UWildBoundInteractionComponent>();
	UWildBoundEvidenceLogSubsystem* EvidenceLog = World->GetSubsystem<UWildBoundEvidenceLogSubsystem>();

	if (!Inventory || !Survival || !Radiation || !Injury || !Interaction || !EvidenceLog)
	{
		return false;
	}

	Pawn->SetActorLocationAndRotation(
		SaveGame.PlayerLocation,
		SaveGame.PlayerRotation,
		false,
		nullptr,
		ETeleportType::TeleportPhysics);

	Inventory->RestorePersistentState(SaveGame.InventoryStacks, SaveGame.HotbarSlots);
	Survival->RestorePersistentVitals(SaveGame.Health, SaveGame.Hunger, SaveGame.Thirst, SaveGame.Stamina);
	Radiation->RestorePersistentState(
		SaveGame.RadiationDose,
		SaveGame.RadiationTreatmentProtectionRemaining);
	Injury->RestorePersistentState(
		SaveGame.BleedingSeverity,
		SaveGame.FractureSeverity,
		SaveGame.PainSeverity);
	EvidenceLog->RestorePersistentEvidence(SaveGame.EvidenceEntries);
	Interaction->RestorePersistentContainerStates(SaveGame.ContainerStates);

	if (SaveGame.bWaterSupplyCollected)
	{
		DestroyActorsWithTag(WaterGroupTag);
	}
	if (SaveGame.bMedicalSupplyCollected)
	{
		DestroyActorsWithTag(MedicalGroupTag);
	}
	if (SaveGame.bFoodSupplyCollected)
	{
		DestroyActorsWithTag(FoodGroupTag);
	}
	if (SaveGame.bCommercialGateOpened)
	{
		DestroyActorsWithTag(CommercialGateGroupTag);
	}

	for (const FVector& SavedLocation : SaveGame.InspectedClueLocations)
	{
		AActor* BestMatch = nullptr;
		float BestDistanceSq = FMath::Square(140.0f);
		for (TActorIterator<AActor> It(World); It; ++It)
		{
			AActor* Candidate = *It;
			if (!Candidate || !Candidate->ActorHasTag(ClueTag))
			{
				continue;
			}

			const float DistanceSq = FVector::DistSquared(SavedLocation, Candidate->GetActorLocation());
			if (DistanceSq <= BestDistanceSq)
			{
				BestDistanceSq = DistanceSq;
				BestMatch = Candidate;
			}
		}

		if (BestMatch)
		{
			BestMatch->Tags.AddUnique(InspectedTag);
		}
	}

	return true;
}

bool UWildBoundSaveSubsystem::ArePersistenceTargetsReady() const
{
	UWorld* World = GetWorld();
	APlayerController* PlayerController = World ? World->GetFirstPlayerController() : nullptr;
	APawn* Pawn = PlayerController ? PlayerController->GetPawn() : nullptr;
	if (!World || !World->IsGameWorld() || !Pawn)
	{
		return false;
	}

	if (!Pawn->FindComponentByClass<UWildBoundInventoryComponent>()
		|| !Pawn->FindComponentByClass<UWildBoundSurvivalComponent>()
		|| !Pawn->FindComponentByClass<UWildBoundRadiationComponent>()
		|| !Pawn->FindComponentByClass<UWildBoundInjuryComponent>()
		|| !Pawn->FindComponentByClass<UWildBoundInteractionComponent>()
		|| !World->GetSubsystem<UWildBoundEvidenceLogSubsystem>())
	{
		return false;
	}

	bool bTownReady = false;
	bool bContainersReady = false;
	bool bRadiationReady = false;
	bool bWaterSupplyReady = false;
	bool bMedicalSupplyReady = false;
	bool bFoodSupplyReady = false;
	bool bCommercialGateReady = false;

	for (TActorIterator<AActor> It(World); It; ++It)
	{
		AActor* Actor = *It;
		if (!Actor)
		{
			continue;
		}

		bTownReady |= Actor->ActorHasTag(TownTag);
		bContainersReady |= Actor->ActorHasTag(ContainerTag);
		bRadiationReady |= Actor->ActorHasTag(RadiationSetpieceTag);
		bWaterSupplyReady |= Actor->ActorHasTag(WaterGroupTag);
		bMedicalSupplyReady |= Actor->ActorHasTag(MedicalGroupTag);
		bFoodSupplyReady |= Actor->ActorHasTag(FoodGroupTag);
		bCommercialGateReady |= Actor->ActorHasTag(CommercialGateGroupTag);

		if (bTownReady
			&& bContainersReady
			&& bRadiationReady
			&& bWaterSupplyReady
			&& bMedicalSupplyReady
			&& bFoodSupplyReady
			&& bCommercialGateReady)
		{
			return true;
		}
	}

	return false;
}

bool UWildBoundSaveSubsystem::HasActorsWithTag(FName Tag) const
{
	UWorld* World = GetWorld();
	if (!World || Tag.IsNone())
	{
		return false;
	}

	for (TActorIterator<AActor> It(World); It; ++It)
	{
		if (AActor* Actor = *It; Actor && Actor->ActorHasTag(Tag))
		{
			return true;
		}
	}

	return false;
}

void UWildBoundSaveSubsystem::DestroyActorsWithTag(FName Tag) const
{
	UWorld* World = GetWorld();
	if (!World || Tag.IsNone())
	{
		return;
	}

	TArray<AActor*> ActorsToDestroy;
	for (TActorIterator<AActor> It(World); It; ++It)
	{
		if (AActor* Actor = *It; Actor && Actor->ActorHasTag(Tag))
		{
			ActorsToDestroy.Add(Actor);
		}
	}

	for (AActor* Actor : ActorsToDestroy)
	{
		if (IsValid(Actor))
		{
			Actor->Destroy();
		}
	}
}
