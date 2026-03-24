// Project Mordecai

#pragma once

#include "AbilitySystemComponent.h"
#include "AbilitySystem/Attributes/LyraAttributeSet.h"

#include "MordecaiAttributeSet.generated.h"

/**
 * UMordecaiAttributeSet
 *
 *	Attribute set for Project Mordecai characters.
 *	Contains core resource attributes (Health, Stamina, SpellPoints, Posture)
 *	and primary attributes (STR, DEX, END, CON, RES, DIS, INT, WIS, CHA).
 */
UCLASS(BlueprintType)
class LYRAGAME_API UMordecaiAttributeSet : public ULyraAttributeSet
{
	GENERATED_BODY()

public:
	UMordecaiAttributeSet();

	// Core Resource Attributes
	ATTRIBUTE_ACCESSORS(UMordecaiAttributeSet, Health);
	ATTRIBUTE_ACCESSORS(UMordecaiAttributeSet, MaxHealth);
	ATTRIBUTE_ACCESSORS(UMordecaiAttributeSet, Stamina);
	ATTRIBUTE_ACCESSORS(UMordecaiAttributeSet, MaxStamina);
	ATTRIBUTE_ACCESSORS(UMordecaiAttributeSet, SpellPoints);
	ATTRIBUTE_ACCESSORS(UMordecaiAttributeSet, MaxSpellPoints);
	ATTRIBUTE_ACCESSORS(UMordecaiAttributeSet, Posture);
	ATTRIBUTE_ACCESSORS(UMordecaiAttributeSet, MaxPosture);

	// Primary Attributes (D&D-like, per character_attributes_v1)
	ATTRIBUTE_ACCESSORS(UMordecaiAttributeSet, Strength);
	ATTRIBUTE_ACCESSORS(UMordecaiAttributeSet, Dexterity);
	ATTRIBUTE_ACCESSORS(UMordecaiAttributeSet, Endurance);
	ATTRIBUTE_ACCESSORS(UMordecaiAttributeSet, Constitution);
	ATTRIBUTE_ACCESSORS(UMordecaiAttributeSet, Resistance);
	ATTRIBUTE_ACCESSORS(UMordecaiAttributeSet, Discipline);
	ATTRIBUTE_ACCESSORS(UMordecaiAttributeSet, Intelligence);
	ATTRIBUTE_ACCESSORS(UMordecaiAttributeSet, Wisdom);
	ATTRIBUTE_ACCESSORS(UMordecaiAttributeSet, Charisma);

	// Derived Attributes — Primary Effects (per character_attributes_v1 primary_stats table)
	ATTRIBUTE_ACCESSORS(UMordecaiAttributeSet, PhysicalDamageMultiplier);
	ATTRIBUTE_ACCESSORS(UMordecaiAttributeSet, AttackSpeedMultiplier);
	ATTRIBUTE_ACCESSORS(UMordecaiAttributeSet, AfflictionResistMultiplier);
	ATTRIBUTE_ACCESSORS(UMordecaiAttributeSet, MagicDamageMultiplier);
	ATTRIBUTE_ACCESSORS(UMordecaiAttributeSet, CastSpeedMultiplier);

	// Status Effect Modifier Attributes (US-014)
	ATTRIBUTE_ACCESSORS(UMordecaiAttributeSet, HealingReceivedMultiplier);
	ATTRIBUTE_ACCESSORS(UMordecaiAttributeSet, MoveSpeedMultiplier);

	// Derived Attributes — Secondary Effects (per character_attributes_v1 secondary_stats table)
	ATTRIBUTE_ACCESSORS(UMordecaiAttributeSet, ArmorPenetrationMultiplier);
	ATTRIBUTE_ACCESSORS(UMordecaiAttributeSet, PhysicalCritChance);
	ATTRIBUTE_ACCESSORS(UMordecaiAttributeSet, StaminaRegenMultiplier);
	ATTRIBUTE_ACCESSORS(UMordecaiAttributeSet, HealthRegenMultiplier);
	ATTRIBUTE_ACCESSORS(UMordecaiAttributeSet, AfflictionRecoveryMultiplier);
	ATTRIBUTE_ACCESSORS(UMordecaiAttributeSet, PostureRecoveryMultiplier);
	ATTRIBUTE_ACCESSORS(UMordecaiAttributeSet, SpellPointsRegenMultiplier);
	ATTRIBUTE_ACCESSORS(UMordecaiAttributeSet, ResistancePenetrationMultiplier);
	ATTRIBUTE_ACCESSORS(UMordecaiAttributeSet, MagicCritChance);

	/** Recalculate all derived stats from current primary attribute values. */
	void RecalculateDerivedStats();

	// Unscaled base values for resource max attributes (fixed until level progression exists)
	static constexpr float BaseMaxHealthValue = 100.0f;
	static constexpr float BaseMaxStaminaValue = 100.0f;
	static constexpr float BaseMaxPostureValue = 100.0f;
	static constexpr float BaseMaxSpellPointsValue = 10.0f;

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue) override;
	virtual void PreAttributeBaseChange(const FGameplayAttribute& Attribute, float& NewValue) const override;
	virtual void PostAttributeChange(const FGameplayAttribute& Attribute, float OldValue, float NewValue) override;

protected:
	UFUNCTION()
	void OnRep_Health(const FGameplayAttributeData& OldValue);
	UFUNCTION()
	void OnRep_MaxHealth(const FGameplayAttributeData& OldValue);
	UFUNCTION()
	void OnRep_Stamina(const FGameplayAttributeData& OldValue);
	UFUNCTION()
	void OnRep_MaxStamina(const FGameplayAttributeData& OldValue);
	UFUNCTION()
	void OnRep_SpellPoints(const FGameplayAttributeData& OldValue);
	UFUNCTION()
	void OnRep_MaxSpellPoints(const FGameplayAttributeData& OldValue);
	UFUNCTION()
	void OnRep_Posture(const FGameplayAttributeData& OldValue);
	UFUNCTION()
	void OnRep_MaxPosture(const FGameplayAttributeData& OldValue);

	UFUNCTION()
	void OnRep_Strength(const FGameplayAttributeData& OldValue);
	UFUNCTION()
	void OnRep_Dexterity(const FGameplayAttributeData& OldValue);
	UFUNCTION()
	void OnRep_Endurance(const FGameplayAttributeData& OldValue);
	UFUNCTION()
	void OnRep_Constitution(const FGameplayAttributeData& OldValue);
	UFUNCTION()
	void OnRep_Resistance(const FGameplayAttributeData& OldValue);
	UFUNCTION()
	void OnRep_Discipline(const FGameplayAttributeData& OldValue);
	UFUNCTION()
	void OnRep_Intelligence(const FGameplayAttributeData& OldValue);
	UFUNCTION()
	void OnRep_Wisdom(const FGameplayAttributeData& OldValue);
	UFUNCTION()
	void OnRep_Charisma(const FGameplayAttributeData& OldValue);

	// Derived attribute OnRep
	UFUNCTION()
	void OnRep_PhysicalDamageMultiplier(const FGameplayAttributeData& OldValue);
	UFUNCTION()
	void OnRep_AttackSpeedMultiplier(const FGameplayAttributeData& OldValue);
	UFUNCTION()
	void OnRep_AfflictionResistMultiplier(const FGameplayAttributeData& OldValue);
	UFUNCTION()
	void OnRep_MagicDamageMultiplier(const FGameplayAttributeData& OldValue);
	UFUNCTION()
	void OnRep_CastSpeedMultiplier(const FGameplayAttributeData& OldValue);
	UFUNCTION()
	void OnRep_ArmorPenetrationMultiplier(const FGameplayAttributeData& OldValue);
	UFUNCTION()
	void OnRep_PhysicalCritChance(const FGameplayAttributeData& OldValue);
	UFUNCTION()
	void OnRep_StaminaRegenMultiplier(const FGameplayAttributeData& OldValue);
	UFUNCTION()
	void OnRep_HealthRegenMultiplier(const FGameplayAttributeData& OldValue);
	UFUNCTION()
	void OnRep_AfflictionRecoveryMultiplier(const FGameplayAttributeData& OldValue);
	UFUNCTION()
	void OnRep_PostureRecoveryMultiplier(const FGameplayAttributeData& OldValue);
	UFUNCTION()
	void OnRep_SpellPointsRegenMultiplier(const FGameplayAttributeData& OldValue);
	UFUNCTION()
	void OnRep_ResistancePenetrationMultiplier(const FGameplayAttributeData& OldValue);
	UFUNCTION()
	void OnRep_MagicCritChance(const FGameplayAttributeData& OldValue);

	// Status effect modifier OnRep (US-014)
	UFUNCTION()
	void OnRep_HealingReceivedMultiplier(const FGameplayAttributeData& OldValue);
	UFUNCTION()
	void OnRep_MoveSpeedMultiplier(const FGameplayAttributeData& OldValue);

	void ClampAttribute(const FGameplayAttribute& Attribute, float& NewValue) const;

private:
	// --- Core Resources ---
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_Health, Category = "Mordecai|Resources", Meta = (AllowPrivateAccess = true))
	FGameplayAttributeData Health;

	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_MaxHealth, Category = "Mordecai|Resources", Meta = (AllowPrivateAccess = true))
	FGameplayAttributeData MaxHealth;

	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_Stamina, Category = "Mordecai|Resources", Meta = (AllowPrivateAccess = true))
	FGameplayAttributeData Stamina;

	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_MaxStamina, Category = "Mordecai|Resources", Meta = (AllowPrivateAccess = true))
	FGameplayAttributeData MaxStamina;

	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_SpellPoints, Category = "Mordecai|Resources", Meta = (AllowPrivateAccess = true))
	FGameplayAttributeData SpellPoints;

	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_MaxSpellPoints, Category = "Mordecai|Resources", Meta = (AllowPrivateAccess = true))
	FGameplayAttributeData MaxSpellPoints;

	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_Posture, Category = "Mordecai|Resources", Meta = (AllowPrivateAccess = true))
	FGameplayAttributeData Posture;

	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_MaxPosture, Category = "Mordecai|Resources", Meta = (AllowPrivateAccess = true))
	FGameplayAttributeData MaxPosture;

	// --- Primary Attributes (per character_attributes_v1) ---
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_Strength, Category = "Mordecai|Primary", Meta = (AllowPrivateAccess = true))
	FGameplayAttributeData Strength;

	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_Dexterity, Category = "Mordecai|Primary", Meta = (AllowPrivateAccess = true))
	FGameplayAttributeData Dexterity;

	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_Endurance, Category = "Mordecai|Primary", Meta = (AllowPrivateAccess = true))
	FGameplayAttributeData Endurance;

	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_Constitution, Category = "Mordecai|Primary", Meta = (AllowPrivateAccess = true))
	FGameplayAttributeData Constitution;

	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_Resistance, Category = "Mordecai|Primary", Meta = (AllowPrivateAccess = true))
	FGameplayAttributeData Resistance;

	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_Discipline, Category = "Mordecai|Primary", Meta = (AllowPrivateAccess = true))
	FGameplayAttributeData Discipline;

	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_Intelligence, Category = "Mordecai|Primary", Meta = (AllowPrivateAccess = true))
	FGameplayAttributeData Intelligence;

	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_Wisdom, Category = "Mordecai|Primary", Meta = (AllowPrivateAccess = true))
	FGameplayAttributeData Wisdom;

	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_Charisma, Category = "Mordecai|Primary", Meta = (AllowPrivateAccess = true))
	FGameplayAttributeData Charisma;

	// --- Derived Attributes: Primary Effects (multipliers default 1.0) ---
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_PhysicalDamageMultiplier, Category = "Mordecai|Derived", Meta = (AllowPrivateAccess = true))
	FGameplayAttributeData PhysicalDamageMultiplier;

	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_AttackSpeedMultiplier, Category = "Mordecai|Derived", Meta = (AllowPrivateAccess = true))
	FGameplayAttributeData AttackSpeedMultiplier;

	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_AfflictionResistMultiplier, Category = "Mordecai|Derived", Meta = (AllowPrivateAccess = true))
	FGameplayAttributeData AfflictionResistMultiplier;

	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_MagicDamageMultiplier, Category = "Mordecai|Derived", Meta = (AllowPrivateAccess = true))
	FGameplayAttributeData MagicDamageMultiplier;

	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_CastSpeedMultiplier, Category = "Mordecai|Derived", Meta = (AllowPrivateAccess = true))
	FGameplayAttributeData CastSpeedMultiplier;

	// --- Derived Attributes: Secondary Effects ---
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_ArmorPenetrationMultiplier, Category = "Mordecai|Derived", Meta = (AllowPrivateAccess = true))
	FGameplayAttributeData ArmorPenetrationMultiplier;

	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_PhysicalCritChance, Category = "Mordecai|Derived", Meta = (AllowPrivateAccess = true))
	FGameplayAttributeData PhysicalCritChance;

	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_StaminaRegenMultiplier, Category = "Mordecai|Derived", Meta = (AllowPrivateAccess = true))
	FGameplayAttributeData StaminaRegenMultiplier;

	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_HealthRegenMultiplier, Category = "Mordecai|Derived", Meta = (AllowPrivateAccess = true))
	FGameplayAttributeData HealthRegenMultiplier;

	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_AfflictionRecoveryMultiplier, Category = "Mordecai|Derived", Meta = (AllowPrivateAccess = true))
	FGameplayAttributeData AfflictionRecoveryMultiplier;

	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_PostureRecoveryMultiplier, Category = "Mordecai|Derived", Meta = (AllowPrivateAccess = true))
	FGameplayAttributeData PostureRecoveryMultiplier;

	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_SpellPointsRegenMultiplier, Category = "Mordecai|Derived", Meta = (AllowPrivateAccess = true))
	FGameplayAttributeData SpellPointsRegenMultiplier;

	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_ResistancePenetrationMultiplier, Category = "Mordecai|Derived", Meta = (AllowPrivateAccess = true))
	FGameplayAttributeData ResistancePenetrationMultiplier;

	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_MagicCritChance, Category = "Mordecai|Derived", Meta = (AllowPrivateAccess = true))
	FGameplayAttributeData MagicCritChance;

	// --- Status Effect Modifier Attributes (US-014) ---
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_HealingReceivedMultiplier, Category = "Mordecai|StatusModifiers", Meta = (AllowPrivateAccess = true))
	FGameplayAttributeData HealingReceivedMultiplier;

	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_MoveSpeedMultiplier, Category = "Mordecai|StatusModifiers", Meta = (AllowPrivateAccess = true))
	FGameplayAttributeData MoveSpeedMultiplier;
};
