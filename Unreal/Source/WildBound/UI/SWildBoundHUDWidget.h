#pragma once

#include "CoreMinimal.h"
#include "Styling/SlateColor.h"
#include "Widgets/SCompoundWidget.h"

class UWildBoundSurvivalComponent;

class SWildBoundHUDWidget : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SWildBoundHUDWidget) {}
		SLATE_ARGUMENT(TWeakObjectPtr<UWildBoundSurvivalComponent>, SurvivalComponent)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);
	void SetSurvivalComponent(UWildBoundSurvivalComponent* InSurvivalComponent);

private:
	TWeakObjectPtr<UWildBoundSurvivalComponent> SurvivalComponent;

	TOptional<float> GetHealthPercent() const;
	TOptional<float> GetHungerPercent() const;
	TOptional<float> GetThirstPercent() const;
	TOptional<float> GetStaminaPercent() const;
	TOptional<float> GetRadiationDosePercent() const;

	FText GetHealthText() const;
	FText GetHungerText() const;
	FText GetThirstText() const;
	FText GetStaminaText() const;
	FText GetRadiationText() const;
	FSlateColor GetRadiationColor() const;

	EVisibility GetSurvivalWarningVisibility() const;
	FText GetSurvivalWarningText() const;
	FSlateColor GetSurvivalWarningColor() const;
	FSlateColor GetSurvivalWarningBackground() const;

	int32 GetSelectedHotbarSlot() const;
	FSlateColor GetHotbarSlotOneBackground() const;
	FSlateColor GetHotbarSlotTwoBackground() const;
	FSlateColor GetHotbarSlotThreeBackground() const;
	FText GetHotbarSlotName(int32 SlotIndex) const;
	FText GetHotbarSlotCount(int32 SlotIndex) const;
};
