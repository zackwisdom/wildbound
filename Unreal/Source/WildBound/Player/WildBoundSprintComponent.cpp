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
	}

	SurvivalComponent = GetOwner() ? GetOwner()->FindComponentByClass<UWildBoundSurvivalComponent>() : nullptr;
	InventoryComponent = GetOwner() ? GetOwner()->FindComponentByClass<UWildBoundInventoryComponent>() : nullptr;
}

void UWildBoundSprintComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	SetSprinting(false);
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
		const float DrainMultiplier = FMath::Lerp(
			1.0f,
			MaximumEncumberedSprintDrainMultiplier,
			EncumbranceSeverity);
		const float DrainAmount = SprintStaminaDrainPerSecond * DrainMultiplier * DeltaTime;
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
	return FMath::Lerp(1.0f, MinimumEncumberedSpeedMultiplier, GetEncumbranceSeverity());
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
