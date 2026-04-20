// Project Mordecai — Item Library (US-032)

#pragma once

#include "Kismet/BlueprintFunctionLibrary.h"
#include "GameplayTagContainer.h"
#include "Mordecai/Items/MordecaiItemTypes.h"
#include "Mordecai/Items/MordecaiItemInstance.h"
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

	// --- Identification helpers (US-033) ---

	/**
	 * Display name for an instance. Identified → Def->DisplayName.
	 * Unidentified + ShowPartialInfoBeforeIdentify=true → "Unidentified <Rarity> <Subtype|Type>".
	 * Unidentified + ShowPartialInfoBeforeIdentify=false → "Unknown Item".
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Mordecai|Item|Identification")
	static FText GetDisplayName(const FMordecaiItemInstance& Instance);

	/**
	 * Description for an instance. Identified → Def->Description.
	 * Unidentified + ShowPartial=true → Def->ShortDescription (or empty if missing).
	 * Unidentified + ShowPartial=false → empty FText.
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Mordecai|Item|Identification")
	static FText GetDescription(const FMordecaiItemInstance& Instance);

	/**
	 * Visible tags for an instance. Identified → full Def->Tags. Unidentified →
	 * only Mordecai.Item.Rarity.* and Mordecai.Item.Type.* tags (hides combat/
	 * damage tags that would leak identity).
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Mordecai|Item|Identification")
	static FGameplayTagContainer GetVisibleTags(const FMordecaiItemInstance& Instance);

	/**
	 * Whether the instance can currently be equipped. Blocks if the definition
	 * requires identification and the instance is still unidentified, or if the
	 * type is not equippable (see UMordecaiItemDefinition::IsEquippable).
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Mordecai|Item|Identification")
	static bool CanEquipInstance(const FMordecaiItemInstance& Instance);

	/** Return the native Mordecai.Item.Identification.* tag for the given state. */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Mordecai|Item|Identification")
	static FGameplayTag GetIdentificationTag(EMordecaiIdentificationState State);
};
