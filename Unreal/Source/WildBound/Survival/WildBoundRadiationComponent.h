#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "WildBoundRadiationComponent.generated.h"

class UAudioComponent;
class USoundWaveProcedural;

UCLASS(ClassGroup=(WildBound), meta=(BlueprintSpawnableComponent))
class WILDBOUND_API UWildBoundRadiationComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UWildBoundRadiationComponent();

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category="WildBound|Radiation")
	float CurrentExposure = 0.0f;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category="WildBound|Radiation")
	float AccumulatedDose = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="WildBound|Radiation", meta=(ClampMin="1.0"))
	float MaxDose = 100.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="WildBound|Radiation", meta=(ClampMin="100.0"))
	float OuterRadius = 1200.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="WildBound|Radiation", meta=(ClampMin="0.0"))
	float InnerRadius = 250.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="WildBound|Radiation", meta=(ClampMin="0.0"))
	float FullExposureDosePerSecond = 1.25f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="WildBound|Radiation", meta=(ClampMin="0.0", ClampMax="100.0"))
	float HighDoseDamageThreshold = 70.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="WildBound|Radiation", meta=(ClampMin="0.0"))
	float HighDoseDamagePerSecond = 1.5f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="WildBound|Radiation|Geiger", meta=(ClampMin="0.0", ClampMax="1.0"))
	float GeigerVolume = 0.72f;

	UFUNCTION(BlueprintPure, Category="WildBound|Radiation")
	float GetExposurePercent() const;

	UFUNCTION(BlueprintPure, Category="WildBound|Radiation")
	float GetDosePercent() const;

	UFUNCTION(BlueprintPure, Category="WildBound|Radiation")
	bool IsExposed() const { return CurrentExposure > 1.0f; }

private:
	UPROPERTY(Transient)
	TObjectPtr<UAudioComponent> GeigerAudioComponent;

	UPROPERTY(Transient)
	TObjectPtr<USoundWaveProcedural> GeigerSoundWave;

	float SecondsUntilNextGeigerClick = 0.0f;

	void InitializeGeigerAudio();
	void UpdateGeigerAudio(float DeltaTime);
	void QueueGeigerAudioFrame(bool bEmitClick, float ExposurePercent);
	float GetGeigerClickInterval(float ExposurePercent) const;
};
