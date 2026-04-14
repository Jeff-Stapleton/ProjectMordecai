// Project Mordecai — Status Effect Base Gameplay Effect (US-013)

#pragma once

#include "GameplayEffect.h"
#include "Mordecai/StatusEffects/MordecaiStatusEffectTypes.h"

#include "MordecaiStatusEffectGameplayEffect.generated.h"

/**
 * UMordecaiStatusEffectGameplayEffect
 *
 *	Base class for all status effect GEs (AC-013.3).
 *	Subclasses set StatusTag, DurationPolicy, and Period in their constructors,
 *	then call InitializeStatusEffect() to configure tag granting and immunity.
 *
 *	Pattern: each concrete status GE inherits from this base, sets its StatusTag,
 *	duration, period, and specific modifiers. No concrete status effects are
 *	implemented in this story (US-013) — that is US-014 through US-018.
 */
UCLASS()
class LYRAGAME_API UMordecaiStatusEffectGameplayEffect : public UGameplayEffect
{
	GENERATED_BODY()

public:
	UMordecaiStatusEffectGameplayEffect();

	/**
	 * The gameplay tag applied to the target while this status effect is active.
	 * Must be one of the Mordecai.Status.* tags.
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Mordecai|StatusEffect")
	FGameplayTag StatusTag;

	/**
	 * Stacking policy (AggregateByTarget). Default: Refresh (StackLimitCount=1).
	 * Subclasses override StackLimitCount for multi-stack effects.
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Mordecai|StatusEffect")
	EMordecaiStackingPolicy StackingPolicy = EMordecaiStackingPolicy::Refresh;

	/**
	 * Configure tag granting and immunity blocking based on StatusTag.
	 * Called automatically from PostInitProperties (safe for CDO construction).
	 * Can also be called manually for runtime instances (guarded against double-init).
	 */
	void InitializeStatusEffect();

	virtual void PostInitProperties() override;
};
