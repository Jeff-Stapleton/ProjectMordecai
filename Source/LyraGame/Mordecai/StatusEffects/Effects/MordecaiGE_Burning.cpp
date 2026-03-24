// Project Mordecai — Burning Status Effect GE (US-014)

#include "Mordecai/StatusEffects/Effects/MordecaiGE_Burning.h"
#include "Mordecai/MordecaiGameplayTags.h"
#include "Mordecai/AbilitySystem/MordecaiAttributeSet.h"
#include "AbilitySystemComponent.h"

// ---------------------------------------------------------------------------
// UMordecaiBurningExecCalc
// ---------------------------------------------------------------------------

UMordecaiBurningExecCalc::UMordecaiBurningExecCalc()
{
	// We output to Health — no need to capture input attributes
}

void UMordecaiBurningExecCalc::Execute_Implementation(
	const FGameplayEffectCustomExecutionParameters& ExecutionParams,
	FGameplayEffectCustomExecutionOutput& OutExecutionOutput) const
{
	// Read configuration from the owning GE
	const FGameplayEffectSpec& Spec = ExecutionParams.GetOwningSpec();
	const UMordecaiGE_Burning* BurningGE = Cast<UMordecaiGE_Burning>(Spec.Def);
	const float DamagePerTick = BurningGE ? BurningGE->BurningDamagePerTick : 3.0f;
	const float InterruptChance = BurningGE ? BurningGE->BurningInterruptChance : 0.15f;

	// Deal fire damage (reduce Health)
	OutExecutionOutput.AddOutputModifier(
		FGameplayModifierEvaluatedData(
			UMordecaiAttributeSet::GetHealthAttribute(),
			EGameplayModOp::Additive,
			-DamagePerTick));

	// Check for cast interruption
	UAbilitySystemComponent* TargetASC = ExecutionParams.GetTargetAbilitySystemComponent();
	if (TargetASC)
	{
		UMordecaiGE_Burning::TryInterruptCasting(TargetASC, InterruptChance);
	}
}

// ---------------------------------------------------------------------------
// UMordecaiGE_Burning
// ---------------------------------------------------------------------------

UMordecaiGE_Burning::UMordecaiGE_Burning()
{
	StatusTag = MordecaiGameplayTags::Status_Burning;
	DurationMagnitude = FScalableFloat(BurningDurationSec);
	Period = FScalableFloat(BurningTickIntervalSec);

	// Register the periodic execution calculation
	FGameplayEffectExecutionDefinition ExecDef;
	ExecDef.CalculationClass = UMordecaiBurningExecCalc::StaticClass();
	Executions.Add(ExecDef);

	// InitializeStatusEffect() is called automatically from PostInitProperties
}

bool UMordecaiGE_Burning::TryInterruptCasting(UAbilitySystemComponent* TargetASC, float InterruptChance)
{
	if (!TargetASC)
	{
		return false;
	}

	// Only interrupt if target is currently casting
	if (!TargetASC->HasMatchingGameplayTag(MordecaiGameplayTags::State_Casting))
	{
		return false;
	}

	// Roll for interrupt
	if (FMath::FRand() < InterruptChance)
	{
		FGameplayEventData EventData;
		TargetASC->HandleGameplayEvent(MordecaiGameplayTags::Event_CastInterrupted, &EventData);
		return true;
	}

	return false;
}
