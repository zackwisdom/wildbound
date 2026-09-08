#pragma once

#include "CoreMinimal.h"
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

	FText GetHealthText() const;
	FText GetHungerText() const;
	FText GetThirstText() const;
	FText GetStaminaText() const;
};
