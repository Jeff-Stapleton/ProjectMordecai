// Project Mordecai — Silenced Status Effect GE (US-017)

#pragma once

#include "Mordecai/StatusEffects/MordecaiStatusEffectGameplayEffect.h"

#include "MordecaiGE_Silenced.generated.h"

/**
 * UMordecaiGE_Silenced
 *
 *	Silenced status effect: blocks spellcasting and pauses SP regen.
 *	- Applies Mordecai.Status.Silenced tag for duration
 *	- Spell abilities are blocked via ActivationBlockedTags on SpellBase (US-019)
 *	- Reduces SpellPointsRegenMultiplier to 0 (additive -1.0 on base 1.0)
 *	- Weapon skills remain usable unless tagged Mordecai.Ability.Category.Magical
 *	- Blocked by Mordecai.Immunity.Silenced
 */
UCLASS()
class LYRAGAME_API UMordecaiGE_Silenced : public UMordecaiStatusEffectGameplayEffect
{
	GENERATED_BODY()

public:
	UMordecaiGE_Silenced();

	UPROPERTY(EditDefaultsOnly, Category = "Mordecai|Silenced")
	float SilencedDurationSec = 5.0f;
};
