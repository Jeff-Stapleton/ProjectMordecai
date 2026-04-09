// Project Mordecai — Weakened Status Effect GE (US-016)

#pragma once

#include "Mordecai/StatusEffects/MordecaiStatusEffectGameplayEffect.h"

#include "MordecaiGE_Weakened.generated.h"

/**
 * UMordecaiGE_Weakened
 *
 *	Weakened status effect: reduces outgoing posture damage.
 *	- Applies Mordecai.Status.Weakened tag for duration
 *	- Reduces OutgoingPostureDamageMultiplier by WeakenedPostureDamageReductionPct
 *	- Heavy attacks (Mordecai.Attack.Heavy) partially bypass via WeakenedHeavyBypassPct
 *	  (bypass calculation happens in the posture damage pipeline, US-007)
 *	- Blocked by Mordecai.Immunity.Weakened
 */
UCLASS()
class LYRAGAME_API UMordecaiGE_Weakened : public UMordecaiStatusEffectGameplayEffect
{
	GENERATED_BODY()

public:
	UMordecaiGE_Weakened();

	UPROPERTY(EditDefaultsOnly, Category = "Mordecai|Weakened")
	float WeakenedDurationSec = 8.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Mordecai|Weakened")
	float WeakenedPostureDamageReductionPct = 0.40f;

	/** Fraction of the Weakened penalty that heavy attacks bypass (0.50 = heavy attacks only suffer 50% of the penalty). */
	UPROPERTY(EditDefaultsOnly, Category = "Mordecai|Weakened")
	float WeakenedHeavyBypassPct = 0.50f;
};
