#include "SWildBoundHUDWidget.h"

#include "../Inventory/WildBoundInventoryComponent.h"
#include "../Survival/WildBoundRadiationComponent.h"
#include "../Survival/WildBoundSurvivalComponent.h"
#include "GameFramework/Actor.h"
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
		.WidthOverride(300.0f)
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
				+ SVerticalBox::Slot()
				.AutoHeight()
				.Padding(0.0f, 9.0f, 0.0f, 4.0f)
				[
					SNew(SSeparator)
				]
				+ SVerticalBox::Slot()
				.AutoHeight()
				.Padding(0.0f, 1.0f, 0.0f, 3.0f)
				[
					SNew(STextBlock)
					.Text(this, &SWildBoundHUDWidget::GetRadiationText)
					.Font(FCoreStyle::GetDefaultFontStyle("Bold", 9))
					.ColorAndOpacity(this, &SWildBoundHUDWidget::GetRadiationColor)
				]
				+ SVerticalBox::Slot()
				.AutoHeight()
				.Padding(0.0f, 0.0f, 0.0f, 2.0f)
				[
					SNew(SProgressBar)
					.Percent(this, &SWildBoundHUDWidget::GetRadiationDosePercent)
					.FillColorAndOpacity(FLinearColor(0.73f, 0.49f, 0.10f, 1.0f))
				]
				+ SVerticalBox::Slot()
				.AutoHeight()
				.Padding(0.0f, 8.0f, 0.0f, 7.0f)
				[
					SNew(SSeparator)
				]
				+ SVerticalBox::Slot()
				.AutoHeight()
				.Padding(0.0f, 0.0f, 0.0f, 5.0f)
				[
					SNew(STextBlock)
					.Text(FText::FromString(TEXT("SCAVENGED SUPPLIES")))
					.Font(FCoreStyle::GetDefaultFontStyle("Bold", 10))
					.ColorAndOpacity(FLinearColor(0.72f, 0.75f, 0.68f, 1.0f))
				]
				+ SVerticalBox::Slot()
				.AutoHeight()
				[
					SNew(STextBlock)
					.Text(this, &SWildBoundHUDWidget::GetInventoryText)
					.Font(FCoreStyle::GetDefaultFontStyle("Regular", 10))
					.ColorAndOpacity(FLinearColor(0.88f, 0.90f, 0.84f, 1.0f))
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

	if (Exposure >= 70)
	{
		ClickPattern = TEXT("|||||| DANGER");
	}
	else if (Exposure >= 40)
	{
		ClickPattern = TEXT("CLICK CLICK CLICK");
	}
	else if (Exposure >= 15)
	{
		ClickPattern = TEXT("click...click");
	}
	else if (Exposure >= 2)
	{
		ClickPattern = TEXT("click");
	}

	return FText::FromString(FString::Printf(
		TEXT("GEIGER  %s   EXP %d%%  DOSE %d"),
		ClickPattern,
		Exposure,
		Dose));
}

FSlateColor SWildBoundHUDWidget::GetRadiationColor() const
{
	const UWildBoundSurvivalComponent* Survival = SurvivalComponent.Get();
	const AActor* Owner = Survival ? Survival->GetOwner() : nullptr;
	const UWildBoundRadiationComponent* Radiation = Owner ? Owner->FindComponentByClass<UWildBoundRadiationComponent>() : nullptr;
	const float Exposure = Radiation ? Radiation->CurrentExposure : 0.0f;

	if (Exposure >= 70.0f)
	{
		return FSlateColor(FLinearColor(0.92f, 0.18f, 0.10f, 1.0f));
	}
	if (Exposure >= 40.0f)
	{
		return FSlateColor(FLinearColor(0.95f, 0.48f, 0.08f, 1.0f));
	}
	if (Exposure >= 10.0f)
	{
		return FSlateColor(FLinearColor(0.82f, 0.69f, 0.22f, 1.0f));
	}

	return FSlateColor(FLinearColor(0.55f, 0.58f, 0.54f, 1.0f));
}

FText SWildBoundHUDWidget::GetInventoryText() const
{
	const UWildBoundSurvivalComponent* Survival = SurvivalComponent.Get();
	const AActor* Owner = Survival ? Survival->GetOwner() : nullptr;
	const UWildBoundInventoryComponent* Inventory = Owner ? Owner->FindComponentByClass<UWildBoundInventoryComponent>() : nullptr;

	const int32 Water = Inventory ? Inventory->GetItemCount(TEXT("Water")) : 0;
	const int32 Food = Inventory ? Inventory->GetItemCount(TEXT("Food")) : 0;
	const int32 Medical = Inventory ? Inventory->GetItemCount(TEXT("MedicalSupplies")) : 0;

	return FText::FromString(FString::Printf(
		TEXT("[1] WATER        x%d\n[2] FOOD         x%d\n[3] MED KIT      x%d"),
		Water,
		Food,
		Medical));
}
