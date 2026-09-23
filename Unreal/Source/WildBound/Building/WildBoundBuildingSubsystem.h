#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "TimerManager.h"
#include "WildBoundBuildingSubsystem.generated.h"

class AStaticMeshActor;
class UMaterialInstanceDynamic;
class UWildBoundInventoryComponent;
class UWorld;

USTRUCT(BlueprintType)
struct FWildBoundPlacedBuildState
{
	GENERATED_BODY()

	UPROPERTY(SaveGame, BlueprintReadOnly, Category="WildBound|Building")
	FName BuildTypeId = NAME_None;

	UPROPERTY(SaveGame, BlueprintReadOnly, Category="WildBound|Building")
	FVector Location = FVector::ZeroVector;

	UPROPERTY(SaveGame, BlueprintReadOnly, Category="WildBound|Building")
	FRotator Rotation = FRotator::ZeroRotator;
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

	const TArray<FWildBoundPlacedBuildState>& GetPlacedBuildStates() const { return PlacedBuilds; }
	void RestorePlacedBuilds(const TArray<FWildBoundPlacedBuildState>& SavedBuilds);

private:
	FTimerHandle BuildingUpdateTimer;
	TWeakObjectPtr<AStaticMeshActor> PreviewActor;
	TWeakObjectPtr<UMaterialInstanceDynamic> PreviewMaterial;
	TArray<FWildBoundPlacedBuildState> PlacedBuilds;
	TMap<FName, int32> ActiveMaterialCosts;

	FName ActiveBuildTypeId = NAME_None;
	FString ActiveDisplayName;
	FVector PreviewLocation = FVector::ZeroVector;
	FRotator PreviewRotation = FRotator::ZeroRotator;
	bool bPlacementActive = false;
	bool bPlacementValid = false;
	bool bGridSnapEnabled = true;
	float PlacementStartedAt = 0.0f;
	float CurrentYaw = 0.0f;

	void UpdateBuildingMode();
	void UpdatePreviewTransform();
	void TryPlaceActiveBuild();
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
	void DestroyAllPlacedBuildActors();

	UWildBoundInventoryComponent* GetPlayerInventory() const;
	FVector GetBuildHalfExtents(FName BuildTypeId) const;
	float GetBuildVerticalOffset(FName BuildTypeId) const;
	FVector SnapLocation(const FVector& Location) const;
};
