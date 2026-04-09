// Project Mordecai — Exposed Status Effect GE (US-016)

#pragma once

#include "Mordecai/StatusEffects/MordecaiStatusEffectGameplayEffect.h"

#include "MordecaiGE_Exposed.generated.h"

/**
 * UMordecaiGE_Exposed
 *
 *	Exposed status effect: next incoming hit deals bonus damage.
 *	- Applies Mordecai.Status.Exposed tag for duration (or until first hit)
 *	- Increases IncomingDamageMultiplier by ExposedBonusDamagePct
 *	- Consumed on first damage received (StatusEffectComponent handles removal via NotifyDamageTaken)
 *	- Perfect block removes Exposed without consuming the hit bonus (interface for US-006)
 *	- Blocked by Mordecai.Immunity.Exposed
 */
UCLASS()
class LYRAGAME_API UMordecaiGE_Exposed : public UMordecaiStatusEffectGameplayEffect
{
	GENERATED_BODY()

public:
	UMordecaiGE_Exposed();

	UPROPERTY(EditDefaultsOnly, Category = "Mordecai|Exposed")
	float ExposedDurationSec = 5.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Mordecai|Exposed")
	float ExposedBonusDamagePct = 0.30f;
};
