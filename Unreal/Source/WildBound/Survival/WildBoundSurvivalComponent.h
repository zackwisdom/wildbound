#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "WildBoundSurvivalComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FWildBoundSurvivalStatsChanged);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FWildBoundPlayerDied);

UCLASS(ClassGroup=(WildBound), meta=(BlueprintSpawnableComponent))
class WILDBOUND_API UWildBoundSurvivalComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UWildBoundSurvivalComponent();

	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	UPROPERTY(BlueprintAssignable, Category="WildBound|Survival")
	FWildBoundSurvivalStatsChanged OnStatsChanged;

	UPROPERTY(BlueprintAssignable, Category="WildBound|Survival")
	FWildBoundPlayerDied OnPlayerDied;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="WildBound|Survival|Maximums", meta=(ClampMin="1.0"))
	float MaxHealth = 100.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="WildBound|Survival|Maximums", meta=(ClampMin="1.0"))
	float MaxHunger = 100.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="WildBound|Survival|Maximums", meta=(ClampMin="1.0"))
	float MaxThirst = 100.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="WildBound|Survival|Maximums", meta=(ClampMin="1.0"))
	float MaxStamina = 100.0f;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category="WildBound|Survival|Vitals")
	float Health = 100.0f;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category="WildBound|Survival|Vitals")
	float Hunger = 100.0f;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category="WildBound|Survival|Vitals")
	float Thirst = 100.0f;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category="WildBound|Survival|Vitals")
	float Stamina = 100.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="WildBound|Survival|Rates", meta=(ClampMin="0.0"))
	float HungerDrainPerSecond = 0.08f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="WildBound|Survival|Rates", meta=(ClampMin="0.0"))
	float ThirstDrainPerSecond = 0.12f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="WildBound|Survival|Rates", meta=(ClampMin="0.0"))
	float StaminaRegenPerSecond = 14.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="WildBound|Survival|Rates", meta=(ClampMin="1.0"))
	float StationaryStaminaRegenMultiplier = 2.25f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="WildBound|Survival|Rates", meta=(ClampMin="0.0"))
	float StationaryVelocityThreshold = 5.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="WildBound|Survival|Rates", meta=(ClampMin="0.0", ClampMax="1.0"))
	float EncumberedMovingStaminaRegenMultiplier = 0.20f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="WildBound|Survival|Rates", meta=(ClampMin="0.0"))
	float StarvationDamagePerSecond = 2.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="WildBound|Survival|Rates", meta=(ClampMin="0.0"))
	float DehydrationDamagePerSecond = 4.0f;

	UFUNCTION(BlueprintCallable, Category="WildBound|Survival")
	bool ConsumeStamina(float Amount);

	UFUNCTION(BlueprintCallable, Category="WildBound|Survival")
	void AddHunger(float Amount);

	UFUNCTION(BlueprintCallable, Category="WildBound|Survival")
	void AddThirst(float Amount);

	UFUNCTION(BlueprintCallable, Category="WildBound|Survival")
	void RestoreStamina(float Amount);

	UFUNCTION(BlueprintCallable, Category="WildBound|Survival")
	void ApplySurvivalDamage(float Amount);

	UFUNCTION(BlueprintCallable, Category="WildBound|Survival")
	void Heal(float Amount);

	UFUNCTION(BlueprintPure, Category="WildBound|Survival")
	bool IsAlive() const { return Health > 0.0f; }

	UFUNCTION(BlueprintPure, Category="WildBound|Survival")
	float GetHealthPercent() const;

	UFUNCTION(BlueprintPure, Category="WildBound|Survival")
	float GetHungerPercent() const;

	UFUNCTION(BlueprintPure, Category="WildBound|Survival")
	float GetThirstPercent() const;

	UFUNCTION(BlueprintPure, Category="WildBound|Survival")
	float GetStaminaPercent() const;

private:
	bool bDeathBroadcast = false;

	void BroadcastStatsChanged();
};
