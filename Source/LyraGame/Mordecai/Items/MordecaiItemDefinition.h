// Project Mordecai — Item Definition (US-032)

#pragma once

#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"
#include "Mordecai/Items/MordecaiItemTypes.h"
#include "Mordecai/Weapons/MordecaiWeaponTypes.h"

#include "MordecaiItemDefinition.generated.h"

class UTexture2D;

/**
 * UMordecaiItemDefinition
 *
 * Static schema for a single Mordecai item (mirrors item_schema_v2). One asset
 * per item variant. Carries identity, classification, storage rules, stack
 * rules, and ownership binding — but NOT runtime instance state.
 *
 * Runtime state (counts, identification status, affix rolls) lives in
 * FMordecaiItemInstance (US-031).
 */
UCLASS(BlueprintType)
class LYRAGAME_API UMordecaiItemDefinition : public UDataAsset
{
	GENERATED_BODY()

public:
	// --- Identity ---

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Mordecai|Item|Identity")
	FName ItemId;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Mordecai|Item|Identity")
	FText DisplayName;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Mordecai|Item|Identity")
	FText Description;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Mordecai|Item|Identity")
	FText ShortDescription;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Mordecai|Item|Identity")
	TSoftObjectPtr<UTexture2D> Icon;

	// --- Classification ---

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Mordecai|Item|Classification")
	EMordecaiItemType ItemType = EMordecaiItemType::Material;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Mordecai|Item|Classification")
	FName Subtype;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Mordecai|Item|Classification")
	EMordecaiItemRarity Rarity = EMordecaiItemRarity::Common;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Mordecai|Item|Classification")
	FGameplayTagContainer Tags;

	// --- Storage ---

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Mordecai|Item|Storage")
	EMordecaiCarryModel CarryModel = EMordecaiCarryModel::Unlimited;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Mordecai|Item|Storage")
	bool AutoStoreOnPickup = false;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Mordecai|Item|Storage")
	EMordecaiStorageDomain StorageDomain = EMordecaiStorageDomain::PlayerInventory;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Mordecai|Item|Storage")
	EMordecaiSortPriority SortPriority = EMordecaiSortPriority::Normal;

	// --- Stack ---

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Mordecai|Item|Stack")
	bool Stackable = false;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Mordecai|Item|Stack")
	int32 MaxStackSize = 1;

	// --- Ownership ---

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Mordecai|Item|Ownership")
	EMordecaiBindType BindType = EMordecaiBindType::None;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Mordecai|Item|Ownership")
	bool DropOnDeath = false;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Mordecai|Item|Ownership")
	bool Tradable = true;

	// --- Category helpers ---

	/** True iff AutoStoreOnPickup is set. US-031 uses this to route pickups to town storage. */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Mordecai|Item")
	bool IsAutoStored() const { return AutoStoreOnPickup; }

	/** True iff ItemType is Weapon / Armor / Trinket / MagicalItem. */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Mordecai|Item")
	bool IsEquippable() const;

	/** True iff Stackable is set AND MaxStackSize > 1. */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Mordecai|Item")
	bool IsStackable() const { return Stackable && MaxStackSize > 1; }
};
