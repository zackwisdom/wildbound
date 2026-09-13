#include "WildBoundCraftingComponent.h"

#include "../Inventory/WildBoundInventoryComponent.h"
#include "../Player/WildBoundBackpackComponent.h"
#include "../UI/SWildBoundCraftingWidget.h"
#include "Engine/Engine.h"
#include "Engine/GameViewportClient.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "InputCoreTypes.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundWaveProcedural.h"
#include "Widgets/SOverlay.h"

namespace
{
	const FName WorkbenchTag(TEXT("WBWorkbench"));
	const FName ReinforcedBackpackItemId(TEXT("ReinforcedBackpack"));
	const FName FilterMaskItemId(TEXT("FilterMask"));
	const FName CanteenItemId(TEXT("Canteen"));
	const FName UtilityBeltItemId(TEXT("UtilityBelt"));

	constexpr int32 CraftSampleRate = 22050;
	constexpr float CraftSoundDuration = 0.24f;

	FWildBoundCraftingIngredient Ingredient(const TCHAR* ItemId, int32 Quantity)
	{
		FWildBoundCraftingIngredient Result;
		Result.ItemId = FName(ItemId);
		Result.Quantity = Quantity;
		return Result;
	}

	bool IsUniqueGearItem(const FName& ItemId)
	{
		return ItemId == ReinforcedBackpackItemId
			|| ItemId == FilterMaskItemId
			|| ItemId == CanteenItemId
			|| ItemId == UtilityBeltItemId;
	}

	USoundWaveProcedural* BuildCraftCompletionWave(UObject* Outer, int32 RarityTier, bool bWorkbench)
	{
		USoundWaveProcedural* Wave = NewObject<USoundWaveProcedural>(Outer);
		if (!Wave)
		{
			return nullptr;
		}

		Wave->NumChannels = 1;
		Wave->SetSampleRate(CraftSampleRate);
		Wave->Duration = CraftSoundDuration;
		Wave->bLooping = false;
		Wave->SoundGroup = SOUNDGROUP_Default;

		const int32 Tier = FMath::Clamp(RarityTier, 0, 3);
		const int32 SampleCount = FMath::RoundToInt(CraftSampleRate * CraftSoundDuration);
		TArray<int16> Samples;
		Samples.SetNumZeroed(SampleCount);

		const float BaseFrequency = bWorkbench
			? (420.0f + static_cast<float>(Tier) * 72.0f)
			: (560.0f + static_cast<float>(Tier) * 88.0f);
		const float AccentFrequency = BaseFrequency * 1.48f;
		const float HighFrequency = BaseFrequency * 2.05f;
		const float Gain = 0.36f + static_cast<float>(Tier) * 0.055f;

		for (int32 SampleIndex = 0; SampleIndex < SampleCount; ++SampleIndex)
		{
			const float TimeSeconds = static_cast<float>(SampleIndex) / static_cast<float>(CraftSampleRate);
			const float NormalizedTime = TimeSeconds / CraftSoundDuration;
			const float Envelope = FMath::Pow(FMath::Clamp(1.0f - NormalizedTime, 0.0f, 1.0f), 1.8f);
			const float ToneA = FMath::Sin(2.0f * PI * BaseFrequency * TimeSeconds);
			const float ToneB = FMath::Sin(2.0f * PI * AccentFrequency * TimeSeconds);
			const float ToneC = FMath::Sin(2.0f * PI * HighFrequency * TimeSeconds);
			const float ClickEnvelope = FMath::Clamp(1.0f - NormalizedTime * 8.0f, 0.0f, 1.0f);
			const float Click = FMath::Sin(2.0f * PI * 1450.0f * TimeSeconds) * ClickEnvelope;

			float Signal = ToneA * 0.50f + ToneB * 0.24f + ToneC * 0.10f + Click * 0.18f;
			if (Tier >= 2 && NormalizedTime > 0.34f && NormalizedTime < 0.68f)
			{
				Signal += FMath::Sin(2.0f * PI * AccentFrequency * 1.42f * TimeSeconds) * 0.13f;
			}

			const int32 PCM = FMath::Clamp(
				FMath::RoundToInt(Signal * Envelope * Gain * 32767.0f),
				-32768,
				32767);
			Samples[SampleIndex] = static_cast<int16>(PCM);
		}

		Wave->QueueAudio(
			reinterpret_cast<const uint8*>(Samples.GetData()),
			Samples.Num() * sizeof(int16));
		return Wave;
	}
}

UWildBoundCraftingComponent::UWildBoundCraftingComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = true;
	PrimaryComponentTick.TickGroup = TG_PostUpdateWork;
}

void UWildBoundCraftingComponent::BeginPlay()
{
	Super::BeginPlay();
	InventoryComponent = GetOwner()
		? GetOwner()->FindComponentByClass<UWildBoundInventoryComponent>()
		: nullptr;
	bWorkbenchMode = false;
	BuildRecipesForCurrentMode();
	EnsureCraftingWidget();
}

void UWildBoundCraftingComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	SetCraftingOpen(false);
	RemoveCraftingWidget();
	Super::EndPlay(EndPlayReason);
}

void UWildBoundCraftingComponent::TickComponent(
	float DeltaTime,
	ELevelTick TickType,
	FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (!InventoryComponent.IsValid() && GetOwner())
	{
		InventoryComponent = GetOwner()->FindComponentByClass<UWildBoundInventoryComponent>();
	}

	EnsureCraftingWidget();

	if (bCraftInProgress)
	{
		CraftElapsedSeconds += FMath::Max(0.0f, DeltaTime);
		if (CraftElapsedSeconds >= ActiveCraftDurationSeconds)
		{
			CompletePendingCraft();
		}
	}

	APawn* Pawn = Cast<APawn>(GetOwner());
	APlayerController* PlayerController = Pawn ? Cast<APlayerController>(Pawn->GetController()) : nullptr;
	if (!PlayerController)
	{
		return;
	}

	if (PlayerController->WasInputKeyJustPressed(EKeys::C))
	{
		ToggleCrafting();
		return;
	}

	if (!bCraftingOpen)
	{
		return;
	}

	if (PlayerController->WasInputKeyJustPressed(EKeys::Escape)
		|| PlayerController->WasInputKeyJustPressed(EKeys::I)
		|| PlayerController->WasInputKeyJustPressed(EKeys::Tab))
	{
		SetCraftingOpen(false);
		return;
	}

	if (PlayerController->WasInputKeyJustPressed(EKeys::Up))
	{
		MoveSelection(-1);
	}
	else if (PlayerController->WasInputKeyJustPressed(EKeys::Down))
	{
		MoveSelection(1);
	}
	else if (PlayerController->WasInputKeyJustPressed(EKeys::Enter))
	{
		CraftSelectedRecipe();
	}
}

bool UWildBoundCraftingComponent::IsNearWorkbench() const
{
	const UWorld* World = GetWorld();
	const AActor* Owner = GetOwner();
	if (!World || !Owner)
	{
		return false;
	}

	const float RadiusSq = FMath::Square(WorkbenchUseRadius);
	for (TActorIterator<AActor> It(const_cast<UWorld*>(World)); It; ++It)
	{
		const AActor* Actor = *It;
		if (Actor
			&& Actor->ActorHasTag(WorkbenchTag)
			&& FVector::DistSquared(Owner->GetActorLocation(), Actor->GetActorLocation()) <= RadiusSq)
		{
			return true;
		}
	}
	return false;
}

void UWildBoundCraftingComponent::BuildRecipesForCurrentMode()
{
	Recipes.Reset();

	if (!bWorkbenchMode)
	{
		FWildBoundCraftingRecipe FirstAid;
		FirstAid.RecipeId = TEXT("FirstAidKit");
		FirstAid.DisplayName = TEXT("HAND: FIRST-AID KIT");
		FirstAid.Description = TEXT("Assemble a basic medical kit from cloth, disinfectant chemicals, and adhesive.");
		FirstAid.OutputItemId = TEXT("MedicalSupplies");
		FirstAid.OutputQuantity = 1;
		FirstAid.Ingredients =
		{
			Ingredient(TEXT("Cloth"), 3),
			Ingredient(TEXT("Chemicals"), 1),
			Ingredient(TEXT("Adhesive"), 1)
		};
		Recipes.Add(FirstAid);

		FWildBoundCraftingRecipe TraumaKit;
		TraumaKit.RecipeId = TEXT("TraumaKit");
		TraumaKit.DisplayName = TEXT("HAND: FIELD TRAUMA KIT");
		TraumaKit.Description = TEXT("Combine medical stock into a heavier emergency kit. Restores 80 health when used from the hotbar.");
		TraumaKit.OutputItemId = TEXT("TraumaKit");
		TraumaKit.OutputQuantity = 1;
		TraumaKit.Ingredients =
		{
			Ingredient(TEXT("MedicalSupplies"), 2),
			Ingredient(TEXT("Cloth"), 2),
			Ingredient(TEXT("Adhesive"), 1)
		};
		Recipes.Add(TraumaKit);

		FWildBoundCraftingRecipe RadTreatment;
		RadTreatment.RecipeId = TEXT("RadTreatment");
		RadTreatment.DisplayName = TEXT("HAND: RADIATION TREATMENT");
		RadTreatment.Description = TEXT("Prepare a single-use emergency radiation treatment. Removes 30 accumulated dose when used from the hotbar.");
		RadTreatment.OutputItemId = TEXT("RadTreatment");
		RadTreatment.OutputQuantity = 1;
		RadTreatment.Ingredients =
		{
			Ingredient(TEXT("MedicalSupplies"), 1),
			Ingredient(TEXT("Chemicals"), 2),
			Ingredient(TEXT("Water"), 1)
		};
		Recipes.Add(RadTreatment);
	}
	else
	{
		FWildBoundCraftingRecipe Flashlight;
		Flashlight.RecipeId = TEXT("Flashlight");
		Flashlight.DisplayName = TEXT("BENCH: FLASHLIGHT");
		Flashlight.Description = TEXT("Assemble a working handheld light from electronics, batteries, plastic housing, and wire.");
		Flashlight.OutputItemId = TEXT("Flashlight");
		Flashlight.OutputQuantity = 1;
		Flashlight.Ingredients =
		{
			Ingredient(TEXT("Electronics"), 1),
			Ingredient(TEXT("Battery"), 2),
			Ingredient(TEXT("Plastic"), 1),
			Ingredient(TEXT("Wire"), 1)
		};
		Recipes.Add(Flashlight);

		FWildBoundCraftingRecipe Crowbar;
		Crowbar.RecipeId = TEXT("Crowbar");
		Crowbar.DisplayName = TEXT("BENCH: IMPROVISED CROWBAR");
		Crowbar.Description = TEXT("Shape and reinforce salvaged metal into a heavy pry tool for sealed containers and barred routes.");
		Crowbar.OutputItemId = TEXT("Crowbar");
		Crowbar.OutputQuantity = 1;
		Crowbar.Ingredients =
		{
			Ingredient(TEXT("ScrapMetal"), 5),
			Ingredient(TEXT("MechanicalParts"), 2),
			Ingredient(TEXT("Cloth"), 1)
		};
		Recipes.Add(Crowbar);

		FWildBoundCraftingRecipe Backpack;
		Backpack.RecipeId = TEXT("ReinforcedBackpack");
		Backpack.DisplayName = TEXT("BENCH: REINFORCED BACKPACK");
		Backpack.Description = TEXT("Reinforce the pack frame and straps. Increases carrying capacity by 12 kg while carried.");
		Backpack.OutputItemId = ReinforcedBackpackItemId;
		Backpack.OutputQuantity = 1;
		Backpack.Ingredients =
		{
			Ingredient(TEXT("ScrapMetal"), 3),
			Ingredient(TEXT("Cloth"), 6),
			Ingredient(TEXT("Plastic"), 3),
			Ingredient(TEXT("Adhesive"), 2),
			Ingredient(TEXT("MechanicalParts"), 1)
		};
		Recipes.Add(Backpack);

		FWildBoundCraftingRecipe FilterMask;
		FilterMask.RecipeId = TEXT("FilterMask");
		FilterMask.DisplayName = TEXT("BENCH: FILTER MASK");
		FilterMask.Description = TEXT("Build a sealed particulate mask. Reduces radiation dose accumulation by 45% while carried.");
		FilterMask.OutputItemId = FilterMaskItemId;
		FilterMask.OutputQuantity = 1;
		FilterMask.Ingredients =
		{
			Ingredient(TEXT("Cloth"), 3),
			Ingredient(TEXT("Plastic"), 2),
			Ingredient(TEXT("Chemicals"), 2),
			Ingredient(TEXT("Adhesive"), 1)
		};
		Recipes.Add(FilterMask);

		FWildBoundCraftingRecipe Canteen;
		Canteen.RecipeId = TEXT("Canteen");
		Canteen.DisplayName = TEXT("BENCH: SEALED CANTEEN");
		Canteen.Description = TEXT("Build a reusable sealed canteen. While carried, drinking water restores 45 thirst instead of 35.");
		Canteen.OutputItemId = CanteenItemId;
		Canteen.OutputQuantity = 1;
		Canteen.Ingredients =
		{
			Ingredient(TEXT("ScrapMetal"), 2),
			Ingredient(TEXT("Plastic"), 2),
			Ingredient(TEXT("Adhesive"), 1)
		};
		Recipes.Add(Canteen);

		FWildBoundCraftingRecipe UtilityBelt;
		UtilityBelt.RecipeId = TEXT("UtilityBelt");
		UtilityBelt.DisplayName = TEXT("BENCH: UTILITY BELT");
		UtilityBelt.Description = TEXT("Build a rugged tool belt with extra pouches. Adds 4 inventory slots while carried.");
		UtilityBelt.OutputItemId = UtilityBeltItemId;
		UtilityBelt.OutputQuantity = 1;
		UtilityBelt.Ingredients =
		{
			Ingredient(TEXT("Cloth"), 4),
			Ingredient(TEXT("ScrapMetal"), 2),
			Ingredient(TEXT("Adhesive"), 1),
			Ingredient(TEXT("MechanicalParts"), 1)
		};
		Recipes.Add(UtilityBelt);
	}

	SelectedRecipeIndex = 0;
}

void UWildBoundCraftingComponent::EnsureCraftingWidget()
{
	if (CraftingViewportRoot.IsValid())
	{
		if (CraftingWidget.IsValid())
		{
			CraftingWidget->SetCraftingComponent(this);
		}
		return;
	}

	if (!InventoryComponent.IsValid() || !GEngine || !GEngine->GameViewport)
	{
		return;
	}

	TSharedPtr<SOverlay> Overlay;
	SAssignNew(Overlay, SOverlay)
	+ SOverlay::Slot()
	.HAlign(HAlign_Center)
	.VAlign(VAlign_Center)
	.Padding(FMargin(24.0f))
	[
		SAssignNew(CraftingWidget, SWildBoundCraftingWidget)
		.CraftingComponent(this)
	];

	CraftingViewportRoot = Overlay;
	CraftingViewportRoot->SetVisibility(EVisibility::Collapsed);
	GEngine->GameViewport->AddViewportWidgetContent(CraftingViewportRoot.ToSharedRef(), 140);
}

void UWildBoundCraftingComponent::ToggleCrafting()
{
	if (bCraftingOpen)
	{
		SetCraftingOpen(false);
		return;
	}
	OpenCrafting(IsNearWorkbench());
}

void UWildBoundCraftingComponent::OpenCrafting(bool bUseWorkbench)
{
	const UWildBoundBackpackComponent* Backpack = GetOwner()
		? GetOwner()->FindComponentByClass<UWildBoundBackpackComponent>()
		: nullptr;
	if (Backpack && Backpack->IsBackpackOpen())
	{
		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(91020, 1.8f, FColor(190, 190, 175), TEXT("Close the backpack before opening crafting."));
		}
		return;
	}

	bWorkbenchMode = bUseWorkbench;
	BuildRecipesForCurrentMode();
	CancelPendingCraft();
	LastCraftSuccessWorldTime = -1000.0f;
	LastCraftedDisplayName.Reset();
	SetCraftingOpen(true);

	if (bCraftingOpen && GEngine)
	{
		GEngine->AddOnScreenDebugMessage(
			91023,
			1.6f,
			bWorkbenchMode ? FColor(205, 190, 135) : FColor(175, 205, 165),
			bWorkbenchMode
				? TEXT("WORKBENCH CRAFTING — advanced recipes available")
				: TEXT("HAND CRAFTING — find a workbench for advanced recipes"));
	}
}

void UWildBoundCraftingComponent::SetCraftingOpen(bool bOpen)
{
	if (bCraftingOpen == bOpen && (!bOpen || CraftingViewportRoot.IsValid()))
	{
		return;
	}

	EnsureCraftingWidget();
	if (!bOpen)
	{
		CancelPendingCraft();
	}
	bCraftingOpen = bOpen && CraftingViewportRoot.IsValid();

	if (CraftingViewportRoot.IsValid())
	{
		CraftingViewportRoot->SetVisibility(
			bCraftingOpen ? EVisibility::Visible : EVisibility::Collapsed);
	}

	APawn* Pawn = Cast<APawn>(GetOwner());
	APlayerController* PlayerController = Pawn ? Cast<APlayerController>(Pawn->GetController()) : nullptr;
	if (PlayerController)
	{
		PlayerController->SetIgnoreMoveInput(bCraftingOpen);
		PlayerController->SetIgnoreLookInput(bCraftingOpen);
		PlayerController->bShowMouseCursor = bCraftingOpen;

		if (bCraftingOpen)
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
}

void UWildBoundCraftingComponent::MoveSelection(int32 Direction)
{
	if (bCraftInProgress || Recipes.IsEmpty() || Direction == 0)
	{
		return;
	}

	SelectedRecipeIndex = (SelectedRecipeIndex + Direction) % Recipes.Num();
	if (SelectedRecipeIndex < 0)
	{
		SelectedRecipeIndex += Recipes.Num();
	}
}

void UWildBoundCraftingComponent::SelectRecipeFromMouse(int32 RecipeIndex)
{
	if (!bCraftInProgress && Recipes.IsValidIndex(RecipeIndex))
	{
		SelectedRecipeIndex = RecipeIndex;
	}
}

void UWildBoundCraftingComponent::CraftSelectedRecipeFromMouse()
{
	CraftSelectedRecipe();
}

bool UWildBoundCraftingComponent::CanCraftRecipe(int32 RecipeIndex) const
{
	const UWildBoundInventoryComponent* Inventory = InventoryComponent.Get();
	if (!Inventory || !Recipes.IsValidIndex(RecipeIndex))
	{
		return false;
	}

	const FWildBoundCraftingRecipe& Recipe = Recipes[RecipeIndex];
	if (IsUniqueGearItem(Recipe.OutputItemId) && Inventory->HasItem(Recipe.OutputItemId, 1))
	{
		return false;
	}

	for (const FWildBoundCraftingIngredient& Requirement : Recipe.Ingredients)
	{
		if (!Inventory->HasItem(Requirement.ItemId, Requirement.Quantity))
		{
			return false;
		}
	}
	return true;
}

float UWildBoundCraftingComponent::GetCraftProgress() const
{
	if (bCraftInProgress && ActiveCraftDurationSeconds > KINDA_SMALL_NUMBER)
	{
		return FMath::Clamp(CraftElapsedSeconds / ActiveCraftDurationSeconds, 0.0f, 1.0f);
	}
	return GetCraftSuccessFlashAlpha() > 0.0f ? 1.0f : 0.0f;
}

float UWildBoundCraftingComponent::GetCraftSuccessFlashAlpha() const
{
	const UWorld* World = GetWorld();
	if (!World)
	{
		return 0.0f;
	}

	const float Age = World->GetTimeSeconds() - LastCraftSuccessWorldTime;
	if (Age < 0.0f || Age > 0.70f)
	{
		return 0.0f;
	}
	return FMath::Clamp(1.0f - Age / 0.70f, 0.0f, 1.0f);
}

void UWildBoundCraftingComponent::CraftSelectedRecipe()
{
	if (bCraftInProgress)
	{
		return;
	}

	UWildBoundInventoryComponent* Inventory = InventoryComponent.Get();
	if (!Inventory || !Recipes.IsValidIndex(SelectedRecipeIndex))
	{
		return;
	}

	const FWildBoundCraftingRecipe& Recipe = Recipes[SelectedRecipeIndex];
	if (IsUniqueGearItem(Recipe.OutputItemId) && Inventory->HasItem(Recipe.OutputItemId, 1))
	{
		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(91021, 1.8f, FColor(190, 190, 175), TEXT("You already have this gear."));
		}
		return;
	}

	if (!CanCraftRecipe(SelectedRecipeIndex))
	{
		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(91021, 1.8f, FColor(220, 145, 110), TEXT("Missing crafting materials."));
		}
		return;
	}

	PendingRecipeIndex = SelectedRecipeIndex;
	CraftElapsedSeconds = 0.0f;
	ActiveCraftDurationSeconds = bWorkbenchMode ? 1.15f : 0.82f;
	bCraftInProgress = true;
	LastCraftSuccessWorldTime = -1000.0f;
}

void UWildBoundCraftingComponent::CompletePendingCraft()
{
	UWildBoundInventoryComponent* Inventory = InventoryComponent.Get();
	if (!Inventory || !Recipes.IsValidIndex(PendingRecipeIndex))
	{
		CancelPendingCraft();
		return;
	}

	const int32 RecipeIndex = PendingRecipeIndex;
	const FWildBoundCraftingRecipe& Recipe = Recipes[RecipeIndex];
	if (!CanCraftRecipe(RecipeIndex))
	{
		CancelPendingCraft();
		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(91021, 1.8f, FColor(220, 145, 110), TEXT("Craft interrupted — materials changed."));
		}
		return;
	}

	TArray<FWildBoundCraftingIngredient> RemovedIngredients;
	for (const FWildBoundCraftingIngredient& Requirement : Recipe.Ingredients)
	{
		if (!Inventory->RemoveItem(Requirement.ItemId, Requirement.Quantity))
		{
			for (const FWildBoundCraftingIngredient& Removed : RemovedIngredients)
			{
				Inventory->AddItem(Removed.ItemId, Removed.Quantity);
			}
			CancelPendingCraft();
			return;
		}
		RemovedIngredients.Add(Requirement);
	}

	if (!Inventory->AddItem(Recipe.OutputItemId, Recipe.OutputQuantity))
	{
		for (const FWildBoundCraftingIngredient& Removed : RemovedIngredients)
		{
			Inventory->AddItem(Removed.ItemId, Removed.Quantity);
		}

		CancelPendingCraft();
		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(91021, 2.0f, FColor(220, 145, 110), TEXT("Not enough inventory space for crafted item."));
		}
		return;
	}

	LastCraftedDisplayName = Recipe.DisplayName;
	LastCraftedDisplayName.RemoveFromStart(TEXT("HAND: "));
	LastCraftedDisplayName.RemoveFromStart(TEXT("BENCH: "));
	if (const UWorld* World = GetWorld())
	{
		LastCraftSuccessWorldTime = World->GetTimeSeconds();
	}

	const int32 RarityTier = Inventory->GetItemRarityTier(Recipe.OutputItemId);
	bCraftInProgress = false;
	PendingRecipeIndex = INDEX_NONE;
	CraftElapsedSeconds = 0.0f;
	ActiveCraftDurationSeconds = 0.0f;
	PlayCraftCompletionSound(RarityTier);

	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(
			91021,
			2.0f,
			FColor(170, 215, 155),
			FString::Printf(TEXT("Crafted: %s x%d"), *LastCraftedDisplayName, Recipe.OutputQuantity));
	}
}

void UWildBoundCraftingComponent::CancelPendingCraft()
{
	bCraftInProgress = false;
	PendingRecipeIndex = INDEX_NONE;
	CraftElapsedSeconds = 0.0f;
	ActiveCraftDurationSeconds = 0.0f;
}

void UWildBoundCraftingComponent::PlayCraftCompletionSound(int32 RarityTier) const
{
	UWorld* World = GetWorld();
	AActor* Owner = GetOwner();
	if (!World || !Owner)
	{
		return;
	}

	USoundWaveProcedural* Wave = BuildCraftCompletionWave(World, RarityTier, bWorkbenchMode);
	if (!Wave)
	{
		return;
	}

	UGameplayStatics::SpawnSoundAtLocation(
		World,
		Wave,
		Owner->GetActorLocation(),
		FRotator::ZeroRotator,
		0.72f,
		1.0f,
		0.0f,
		nullptr,
		nullptr,
		true);
}

void UWildBoundCraftingComponent::RemoveCraftingWidget()
{
	if (GEngine && GEngine->GameViewport && CraftingViewportRoot.IsValid())
	{
		GEngine->GameViewport->RemoveViewportWidgetContent(CraftingViewportRoot.ToSharedRef());
	}

	CancelPendingCraft();
	CraftingWidget.Reset();
	CraftingViewportRoot.Reset();
	bCraftingOpen = false;
}
