// Project Mordecai — Dodge Gameplay Ability (US-005)

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Abilities/LyraGameplayAbility.h"
#include "MordecaiCombatTypes.h"

#include "MordecaiGA_Dodge.generated.h"

/**
 * UMordecaiGA_Dodge
 *
 * GAS gameplay ability for dodge. Grants i-frames (Mordecai.State.Dodging tag)
 * during dodge duration, consumes stamina, moves character in input direction,
 * and has a perfect dodge timing window. Timer-based phases, no animation.
 *
 * I-frames: do NOT disable collision. The damage pipeline checks for the
 * Mordecai.State.Dodging tag to negate incoming damage.
 *
 * See: US-005, combat_system_v1.md (Defense section)
 */
UCLASS()
class LYRAGAME_API UMordecaiGA_Dodge : public ULyraGameplayAbility
{
	GENERATED_BODY()

public:
	UMordecaiGA_Dodge(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	// --- Configuration (AC-005.5) ---

	/** Distance traveled during dodge in centimeters. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Mordecai|Dodge")
	float DodgeDistanceCm = 400.f;

	/** Total dodge duration in milliseconds. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Mordecai|Dodge")
	float DodgeDurationMs = 400.f;

	/** Stamina consumed on activation (AC-005.6). Consumed even at 0 stamina. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Mordecai|Dodge")
	float StaminaCost = 15.f;

	/** Duration of perfect dodge window at start of dodge in ms (AC-005.8).
	 *  TODO(DECISION): Final value TBD by design. Placeholder 150ms. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Mordecai|Dodge")
	float PerfectDodgeWindowMs = 150.f;

	/** Stamina refunded when perfect dodge triggers (AC-005.9).
	 *  TODO(DECISION): Final value TBD by design. Placeholder 10.0. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Mordecai|Dodge")
	float PerfectDodgeStaminaRefund = 10.f;

	/** Cooldown after dodge before another dodge can activate (AC-005.10). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Mordecai|Dodge")
	float DodgeCooldownMs = 300.f;

	// --- Public Getters (testable API) ---

	/** Get current dodge phase. */
	UFUNCTION(BlueprintCallable, Category = "Mordecai|Dodge")
	EMordecaiDodgePhase GetCurrentDodgePhase() const { return CurrentDodgePhase; }

	/** Get dodge direction (set at activation, locked during dodge — AC-005.13). */
	UFUNCTION(BlueprintCallable, Category = "Mordecai|Dodge")
	FVector GetDodgeDirection() const { return DodgeDirection; }

	/** Whether perfect dodge is currently active. */
	UFUNCTION(BlueprintCallable, Category = "Mordecai|Dodge")
	bool IsPerfectDodgeActive() const { return bPerfectDodgeActive; }

	/** Get stamina cost. */
	float GetStaminaCost() const { return StaminaCost; }

	/** Get dodge duration in seconds. */
	float GetDodgeDurationSeconds() const { return DodgeDurationMs / 1000.f; }

	/** Get perfect dodge window in seconds. */
	float GetPerfectDodgeWindowSeconds() const { return PerfectDodgeWindowMs / 1000.f; }

	/** Get cooldown duration in seconds. */
	float GetCooldownDurationSeconds() const { return DodgeCooldownMs / 1000.f; }

	/** Compute dodge direction from movement input and character forward (AC-005.4).
	 *  If no meaningful input, returns backward (opposite of CharacterForward).
	 *  Static pure function — testable without an ability instance. */
	static FVector ComputeDodgeDirection(const FVector& MovementInput, const FVector& CharacterForward);

	/** Compute impulse velocity for dodge movement (AC-005.5).
	 *  Returns Direction * (DodgeDistanceCm / DodgeDurationSeconds). */
	FVector ComputeImpulseVelocity(const FVector& Direction) const;

	/** Transition to a new dodge phase. Public for testing. */
	void TransitionToDodgePhase(EMordecaiDodgePhase NewPhase);

	/** End the perfect dodge window. Public for testing. */
	void EndPerfectDodgeWindow();

	/** Check if ActivationBlockedTags contains the given tag. */
	bool HasActivationBlockedTag(const FGameplayTag& Tag) const;

protected:
	//~ UGameplayAbility interface
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;
	//~ End UGameplayAbility interface

	// --- Phase Callbacks ---
	void OnDodgeComplete();
	void OnPerfectDodgeWindowEnd();
	void OnCooldownComplete();

	// --- Stamina ---
	void ApplyStaminaCost();

	// --- Movement ---
	void ApplyDodgeMovement();
	void StopDodgeMovement();

	// --- Tag Management ---
	void ApplyDodgingTag();
	void RemoveDodgingTag();
	void ApplyPerfectDodgeTag();
	void RemovePerfectDodgeTag();
	void ApplyDodgeCooldownTag();
	void RemoveDodgeCooldownTag();

	// --- State ---
	EMordecaiDodgePhase CurrentDodgePhase = EMordecaiDodgePhase::None;
	FVector DodgeDirection = FVector::ZeroVector;
	bool bPerfectDodgeActive = false;

	FTimerHandle DodgeTimerHandle;
	FTimerHandle PerfectDodgeTimerHandle;
	FTimerHandle CooldownTimerHandle;
};
