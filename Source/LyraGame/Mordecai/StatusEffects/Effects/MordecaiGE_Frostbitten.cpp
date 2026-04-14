// Project Mordecai — Frostbitten Status Effect GE (US-015)

#include "Mordecai/StatusEffects/Effects/MordecaiGE_Frostbitten.h"
#include "Mordecai/MordecaiGameplayTags.h"
#include "Mordecai/AbilitySystem/MordecaiAttributeSet.h"

UMordecaiGE_Frostbitten::UMordecaiGE_Frostbitten()
{
	StatusTag = MordecaiGameplayTags::Status_Frostbitten;
	DurationMagnitude = FScalableFloat(FrostbittenDurationSec);

	// AC-015.5: Stack count stacking, up to FrostbittenMaxStacks
	PRAGMA_DISABLE_DEPRECATION_WARNINGS
	StackLimitCount = FrostbittenMaxStacks;
	PRAGMA_ENABLE_DEPRECATION_WARNINGS

	// AC-015.2: Movement speed reduction (flat, same at all stack levels)
	// Additive -0.25 on base 1.0 → 0.75 (25% reduction)
	FGameplayModifierInfo MoveSpeedMod;
	MoveSpeedMod.Attribute = UMordecaiAttributeSet::GetMoveSpeedMultiplierAttribute();
	MoveSpeedMod.ModifierOp = EGameplayModOp::Additive;
	MoveSpeedMod.ModifierMagnitude = FGameplayEffectModifierMagnitude(FScalableFloat(-FrostbittenMoveSpeedReductionPct));
	Modifiers.Add(MoveSpeedMod);

	// AC-015.3: Attack speed reduction (flat)
	// Additive -0.20 on base 1.0 → 0.80 (20% reduction)
	FGameplayModifierInfo AttackSpeedMod;
	AttackSpeedMod.Attribute = UMordecaiAttributeSet::GetAttackSpeedMultiplierAttribute();
	AttackSpeedMod.ModifierOp = EGameplayModOp::Additive;
	AttackSpeedMod.ModifierMagnitude = FGameplayEffectModifierMagnitude(FScalableFloat(-FrostbittenAttackSpeedReductionPct));
	Modifiers.Add(AttackSpeedMod);

	// AC-015.4: Dodge recovery increase (flat)
	// Additive +0.30 on base 1.0 → 1.30 (30% longer recovery)
	FGameplayModifierInfo DodgeRecoveryMod;
	DodgeRecoveryMod.Attribute = UMordecaiAttributeSet::GetDodgeRecoveryMultiplierAttribute();
	DodgeRecoveryMod.ModifierOp = EGameplayModOp::Additive;
	DodgeRecoveryMod.ModifierMagnitude = FGameplayEffectModifierMagnitude(FScalableFloat(FrostbittenDodgeRecoveryIncreasePct));
	Modifiers.Add(DodgeRecoveryMod);

	// InitializeStatusEffect() is called automatically from PostInitProperties.
	// Stack-change monitoring (Frozen trigger at max stacks) is handled by
	// UMordecaiStatusEffectComponent::StartFrostbittenTracking().
}
