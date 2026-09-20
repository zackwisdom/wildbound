#include "WildBoundEvidenceLogSubsystem.h"

#include "../Crafting/WildBoundCraftingComponent.h"
#include "../Player/WildBoundBackpackComponent.h"
#include "../Player/WildBoundInteractionComponent.h"
#include "Engine/Engine.h"
#include "Engine/GameViewportClient.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "InputCoreTypes.h"
#include "Styling/CoreStyle.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Layout/SSeparator.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/SOverlay.h"
#include "Widgets/Text/STextBlock.h"

void UWildBoundEvidenceLogSubsystem::OnWorldBeginPlay(UWorld& InWorld)
{
	Super::OnWorldBeginPlay(InWorld);

	if (!InWorld.IsGameWorld())
	{
		return;
	}

	EnsureEvidenceWidget();
	InWorld.GetTimerManager().SetTimer(
		EvidenceInputTimer,
		this,
		&UWildBoundEvidenceLogSubsystem::UpdateInput,
		0.05f,
		true,
		0.15f);
}

void UWildBoundEvidenceLogSubsystem::Deinitialize()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(EvidenceInputTimer);
	}

	SetEvidenceLogOpen(false);
	RemoveEvidenceWidget();
	EvidenceEntries.Reset();
	Super::Deinitialize();
}

bool UWildBoundEvidenceLogSubsystem::RecordEvidence(
	FName EvidenceId,
	const FString& Title,
	const FString& Source,
	const FString& Body)
{
	if (EvidenceId.IsNone() || Title.IsEmpty() || Body.IsEmpty() || HasEvidence(EvidenceId))
	{
		return false;
	}

	FWildBoundEvidenceEntry Entry;
	Entry.EvidenceId = EvidenceId;
	Entry.Title = Title;
	Entry.Source = Source;
	Entry.Body = Body;
	Entry.DiscoveredAtSeconds = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0f;
	EvidenceEntries.Add(MoveTemp(Entry));

	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(
			91420,
			3.4f,
			FColor(215, 196, 130),
			TEXT("EVIDENCE LOG UPDATED   |   J TO REVIEW"));
	}

	return true;
}

bool UWildBoundEvidenceLogSubsystem::HasEvidence(FName EvidenceId) const
{
	return EvidenceEntries.ContainsByPredicate([EvidenceId](const FWildBoundEvidenceEntry& Entry)
	{
		return Entry.EvidenceId == EvidenceId;
	});
}

void UWildBoundEvidenceLogSubsystem::UpdateInput()
{
	UWorld* World = GetWorld();
	APlayerController* PlayerController = World ? World->GetFirstPlayerController() : nullptr;
	if (!PlayerController)
	{
		return;
	}

	EnsureEvidenceWidget();

	if (bEvidenceLogOpen)
	{
		if (PlayerController->WasInputKeyJustPressed(EKeys::Escape)
			|| PlayerController->WasInputKeyJustPressed(EKeys::J))
		{
			SetEvidenceLogOpen(false);
		}
		return;
	}

	if (PlayerController->WasInputKeyJustPressed(EKeys::J) && CanOpenEvidenceLog())
	{
		SetEvidenceLogOpen(true);
	}
}

bool UWildBoundEvidenceLogSubsystem::CanOpenEvidenceLog() const
{
	UWorld* World = GetWorld();
	APlayerController* PlayerController = World ? World->GetFirstPlayerController() : nullptr;
	APawn* Pawn = PlayerController ? PlayerController->GetPawn() : nullptr;
	if (!Pawn)
	{
		return false;
	}

	const UWildBoundBackpackComponent* Backpack = Pawn->FindComponentByClass<UWildBoundBackpackComponent>();
	const UWildBoundCraftingComponent* Crafting = Pawn->FindComponentByClass<UWildBoundCraftingComponent>();
	const UWildBoundInteractionComponent* Interaction = Pawn->FindComponentByClass<UWildBoundInteractionComponent>();

	if ((Backpack && Backpack->IsBackpackOpen())
		|| (Crafting && Crafting->IsCraftingOpen())
		|| (Interaction && (Interaction->IsLootWindowOpen() || Interaction->IsTreatmentInProgress())))
	{
		return false;
	}

	return true;
}

void UWildBoundEvidenceLogSubsystem::EnsureEvidenceWidget()
{
	if (EvidenceViewportRoot.IsValid() || !GEngine || !GEngine->GameViewport)
	{
		return;
	}

	TSharedPtr<SOverlay> Overlay;
	SAssignNew(Overlay, SOverlay)
	+ SOverlay::Slot()
	.HAlign(HAlign_Fill)
	.VAlign(VAlign_Fill)
	[
		SNew(SBorder)
		.BorderBackgroundColor(FLinearColor(0.0f, 0.0f, 0.0f, 0.62f))
		.Visibility_Lambda([this]()
		{
			return bEvidenceLogOpen ? EVisibility::Visible : EVisibility::Collapsed;
		})
		[
			SNew(SOverlay)
			+ SOverlay::Slot()
			.HAlign(HAlign_Center)
			.VAlign(VAlign_Center)
			.Padding(FMargin(28.0f))
			[
				SNew(SBox)
				.WidthOverride(920.0f)
				.HeightOverride(650.0f)
				[
					SNew(SBorder)
					.Padding(FMargin(24.0f, 20.0f))
					.BorderBackgroundColor(FLinearColor(0.014f, 0.018f, 0.016f, 0.995f))
					[
						SNew(SVerticalBox)
						+ SVerticalBox::Slot().AutoHeight()
						[
							SNew(STextBlock)
							.Text_Lambda([this]()
							{
								return FText::FromString(FString::Printf(
									TEXT("EVIDENCE LOG   /   %d RECORD%s"),
									EvidenceEntries.Num(),
									EvidenceEntries.Num() == 1 ? TEXT("") : TEXT("S")));
							})
							.Font(FCoreStyle::GetDefaultFontStyle("Bold", 18))
							.ColorAndOpacity(FLinearColor(0.88f, 0.82f, 0.60f, 1.0f))
						]
						+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 3.0f, 0.0f, 10.0f)
						[
							SNew(STextBlock)
							.Text(FText::FromString(TEXT("RECOVERED DOCUMENTS / FIELD RECORDS / INCIDENT TIMELINE")))
							.Font(FCoreStyle::GetDefaultFontStyle("Bold", 8))
							.ColorAndOpacity(FLinearColor(0.47f, 0.53f, 0.47f, 1.0f))
						]
						+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 0.0f, 0.0f, 12.0f)
						[
							SNew(SSeparator)
						]
						+ SVerticalBox::Slot().FillHeight(1.0f)
						[
							SNew(SScrollBox)
							+ SScrollBox::Slot()
							[
								SNew(STextBlock)
								.Text_Lambda([this]()
								{
									return BuildEvidenceText();
								})
								.AutoWrapText(true)
								.Font(FCoreStyle::GetDefaultFontStyle("Regular", 11))
								.ColorAndOpacity(FLinearColor(0.82f, 0.85f, 0.79f, 1.0f))
							]
						]
						+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 12.0f, 0.0f, 0.0f)
						[
							SNew(SBorder)
							.Padding(FMargin(10.0f, 7.0f))
							.BorderBackgroundColor(FLinearColor(0.035f, 0.043f, 0.036f, 0.98f))
							[
								SNew(STextBlock)
								.Text(FText::FromString(TEXT("J / ESC CLOSE   |   RECORDS REMAIN AVAILABLE FOR THIS RUN")))
								.Font(FCoreStyle::GetDefaultFontStyle("Bold", 8))
								.ColorAndOpacity(FLinearColor(0.61f, 0.67f, 0.58f, 1.0f))
								.Justification(ETextJustify::Center)
							]
						]
					]
				]
			]
		]
	];

	EvidenceViewportRoot = Overlay;
	GEngine->GameViewport->AddViewportWidgetContent(EvidenceViewportRoot.ToSharedRef(), 145);
}

void UWildBoundEvidenceLogSubsystem::SetEvidenceLogOpen(bool bOpen)
{
	if (bEvidenceLogOpen == bOpen)
	{
		return;
	}

	if (bOpen && !CanOpenEvidenceLog())
	{
		return;
	}

	bEvidenceLogOpen = bOpen;

	UWorld* World = GetWorld();
	APlayerController* PlayerController = World ? World->GetFirstPlayerController() : nullptr;
	if (!PlayerController)
	{
		return;
	}

	PlayerController->SetIgnoreMoveInput(bOpen);
	PlayerController->SetIgnoreLookInput(bOpen);
	PlayerController->bShowMouseCursor = bOpen;

	if (bOpen)
	{
		FInputModeGameAndUI InputMode;
		InputMode.SetHideCursorDuringCapture(false);
		InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
		PlayerController->SetInputMode(InputMode);
	}
	else
	{
		FInputModeGameOnly InputMode;
		PlayerController->SetInputMode(InputMode);
	}
}

FText UWildBoundEvidenceLogSubsystem::BuildEvidenceText() const
{
	if (EvidenceEntries.IsEmpty())
	{
		return FText::FromString(TEXT(
			"NO EVIDENCE RECORDED\n\n"
			"Inspect field documents, monitoring equipment, and incident records to preserve them here."));
	}

	TArray<FString> Sections;
	for (int32 Index = 0; Index < EvidenceEntries.Num(); ++Index)
	{
		const FWildBoundEvidenceEntry& Entry = EvidenceEntries[Index];
		const int32 Minutes = FMath::FloorToInt(Entry.DiscoveredAtSeconds / 60.0f);
		const int32 Seconds = FMath::FloorToInt(FMath::Fmod(Entry.DiscoveredAtSeconds, 60.0f));

		Sections.Add(FString::Printf(
			TEXT("%02d  /  %s\nSOURCE: %s   |   RECOVERED %02d:%02d\n\n%s"),
			Index + 1,
			*Entry.Title,
			*Entry.Source,
			Minutes,
			Seconds,
			*Entry.Body));
	}

	return FText::FromString(FString::Join(Sections, TEXT("\n\n----------------------------------------\n\n")));
}

void UWildBoundEvidenceLogSubsystem::RemoveEvidenceWidget()
{
	if (EvidenceViewportRoot.IsValid() && GEngine && GEngine->GameViewport)
	{
		GEngine->GameViewport->RemoveViewportWidgetContent(EvidenceViewportRoot.ToSharedRef());
	}

	EvidenceViewportRoot.Reset();
}
