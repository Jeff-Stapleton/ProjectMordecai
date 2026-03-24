// Project Mordecai — Poisoned Status Effect GE (US-014)

#pragma once

#include "Mordecai/StatusEffects/MordecaiStatusEffectGameplayEffect.h"

#include "MordecaiGE_Poisoned.generated.h"

/**
 * UMordecaiGE_Poisoned
 *
 *	Poisoned status effect: stamina regen suppression + movement reduction.
 *	- Applies Mordecai.Status.Poisoned tag for duration
 *	- Reduces StaminaRegenMultiplier by PoisonedStaminaRegenReductionPct
 *	- Reduces MoveSpeedMultiplier by PoisonedMoveSpeedReductionPct
 *	- Blocked by Mordecai.Immunity.Poisoned
 */
UCLASS()
class LYRAGAME_API UMordecaiGE_Poisoned : public UMordecaiStatusEffectGameplayEffect
{
	GENERATED_BODY()

public:
	UMordecaiGE_Poisoned();

	UPROPERTY(EditDefaultsOnly, Category = "Mordecai|Poisoned")
	float PoisonedDurationSec = 8.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Mordecai|Poisoned")
	float PoisonedStaminaRegenReductionPct = 0.60f;

	UPROPERTY(EditDefaultsOnly, Category = "Mordecai|Poisoned")
	float PoisonedMoveSpeedReductionPct = 0.15f;
};
