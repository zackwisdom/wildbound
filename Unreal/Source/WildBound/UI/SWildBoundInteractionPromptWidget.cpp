#include "SWildBoundInteractionPromptWidget.h"

#include "../Crafting/WildBoundCraftingComponent.h"
#include "../Player/WildBoundInteractionComponent.h"
#include "../Survival/WildBoundSurvivalComponent.h"
#include "GameFramework/Actor.h"
#include "Styling/CoreStyle.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Text/STextBlock.h"

void SWildBoundInteractionPromptWidget::Construct(const FArguments& InArgs)
{
	SurvivalComponent = InArgs._SurvivalComponent;

	ChildSlot
	[
		SNew(SBox)
		.WidthOverride(460.0f)
		[
			SNew(SBorder)
			.Visibility(this, &SWildBoundInteractionPromptWidget::GetPromptVisibility)
			.Padding(FMargin(14.0f, 8.0f))
			.BorderBackgroundColor(FLinearColor(0.012f, 0.016f, 0.015f, 0.78f))
			[
				SNew(STextBlock)
				.Text(this, &SWildBoundInteractionPromptWidget::GetPromptText)
				.Justification(ETextJustify::Center)
				.Font(FCoreStyle::GetDefaultFontStyle("Bold", 11))
				.ColorAndOpacity(FLinearColor(0.95f, 0.95f, 0.90f, 1.0f))
				.ShadowOffset(FVector2D(1.0f, 1.0f))
			]
		]
	];
}

FText SWildBoundInteractionPromptWidget::GetPromptText() const
{
	const UWildBoundSurvivalComponent* Survival = SurvivalComponent.Get();
	const AActor* Owner = Survival ? Survival->GetOwner() : nullptr;
	if (!Owner)
	{
		return FText::GetEmpty();
	}

	const UWildBoundInteractionComponent* Interaction = Owner->FindComponentByClass<UWildBoundInteractionComponent>();
	if (Interaction)
	{
		const FString InteractionPrompt = Interaction->GetContextPrompt();
		if (!InteractionPrompt.IsEmpty())
		{
			return FText::FromString(InteractionPrompt);
		}
	}

	const UWildBoundCraftingComponent* Crafting = Owner->FindComponentByClass<UWildBoundCraftingComponent>();
	if (Crafting && !Crafting->IsCraftingOpen() && Crafting->IsNearWorkbench())
	{
		return FText::FromString(TEXT("[C] USE WORKBENCH"));
	}

	return FText::GetEmpty();
}

EVisibility SWildBoundInteractionPromptWidget::GetPromptVisibility() const
{
	return GetPromptText().IsEmpty() ? EVisibility::Collapsed : EVisibility::HitTestInvisible;
}
