// Project Mordecai — Frostbitten Status Effect GE (US-015)

#pragma once

#include "Mordecai/StatusEffects/MordecaiStatusEffectGameplayEffect.h"

#include "MordecaiGE_Frostbitten.generated.h"

/**
 * UMordecaiGE_Frostbitten
 *
 *	Progressive slow status effect from Frost damage (AC-015.1 through AC-015.8).
 *	- Applies Mordecai.Status.Frostbitten tag for duration
 *	- Reduces MoveSpeedMultiplier, AttackSpeedMultiplier (flat, not per-stack)
 *	- Increases DodgeRecoveryMultiplier (flat, not per-stack)
 *	- Stacks up to FrostbittenMaxStacks (AggregateByTarget)
 *	- At max stacks: triggers Frozen via component stack-change listener
 *	- Drenched amplification: bonus stacks via UMordecaiGE_Drenched::GetFrostBonusStacks
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
