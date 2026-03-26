// Project Mordecai — Frostbitten Status Effect GE (US-015)

#pragma once

#include "Mordecai/StatusEffects/MordecaiStatusEffectGameplayEffect.h"

#include "MordecaiGE_Frostbitten.generated.h"

/**
 * UMordecaiGE_Frostbitten
 *
 *	Frostbitten status effect: progressive slow + freeze at high stacks.
 *	- Applies Mordecai.Status.Frostbitten tag for duration
 *	- Reduces MoveSpeedMultiplier, AttackSpeedMultiplier (flat while active)
 *	- Increases DodgeRecoveryMultiplier (flat while active)
 *	- Stacks up to FrostbittenMaxStacks; at max stacks, triggers Frozen
 *	- Blocked by Mordecai.Immunity.Frostbitten
 */
UCLASS()
class LYRAGAME_API UMordecaiGE_Frostbitten : public UMordecaiStatusEffectGameplayEffect
{
	GENERATED_BODY()

public:
	UMordecaiGE_Frostbitten();

	UPROPERTY(EditDefaultsOnly, Category = "Mordecai|Frostbitten")
	float FrostbittenDurationSec = 6.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Mordecai|Frostbitten")
	float FrostbittenMoveSpeedReductionPct = 0.25f;

	UPROPERTY(EditDefaultsOnly, Category = "Mordecai|Frostbitten")
	float FrostbittenAttackSpeedReductionPct = 0.20f;

	UPROPERTY(EditDefaultsOnly, Category = "Mordecai|Frostbitten")
	float FrostbittenDodgeRecoveryIncreasePct = 0.30f;

	UPROPERTY(EditDefaultsOnly, Category = "Mordecai|Frostbitten")
	int32 FrostbittenMaxStacks = 5;
};
