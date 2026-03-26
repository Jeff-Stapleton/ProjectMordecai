// Project Mordecai — Frozen Status Effect GE (US-015)

#include "Mordecai/StatusEffects/Effects/MordecaiGE_Frozen.h"
#include "Mordecai/MordecaiGameplayTags.h"

UMordecaiGE_Frozen::UMordecaiGE_Frozen()
{
	StatusTag = MordecaiGameplayTags::Status_Frozen;
	DurationMagnitude = FScalableFloat(FrozenDurationSec);

	// No stacking — Frozen is a single-instance effect (AC-015.6)
	PRAGMA_DISABLE_DEPRECATION_WARNINGS
	StackLimitCount = 1;
	PRAGMA_ENABLE_DEPRECATION_WARNINGS

	// InitializeStatusEffect() is called automatically from PostInitProperties
}
