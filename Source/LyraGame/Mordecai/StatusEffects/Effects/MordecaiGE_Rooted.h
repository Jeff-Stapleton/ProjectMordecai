// Project Mordecai — Rooted Status Effect GE (US-017)

#pragma once

#include "Mordecai/StatusEffects/MordecaiStatusEffectGameplayEffect.h"

#include "MordecaiGE_Rooted.generated.h"

/**
 * UMordecaiGE_Rooted
 *
 *	Rooted status effect: blocks movement but allows combat actions.
 *	- Applies Mordecai.Status.Rooted tag for duration
 *	- Sets MoveSpeedMultiplier to 0 (additive -1.0 on base 1.0)
 *	- Character can still attack, block, cast spells, and use abilities
 *	- Break-free action: Mordecai.Event.BreakFree consumes stamina and removes Root
 *	- Duration refreshes on re-application (default stacking policy)
 *	- Blocked by Mordecai.Immunity.Rooted
 */
UCLASS()
class LYRAGAME_API UMordecaiGE_Rooted : public UMordecaiStatusEffectGameplayEffect
{
	GENERATED_BODY()

public:
	UMordecaiGE_Rooted();

	UPROPERTY(EditDefaultsOnly, Category = "Mordecai|Rooted")
	float RootedDurationSec = 3.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Mordecai|Rooted")
	float RootedBreakFreeStaminaCost = 25.0f;
};
