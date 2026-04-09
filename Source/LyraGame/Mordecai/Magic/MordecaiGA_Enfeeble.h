// Project Mordecai — Enfeeble Spell (US-023)

#pragma once

#include "CoreMinimal.h"
#include "MordecaiGA_SpellBase.h"
#include "AttributeSet.h"

#include "MordecaiGA_Enfeeble.generated.h"

/**
 * UMordecaiGA_Enfeeble
 *
 *	Single-target debuff spell. Delivery: TraceHit (line trace in aim direction
 *	up to Range). On hit, applies a Weakened GE to the first enemy hit.
 *
 *	While weakened: target's PhysicalDamageMultiplier is reduced by
 *	BasePower/100 (e.g. BasePower=25 means -0.25). Reverts when GE expires.
 *
 *	See: US-023, AC-023.4 through AC-023.6
 */
UCLASS()
class LYRAGAME_API UMordecaiGA_Enfeeble : public UMordecaiGA_SpellBase
{
	GENERATED_BODY()

public:
	UMordecaiGA_Enfeeble(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	// --- Public Getters (testable API) ---

	/** Get enfeeble range from SpellDataAsset (AC-023.4). */
	float GetEnfeebleRange() const;

	/** Get weakened duration from SpellDataAsset (AC-023.5). */
	float GetEnfeebleDuration() const;

	/** Get the weakened status tag (AC-023.5). */
	FGameplayTag GetWeakenedTag() const;

	/** Get the PhysicalDamageMultiplier reduction (BasePower/100) (AC-023.5). */
	float GetWeakenedReduction() const;

	/** Get the attribute debuffed by Enfeeble (AC-023.5). */
	FGameplayAttribute GetDebuffedAttribute() const;

	/**
	 * Compute the weakened reduction from BasePower (AC-023.5).
	 * Pure static function for testability.
	 *
	 * @param BasePower  BasePower from SpellDataAsset (e.g. 25 = 25%)
	 * @return Reduction to apply (e.g. 0.25)
	 */
	static float ComputeWeakenedReduction(float BasePower);

protected:
	virtual void OnSpellCast() override;

private:
	/** Apply the weakened GE to a target actor's ASC. */
	void ApplyWeakenedToTarget(AActor* Target);
};
