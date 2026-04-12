// Project Mordecai — Drenched Status Effect GE (US-018)

#pragma once

#include "Mordecai/StatusEffects/MordecaiStatusEffectGameplayEffect.h"

#include "MordecaiGE_Drenched.generated.h"

class UAbilitySystemComponent;

/**
 * UMordecaiGE_Drenched
 *
 *	Drenched status effect: environmental elemental interaction modifier.
 *	- Applies Mordecai.Status.Drenched tag for duration
 *	- Reduces fire damage taken (FireDamageReceivedMultiplier)
 *	- Amplifies Shocked effects (micro-stun chance multiplier)
 *	- Amplifies Frostbitten buildup (bonus frost stacks)
 *	- Bidirectional interaction with Burning (dousing)
 *	- Blocked by Mordecai.Immunity.Drenched
 */
UCLASS()
class LYRAGAME_API UMordecaiGE_Drenched : public UMordecaiStatusEffectGameplayEffect
{
	GENERATED_BODY()

public:
	UMordecaiGE_Drenched();

	UPROPERTY(EditDefaultsOnly, Category = "Mordecai|Drenched")
	float DrenchedDurationSec = 15.0f;

	/** Fire damage reduction while Drenched (0.25 = 25% less fire damage). */
	UPROPERTY(EditDefaultsOnly, Category = "Mordecai|Drenched")
	float DrenchedFireDamageReductionPct = 0.25f;

	/** Multiplier applied to Shocked micro-stun chance while Drenched (1.50 = 50% more likely). */
	UPROPERTY(EditDefaultsOnly, Category = "Mordecai|Drenched")
	float DrenchedShockedMultiplier = 1.50f;

	/** Bonus frost stacks added when Frostbitten is applied to a Drenched target. */
	UPROPERTY(EditDefaultsOnly, Category = "Mordecai|Drenched")
	int32 DrenchedFrostBonusStacks = 1;

	/**
	 * Returns the Shocked chance multiplier if target has Drenched, 1.0 otherwise.
	 * Called by Shocked logic to check for Drenched amplification.
	 */
	static float GetShockedChanceMultiplier(const UAbilitySystemComponent* TargetASC);

	/**
	 * Returns the bonus frost stacks if target has Drenched, 0 otherwise.
	 * Called by Frostbitten logic to check for Drenched amplification.
	 */
	static int32 GetFrostBonusStacks(const UAbilitySystemComponent* TargetASC);
};
