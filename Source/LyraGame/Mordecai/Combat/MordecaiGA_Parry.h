// Project Mordecai — Parry Gameplay Ability (US-006)

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Abilities/LyraGameplayAbility.h"
#include "MordecaiCombatTypes.h"

#include "MordecaiGA_Parry.generated.h"

/**
 * UMordecaiGA_Parry
 *
 * GAS gameplay ability for parrying. Activated by simultaneous block+attack input.
 * Has a timed parry window during which incoming attacks are parried for large
 * posture damage and a riposte window. Failed parry (window expires without hit)
 * results in animation lock / whiff penalty.
 *
 * Timer-based, no animation integration.
 *
 * See: US-006, combat_system_v1.md (Parry section)
 */
UCLASS()
class LYRAGAME_API UMordecaiGA_Parry : public ULyraGameplayAbility
{
	GENERATED_BODY()

public:
	UMordecaiGA_Parry(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	// --- Configuration ---

	/** Total parry window duration in milliseconds (AC-006.12). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Mordecai|Parry")
	float ParryWindowMs = 500.f;

	/** Posture damage dealt to attacker on successful parry (AC-006.13). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Mordecai|Parry")
	float ParryPostureDamage = 50.f;

	/** Duration of whiff penalty in milliseconds after failed parry (AC-006.14). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Mordecai|Parry")
	float ParryWhiffDurationMs = 600.f;

	/** Stamina consumed on parry activation (AC-006.15). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Mordecai|Parry")
	float ParryStaminaCost = 20.f;

	// --- Public Getters (testable API) ---

	/** Get current parry phase. */
	UFUNCTION(BlueprintCallable, Category = "Mordecai|Parry")
	EMordecaiParryPhase GetCurrentParryPhase() const { return CurrentParryPhase; }

	/** Whether the parry was successful (hit was parried during window). */
	UFUNCTION(BlueprintCallable, Category = "Mordecai|Parry")
	bool WasParrySuccessful() const { return bParrySuccessful; }

	/** Get parry window duration in seconds. */
	float GetParryWindowSeconds() const { return ParryWindowMs / 1000.f; }

	/** Get whiff duration in seconds. */
	float GetParryWhiffDurationSeconds() const { return ParryWhiffDurationMs / 1000.f; }

	/** Get stamina cost. */
	float GetStaminaCost() const { return ParryStaminaCost; }

	/** Simulate receiving a hit during parry window. If in Active phase,
	 *  applies parried state to attacker and riposte state to self.
	 *  Returns true if parry was successful. */
	bool ReceiveParriedHit(UAbilitySystemComponent* AttackerASC);

	/** Transition to a new parry phase. Public for testing. */
	void TransitionToParryPhase(EMordecaiParryPhase NewPhase);

	/** Check if ActivationBlockedTags contains the given tag. */
	bool HasActivationBlockedTag(const FGameplayTag& Tag) const;

protected:
	//~ UGameplayAbility interface
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;
	//~ End UGameplayAbility interface

	// --- Phase Callbacks ---
	void OnParryWindowExpired();
	void OnWhiffComplete();

	// --- Stamina ---
	void ApplyStaminaCost();

	// --- Tag Management ---
	void ApplyParryingTag();
	void RemoveParryingTag();
	void ApplyParryWhiffTag();
	void RemoveParryWhiffTag();
	void ApplyParriedTagToAttacker(UAbilitySystemComponent* AttackerASC);
	void ApplyRiposteTag();
	void RemoveRiposteTag();

	// --- State ---
	EMordecaiParryPhase CurrentParryPhase = EMordecaiParryPhase::None;
	bool bParrySuccessful = false;

	FTimerHandle ParryWindowTimerHandle;
	FTimerHandle WhiffTimerHandle;
};
