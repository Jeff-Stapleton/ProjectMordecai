// Project Mordecai — Shocked Status Effect GE (US-015)

#pragma once

#include "Mordecai/StatusEffects/MordecaiStatusEffectGameplayEffect.h"

#include "MordecaiGE_Shocked.generated.h"

class UAbilitySystemComponent;

/**
 * UMordecaiGE_Shocked
 *
 *	Timing disruption status from Lightning damage (AC-015.9 through AC-015.15).
 *	- Applies Mordecai.Status.Shocked tag for duration
 *	- Stacks up to ShockedMaxStacks (AggregateByTarget)
 *	- Per-stack BlockStaminaCostMultiplier increase (via companion GE, managed by component)
 *	- Micro-stun on hit: per-stack chance, managed by component NotifyDamageTaken
 *	- Cast interrupt on hit: per-stack chance, managed by component
 *	- Drenched amplification: multiplied micro-stun chance via GetShockedChanceMultiplier
 *	- Blocked by Mordecai.Immunity.Shocked
 *
 *	NOTE: No attribute modifiers in this GE. Per-stack BlockStaminaCostMultiplier is
 *	handled by a companion dynamic GE managed by MordecaiStatusEffectComponent.
 */
UCLASS()
class LYRAGAME_API UMordecaiGE_Shocked : public UMordecaiStatusEffectGameplayEffect
{
	GENERATED_BODY()

public:
	UMordecaiGE_Shocked();

	UPROPERTY(EditDefaultsOnly, Category = "Mordecai|Shocked")
	float ShockedDurationSec = 5.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Mordecai|Shocked")
	int32 ShockedMaxStacks = 3;

	/** Micro-stun chance per stack (0.15 = 15% per stack). */
	UPROPERTY(EditDefaultsOnly, Category = "Mordecai|Shocked")
	float ShockedMicroStunChancePerStack = 0.15f;

	/** Duration of micro-stun in seconds. */
	UPROPERTY(EditDefaultsOnly, Category = "Mordecai|Shocked")
	float ShockedMicroStunDurationMs = 300.0f;

	/** Block stamina cost increase per stack (0.20 = +20% per stack). */
	UPROPERTY(EditDefaultsOnly, Category = "Mordecai|Shocked")
	float ShockedBlockStaminaCostMultiplierPerStack = 0.20f;

	/** Cast interrupt chance per stack (0.25 = 25% per stack). */
	UPROPERTY(EditDefaultsOnly, Category = "Mordecai|Shocked")
	float ShockedCastInterruptChance = 0.25f;
};
