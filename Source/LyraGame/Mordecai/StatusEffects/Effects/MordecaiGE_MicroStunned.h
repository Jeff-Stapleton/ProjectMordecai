// Project Mordecai — MicroStunned Status Effect GE (US-015)

#pragma once

#include "Mordecai/StatusEffects/MordecaiStatusEffectGameplayEffect.h"

#include "MordecaiGE_MicroStunned.generated.h"

/**
 * UMordecaiGE_MicroStunned
 *
 *	Very short duration stun applied by Shocked on-hit mechanic.
 *	- Applies Mordecai.Status.MicroStunned tag for MicroStunDurationSec
 *	- Briefly interrupts current action (caller cancels active ability)
 *	- The tag alone blocks new activations for its brief duration
 *	- No stacking — single instance only
 */
UCLASS()
class LYRAGAME_API UMordecaiGE_MicroStunned : public UMordecaiStatusEffectGameplayEffect
{
	GENERATED_BODY()

public:
	UMordecaiGE_MicroStunned();

	UPROPERTY(EditDefaultsOnly, Category = "Mordecai|MicroStunned")
	float MicroStunDurationSec = 0.3f;
};
