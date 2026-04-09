// Project Mordecai — Rooted Status Effect GE (US-017)

#include "Mordecai/StatusEffects/Effects/MordecaiGE_Rooted.h"
#include "Mordecai/MordecaiGameplayTags.h"
#include "Mordecai/AbilitySystem/MordecaiAttributeSet.h"

UMordecaiGE_Rooted::UMordecaiGE_Rooted()
{
	StatusTag = MordecaiGameplayTags::Status_Rooted;
	DurationMagnitude = FScalableFloat(RootedDurationSec);

	// AC-017.7: Block movement — Additive -1.0 on 1.0 base -> 0.0
	FGameplayModifierInfo MoveSpeedMod;
	MoveSpeedMod.Attribute = UMordecaiAttributeSet::GetMoveSpeedMultiplierAttribute();
	MoveSpeedMod.ModifierOp = EGameplayModOp::Additive;
	MoveSpeedMod.ModifierMagnitude = FGameplayEffectModifierMagnitude(FScalableFloat(-1.0f));
	Modifiers.Add(MoveSpeedMod);

	// InitializeStatusEffect() is called automatically from PostInitProperties
	// AC-017.10: Duration refresh on re-apply is handled by default stacking policy (Refresh)
}
