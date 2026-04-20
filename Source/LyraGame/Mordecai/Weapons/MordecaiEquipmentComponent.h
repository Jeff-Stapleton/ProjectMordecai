// Project Mordecai — Equipment Component (US-024)

#pragma once

#include "Components/ActorComponent.h"
#include "GameplayAbilitySpec.h"
#include "ActiveGameplayEffectHandle.h"
#include "Mordecai/Weapons/MordecaiWeaponTypes.h"

#include "MordecaiEquipmentComponent.generated.h"

class UMordecaiWeaponDataAsset;
class UMordecaiAttackProfileDataAsset;
class UAbilitySystemComponent;
struct FMordecaiItemInstance;

/** Broadcast when a weapon is equipped or unequipped. NewWeapon is nullptr on unequip. */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(
	FOnMordecaiWeaponChanged,
	EMordecaiEquipSlot, ChangedSlot,
	const UMordecaiWeaponDataAsset*, NewWeapon);

/**
 * UMordecaiEquipmentComponent
 *
 * ActorComponent on the player character that manages equipped weapon state.
 * On equip: applies stat modifier GE, grants abilities, adds tags, and sets
 * the weapon's attack profiles as the active combo chain.
 * On unequip: removes all of the above and resets to Unarmed defaults.
 *
 * See: US-024, item_schema_v2.md, attack_taxonomy_v1.md
 */
UCLASS(BlueprintType, meta = (BlueprintSpawnableComponent))
class LYRAGAME_API UMordecaiEquipmentComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UMordecaiEquipmentComponent(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	// --- Equip / Unequip ---

	/**
	 * Equip a weapon to the specified slot. On equip:
	 * - Applies StatModifiers as an infinite-duration GE
	 * - Grants abilities via ASC
	 * - Adds GrantedTags to ASC
	 * - Sets weapon's LightAttackProfiles as active combo chain
	 * - Marks Weapon.IsEquipped = true
	 */
	UFUNCTION(BlueprintCallable, Category = "Mordecai|Equipment")
	bool EquipWeapon(UPARAM(ref) FMordecaiWeaponInstance& Weapon, EMordecaiEquipSlot Slot);

	/**
	 * Unequip the weapon from the specified slot. On unequip:
	 * - Removes stat modifier GE
	 * - Removes granted abilities
	 * - Removes granted tags
	 * - Resets melee attack to Unarmed defaults
	 * - Marks Weapon.IsEquipped = false
	 */
	UFUNCTION(BlueprintCallable, Category = "Mordecai|Equipment")
	bool UnequipWeapon(EMordecaiEquipSlot Slot);

	// --- Queries ---

	/** Get the weapon instance currently in the specified slot (nullptr if empty). C++ only. */
	const FMordecaiWeaponInstance* GetEquippedWeapon(EMordecaiEquipSlot Slot) const;

	/** Get mutable weapon instance in the specified slot (nullptr if empty). */
	FMordecaiWeaponInstance* GetEquippedWeaponMutable(EMordecaiEquipSlot Slot);

	/** Blueprint-friendly: returns true if a weapon is equipped in the given slot. */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Mordecai|Equipment")
	bool HasEquippedWeapon(EMordecaiEquipSlot Slot) const;

	/** Get the active light attack profiles from the MainHand weapon (or Unarmed fallback). */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Mordecai|Equipment")
	TArray<UMordecaiAttackProfileDataAsset*> GetActiveLightAttackProfiles() const;

	/** Get the active heavy attack profile from the MainHand weapon (or nullptr). */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Mordecai|Equipment")
	UMordecaiAttackProfileDataAsset* GetActiveHeavyAttackProfile() const;

	/** Get weapon BaseDamage for the MainHand weapon (0 if Unarmed). */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Mordecai|Equipment")
	float GetWeaponBaseDamage() const;

	/** Get weapon AttackSpeedMultiplier for the MainHand weapon (1.0 if Unarmed). */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Mordecai|Equipment")
	float GetWeaponAttackSpeedMultiplier() const;

	/** Get weapon PostureDamageBonus for the MainHand weapon (0 if Unarmed). */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Mordecai|Equipment")
	float GetWeaponPostureDamageBonus() const;

	// --- Weapon Cycling (US-077) ---

	/** List of weapons the player can cycle through. Populated by game logic. */
	UPROPERTY(BlueprintReadOnly, Category = "Mordecai|Equipment")
	TArray<FMordecaiWeaponInstance> AvailableWeapons;

	/** Create a weapon instance from a data asset and append to AvailableWeapons. Returns index. */
	UFUNCTION(BlueprintCallable, Category = "Mordecai|Equipment")
	int32 AddAvailableWeapon(const UMordecaiWeaponDataAsset* WeaponAsset);

	/** Equip the next weapon in AvailableWeapons (wraps). Does nothing if list is empty. */
	UFUNCTION(BlueprintCallable, Category = "Mordecai|Equipment")
	void CycleNextWeapon();

	/** Equip the previous weapon in AvailableWeapons (wraps). Does nothing if list is empty. */
	UFUNCTION(BlueprintCallable, Category = "Mordecai|Equipment")
	void CyclePrevWeapon();

	/** Index in AvailableWeapons of the currently equipped MainHand weapon, or -1. */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Mordecai|Equipment")
	int32 GetCurrentWeaponIndex() const;

	/** Number of weapons in AvailableWeapons. */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Mordecai|Equipment")
	int32 GetAvailableWeaponCount() const { return AvailableWeapons.Num(); }

	/** Broadcasts on every EquipWeapon / UnequipWeapon call. */
	UPROPERTY(BlueprintAssignable, Category = "Mordecai|Equipment")
	FOnMordecaiWeaponChanged OnWeaponChanged;

	// --- Unarmed Defaults ---

	/** Default unarmed attack profiles (designer-configurable fallback). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Mordecai|Equipment")
	TArray<TObjectPtr<UMordecaiAttackProfileDataAsset>> UnarmedAttackProfiles;

	// --- Dependency Injection (testing) ---

	/** Override the ASC reference for headless tests. */
	void SetASCOverride(UAbilitySystemComponent* InASC);

	/**
	 * US-033 hook: static helper that defers to UMordecaiItemLibrary::CanEquipInstance.
	 * Future story will thread this into the full EquipWeapon path when weapon
	 * instances get linked to FMordecaiItemInstance records.
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Mordecai|Equipment")
	static bool IsInstanceEquippable(const FMordecaiItemInstance& Instance);

	// --- Replication ---

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

private:
	/** Find the ASC on the owning actor (or use override). */
	UAbilitySystemComponent* FindAbilitySystemComponent() const;

	/** Internal: apply stat modifiers, abilities, tags for a weapon slot. */
	void ApplyWeaponEffects(const UMordecaiWeaponDataAsset* WeaponData, EMordecaiEquipSlot Slot);

	/** Internal: remove stat modifiers, abilities, tags for a weapon slot. */
	void RemoveWeaponEffects(EMordecaiEquipSlot Slot);

	// --- Slot State ---

	UPROPERTY()
	FMordecaiWeaponInstance MainHandWeapon;

	UPROPERTY()
	FMordecaiWeaponInstance OffHandWeapon;

	// --- Applied Effect Tracking (per slot) ---

	struct FSlotEffectState
	{
		FActiveGameplayEffectHandle StatModifierGEHandle;
		TArray<FGameplayAbilitySpecHandle> AbilityHandles;
		FGameplayTagContainer AppliedTags;
	};

	TMap<EMordecaiEquipSlot, FSlotEffectState> SlotEffects;

	/** Override ASC for testing. */
	UPROPERTY()
	TObjectPtr<UAbilitySystemComponent> ASCOverride;
};
