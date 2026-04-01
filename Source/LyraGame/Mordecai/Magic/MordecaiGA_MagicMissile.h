// Project Mordecai — Magic Missile Spell (US-020)

#pragma once

#include "CoreMinimal.h"
#include "MordecaiGA_SpellBase.h"
#include "Mordecai/Combat/MordecaiCombatTypes.h"

#include "MordecaiGA_MagicMissile.generated.h"

/**
 * UMordecaiGA_MagicMissile
 *
 *	Channeled spell that fires N projectiles over the CastTime duration.
 *	Each projectile deals force damage = SpellPower / N.
 *	Projectiles use aim assist to home toward the nearest valid target.
 *
 *	See: US-020, AC-020.7, AC-020.8, AC-020.9
 */
UCLASS()
class LYRAGAME_API UMordecaiGA_MagicMissile : public UMordecaiGA_SpellBase
{
	GENERATED_BODY()

public:
	UMordecaiGA_MagicMissile(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	// --- Public Getters (testable API) ---

	/** Get the damage type for this spell (AC-020.9). */
	EMordecaiDamageType GetSpellDamageType() const { return EMordecaiDamageType::Arcane; }

	/** Get the number of projectiles to fire (AC-020.7). */
	int32 GetProjectileCount() const;

	/** Compute damage per projectile: SpellPower / ProjectileCount (AC-020.7). */
	float GetDamagePerProjectile(float TotalSpellPower) const;

	/** Returns true — Magic Missile projectiles use homing (AC-020.8). */
	bool IsHomingEnabled() const { return true; }

protected:
	virtual void OnSpellCast() override;

	/** Fire a single Magic Missile projectile toward the best target. */
	void FireSingleMissile(float DamageMagnitude);

private:
	/** Timer handle for channeled missile firing. */
	FTimerHandle MissileTimerHandle;

	/** Missiles remaining to fire. */
	int32 MissilesRemaining = 0;

	/** Damage per missile (cached at cast start). */
	float CachedDamagePerMissile = 0.f;
};
