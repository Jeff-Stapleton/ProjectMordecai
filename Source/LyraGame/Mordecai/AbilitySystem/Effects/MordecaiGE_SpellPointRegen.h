// Project Mordecai — Spell-point regeneration GameplayEffect (US-090)

#pragma once

#include "GameplayEffect.h"

#include "MordecaiGE_SpellPointRegen.generated.h"

/**
 * Infinite authoritative spell-point regeneration.
 *
 * Executes once per second, adding BaseRegenPerSecond multiplied by the
 * target's live SpellPointsRegenMultiplier. Ongoing tag requirements inhibit
 * execution while the target is Dead or Silenced without removing the effect.
 */
UCLASS()
class LYRAGAME_API UMordecaiGE_SpellPointRegen : public UGameplayEffect
{
	GENERATED_BODY()

public:
	UMordecaiGE_SpellPointRegen();

	static constexpr float BaseRegenPerSecond = 0.25f;
	static constexpr float RegenPeriodSeconds = 1.0f;

protected:
	virtual void PostInitProperties() override;
};
