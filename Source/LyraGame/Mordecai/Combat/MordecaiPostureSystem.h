// Project Mordecai — Posture & Stagger System (US-007)

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"

#include "MordecaiPostureSystem.generated.h"

/**
 * UMordecaiPostureSystem
 *
 * Manages posture meter logic: posture damage computation, posture break detection,
 * stagger/knock down state, damage vulnerability multipliers, and posture regeneration.
 *
 * Created per-actor. The computation methods are pure and testable. State management
 * methods track per-actor posture state (break, knock down, regen delay).
 *
 * This class provides the logic layer. GAS integration (GameplayEffects, tags, timers)
 * is handled by the AttributeSet's PostGameplayEffectExecute and external GE application.
 *
 * See: US-007, combat_system_v1.md (Posture System)
 */
UCLASS(BlueprintType)
class LYRAGAME_API UMordecaiPostureSystem : public UObject
{
	GENERATED_BODY()

public:
	UMordecaiPostureSystem();

	// --- Configuration ---

	/** Duration of posture broken (stagger) state in milliseconds (AC-007.9). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Mordecai|Posture")
	float PostureBrokenDurationMs = 1000.f;

	/** Damage multiplier during stagger (AC-007.10). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Mordecai|Posture")
	float StaggeredDamageMultiplier = 1.5f;

	/** Damage threshold during stagger that triggers knock down (AC-007.10a). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Mordecai|Posture")
	float KnockDownDamageThreshold = 50.f;

	/** Duration of knocked down state in milliseconds (AC-007.10a). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Mordecai|Posture")
	float KnockDownDurationMs = 2000.f;

	/** Damage multiplier during knocked down state (AC-007.10a). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Mordecai|Posture")
	float KnockDownDamageMultiplier = 2.0f;

	/** Posture regeneration rate in units per second (AC-007.12). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Mordecai|Posture")
	float PostureRegenRate = 10.f;

	/** Delay in ms before posture starts regenerating after taking posture damage (AC-007.13). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Mordecai|Posture")
	float PostureRegenDelayMs = 3000.f;

	/** Posture damage multiplier when target is blocking (AC-007.5). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Mordecai|Posture")
	float BlockPostureDamageMultiplier = 0.5f;

	/** Stub: Multiplier for outgoing posture damage when attacker has Weakened status (AC-007.15).
	 *  Default 1.0 (no effect). Wired in Epic 4. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Mordecai|Posture|StatusEffects")
	float WeakenedPostureMultiplier = 1.0f;

	/** Stub: Multiplier for incoming posture damage when target has Brittle status (AC-007.16).
	 *  Default 1.0 (no effect). Wired in Epic 4. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Mordecai|Posture|StatusEffects")
	float BrittlePostureMultiplier = 1.0f;

	// --- Computation Methods (pure, testable) ---

	/** Compute posture damage: BasePower x PostureDamageScalar x BlockMult x WeakenedMult x BrittleMult.
	 *  Returns a positive value representing the amount of posture to subtract (AC-007.4, AC-007.5). */
	float ComputePostureDamage(float BasePower, float PostureDamageScalar, bool bIsBlocking = false) const;

	/** Compute damage during stagger: IncomingDamage x StaggeredDamageMultiplier (AC-007.10). */
	float ComputeStaggeredDamage(float IncomingDamage) const;

	/** Compute damage during knock down: IncomingDamage x KnockDownDamageMultiplier (AC-007.10a). */
	float ComputeKnockDownDamage(float IncomingDamage) const;

	/** Get the appropriate damage multiplier based on current vulnerability state.
	 *  Returns 1.0 if not vulnerable, 1.5 if staggered, 2.0 if knocked down. */
	float GetVulnerabilityDamageMultiplier() const;

	// --- State Queries ---

	/** Check if posture should break (posture at or below 0) (AC-007.7). */
	bool ShouldBreakPosture(float PostureAfterDamage) const;

	/** Check if cumulative damage during stagger exceeds knock down threshold (AC-007.10a). */
	bool ShouldKnockDown() const;

	/** Check if posture regen is currently allowed (AC-007.13, AC-007.14). */
	bool IsRegenAllowed(double CurrentTime) const;

	/** Compute posture regen amount for a given delta time (AC-007.12). */
	float ComputeRegenAmount(float DeltaTime) const;

	/** Whether the actor is currently in PostureBroken state. */
	UFUNCTION(BlueprintCallable, Category = "Mordecai|Posture")
	bool IsPostureBroken() const { return bIsPostureBroken; }

	/** Whether the actor is currently in KnockedDown state. */
	UFUNCTION(BlueprintCallable, Category = "Mordecai|Posture")
	bool IsKnockedDown() const { return bIsKnockedDown; }

	/** Whether the actor is in any vulnerability state (broken or knocked down). */
	UFUNCTION(BlueprintCallable, Category = "Mordecai|Posture")
	bool IsVulnerable() const { return bIsPostureBroken || bIsKnockedDown; }

	// --- State Management ---

	/** Notify that posture damage was applied at the given time (AC-007.13). */
	void NotifyPostureDamageApplied(double CurrentTime);

	/** Enter posture broken (stagger) state (AC-007.7). */
	void EnterPostureBroken();

	/** Exit posture broken state, resetting posture to max (AC-007.11). Returns MaxPosture. */
	float ExitPostureBroken(float MaxPosture);

	/** Accumulate damage during stagger for knock down threshold check (AC-007.10a). */
	void AccumulateStaggerDamage(float DamageAmount);

	/** Enter knock down state, replacing stagger (AC-007.10a). */
	void EnterKnockDown();

	/** Exit knock down state, resetting posture to max. Returns MaxPosture. */
	float ExitKnockDown(float MaxPosture);

	/** Reset all state. */
	void Reset();

	// --- Duration Getters ---

	float GetPostureBrokenDurationSeconds() const { return PostureBrokenDurationMs / 1000.f; }
	float GetKnockDownDurationSeconds() const { return KnockDownDurationMs / 1000.f; }
	float GetPostureRegenDelaySeconds() const { return PostureRegenDelayMs / 1000.f; }
	float GetCumulativeDamageDuringStagger() const { return CumulativeDamageDuringStagger; }

private:
	bool bIsPostureBroken = false;
	bool bIsKnockedDown = false;
	double LastPostureDamageTime = -999.0;
	float CumulativeDamageDuringStagger = 0.f;
};
