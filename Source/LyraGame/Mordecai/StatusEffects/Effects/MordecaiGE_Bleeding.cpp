// Project Mordecai — Bleeding Status Effect GE (US-014)

#include "Mordecai/StatusEffects/Effects/MordecaiGE_Bleeding.h"
#include "Mordecai/MordecaiGameplayTags.h"
#include "Mordecai/AbilitySystem/MordecaiAttributeSet.h"

UMordecaiGE_Bleeding::UMordecaiGE_Bleeding()
{
	StatusTag = MordecaiGameplayTags::Status_Bleeding;
	DurationMagnitude = FScalableFloat(BleedingDurationSec);

	// Healing reduction modifier: reduce HealingReceivedMultiplier while active
	FGameplayModifierInfo HealReductionMod;
	HealReductionMod.Attribute = UMordecaiAttributeSet::GetHealingReceivedMultiplierAttribute();
	HealReductionMod.ModifierOp = EGameplayModOp::Additive;
	HealReductionMod.ModifierMagnitude = FGameplayEffectModifierMagnitude(FScalableFloat(-BleedingHealReductionPct));
	Modifiers.Add(HealReductionMod);

	// InitializeStatusEffect() is called automatically from PostInitProperties
}
