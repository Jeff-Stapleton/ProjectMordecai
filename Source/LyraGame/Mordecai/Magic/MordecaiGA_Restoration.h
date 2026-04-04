// Project Mordecai — Restoration Spell (US-022)

#pragma once

#include "CoreMinimal.h"
#include "MordecaiGA_SpellBase.h"

#include "MordecaiGA_Restoration.generated.h"

/**
 * UMordecaiGA_Restoration
 *
 *	Self-targeted heal-over-time spell. Applies a periodic HoT GE that
 *	restores health each second for Duration seconds. Total heal = SpellPower
 *	(BasePower scaled by attribute/rank). Each tick heals TotalHeal / Duration.
 *	Health cannot exceed MaxHealth.
 *
 *	Applies Mordecai.Status.Restoration tag while active.
 *	Reapplying refreshes duration and recalculates heal amount.
 *
 *	See: US-022, AC-022.4 through AC-022.6, AC-022.7, AC-022.8
 */
UCLASS()
class LYRAGAME_API UMordecaiGA_Restoration : public UMordecaiGA_SpellBase
{
	GENERATED_BODY()

public:
	UMordecaiGA_Restoration(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	// --- Public Getters (testable API) ---

	/** Get HoT duration from SpellDataAsset BuffDuration (AC-022.6). */
	float GetRestorationDuration() const;

	/** Get the Restoration status tag (AC-022.6). */
	FGameplayTag GetRestorationTag() const;

	/** Get the tick period in seconds (AC-022.5). */
	float GetTickPeriod() const { return 1.0f; }

	/** Max stacks for Restoration GE (AC-022.6). */
	int32 GetMaxRestorationStacks() const { return 1; }

	/** Whether reapplying refreshes duration (AC-022.6). */
	bool GetRefreshDurationOnReapply() const { return true; }

	/**
	 * Compute heal per tick (AC-022.5).
	 * Pure static function for testability.
	 *
	 * @param TotalHeal  Total heal amount (SpellPower)
	 * @param Duration   Duration in seconds
	 * @return Heal amount per 1-second tick
	 */
	static float ComputeHealPerTick(float TotalHeal, float Duration);

	/**
	 * Clamp heal to not exceed MaxHealth (AC-022.5).
	 * Pure static function for testability.
	 *
	 * @param CurrentHealth  Current health value
	 * @param HealAmount     Desired heal amount
	 * @param MaxHealth      Maximum health
	 * @return Actual heal amount (clamped)
	 */
	static float ClampHeal(float CurrentHealth, float HealAmount, float MaxHealth);

protected:
	virtual void OnSpellCast() override;
};
