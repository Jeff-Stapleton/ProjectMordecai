// Project Mordecai — Cursed Status Effect GE (US-059)

#include "Mordecai/StatusEffects/Effects/MordecaiGE_Cursed.h"
#include "Mordecai/MordecaiGameplayTags.h"
#include "Mordecai/AbilitySystem/MordecaiAttributeSet.h"

UMordecaiGE_Cursed::UMordecaiGE_Cursed()
{
	StatusTag = MordecaiGameplayTags::Status_Cursed;
	DurationMagnitude = FScalableFloat(CursedDurationSec);

	// Spell point regen reduction: Additive -0.50 on 1.0 base -> 0.50 (50% less spell regen)
	FGameplayModifierInfo SpellRegenMod;
	SpellRegenMod.Attribute = UMordecaiAttributeSet::GetSpellPointsRegenMultiplierAttribute();
	SpellRegenMod.ModifierOp = EGameplayModOp::Additive;
	SpellRegenMod.ModifierMagnitude = FGameplayEffectModifierMagnitude(FScalableFloat(-CursedSpellRegenReductionPct));
	Modifiers.Add(SpellRegenMod);

	// Healing received reduction: Additive -0.30 on 1.0 base -> 0.70 (30% less healing)
	FGameplayModifierInfo HealReductionMod;
	HealReductionMod.Attribute = UMordecaiAttributeSet::GetHealingReceivedMultiplierAttribute();
	HealReductionMod.ModifierOp = EGameplayModOp::Additive;
	HealReductionMod.ModifierMagnitude = FGameplayEffectModifierMagnitude(FScalableFloat(-CursedHealReductionPct));
	Modifiers.Add(HealReductionMod);

	// AC-059.16: While Cursed, certain buffs are disabled. Buff GEs that should be
	// suppressed by Cursed should add Mordecai.Status.Cursed to their
	// OngoingTagRequirements (i.e., they are removed/inhibited while Cursed is active).
	// TODO(DECISION): Which specific buffs are disabled by Cursed.

	// InitializeStatusEffect() is called automatically from PostInitProperties
}
