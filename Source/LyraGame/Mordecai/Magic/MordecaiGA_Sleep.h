// Project Mordecai — Sleep Spell (US-021)

#pragma once

#include "CoreMinimal.h"
#include "MordecaiGA_SpellBase.h"

#include "MordecaiGA_Sleep.generated.h"

/**
 * UMordecaiGA_Sleep
 *
 *	Single-target crowd control spell. Delivery: TraceHit (line trace in aim
 *	direction up to Range). On hit, applies a Sleep GE to the first enemy hit.
 *
 *	While sleeping: target cannot move (Rooted tag), cannot activate abilities
 *	(Sleeping tag blocks activation), cannot attack. Any damage dealt to a
 *	sleeping target removes the Sleep GE immediately (wakes them up).
 *
 *	See: US-021, AC-021.5 through AC-021.7
 */
UCLASS()
class LYRAGAME_API UMordecaiGA_Sleep : public UMordecaiGA_SpellBase
{
	GENERATED_BODY()

public:
	UMordecaiGA_Sleep(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	// --- Public Getters (testable API) ---

	/** Get sleep range from SpellDataAsset (AC-021.5). */
	float GetSleepRange() const;

	/** Get sleep duration from SpellDataAsset (AC-021.6). */
	float GetSleepDuration() const;

	/** Get the sleep status tag (AC-021.6). */
	FGameplayTag GetSleepTag() const;

	/** Get tags applied by the Sleep GE that block movement/abilities (AC-021.6). */
	TArray<FGameplayTag> GetSleepBlockedTags() const;

	/** Whether this spell should wake target on damage (AC-021.7). Always true. */
	bool ShouldWakeOnDamage() const;

protected:
	virtual void OnSpellCast() override;

private:
	/** Apply the sleep GE to a target actor's ASC. */
	void ApplySleepToTarget(AActor* Target);
};
