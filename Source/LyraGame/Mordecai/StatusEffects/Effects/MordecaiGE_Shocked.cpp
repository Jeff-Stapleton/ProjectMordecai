// Project Mordecai — Shocked Status Effect GE (US-015)

#include "Mordecai/StatusEffects/Effects/MordecaiGE_Shocked.h"
#include "Mordecai/MordecaiGameplayTags.h"

UMordecaiGE_Shocked::UMordecaiGE_Shocked()
{
	StatusTag = MordecaiGameplayTags::Status_Shocked;
	DurationMagnitude = FScalableFloat(ShockedDurationSec);

	// AC-015.10: Stack count stacking, up to ShockedMaxStacks
	PRAGMA_DISABLE_DEPRECATION_WARNINGS
	StackLimitCount = ShockedMaxStacks;
	PRAGMA_ENABLE_DEPRECATION_WARNINGS

	// NOTE: No attribute modifiers here. Per-stack BlockStaminaCostMultiplier
	// is managed by MordecaiStatusEffectComponent via a companion dynamic GE
	// that scales with stack count. See StartShockedTracking() / OnShockedStackChanged().

	// InitializeStatusEffect() is called automatically from PostInitProperties.
}
