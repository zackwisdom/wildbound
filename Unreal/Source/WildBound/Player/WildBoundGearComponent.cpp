#include "WildBoundGearComponent.h"

#include "../Inventory/WildBoundInventoryComponent.h"
#include "../Survival/WildBoundRadiationComponent.h"
#include "GameFramework/Actor.h"

namespace
{
	const FName ReinforcedBackpackItemId(TEXT("ReinforcedBackpack"));
	const FName FilterMaskItemId(TEXT("FilterMask"));
}

UWildBoundGearComponent::UWildBoundGearComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = true;
	PrimaryComponentTick.TickInterval = 0.25f;
}

void UWildBoundGearComponent::BeginPlay()
{
	Super::BeginPlay();
	RefreshComponentReferences();
	ApplyGearEffects();
}

void UWildBoundGearComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	RestoreBaseValues();
	Super::EndPlay(EndPlayReason);
}

void UWildBoundGearComponent::TickComponent(
	float DeltaTime,
	ELevelTick TickType,
	FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	RefreshComponentReferences();
	ApplyGearEffects();
}

void UWildBoundGearComponent::RefreshComponentReferences()
{
	AActor* Owner = GetOwner();
	if (!Owner)
	{
		return;
	}

	if (!InventoryComponent.IsValid())
	{
		InventoryComponent = Owner->FindComponentByClass<UWildBoundInventoryComponent>();
		if (InventoryComponent.IsValid() && BaseCarryWeight < 0.0f)
		{
			BaseCarryWeight = InventoryComponent->MaxCarryWeight;
		}
	}

	if (!RadiationComponent.IsValid())
	{
		RadiationComponent = Owner->FindComponentByClass<UWildBoundRadiationComponent>();
		if (RadiationComponent.IsValid() && BaseRadiationDosePerSecond < 0.0f)
		{
			BaseRadiationDosePerSecond = RadiationComponent->FullExposureDosePerSecond;
		}
	}
}

bool UWildBoundGearComponent::HasReinforcedBackpack() const
{
	const UWildBoundInventoryComponent* Inventory = InventoryComponent.Get();
	return Inventory && Inventory->HasItem(ReinforcedBackpackItemId, 1);
}

bool UWildBoundGearComponent::HasFilterMask() const
{
	const UWildBoundInventoryComponent* Inventory = InventoryComponent.Get();
	return Inventory && Inventory->HasItem(FilterMaskItemId, 1);
}

void UWildBoundGearComponent::ApplyGearEffects()
{
	if (UWildBoundInventoryComponent* Inventory = InventoryComponent.Get())
	{
		if (BaseCarryWeight < 0.0f)
		{
			BaseCarryWeight = Inventory->MaxCarryWeight;
		}

		const float DesiredCarryWeight = BaseCarryWeight
			+ (HasReinforcedBackpack() ? ReinforcedBackpackCapacityBonus : 0.0f);
		Inventory->MaxCarryWeight = DesiredCarryWeight;
	}

	if (UWildBoundRadiationComponent* Radiation = RadiationComponent.Get())
	{
		if (BaseRadiationDosePerSecond < 0.0f)
		{
			BaseRadiationDosePerSecond = Radiation->FullExposureDosePerSecond;
		}

		Radiation->FullExposureDosePerSecond = BaseRadiationDosePerSecond
			* (HasFilterMask() ? FilterMaskDoseMultiplier : 1.0f);
	}
}

void UWildBoundGearComponent::RestoreBaseValues()
{
	if (UWildBoundInventoryComponent* Inventory = InventoryComponent.Get())
	{
		if (BaseCarryWeight >= 0.0f)
		{
			Inventory->MaxCarryWeight = BaseCarryWeight;
		}
	}

	if (UWildBoundRadiationComponent* Radiation = RadiationComponent.Get())
	{
		if (BaseRadiationDosePerSecond >= 0.0f)
		{
			Radiation->FullExposureDosePerSecond = BaseRadiationDosePerSecond;
		}
	}
}
