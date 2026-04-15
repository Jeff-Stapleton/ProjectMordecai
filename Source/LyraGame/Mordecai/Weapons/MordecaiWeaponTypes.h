// Project Mordecai — Weapon Types (US-024)

#pragma once

#include "GameplayTagContainer.h"
#include "GameplayEffectTypes.h"
#include "AttributeSet.h"

#include "MordecaiWeaponTypes.generated.h"

/** Weapon class — determines valid attack profiles, animations, and skill associations. */
UENUM(BlueprintType)
enum class EMordecaiWeaponType : uint8
{
	Longsword = 0,
	Greatsword,
	Shortsword,
	Dagger,
	Axe,
	Mace,
	Spear,
	Quarterstaff,
	Unarmed,
	Longbow,
	Shortbow,
	Crossbow,
	Throwable,
	Wand
};

/** Equipment slot — determines which hand(s) the weapon occupies. */
UENUM(BlueprintType)
enum class EMordecaiEquipSlot : uint8
{
	MainHand = 0,
	OffHand,
	TwoHand
};

/** Item rarity tier — affects affix count, stat budget, and visual treatment. */
UENUM(BlueprintType)
enum class EMordecaiItemRarity : uint8
{
	Common = 0,
	Green,
	Blue,
	Purple,
	Red
};

/** Operation type for stat modifiers applied by equipped items. */
UENUM(BlueprintType)
enum class EMordecaiModifierOp : uint8
{
	Add = 0,
	Multiply,
	PercentAdd
};

/**
 * FMordecaiStatModifier
 *
 * A single attribute modifier from an equipped item. Attribute + operation + value.
 * Applied as an infinite-duration GameplayEffect while the item is equipped.
 */
USTRUCT(BlueprintType)
struct LYRAGAME_API FMordecaiStatModifier
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Mordecai|Weapons")
	FGameplayAttribute Attribute;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Mordecai|Weapons")
	EMordecaiModifierOp Operation = EMordecaiModifierOp::Add;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Mordecai|Weapons")
	float Value = 0.f;
};

/**
 * FMordecaiWeaponInstance
 *
 * Runtime representation of a player-owned weapon. References the static
 * DataAsset plus per-instance state (affix rolls, equipped flag).
 */
USTRUCT(BlueprintType)
struct LYRAGAME_API FMordecaiWeaponInstance
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Mordecai|Weapons")
	FGuid InstanceId;

	UPROPERTY(BlueprintReadOnly, Category = "Mordecai|Weapons")
	TObjectPtr<class UMordecaiWeaponDataAsset> WeaponDataAsset = nullptr;

	UPROPERTY(BlueprintReadOnly, Category = "Mordecai|Weapons")
	TArray<FName> AffixRolls;

	UPROPERTY(BlueprintReadOnly, Category = "Mordecai|Weapons")
	bool IsEquipped = false;

	bool IsValid() const { return WeaponDataAsset != nullptr; }
};
