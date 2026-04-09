// Project Mordecai — Corroded Status Effect GE (US-016)

#pragma once

#include "Mordecai/StatusEffects/MordecaiStatusEffectGameplayEffect.h"

#include "MordecaiGE_Corroded.generated.h"

/**
 * UMordecaiGE_Corroded
 *
 *	Corroded status effect: reduces block stability and armor efficiency.
 *	- Applies Mordecai.Status.Corroded tag for duration
 *	- Reduces BlockStabilityMultiplier by CorrodedBlockStabilityReductionPct
 *	- Reduces ArmorEfficiencyMultiplier by CorrodedArmorReductionPct
 *	- While Corroded, Sharpen/Repair buffs are blocked (Mordecai.Status.Corroded is a
 *	  blocking tag for those GEs — documented for Epic 6 when those GEs are created)
 *	- Blocked by Mordecai.Immunity.Corroded
 */
UCLASS()
class LYRAGAME_API UMordecaiGE_Corroded : public UMordecaiStatusEffectGameplayEffect
{
	GENERATED_BODY()

public:
	UMordecaiGE_Corroded();

	UPROPERTY(EditDefaultsOnly, Category = "Mordecai|Corroded")
	float CorrodedDurationSec = 10.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Mordecai|Corroded")
	float CorrodedBlockStabilityReductionPct = 0.30f;

	UPROPERTY(EditDefaultsOnly, Category = "Mordecai|Corroded")
	float CorrodedArmorReductionPct = 0.25f;
};
