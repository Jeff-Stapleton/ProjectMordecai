// Project Mordecai — Blinded Status Effect GE (US-059)

#pragma once

#include "Mordecai/StatusEffects/MordecaiStatusEffectGameplayEffect.h"

#include "MordecaiGE_Blinded.generated.h"

/**
 * UMordecaiGE_Blinded
 *
 *	Blinded status effect: reduces accuracy, aim assist, and stealth detection.
 *	- Applies Mordecai.Status.Blinded tag for duration
 *	- Reduces RangedAccuracyMultiplier by BlindedAccuracyReductionPct
 *	- Reduces AimAssistMultiplier by BlindedAimAssistReductionPct
 *	- Reduces StealthDetectionMultiplier by BlindedDetectionReductionPct
 *	- Perfect dodge grants brief clarity (AC-059.5) — interface only, wiring deferred
 *	- Blocked by Mordecai.Immunity.Blinded
 */
UCLASS()
class LYRAGAME_API UMordecaiGE_Blinded : public UMordecaiStatusEffectGameplayEffect
{
	GENERATED_BODY()

public:
	UMordecaiGE_Blinded();

	UPROPERTY(EditDefaultsOnly, Category = "Mordecai|Blinded")
	float BlindedDurationSec = 4.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Mordecai|Blinded")
	float BlindedAccuracyReductionPct = 0.40f;

	UPROPERTY(EditDefaultsOnly, Category = "Mordecai|Blinded")
	float BlindedAimAssistReductionPct = 0.50f;

	UPROPERTY(EditDefaultsOnly, Category = "Mordecai|Blinded")
	float BlindedDetectionReductionPct = 0.60f;

	UPROPERTY(EditDefaultsOnly, Category = "Mordecai|Blinded")
	float BlindedClarityWindowMs = 1000.0f;
};
