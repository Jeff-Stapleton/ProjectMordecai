// Project Mordecai — Fear Status Effect GE (US-059)

#pragma once

#include "Mordecai/StatusEffects/MordecaiStatusEffectGameplayEffect.h"

#include "MordecaiGE_Fear.generated.h"

/**
 * UMordecaiGE_Fear
 *
 *	Fear status effect: amplifies stamina penalties, reduces guard stability, applies aim drift.
 *	- Applies Mordecai.Status.Fear tag for duration
 *	- Amplifies StaminaTierPenaltyMultiplier by FearStaminaTierPenaltyMultiplier
 *	- Reduces BlockStabilityMultiplier by FearGuardStabilityReductionPct
 *	- Reduces RangedAccuracyMultiplier by FearAimDriftPct (stacks with Blinded)
 *	- Resolve mechanic (AC-059.11): PerfectBlock/PerfectParry cancels Fear — interface only
 *	- Blocked by Mordecai.Immunity.Fear
 */
UCLASS()
class LYRAGAME_API UMordecaiGE_Fear : public UMordecaiStatusEffectGameplayEffect
{
	GENERATED_BODY()

public:
	UMordecaiGE_Fear();

	UPROPERTY(EditDefaultsOnly, Category = "Mordecai|Fear")
	float FearDurationSec = 5.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Mordecai|Fear")
	float FearStaminaTierPenaltyMultiplier = 1.50f;

	UPROPERTY(EditDefaultsOnly, Category = "Mordecai|Fear")
	float FearGuardStabilityReductionPct = 0.25f;

	UPROPERTY(EditDefaultsOnly, Category = "Mordecai|Fear")
	float FearAimDriftPct = 0.15f;
};
