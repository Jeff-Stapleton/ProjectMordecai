// Project Mordecai — Cursed Status Effect GE (US-059)

#pragma once

#include "Mordecai/StatusEffects/MordecaiStatusEffectGameplayEffect.h"

#include "MordecaiGE_Cursed.generated.h"

/**
 * UMordecaiGE_Cursed
 *
 *	Cursed status effect: reduces spell regen and healing received.
 *	- Applies Mordecai.Status.Cursed tag for duration
 *	- Reduces SpellPointsRegenMultiplier by CursedSpellRegenReductionPct
 *	- Reduces HealingReceivedMultiplier by CursedHealReductionPct (stacks with Bleeding)
 *	- Certain buffs are disabled while Cursed (AC-059.16) — buff GEs should check for
 *	  Mordecai.Status.Cursed in their OngoingTagRequirements. TODO(DECISION): which buffs
 *	- Blocked by Mordecai.Immunity.Cursed
 */
UCLASS()
class LYRAGAME_API UMordecaiGE_Cursed : public UMordecaiStatusEffectGameplayEffect
{
	GENERATED_BODY()

public:
	UMordecaiGE_Cursed();

	UPROPERTY(EditDefaultsOnly, Category = "Mordecai|Cursed")
	float CursedDurationSec = 10.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Mordecai|Cursed")
	float CursedSpellRegenReductionPct = 0.50f;

	UPROPERTY(EditDefaultsOnly, Category = "Mordecai|Cursed")
	float CursedHealReductionPct = 0.30f;
};
