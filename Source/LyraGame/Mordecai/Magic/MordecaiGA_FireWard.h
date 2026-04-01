// Project Mordecai — Fire Ward Spell (US-021)

#pragma once

#include "CoreMinimal.h"
#include "MordecaiGA_SpellBase.h"

#include "MordecaiGA_FireWard.generated.h"

/**
 * UMordecaiGA_FireWard
 *
 *	Self-targeted absorb shield spell. Creates a shield with ShieldHP = BasePower
 *	from SpellDataAsset. The shield lasts for Duration or until depleted.
 *	Applies Mordecai.Status.FireWard tag while active.
 *
 *	Only absorbs fire damage (Mordecai.Damage.Fire). Other damage types pass
 *	through unaffected. When shield HP reaches 0, the GE is removed.
 *
 *	Uses FireWardShieldHP attribute on MordecaiAttributeSet to track shield.
 *
 *	See: US-021, AC-021.8 through AC-021.10
 */
UCLASS()
class LYRAGAME_API UMordecaiGA_FireWard : public UMordecaiGA_SpellBase
{
	GENERATED_BODY()

public:
	UMordecaiGA_FireWard(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	// --- Public Getters (testable API) ---

	/** Get shield HP from SpellDataAsset BasePower (AC-021.8). */
	float GetShieldHP() const;

	/** Get ward duration from SpellDataAsset BuffDuration (AC-021.9). */
	float GetWardDuration() const;

	/** Get the ward status tag (AC-021.9). */
	FGameplayTag GetWardTag() const;

	/** Get the damage tag this ward absorbs (AC-021.10). */
	FGameplayTag GetAbsorbedDamageTag() const;

	/**
	 * Compute damage absorption (AC-021.9, AC-021.10).
	 * Pure static function for testability.
	 *
	 * @param IncomingDamage    Amount of fire damage incoming
	 * @param CurrentShieldHP   Current shield HP
	 * @param OutShieldRemaining Shield HP remaining after absorption (out param)
	 * @return Damage that passes through the shield (0 if fully absorbed)
	 */
	static float ComputeAbsorption(float IncomingDamage, float CurrentShieldHP, float& OutShieldRemaining);

	/** Check if damage type should be absorbed by Fire Ward (AC-021.10). */
	static bool ShouldAbsorbDamageType(const FGameplayTag& DamageTag);

	/** Check if ward should be removed (shield depleted) (AC-021.9). */
	static bool ShouldRemoveWard(float CurrentShieldHP);

protected:
	virtual void OnSpellCast() override;
};
