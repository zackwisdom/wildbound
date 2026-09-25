#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "../Building/WildBoundBuildingSubsystem.h"
#include "../Inventory/WildBoundInventoryComponent.h"
#include "../Player/WildBoundInteractionComponent.h"
#include "WildBoundEvidenceLogSubsystem.h"
#include "WildBoundSaveGame.generated.h"

UCLASS()
class WILDBOUND_API UWildBoundSaveGame : public USaveGame
{
	GENERATED_BODY()

public:
	UPROPERTY(SaveGame)
	int32 SaveVersion = 6;

	UPROPERTY(SaveGame)
	FVector PlayerLocation = FVector::ZeroVector;

	UPROPERTY(SaveGame)
	FRotator PlayerRotation = FRotator::ZeroRotator;

	UPROPERTY(SaveGame)
	TArray<FWildBoundInventoryStack> InventoryStacks;

	UPROPERTY(SaveGame)
	TArray<FName> HotbarSlots;

	UPROPERTY(SaveGame)
	float Health = 100.0f;

	UPROPERTY(SaveGame)
	float Hunger = 100.0f;

	UPROPERTY(SaveGame)
	float Thirst = 100.0f;

	UPROPERTY(SaveGame)
	float Stamina = 100.0f;

	UPROPERTY(SaveGame)
	float RadiationDose = 0.0f;

	UPROPERTY(SaveGame)
	float RadiationTreatmentProtectionRemaining = 0.0f;

	UPROPERTY(SaveGame)
	float BleedingSeverity = 0.0f;

	UPROPERTY(SaveGame)
	float FractureSeverity = 0.0f;

	UPROPERTY(SaveGame)
	float PainSeverity = 0.0f;

	UPROPERTY(SaveGame)
	TArray<FWildBoundEvidenceEntry> EvidenceEntries;

	UPROPERTY(SaveGame)
	TArray<FWildBoundPersistentContainerState> ContainerStates;

	UPROPERTY(SaveGame)
	TArray<FVector> InspectedClueLocations;

	UPROPERTY(SaveGame)
	bool bWaterSupplyCollected = false;

	UPROPERTY(SaveGame)
	bool bMedicalSupplyCollected = false;

	UPROPERTY(SaveGame)
	bool bFoodSupplyCollected = false;

	UPROPERTY(SaveGame)
	bool bCommercialGateOpened = false;

	UPROPERTY(SaveGame)
	int32 OnboardingProgressStage = 0;

	UPROPERTY(SaveGame)
	int32 TutorialHintFlags = 0;

	UPROPERTY(SaveGame)
	TArray<FWildBoundPlacedBuildState> PlacedBuilds;
};
