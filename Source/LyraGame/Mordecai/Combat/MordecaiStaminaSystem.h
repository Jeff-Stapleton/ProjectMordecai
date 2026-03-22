// Project Mordecai — Stamina Tier System (US-008)

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"

#include "MordecaiStaminaSystem.generated.h"

class UAbilitySystemComponent;

/**
 * Stamina tier enum.
 * Green/Yellow/Red represent the three tiers of stamina effectiveness.
 * Exhausted is tracked separately as an overlay (can coexist with Red).
 */
UENUM(BlueprintType)
enum class EMordecaiStaminaTier : uint8
{
	Green   UMETA(DisplayName = "Green"),
	Yellow  UMETA(DisplayName = "Yellow"),
	Red     UMETA(DisplayName = "Red")
};

/**
 * UMordecaiStaminaSystem
 *
 * Manages stamina tier logic: tier calculation from current/max stamina,
 * effectiveness multipliers per tier, stamina regeneration, sprint drain,
 * and regen delay tracking.
 *
 * Created per-actor. The computation methods are pure and testable. State
 * management tracks per-actor regen delay timing.
 *
 * This class provides the logic layer. GAS integration (GameplayEffects, tags,
 * PostGameplayEffectExecute) is handled externally.
 *
 * See: US-008, combat_system_v1.md (Stamina)
 */
UCLASS(BlueprintType)
class LYRAGAME_API UMordecaiStaminaSystem : public UObject
{
	GENERATED_BODY()

public:
	UMordecaiStaminaSystem();

	// --- Configuration: Tier Thresholds (AC-008.3) ---

	/** Percentage of MaxStamina above which tier is Green. Default 0.66 (66%). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Mordecai|Stamina|Tiers")
	float GreenThresholdPercent = 0.66f;

	/** Percentage of MaxStamina above which tier is Yellow (below Green). Default 0.33 (33%). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Mordecai|Stamina|Tiers")
	float YellowThresholdPercent = 0.33f;

	// --- Configuration: Effectiveness Multipliers (AC-008.7) ---

	/** Multiplier at Green tier: 25% bonus for managing stamina well. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Mordecai|Stamina|Multipliers")
	float GreenMultiplier = 1.25f;

	/** Multiplier at Yellow tier: normal effectiveness. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Mordecai|Stamina|Multipliers")
	float YellowMultiplier = 1.0f;

	/** Multiplier at Red tier: 25% penalty when low. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Mordecai|Stamina|Multipliers")
	float RedMultiplier = 0.75f;

	/** Multiplier when Exhausted (Stamina <= 0): 50% penalty. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Mordecai|Stamina|Multipliers")
	float ExhaustedMultiplier = 0.5f;

	// --- Configuration: Regeneration (AC-008.10, AC-008.11, AC-008.13) ---

	/** Stamina regen rate in units per second. Default 15.0.
	 *  TODO(DECISION): END attribute scaling (END x 2.0%) — stubbed as base rate only until Epic 3. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Mordecai|Stamina|Regen")
	float StaminaRegenRate = 15.f;

	/** Delay in ms before stamina starts regenerating after consumption. Default 1000ms. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Mordecai|Stamina|Regen")
	float StaminaRegenDelayMs = 1000.f;

	// --- Configuration: Sprint Drain (AC-008.14) ---

	/** Stamina drain rate while sprinting, in units per second. Default 10.0. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Mordecai|Stamina|Sprint")
	float SprintStaminaDrainRate = 10.f;

	// --- Configuration: Stamina Floor (AC-008.1) ---

	/** Minimum stamina value (allows negative). Default -50.0. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Mordecai|Stamina")
	float StaminaFloor = -50.f;

	// --- Computation Methods (pure, testable) ---

	/** Determine the stamina tier based on current and max stamina (AC-008.3).
	 *  Returns Green/Yellow/Red. Exhausted is a separate overlay (see IsExhausted). */
	EMordecaiStaminaTier GetTierForStamina(float CurrentStamina, float MaxStamina) const;

	/** Check if the character is in Exhausted state (Stamina <= 0) (AC-008.6). */
	bool IsExhausted(float CurrentStamina) const;

	/** Get the effectiveness multiplier for a given tier (AC-008.7). */
	float GetMultiplierForTier(EMordecaiStaminaTier Tier) const;

	/** Get the effectiveness multiplier for current stamina values (AC-008.8).
	 *  Returns ExhaustedMultiplier if Stamina <= 0, otherwise the tier multiplier. */
	float GetTierMultiplier(float CurrentStamina, float MaxStamina) const;

	/** Compute stamina regen amount for a given delta time (AC-008.10). */
	float ComputeRegenAmount(float DeltaTime) const;

	/** Compute stamina drain from sprinting for a given delta time (AC-008.14, AC-008.15). */
	float ComputeSprintDrain(float DeltaTime) const;

	// --- State Queries ---

	/** Check if stamina regen is currently allowed (past delay) (AC-008.11). */
	bool IsRegenAllowed(double CurrentTime) const;

	// --- State Management ---

	/** Notify that stamina was consumed at the given time (AC-008.11, AC-008.12). */
	void NotifyStaminaConsumed(double CurrentTime);

	/** Reset all state. */
	void Reset();

	// --- Duration Getters ---

	float GetStaminaRegenDelaySeconds() const { return StaminaRegenDelayMs / 1000.f; }

	// --- Static Helper (AC-008.8) ---

	/** Query the current tier multiplier from an ASC by reading tier tags.
	 *  Other systems (dodge, attack, block) call this to get effectiveness. */
	UFUNCTION(BlueprintCallable, Category = "Mordecai|Stamina", meta = (DisplayName = "Get Stamina Tier Multiplier"))
	static float GetTierMultiplierFromASC(UAbilitySystemComponent* ASC);

private:
	double LastConsumptionTime = -999.0;
};
