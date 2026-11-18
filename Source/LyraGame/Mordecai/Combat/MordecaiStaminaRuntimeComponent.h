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
	bool IsSprintingAvatarMoving(const UAbilitySystemComponent* ASC) const;
	void ClearSprintToggle(UAbilitySystemComponent* ASC);
	void ResetSprintTracking();

	/** Horizontal speed above this fraction of base walk speed counts as moving sprint. */
	UPROPERTY(EditDefaultsOnly, Category = "Mordecai|Stamina|Sprint")
	float SprintMovementThresholdRatio = 0.1f;

	/** Stationary grace before a toggled sprint clears itself. */
	UPROPERTY(EditDefaultsOnly, Category = "Mordecai|Stamina|Sprint")
	float SprintStationaryAutoClearSeconds = 0.25f;

	/** Delay before a never-moved toggle starts accumulating stationary clear time. */
	UPROPERTY(EditDefaultsOnly, Category = "Mordecai|Stamina|Sprint")
	float SprintToggleGraceSeconds = 1.f;

	UPROPERTY(Transient)
	TObjectPtr<UMordecaiStaminaSystem> StaminaSystem;

	TWeakObjectPtr<UAbilitySystemComponent> BoundASC;
	FDelegateHandle StaminaChangedHandle;
	FDelegateHandle MaxStaminaChangedHandle;
	float StationarySprintSeconds = 0.f;
	float SprintToggleSeconds = 0.f;
	bool bSprintMovedSinceToggle = false;
	bool bWasSprinting = false;
};
