// Project Mordecai — Fireball Spell (US-020)

#pragma once

#include "CoreMinimal.h"
#include "MordecaiGA_SpellBase.h"
#include "Mordecai/Combat/MordecaiCombatTypes.h"

#include "MordecaiGA_Fireball.generated.h"

/**
 * UMordecaiGA_Fireball
 *
 *	Fireball spell: spawns a projectile in the caster's aim direction.
 *	On projectile hit, applies fire damage GE with magnitude = SpellPower.
 *	Projectile speed, lifetime, and collision radius are configurable
 *	via the SpellDataAsset.
 *
 *	See: US-020, AC-020.1, AC-020.2, AC-020.3
 */
UCLASS()
class LYRAGAME_API UMordecaiGA_Fireball : public UMordecaiGA_SpellBase
{
	GENERATED_BODY()

public:
	UMordecaiGA_Fireball(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	// --- Public Getters (testable API) ---

	/** Get the damage type for this spell (AC-020.3). */
	EMordecaiDamageType GetSpellDamageType() const { return EMordecaiDamageType::Fire; }

	/** Build a projectile spec from the SpellDataAsset (AC-020.2). */
	FMordecaiProjectileSpec BuildProjectileSpec() const;

	/** Build a damage profile for the fireball projectile. */
	FMordecaiDamageProfile BuildDamageProfile(float SpellPower) const;

protected:
	virtual void OnSpellCast() override;
};
