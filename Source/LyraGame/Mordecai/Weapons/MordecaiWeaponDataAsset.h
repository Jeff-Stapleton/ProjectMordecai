// Project Mordecai — Weapon Data Asset (US-024)

#pragma once

#include "Engine/DataAsset.h"
#include "Mordecai/Weapons/MordecaiWeaponTypes.h"

#include "MordecaiWeaponDataAsset.generated.h"

class UMordecaiAttackProfileDataAsset;
class UGameplayEffect;
class UGameplayAbility;

/**
 * UMordecaiWeaponDataAsset
 *
 * Static data for a single weapon class. One asset per weapon variant.
 * Contains weapon identity, combat stats, attack profile chains,
 * stat modifiers, and abilities/tags granted while equipped.
 *
 * See: item_schema_v2.md, attack_taxonomy_v1.md, player_attacks_agent_brief_v1.md
 */
UCLASS(BlueprintType)
class LYRAGAME_API UMordecaiWeaponDataAsset : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	// --- Identity ---

	/** Unique weapon identifier (e.g. "Longsword_Iron"). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Mordecai|Weapons")
	FName WeaponId;

	/** Localized display name shown in UI. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Mordecai|Weapons")
	FText DisplayName;

	/** Weapon class type. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Mordecai|Weapons")
	EMordecaiWeaponType WeaponType = EMordecaiWeaponType::Unarmed;

	/** Which slot(s) this weapon occupies. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Mordecai|Weapons")
	EMordecaiEquipSlot EquipSlot = EMordecaiEquipSlot::MainHand;

	/** Item rarity tier. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Mordecai|Weapons")
	EMordecaiItemRarity Rarity = EMordecaiItemRarity::Common;

	// --- Combat Stats ---

	/** Base weapon damage added to attack power. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Mordecai|Weapons")
	float BaseDamage = 0.f;

	/** Attack speed multiplier (1.0 = normal, >1 = faster). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Mordecai|Weapons")
	float AttackSpeedMultiplier = 1.f;

	/** Weapon reach in centimeters. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Mordecai|Weapons")
	float Range = 0.f;

	/** Flat bonus to posture damage on all attacks. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Mordecai|Weapons")
	float PostureDamageBonus = 0.f;

	// --- Attack Profiles ---

	/** Ordered light attack combo chain. Index 0 = first swing. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Mordecai|Weapons")
	TArray<TObjectPtr<UMordecaiAttackProfileDataAsset>> LightAttackProfiles;

	/** Heavy/charged attack profile. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Mordecai|Weapons")
	TObjectPtr<UMordecaiAttackProfileDataAsset> HeavyAttackProfile;

	// --- Equipment Effects ---

	/** Attribute modifiers applied while equipped (via infinite-duration GE). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Mordecai|Weapons")
	TArray<FMordecaiStatModifier> StatModifiers;

	/** Abilities granted while equipped. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Mordecai|Weapons")
	TArray<TSubclassOf<UGameplayAbility>> GrantedAbilities;

	/** Tags applied to the character while equipped. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Mordecai|Weapons")
	FGameplayTagContainer GrantedTags;

	virtual FPrimaryAssetId GetPrimaryAssetId() const override;
};
