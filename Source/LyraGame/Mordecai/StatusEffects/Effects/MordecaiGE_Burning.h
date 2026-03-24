// Project Mordecai — Burning Status Effect GE (US-014)

#pragma once

#include "GameplayEffectExecutionCalculation.h"
#include "Mordecai/StatusEffects/MordecaiStatusEffectGameplayEffect.h"

#include "MordecaiGE_Burning.generated.h"

/**
 * UMordecaiBurningExecCalc
 *
 *	Execution calculation for Burning periodic ticks.
 *	Each period: deals fire damage to Health and checks for cast interruption.
 */
UCLASS()
class LYRAGAME_API UMordecaiBurningExecCalc : public UGameplayEffectExecutionCalculation
{
	GENERATED_BODY()

public:
	UMordecaiBurningExecCalc();

	virtual void Execute_Implementation(
		const FGameplayEffectCustomExecutionParameters& ExecutionParams,
		FGameplayEffectCustomExecutionOutput& OutExecutionOutput) const override;
};

/**
 * UMordecaiGE_Burning
 *
 *	Burning status effect: periodic fire damage + cast interruption chance.
 *	- Applies Mordecai.Status.Burning tag for duration
 *	- Deals BurningDamagePerTick fire damage every BurningTickIntervalSec
 *	- Each tick has BurningInterruptChance to interrupt casting
 *	- Re-application refreshes duration (framework default stacking)
 *	- Blocked by Mordecai.Immunity.Burning
 */
UCLASS()
class LYRAGAME_API UMordecaiGE_Burning : public UMordecaiStatusEffectGameplayEffect
{
	GENERATED_BODY()

public:
	UMordecaiGE_Burning();

	UPROPERTY(EditDefaultsOnly, Category = "Mordecai|Burning")
	float BurningDurationSec = 5.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Mordecai|Burning")
	float BurningTickIntervalSec = 1.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Mordecai|Burning")
	float BurningDamagePerTick = 3.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Mordecai|Burning")
	float BurningInterruptChance = 0.15f;

	/**
	 * Static helper for cast interrupt logic (testable in isolation).
	 * Returns true if the cast was interrupted. Also sends the gameplay event.
	 */
	static bool TryInterruptCasting(UAbilitySystemComponent* TargetASC, float InterruptChance);
};
