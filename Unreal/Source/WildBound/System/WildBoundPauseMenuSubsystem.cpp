#include "WildBoundPauseMenuSubsystem.h"

#include "WildBoundEvidenceLogSubsystem.h"
#include "WildBoundMainMenuSubsystem.h"
#include "WildBoundSaveSubsystem.h"
#include "../Crafting/WildBoundCraftingComponent.h"
#include "../Player/WildBoundBackpackComponent.h"
#include "../Player/WildBoundInteractionComponent.h"
#include "Engine/Engine.h"
#include "GameFramework/GameUserSettings.h"
#include "Engine/GameViewportClient.h"
#include "Engine/World.h"
#include "Framework/Application/SlateApplication.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "InputCoreTypes.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetSystemLibrary.h"
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
	class SWildBoundPauseMenuWidget : public SCompoundWidget
	{
	public:
		SLATE_BEGIN_ARGS(SWildBoundPauseMenuWidget) {}
			SLATE_ARGUMENT(UWildBoundPauseMenuSubsystem*, Owner)
		SLATE_END_ARGS()

		void Construct(const FArguments& InArgs)
		{
			OwnerSubsystem = InArgs._Owner;
			const TWeakObjectPtr<UWildBoundPauseMenuSubsystem> WeakOwner = OwnerSubsystem;

			ChildSlot
			[
				SNew(SOverlay)

				+ SOverlay::Slot()
				.HAlign(HAlign_Fill)
				.VAlign(VAlign_Fill)
				[
					SNew(SBorder)
					.BorderBackgroundColor(FLinearColor(0.0f, 0.0f, 0.0f, 0.72f))
				]

				+ SOverlay::Slot()
				.HAlign(HAlign_Center)
				.VAlign(VAlign_Center)
				.Padding(FMargin(24.0f))
				[
					SNew(SBox)
					.WidthOverride(520.0f)
					[
						SNew(SBorder)
						.Padding(FMargin(26.0f, 22.0f))
						.BorderBackgroundColor(FLinearColor(0.014f, 0.018f, 0.016f, 0.995f))
						[
							SNew(SOverlay)

							+ SOverlay::Slot()
							[
								SNew(SVerticalBox)
								.Visibility_Lambda([WeakOwner]()
								{
									const UWildBoundPauseMenuSubsystem* Owner = WeakOwner.Get();
									return Owner && !Owner->IsSettingsOpen()
										? EVisibility::Visible
										: EVisibility::Collapsed;
								})

								+ SVerticalBox::Slot().AutoHeight()
								[
									SNew(STextBlock)
									.Text(FText::FromString(TEXT("WILDBOUND")))
									.Font(FCoreStyle::GetDefaultFontStyle("Bold", 24))
									.ColorAndOpacity(FLinearColor(0.90f, 0.84f, 0.63f, 1.0f))
								]

								+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 2.0f, 0.0f, 12.0f)
								[
									SNew(STextBlock)
									.Text(FText::FromString(TEXT("PAUSED  /  EXCLUSION ZONE")))
									.Font(FCoreStyle::GetDefaultFontStyle("Bold", 9))
									.ColorAndOpacity(FLinearColor(0.48f, 0.55f, 0.48f, 1.0f))
								]

								+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 0.0f, 0.0f, 14.0f)
								[
									SNew(SSeparator)
								]

								+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 3.0f)
								[
									SNew(SButton)
									.ContentPadding(FMargin(14.0f, 10.0f))
									.OnClicked_Lambda([WeakOwner]()
									{
										if (UWildBoundPauseMenuSubsystem* Owner = WeakOwner.Get())
										{
											Owner->ContinueGame();
										}
										return FReply::Handled();
									})
									[
										SNew(STextBlock)
										.Text(FText::FromString(TEXT("CONTINUE")))
										.Font(FCoreStyle::GetDefaultFontStyle("Bold", 11))
									]
								]

								+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 3.0f)
								[
									SNew(SButton)
									.ContentPadding(FMargin(14.0f, 10.0f))
									.OnClicked_Lambda([WeakOwner]()
									{
										if (UWildBoundPauseMenuSubsystem* Owner = WeakOwner.Get())
										{
											Owner->SaveGame();
										}
										return FReply::Handled();
									})
									[
										SNew(STextBlock)
										.Text(FText::FromString(TEXT("SAVE GAME")))
										.Font(FCoreStyle::GetDefaultFontStyle("Bold", 11))
									]
								]

								+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 3.0f)
								[
									SNew(SButton)
									.IsEnabled_Lambda([WeakOwner]()
									{
										const UWildBoundPauseMenuSubsystem* Owner = WeakOwner.Get();
										return Owner && Owner->HasSaveGame();
									})
									.ContentPadding(FMargin(14.0f, 10.0f))
									.OnClicked_Lambda([WeakOwner]()
									{
										if (UWildBoundPauseMenuSubsystem* Owner = WeakOwner.Get())
										{
											Owner->LoadGame();
										}
										return FReply::Handled();
									})
									[
										SNew(STextBlock)
										.Text(FText::FromString(TEXT("LOAD LAST SAVE")))
										.Font(FCoreStyle::GetDefaultFontStyle("Bold", 11))
									]
								]

								+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 3.0f)
								[
									SNew(SButton)
									.ContentPadding(FMargin(14.0f, 10.0f))
									.OnClicked_Lambda([WeakOwner]()
									{
										if (UWildBoundPauseMenuSubsystem* Owner = WeakOwner.Get())
										{
											Owner->OpenSettings();
										}
										return FReply::Handled();
									})
									[
										SNew(STextBlock)
										.Text(FText::FromString(TEXT("SETTINGS")))
										.Font(FCoreStyle::GetDefaultFontStyle("Bold", 11))
									]
								]

								+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 3.0f, 0.0f, 14.0f)
								[
									SNew(SButton)
									.ContentPadding(FMargin(14.0f, 10.0f))
									.OnClicked_Lambda([WeakOwner]()
									{
										if (UWildBoundPauseMenuSubsystem* Owner = WeakOwner.Get())
										{
											Owner->QuitGame();
										}
										return FReply::Handled();
									})
									[
										SNew(STextBlock)
										.Text(FText::FromString(TEXT("SAVE & QUIT")))
										.Font(FCoreStyle::GetDefaultFontStyle("Bold", 11))
									]
								]

								+ SVerticalBox::Slot().AutoHeight()
								[
									SNew(STextBlock)
									.Text(FText::FromString(TEXT("ESC CONTINUE   |   AUTOSAVE ACTIVE")))
									.Font(FCoreStyle::GetDefaultFontStyle("Regular", 8))
									.ColorAndOpacity(FLinearColor(0.53f, 0.59f, 0.52f, 1.0f))
									.Justification(ETextJustify::Center)
								]
							]

							+ SOverlay::Slot()
							[
								SNew(SVerticalBox)
								.Visibility_Lambda([WeakOwner]()
								{
									const UWildBoundPauseMenuSubsystem* Owner = WeakOwner.Get();
									return Owner && Owner->IsSettingsOpen()
										? EVisibility::Visible
										: EVisibility::Collapsed;
								})

								+ SVerticalBox::Slot().AutoHeight()
								[
									SNew(STextBlock)
									.Text(FText::FromString(TEXT("SETTINGS")))
									.Font(FCoreStyle::GetDefaultFontStyle("Bold", 20))
									.ColorAndOpacity(FLinearColor(0.90f, 0.84f, 0.63f, 1.0f))
								]

								+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 3.0f, 0.0f, 12.0f)
								[
									SNew(STextBlock)
									.Text(FText::FromString(TEXT("DISPLAY / PERFORMANCE")))
									.Font(FCoreStyle::GetDefaultFontStyle("Bold", 8))
									.ColorAndOpacity(FLinearColor(0.48f, 0.55f, 0.48f, 1.0f))
								]

								+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 0.0f, 0.0f, 14.0f)
								[
									SNew(SSeparator)
								]

								+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 5.0f)
								[
									SNew(SBorder)
									.Padding(FMargin(10.0f))
									.BorderBackgroundColor(FLinearColor(0.030f, 0.038f, 0.032f, 0.98f))
									[
										SNew(SHorizontalBox)
										+ SHorizontalBox::Slot().FillWidth(1.0f).VAlign(VAlign_Center)
										[
											SNew(STextBlock)
											.Text(FText::FromString(TEXT("GRAPHICS QUALITY")))
											.Font(FCoreStyle::GetDefaultFontStyle("Bold", 9))
										]
										+ SHorizontalBox::Slot().AutoWidth().Padding(4.0f, 0.0f)
										[
											SNew(SButton)
											.ContentPadding(FMargin(10.0f, 5.0f))
											.OnClicked_Lambda([WeakOwner]()
											{
												if (UWildBoundPauseMenuSubsystem* Owner = WeakOwner.Get()) Owner->ChangeGraphicsQuality(-1);
												return FReply::Handled();
											})
											[
												SNew(STextBlock).Text(FText::FromString(TEXT("<")))
											]
										]
										+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(8.0f, 0.0f)
										[
											SNew(STextBlock)
											.Text_Lambda([WeakOwner]()
											{
												const UWildBoundPauseMenuSubsystem* Owner = WeakOwner.Get();
												return Owner ? Owner->GetGraphicsQualityText() : FText::GetEmpty();
											})
											.Font(FCoreStyle::GetDefaultFontStyle("Bold", 9))
										]
										+ SHorizontalBox::Slot().AutoWidth().Padding(4.0f, 0.0f)
										[
											SNew(SButton)
											.ContentPadding(FMargin(10.0f, 5.0f))
											.OnClicked_Lambda([WeakOwner]()
											{
												if (UWildBoundPauseMenuSubsystem* Owner = WeakOwner.Get()) Owner->ChangeGraphicsQuality(1);
												return FReply::Handled();
											})
											[
												SNew(STextBlock).Text(FText::FromString(TEXT(">")))
											]
										]
									]
								]

								+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 5.0f)
								[
									SNew(SButton)
									.ContentPadding(FMargin(12.0f, 10.0f))
									.OnClicked_Lambda([WeakOwner]()
									{
										if (UWildBoundPauseMenuSubsystem* Owner = WeakOwner.Get()) Owner->ToggleVSync();
										return FReply::Handled();
									})
									[
										SNew(STextBlock)
										.Text_Lambda([WeakOwner]()
										{
											const UWildBoundPauseMenuSubsystem* Owner = WeakOwner.Get();
											return Owner ? Owner->GetVSyncText() : FText::GetEmpty();
										})
										.Font(FCoreStyle::GetDefaultFontStyle("Bold", 10))
									]
								]

								+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 5.0f)
								[
									SNew(SButton)
									.ContentPadding(FMargin(12.0f, 10.0f))
									.OnClicked_Lambda([WeakOwner]()
									{
										if (UWildBoundPauseMenuSubsystem* Owner = WeakOwner.Get()) Owner->CycleFrameRateLimit();
										return FReply::Handled();
									})
									[
										SNew(STextBlock)
										.Text_Lambda([WeakOwner]()
										{
											const UWildBoundPauseMenuSubsystem* Owner = WeakOwner.Get();
											return Owner ? Owner->GetFrameRateLimitText() : FText::GetEmpty();
										})
										.Font(FCoreStyle::GetDefaultFontStyle("Bold", 10))
									]
								]

								+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 16.0f, 0.0f, 0.0f)
								[
									SNew(SButton)
									.ContentPadding(FMargin(12.0f, 10.0f))
									.OnClicked_Lambda([WeakOwner]()
									{
										if (UWildBoundPauseMenuSubsystem* Owner = WeakOwner.Get()) Owner->CloseSettings();
										return FReply::Handled();
									})
									[
										SNew(STextBlock)
										.Text(FText::FromString(TEXT("BACK")))
										.Font(FCoreStyle::GetDefaultFontStyle("Bold", 10))
									]
								]

								+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 10.0f, 0.0f, 0.0f)
								[
									SNew(STextBlock)
									.Text(FText::FromString(TEXT("ESC BACK   |   SETTINGS SAVE IMMEDIATELY")))
									.Font(FCoreStyle::GetDefaultFontStyle("Regular", 8))
									.ColorAndOpacity(FLinearColor(0.53f, 0.59f, 0.52f, 1.0f))
									.Justification(ETextJustify::Center)
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

		virtual FReply OnKeyDown(const FGeometry& MyGeometry, const FKeyEvent& InKeyEvent) override
		{
			if (InKeyEvent.GetKey() == EKeys::Escape)
			{
				if (UWildBoundPauseMenuSubsystem* Owner = OwnerSubsystem.Get())
				{
					Owner->HandleEscape();
				}
				return FReply::Handled();
			}

			return SCompoundWidget::OnKeyDown(MyGeometry, InKeyEvent);
		}

	private:
		TWeakObjectPtr<UWildBoundPauseMenuSubsystem> OwnerSubsystem;
	};
}

void UWildBoundPauseMenuSubsystem::OnWorldBeginPlay(UWorld& InWorld)
{
	Super::OnWorldBeginPlay(InWorld);

	if (!InWorld.IsGameWorld())
	{
		return;
	}

	EnsurePauseWidget();
	bBlockingUIWasOpenLastCheck = HasBlockingUIOpen();

	InWorld.GetTimerManager().SetTimer(
		PauseInputTimer,
		this,
		&UWildBoundPauseMenuSubsystem::UpdatePauseInput,
		0.04f,
		true,
		0.20f);
}

void UWildBoundPauseMenuSubsystem::Deinitialize()
{
	if (bPauseMenuOpen)
	{
		ApplyPauseState(false);
	}

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(PauseInputTimer);
	}

	RemovePauseWidget();
	Super::Deinitialize();
}

void UWildBoundPauseMenuSubsystem::UpdatePauseInput()
{
	if (bPauseMenuOpen)
	{
		return;
	}

	UWorld* World = GetWorld();
	APlayerController* PlayerController = World ? World->GetFirstPlayerController() : nullptr;
	if (!PlayerController)
	{
		return;
	}

	EnsurePauseWidget();

	const bool bBlockingUIOpen = HasBlockingUIOpen();
	if (PlayerController->WasInputKeyJustPressed(EKeys::Escape)
		&& !bBlockingUIOpen
		&& !bBlockingUIWasOpenLastCheck)
	{
		OpenPauseMenu();
	}

	bBlockingUIWasOpenLastCheck = bBlockingUIOpen;
}

void UWildBoundPauseMenuSubsystem::EnsurePauseWidget()
{
	if (PauseViewportRoot.IsValid() || !GEngine || !GEngine->GameViewport)
	{
		return;
	}

	TSharedPtr<SWildBoundPauseMenuWidget> PauseWidget;
	SAssignNew(PauseWidget, SWildBoundPauseMenuWidget)
	.Owner(this);

	PauseViewportRoot = PauseWidget;
	GEngine->GameViewport->AddViewportWidgetContent(PauseViewportRoot.ToSharedRef(), 170);
}

void UWildBoundPauseMenuSubsystem::OpenPauseMenu()
{
	if (bPauseMenuOpen || HasBlockingUIOpen())
	{
		return;
	}

	EnsurePauseWidget();
	if (!PauseViewportRoot.IsValid())
	{
		return;
	}

	bSettingsOpen = false;
	bPauseMenuOpen = true;
	PauseViewportRoot->SetVisibility(EVisibility::Visible);
	ApplyPauseState(true);

	if (FSlateApplication::IsInitialized())
	{
		FSlateApplication::Get().SetKeyboardFocus(PauseViewportRoot, EFocusCause::SetDirectly);
	}
}

void UWildBoundPauseMenuSubsystem::ContinueGame()
{
	if (!bPauseMenuOpen)
	{
		return;
	}

	bSettingsOpen = false;
	bPauseMenuOpen = false;

	if (PauseViewportRoot.IsValid())
	{
		PauseViewportRoot->SetVisibility(EVisibility::Collapsed);
	}

	ApplyPauseState(false);
	bBlockingUIWasOpenLastCheck = false;
}

void UWildBoundPauseMenuSubsystem::HandleEscape()
{
	if (!bPauseMenuOpen)
	{
		return;
	}

	if (bSettingsOpen)
	{
		CloseSettings();
		return;
	}

	ContinueGame();
}

void UWildBoundPauseMenuSubsystem::SaveGame()
{
	UWorld* World = GetWorld();
	UWildBoundSaveSubsystem* SaveSubsystem = World
		? World->GetSubsystem<UWildBoundSaveSubsystem>()
		: nullptr;

	if (SaveSubsystem)
	{
		SaveSubsystem->SaveNow(true);
	}
}

void UWildBoundPauseMenuSubsystem::LoadGame()
{
	UWorld* World = GetWorld();
	UWildBoundSaveSubsystem* SaveSubsystem = World
		? World->GetSubsystem<UWildBoundSaveSubsystem>()
		: nullptr;

	if (!SaveSubsystem || !SaveSubsystem->HasSaveGame())
	{
		return;
	}

	ContinueGame();
	SaveSubsystem->ReloadLastSave();
}

void UWildBoundPauseMenuSubsystem::OpenSettings()
{
	if (bPauseMenuOpen)
	{
		bSettingsOpen = true;
	}
}

void UWildBoundPauseMenuSubsystem::CloseSettings()
{
	bSettingsOpen = false;
}

void UWildBoundPauseMenuSubsystem::QuitGame()
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	if (UWildBoundSaveSubsystem* SaveSubsystem = World->GetSubsystem<UWildBoundSaveSubsystem>())
	{
		SaveSubsystem->SaveNow(false);
	}

	APlayerController* PlayerController = World->GetFirstPlayerController();
	UKismetSystemLibrary::QuitGame(
		World,
		PlayerController,
		EQuitPreference::Quit,
		false);
}

void UWildBoundPauseMenuSubsystem::ChangeGraphicsQuality(int32 Delta)
{
	UGameUserSettings* Settings = GEngine ? GEngine->GetGameUserSettings() : nullptr;
	if (!Settings || Delta == 0)
	{
		return;
	}

	int32 CurrentLevel = Settings->GetOverallScalabilityLevel();
	if (CurrentLevel < 0 || CurrentLevel > 4)
	{
		CurrentLevel = 2;
	}

	const int32 NewLevel = (CurrentLevel + Delta + 5) % 5;
	Settings->SetOverallScalabilityLevel(NewLevel);
	Settings->ApplySettings(false);
	Settings->SaveSettings();
}

void UWildBoundPauseMenuSubsystem::ToggleVSync()
{
	UGameUserSettings* Settings = GEngine ? GEngine->GetGameUserSettings() : nullptr;
	if (!Settings)
	{
		return;
	}

	Settings->SetVSyncEnabled(!Settings->IsVSyncEnabled());
	Settings->ApplySettings(false);
	Settings->SaveSettings();
}

void UWildBoundPauseMenuSubsystem::CycleFrameRateLimit()
{
	UGameUserSettings* Settings = GEngine ? GEngine->GetGameUserSettings() : nullptr;
	if (!Settings)
	{
		return;
	}

	const float CurrentLimit = Settings->GetFrameRateLimit();
	float NewLimit = 60.0f;

	if (CurrentLimit > 0.5f && CurrentLimit <= 60.5f)
	{
		NewLimit = 120.0f;
	}
	else if (CurrentLimit > 60.5f && CurrentLimit <= 120.5f)
	{
		NewLimit = 144.0f;
	}
	else if (CurrentLimit > 120.5f && CurrentLimit <= 144.5f)
	{
		NewLimit = 0.0f;
	}
	else if (CurrentLimit <= 0.5f)
	{
		NewLimit = 60.0f;
	}

	Settings->SetFrameRateLimit(NewLimit);
	Settings->ApplySettings(false);
	Settings->SaveSettings();
}

FText UWildBoundPauseMenuSubsystem::GetGraphicsQualityText() const
{
	const UGameUserSettings* Settings = GEngine ? GEngine->GetGameUserSettings() : nullptr;
	const int32 Level = Settings ? Settings->GetOverallScalabilityLevel() : -1;

	switch (Level)
	{
	case 0: return FText::FromString(TEXT("LOW"));
	case 1: return FText::FromString(TEXT("MEDIUM"));
	case 2: return FText::FromString(TEXT("HIGH"));
	case 3: return FText::FromString(TEXT("EPIC"));
	case 4: return FText::FromString(TEXT("CINEMATIC"));
	default: return FText::FromString(TEXT("CUSTOM"));
	}
}

FText UWildBoundPauseMenuSubsystem::GetVSyncText() const
{
	const UGameUserSettings* Settings = GEngine ? GEngine->GetGameUserSettings() : nullptr;
	const bool bEnabled = Settings && Settings->IsVSyncEnabled();
	return FText::FromString(bEnabled ? TEXT("VSYNC  /  ON") : TEXT("VSYNC  /  OFF"));
}

FText UWildBoundPauseMenuSubsystem::GetFrameRateLimitText() const
{
	const UGameUserSettings* Settings = GEngine ? GEngine->GetGameUserSettings() : nullptr;
	const float Limit = Settings ? Settings->GetFrameRateLimit() : 0.0f;

	if (Limit <= 0.5f)
	{
		return FText::FromString(TEXT("FRAME LIMIT  /  UNLIMITED"));
	}

	return FText::FromString(FString::Printf(
		TEXT("FRAME LIMIT  /  %d FPS"),
		FMath::RoundToInt(Limit)));
}

bool UWildBoundPauseMenuSubsystem::HasSaveGame() const
{
	const UWorld* World = GetWorld();
	const UWildBoundSaveSubsystem* SaveSubsystem = World
		? World->GetSubsystem<UWildBoundSaveSubsystem>()
		: nullptr;

	return SaveSubsystem && SaveSubsystem->HasSaveGame();
}

bool UWildBoundPauseMenuSubsystem::HasBlockingUIOpen() const
{
	UWorld* World = GetWorld();
	APlayerController* PlayerController = World ? World->GetFirstPlayerController() : nullptr;
	APawn* Pawn = PlayerController ? PlayerController->GetPawn() : nullptr;
	if (!World || !Pawn)
	{
		return true;
	}

	const UWildBoundBackpackComponent* Backpack = Pawn->FindComponentByClass<UWildBoundBackpackComponent>();
	const UWildBoundCraftingComponent* Crafting = Pawn->FindComponentByClass<UWildBoundCraftingComponent>();
	const UWildBoundInteractionComponent* Interaction = Pawn->FindComponentByClass<UWildBoundInteractionComponent>();
	const UWildBoundEvidenceLogSubsystem* EvidenceLog = World->GetSubsystem<UWildBoundEvidenceLogSubsystem>();
	const UWildBoundMainMenuSubsystem* MainMenu = World->GetSubsystem<UWildBoundMainMenuSubsystem>();

	return (MainMenu && MainMenu->IsMainMenuOpen())
		|| (Backpack && Backpack->IsBackpackOpen())
		|| (Crafting && Crafting->IsCraftingOpen())
		|| (Interaction && (Interaction->IsLootWindowOpen() || Interaction->IsTreatmentInProgress()))
		|| (EvidenceLog && EvidenceLog->IsEvidenceLogOpen());
}

void UWildBoundPauseMenuSubsystem::ApplyPauseState(bool bPaused)
{
	UWorld* World = GetWorld();
	APlayerController* PlayerController = World ? World->GetFirstPlayerController() : nullptr;
	if (!World || !PlayerController)
	{
		return;
	}

	PlayerController->SetIgnoreMoveInput(bPaused);
	PlayerController->SetIgnoreLookInput(bPaused);
	PlayerController->bShowMouseCursor = bPaused;

	if (bPaused)
	{
		FInputModeGameAndUI InputMode;
		if (PauseViewportRoot.IsValid())
		{
			InputMode.SetWidgetToFocus(PauseViewportRoot);
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

void UWildBoundPauseMenuSubsystem::RemovePauseWidget()
{
	if (PauseViewportRoot.IsValid() && GEngine && GEngine->GameViewport)
	{
		GEngine->GameViewport->RemoveViewportWidgetContent(PauseViewportRoot.ToSharedRef());
	}

	PauseViewportRoot.Reset();
}
