// Project Mordecai — Frozen Status Effect GE (US-015)

#pragma once

#include "Mordecai/StatusEffects/MordecaiStatusEffectGameplayEffect.h"

#include "MordecaiGE_Frozen.generated.h"

/**
 * UMordecaiGE_Frozen
 *
 *	Full incapacitation triggered by reaching max Frostbitten stacks (AC-015.6).
 *	- Applies Mordecai.Status.Frozen tag for FrozenDurationSec
 *	- Blocks ALL ability activation via BlockAbilityTags GE component
 *	- Character cannot move, attack, block, or dodge while Frozen
 *	- No stacking — single instance only
 *	- Blocked by Mordecai.Immunity.Frozen
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
