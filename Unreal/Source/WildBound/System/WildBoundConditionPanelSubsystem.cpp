#include "WildBoundConditionPanelSubsystem.h"

#include "../Inventory/WildBoundInventoryComponent.h"
#include "../Player/WildBoundBackpackComponent.h"
#include "../Survival/WildBoundInjuryComponent.h"
#include "../Survival/WildBoundStatusEffectComponent.h"
#include "Engine/Engine.h"
#include "Engine/GameViewportClient.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
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
	const FName MedicalItemId(TEXT("MedicalSupplies"));
	const FName TraumaKitItemId(TEXT("TraumaKit"));
	const FName FoodItemId(TEXT("Food"));
	const FName WaterItemId(TEXT("Water"));
	const FName RadTreatmentItemId(TEXT("RadTreatment"));
	const FName FilterMaskItemId(TEXT("FilterMask"));

	FString GetSeverityLabel(EWildBoundStatusSeverity Severity)
	{
		switch (Severity)
		{
		case EWildBoundStatusSeverity::Critical: return TEXT("CRITICAL");
		case EWildBoundStatusSeverity::Warning: return TEXT("WARNING");
		case EWildBoundStatusSeverity::Positive: return TEXT("ACTIVE");
		case EWildBoundStatusSeverity::Notice:
		default: return TEXT("NOTICE");
		}
	}

	FString GetOwnedText(const UWildBoundInventoryComponent* Inventory, FName ItemId)
	{
		const int32 Count = Inventory ? Inventory->GetItemCount(ItemId) : 0;
		return Count > 0
			? FString::Printf(TEXT("OWNED x%d"), Count)
			: TEXT("NONE OWNED");
	}

	FString GetTreatmentText(const FWildBoundStatusEffect& Effect, const UWildBoundInventoryComponent* Inventory)
	{
		const FName Id = Effect.EffectId;

		if (Id == FName(TEXT("SevereBleeding")) || Id == FName(TEXT("Bleeding")))
		{
			return FString::Printf(
				TEXT("TREAT: MEDICAL SUPPLIES [%s]  /  TRAUMA KIT [%s]"),
				*GetOwnedText(Inventory, MedicalItemId),
				*GetOwnedText(Inventory, TraumaKitItemId));
		}
		if (Id == FName(TEXT("SevereFracture")) || Id == FName(TEXT("Fracture")))
		{
			return FString::Printf(TEXT("TREAT: TRAUMA KIT [%s]"), *GetOwnedText(Inventory, TraumaKitItemId));
		}
		if (Id == FName(TEXT("SeverePain")) || Id == FName(TEXT("Pain")))
		{
			return FString::Printf(
				TEXT("TREAT: MEDICAL SUPPLIES [%s]  /  TRAUMA KIT [%s - STRONGER]"),
				*GetOwnedText(Inventory, MedicalItemId),
				*GetOwnedText(Inventory, TraumaKitItemId));
		}
		if (Id == FName(TEXT("SevereInjury")) || Id == FName(TEXT("Injured")))
		{
			return FString::Printf(
				TEXT("TREAT: MEDICAL SUPPLIES [%s]  /  TRAUMA KIT [%s]"),
				*GetOwnedText(Inventory, MedicalItemId),
				*GetOwnedText(Inventory, TraumaKitItemId));
		}
		if (Id == FName(TEXT("Starving")) || Id == FName(TEXT("Hungry")))
		{
			return FString::Printf(TEXT("TREAT: FOOD [%s]"), *GetOwnedText(Inventory, FoodItemId));
		}
		if (Id == FName(TEXT("Dehydrated")) || Id == FName(TEXT("Thirsty")))
		{
			return FString::Printf(TEXT("TREAT: WATER [%s]"), *GetOwnedText(Inventory, WaterItemId));
		}
		if (Id == FName(TEXT("SevereRadiation")) || Id == FName(TEXT("RadiationSickness")))
		{
			return FString::Printf(TEXT("TREAT: RADIATION TREATMENT [%s]"), *GetOwnedText(Inventory, RadTreatmentItemId));
		}
		if (Id == FName(TEXT("RadiationExposure")))
		{
			return FString::Printf(
				TEXT("ACTION: LEAVE HOT ZONE  |  FILTER MASK [%s] REDUCES INTAKE  |  RAD TREATMENT [%s] REDUCES DOSE"),
				*GetOwnedText(Inventory, FilterMaskItemId),
				*GetOwnedText(Inventory, RadTreatmentItemId));
		}

		return TEXT("TREATMENT: MONITOR CONDITION");
	}

	FString GetInjuryMetric(const FWildBoundStatusEffect& Effect, const UWildBoundInjuryComponent* Injury)
	{
		if (!Injury)
		{
			return FString();
		}

		const FName Id = Effect.EffectId;
		if (Id == FName(TEXT("SevereBleeding")) || Id == FName(TEXT("Bleeding")))
		{
			return FString::Printf(TEXT("  |  BLEEDING %.0f%%"), Injury->BleedingSeverity * 100.0f);
		}
		if (Id == FName(TEXT("SevereFracture")) || Id == FName(TEXT("Fracture")))
		{
			return FString::Printf(TEXT("  |  FRACTURE %.0f%%"), Injury->FractureSeverity * 100.0f);
		}
		if (Id == FName(TEXT("SeverePain")) || Id == FName(TEXT("Pain")))
		{
			return FString::Printf(TEXT("  |  PAIN %.0f%%"), Injury->PainSeverity * 100.0f);
		}
		return FString();
	}

	FText BuildConditionText(
		const UWildBoundBackpackComponent* Backpack,
		const UWildBoundStatusEffectComponent* StatusEffects,
		const UWildBoundInjuryComponent* Injury)
	{
		if (!Backpack)
		{
			return FText::FromString(TEXT("CONDITION DATA UNAVAILABLE"));
		}

		const UWildBoundInventoryComponent* Inventory = Backpack->GetInventoryComponent();
		if (!StatusEffects)
		{
			return FText::FromString(TEXT("NO STATUS COMPONENT\nCondition tracking is unavailable."));
		}

		const TArray<FWildBoundStatusEffect> Effects = StatusEffects->GetActiveEffects();
		TArray<FString> Problems;
		TArray<FString> Support;

		for (const FWildBoundStatusEffect& Effect : Effects)
		{
			if (Effect.bBeneficial)
			{
				FString TimeText;
				if (Effect.RemainingSeconds > 0.0f)
				{
					TimeText = FString::Printf(TEXT("  |  %.0fs"), Effect.RemainingSeconds);
				}
				Support.Add(FString::Printf(
					TEXT("[ACTIVE] %s%s\n%s"),
					*Effect.DisplayName,
					*TimeText,
					*Effect.Detail));
				continue;
			}

			const FString Metric = GetInjuryMetric(Effect, Injury);
			Problems.Add(FString::Printf(
				TEXT("[%s] %s%s\n%s\n%s"),
				*GetSeverityLabel(Effect.Severity),
				*Effect.DisplayName,
				*Metric,
				*Effect.Detail,
				*GetTreatmentText(Effect, Inventory)));
		}

		FString Result;
		if (Problems.IsEmpty())
		{
			Result = TEXT("NO ACTIVE CONDITIONS\nVitals stable. No treatment required.");
		}
		else
		{
			Result = FString::Join(Problems, TEXT("\n\n"));
		}

		if (Injury && Injury->HasAnyInjury())
		{
			Result += FString::Printf(TEXT("\n\nTRAUMA PLAN\n%s"), *Injury->GetTreatmentRequirementText());
		}

		if (!Support.IsEmpty())
		{
			Result += TEXT("\n\nACTIVE SUPPORT\n");
			Result += FString::Join(Support, TEXT("\n\n"));
		}

		return FText::FromString(Result);
	}

	FText BuildConditionHeader(const UWildBoundStatusEffectComponent* StatusEffects)
	{
		if (!StatusEffects)
		{
			return FText::FromString(TEXT("CONDITION  /  UNKNOWN"));
		}

		const EWildBoundStatusSeverity Severity = StatusEffects->GetHighestSeverity();
		const TCHAR* State = TEXT("STABLE");
		if (Severity == EWildBoundStatusSeverity::Critical) State = TEXT("CRITICAL");
		else if (Severity == EWildBoundStatusSeverity::Warning) State = TEXT("ATTENTION");
		else if (Severity == EWildBoundStatusSeverity::Notice) State = TEXT("MONITOR");
		else if (Severity == EWildBoundStatusSeverity::Positive) State = TEXT("SUPPORTED");
		return FText::FromString(FString::Printf(TEXT("CONDITION  /  %s"), State));
	}

	FSlateColor GetConditionColor(const UWildBoundStatusEffectComponent* StatusEffects)
	{
		if (!StatusEffects)
		{
			return FSlateColor(FLinearColor(0.58f, 0.62f, 0.58f, 1.0f));
		}

		switch (StatusEffects->GetHighestSeverity())
		{
		case EWildBoundStatusSeverity::Critical:
			return FSlateColor(FLinearColor(1.0f, 0.38f, 0.30f, 1.0f));
		case EWildBoundStatusSeverity::Warning:
			return FSlateColor(FLinearColor(0.96f, 0.70f, 0.28f, 1.0f));
		case EWildBoundStatusSeverity::Positive:
			return FSlateColor(FLinearColor(0.46f, 0.78f, 0.52f, 1.0f));
		case EWildBoundStatusSeverity::Notice:
		default:
			return FSlateColor(FLinearColor(0.72f, 0.76f, 0.70f, 1.0f));
		}
	}
}

void UWildBoundConditionPanelSubsystem::OnWorldBeginPlay(UWorld& InWorld)
{
	Super::OnWorldBeginPlay(InWorld);

	if (!InWorld.IsGameWorld())
	{
		return;
	}

	EnsureConditionPanel();
	InWorld.GetTimerManager().SetTimer(
		ConditionPanelSetupTimer,
		this,
		&UWildBoundConditionPanelSubsystem::EnsureConditionPanel,
		0.5f,
		true,
		0.10f);
}

void UWildBoundConditionPanelSubsystem::Deinitialize()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(ConditionPanelSetupTimer);
	}
	RemoveConditionPanel();
	Super::Deinitialize();
}

void UWildBoundConditionPanelSubsystem::EnsureConditionPanel()
{
	if (ConditionViewportRoot.IsValid())
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
	if (!Pawn)
	{
		return;
	}

	UWildBoundBackpackComponent* Backpack = Pawn->FindComponentByClass<UWildBoundBackpackComponent>();
	if (!Backpack)
	{
		return;
	}

	BackpackComponent = Backpack;
	const TWeakObjectPtr<UWildBoundBackpackComponent> WeakBackpack = Backpack;
	const TWeakObjectPtr<UWildBoundStatusEffectComponent> WeakStatus = Pawn->FindComponentByClass<UWildBoundStatusEffectComponent>();
	const TWeakObjectPtr<UWildBoundInjuryComponent> WeakInjury = Pawn->FindComponentByClass<UWildBoundInjuryComponent>();

	TSharedPtr<SOverlay> Overlay;
	SAssignNew(Overlay, SOverlay)
	+ SOverlay::Slot()
	.HAlign(HAlign_Right)
	.VAlign(VAlign_Center)
	.Padding(FMargin(0.0f, 0.0f, 26.0f, 0.0f))
	[
		SNew(SBox)
		.WidthOverride(330.0f)
		.HeightOverride(560.0f)
		.Visibility_Lambda([WeakBackpack]()
		{
			const UWildBoundBackpackComponent* BackpackComponent = WeakBackpack.Get();
			return BackpackComponent && BackpackComponent->IsBackpackOpen()
				? EVisibility::Visible
				: EVisibility::Collapsed;
		})
		[
			SNew(SBorder)
			.Padding(FMargin(16.0f, 14.0f))
			.BorderBackgroundColor(FLinearColor(0.012f, 0.017f, 0.015f, 0.995f))
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot().AutoHeight()
				[
					SNew(STextBlock)
					.Text_Lambda([WeakStatus]()
					{
						return BuildConditionHeader(WeakStatus.Get());
					})
					.Font(FCoreStyle::GetDefaultFontStyle("Bold", 14))
					.ColorAndOpacity_Lambda([WeakStatus]()
					{
						return GetConditionColor(WeakStatus.Get());
					})
				]
				+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 4.0f, 0.0f, 8.0f)
				[
					SNew(STextBlock)
					.Text(FText::FromString(TEXT("DIAGNOSIS  /  EFFECTS  /  REQUIRED TREATMENT")))
					.Font(FCoreStyle::GetDefaultFontStyle("Bold", 7))
					.ColorAndOpacity(FLinearColor(0.47f, 0.55f, 0.49f, 1.0f))
				]
				+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 0.0f, 0.0f, 10.0f)
				[
					SNew(SSeparator)
				]
				+ SVerticalBox::Slot().FillHeight(1.0f)
				[
					SNew(SScrollBox)
					+ SScrollBox::Slot()
					[
						SNew(STextBlock)
						.Text_Lambda([WeakBackpack, WeakStatus, WeakInjury]()
						{
							return BuildConditionText(WeakBackpack.Get(), WeakStatus.Get(), WeakInjury.Get());
						})
						.AutoWrapText(true)
						.Font(FCoreStyle::GetDefaultFontStyle("Regular", 9))
						.ColorAndOpacity(FLinearColor(0.84f, 0.87f, 0.81f, 1.0f))
					]
				]
				+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 10.0f, 0.0f, 0.0f)
				[
					SNew(SBorder)
					.Padding(FMargin(8.0f, 6.0f))
					.BorderBackgroundColor(FLinearColor(0.035f, 0.045f, 0.039f, 0.98f))
					[
						SNew(STextBlock)
						.Text(FText::FromString(TEXT("Treatments are used from your survival hotbar.")))
						.Font(FCoreStyle::GetDefaultFontStyle("Regular", 8))
						.ColorAndOpacity(FLinearColor(0.62f, 0.67f, 0.61f, 1.0f))
					]
				]
			]
		]
	];

	ConditionViewportRoot = Overlay;
	GEngine->GameViewport->AddViewportWidgetContent(ConditionViewportRoot.ToSharedRef(), 131);
	World->GetTimerManager().ClearTimer(ConditionPanelSetupTimer);
	UE_LOG(LogTemp, Log, TEXT("WildBound condition panel: backpack medical readout attached."));
}

void UWildBoundConditionPanelSubsystem::RemoveConditionPanel()
{
	if (ConditionViewportRoot.IsValid() && GEngine && GEngine->GameViewport)
	{
		GEngine->GameViewport->RemoveViewportWidgetContent(ConditionViewportRoot.ToSharedRef());
	}
	ConditionViewportRoot.Reset();
	BackpackComponent.Reset();
}
