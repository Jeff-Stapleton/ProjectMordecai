// Project Mordecai — Block Gameplay Ability (US-006)

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Abilities/LyraGameplayAbility.h"
#include "MordecaiCombatTypes.h"

#include "MordecaiGA_Block.generated.h"

/**
 * UMordecaiGA_Block
 *
 * GAS gameplay ability for blocking. Held ability — active while input is held,
 * ends on release. Applies Mordecai.State.Blocking tag. Mitigates incoming damage,
 * drains stamina per blocked hit, and supports perfect block timing window.
 * Guard breaks when stamina reaches 0 while blocking.
 *
 * Timer-based, no animation integration.
 *
 * See: US-006, combat_system_v1.md (Block section)
 */
UCLASS()
class LYRAGAME_API UMordecaiGA_Block : public ULyraGameplayAbility
{
	GENERATED_BODY()

public:
	UMordecaiGA_Block(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	// --- Configuration ---

	/** Damage mitigation factor (0.0 = no mitigation, 1.0 = full). Stubbed for now;
	 *  will be sourced from gear/attributes in later epics (AC-006.3). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Mordecai|Block")
	float Mitigation = 0.5f;

	/** Stability factor for stamina drain reduction (0.0 = full drain, 1.0 = no drain).
	 *  Stubbed for now; increased by DIS/gear in later epics (AC-006.4). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Mordecai|Block")
	float Stability = 0.3f;

	/** Duration of guard broken stagger in milliseconds (AC-006.5). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Mordecai|Block")
	float GuardBrokenDurationMs = 800.f;

	/** Movement speed multiplier while blocking (AC-006.7). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Mordecai|Block")
	float BlockMoveSpeedMultiplier = 0.5f;

	/** Perfect block timing window in milliseconds from block activation (AC-006.9). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Mordecai|Block")
	float PerfectBlockWindowMs = 250.f;

	/** Posture damage dealt to attacker on perfect block (AC-006.10). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Mordecai|Block")
	float PerfectBlockPostureDamage = 30.f;

	// --- Public Getters (testable API) ---

	/** Get current block phase. */
	UFUNCTION(BlueprintCallable, Category = "Mordecai|Block")
	EMordecaiBlockPhase GetCurrentBlockPhase() const { return CurrentBlockPhase; }

	/** Whether perfect block window is currently active. */
	UFUNCTION(BlueprintCallable, Category = "Mordecai|Block")
	bool IsPerfectBlockWindowActive() const { return bPerfectBlockWindowActive; }

	/** Get the world time when block was activated (for perfect block timing). */
	double GetBlockActivationTime() const { return BlockActivationTime; }

	/** Set block activation time. Public for testing. */
	void SetBlockActivationTime(double Time) { BlockActivationTime = Time; }

	/** Compute final damage after block mitigation: IncomingDamage * (1 - Mitigation) (AC-006.3). */
	float ComputeBlockedDamage(float IncomingDamage) const;

	/** Compute stamina drain for a blocked hit: IncomingDamage * (1 - Stability) (AC-006.4). */
	float ComputeStaminaDrain(float IncomingDamage) const;

	/** Check if a hit that lands at the given time qualifies as a perfect block (AC-006.9).
	 *  The hit time must be within PerfectBlockWindowMs of BlockActivationTime. */
	bool IsHitWithinPerfectBlockWindow(double HitTime) const;

	/** Get guard broken duration in seconds. */
	float GetGuardBrokenDurationSeconds() const { return GuardBrokenDurationMs / 1000.f; }

	/** Get perfect block window in seconds. */
	float GetPerfectBlockWindowSeconds() const { return PerfectBlockWindowMs / 1000.f; }

	/** Transition to a new block phase. Public for testing. */
	void TransitionToBlockPhase(EMordecaiBlockPhase NewPhase);

	/** End the perfect block window. Public for testing. */
	void EndPerfectBlockWindow();

	/** Simulate receiving a blocked hit. Applies mitigation, stamina drain, guard break check.
	 *  Returns the mitigated damage dealt. AttackerASC is optional (for perfect block posture damage). */
	float ReceiveBlockedHit(float IncomingDamage, double HitTime, UAbilitySystemComponent* AttackerASC = nullptr);

	/** Check if ActivationBlockedTags contains the given tag. */
	bool HasActivationBlockedTag(const FGameplayTag& Tag) const;

	/** Check if ActivationOwnedTags contains the given tag. */
	bool HasActivationOwnedTag(const FGameplayTag& Tag) const;

protected:
	//~ UGameplayAbility interface
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;
	//~ End UGameplayAbility interface

	// --- Phase Callbacks ---
	void OnGuardBrokenComplete();
	void OnPerfectBlockWindowEnd();

	// --- Tag Management ---
	void ApplyBlockingTag();
	void RemoveBlockingTag();
	void ApplyPerfectBlockTag();
	void RemovePerfectBlockTag();
	void ApplyGuardBrokenTag();
	void RemoveGuardBrokenTag();

	// --- Movement ---
	void ApplyBlockMoveSpeedReduction();
	void RemoveBlockMoveSpeedReduction();

	// --- Guard Break ---
	void TriggerGuardBreak();

	// --- State ---
	EMordecaiBlockPhase CurrentBlockPhase = EMordecaiBlockPhase::None;
	bool bPerfectBlockWindowActive = false;
	double BlockActivationTime = 0.0;

	FTimerHandle PerfectBlockTimerHandle;
	FTimerHandle GuardBrokenTimerHandle;
};
