#include "WildBoundMainMenuSubsystem.h"

#include "WildBoundSaveSubsystem.h"
#include "Engine/Engine.h"
#include "Engine/GameViewportClient.h"
#include "Engine/World.h"
#include "Framework/Application/SlateApplication.h"
#include "GameFramework/GameUserSettings.h"
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
	bool GSuppressNextWildBoundMainMenu = false;

	class SWildBoundMainMenuWidget : public SCompoundWidget
	{
	public:
		SLATE_BEGIN_ARGS(SWildBoundMainMenuWidget) {}
			SLATE_ARGUMENT(UWildBoundMainMenuSubsystem*, Owner)
		SLATE_END_ARGS()

		void Construct(const FArguments& InArgs)
		{
			OwnerSubsystem = InArgs._Owner;
			const TWeakObjectPtr<UWildBoundMainMenuSubsystem> WeakOwner = OwnerSubsystem;

			ChildSlot
			[
				SNew(SOverlay)

				+ SOverlay::Slot()
				.HAlign(HAlign_Fill)
				.VAlign(VAlign_Fill)
				[
					SNew(SBorder)
					.BorderBackgroundColor(FLinearColor(0.008f, 0.011f, 0.010f, 0.995f))
				]

				+ SOverlay::Slot()
				.HAlign(HAlign_Left)
				.VAlign(VAlign_Center)
				.Padding(FMargin(92.0f, 40.0f, 40.0f, 40.0f))
				[
					SNew(SBox)
					.WidthOverride(560.0f)
					[
						SNew(SBorder)
						.Padding(FMargin(30.0f, 28.0f))
						.BorderBackgroundColor(FLinearColor(0.018f, 0.024f, 0.021f, 0.98f))
						[
							SNew(SOverlay)

							+ SOverlay::Slot()
							[
								SNew(SVerticalBox)
								.Visibility_Lambda([WeakOwner]()
								{
									const UWildBoundMainMenuSubsystem* Owner = WeakOwner.Get();
									return Owner
										&& !Owner->IsSettingsPageOpen()
										&& !Owner->IsLoadPageOpen()
										&& !Owner->IsNewGameConfirmationOpen()
										? EVisibility::Visible
										: EVisibility::Collapsed;
								})

								+ SVerticalBox::Slot().AutoHeight()
								[
									SNew(STextBlock)
									.Text(FText::FromString(TEXT("WILDBOUND")))
									.Font(FCoreStyle::GetDefaultFontStyle("Bold", 34))
									.ColorAndOpacity(FLinearColor(0.90f, 0.84f, 0.62f, 1.0f))
								]

								+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 1.0f, 0.0f, 8.0f)
								[
									SNew(STextBlock)
									.Text(FText::FromString(TEXT("THE EXCLUSION ZONE")))
									.Font(FCoreStyle::GetDefaultFontStyle("Bold", 11))
									.ColorAndOpacity(FLinearColor(0.46f, 0.54f, 0.47f, 1.0f))
								]

								+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 0.0f, 0.0f, 16.0f)
								[
									SNew(STextBlock)
									.Text(FText::FromString(TEXT("Survive the fallout. Recover what was left behind.\nFind out what happened before the alert.")))
									.AutoWrapText(true)
									.Font(FCoreStyle::GetDefaultFontStyle("Regular", 10))
									.ColorAndOpacity(FLinearColor(0.70f, 0.74f, 0.68f, 1.0f))
								]

								+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 0.0f, 0.0f, 16.0f)
								[
									SNew(SSeparator)
								]

								+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 3.0f)
								[
									SNew(SButton)
									.IsEnabled_Lambda([WeakOwner]()
									{
										const UWildBoundMainMenuSubsystem* Owner = WeakOwner.Get();
										return Owner && Owner->HasSaveGame();
									})
									.ContentPadding(FMargin(14.0f, 11.0f))
									.OnClicked_Lambda([WeakOwner]()
									{
										if (UWildBoundMainMenuSubsystem* Owner = WeakOwner.Get())
										{
											Owner->ContinueLastGame();
										}
										return FReply::Handled();
									})
									[
										SNew(STextBlock)
										.Text(FText::FromString(TEXT("CONTINUE")))
										.Font(FCoreStyle::GetDefaultFontStyle("Bold", 12))
									]
								]

								+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 3.0f)
								[
									SNew(SButton)
									.ContentPadding(FMargin(14.0f, 11.0f))
									.OnClicked_Lambda([WeakOwner]()
									{
										if (UWildBoundMainMenuSubsystem* Owner = WeakOwner.Get())
										{
											Owner->RequestNewGame();
										}
										return FReply::Handled();
									})
									[
										SNew(STextBlock)
										.Text(FText::FromString(TEXT("NEW GAME")))
										.Font(FCoreStyle::GetDefaultFontStyle("Bold", 12))
									]
								]

								+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 3.0f)
								[
									SNew(SButton)
									.IsEnabled_Lambda([WeakOwner]()
									{
										const UWildBoundMainMenuSubsystem* Owner = WeakOwner.Get();
										return Owner && Owner->HasSaveGame();
									})
									.ContentPadding(FMargin(14.0f, 11.0f))
									.OnClicked_Lambda([WeakOwner]()
									{
										if (UWildBoundMainMenuSubsystem* Owner = WeakOwner.Get())
										{
											Owner->OpenLoadPage();
										}
										return FReply::Handled();
									})
									[
										SNew(STextBlock)
										.Text(FText::FromString(TEXT("LOAD")))
										.Font(FCoreStyle::GetDefaultFontStyle("Bold", 12))
									]
								]

								+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 3.0f)
								[
									SNew(SButton)
									.ContentPadding(FMargin(14.0f, 11.0f))
									.OnClicked_Lambda([WeakOwner]()
									{
										if (UWildBoundMainMenuSubsystem* Owner = WeakOwner.Get())
										{
											Owner->OpenSettingsPage();
										}
										return FReply::Handled();
									})
									[
										SNew(STextBlock)
										.Text(FText::FromString(TEXT("SETTINGS")))
										.Font(FCoreStyle::GetDefaultFontStyle("Bold", 12))
									]
								]

								+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 3.0f)
								[
									SNew(SButton)
									.ContentPadding(FMargin(14.0f, 11.0f))
									.OnClicked_Lambda([WeakOwner]()
									{
										if (UWildBoundMainMenuSubsystem* Owner = WeakOwner.Get())
										{
											Owner->QuitGame();
										}
										return FReply::Handled();
									})
									[
										SNew(STextBlock)
										.Text(FText::FromString(TEXT("QUIT")))
										.Font(FCoreStyle::GetDefaultFontStyle("Bold", 12))
									]
								]

								+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 14.0f, 0.0f, 0.0f)
								[
									SNew(STextBlock)
									.Text(FText::FromString(TEXT("AUTOSAVE ENABLED DURING ACTIVE RUNS")))
									.Font(FCoreStyle::GetDefaultFontStyle("Regular", 8))
									.ColorAndOpacity(FLinearColor(0.46f, 0.51f, 0.46f, 1.0f))
								]
							]

							+ SOverlay::Slot()
							[
								SNew(SVerticalBox)
								.Visibility_Lambda([WeakOwner]()
								{
									const UWildBoundMainMenuSubsystem* Owner = WeakOwner.Get();
									return Owner && Owner->IsLoadPageOpen()
										? EVisibility::Visible
										: EVisibility::Collapsed;
								})

								+ SVerticalBox::Slot().AutoHeight()
								[
									SNew(STextBlock)
									.Text(FText::FromString(TEXT("LOAD GAME")))
									.Font(FCoreStyle::GetDefaultFontStyle("Bold", 22))
									.ColorAndOpacity(FLinearColor(0.90f, 0.84f, 0.62f, 1.0f))
								]

								+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 8.0f, 0.0f, 14.0f)
								[
									SNew(SSeparator)
								]

								+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 4.0f)
								[
									SNew(SButton)
									.ContentPadding(FMargin(14.0f, 14.0f))
									.OnClicked_Lambda([WeakOwner]()
									{
										if (UWildBoundMainMenuSubsystem* Owner = WeakOwner.Get())
										{
											Owner->LoadSelectedSave();
										}
										return FReply::Handled();
									})
									[
										SNew(SVerticalBox)
										+ SVerticalBox::Slot().AutoHeight()
										[
											SNew(STextBlock)
											.Text(FText::FromString(TEXT("AUTOSAVE SLOT")))
											.Font(FCoreStyle::GetDefaultFontStyle("Bold", 11))
										]
										+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 3.0f, 0.0f, 0.0f)
										[
											SNew(STextBlock)
											.Text(FText::FromString(TEXT("Latest persistent WildBound run")))
											.Font(FCoreStyle::GetDefaultFontStyle("Regular", 8))
											.ColorAndOpacity(FLinearColor(0.58f, 0.63f, 0.57f, 1.0f))
										]
									]
								]

								+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 14.0f, 0.0f, 0.0f)
								[
									SNew(SButton)
									.OnClicked_Lambda([WeakOwner]()
									{
										if (UWildBoundMainMenuSubsystem* Owner = WeakOwner.Get()) Owner->BackToRoot();
										return FReply::Handled();
									})
									[
										SNew(STextBlock).Text(FText::FromString(TEXT("BACK")))
									]
								]
							]

							+ SOverlay::Slot()
							[
								SNew(SVerticalBox)
								.Visibility_Lambda([WeakOwner]()
								{
									const UWildBoundMainMenuSubsystem* Owner = WeakOwner.Get();
									return Owner && Owner->IsNewGameConfirmationOpen()
										? EVisibility::Visible
										: EVisibility::Collapsed;
								})

								+ SVerticalBox::Slot().AutoHeight()
								[
									SNew(STextBlock)
									.Text(FText::FromString(TEXT("START NEW GAME?")))
									.Font(FCoreStyle::GetDefaultFontStyle("Bold", 21))
									.ColorAndOpacity(FLinearColor(0.95f, 0.75f, 0.43f, 1.0f))
								]

								+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 10.0f, 0.0f, 18.0f)
								[
									SNew(STextBlock)
									.Text(FText::FromString(TEXT("Starting a new run will overwrite the existing autosave.\nThis cannot be undone.")))
									.AutoWrapText(true)
									.Font(FCoreStyle::GetDefaultFontStyle("Regular", 10))
									.ColorAndOpacity(FLinearColor(0.76f, 0.76f, 0.70f, 1.0f))
								]

								+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 4.0f)
								[
									SNew(SButton)
									.OnClicked_Lambda([WeakOwner]()
									{
										if (UWildBoundMainMenuSubsystem* Owner = WeakOwner.Get()) Owner->ConfirmNewGame();
										return FReply::Handled();
									})
									[
										SNew(STextBlock)
										.Text(FText::FromString(TEXT("DELETE AUTOSAVE & START")))
										.Font(FCoreStyle::GetDefaultFontStyle("Bold", 10))
									]
								]

								+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 4.0f)
								[
									SNew(SButton)
									.OnClicked_Lambda([WeakOwner]()
									{
										if (UWildBoundMainMenuSubsystem* Owner = WeakOwner.Get()) Owner->CancelNewGame();
										return FReply::Handled();
									})
									[
										SNew(STextBlock).Text(FText::FromString(TEXT("CANCEL")))
									]
								]
							]

							+ SOverlay::Slot()
							[
								SNew(SVerticalBox)
								.Visibility_Lambda([WeakOwner]()
								{
									const UWildBoundMainMenuSubsystem* Owner = WeakOwner.Get();
									return Owner && Owner->IsSettingsPageOpen()
										? EVisibility::Visible
										: EVisibility::Collapsed;
								})

								+ SVerticalBox::Slot().AutoHeight()
								[
									SNew(STextBlock)
									.Text(FText::FromString(TEXT("SETTINGS")))
									.Font(FCoreStyle::GetDefaultFontStyle("Bold", 22))
									.ColorAndOpacity(FLinearColor(0.90f, 0.84f, 0.62f, 1.0f))
								]

								+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 8.0f, 0.0f, 14.0f)
								[
									SNew(SSeparator)
								]

								+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 4.0f)
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
											.OnClicked_Lambda([WeakOwner]()
											{
												if (UWildBoundMainMenuSubsystem* Owner = WeakOwner.Get()) Owner->ChangeGraphicsQuality(-1);
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
												const UWildBoundMainMenuSubsystem* Owner = WeakOwner.Get();
												return Owner ? Owner->GetGraphicsQualityText() : FText::GetEmpty();
											})
											.Font(FCoreStyle::GetDefaultFontStyle("Bold", 9))
										]
										+ SHorizontalBox::Slot().AutoWidth().Padding(4.0f, 0.0f)
										[
											SNew(SButton)
											.OnClicked_Lambda([WeakOwner]()
											{
												if (UWildBoundMainMenuSubsystem* Owner = WeakOwner.Get()) Owner->ChangeGraphicsQuality(1);
												return FReply::Handled();
											})
											[
												SNew(STextBlock).Text(FText::FromString(TEXT(">")))
											]
										]
									]
								]

								+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 4.0f)
								[
									SNew(SButton)
									.OnClicked_Lambda([WeakOwner]()
									{
										if (UWildBoundMainMenuSubsystem* Owner = WeakOwner.Get()) Owner->ToggleVSync();
										return FReply::Handled();
									})
									[
										SNew(STextBlock)
										.Text_Lambda([WeakOwner]()
										{
											const UWildBoundMainMenuSubsystem* Owner = WeakOwner.Get();
											return Owner ? Owner->GetVSyncText() : FText::GetEmpty();
										})
										.Font(FCoreStyle::GetDefaultFontStyle("Bold", 10))
									]
								]

								+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 4.0f)
								[
									SNew(SButton)
									.OnClicked_Lambda([WeakOwner]()
									{
										if (UWildBoundMainMenuSubsystem* Owner = WeakOwner.Get()) Owner->CycleFrameRateLimit();
										return FReply::Handled();
									})
									[
										SNew(STextBlock)
										.Text_Lambda([WeakOwner]()
										{
											const UWildBoundMainMenuSubsystem* Owner = WeakOwner.Get();
											return Owner ? Owner->GetFrameRateLimitText() : FText::GetEmpty();
										})
										.Font(FCoreStyle::GetDefaultFontStyle("Bold", 10))
									]
								]

								+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 14.0f, 0.0f, 0.0f)
								[
									SNew(SButton)
									.OnClicked_Lambda([WeakOwner]()
									{
										if (UWildBoundMainMenuSubsystem* Owner = WeakOwner.Get()) Owner->BackToRoot();
										return FReply::Handled();
									})
									[
										SNew(STextBlock).Text(FText::FromString(TEXT("BACK")))
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

		virtual FReply OnKeyDown(const FGeometry& MyGeometry, const FKeyEvent& InKeyEvent) override
		{
			if (InKeyEvent.GetKey() == EKeys::Escape)
			{
				if (UWildBoundMainMenuSubsystem* Owner = OwnerSubsystem.Get())
				{
					if (Owner->IsSettingsPageOpen()
						|| Owner->IsLoadPageOpen()
						|| Owner->IsNewGameConfirmationOpen())
					{
						Owner->BackToRoot();
					}
				}
				return FReply::Handled();
			}

			return SCompoundWidget::OnKeyDown(MyGeometry, InKeyEvent);
		}

	private:
		TWeakObjectPtr<UWildBoundMainMenuSubsystem> OwnerSubsystem;
	};
}

void UWildBoundMainMenuSubsystem::SuppressNextWorldMenuOnce()
{
	GSuppressNextWildBoundMainMenu = true;
}

void UWildBoundMainMenuSubsystem::OnWorldBeginPlay(UWorld& InWorld)
{
	Super::OnWorldBeginPlay(InWorld);

	if (!InWorld.IsGameWorld())
	{
		return;
	}

	if (GSuppressNextWildBoundMainMenu)
	{
		GSuppressNextWildBoundMainMenu = false;
		return;
	}

	TryOpenMainMenu();
	InWorld.GetTimerManager().SetTimer(
		StartupTimer,
		this,
		&UWildBoundMainMenuSubsystem::TryOpenMainMenu,
		0.20f,
		true,
		0.15f);
}

void UWildBoundMainMenuSubsystem::Deinitialize()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(StartupTimer);
	}

	SetStartupInputLock(false);

	if (bMainMenuOpen)
	{
		ApplyMenuState(false);
	}

	RemoveMainMenuWidget();
	Super::Deinitialize();
}

void UWildBoundMainMenuSubsystem::TryOpenMainMenu()
{
	if (bMainMenuOpen)
	{
		return;
	}

	UWorld* World = GetWorld();
	APlayerController* PlayerController = World ? World->GetFirstPlayerController() : nullptr;
	UWildBoundSaveSubsystem* SaveSubsystem = World
		? World->GetSubsystem<UWildBoundSaveSubsystem>()
		: nullptr;

	if (!World || !PlayerController)
	{
		return;
	}

	if (!SaveSubsystem || !SaveSubsystem->IsPersistenceReady())
	{
		SetStartupInputLock(true);
		return;
	}

	SetStartupInputLock(false);
	OpenMainMenu();
	World->GetTimerManager().ClearTimer(StartupTimer);
}

void UWildBoundMainMenuSubsystem::EnsureMainMenuWidget()
{
	if (MainMenuViewportRoot.IsValid() || !GEngine || !GEngine->GameViewport)
	{
		return;
	}

	TSharedPtr<SWildBoundMainMenuWidget> MenuWidget;
	SAssignNew(MenuWidget, SWildBoundMainMenuWidget)
	.Owner(this);

	MainMenuViewportRoot = MenuWidget;
	GEngine->GameViewport->AddViewportWidgetContent(MainMenuViewportRoot.ToSharedRef(), 200);
}

void UWildBoundMainMenuSubsystem::OpenMainMenu()
{
	if (bMainMenuOpen)
	{
		return;
	}

	EnsureMainMenuWidget();
	if (!MainMenuViewportRoot.IsValid())
	{
		return;
	}

	bSettingsPageOpen = false;
	bLoadPageOpen = false;
	bNewGameConfirmationOpen = false;
	bMainMenuOpen = true;
	MainMenuViewportRoot->SetVisibility(EVisibility::Visible);
	ApplyMenuState(true);

	if (FSlateApplication::IsInitialized())
	{
		FSlateApplication::Get().SetKeyboardFocus(MainMenuViewportRoot, EFocusCause::SetDirectly);
	}
}

void UWildBoundMainMenuSubsystem::CloseMainMenu()
{
	if (!bMainMenuOpen)
	{
		return;
	}

	bSettingsPageOpen = false;
	bLoadPageOpen = false;
	bNewGameConfirmationOpen = false;
	bMainMenuOpen = false;

	if (MainMenuViewportRoot.IsValid())
	{
		MainMenuViewportRoot->SetVisibility(EVisibility::Collapsed);
	}

	ApplyMenuState(false);
}

void UWildBoundMainMenuSubsystem::ContinueLastGame()
{
	UWorld* World = GetWorld();
	UWildBoundSaveSubsystem* SaveSubsystem = World
		? World->GetSubsystem<UWildBoundSaveSubsystem>()
		: nullptr;

	if (SaveSubsystem && SaveSubsystem->LoadNow(false))
	{
		CloseMainMenu();
	}
}

void UWildBoundMainMenuSubsystem::RequestNewGame()
{
	if (!HasSaveGame())
	{
		ConfirmNewGame();
		return;
	}

	bSettingsPageOpen = false;
	bLoadPageOpen = false;
	bNewGameConfirmationOpen = true;
}

void UWildBoundMainMenuSubsystem::ConfirmNewGame()
{
	UWorld* World = GetWorld();
	UWildBoundSaveSubsystem* SaveSubsystem = World
		? World->GetSubsystem<UWildBoundSaveSubsystem>()
		: nullptr;

	if (SaveSubsystem && SaveSubsystem->StartNewGame())
	{
		CloseMainMenu();
	}
}

void UWildBoundMainMenuSubsystem::CancelNewGame()
{
	BackToRoot();
}

void UWildBoundMainMenuSubsystem::OpenLoadPage()
{
	if (!HasSaveGame())
	{
		return;
	}

	bSettingsPageOpen = false;
	bNewGameConfirmationOpen = false;
	bLoadPageOpen = true;
}

void UWildBoundMainMenuSubsystem::OpenSettingsPage()
{
	bLoadPageOpen = false;
	bNewGameConfirmationOpen = false;
	bSettingsPageOpen = true;
}

void UWildBoundMainMenuSubsystem::BackToRoot()
{
	bSettingsPageOpen = false;
	bLoadPageOpen = false;
	bNewGameConfirmationOpen = false;
}

void UWildBoundMainMenuSubsystem::LoadSelectedSave()
{
	ContinueLastGame();
}

void UWildBoundMainMenuSubsystem::QuitGame()
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	APlayerController* PlayerController = World->GetFirstPlayerController();
	UKismetSystemLibrary::QuitGame(
		World,
		PlayerController,
		EQuitPreference::Quit,
		false);
}

void UWildBoundMainMenuSubsystem::ChangeGraphicsQuality(int32 Delta)
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

void UWildBoundMainMenuSubsystem::ToggleVSync()
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

void UWildBoundMainMenuSubsystem::CycleFrameRateLimit()
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

FText UWildBoundMainMenuSubsystem::GetGraphicsQualityText() const
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

FText UWildBoundMainMenuSubsystem::GetVSyncText() const
{
	const UGameUserSettings* Settings = GEngine ? GEngine->GetGameUserSettings() : nullptr;
	const bool bEnabled = Settings && Settings->IsVSyncEnabled();
	return FText::FromString(bEnabled ? TEXT("VSYNC  /  ON") : TEXT("VSYNC  /  OFF"));
}

FText UWildBoundMainMenuSubsystem::GetFrameRateLimitText() const
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

bool UWildBoundMainMenuSubsystem::HasSaveGame() const
{
	const UWorld* World = GetWorld();
	const UWildBoundSaveSubsystem* SaveSubsystem = World
		? World->GetSubsystem<UWildBoundSaveSubsystem>()
		: nullptr;
	return SaveSubsystem && SaveSubsystem->HasSaveGame();
}

void UWildBoundMainMenuSubsystem::SetStartupInputLock(bool bLocked)
{
	if (bStartupInputLocked == bLocked)
	{
		return;
	}

	UWorld* World = GetWorld();
	APlayerController* PlayerController = World ? World->GetFirstPlayerController() : nullptr;
	if (!PlayerController)
	{
		return;
	}

	bStartupInputLocked = bLocked;
	if (bLocked)
	{
		PlayerController->SetIgnoreMoveInput(true);
		PlayerController->SetIgnoreLookInput(true);
		PlayerController->bShowMouseCursor = false;
		FInputModeUIOnly InputMode;
		PlayerController->SetInputMode(InputMode);
	}
	else
	{
		PlayerController->ResetIgnoreMoveInput();
		PlayerController->ResetIgnoreLookInput();
	}
}

void UWildBoundMainMenuSubsystem::ApplyMenuState(bool bOpen)
{
	UWorld* World = GetWorld();
	APlayerController* PlayerController = World ? World->GetFirstPlayerController() : nullptr;
	if (!World || !PlayerController)
	{
		return;
	}

	PlayerController->SetIgnoreMoveInput(bOpen);
	PlayerController->SetIgnoreLookInput(bOpen);
	PlayerController->bShowMouseCursor = bOpen;

	if (bOpen)
	{
		FInputModeGameAndUI InputMode;
		if (MainMenuViewportRoot.IsValid())
		{
			InputMode.SetWidgetToFocus(MainMenuViewportRoot);
		}
		InputMode.SetHideCursorDuringCapture(false);
		InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
		PlayerController->SetInputMode(InputMode);
		UGameplayStatics::SetGamePaused(World, true);
	}
	else
	{
		UGameplayStatics::SetGamePaused(World, false);
		PlayerController->ResetIgnoreMoveInput();
		PlayerController->ResetIgnoreLookInput();
		FInputModeGameOnly InputMode;
		PlayerController->SetInputMode(InputMode);
	}
}

void UWildBoundMainMenuSubsystem::RemoveMainMenuWidget()
{
	if (MainMenuViewportRoot.IsValid() && GEngine && GEngine->GameViewport)
	{
		GEngine->GameViewport->RemoveViewportWidgetContent(MainMenuViewportRoot.ToSharedRef());
	}

	MainMenuViewportRoot.Reset();
}
