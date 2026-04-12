// Project Mordecai — Illusion Spell (US-060)

#pragma once

#include "CoreMinimal.h"
#include "MordecaiGA_SpellBase.h"

#include "MordecaiGA_Illusion.generated.h"

class AMordecaiIllusionActor;

/**
 * UMordecaiGA_Illusion
 *
 *	Summon spell. On cast, spawns an AMordecaiIllusionActor in front of the
 *	caster. The decoy draws enemy aggro via the Mordecai.Team.Ally tag.
 *	Decoy HP = BasePower, duration = BuffDuration.
 *
 *	Delivery: SummonActor. Targeting: Self (spawns relative to caster).
 *
 *	See: US-060, AC-060.1 through AC-060.4
 */
UCLASS()
class LYRAGAME_API UMordecaiGA_Illusion : public UMordecaiGA_SpellBase
{
	GENERATED_BODY()

public:
	UMordecaiGA_Illusion(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	// --- Public Getters (testable API) ---

	/** Get decoy HP from BasePower (AC-060.2). */
	float GetDecoyHP() const;

	/** Get decoy duration from BuffDuration (AC-060.2). */
	float GetDecoyDuration() const;

	/** Get the ability tag (AC-060.4). */
	FGameplayTag GetAbilityTag() const;

	/** Compute decoy HP from BasePower. Pure static for testability. */
	static float ComputeDecoyHP(float BasePower);

protected:
	virtual void OnSpellCast() override;
};
