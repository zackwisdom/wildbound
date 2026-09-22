#include "WildBoundDeathSubsystem.h"

#include "WildBoundSaveSubsystem.h"
#include "../Survival/WildBoundSurvivalComponent.h"
#include "Engine/Engine.h"
#include "Engine/GameViewportClient.h"
#include "Engine/World.h"
#include "Framework/Application/SlateApplication.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Styling/CoreStyle.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SSeparator.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/SOverlay.h"
#include "Widgets/Text/STextBlock.h"

namespace
{
	class SWildBoundDeathWidget : public SCompoundWidget
	{
	public:
		SLATE_BEGIN_ARGS(SWildBoundDeathWidget) {}
			SLATE_ARGUMENT(UWildBoundDeathSubsystem*, Owner)
		SLATE_END_ARGS()

		void Construct(const FArguments& InArgs)
		{
			OwnerSubsystem = InArgs._Owner;
			const TWeakObjectPtr<UWildBoundDeathSubsystem> WeakOwner = OwnerSubsystem;

			ChildSlot
			[
				SNew(SOverlay)

				+ SOverlay::Slot()
				.HAlign(HAlign_Fill)
				.VAlign(VAlign_Fill)
				[
					SNew(SBorder)
					.BorderBackgroundColor(FLinearColor(0.025f, 0.006f, 0.005f, 0.88f))
				]

				+ SOverlay::Slot()
				.HAlign(HAlign_Center)
				.VAlign(VAlign_Center)
				.Padding(FMargin(28.0f))
				[
					SNew(SBox)
					.WidthOverride(620.0f)
					[
						SNew(SBorder)
						.Padding(FMargin(30.0f, 26.0f))
						.BorderBackgroundColor(FLinearColor(0.015f, 0.012f, 0.011f, 0.995f))
						[
							SNew(SOverlay)

							+ SOverlay::Slot()
							[
								SNew(SVerticalBox)
								.Visibility_Lambda([WeakOwner]()
								{
									const UWildBoundDeathSubsystem* Owner = WeakOwner.Get();
									return Owner && !Owner->IsRestartConfirmationOpen()
										? EVisibility::Visible
										: EVisibility::Collapsed;
								})

								+ SVerticalBox::Slot().AutoHeight()
								[
									SNew(STextBlock)
									.Text(FText::FromString(TEXT("YOU DIED")))
									.Font(FCoreStyle::GetDefaultFontStyle("Bold", 30))
									.ColorAndOpacity(FLinearColor(0.92f, 0.38f, 0.30f, 1.0f))
								]

								+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 3.0f, 0.0f, 6.0f)
								[
									SNew(STextBlock)
									.Text(FText::FromString(TEXT("CAUSE OF DEATH")))
									.Font(FCoreStyle::GetDefaultFontStyle("Bold", 8))
									.ColorAndOpacity(FLinearColor(0.50f, 0.52f, 0.47f, 1.0f))
								]

								+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 0.0f, 0.0f, 14.0f)
								[
									SNew(STextBlock)
									.Text_Lambda([WeakOwner]()
									{
										const UWildBoundDeathSubsystem* Owner = WeakOwner.Get();
										return Owner ? Owner->GetDeathCauseText() : FText::GetEmpty();
									})
									.Font(FCoreStyle::GetDefaultFontStyle("Bold", 16))
									.ColorAndOpacity(FLinearColor(0.89f, 0.76f, 0.58f, 1.0f))
								]

								+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 0.0f, 0.0f, 16.0f)
								[
									SNew(SSeparator)
								]

								+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 4.0f)
								[
									SNew(SButton)
									.IsEnabled_Lambda([WeakOwner]()
									{
										const UWildBoundDeathSubsystem* Owner = WeakOwner.Get();
										return Owner && Owner->GetWorld()
											&& Owner->GetWorld()->GetSubsystem<UWildBoundSaveSubsystem>()
											&& Owner->GetWorld()->GetSubsystem<UWildBoundSaveSubsystem>()->HasSaveGame();
									})
									.ContentPadding(FMargin(14.0f, 11.0f))
									.OnClicked_Lambda([WeakOwner]()
									{
										if (UWildBoundDeathSubsystem* Owner = WeakOwner.Get()) Owner->LoadLastSave();
										return FReply::Handled();
									})
									[
										SNew(STextBlock)
										.Text(FText::FromString(TEXT("LOAD LAST SAVE")))
										.Font(FCoreStyle::GetDefaultFontStyle("Bold", 11))
									]
								]

								+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 4.0f)
								[
									SNew(SButton)
									.ContentPadding(FMargin(14.0f, 11.0f))
									.OnClicked_Lambda([WeakOwner]()
									{
										if (UWildBoundDeathSubsystem* Owner = WeakOwner.Get()) Owner->RequestRestartFresh();
										return FReply::Handled();
									})
									[
										SNew(STextBlock)
										.Text(FText::FromString(TEXT("RESTART FRESH")))
										.Font(FCoreStyle::GetDefaultFontStyle("Bold", 11))
									]
								]

								+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 4.0f)
								[
									SNew(SButton)
									.ContentPadding(FMargin(14.0f, 11.0f))
									.OnClicked_Lambda([WeakOwner]()
									{
										if (UWildBoundDeathSubsystem* Owner = WeakOwner.Get()) Owner->ReturnToTitle();
										return FReply::Handled();
									})
									[
										SNew(STextBlock)
										.Text(FText::FromString(TEXT("RETURN TO TITLE")))
										.Font(FCoreStyle::GetDefaultFontStyle("Bold", 11))
									]
								]

								+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 14.0f, 0.0f, 0.0f)
								[
									SNew(STextBlock)
									.Text(FText::FromString(TEXT("YOUR LAST LIVING AUTOSAVE HAS BEEN PRESERVED")))
									.Font(FCoreStyle::GetDefaultFontStyle("Regular", 8))
									.ColorAndOpacity(FLinearColor(0.55f, 0.58f, 0.52f, 1.0f))
									.Justification(ETextJustify::Center)
								]
							]

							+ SOverlay::Slot()
							[
								SNew(SVerticalBox)
								.Visibility_Lambda([WeakOwner]()
								{
									const UWildBoundDeathSubsystem* Owner = WeakOwner.Get();
									return Owner && Owner->IsRestartConfirmationOpen()
										? EVisibility::Visible
										: EVisibility::Collapsed;
								})

								+ SVerticalBox::Slot().AutoHeight()
								[
									SNew(STextBlock)
									.Text(FText::FromString(TEXT("RESTART FRESH?")))
									.Font(FCoreStyle::GetDefaultFontStyle("Bold", 21))
									.ColorAndOpacity(FLinearColor(0.95f, 0.66f, 0.35f, 1.0f))
								]

								+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 10.0f, 0.0f, 18.0f)
								[
									SNew(STextBlock)
									.Text(FText::FromString(TEXT("This will erase the existing autosave and begin a completely new run.")))
									.AutoWrapText(true)
									.Font(FCoreStyle::GetDefaultFontStyle("Regular", 10))
									.ColorAndOpacity(FLinearColor(0.77f, 0.76f, 0.69f, 1.0f))
								]

								+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 4.0f)
								[
									SNew(SButton)
									.OnClicked_Lambda([WeakOwner]()
									{
										if (UWildBoundDeathSubsystem* Owner = WeakOwner.Get()) Owner->ConfirmRestartFresh();
										return FReply::Handled();
									})
									[
										SNew(STextBlock)
										.Text(FText::FromString(TEXT("ERASE AUTOSAVE & RESTART")))
										.Font(FCoreStyle::GetDefaultFontStyle("Bold", 10))
									]
								]

								+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 4.0f)
								[
									SNew(SButton)
									.OnClicked_Lambda([WeakOwner]()
									{
										if (UWildBoundDeathSubsystem* Owner = WeakOwner.Get()) Owner->CancelRestartFresh();
										return FReply::Handled();
									})
									[
										SNew(STextBlock).Text(FText::FromString(TEXT("CANCEL")))
									]
								]
							]
						]
					]
				]
			];

			SetVisibility(EVisibility::Collapsed);
		}

		virtual bool SupportsKeyboardFocus() const override
		{
			return true;
		}

	private:
		TWeakObjectPtr<UWildBoundDeathSubsystem> OwnerSubsystem;
	};
}

void UWildBoundDeathSubsystem::OnWorldBeginPlay(UWorld& InWorld)
{
	Super::OnWorldBeginPlay(InWorld);

	if (!InWorld.IsGameWorld())
	{
		return;
	}

	EnsureDeathWidget();
	InWorld.GetTimerManager().SetTimer(
		DeathCheckTimer,
		this,
		&UWildBoundDeathSubsystem::CheckPlayerState,
		0.08f,
		true,
		0.30f);
}

void UWildBoundDeathSubsystem::Deinitialize()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(DeathCheckTimer);
	}

	if (bGameOverOpen)
	{
		ApplyDeathInputState(false);
	}

	RemoveDeathWidget();
	Super::Deinitialize();
}

void UWildBoundDeathSubsystem::CheckPlayerState()
{
	if (bGameOverOpen)
	{
		return;
	}

	UWorld* World = GetWorld();
	APlayerController* PlayerController = World ? World->GetFirstPlayerController() : nullptr;
	APawn* Pawn = PlayerController ? PlayerController->GetPawn() : nullptr;
	UWildBoundSurvivalComponent* Survival = Pawn
		? Pawn->FindComponentByClass<UWildBoundSurvivalComponent>()
		: nullptr;
	UWildBoundSaveSubsystem* SaveSubsystem = World
		? World->GetSubsystem<UWildBoundSaveSubsystem>()
		: nullptr;

	if (!Survival || !SaveSubsystem || !SaveSubsystem->HasStartedSession())
	{
		return;
	}

	SurvivalComponent = Survival;
	if (!Survival->IsAlive())
	{
		ShowGameOver(*Survival);
	}
}

void UWildBoundDeathSubsystem::ShowGameOver(UWildBoundSurvivalComponent& Survival)
{
	if (bGameOverOpen)
	{
		return;
	}

	DeathCause = Survival.GetDeathCauseText();
	bRestartConfirmationOpen = false;
	bGameOverOpen = true;

	if (UWorld* World = GetWorld())
	{
		if (UWildBoundSaveSubsystem* SaveSubsystem = World->GetSubsystem<UWildBoundSaveSubsystem>())
		{
			SaveSubsystem->NotifyPlayerDied();
		}
	}

	EnsureDeathWidget();
	if (DeathViewportRoot.IsValid())
	{
		DeathViewportRoot->SetVisibility(EVisibility::Visible);
	}

	ApplyDeathInputState(true);

	if (FSlateApplication::IsInitialized() && DeathViewportRoot.IsValid())
	{
		FSlateApplication::Get().SetKeyboardFocus(DeathViewportRoot, EFocusCause::SetDirectly);
	}
}

FText UWildBoundDeathSubsystem::GetDeathCauseText() const
{
	return FText::FromString(DeathCause);
}

void UWildBoundDeathSubsystem::LoadLastSave()
{
	UWorld* World = GetWorld();
	UWildBoundSaveSubsystem* SaveSubsystem = World
		? World->GetSubsystem<UWildBoundSaveSubsystem>()
		: nullptr;

	if (SaveSubsystem && SaveSubsystem->HasSaveGame())
	{
		SaveSubsystem->ReloadLastSave();
	}
}

void UWildBoundDeathSubsystem::RequestRestartFresh()
{
	if (bGameOverOpen)
	{
		bRestartConfirmationOpen = true;
	}
}

void UWildBoundDeathSubsystem::ConfirmRestartFresh()
{
	UWorld* World = GetWorld();
	UWildBoundSaveSubsystem* SaveSubsystem = World
		? World->GetSubsystem<UWildBoundSaveSubsystem>()
		: nullptr;

	if (SaveSubsystem)
	{
		SaveSubsystem->RestartFreshRun();
	}
}

void UWildBoundDeathSubsystem::CancelRestartFresh()
{
	bRestartConfirmationOpen = false;
}

void UWildBoundDeathSubsystem::ReturnToTitle()
{
	UWorld* World = GetWorld();
	UWildBoundSaveSubsystem* SaveSubsystem = World
		? World->GetSubsystem<UWildBoundSaveSubsystem>()
		: nullptr;

	if (SaveSubsystem)
	{
		SaveSubsystem->ReturnToTitle();
	}
}

void UWildBoundDeathSubsystem::EnsureDeathWidget()
{
	if (DeathViewportRoot.IsValid() || !GEngine || !GEngine->GameViewport)
	{
		return;
	}

	TSharedPtr<SWildBoundDeathWidget> DeathWidget;
	SAssignNew(DeathWidget, SWildBoundDeathWidget)
	.Owner(this);

	DeathViewportRoot = DeathWidget;
	DeathViewportRoot->SetVisibility(EVisibility::Collapsed);
	GEngine->GameViewport->AddViewportWidgetContent(DeathViewportRoot.ToSharedRef(), 230);
}

void UWildBoundDeathSubsystem::ApplyDeathInputState(bool bActive)
{
	UWorld* World = GetWorld();
	APlayerController* PlayerController = World ? World->GetFirstPlayerController() : nullptr;
	if (!World || !PlayerController)
	{
		return;
	}

	PlayerController->SetIgnoreMoveInput(bActive);
	PlayerController->SetIgnoreLookInput(bActive);
	PlayerController->bShowMouseCursor = bActive;

	if (bActive)
	{
		FInputModeGameAndUI InputMode;
		if (DeathViewportRoot.IsValid())
		{
			InputMode.SetWidgetToFocus(DeathViewportRoot);
		}
		InputMode.SetHideCursorDuringCapture(false);
		InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
		PlayerController->SetInputMode(InputMode);
		UGameplayStatics::SetGamePaused(World, true);
	}
	else
	{
		UGameplayStatics::SetGamePaused(World, false);
		FInputModeGameOnly InputMode;
		PlayerController->SetInputMode(InputMode);
	}
}

void UWildBoundDeathSubsystem::RemoveDeathWidget()
{
	if (DeathViewportRoot.IsValid() && GEngine && GEngine->GameViewport)
	{
		GEngine->GameViewport->RemoveViewportWidgetContent(DeathViewportRoot.ToSharedRef());
	}

	DeathViewportRoot.Reset();
}
