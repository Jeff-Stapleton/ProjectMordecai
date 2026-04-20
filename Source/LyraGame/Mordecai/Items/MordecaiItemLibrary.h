// Project Mordecai — Item Library (US-032)

#pragma once

#include "Kismet/BlueprintFunctionLibrary.h"
#include "GameplayTagContainer.h"
#include "Mordecai/Items/MordecaiItemTypes.h"
#include "Mordecai/Weapons/MordecaiWeaponTypes.h"

#include "MordecaiItemLibrary.generated.h"

class UMordecaiItemDefinition;

/**
 * UMordecaiItemLibrary
 *
 * Static helpers for sorting items, default classification rules, and tag
 * lookup. No runtime state.
 */
UCLASS()
class LYRAGAME_API UMordecaiItemLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/**
	 * Compare two item definitions for inventory sort order.
	 *
	 * Returns negative if A should sort before B, positive if B before A, zero
	 * if equivalent. Order:
	 *   1. Higher SortPriority first (Critical -> Signature -> High -> Normal -> Low)
	 *   2. Tie → ItemType order: Weapon/Armor/Trinket first, then MagicalItem,
	 *      UpgradeKey, QuestItem, Consumable, Material/TownResource last
	 *   3. Tie → ItemId lexicographic
	 *   4. Null handling: nullptrs sort last (null == null → 0)
	 */
	UFUNCTION(BlueprintCallable, Category = "Mordecai|Item")
	static int32 CompareSortPriority(const UMordecaiItemDefinition* A, const UMordecaiItemDefinition* B);

	/** Returns true for Material and TownResource (per item_schema_v2 defaults), false otherwise. */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Mordecai|Item")
	static bool GetTypeDefaultAutoStore(EMordecaiItemType Type);

	/** Return the native Mordecai.Item.Type.* tag for the given item type. Invalid types return empty. */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Mordecai|Item")
	static FGameplayTag GetTypeTag(EMordecaiItemType Type);

	/** Return the native Mordecai.Item.Rarity.* tag for the given rarity. */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Mordecai|Item")
	static FGameplayTag GetRarityTag(EMordecaiItemRarity Rarity);
};
