#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "TimerManager.h"
#include "WildBoundEvidenceLogSubsystem.generated.h"

class SWidget;
class UWorld;

USTRUCT(BlueprintType)
struct FWildBoundEvidenceEntry
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category="WildBound|Evidence")
	FName EvidenceId = NAME_None;

	UPROPERTY(BlueprintReadOnly, Category="WildBound|Evidence")
	FString Title;

	UPROPERTY(BlueprintReadOnly, Category="WildBound|Evidence")
	FString Source;

	UPROPERTY(BlueprintReadOnly, Category="WildBound|Evidence")
	FString Body;

	UPROPERTY(BlueprintReadOnly, Category="WildBound|Evidence")
	float DiscoveredAtSeconds = 0.0f;
};

struct FWildBoundMysteryConclusion
{
	FName ConclusionId = NAME_None;
	FString Title;
	FString Summary;
};

UCLASS()
class WILDBOUND_API UWildBoundEvidenceLogSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	virtual void OnWorldBeginPlay(UWorld& InWorld) override;
	virtual void Deinitialize() override;

	UFUNCTION(BlueprintCallable, Category="WildBound|Evidence")
	bool RecordEvidence(FName EvidenceId, const FString& Title, const FString& Source, const FString& Body);

	UFUNCTION(BlueprintPure, Category="WildBound|Evidence")
	bool HasEvidence(FName EvidenceId) const;

	const TArray<FWildBoundEvidenceEntry>& GetEvidenceEntries() const { return EvidenceEntries; }

	UFUNCTION(BlueprintPure, Category="WildBound|Evidence")
	bool IsEvidenceLogOpen() const { return bEvidenceLogOpen; }

private:
	TArray<FWildBoundEvidenceEntry> EvidenceEntries;
	TArray<FWildBoundMysteryConclusion> UnlockedConclusions;
	TSharedPtr<SWidget> EvidenceViewportRoot;
	FTimerHandle EvidenceInputTimer;
	bool bEvidenceLogOpen = false;

	void UpdateInput();
	void EnsureEvidenceWidget();
	void SetEvidenceLogOpen(bool bOpen);
	void RemoveEvidenceWidget();
	bool CanOpenEvidenceLog() const;
	void EvaluateMysteryProgress();
	bool UnlockConclusion(FName ConclusionId, const FString& Title, const FString& Summary);
	FText BuildEvidenceText() const;
	FString BuildCaseAnalysisText() const;
	FString BuildTimelineText() const;
};
