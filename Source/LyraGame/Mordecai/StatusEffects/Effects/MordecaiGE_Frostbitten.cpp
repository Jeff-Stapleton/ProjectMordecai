// Project Mordecai — Frostbitten Status Effect GE (US-015)

#include "Mordecai/StatusEffects/Effects/MordecaiGE_Frostbitten.h"
#include "Mordecai/MordecaiGameplayTags.h"
#include "Mordecai/AbilitySystem/MordecaiAttributeSet.h"

UMordecaiGE_Frostbitten::UMordecaiGE_Frostbitten()
{
	StatusTag = MordecaiGameplayTags::Status_Frostbitten;
	DurationMagnitude = FScalableFloat(FrostbittenDurationSec);

	// Stack count up to max, refresh duration on each application (AC-015.5)
	PRAGMA_DISABLE_DEPRECATION_WARNINGS
	StackingType = EGameplayEffectStackingType::AggregateByTarget;
	StackLimitCount = FrostbittenMaxStacks;
	StackDurationRefreshPolicy = EGameplayEffectStackingDurationPolicy::RefreshOnSuccessfulApplication;
	StackPeriodResetPolicy = EGameplayEffectStackingPeriodPolicy::NeverReset;
	StackExpirationPolicy = EGameplayEffectStackingExpirationPolicy::ClearEntireStack;
	PRAGMA_ENABLE_DEPRECATION_WARNINGS

	// Move speed reduction: Additive -0.25 on 1.0 base → 0.75 (AC-015.2)
	FGameplayModifierInfo MoveSpeedMod;
	MoveSpeedMod.Attribute = UMordecaiAttributeSet::GetMoveSpeedMultiplierAttribute();
	MoveSpeedMod.ModifierOp = EGameplayModOp::Additive;
	MoveSpeedMod.ModifierMagnitude = FGameplayEffectModifierMagnitude(FScalableFloat(-FrostbittenMoveSpeedReductionPct));
	Modifiers.Add(MoveSpeedMod);

	// Attack speed reduction: Additive -0.20 on 1.0 base → 0.80 (AC-015.3)
	FGameplayModifierInfo AttackSpeedMod;
	AttackSpeedMod.Attribute = UMordecaiAttributeSet::GetAttackSpeedMultiplierAttribute();
	AttackSpeedMod.ModifierOp = EGameplayModOp::Additive;
	AttackSpeedMod.ModifierMagnitude = FGameplayEffectModifierMagnitude(FScalableFloat(-FrostbittenAttackSpeedReductionPct));
	Modifiers.Add(AttackSpeedMod);

	// Dodge recovery increase: Additive +0.30 on 1.0 base → 1.30 (AC-015.4)
	FGameplayModifierInfo DodgeRecoveryMod;
	DodgeRecoveryMod.Attribute = UMordecaiAttributeSet::GetDodgeRecoveryMultiplierAttribute();
	DodgeRecoveryMod.ModifierOp = EGameplayModOp::Additive;
	DodgeRecoveryMod.ModifierMagnitude = FGameplayEffectModifierMagnitude(FScalableFloat(FrostbittenDodgeRecoveryIncreasePct));
	Modifiers.Add(DodgeRecoveryMod);

	// InitializeStatusEffect() is called automatically from PostInitProperties
}
