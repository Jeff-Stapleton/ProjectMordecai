// Project Mordecai — Item Types (US-032)

#pragma once

#include "CoreMinimal.h"
#include "MordecaiItemTypes.generated.h"

/** Top-level item classification (mirrors item_schema_v2 Section 4). */
UENUM(BlueprintType)
enum class EMordecaiItemType : uint8
{
	Weapon = 0,
	Armor,
	Trinket,
	Consumable,
	Material,
	TownResource,
	UpgradeKey,
	QuestItem,
	MagicalItem,
	CurrencyProxy
};

/** Whether an item obeys unlimited-carry rules or has a special carry rule. */
UENUM(BlueprintType)
enum class EMordecaiCarryModel : uint8
{
	Unlimited = 0,
	SpecialRule
};

/** Where an item lives in persistent storage. */
UENUM(BlueprintType)
enum class EMordecaiStorageDomain : uint8
{
	PlayerInventory = 0,
	TownStorage,
	QuestState
};

/** Sort priority bucket used by inventory display and library helpers. Higher values sort first. */
UENUM(BlueprintType)
enum class EMordecaiSortPriority : uint8
{
	Low = 0,
	Normal,
	High,
	Signature,
	Critical
};

/** Ownership binding for items (affects dropping, trading, quest flow). */
UENUM(BlueprintType)
enum class EMordecaiBindType : uint8
{
	None = 0,
	BindOnPickup,
	BindOnEquip,
	QuestBound
};
