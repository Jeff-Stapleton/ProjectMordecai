// Project Mordecai — Shocked Status Effect GE (US-015)

#pragma once

#include "GameplayModMagnitudeCalculation.h"
#include "Mordecai/StatusEffects/MordecaiStatusEffectGameplayEffect.h"

#include "MordecaiGE_Shocked.generated.h"

/**
 * UMordecaiShockedBlockCostMMC
 *
 *	Custom magnitude calculation for Shocked block stamina cost modifier.
 *	Returns perStackValue * currentStackCount so the modifier scales with stacks.
 */
UCLASS()
class LYRAGAME_API UMordecaiShockedBlockCostMMC : public UGameplayModMagnitudeCalculation
{
	GENERATED_BODY()

public:
	virtual float CalculateBaseMagnitude_Implementation(const FGameplayEffectSpec& Spec) const override;
};

/**
 * UMordecaiGE_Shocked
 *
 *	Shocked status effect: timing disruption via micro-stuns and block cost increase.
 *	- Applies Mordecai.Status.Shocked tag for duration
 *	- Stacks up to ShockedMaxStacks; more stacks = higher micro-stun chance
 *	- On-hit: rolls for micro-stun (chance per stack) — applies MicroStunned
 *	- On-hit while casting: rolls for cast interruption (chance per stack)
 *	- Increases BlockStaminaCostMultiplier per stack via custom MMC
 *	- Perfect block clears 1 stack (interface only — wiring deferred to US-006)
 *	- Blocked by Mordecai.Immunity.Shocked
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

	UPROPERTY(EditDefaultsOnly, Category = "Mordecai|Shocked")
	float ShockedMicroStunChancePerStack = 0.15f;

	UPROPERTY(EditDefaultsOnly, Category = "Mordecai|Shocked")
	float ShockedMicroStunDurationMs = 300.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Mordecai|Shocked")
	float ShockedBlockStaminaCostMultiplierPerStack = 0.20f;

	UPROPERTY(EditDefaultsOnly, Category = "Mordecai|Shocked")
	float ShockedCastInterruptChance = 0.25f;

	/**
	 * Static helper for micro-stun logic (testable in isolation).
	 * Rolls against Chance; on success, applies MicroStunned GE to TargetASC.
	 */
	static bool TryMicroStun(UAbilitySystemComponent* TargetASC, float Chance);

	/**
	 * Static helper for Shocked cast interrupt logic (testable in isolation).
	 * Only interrupts if target has Mordecai.State.Casting tag.
	 * On success, sends Mordecai.Event.CastInterrupted gameplay event.
	 */
	static bool TryShockedCastInterrupt(UAbilitySystemComponent* TargetASC, float Chance);
};
