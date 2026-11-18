// Project Mordecai — authoritative player stamina runtime (US-122 / issue #143)

#pragma once

#include "Components/ActorComponent.h"
#include "GameplayEffectTypes.h"

#include "MordecaiStaminaRuntimeComponent.generated.h"

class UAbilitySystemComponent;
class UMordecaiStaminaSystem;

/**
 * One authority-side runtime for the PlayerState ASC's stamina.
 *
 * Observing the Stamina attribute here means every spend path (attack, dodge,
 * block, parry, sprint, and future GameplayEffects) resets the same delay.
 * The component lives on AMordecaiPlayerState, alongside the ASC, so it is not
 * recreated when the pawn respawns.
 */
UCLASS(ClassGroup = (Mordecai), meta = (BlueprintSpawnableComponent))
class LYRAGAME_API UMordecaiStaminaRuntimeComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UMordecaiStaminaRuntimeComponent(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void TickComponent(
		float DeltaTime,
		ELevelTick TickType,
		FActorComponentTickFunction* ThisTickFunction) override;

private:
	void BindToAbilitySystem();
	void UnbindFromAbilitySystem();
	void OnStaminaChanged(const FOnAttributeChangeData& Data);
	void OnMaxStaminaChanged(const FOnAttributeChangeData& Data);
	void MarkStaminaConsumed();
	void UpdateTierTags();
	double GetRuntimeSeconds() const;

	UPROPERTY(Transient)
	TObjectPtr<UMordecaiStaminaSystem> StaminaSystem;

	TWeakObjectPtr<UAbilitySystemComponent> BoundASC;
	FDelegateHandle StaminaChangedHandle;
	FDelegateHandle MaxStaminaChangedHandle;
};
