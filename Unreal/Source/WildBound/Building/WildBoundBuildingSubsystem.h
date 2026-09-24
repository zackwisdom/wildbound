#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "TimerManager.h"
#include "WildBoundBuildingSubsystem.generated.h"

class AActor;
class AStaticMeshActor;
class UMaterialInstanceDynamic;
class UWildBoundInventoryComponent;
class UWorld;

USTRUCT(BlueprintType)
struct FWildBoundPlacedBuildState
{
	GENERATED_BODY()

	UPROPERTY(SaveGame, BlueprintReadOnly, Category="WildBound|Building")
	int32 BuildId = 0;

	UPROPERTY(SaveGame, BlueprintReadOnly, Category="WildBound|Building")
	FName BuildTypeId = NAME_None;

	UPROPERTY(SaveGame, BlueprintReadOnly, Category="WildBound|Building")
	FVector Location = FVector::ZeroVector;

	UPROPERTY(SaveGame, BlueprintReadOnly, Category="WildBound|Building")
	FRotator Rotation = FRotator::ZeroRotator;

	UPROPERTY(SaveGame, BlueprintReadOnly, Category="WildBound|Building")
	bool bUtilityEnabled = true;

	UPROPERTY(SaveGame, BlueprintReadOnly, Category="WildBound|Building")
	int32 StoredUtilityUnits = 0;

	UPROPERTY(SaveGame, BlueprintReadOnly, Category="WildBound|Building")
	float UtilityProgress = 0.0f;
};

UCLASS()
class WILDBOUND_API UWildBoundBuildingSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	virtual void OnWorldBeginPlay(UWorld& InWorld) override;
	virtual void Deinitialize() override;

	bool BeginPlacement(
		FName BuildTypeId,
		const FString& DisplayName,
		const TMap<FName, int32>& MaterialCosts);

	UFUNCTION(BlueprintPure, Category="WildBound|Building")
	bool IsPlacementActive() const { return bPlacementActive; }

	FString GetUtilityInteractionPrompt(const AActor* Actor) const;
	bool TryUseUtility(AActor* Actor);
	float GetRestHealthRecoveryAt(const FVector& Location) const;
	FString GetShelterProgressionNameAt(const FVector& Location) const;

	const TArray<FWildBoundPlacedBuildState>& GetPlacedBuildStates() const { return PlacedBuilds; }
	void RestorePlacedBuilds(const TArray<FWildBoundPlacedBuildState>& SavedBuilds);

private:
	FTimerHandle BuildingUpdateTimer;
	FTimerHandle UtilityUpdateTimer;
	TWeakObjectPtr<AStaticMeshActor> PreviewActor;
	TWeakObjectPtr<UMaterialInstanceDynamic> PreviewMaterial;
	TArray<FWildBoundPlacedBuildState> PlacedBuilds;
	TMap<TWeakObjectPtr<AActor>, int32> BuildIdByActor;
	TMap<FName, int32> ActiveMaterialCosts;

	FName ActiveBuildTypeId = NAME_None;
	FString ActiveDisplayName;
	FVector PreviewLocation = FVector::ZeroVector;
	FRotator PreviewRotation = FRotator::ZeroRotator;
	bool bPlacementActive = false;
	bool bPlacementValid = false;
	bool bGridSnapEnabled = true;
	bool bPieceSnapped = false;
	bool bRelocatingBuild = false;
	int32 RelocatingBuildId = 0;
	FWildBoundPlacedBuildState RelocationOriginalState;
	int32 NextBuildId = 1;
	int32 PendingDismantleBuildId = 0;
	float DismantleHoldStartedAt = -1.0f;
	float PlacementStartedAt = 0.0f;
	float CurrentYaw = 0.0f;

	void UpdateBuildingMode();
	void UpdateManagementMode();
	void UpdateUtilities();
	void RefreshPoweredLights();
	void UpdatePreviewTransform();
	void TryPlaceActiveBuild();
	void BeginRelocation(int32 BuildId);
	void DismantleBuild(int32 BuildId);
	void CancelPlacement(bool bShowMessage = true);
	void EnsurePreviewActor();
	void DestroyPreviewActor();
	void UpdatePreviewMaterial();
	bool CanAffordActiveBuild() const;
	bool ConsumeActiveBuildMaterials();
	bool ValidatePlacement(
		const FVector& Location,
		const FRotator& Rotation,
		AActor* GroundActor) const;
	bool SpawnPlacedBuild(
		FName BuildTypeId,
		const FVector& Location,
		const FRotator& Rotation,
		TArray<AActor*>* OutSpawnedActors = nullptr);
	void RegisterBuildActors(int32 BuildId, const TArray<AActor*>& Actors);
	void DestroyBuildActors(int32 BuildId);
	void DestroyAllPlacedBuildActors();
	int32 FindBuildIdForActor(const AActor* Actor) const;
	FWildBoundPlacedBuildState* FindBuildState(int32 BuildId);
	const FWildBoundPlacedBuildState* FindBuildState(int32 BuildId) const;
	bool CanManageBuild(int32 BuildId, bool bShowMessage) const;
	TMap<FName, int32> GetMaterialCostsForBuild(FName BuildTypeId) const;
	FString GetBuildDisplayName(FName BuildTypeId) const;
	bool TryApplyPieceSnap(FVector& InOutLocation, FRotator& InOutRotation) const;
	bool IsDuplicatePlacement(const FVector& Location, FName IgnoreBuildType = NAME_None, int32 IgnoreBuildId = 0) const;
	bool IsPowerAvailableAt(const FVector& Location) const;
	int32 GetShelterProgressionTierAt(const FVector& Location) const;

	UWildBoundInventoryComponent* GetPlayerInventory() const;
	FVector GetBuildHalfExtents(FName BuildTypeId) const;
	float GetBuildVerticalOffset(FName BuildTypeId) const;
	FVector SnapLocation(const FVector& Location) const;
};
