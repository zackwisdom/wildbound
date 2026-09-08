#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "WildBoundCraftingComponent.generated.h"

class SWidget;
class SWildBoundCraftingWidget;
class UWildBoundInventoryComponent;

USTRUCT(BlueprintType)
struct FWildBoundCraftingIngredient
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="WildBound|Crafting")
	FName ItemId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="WildBound|Crafting", meta=(ClampMin="1"))
	int32 Quantity = 1;
};

USTRUCT(BlueprintType)
struct FWildBoundCraftingRecipe
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="WildBound|Crafting")
	FName RecipeId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="WildBound|Crafting")
	FString DisplayName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="WildBound|Crafting")
	FString Description;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="WildBound|Crafting")
	FName OutputItemId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="WildBound|Crafting", meta=(ClampMin="1"))
	int32 OutputQuantity = 1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="WildBound|Crafting")
	TArray<FWildBoundCraftingIngredient> Ingredients;
};

UCLASS(ClassGroup=(WildBound), meta=(BlueprintSpawnableComponent))
class WILDBOUND_API UWildBoundCraftingComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UWildBoundCraftingComponent();

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	UFUNCTION(BlueprintPure, Category="WildBound|Crafting")
	bool IsCraftingOpen() const { return bCraftingOpen; }

	UFUNCTION(BlueprintPure, Category="WildBound|Crafting")
	int32 GetSelectedRecipeIndex() const { return SelectedRecipeIndex; }

	UFUNCTION(BlueprintPure, Category="WildBound|Crafting")
	bool CanCraftRecipe(int32 RecipeIndex) const;

	const TArray<FWildBoundCraftingRecipe>& GetRecipes() const { return Recipes; }
	UWildBoundInventoryComponent* GetInventoryComponent() const { return InventoryComponent.Get(); }

private:
	TWeakObjectPtr<UWildBoundInventoryComponent> InventoryComponent;
	TArray<FWildBoundCraftingRecipe> Recipes;
	TSharedPtr<SWildBoundCraftingWidget> CraftingWidget;
	TSharedPtr<SWidget> CraftingViewportRoot;
	bool bCraftingOpen = false;
	int32 SelectedRecipeIndex = 0;

	void BuildStarterRecipes();
	void EnsureCraftingWidget();
	void ToggleCrafting();
	void SetCraftingOpen(bool bOpen);
	void MoveSelection(int32 Direction);
	void CraftSelectedRecipe();
	void RemoveCraftingWidget();
};
