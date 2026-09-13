#include "WildBoundSprintComponent.h"

#include "../Inventory/WildBoundInventoryComponent.h"
#include "../Survival/WildBoundSurvivalComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "InputCoreTypes.h"

UWildBoundSprintComponent::UWildBoundSprintComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = true;
}

void UWildBoundSprintComponent::BeginPlay()
{
	Super::BeginPlay();

	ACharacter* Character = Cast<ACharacter>(GetOwner());
	CharacterOwner = Character;

	if (Character && Character->GetCharacterMovement())
	{
		BaseWalkSpeed = Character->GetCharacterMovement()->MaxWalkSpeed;
		BaseMaxAcceleration = Character->GetCharacterMovement()->MaxAcceleration;
	}

	SurvivalComponent = GetOwner() ? GetOwner()->FindComponentByClass<UWildBoundSurvivalComponent>() : nullptr;
	InventoryComponent = GetOwner() ? GetOwner()->FindComponentByClass<UWildBoundInventoryComponent>() : nullptr;
}

void UWildBoundSprintComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	bIsSprinting = false;
	if (ACharacter* Character = CharacterOwner.Get())
	{
		if (UCharacterMovementComponent* Movement = Character->GetCharacterMovement())
		{
			Movement->MaxWalkSpeed = BaseWalkSpeed;
			Movement->MaxAcceleration = BaseMaxAcceleration;
		}
	}
	Super::EndPlay(EndPlayReason);
}

void UWildBoundSprintComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	ACharacter* Character = CharacterOwner.Get();
	UWildBoundSurvivalComponent* Survival = SurvivalComponent.Get();
	if (!Character || !Survival || !Survival->IsAlive())
	{
		SetSprinting(false);
		return;
	}

	if (!InventoryComponent.IsValid() && GetOwner())
	{
		InventoryComponent = GetOwner()->FindComponentByClass<UWildBoundInventoryComponent>();
	}

	APlayerController* PlayerController = Cast<APlayerController>(Character->GetController());
	const bool bSprintKeyDown = PlayerController &&
		(PlayerController->IsInputKeyDown(EKeys::LeftShift) || PlayerController->IsInputKeyDown(EKeys::RightShift));

	const bool bMoving = Character->GetVelocity().SizeSquared2D() > FMath::Square(10.0f);
	const float EncumbranceSeverity = GetEncumbranceSeverity();
	const bool bOverEncumbered = EncumbranceSeverity > 0.0f;

	if (bMoving && bOverEncumbered)
	{
		const float WalkingPenaltyScale = FMath::Lerp(0.35f, 1.0f, EncumbranceSeverity);
		Survival->ConsumeStamina(
			EncumberedWalkingStaminaDrainPerSecond * WalkingPenaltyScale * DeltaTime);
	}

	const bool bCanBeginSprint = bIsSprinting || Survival->Stamina >= MinimumStaminaToStartSprint;
	const bool bWantsSprint = bSprintKeyDown && bMoving && bCanBeginSprint;

	if (bWantsSprint)
	{
		const float EncumbranceDrainMultiplier = FMath::Lerp(
			1.0f,
			MaximumEncumberedSprintDrainMultiplier,
			EncumbranceSeverity);
		const float NutritionDrainMultiplier = Survival->GetNutritionSprintDrainMultiplier();
		const float DrainAmount = SprintStaminaDrainPerSecond
			* EncumbranceDrainMultiplier
			* NutritionDrainMultiplier
			* DeltaTime;

		if (Survival->ConsumeStamina(DrainAmount))
		{
			SetSprinting(true);
			ApplyMovementSpeed();
			return;
		}
	}

	SetSprinting(false);
	ApplyMovementSpeed();
}

float UWildBoundSprintComponent::GetEncumbranceSeverity() const
{
	const UWildBoundInventoryComponent* Inventory = InventoryComponent.Get();
	if (!Inventory || !Inventory->IsOverEncumbered())
	{
		return 0.0f;
	}

	// Reaches full penalty at 200% of the normal carry limit.
	return FMath::Clamp(Inventory->GetCarryWeightRatio() - 1.0f, 0.0f, 1.0f);
}

float UWildBoundSprintComponent::GetCurrentSpeedMultiplier() const
{
	const float EncumbranceMultiplier = FMath::Lerp(
		1.0f,
		MinimumEncumberedSpeedMultiplier,
		GetEncumbranceSeverity());
	const UWildBoundSurvivalComponent* Survival = SurvivalComponent.Get();
	const float NutritionMultiplier = Survival ? Survival->GetNutritionMoveSpeedMultiplier() : 1.0f;
	return EncumbranceMultiplier * NutritionMultiplier;
}

float UWildBoundSprintComponent::GetFatigueAccelerationMultiplier() const
{
	const UWildBoundSurvivalComponent* Survival = SurvivalComponent.Get();
	if (!Survival)
	{
		return 1.0f;
	}

	constexpr float LowStaminaThreshold = 0.35f;
	const float StaminaSeverity = FMath::Clamp(
		(LowStaminaThreshold - Survival->GetStaminaPercent()) / LowStaminaThreshold,
		0.0f,
		1.0f);

	const float NutritionMultiplier = Survival->GetNutritionMoveSpeedMultiplier();
	const float NutritionRange = FMath::Max(1.0f - Survival->MinimumNutritionMoveSpeedMultiplier, KINDA_SMALL_NUMBER);
	const float NutritionSeverity = FMath::Clamp(
		(1.0f - NutritionMultiplier) / NutritionRange,
		0.0f,
		1.0f);

	const float CombinedSeverity = FMath::Clamp(
		1.0f - ((1.0f - StaminaSeverity) * (1.0f - NutritionSeverity)),
		0.0f,
		1.0f);
	return FMath::Lerp(1.0f, MinimumFatiguedAccelerationMultiplier, CombinedSeverity);
}

void UWildBoundSprintComponent::ApplyMovementSpeed()
{
	ACharacter* Character = CharacterOwner.Get();
	UCharacterMovementComponent* Movement = Character ? Character->GetCharacterMovement() : nullptr;
	if (!Movement)
	{
		return;
	}

	const float TargetBaseSpeed = bIsSprinting ? SprintSpeed : BaseWalkSpeed;
	Movement->MaxWalkSpeed = TargetBaseSpeed * GetCurrentSpeedMultiplier();
	Movement->MaxAcceleration = BaseMaxAcceleration * GetFatigueAccelerationMultiplier();
}

void UWildBoundSprintComponent::SetSprinting(bool bNewSprinting)
{
	if (bIsSprinting == bNewSprinting)
	{
		return;
	}

	bIsSprinting = bNewSprinting;
	ApplyMovementSpeed();
}
