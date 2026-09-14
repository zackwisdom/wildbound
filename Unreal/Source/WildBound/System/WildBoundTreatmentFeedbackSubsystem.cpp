#include "WildBoundTreatmentFeedbackSubsystem.h"

#include "../Player/WildBoundInteractionComponent.h"
#include "Engine/Engine.h"
#include "Engine/GameViewportClient.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "Styling/CoreStyle.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Notifications/SProgressBar.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/SOverlay.h"
#include "Widgets/Text/STextBlock.h"

void UWildBoundTreatmentFeedbackSubsystem::OnWorldBeginPlay(UWorld& InWorld)
{
	Super::OnWorldBeginPlay(InWorld);

	if (!InWorld.IsGameWorld())
	{
		return;
	}

	EnsureTreatmentFeedback();
	InWorld.GetTimerManager().SetTimer(
		TreatmentFeedbackSetupTimer,
		this,
		&UWildBoundTreatmentFeedbackSubsystem::EnsureTreatmentFeedback,
		0.5f,
		true,
		0.10f);
}

void UWildBoundTreatmentFeedbackSubsystem::Deinitialize()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(TreatmentFeedbackSetupTimer);
	}

	RemoveTreatmentFeedback();
	Super::Deinitialize();
}

void UWildBoundTreatmentFeedbackSubsystem::EnsureTreatmentFeedback()
{
	if (TreatmentViewportRoot.IsValid())
	{
		return;
	}

	UWorld* World = GetWorld();
	if (!World || !World->IsGameWorld() || !GEngine || !GEngine->GameViewport)
	{
		return;
	}

	APlayerController* PlayerController = World->GetFirstPlayerController();
	APawn* Pawn = PlayerController ? PlayerController->GetPawn() : nullptr;
	UWildBoundInteractionComponent* Interaction = Pawn
		? Pawn->FindComponentByClass<UWildBoundInteractionComponent>()
		: nullptr;
	if (!Interaction)
	{
		return;
	}

	const TWeakObjectPtr<UWildBoundInteractionComponent> WeakInteraction = Interaction;

	TSharedPtr<SOverlay> Overlay;
	SAssignNew(Overlay, SOverlay)
	+ SOverlay::Slot()
	.HAlign(HAlign_Center)
	.VAlign(VAlign_Bottom)
	.Padding(FMargin(0.0f, 0.0f, 0.0f, 228.0f))
	[
		SNew(SBox)
		.WidthOverride(430.0f)
		.Visibility_Lambda([WeakInteraction]()
		{
			const UWildBoundInteractionComponent* Treatment = WeakInteraction.Get();
			return Treatment && Treatment->IsTreatmentInProgress()
				? EVisibility::HitTestInvisible
				: EVisibility::Collapsed;
		})
		[
			SNew(SBorder)
			.Padding(FMargin(15.0f, 11.0f))
			.BorderBackgroundColor(FLinearColor(0.018f, 0.026f, 0.022f, 0.96f))
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot().AutoHeight()
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot().FillWidth(1.0f)
					[
						SNew(STextBlock)
						.Text_Lambda([WeakInteraction]()
						{
							const UWildBoundInteractionComponent* Treatment = WeakInteraction.Get();
							return Treatment
								? FText::FromString(Treatment->GetTreatmentLabel())
								: FText::GetEmpty();
						})
						.Font(FCoreStyle::GetDefaultFontStyle("Bold", 10))
						.ColorAndOpacity(FLinearColor(0.84f, 0.90f, 0.80f, 1.0f))
					]
					+ SHorizontalBox::Slot().AutoWidth()
					[
						SNew(STextBlock)
						.Text_Lambda([WeakInteraction]()
						{
							const UWildBoundInteractionComponent* Treatment = WeakInteraction.Get();
							const int32 Percent = Treatment
								? FMath::RoundToInt(Treatment->GetTreatmentProgress() * 100.0f)
								: 0;
							return FText::FromString(FString::Printf(TEXT("%d%%"), Percent));
						})
						.Font(FCoreStyle::GetDefaultFontStyle("Bold", 10))
						.ColorAndOpacity(FLinearColor(0.95f, 0.79f, 0.42f, 1.0f))
					]
				]
				+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 7.0f, 0.0f, 6.0f)
				[
					SNew(SProgressBar)
					.Percent_Lambda([WeakInteraction]() -> TOptional<float>
					{
						const UWildBoundInteractionComponent* Treatment = WeakInteraction.Get();
						return TOptional<float>(Treatment ? Treatment->GetTreatmentProgress() : 0.0f);
					})
					.FillColorAndOpacity(FLinearColor(0.62f, 0.74f, 0.42f, 1.0f))
				]
				+ SVerticalBox::Slot().AutoHeight()
				[
					SNew(STextBlock)
					.Text(FText::FromString(TEXT("MOVEMENT LOCKED   |   ESC CANCEL   |   ITEM CONSUMED ON COMPLETION")))
					.Font(FCoreStyle::GetDefaultFontStyle("Regular", 7))
					.ColorAndOpacity(FLinearColor(0.55f, 0.61f, 0.55f, 1.0f))
					.Justification(ETextJustify::Center)
				]
			]
		]
	];

	TreatmentViewportRoot = Overlay;
	GEngine->GameViewport->AddViewportWidgetContent(TreatmentViewportRoot.ToSharedRef(), 136);
	World->GetTimerManager().ClearTimer(TreatmentFeedbackSetupTimer);
	UE_LOG(LogTemp, Log, TEXT("WildBound treatment feedback: timed medical progress HUD attached."));
}

void UWildBoundTreatmentFeedbackSubsystem::RemoveTreatmentFeedback()
{
	if (TreatmentViewportRoot.IsValid() && GEngine && GEngine->GameViewport)
	{
		GEngine->GameViewport->RemoveViewportWidgetContent(TreatmentViewportRoot.ToSharedRef());
	}
	TreatmentViewportRoot.Reset();
}
