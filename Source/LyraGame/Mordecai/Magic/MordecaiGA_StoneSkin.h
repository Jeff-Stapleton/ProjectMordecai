// Project Mordecai — Stone Skin Spell (US-020)

#pragma once

#include "CoreMinimal.h"
#include "MordecaiGA_SpellBase.h"

#include "MordecaiGA_StoneSkin.generated.h"

/**
 * UMordecaiGA_StoneSkin
 *
 *	Self-buff spell: applies a duration GE to the caster granting
 *	a flat DamageReduction attribute bonus. The buff applies the
 *	Mordecai.Status.StoneSkin tag and is removed when duration expires.
 *
 *	See: US-020, AC-020.10, AC-020.11
 */
UCLASS()
class LYRAGAME_API UMordecaiGA_StoneSkin : public UMordecaiGA_SpellBase
{
	GENERATED_BODY()

public:
	UMordecaiGA_StoneSkin(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	// --- Public Getters (testable API) ---

	/** Get the DamageReduction amount (read from BasePower in SpellDataAsset) (AC-020.10). */
	float GetDamageReductionAmount() const;

	/** Get the buff duration from SpellDataAsset (AC-020.11). */
	float GetBuffDuration() const;

	/** Get the status tag applied by this buff (AC-020.11). */
	FGameplayTag GetStatusTag() const;

protected:
	virtual void OnSpellCast() override;
};
