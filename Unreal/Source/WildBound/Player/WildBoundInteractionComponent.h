#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "WildBoundInteractionComponent.generated.h"

UCLASS(ClassGroup=(WildBound), meta=(BlueprintSpawnableComponent))
class WILDBOUND_API UWildBoundInteractionComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UWildBoundInteractionComponent();

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

private:
	float InteractionDistance = 450.0f;

	void TryInteract(AActor* TargetActor);
	FString GetInteractionPrompt(const AActor* TargetActor) const;
	void DestroyInteractionGroup(const FName& GroupTag);
	void HandleQuickUse(class APlayerController& PlayerController);
	void TryUseInventoryItem(FName ItemId);
};
