// Project Mordecai — Silenced Status Effect GE (US-017)

#include "Mordecai/StatusEffects/Effects/MordecaiGE_Silenced.h"
#include "Mordecai/MordecaiGameplayTags.h"
#include "Mordecai/AbilitySystem/MordecaiAttributeSet.h"

UMordecaiGE_Silenced::UMordecaiGE_Silenced()
{
	StatusTag = MordecaiGameplayTags::Status_Silenced;
	DurationMagnitude = FScalableFloat(SilencedDurationSec);

	// AC-017.3: Pause SP regen — Additive -1.0 on 1.0 base -> 0.0
	FGameplayModifierInfo SPRegenMod;
	SPRegenMod.Attribute = UMordecaiAttributeSet::GetSpellPointsRegenMultiplierAttribute();
	SPRegenMod.ModifierOp = EGameplayModOp::Additive;
	SPRegenMod.ModifierMagnitude = FGameplayEffectModifierMagnitude(FScalableFloat(-1.0f));
	Modifiers.Add(SPRegenMod);

	// InitializeStatusEffect() is called automatically from PostInitProperties
}
