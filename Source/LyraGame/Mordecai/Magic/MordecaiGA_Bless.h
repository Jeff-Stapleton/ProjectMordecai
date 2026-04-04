// Project Mordecai — Bless Spell (US-022)

#pragma once

#include "CoreMinimal.h"
#include "MordecaiGA_SpellBase.h"

#include "MordecaiGA_Bless.generated.h"

/**
 * UMordecaiGA_Bless
 *
 *	Self-targeted buff spell. Increases PhysicalDamageMultiplier and
 *	MagicDamageMultiplier by BlessBonus (BasePower/100) for Duration seconds.
 *	Applies Mordecai.Status.Blessed tag while active.
 *
 *	Non-stacking: reapplying refreshes duration without doubling the bonus.
 *
 *	See: US-022, AC-022.1 through AC-022.3, AC-022.7, AC-022.8
 */
UCLASS()
class LYRAGAME_API UMordecaiGA_Bless : public UMordecaiGA_SpellBase
{
	GENERATED_BODY()

public:
	UMordecaiGA_Bless(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	// --- Public Getters (testable API) ---

	/** Get the BlessBonus from SpellDataAsset (BasePower/100). */
	float GetBlessBonus() const;

	/** Get Bless duration from SpellDataAsset BuffDuration (AC-022.2). */
	float GetBlessDuration() const;

	/** Get the Bless status tag (AC-022.2). */
	FGameplayTag GetBlessTag() const;

	/** Get the attributes this buff modifies (AC-022.1). */
	TArray<FGameplayAttribute> GetBuffedAttributes() const;

	/** Max stacks for Bless GE (AC-022.3). */
	int32 GetMaxBlessStacks() const { return 1; }

	/** Whether reapplying refreshes duration (AC-022.3). */
	bool GetRefreshDurationOnReapply() const { return true; }

	/**
	 * Compute BlessBonus from BasePower (AC-022.1).
	 * Pure static function for testability.
	 *
	 * @param BasePower  BasePower from SpellDataAsset (e.g. 15 = +15%)
	 * @return Additive bonus to damage multipliers (e.g. 0.15)
	 */
	static float ComputeBlessBonus(float BasePower);

protected:
	virtual void OnSpellCast() override;
};
