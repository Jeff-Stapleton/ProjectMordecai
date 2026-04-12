// Project Mordecai — Blur Spell (US-060)

#pragma once

#include "CoreMinimal.h"
#include "MordecaiGA_SpellBase.h"

#include "MordecaiGA_Blur.generated.h"

/**
 * UMordecaiGA_Blur
 *
 *	Self-targeted evasion buff. On cast, applies a duration GE granting the
 *	Mordecai.Status.Blurred tag and setting RangedEvasionChance attribute.
 *	While active, incoming ranged projectile attacks have a configurable
 *	miss chance (BasePower as percentage). Melee attacks are unaffected.
 *
 *	Delivery: InstantApply. Targeting: Self.
 *
 *	See: US-060, AC-060.5 through AC-060.8
 */
UCLASS()
class LYRAGAME_API UMordecaiGA_Blur : public UMordecaiGA_SpellBase
{
	GENERATED_BODY()

public:
	UMordecaiGA_Blur(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	// --- Public Getters (testable API) ---

	/** Get blur duration from BuffDuration (AC-060.6). */
	float GetBlurDuration() const;

	/** Get miss chance as fraction 0-1 from BasePower (AC-060.6, AC-060.7). */
	float GetMissChance() const;

	/** Get the Blurred status tag (AC-060.8). */
	FGameplayTag GetBlurredTag() const;

	/** Get the ability tag (AC-060.8). */
	FGameplayTag GetAbilityTag() const;

	/** Compute miss chance fraction from BasePower. Pure static for testability. */
	static float ComputeMissChance(float BasePower);

	/**
	 * Roll whether a projectile misses a blurred target (AC-060.7).
	 * Returns true if the attack MISSES (damage should be skipped).
	 * Pure static for testability — pass in evasion chance and a pre-rolled value.
	 */
	static bool RollMiss(float EvasionChance, float Roll);

protected:
	virtual void OnSpellCast() override;

private:
	void ApplyBlurGE();
};
