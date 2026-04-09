// Project Mordecai — Brittle Status Effect GE (US-016)

#pragma once

#include "Mordecai/StatusEffects/MordecaiStatusEffectGameplayEffect.h"

#include "MordecaiGE_Brittle.generated.h"

/**
 * UMordecaiGE_Brittle
 *
 *	Brittle status effect: increases incoming posture damage and lowers posture break threshold.
 *	- Applies Mordecai.Status.Brittle tag for duration
 *	- Increases IncomingPostureDamageMultiplier by BrittlePostureDamageIncreasePct
 *	- Reduces MaxPosture by BrittleMaxPostureReductionPct (Multiply modifier)
 *	- Perfect block halves the posture damage increase (interface for US-006)
 *	- Blocked by Mordecai.Immunity.Brittle
 */
UCLASS()
class LYRAGAME_API UMordecaiGE_Brittle : public UMordecaiStatusEffectGameplayEffect
{
	GENERATED_BODY()

public:
	UMordecaiGE_Brittle();

	UPROPERTY(EditDefaultsOnly, Category = "Mordecai|Brittle")
	float BrittleDurationSec = 7.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Mordecai|Brittle")
	float BrittlePostureDamageIncreasePct = 0.35f;

	UPROPERTY(EditDefaultsOnly, Category = "Mordecai|Brittle")
	float BrittleMaxPostureReductionPct = 0.20f;
};
