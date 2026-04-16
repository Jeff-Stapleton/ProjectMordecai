// Project Mordecai — Weapon Profile Factory (US-025)

#pragma once

#include "CoreMinimal.h"
#include "MordecaiWeaponProfileFactory.generated.h"

class UMordecaiWeaponDataAsset;

/**
 * UMordecaiWeaponProfileFactory
 *
 * Static factory that creates fully-configured weapon setups (WeaponDataAsset +
 * all AttackProfiles) for each weapon type. Used by tests and later by DataAsset
 * generation tools.
 */
UCLASS()
class LYRAGAME_API UMordecaiWeaponProfileFactory : public UObject
{
	GENERATED_BODY()

public:
	/** Create a fully-configured Longsword: 3 sweep lights + 360° sweep heavy. */
	static UMordecaiWeaponDataAsset* CreateLongsword(UObject* Outer);

	/** Create a fully-configured Greatsword: 2 wide sweep lights + slam heavy. */
	static UMordecaiWeaponDataAsset* CreateGreatsword(UObject* Outer);

	/** Create a fully-configured Shortsword: 3 sweep + 1 thrust lights + thrust heavy. */
	static UMordecaiWeaponDataAsset* CreateShortsword(UObject* Outer);

	/** Create a fully-configured Dagger: 5 thrust lights + lunge thrust heavy. */
	static UMordecaiWeaponDataAsset* CreateDagger(UObject* Outer);

	// --- Blunt & Polearm (US-026) ---

	/** Create a fully-configured Axe: 2 wide sweep lights + slam heavy. */
	static UMordecaiWeaponDataAsset* CreateAxe(UObject* Outer);

	/** Create a fully-configured Mace: sweep + slam lights + ground pound heavy. */
	static UMordecaiWeaponDataAsset* CreateMace(UObject* Outer);

	/** Create a fully-configured Spear: 2 thrust + sweep lights + lunge thrust heavy. */
	static UMordecaiWeaponDataAsset* CreateSpear(UObject* Outer);

	/** Create a fully-configured Quarterstaff: sweep+thrust+sweep lights + 360° sweep heavy. */
	static UMordecaiWeaponDataAsset* CreateQuarterstaff(UObject* Outer);

	/** Create a fully-configured Unarmed: 3 punch thrust lights + slam heavy. */
	static UMordecaiWeaponDataAsset* CreateUnarmed(UObject* Outer);
};
