// Project Mordecai — Melee Attack Gameplay Ability (US-004)

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Abilities/LyraGameplayAbility.h"
#include "MordecaiCombatTypes.h"

#include "MordecaiGA_MeleeAttack.generated.h"

class UMordecaiAttackProfileDataAsset;
class UAbilitySystemComponent;
struct FMordecaiHitResult;

/**
 * UMordecaiGA_MeleeAttack
 *
 * GAS gameplay ability for melee attacks. Reads attack parameters from a
 * UMordecaiAttackProfileDataAsset, executes Windup->Active->Recovery phases,
 * performs hit detection during Active, and applies damage via GameplayEffects.
 * Supports light attack combo chains.
 *
 * See: US-004, attack_taxonomy_v1.md, combat_system_v1.md
 */
UCLASS()
class LYRAGAME_API UMordecaiGA_MeleeAttack : public ULyraGameplayAbility
{
	GENERATED_BODY()

public:
	UMordecaiGA_MeleeAttack(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	// --- Configuration (AC-004.2) ---

	/** Attack profiles for each combo step. Index 0 = step 1. Must have at least one entry. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Mordecai|Attack")
	TArray<TObjectPtr<UMordecaiAttackProfileDataAsset>> AttackProfiles;

	/** Combo window timeout in milliseconds (AC-004.10). Default 800ms per design. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Mordecai|Combo")
	float ComboWindowTimeoutMs = 800.f;

	// --- Public Getters (testable API) ---

	/** Get the attack profile for the current combo step. Returns nullptr if invalid. */
	UFUNCTION(BlueprintCallable, Category = "Mordecai|Attack")
	const UMordecaiAttackProfileDataAsset* GetActiveProfile() const;

	/** Get current combo index (0-based). */
	UFUNCTION(BlueprintCallable, Category = "Mordecai|Attack")
	int32 GetCurrentComboIndex() const { return CurrentComboIndex; }

	/** Get current attack phase. */
	UFUNCTION(BlueprintCallable, Category = "Mordecai|Attack")
	EMordecaiAttackPhase GetCurrentPhase() const { return CurrentPhase; }

	/** Get phase duration in seconds from the active profile (AC-004.3). */
	float GetPhaseDurationSeconds(EMordecaiAttackPhase Phase) const;

	/** Compute health damage from active profile. Returns negative value (AC-004.5). */
	float ComputeHealthDamage() const;

	/** Compute posture damage from active profile. Returns negative value (AC-004.5). */
	float ComputePostureDamage() const;

	/** Get stamina cost from active profile (AC-004.7). */
	float GetStaminaCost() const;

	/** Check if character should be rooted during the given phase (AC-004.8). */
	bool ShouldBeRootedDuringPhase(EMordecaiAttackPhase Phase) const;

	/** Check if ability can be cancelled into dodge in current state (AC-004.12). */
	bool CanCancelIntoDodge() const;

	/** Check if ability can be cancelled into block in current state (AC-004.13). */
	bool CanCancelIntoBlock() const;

	/** Advance combo to next step. Returns true if advanced (AC-004.9). */
	bool AdvanceCombo();

	/** Reset combo to step 0 (AC-004.10). */
	void ResetCombo();

	/** Map damage type enum to gameplay tag (AC-004.6). */
	static FGameplayTag GetDamageTagForType(EMordecaiDamageType DamageType);

	/** Transition to a new phase. Public for testing; called internally by timer callbacks. */
	void TransitionToPhase(EMordecaiAttackPhase NewPhase);

	/** Notify that Light input was received during combo window (AC-004.9). */
	void NotifyComboInput();

	/** Notify that a non-Light input was received, resets combo (AC-004.10). */
	void NotifyDifferentInput();

	/** Check if ActivationOwnedTags contains the given tag (AC-004.14 testability). */
	bool HasActivationOwnedTag(const FGameplayTag& Tag) const;

	/** Check if ActivationBlockedTags contains the given tag (AC-004.14 testability). */
	bool HasActivationBlockedTag(const FGameplayTag& Tag) const;

protected:
	//~ UGameplayAbility interface
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;
	//~ End UGameplayAbility interface

	// --- Phase Callbacks ---
	void OnWindupComplete();
	void OnActiveComplete();
	void OnRecoveryComplete();
	void OnComboWindowTimeout();

	// --- Hit Detection & Damage ---
	void PerformHitDetectionAndApplyDamage();
	void ApplyDamageToTarget(AActor* TargetActor, const FVector& HitLocation);
	void ApplyStaminaCost();
	void ApplyOnUsePayloads();
	void ApplyOnHitPayloads(UAbilitySystemComponent* TargetASC);

	// --- Rooting ---
	void UpdateRooting();

	// --- State ---
	int32 CurrentComboIndex = 0;
	EMordecaiAttackPhase CurrentPhase = EMordecaiAttackPhase::None;
	bool bComboInputReceived = false;
	bool bIsRooted = false;

	FTimerHandle PhaseTimerHandle;
	FTimerHandle ComboWindowTimerHandle;
};
