#include "WildBoundOnboardingSubsystem.h"

#include "WildBoundDeathSubsystem.h"
#include "WildBoundEvidenceLogSubsystem.h"
#include "WildBoundMainMenuSubsystem.h"
#include "WildBoundPauseMenuSubsystem.h"
#include "WildBoundSaveSubsystem.h"
#include "../Crafting/WildBoundCraftingComponent.h"
#include "../Inventory/WildBoundInventoryComponent.h"
#include "../Player/WildBoundBackpackComponent.h"
#include "../Player/WildBoundInteractionComponent.h"
#include "../Survival/WildBoundInjuryComponent.h"
#include "../Survival/WildBoundRadiationComponent.h"
#include "../Survival/WildBoundSurvivalComponent.h"
#include "Engine/Engine.h"
#include "Engine/GameViewportClient.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "Styling/CoreStyle.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/SOverlay.h"
#include "Widgets/Text/STextBlock.h"

namespace
{
	const FName WaterItemId(TEXT("Water"));
	const FName SearchedContainerTag(TEXT("WBContainerSearched"));

	constexpr int32 NutritionHintFlag = 1 << 0;
	constexpr int32 RadiationHintFlag = 1 << 1;
	constexpr int32 InjuryHintFlag = 1 << 2;
	constexpr int32 CompletedStage = 6;
}

void UWildBoundOnboardingSubsystem::OnWorldBeginPlay(UWorld& InWorld)
{
	Super::OnWorldBeginPlay(InWorld);

	if (!InWorld.IsGameWorld())
	{
		return;
	}

	EnsureOnboardingWidget();
	InWorld.GetTimerManager().SetTimer(
		UpdateTimer,
		this,
		&UWildBoundOnboardingSubsystem::UpdateOnboarding,
		0.10f,
		true,
		0.25f);
}

void UWildBoundOnboardingSubsystem::Deinitialize()
{
	SetIntroInputLock(false);

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(UpdateTimer);
	}

	RemoveOnboardingWidget();
	Super::Deinitialize();
}

void UWildBoundOnboardingSubsystem::RestorePersistentState(int32 SavedProgressStage, int32 SavedHintFlags)
{
	ProgressStage = FMath::Clamp(SavedProgressStage, 0, CompletedStage);
	TutorialHintFlags = FMath::Max(0, SavedHintFlags);
	bIntroActive = false;
	TutorialToast.Reset();
	TutorialToastExpiresAt = -1.0f;

	UWorld* World = GetWorld();
	APlayerController* PlayerController = World ? World->GetFirstPlayerController() : nullptr;
	APawn* Pawn = PlayerController ? PlayerController->GetPawn() : nullptr;
	if (Pawn && ProgressStage >= 1 && ProgressStage < CompletedStage)
	{
		RunStartLocation = Pawn->GetActorLocation();
		bRunStartCaptured = true;
	}
	else
	{
		bRunStartCaptured = false;
	}

	SetIntroInputLock(false);
}

void UWildBoundOnboardingSubsystem::UpdateOnboarding()
{
	UWorld* World = GetWorld();
	APlayerController* PlayerController = World ? World->GetFirstPlayerController() : nullptr;
	APawn* Pawn = PlayerController ? PlayerController->GetPawn() : nullptr;
	UWildBoundSaveSubsystem* SaveSubsystem = World
		? World->GetSubsystem<UWildBoundSaveSubsystem>()
		: nullptr;

	if (!World || !PlayerController || !Pawn || !SaveSubsystem || !SaveSubsystem->HasStartedSession())
	{
		return;
	}

	if (ProgressStage == 0)
	{
		StartNewRunOnboarding();
	}

	if (bIntroActive)
	{
		const float IntroElapsed = World->GetTimeSeconds() - IntroStartedAt;
		SetIntroInputLock(IntroElapsed < IntroInputLockDuration);

		if (IntroElapsed >= IntroDuration)
		{
			bIntroActive = false;
			SetIntroInputLock(false);
		}
	}

	EvaluateObjectiveProgress();
	EvaluateContextualTutorials();

	if (TutorialToastExpiresAt >= 0.0f && World->GetTimeSeconds() >= TutorialToastExpiresAt)
	{
		TutorialToast.Reset();
		TutorialToastExpiresAt = -1.0f;
	}
}

void UWildBoundOnboardingSubsystem::StartNewRunOnboarding()
{
	UWorld* World = GetWorld();
	APlayerController* PlayerController = World ? World->GetFirstPlayerController() : nullptr;
	APawn* Pawn = PlayerController ? PlayerController->GetPawn() : nullptr;
	if (!World || !Pawn)
	{
		return;
	}

	RunStartLocation = Pawn->GetActorLocation();
	bRunStartCaptured = true;
	ProgressStage = 1;
	bIntroActive = true;
	IntroStartedAt = World->GetTimeSeconds();
	SetIntroInputLock(true);
}

void UWildBoundOnboardingSubsystem::AdvanceStage(int32 NewStage)
{
	if (NewStage <= ProgressStage)
	{
		return;
	}

	ProgressStage = FMath::Clamp(NewStage, 0, CompletedStage);

	if (ProgressStage == 2)
	{
		ShowTutorialToast(TEXT("SURVIVAL   |   Hunger and thirst weaken stamina before they begin damaging health."), 5.2f);
	}
	else if (ProgressStage == 3)
	{
		ShowTutorialToast(TEXT("SCAVENGING   |   Containers can hold tools, crafting materials, food, and medical supplies."), 4.8f);
	}
	else if (ProgressStage == 5)
	{
		ShowTutorialToast(TEXT("EVIDENCE   |   Inspect records with E. Recovered clues are stored permanently in the J evidence log."), 5.4f);
	}
	else if (ProgressStage >= CompletedStage)
	{
		ShowTutorialToast(TEXT("OBJECTIVE COMPLETE   |   The town is open to you now. Follow the evidence or scavenge at your own risk."), 6.0f);
	}
}

void UWildBoundOnboardingSubsystem::EvaluateObjectiveProgress()
{
	UWorld* World = GetWorld();
	APlayerController* PlayerController = World ? World->GetFirstPlayerController() : nullptr;
	APawn* Pawn = PlayerController ? PlayerController->GetPawn() : nullptr;
	if (!World || !Pawn)
	{
		return;
	}

	const UWildBoundInventoryComponent* Inventory = Pawn->FindComponentByClass<UWildBoundInventoryComponent>();
	const UWildBoundEvidenceLogSubsystem* EvidenceLog = World->GetSubsystem<UWildBoundEvidenceLogSubsystem>();

	if (ProgressStage == 1)
	{
		if (!bRunStartCaptured)
		{
			RunStartLocation = Pawn->GetActorLocation();
			bRunStartCaptured = true;
		}

		if (FVector::Dist2D(RunStartLocation, Pawn->GetActorLocation()) >= 450.0f)
		{
			AdvanceStage(2);
		}
	}

	if (ProgressStage == 2 && Inventory && Inventory->HasItem(WaterItemId, 1))
	{
		AdvanceStage(3);
	}

	if (ProgressStage == 3 && HasSearchedContainer())
	{
		AdvanceStage(4);
	}

	if (ProgressStage == 4 && bRunStartCaptured
		&& FVector::Dist2D(RunStartLocation, Pawn->GetActorLocation()) >= 3200.0f)
	{
		AdvanceStage(5);
	}

	if (ProgressStage == 5 && EvidenceLog && !EvidenceLog->GetEvidenceEntries().IsEmpty())
	{
		AdvanceStage(CompletedStage);
	}
}

void UWildBoundOnboardingSubsystem::EvaluateContextualTutorials()
{
	UWorld* World = GetWorld();
	APlayerController* PlayerController = World ? World->GetFirstPlayerController() : nullptr;
	APawn* Pawn = PlayerController ? PlayerController->GetPawn() : nullptr;
	if (!World || !Pawn || IsTutorialToastVisible())
	{
		return;
	}

	const UWildBoundSurvivalComponent* Survival = Pawn->FindComponentByClass<UWildBoundSurvivalComponent>();
	const UWildBoundRadiationComponent* Radiation = Pawn->FindComponentByClass<UWildBoundRadiationComponent>();
	const UWildBoundInjuryComponent* Injury = Pawn->FindComponentByClass<UWildBoundInjuryComponent>();

	if (!(TutorialHintFlags & NutritionHintFlag)
		&& Survival
		&& (Survival->GetHungerPercent() <= 0.45f || Survival->GetThirstPercent() <= 0.45f))
	{
		TutorialHintFlags |= NutritionHintFlag;
		ShowTutorialToast(TEXT("LOW NUTRITION   |   Use Food or Water from the hotbar. Hunger and thirst reduce movement and stamina efficiency."), 6.0f);
		return;
	}

	if (!(TutorialHintFlags & RadiationHintFlag)
		&& Radiation
		&& Radiation->CurrentExposure >= 8.0f)
	{
		TutorialHintFlags |= RadiationHintFlag;
		ShowTutorialToast(TEXT("RADIATION   |   Geiger activity means active exposure. Distance, a Filter Mask, and radiation treatment reduce the risk."), 6.0f);
		return;
	}

	if (!(TutorialHintFlags & InjuryHintFlag)
		&& Injury
		&& Injury->HasAnyInjury())
	{
		TutorialHintFlags |= InjuryHintFlag;
		ShowTutorialToast(TEXT("INJURY   |   Medical Supplies control bleeding and pain. Trauma Kits are required for fractures."), 6.0f);
	}
}

void UWildBoundOnboardingSubsystem::ShowTutorialToast(const FString& Text, float DurationSeconds)
{
	UWorld* World = GetWorld();
	TutorialToast = Text;
	TutorialToastExpiresAt = World
		? World->GetTimeSeconds() + FMath::Max(DurationSeconds, 0.5f)
		: -1.0f;
}

bool UWildBoundOnboardingSubsystem::HasSearchedContainer() const
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return false;
	}

	for (TActorIterator<AActor> It(World); It; ++It)
	{
		AActor* Actor = *It;
		if (Actor && Actor->ActorHasTag(SearchedContainerTag))
		{
			return true;
		}
	}

	return false;
}

FText UWildBoundOnboardingSubsystem::GetObjectiveTitle() const
{
	switch (ProgressStage)
	{
	case 1: return FText::FromString(TEXT("GET YOUR BEARINGS"));
	case 2: return FText::FromString(TEXT("SECURE WATER"));
	case 3: return FText::FromString(TEXT("SEARCH FOR SUPPLIES"));
	case 4: return FText::FromString(TEXT("PUSH INTO TOWN"));
	case 5: return FText::FromString(TEXT("FIND OUT WHAT HAPPENED"));
	default: return FText::GetEmpty();
	}
}

FText UWildBoundOnboardingSubsystem::GetObjectiveText() const
{
	switch (ProgressStage)
	{
	case 1:
		return FText::FromString(TEXT("Move around and get oriented. The town is ahead."));
	case 2:
		return FText::FromString(TEXT("Find drinkable water before you move deeper into the exclusion zone."));
	case 3:
		return FText::FromString(TEXT("Search a nearby container for useful supplies."));
	case 4:
		return FText::FromString(TEXT("Head deeper into town. Watch your vitals and listen to the Geiger counter."));
	case 5:
		return FText::FromString(TEXT("Inspect a field record or abandoned monitoring station. Someone knew about the radiation before the alert."));
	default:
		return FText::GetEmpty();
	}
}

FText UWildBoundOnboardingSubsystem::GetControlHintText() const
{
	switch (ProgressStage)
	{
	case 1: return FText::FromString(TEXT("WASD MOVE   |   MOUSE LOOK   |   SHIFT SPRINT"));
	case 2: return FText::FromString(TEXT("E INTERACT   |   TAB BACKPACK   |   1-3 HOTBAR"));
	case 3: return FText::FromString(TEXT("E SEARCH / TAKE   |   TAB MANAGE INVENTORY"));
	case 4: return FText::FromString(TEXT("SHIFT SPRINT   |   WATCH HUNGER / THIRST / STAMINA"));
	case 5: return FText::FromString(TEXT("E INSPECT   |   J EVIDENCE LOG"));
	default: return FText::GetEmpty();
	}
}

FText UWildBoundOnboardingSubsystem::GetTutorialToastText() const
{
	return FText::FromString(TutorialToast);
}

float UWildBoundOnboardingSubsystem::GetIntroOpacity() const
{
	UWorld* World = GetWorld();
	if (!bIntroActive || !World)
	{
		return 0.0f;
	}

	const float Elapsed = World->GetTimeSeconds() - IntroStartedAt;
	if (Elapsed < 0.35f)
	{
		return FMath::Clamp(Elapsed / 0.35f, 0.0f, 1.0f);
	}
	if (Elapsed > IntroDuration - 0.85f)
	{
		return FMath::Clamp((IntroDuration - Elapsed) / 0.85f, 0.0f, 1.0f);
	}
	return 1.0f;
}

bool UWildBoundOnboardingSubsystem::IsIntroVisible() const
{
	return bIntroActive && ShouldShowOnboardingUI();
}

bool UWildBoundOnboardingSubsystem::IsObjectiveVisible() const
{
	return ProgressStage >= 1
		&& ProgressStage < CompletedStage
		&& !bIntroActive
		&& ShouldShowOnboardingUI();
}

bool UWildBoundOnboardingSubsystem::IsTutorialToastVisible() const
{
	UWorld* World = GetWorld();
	return !TutorialToast.IsEmpty()
		&& World
		&& TutorialToastExpiresAt > World->GetTimeSeconds()
		&& ShouldShowOnboardingUI();
}

bool UWildBoundOnboardingSubsystem::ShouldShowOnboardingUI() const
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return false;
	}

	APlayerController* PlayerController = World->GetFirstPlayerController();
	APawn* Pawn = PlayerController ? PlayerController->GetPawn() : nullptr;
	const UWildBoundSaveSubsystem* SaveSubsystem = World->GetSubsystem<UWildBoundSaveSubsystem>();
	const UWildBoundMainMenuSubsystem* MainMenu = World->GetSubsystem<UWildBoundMainMenuSubsystem>();
	const UWildBoundPauseMenuSubsystem* PauseMenu = World->GetSubsystem<UWildBoundPauseMenuSubsystem>();
	const UWildBoundDeathSubsystem* Death = World->GetSubsystem<UWildBoundDeathSubsystem>();
	const UWildBoundEvidenceLogSubsystem* Evidence = World->GetSubsystem<UWildBoundEvidenceLogSubsystem>();
	const UWildBoundBackpackComponent* Backpack = Pawn ? Pawn->FindComponentByClass<UWildBoundBackpackComponent>() : nullptr;
	const UWildBoundCraftingComponent* Crafting = Pawn ? Pawn->FindComponentByClass<UWildBoundCraftingComponent>() : nullptr;
	const UWildBoundInteractionComponent* Interaction = Pawn ? Pawn->FindComponentByClass<UWildBoundInteractionComponent>() : nullptr;

	return SaveSubsystem
		&& SaveSubsystem->HasStartedSession()
		&& !(MainMenu && MainMenu->IsMainMenuOpen())
		&& !(PauseMenu && PauseMenu->IsPauseMenuOpen())
		&& !(Death && Death->IsGameOverOpen())
		&& !(Evidence && Evidence->IsEvidenceLogOpen())
		&& !(Backpack && Backpack->IsBackpackOpen())
		&& !(Crafting && Crafting->IsCraftingOpen())
		&& !(Interaction && (Interaction->IsLootWindowOpen() || Interaction->IsTreatmentInProgress()));
}

void UWildBoundOnboardingSubsystem::EnsureOnboardingWidget()
{
	if (OnboardingViewportRoot.IsValid() || !GEngine || !GEngine->GameViewport)
	{
		return;
	}

	const TWeakObjectPtr<UWildBoundOnboardingSubsystem> WeakOwner = this;

	TSharedPtr<SOverlay> Overlay;
	SAssignNew(Overlay, SOverlay)

	+ SOverlay::Slot()
	.HAlign(HAlign_Fill)
	.VAlign(VAlign_Fill)
	[
		SNew(SBorder)
		.Visibility_Lambda([WeakOwner]()
		{
			const UWildBoundOnboardingSubsystem* Owner = WeakOwner.Get();
			return Owner && Owner->IsIntroVisible()
				? EVisibility::HitTestInvisible
				: EVisibility::Collapsed;
		})
		.BorderBackgroundColor_Lambda([WeakOwner]()
		{
			const UWildBoundOnboardingSubsystem* Owner = WeakOwner.Get();
			const float Alpha = Owner ? Owner->GetIntroOpacity() : 0.0f;
			return FSlateColor(FLinearColor(0.005f, 0.008f, 0.007f, 0.92f * Alpha));
		})
		[
			SNew(SOverlay)
			+ SOverlay::Slot()
			.HAlign(HAlign_Center)
			.VAlign(VAlign_Center)
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center)
				[
					SNew(STextBlock)
					.Text(FText::FromString(TEXT("WILDBOUND")))
					.Font(FCoreStyle::GetDefaultFontStyle("Bold", 30))
					.ColorAndOpacity_Lambda([WeakOwner]()
					{
						const UWildBoundOnboardingSubsystem* Owner = WeakOwner.Get();
						return FSlateColor(FLinearColor(0.90f, 0.84f, 0.62f, Owner ? Owner->GetIntroOpacity() : 0.0f));
					})
				]
				+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center).Padding(0.0f, 6.0f, 0.0f, 0.0f)
				[
					SNew(STextBlock)
					.Text(FText::FromString(TEXT("EXCLUSION ZONE")))
					.Font(FCoreStyle::GetDefaultFontStyle("Bold", 10))
					.ColorAndOpacity_Lambda([WeakOwner]()
					{
						const UWildBoundOnboardingSubsystem* Owner = WeakOwner.Get();
						return FSlateColor(FLinearColor(0.52f, 0.59f, 0.51f, Owner ? Owner->GetIntroOpacity() : 0.0f));
					})
				]
				+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center).Padding(0.0f, 16.0f, 0.0f, 0.0f)
				[
					SNew(STextBlock)
					.Text(FText::FromString(TEXT("SURVIVE. SCAVENGE. FIND OUT WHAT HAPPENED BEFORE THE ALERT.")))
					.Font(FCoreStyle::GetDefaultFontStyle("Regular", 9))
					.ColorAndOpacity_Lambda([WeakOwner]()
					{
						const UWildBoundOnboardingSubsystem* Owner = WeakOwner.Get();
						return FSlateColor(FLinearColor(0.70f, 0.72f, 0.66f, Owner ? Owner->GetIntroOpacity() : 0.0f));
					})
				]
			]
		]
	]

	+ SOverlay::Slot()
	.HAlign(HAlign_Left)
	.VAlign(VAlign_Top)
	.Padding(FMargin(28.0f, 36.0f, 0.0f, 0.0f))
	[
		SNew(SBox)
		.WidthOverride(430.0f)
		[
			SNew(SBorder)
			.Visibility_Lambda([WeakOwner]()
			{
				const UWildBoundOnboardingSubsystem* Owner = WeakOwner.Get();
				return Owner && Owner->IsObjectiveVisible()
					? EVisibility::HitTestInvisible
					: EVisibility::Collapsed;
			})
			.Padding(FMargin(14.0f, 12.0f))
			.BorderBackgroundColor(FLinearColor(0.012f, 0.017f, 0.015f, 0.93f))
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot().AutoHeight()
				[
					SNew(STextBlock)
					.Text(FText::FromString(TEXT("CURRENT OBJECTIVE")))
					.Font(FCoreStyle::GetDefaultFontStyle("Bold", 8))
					.ColorAndOpacity(FLinearColor(0.47f, 0.55f, 0.46f, 1.0f))
				]
				+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 4.0f, 0.0f, 4.0f)
				[
					SNew(STextBlock)
					.Text_Lambda([WeakOwner]()
					{
						const UWildBoundOnboardingSubsystem* Owner = WeakOwner.Get();
						return Owner ? Owner->GetObjectiveTitle() : FText::GetEmpty();
					})
					.Font(FCoreStyle::GetDefaultFontStyle("Bold", 13))
					.ColorAndOpacity(FLinearColor(0.90f, 0.84f, 0.62f, 1.0f))
				]
				+ SVerticalBox::Slot().AutoHeight()
				[
					SNew(STextBlock)
					.Text_Lambda([WeakOwner]()
					{
						const UWildBoundOnboardingSubsystem* Owner = WeakOwner.Get();
						return Owner ? Owner->GetObjectiveText() : FText::GetEmpty();
					})
					.AutoWrapText(true)
					.Font(FCoreStyle::GetDefaultFontStyle("Regular", 9))
					.ColorAndOpacity(FLinearColor(0.77f, 0.80f, 0.74f, 1.0f))
				]
				+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 9.0f, 0.0f, 0.0f)
				[
					SNew(STextBlock)
					.Text_Lambda([WeakOwner]()
					{
						const UWildBoundOnboardingSubsystem* Owner = WeakOwner.Get();
						return Owner ? Owner->GetControlHintText() : FText::GetEmpty();
					})
					.Font(FCoreStyle::GetDefaultFontStyle("Bold", 8))
					.ColorAndOpacity(FLinearColor(0.53f, 0.61f, 0.54f, 1.0f))
				]
			]
		]
	]

	+ SOverlay::Slot()
	.HAlign(HAlign_Center)
	.VAlign(VAlign_Bottom)
	.Padding(FMargin(0.0f, 0.0f, 0.0f, 176.0f))
	[
		SNew(SBox)
		.WidthOverride(760.0f)
		[
			SNew(SBorder)
			.Visibility_Lambda([WeakOwner]()
			{
				const UWildBoundOnboardingSubsystem* Owner = WeakOwner.Get();
				return Owner && Owner->IsTutorialToastVisible()
					? EVisibility::HitTestInvisible
					: EVisibility::Collapsed;
			})
			.Padding(FMargin(16.0f, 10.0f))
			.BorderBackgroundColor(FLinearColor(0.032f, 0.040f, 0.032f, 0.94f))
			[
				SNew(STextBlock)
				.Text_Lambda([WeakOwner]()
				{
					const UWildBoundOnboardingSubsystem* Owner = WeakOwner.Get();
					return Owner ? Owner->GetTutorialToastText() : FText::GetEmpty();
				})
				.AutoWrapText(true)
				.Font(FCoreStyle::GetDefaultFontStyle("Bold", 9))
				.ColorAndOpacity(FLinearColor(0.82f, 0.84f, 0.75f, 1.0f))
				.Justification(ETextJustify::Center)
			]
		]
	];

	OnboardingViewportRoot = Overlay;
	GEngine->GameViewport->AddViewportWidgetContent(OnboardingViewportRoot.ToSharedRef(), 118);
}

void UWildBoundOnboardingSubsystem::RemoveOnboardingWidget()
{
	if (OnboardingViewportRoot.IsValid() && GEngine && GEngine->GameViewport)
	{
		GEngine->GameViewport->RemoveViewportWidgetContent(OnboardingViewportRoot.ToSharedRef());
	}

	OnboardingViewportRoot.Reset();
}

void UWildBoundOnboardingSubsystem::SetIntroInputLock(bool bLocked)
{
	UWorld* World = GetWorld();
	APlayerController* PlayerController = World ? World->GetFirstPlayerController() : nullptr;
	if (!PlayerController)
	{
		return;
	}

	APawn* Pawn = PlayerController->GetPawn();
	const UWildBoundMainMenuSubsystem* MainMenu = World ? World->GetSubsystem<UWildBoundMainMenuSubsystem>() : nullptr;
	const UWildBoundPauseMenuSubsystem* PauseMenu = World ? World->GetSubsystem<UWildBoundPauseMenuSubsystem>() : nullptr;
	const UWildBoundDeathSubsystem* Death = World ? World->GetSubsystem<UWildBoundDeathSubsystem>() : nullptr;
	const UWildBoundEvidenceLogSubsystem* Evidence = World ? World->GetSubsystem<UWildBoundEvidenceLogSubsystem>() : nullptr;
	const UWildBoundBackpackComponent* Backpack = Pawn ? Pawn->FindComponentByClass<UWildBoundBackpackComponent>() : nullptr;
	const UWildBoundCraftingComponent* Crafting = Pawn ? Pawn->FindComponentByClass<UWildBoundCraftingComponent>() : nullptr;
	const UWildBoundInteractionComponent* Interaction = Pawn ? Pawn->FindComponentByClass<UWildBoundInteractionComponent>() : nullptr;

	if (!bLocked && ((MainMenu && MainMenu->IsMainMenuOpen())
		|| (PauseMenu && PauseMenu->IsPauseMenuOpen())
		|| (Death && Death->IsGameOverOpen())
		|| (Evidence && Evidence->IsEvidenceLogOpen())
		|| (Backpack && Backpack->IsBackpackOpen())
		|| (Crafting && Crafting->IsCraftingOpen())
		|| (Interaction && (Interaction->IsLootWindowOpen() || Interaction->IsTreatmentInProgress()))))
	{
		return;
	}

	PlayerController->SetIgnoreMoveInput(bLocked);
	PlayerController->SetIgnoreLookInput(bLocked);
}
