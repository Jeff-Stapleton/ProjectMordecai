// Project Mordecai — Focused Status Effect GE (US-018)

#pragma once

#include "Mordecai/StatusEffects/MordecaiStatusEffectGameplayEffect.h"

#include "MordecaiGE_Focused.generated.h"

/**
 * UMordecaiGE_Focused
 *
 *	Focused buff: granted by perfect action streaks, rewards mastery.
 *	- Applies Mordecai.Status.Focused tag (infinite duration)
 *	- Stamina tier penalties forgiven (StaminaTierPenaltyMultiplier → 0)
 *	- Posture damage dealt bonus (OutgoingPostureDamageMultiplier + 20%)
 *	- Breaks on taking any hit (removed by PerfectActionTracker on damage)
 *	- Self-buff only — no immunity check, not blocked by Cursed
 */
UCLASS()
class LYRAGAME_API UMordecaiGE_Focused : public UMordecaiStatusEffectGameplayEffect
{
	GENERATED_BODY()

public:
	UMordecaiGE_Focused();

	/** Bonus outgoing posture damage multiplier while Focused (0.20 = 20% more). */
	UPROPERTY(EditDefaultsOnly, Category = "Mordecai|Focused")
	float FocusedPostureDamageBonus = 0.20f;
};
