// Project Mordecai — Enchant Weapon Spell (US-023)

#pragma once

#include "CoreMinimal.h"
#include "MordecaiGA_SpellBase.h"

#include "MordecaiGA_EnchantWeapon.generated.h"

/**
 * UMordecaiGA_EnchantWeapon
 *
 *	Self-targeted buff spell. Delivery: InstantApply. Applies a duration GE
 *	that adds bonus elemental damage to melee attacks.
 *
 *	While active: melee attacks deal bonus damage = BasePower (from
 *	SpellDataAsset) of Fire element (tagged Mordecai.Damage.Fire).
 *	The bonus is stored in the EnchantWeaponBonusDamage attribute and
 *	applied as a separate GE by MeleeAttack's hit processing.
 *
 *	See: US-023, AC-023.7 through AC-023.9
 */
UCLASS()
class LYRAGAME_API UMordecaiGA_EnchantWeapon : public UMordecaiGA_SpellBase
{
	GENERATED_BODY()

public:
	UMordecaiGA_EnchantWeapon(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	// --- Public Getters (testable API) ---

	/** Get enchant duration from SpellDataAsset (AC-023.8). */
	float GetEnchantDuration() const;

	/** Get bonus damage from SpellDataAsset BasePower (AC-023.8). */
	float GetEnchantBonusDamage() const;

	/** Get the enchant weapon status tag (AC-023.9). */
	FGameplayTag GetEnchantTag() const;

	/** Get the damage type tag for the enchant bonus (AC-023.8). */
	FGameplayTag GetEnchantDamageTag() const;

	/**
	 * Compute bonus damage from BasePower (AC-023.8).
	 * Pure static function for testability.
	 *
	 * @param BasePower  BasePower from SpellDataAsset
	 * @return Bonus damage to add to melee attacks
	 */
	static float ComputeBonusDamage(float BasePower);

protected:
	virtual void OnSpellCast() override;
};
