// Project Mordecai — Frozen Status Effect GE (US-015)

#pragma once

#include "Mordecai/StatusEffects/MordecaiStatusEffectGameplayEffect.h"

#include "MordecaiGE_Frozen.generated.h"

/**
 * UMordecaiGE_Frozen
 *
 *	Frozen status effect: full incapacitation triggered at max Frostbitten stacks.
 *	- Applies Mordecai.Status.Frozen tag for duration
 *	- While Frozen, the character cannot move, attack, block, or dodge
 *	  (abilities should check Mordecai.Status.Frozen in ActivationBlockedTags)
 *	- Auto-expires after FrozenDurationSec
 *	- No stacking — single instance only
 */
UCLASS()
class LYRAGAME_API UMordecaiGE_Frozen : public UMordecaiStatusEffectGameplayEffect
{
	GENERATED_BODY()

public:
	UMordecaiGE_Frozen();

	UPROPERTY(EditDefaultsOnly, Category = "Mordecai|Frozen")
	float FrozenDurationSec = 2.0f;
};
