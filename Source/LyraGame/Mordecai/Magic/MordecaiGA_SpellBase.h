// Project Mordecai — Spell Base Gameplay Ability (US-019)

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Abilities/LyraGameplayAbility.h"
#include "MordecaiSpellTypes.h"

#include "MordecaiGA_SpellBase.generated.h"

class UMordecaiSpellDataAsset;
class UAbilitySystemComponent;

/**
 * UMordecaiGA_SpellBase
 *
 *	Base gameplay ability for all spells. Reads spell parameters from a
 *	UMordecaiSpellDataAsset, enforces SP cost/cooldown, executes casting
 *	phases (Windup->Cast->Recovery), handles movement policies, interruption,
 *	and computes spell power with attribute and rank scaling.
 *
 *	Subclasses override OnSpellWindupComplete, OnSpellCast, and
 *	OnSpellRecoveryComplete to implement specific spell behaviors.
 *
 *	See: US-019, ability_schema_v1.md, ability_system_v1.md
 */
UCLASS()
class LYRAGAME_API UMordecaiGA_SpellBase : public ULyraGameplayAbility
{
	GENERATED_BODY()

public:
	UMordecaiGA_SpellBase(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	// --- Configuration (AC-019.2) ---

	/** Spell data asset defining all spell parameters. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Mordecai|Spell")
	TObjectPtr<UMordecaiSpellDataAsset> SpellData;

	/** If true, this activation is an upcast (AC-019.11). */
	UPROPERTY(BlueprintReadWrite, Category = "Mordecai|Spell")
	bool bIsUpcast = false;

	// --- Public Getters (testable API) ---

	/** Get the current casting phase. */
	UFUNCTION(BlueprintCallable, Category = "Mordecai|Spell")
	EMordecaiCastingPhase GetCurrentPhase() const { return CurrentPhase; }

	/** Get the effective SP cost, accounting for upcast multiplier if applicable (AC-019.3, AC-019.11). */
	float GetEffectiveSPCost(bool bUpcast) const;

	/** Check if the caster has enough SP to cast (AC-019.3). */
	bool CheckSpellPointCost(float CurrentSP, bool bUpcast) const;

	/** Get cooldown duration from data asset (AC-019.5). */
	float GetCooldownDuration() const;

	/** Get cooldown tag from data asset (AC-019.5). */
	FGameplayTag GetCooldownTag() const;

	/** Get windup phase duration in seconds (AC-019.6). */
	float GetWindupDuration() const;

	/** Get cast phase duration in seconds (AC-019.6). */
	float GetCastDuration() const;

	/** Get recovery phase duration in seconds (AC-019.6). */
	float GetRecoveryDuration() const;

	/** Get the movement policy tag for the current spell (AC-019.7). Returns invalid tag for FreeMove. */
	FGameplayTag GetMovementPolicyTag() const;

	/** Returns true if the spell is interruptible (AC-019.8). */
	bool IsInterruptible() const;

	/** Returns true if the spell should be interrupted in the current phase (AC-019.8). */
	bool ShouldInterruptInCurrentPhase() const;

	/**
	 * Compute spell power with attribute scaling, rank scaling, and upcast (AC-019.9, AC-019.10, AC-019.11).
	 *
	 * @param AttributeScalingBonus  Σ(ScalingStat.Coefficient × EffectiveModForStat) — pre-computed
	 * @param SkillRank              Current skill rank for the spell (0-20)
	 * @param bUpcast                Whether this is an upcast
	 * @return Final spell power value
	 */
	float ComputeSpellPower(float AttributeScalingBonus, int32 SkillRank, bool bUpcast) const;

	/** Transition to a new casting phase. Public for testing; called internally by timer callbacks. */
	void TransitionToPhase(EMordecaiCastingPhase NewPhase);

	/** Check if ActivationBlockedTags contains the given tag (testability). */
	bool HasActivationBlockedTag(const FGameplayTag& Tag) const;

protected:
	//~ UGameplayAbility interface
	virtual bool CanActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayTagContainer* SourceTags = nullptr, const FGameplayTagContainer* TargetTags = nullptr, FGameplayTagContainer* OptionalRelevantTags = nullptr) const override;
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;
	//~ End UGameplayAbility interface

	// --- Virtual Hooks for Subclasses (AC-019.6) ---

	/** Called when Windup phase completes. Override in subclasses for pre-cast behavior. */
	virtual void OnSpellWindupComplete();

	/** Called when Cast phase fires. Override in subclasses to deliver spell effects (projectiles, AoE, etc). */
	virtual void OnSpellCast();

	/** Called when Recovery phase completes. Override in subclasses for post-cast cleanup. */
	virtual void OnSpellRecoveryComplete();

	// --- Phase Callbacks ---
	void HandleWindupComplete();
	void HandleCastComplete();
	void HandleRecoveryComplete();

	// --- SP & Cooldown ---
	void ApplySpellPointCost();
	void ApplySpellCooldown();

	// --- Movement Policy ---
	void ApplyMovementPolicyTag();
	void RemoveMovementPolicyTag();

	// --- Interruption ---
	void OnDamageTakenCallback(const FGameplayEventData* Payload);
	FDelegateHandle DamageEventDelegateHandle;

	// --- Internal Helpers ---
	float ComputeAttributeScalingSumFromASC() const;
	int32 GetSkillRankFromComponent() const;

	// --- State ---
	EMordecaiCastingPhase CurrentPhase = EMordecaiCastingPhase::None;
	bool bMovementPolicyTagApplied = false;
	bool bCooldownApplied = false;

	FTimerHandle PhaseTimerHandle;

	/** Cached cooldown tag container for GetCooldownTags override. */
	mutable FGameplayTagContainer CooldownTagContainer;
};
