#include "SWildBoundHUDWidget.h"

#include "../Inventory/WildBoundInventoryComponent.h"
#include "../Player/WildBoundInteractionComponent.h"
#include "../Survival/WildBoundRadiationComponent.h"
#include "../Survival/WildBoundSurvivalComponent.h"
#include "GameFramework/Actor.h"
#include "Styling/CoreStyle.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SSeparator.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/SOverlay.h"
#include "Widgets/Notifications/SProgressBar.h"
#include "Widgets/Text/STextBlock.h"

void SWildBoundHUDWidget::Construct(const FArguments& InArgs)
{
	SurvivalComponent = InArgs._SurvivalComponent;

	ChildSlot
	[
		SNew(SOverlay)

		+ SOverlay::Slot()
		.HAlign(HAlign_Left)
		.VAlign(VAlign_Bottom)
		.Padding(FMargin(28.0f, 0.0f, 0.0f, 30.0f))
		[
			SNew(SBox)
			.WidthOverride(300.0f)
			[
				SNew(SBorder)
				.Padding(FMargin(14.0f, 12.0f))
				.BorderBackgroundColor(FLinearColor(0.015f, 0.02f, 0.018f, 0.82f))
				[
					SNew(SVerticalBox)
					+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 0.0f, 0.0f, 6.0f)
					[
						SNew(STextBlock)
						.Text(FText::FromString(TEXT("WILDBOUND")))
						.Font(FCoreStyle::GetDefaultFontStyle("Bold", 13))
					]
					+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 0.0f, 0.0f, 8.0f)
					[
						SNew(SSeparator)
					]
					+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 2.0f)
					[
						SNew(SVerticalBox)
						+ SVerticalBox::Slot().AutoHeight()
						[
							SNew(STextBlock).Text(this, &SWildBoundHUDWidget::GetHealthText)
						]
						+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 2.0f, 0.0f, 4.0f)
						[
							SNew(SProgressBar).Percent(this, &SWildBoundHUDWidget::GetHealthPercent).FillColorAndOpacity(FLinearColor(0.72f, 0.12f, 0.10f, 1.0f))
						]
					]
					+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 2.0f)
					[
						SNew(SVerticalBox)
						+ SVerticalBox::Slot().AutoHeight()
						[
							SNew(STextBlock).Text(this, &SWildBoundHUDWidget::GetHungerText)
						]
						+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 2.0f, 0.0f, 4.0f)
						[
							SNew(SProgressBar).Percent(this, &SWildBoundHUDWidget::GetHungerPercent).FillColorAndOpacity(FLinearColor(0.72f, 0.48f, 0.12f, 1.0f))
						]
					]
					+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 2.0f)
					[
						SNew(SVerticalBox)
						+ SVerticalBox::Slot().AutoHeight()
						[
							SNew(STextBlock).Text(this, &SWildBoundHUDWidget::GetThirstText)
						]
						+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 2.0f, 0.0f, 4.0f)
						[
							SNew(SProgressBar).Percent(this, &SWildBoundHUDWidget::GetThirstPercent).FillColorAndOpacity(FLinearColor(0.10f, 0.42f, 0.72f, 1.0f))
						]
					]
					+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 2.0f)
					[
						SNew(SVerticalBox)
						+ SVerticalBox::Slot().AutoHeight()
						[
							SNew(STextBlock).Text(this, &SWildBoundHUDWidget::GetStaminaText)
						]
						+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 2.0f, 0.0f, 0.0f)
						[
							SNew(SProgressBar).Percent(this, &SWildBoundHUDWidget::GetStaminaPercent).FillColorAndOpacity(FLinearColor(0.22f, 0.68f, 0.30f, 1.0f))
						]
					]
					+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 9.0f, 0.0f, 4.0f)
					[
						SNew(SSeparator)
					]
					+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 1.0f, 0.0f, 3.0f)
					[
						SNew(STextBlock)
						.Text(this, &SWildBoundHUDWidget::GetRadiationText)
						.Font(FCoreStyle::GetDefaultFontStyle("Bold", 9))
						.ColorAndOpacity(this, &SWildBoundHUDWidget::GetRadiationColor)
					]
					+ SVerticalBox::Slot().AutoHeight()
					[
						SNew(SProgressBar).Percent(this, &SWildBoundHUDWidget::GetRadiationDosePercent).FillColorAndOpacity(FLinearColor(0.73f, 0.49f, 0.10f, 1.0f))
					]
				]
			]
		]

		+ SOverlay::Slot()
		.HAlign(HAlign_Center)
		.VAlign(VAlign_Bottom)
		.Padding(FMargin(0.0f, 0.0f, 0.0f, 142.0f))
		[
			SNew(SBorder)
			.Visibility(this, &SWildBoundHUDWidget::GetSurvivalWarningVisibility)
			.Padding(FMargin(16.0f, 9.0f))
			.BorderBackgroundColor(this, &SWildBoundHUDWidget::GetSurvivalWarningBackground)
			[
				SNew(STextBlock)
				.Text(this, &SWildBoundHUDWidget::GetSurvivalWarningText)
				.Font(FCoreStyle::GetDefaultFontStyle("Bold", 10))
				.ColorAndOpacity(this, &SWildBoundHUDWidget::GetSurvivalWarningColor)
				.Justification(ETextJustify::Center)
			]
		]

		+ SOverlay::Slot()
		.HAlign(HAlign_Right)
		.VAlign(VAlign_Bottom)
		.Padding(FMargin(0.0f, 0.0f, 28.0f, 30.0f))
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Right).Padding(0.0f, 0.0f, 0.0f, 5.0f)
			[
				SNew(STextBlock)
				.Text(FText::FromString(TEXT("SURVIVAL HOTBAR")))
				.Font(FCoreStyle::GetDefaultFontStyle("Bold", 9))
				.ColorAndOpacity(FLinearColor(0.72f, 0.75f, 0.68f, 0.95f))
			]
			+ SVerticalBox::Slot().AutoHeight()
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().AutoWidth().Padding(0.0f, 0.0f, 6.0f, 0.0f)
				[
					SNew(SBox).WidthOverride(118.0f).HeightOverride(92.0f)
					[
						SNew(SBorder)
						.Padding(FMargin(9.0f, 7.0f))
						.BorderBackgroundColor(this, &SWildBoundHUDWidget::GetHotbarSlotOneBackground)
						[
							SNew(SVerticalBox)
							+ SVerticalBox::Slot().AutoHeight()
							[
								SNew(STextBlock).Text(FText::FromString(TEXT("1"))).Font(FCoreStyle::GetDefaultFontStyle("Bold", 10))
							]
							+ SVerticalBox::Slot().FillHeight(1.0f).VAlign(VAlign_Center)
							[
								SNew(STextBlock).Text_Lambda([this]() { return GetHotbarSlotName(0); }).Font(FCoreStyle::GetDefaultFontStyle("Bold", 10)).AutoWrapText(true)
							]
							+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Right)
							[
								SNew(STextBlock).Text_Lambda([this]() { return GetHotbarSlotCount(0); }).Font(FCoreStyle::GetDefaultFontStyle("Bold", 11))
							]
						]
					]
				]
				+ SHorizontalBox::Slot().AutoWidth().Padding(0.0f, 0.0f, 6.0f, 0.0f)
				[
					SNew(SBox).WidthOverride(118.0f).HeightOverride(92.0f)
					[
						SNew(SBorder)
						.Padding(FMargin(9.0f, 7.0f))
						.BorderBackgroundColor(this, &SWildBoundHUDWidget::GetHotbarSlotTwoBackground)
						[
							SNew(SVerticalBox)
							+ SVerticalBox::Slot().AutoHeight()
							[
								SNew(STextBlock).Text(FText::FromString(TEXT("2"))).Font(FCoreStyle::GetDefaultFontStyle("Bold", 10))
							]
							+ SVerticalBox::Slot().FillHeight(1.0f).VAlign(VAlign_Center)
							[
								SNew(STextBlock).Text_Lambda([this]() { return GetHotbarSlotName(1); }).Font(FCoreStyle::GetDefaultFontStyle("Bold", 10)).AutoWrapText(true)
							]
							+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Right)
							[
								SNew(STextBlock).Text_Lambda([this]() { return GetHotbarSlotCount(1); }).Font(FCoreStyle::GetDefaultFontStyle("Bold", 11))
							]
						]
					]
				]
				+ SHorizontalBox::Slot().AutoWidth()
				[
					SNew(SBox).WidthOverride(118.0f).HeightOverride(92.0f)
					[
						SNew(SBorder)
						.Padding(FMargin(9.0f, 7.0f))
						.BorderBackgroundColor(this, &SWildBoundHUDWidget::GetHotbarSlotThreeBackground)
						[
							SNew(SVerticalBox)
							+ SVerticalBox::Slot().AutoHeight()
							[
								SNew(STextBlock).Text(FText::FromString(TEXT("3"))).Font(FCoreStyle::GetDefaultFontStyle("Bold", 10))
							]
							+ SVerticalBox::Slot().FillHeight(1.0f).VAlign(VAlign_Center)
							[
								SNew(STextBlock).Text_Lambda([this]() { return GetHotbarSlotName(2); }).Font(FCoreStyle::GetDefaultFontStyle("Bold", 10)).AutoWrapText(true)
							]
							+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Right)
							[
								SNew(STextBlock).Text_Lambda([this]() { return GetHotbarSlotCount(2); }).Font(FCoreStyle::GetDefaultFontStyle("Bold", 11))
							]
						]
					]
				]
			]
			+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Right).Padding(0.0f, 5.0f, 0.0f, 0.0f)
			[
				SNew(STextBlock)
				.Text(FText::FromString(TEXT("1-3 / MOUSE WHEEL SELECT    E USE    TAB MANAGE")))
				.Font(FCoreStyle::GetDefaultFontStyle("Regular", 8))
				.ColorAndOpacity(FLinearColor(0.60f, 0.62f, 0.58f, 0.92f))
			]
		]
	];
}

void SWildBoundHUDWidget::SetSurvivalComponent(UWildBoundSurvivalComponent* InSurvivalComponent)
{
	SurvivalComponent = InSurvivalComponent;
}

TOptional<float> SWildBoundHUDWidget::GetHealthPercent() const
{
	return SurvivalComponent.IsValid() ? TOptional<float>(SurvivalComponent->GetHealthPercent()) : TOptional<float>(0.0f);
}

TOptional<float> SWildBoundHUDWidget::GetHungerPercent() const
{
	return SurvivalComponent.IsValid() ? TOptional<float>(SurvivalComponent->GetHungerPercent()) : TOptional<float>(0.0f);
}

TOptional<float> SWildBoundHUDWidget::GetThirstPercent() const
{
	return SurvivalComponent.IsValid() ? TOptional<float>(SurvivalComponent->GetThirstPercent()) : TOptional<float>(0.0f);
}

TOptional<float> SWildBoundHUDWidget::GetStaminaPercent() const
{
	return SurvivalComponent.IsValid() ? TOptional<float>(SurvivalComponent->GetStaminaPercent()) : TOptional<float>(0.0f);
}

TOptional<float> SWildBoundHUDWidget::GetRadiationDosePercent() const
{
	const UWildBoundSurvivalComponent* Survival = SurvivalComponent.Get();
	const AActor* Owner = Survival ? Survival->GetOwner() : nullptr;
	const UWildBoundRadiationComponent* Radiation = Owner ? Owner->FindComponentByClass<UWildBoundRadiationComponent>() : nullptr;
	return Radiation ? TOptional<float>(Radiation->GetDosePercent()) : TOptional<float>(0.0f);
}

FText SWildBoundHUDWidget::GetHealthText() const
{
	const UWildBoundSurvivalComponent* C = SurvivalComponent.Get();
	return FText::FromString(FString::Printf(TEXT("HEALTH   %d"), C ? FMath::RoundToInt(C->Health) : 0));
}

FText SWildBoundHUDWidget::GetHungerText() const
{
	const UWildBoundSurvivalComponent* C = SurvivalComponent.Get();
	return FText::FromString(FString::Printf(TEXT("HUNGER   %d"), C ? FMath::RoundToInt(C->Hunger) : 0));
}

FText SWildBoundHUDWidget::GetThirstText() const
{
	const UWildBoundSurvivalComponent* C = SurvivalComponent.Get();
	return FText::FromString(FString::Printf(TEXT("THIRST   %d"), C ? FMath::RoundToInt(C->Thirst) : 0));
}

FText SWildBoundHUDWidget::GetStaminaText() const
{
	const UWildBoundSurvivalComponent* C = SurvivalComponent.Get();
	return FText::FromString(FString::Printf(TEXT("STAMINA  %d"), C ? FMath::RoundToInt(C->Stamina) : 0));
}

FText SWildBoundHUDWidget::GetRadiationText() const
{
	const UWildBoundSurvivalComponent* Survival = SurvivalComponent.Get();
	const AActor* Owner = Survival ? Survival->GetOwner() : nullptr;
	const UWildBoundRadiationComponent* Radiation = Owner ? Owner->FindComponentByClass<UWildBoundRadiationComponent>() : nullptr;

	if (!Radiation)
	{
		return FText::FromString(TEXT("GEIGER  ----"));
	}

	const int32 Exposure = FMath::RoundToInt(Radiation->CurrentExposure);
	const int32 Dose = FMath::RoundToInt(Radiation->AccumulatedDose);
	const TCHAR* ClickPattern = TEXT("quiet");
	if (Exposure >= 70) ClickPattern = TEXT("|||||| DANGER");
	else if (Exposure >= 40) ClickPattern = TEXT("CLICK CLICK CLICK");
	else if (Exposure >= 15) ClickPattern = TEXT("click...click");
	else if (Exposure >= 2) ClickPattern = TEXT("click");

	return FText::FromString(FString::Printf(TEXT("GEIGER  %s   EXP %d%%  DOSE %d"), ClickPattern, Exposure, Dose));
}

FSlateColor SWildBoundHUDWidget::GetRadiationColor() const
{
	const UWildBoundSurvivalComponent* Survival = SurvivalComponent.Get();
	const AActor* Owner = Survival ? Survival->GetOwner() : nullptr;
	const UWildBoundRadiationComponent* Radiation = Owner ? Owner->FindComponentByClass<UWildBoundRadiationComponent>() : nullptr;
	const float Exposure = Radiation ? Radiation->CurrentExposure : 0.0f;

	if (Exposure >= 70.0f) return FSlateColor(FLinearColor(0.92f, 0.18f, 0.10f, 1.0f));
	if (Exposure >= 40.0f) return FSlateColor(FLinearColor(0.95f, 0.48f, 0.08f, 1.0f));
	if (Exposure >= 10.0f) return FSlateColor(FLinearColor(0.82f, 0.69f, 0.22f, 1.0f));
	return FSlateColor(FLinearColor(0.55f, 0.58f, 0.54f, 1.0f));
}

EVisibility SWildBoundHUDWidget::GetSurvivalWarningVisibility() const
{
	const UWildBoundSurvivalComponent* Survival = SurvivalComponent.Get();
	return Survival && Survival->IsNutritionLow() ? EVisibility::HitTestInvisible : EVisibility::Collapsed;
}

FText SWildBoundHUDWidget::GetSurvivalWarningText() const
{
	const UWildBoundSurvivalComponent* Survival = SurvivalComponent.Get();
	if (!Survival)
	{
		return FText::GetEmpty();
	}

	const float Hunger = Survival->GetHungerPercent();
	const float Thirst = Survival->GetThirstPercent();
	const bool bHungerCritical = Hunger <= Survival->CriticalNutritionThreshold;
	const bool bThirstCritical = Thirst <= Survival->CriticalNutritionThreshold;
	const bool bHungerDamaging = Hunger <= Survival->HealthDamageThreshold;
	const bool bThirstDamaging = Thirst <= Survival->HealthDamageThreshold;

	if (bHungerDamaging && bThirstDamaging)
	{
		return FText::FromString(TEXT("STARVING + SEVERELY DEHYDRATED   |   HEALTH FAILING"));
	}
	if (bThirstDamaging)
	{
		return FText::FromString(TEXT("SEVERE DEHYDRATION   |   HEALTH FAILING"));
	}
	if (bHungerDamaging)
	{
		return FText::FromString(TEXT("STARVING   |   HEALTH FAILING"));
	}
	if (bHungerCritical && bThirstCritical)
	{
		return FText::FromString(TEXT("CRITICAL HUNGER + THIRST   |   MOVEMENT / STAMINA COMPROMISED"));
	}
	if (bThirstCritical)
	{
		return FText::FromString(TEXT("CRITICALLY THIRSTY   |   STAMINA DRAIN INCREASED"));
	}
	if (bHungerCritical)
	{
		return FText::FromString(TEXT("CRITICALLY HUNGRY   |   STAMINA RECOVERY REDUCED"));
	}
	if (Hunger <= Survival->LowNutritionThreshold && Thirst <= Survival->LowNutritionThreshold)
	{
		return FText::FromString(TEXT("HUNGRY + THIRSTY   |   PERFORMANCE DECLINING"));
	}
	if (Thirst <= Survival->LowNutritionThreshold)
	{
		return FText::FromString(TEXT("THIRSTY   |   STAMINA RECOVERY SLOWING"));
	}
	return FText::FromString(TEXT("HUNGRY   |   STAMINA RECOVERY SLOWING"));
}

FSlateColor SWildBoundHUDWidget::GetSurvivalWarningColor() const
{
	const UWildBoundSurvivalComponent* Survival = SurvivalComponent.Get();
	if (!Survival)
	{
		return FSlateColor(FLinearColor::White);
	}

	const float LowestVital = FMath::Min(Survival->GetHungerPercent(), Survival->GetThirstPercent());
	if (LowestVital <= Survival->HealthDamageThreshold)
	{
		return FSlateColor(FLinearColor(1.0f, 0.55f, 0.48f, 1.0f));
	}
	if (LowestVital <= Survival->CriticalNutritionThreshold)
	{
		return FSlateColor(FLinearColor(1.0f, 0.72f, 0.34f, 1.0f));
	}
	return FSlateColor(FLinearColor(0.95f, 0.82f, 0.45f, 1.0f));
}

FSlateColor SWildBoundHUDWidget::GetSurvivalWarningBackground() const
{
	const UWildBoundSurvivalComponent* Survival = SurvivalComponent.Get();
	if (!Survival)
	{
		return FSlateColor(FLinearColor(0.05f, 0.05f, 0.04f, 0.90f));
	}

	const float LowestVital = FMath::Min(Survival->GetHungerPercent(), Survival->GetThirstPercent());
	if (LowestVital <= Survival->HealthDamageThreshold)
	{
		return FSlateColor(FLinearColor(0.28f, 0.035f, 0.025f, 0.94f));
	}
	if (LowestVital <= Survival->CriticalNutritionThreshold)
	{
		return FSlateColor(FLinearColor(0.23f, 0.10f, 0.025f, 0.93f));
	}
	return FSlateColor(FLinearColor(0.16f, 0.12f, 0.025f, 0.91f));
}

int32 SWildBoundHUDWidget::GetSelectedHotbarSlot() const
{
	const UWildBoundSurvivalComponent* Survival = SurvivalComponent.Get();
	const AActor* Owner = Survival ? Survival->GetOwner() : nullptr;
	const UWildBoundInteractionComponent* Interaction = Owner ? Owner->FindComponentByClass<UWildBoundInteractionComponent>() : nullptr;
	return Interaction ? Interaction->GetSelectedHotbarSlot() : 0;
}

FSlateColor SWildBoundHUDWidget::GetHotbarSlotOneBackground() const
{
	return FSlateColor(GetSelectedHotbarSlot() == 0 ? FLinearColor(0.075f, 0.20f, 0.28f, 0.96f) : FLinearColor(0.022f, 0.028f, 0.028f, 0.86f));
}

FSlateColor SWildBoundHUDWidget::GetHotbarSlotTwoBackground() const
{
	return FSlateColor(GetSelectedHotbarSlot() == 1 ? FLinearColor(0.25f, 0.205f, 0.075f, 0.96f) : FLinearColor(0.022f, 0.028f, 0.028f, 0.86f));
}

FSlateColor SWildBoundHUDWidget::GetHotbarSlotThreeBackground() const
{
	return FSlateColor(GetSelectedHotbarSlot() == 2 ? FLinearColor(0.30f, 0.075f, 0.06f, 0.96f) : FLinearColor(0.022f, 0.028f, 0.028f, 0.86f));
}

FText SWildBoundHUDWidget::GetHotbarSlotName(int32 SlotIndex) const
{
	const UWildBoundSurvivalComponent* Survival = SurvivalComponent.Get();
	const AActor* Owner = Survival ? Survival->GetOwner() : nullptr;
	const UWildBoundInventoryComponent* Inventory = Owner ? Owner->FindComponentByClass<UWildBoundInventoryComponent>() : nullptr;
	if (!Inventory)
	{
		return FText::FromString(TEXT("EMPTY"));
	}

	const FName ItemId = Inventory->GetHotbarItemId(SlotIndex);
	if (ItemId.IsNone())
	{
		return FText::FromString(TEXT("EMPTY"));
	}

	return FText::FromString(Inventory->GetItemDisplayName(ItemId).ToUpper());
}

FText SWildBoundHUDWidget::GetHotbarSlotCount(int32 SlotIndex) const
{
	const UWildBoundSurvivalComponent* Survival = SurvivalComponent.Get();
	const AActor* Owner = Survival ? Survival->GetOwner() : nullptr;
	const UWildBoundInventoryComponent* Inventory = Owner ? Owner->FindComponentByClass<UWildBoundInventoryComponent>() : nullptr;
	if (!Inventory)
	{
		return FText::FromString(TEXT("x0"));
	}

	const FName ItemId = Inventory->GetHotbarItemId(SlotIndex);
	const int32 Count = ItemId.IsNone() ? 0 : Inventory->GetItemCount(ItemId);
	return FText::FromString(FString::Printf(TEXT("x%d"), Count));
}
