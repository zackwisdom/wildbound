#include "WildBoundSprintComponent.h"

#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "InputCoreTypes.h"
#include "Survival/WildBoundSurvivalComponent.h"

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

	APlayerController* PlayerController = Cast<APlayerController>(Character->GetController());
	const bool bSprintKeyDown = PlayerController &&
		(PlayerController->IsInputKeyDown(EKeys::LeftShift) || PlayerController->IsInputKeyDown(EKeys::RightShift));

	const bool bMoving = Character->GetVelocity().SizeSquared2D() > FMath::Square(10.0f);
	const bool bCanBeginSprint = bIsSprinting || Survival->Stamina >= MinimumStaminaToStartSprint;
	const bool bWantsSprint = bSprintKeyDown && bMoving && bCanBeginSprint;

	if (bWantsSprint)
	{
		const float DrainAmount = SprintStaminaDrainPerSecond * DeltaTime;
		if (Survival->ConsumeStamina(DrainAmount))
		{
			SetSprinting(true);
			return;
		}
	}

	SetSprinting(false);
}

void UWildBoundSprintComponent::SetSprinting(bool bNewSprinting)
{
	if (bIsSprinting == bNewSprinting)
	{
		return;
	}

	bIsSprinting = bNewSprinting;

	if (ACharacter* Character = CharacterOwner.Get())
	{
		if (UCharacterMovementComponent* Movement = Character->GetCharacterMovement())
		{
			Movement->MaxWalkSpeed = bIsSprinting ? SprintSpeed : BaseWalkSpeed;
		}
	}
}
