// Project Mordecai — MicroStunned Status Effect GE (US-015)

#include "Mordecai/StatusEffects/Effects/MordecaiGE_MicroStunned.h"
#include "Mordecai/MordecaiGameplayTags.h"

UMordecaiGE_MicroStunned::UMordecaiGE_MicroStunned()
{
	StatusTag = MordecaiGameplayTags::Status_MicroStunned;
	DurationMagnitude = FScalableFloat(MicroStunDurationSec);

	// No stacking — single instance (AC-015.11)
	PRAGMA_DISABLE_DEPRECATION_WARNINGS
	StackLimitCount = 1;
	PRAGMA_ENABLE_DEPRECATION_WARNINGS

	// InitializeStatusEffect() is called automatically from PostInitProperties
}
