#include "SWildBoundHUDWidget.h"

#include "../Survival/WildBoundSurvivalComponent.h"
#include "Styling/CoreStyle.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SSeparator.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Notifications/SProgressBar.h"
#include "Widgets/Text/STextBlock.h"

void SWildBoundHUDWidget::Construct(const FArguments& InArgs)
{
	SurvivalComponent = InArgs._SurvivalComponent;

	ChildSlot
	[
		SNew(SBox)
		.WidthOverride(280.0f)
		[
			SNew(SBorder)
			.Padding(FMargin(14.0f, 12.0f))
			.BorderBackgroundColor(FLinearColor(0.015f, 0.02f, 0.018f, 0.82f))
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot()
				.AutoHeight()
				.Padding(0.0f, 0.0f, 0.0f, 6.0f)
				[
					SNew(STextBlock)
					.Text(FText::FromString(TEXT("WILDBOUND")))
					.Font(FCoreStyle::GetDefaultFontStyle("Bold", 13))
				]
				+ SVerticalBox::Slot()
				.AutoHeight()
				.Padding(0.0f, 0.0f, 0.0f, 8.0f)
				[
					SNew(SSeparator)
				]
				+ SVerticalBox::Slot()
				.AutoHeight()
				.Padding(0.0f, 2.0f)
				[
					SNew(SVerticalBox)
					+ SVerticalBox::Slot().AutoHeight()
					[
						SNew(STextBlock).Text(this, &SWildBoundHUDWidget::GetHealthText)
					]
					+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 2.0f, 0.0f, 4.0f)
					[
						SNew(SProgressBar)
						.Percent(this, &SWildBoundHUDWidget::GetHealthPercent)
						.FillColorAndOpacity(FLinearColor(0.72f, 0.12f, 0.10f, 1.0f))
					]
				]
				+ SVerticalBox::Slot()
				.AutoHeight()
				.Padding(0.0f, 2.0f)
				[
					SNew(SVerticalBox)
					+ SVerticalBox::Slot().AutoHeight()
					[
						SNew(STextBlock).Text(this, &SWildBoundHUDWidget::GetHungerText)
					]
					+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 2.0f, 0.0f, 4.0f)
					[
						SNew(SProgressBar)
						.Percent(this, &SWildBoundHUDWidget::GetHungerPercent)
						.FillColorAndOpacity(FLinearColor(0.72f, 0.48f, 0.12f, 1.0f))
					]
				]
				+ SVerticalBox::Slot()
				.AutoHeight()
				.Padding(0.0f, 2.0f)
				[
					SNew(SVerticalBox)
					+ SVerticalBox::Slot().AutoHeight()
					[
						SNew(STextBlock).Text(this, &SWildBoundHUDWidget::GetThirstText)
					]
					+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 2.0f, 0.0f, 4.0f)
					[
						SNew(SProgressBar)
						.Percent(this, &SWildBoundHUDWidget::GetThirstPercent)
						.FillColorAndOpacity(FLinearColor(0.10f, 0.42f, 0.72f, 1.0f))
					]
				]
				+ SVerticalBox::Slot()
				.AutoHeight()
				.Padding(0.0f, 2.0f)
				[
					SNew(SVerticalBox)
					+ SVerticalBox::Slot().AutoHeight()
					[
						SNew(STextBlock).Text(this, &SWildBoundHUDWidget::GetStaminaText)
					]
					+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 2.0f, 0.0f, 0.0f)
					[
						SNew(SProgressBar)
						.Percent(this, &SWildBoundHUDWidget::GetStaminaPercent)
						.FillColorAndOpacity(FLinearColor(0.22f, 0.68f, 0.30f, 1.0f))
					]
				]
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
