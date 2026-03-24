// Project Mordecai — Poisoned Status Effect GE (US-014)

#include "Mordecai/StatusEffects/Effects/MordecaiGE_Poisoned.h"
#include "Mordecai/MordecaiGameplayTags.h"
#include "Mordecai/AbilitySystem/MordecaiAttributeSet.h"

UMordecaiGE_Poisoned::UMordecaiGE_Poisoned()
{
	StatusTag = MordecaiGameplayTags::Status_Poisoned;
	DurationMagnitude = FScalableFloat(PoisonedDurationSec);

	// Stamina regen reduction: Additive -0.60 on 1.0 base → 0.40 (60% reduction)
	FGameplayModifierInfo StaminaRegenMod;
	StaminaRegenMod.Attribute = UMordecaiAttributeSet::GetStaminaRegenMultiplierAttribute();
	StaminaRegenMod.ModifierOp = EGameplayModOp::Additive;
	StaminaRegenMod.ModifierMagnitude = FGameplayEffectModifierMagnitude(FScalableFloat(-PoisonedStaminaRegenReductionPct));
	Modifiers.Add(StaminaRegenMod);

	// Move speed reduction: Additive -0.15 on 1.0 base → 0.85 (15% reduction)
	FGameplayModifierInfo MoveSpeedMod;
	MoveSpeedMod.Attribute = UMordecaiAttributeSet::GetMoveSpeedMultiplierAttribute();
	MoveSpeedMod.ModifierOp = EGameplayModOp::Additive;
	MoveSpeedMod.ModifierMagnitude = FGameplayEffectModifierMagnitude(FScalableFloat(-PoisonedMoveSpeedReductionPct));
	Modifiers.Add(MoveSpeedMod);

	// InitializeStatusEffect() is called automatically from PostInitProperties
}
