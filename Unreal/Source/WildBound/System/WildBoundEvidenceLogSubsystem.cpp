#include "WildBoundEvidenceLogSubsystem.h"

#include "../Building/WildBoundBuildingSubsystem.h"
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

namespace
{
	const FName SectorC17EvidenceId(TEXT("SectorC17Survey"));
	const FName ClinicEvidenceId(TEXT("ClinicIntakeLog"));
	const FName WarehouseEvidenceId(TEXT("MunicipalTransferManifest"));
	const FName DrainageEvidenceId(TEXT("CivilDefenseMonitor04"));
	const FName TreatmentEvidenceId(TEXT("WaterAuthorityDirective"));

	constexpr int32 TotalMysteryConclusions = 4;
}

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
	UnlockedConclusions.Reset();
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
	EvaluateMysteryProgress();

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

void UWildBoundEvidenceLogSubsystem::RestorePersistentEvidence(
	const TArray<FWildBoundEvidenceEntry>& SavedEntries)
{
	EvidenceEntries.Reset();
	UnlockedConclusions.Reset();

	for (const FWildBoundEvidenceEntry& SavedEntry : SavedEntries)
	{
		if (SavedEntry.EvidenceId.IsNone()
			|| SavedEntry.Title.IsEmpty()
			|| SavedEntry.Body.IsEmpty()
			|| HasEvidence(SavedEntry.EvidenceId))
		{
			continue;
		}

		EvidenceEntries.Add(SavedEntry);
	}

	EvaluateMysteryProgress();
}

void UWildBoundEvidenceLogSubsystem::EvaluateMysteryProgress()
{
	const bool bHasC17 = HasEvidence(SectorC17EvidenceId);
	const bool bHasClinic = HasEvidence(ClinicEvidenceId);
	const bool bHasWarehouse = HasEvidence(WarehouseEvidenceId);
	const bool bHasDrainage = HasEvidence(DrainageEvidenceId);
	const bool bHasTreatment = HasEvidence(TreatmentEvidenceId);

	if (bHasClinic && bHasDrainage)
	{
		UnlockConclusion(
			FName(TEXT("PredetonationExposure")),
			TEXT("PRE-DETONATION EXPOSURE CONFIRMED"),
			TEXT("Independent clinic and Civil Defense records both show abnormal radiation before the 04:47 detonation alert. The detonation cannot explain the earliest documented exposure."));
	}

	if (bHasC17 && bHasWarehouse)
	{
		UnlockConclusion(
			FName(TEXT("SampleTransferBeforeAlert")),
			TEXT("SAMPLES MOVED BEFORE THE PUBLIC ALERT"),
			TEXT("Sector C-17 recorded three environmental samples leaving the area, and the municipal manifest places their transfer at 03:52. Someone was already collecting and routing contamination evidence before the emergency declaration."));
	}

	if (bHasDrainage && bHasTreatment)
	{
		UnlockConclusion(
			FName(TEXT("WarningSuppressed")),
			TEXT("THE WARNING NETWORK WAS SUPPRESSED"),
			TEXT("Civil Defense Monitor 04 was disconnected remotely at 04:31. Three minutes later, Water Authority personnel were ordered not to broadcast the contamination alarm. The shutdown was coordinated, not an equipment failure."));
	}

	if (bHasC17 && bHasClinic && bHasWarehouse && bHasDrainage && bHasTreatment)
	{
		UnlockConclusion(
			FName(TEXT("CoordinatedPredetonationResponse")),
			TEXT("A COORDINATED RESPONSE BEGAN BEFORE THE DETONATION"),
			TEXT("Multiple agencies were measuring exposure, moving samples, treating exposed civilians, and suppressing warnings before the detonation alert. The evidence establishes a pre-existing radiological incident and an organized response, but does not yet identify the receiving authority or the original source."));
	}
}

bool UWildBoundEvidenceLogSubsystem::UnlockConclusion(
	FName ConclusionId,
	const FString& Title,
	const FString& Summary)
{
	if (ConclusionId.IsNone()
		|| UnlockedConclusions.ContainsByPredicate([ConclusionId](const FWildBoundMysteryConclusion& Conclusion)
		{
			return Conclusion.ConclusionId == ConclusionId;
		}))
	{
		return false;
	}

	FWildBoundMysteryConclusion Conclusion;
	Conclusion.ConclusionId = ConclusionId;
	Conclusion.Title = Title;
	Conclusion.Summary = Summary;
	UnlockedConclusions.Add(MoveTemp(Conclusion));

	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(
			91421,
			4.6f,
			FColor(232, 202, 112),
			FString::Printf(TEXT("CASE ANALYSIS UPDATED   |   %s"), *Title));
	}

	return true;
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
	const UWildBoundBuildingSubsystem* Building = World
		? World->GetSubsystem<UWildBoundBuildingSubsystem>()
		: nullptr;

	if ((Building && Building->IsPlacementActive())
		|| (Backpack && Backpack->IsBackpackOpen())
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
									TEXT("EVIDENCE LOG   /   %d RECORD%s   /   %d OF %d CONCLUSIONS"),
									EvidenceEntries.Num(),
									EvidenceEntries.Num() == 1 ? TEXT("") : TEXT("S"),
									UnlockedConclusions.Num(),
									TotalMysteryConclusions));
							})
							.Font(FCoreStyle::GetDefaultFontStyle("Bold", 18))
							.ColorAndOpacity(FLinearColor(0.88f, 0.82f, 0.60f, 1.0f))
						]
						+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 3.0f, 0.0f, 10.0f)
						[
							SNew(STextBlock)
							.Text(FText::FromString(TEXT("CASE ANALYSIS / INCIDENT TIMELINE / RECOVERED RECORDS")))
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

	TArray<FString> RecordSections;
	for (int32 Index = 0; Index < EvidenceEntries.Num(); ++Index)
	{
		const FWildBoundEvidenceEntry& Entry = EvidenceEntries[Index];
		const int32 Minutes = FMath::FloorToInt(Entry.DiscoveredAtSeconds / 60.0f);
		const int32 Seconds = FMath::FloorToInt(FMath::Fmod(Entry.DiscoveredAtSeconds, 60.0f));

		RecordSections.Add(FString::Printf(
			TEXT("%02d  /  %s\nSOURCE: %s   |   RECOVERED %02d:%02d\n\n%s"),
			Index + 1,
			*Entry.Title,
			*Entry.Source,
			Minutes,
			Seconds,
			*Entry.Body));
	}

	const FString Divider = TEXT("\n\n========================================\n\n");
	FString Result = TEXT("CASE ANALYSIS\n");
	Result += BuildCaseAnalysisText();
	Result += Divider;
	Result += TEXT("INCIDENT TIMELINE\n");
	Result += BuildTimelineText();
	Result += Divider;
	Result += TEXT("RECOVERED RECORDS\n\n");
	Result += FString::Join(RecordSections, TEXT("\n\n----------------------------------------\n\n"));
	return FText::FromString(Result);
}

FString UWildBoundEvidenceLogSubsystem::BuildCaseAnalysisText() const
{
	if (UnlockedConclusions.IsEmpty())
	{
		return FString::Printf(
			TEXT("0 / %d conclusions established.\nRecover independent records that corroborate one another before drawing a conclusion."),
			TotalMysteryConclusions);
	}

	TArray<FString> Sections;
	for (int32 Index = 0; Index < UnlockedConclusions.Num(); ++Index)
	{
		const FWildBoundMysteryConclusion& Conclusion = UnlockedConclusions[Index];
		Sections.Add(FString::Printf(
			TEXT("[%02d] %s\n%s"),
			Index + 1,
			*Conclusion.Title,
			*Conclusion.Summary));
	}

	return FString::Printf(
		TEXT("%d / %d conclusions established.\n\n%s"),
		UnlockedConclusions.Num(),
		TotalMysteryConclusions,
		*FString::Join(Sections, TEXT("\n\n")));
}

FString UWildBoundEvidenceLogSubsystem::BuildTimelineText() const
{
	TArray<FString> Events;

	if (HasEvidence(WarehouseEvidenceId))
	{
		Events.Add(TEXT("03:52  |  Municipal warehouse receives three sealed environmental samples from Sector C-17."));
	}
	if (HasEvidence(DrainageEvidenceId))
	{
		Events.Add(TEXT("04:09  |  Civil Defense Monitor 04 reports normal baseline."));
		Events.Add(TEXT("04:13  |  Monitor 04 rises to 2.4x baseline."));
	}
	if (HasEvidence(ClinicEvidenceId))
	{
		Events.Add(TEXT("04:18  |  Clinic records first civilians reporting metallic taste and nausea."));
		Events.Add(TEXT("04:21  |  Clinic portable meter reads 3.8x baseline."));
	}
	if (HasEvidence(DrainageEvidenceId))
	{
		Events.Add(TEXT("04:26  |  Monitor 04 reaches 4.1x baseline."));
		Events.Add(TEXT("04:31  |  Monitor 04 is disconnected remotely before the alert network activates."));
	}
	if (HasEvidence(TreatmentEvidenceId))
	{
		Events.Add(TEXT("04:34  |  Water Authority directive orders personnel not to broadcast the contamination alarm."));
	}
	if (HasEvidence(ClinicEvidenceId) || HasEvidence(TreatmentEvidenceId))
	{
		Events.Add(TEXT("04:47  |  Detonation alert is received."));
	}
	if (HasEvidence(SectorC17EvidenceId))
	{
		Events.Add(TEXT("UNDATED |  Sector C-17 survey confirms elevated background radiation before the detonation alert and records three samples transferred off-site."));
	}

	return Events.IsEmpty()
		? TEXT("No timestamped incident events recovered yet.")
		: FString::Join(Events, TEXT("\n"));
}

void UWildBoundEvidenceLogSubsystem::RemoveEvidenceWidget()
{
	if (EvidenceViewportRoot.IsValid() && GEngine && GEngine->GameViewport)
	{
		GEngine->GameViewport->RemoveViewportWidgetContent(EvidenceViewportRoot.ToSharedRef());
	}

	EvidenceViewportRoot.Reset();
}
