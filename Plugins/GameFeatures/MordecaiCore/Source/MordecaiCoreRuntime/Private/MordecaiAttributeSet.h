// Copyright Project Mordecai. All Rights Reserved.

#pragma once

#include "AttributeSet.h"
#include "AbilitySystemComponent.h"

#include "MordecaiAttributeSet.generated.h"

// Standard GAS attribute accessor macro (same as Lyra's, defined locally to avoid coupling).
#define MORDECAI_ATTRIBUTE_ACCESSORS(ClassName, PropertyName) \
	GAMEPLAYATTRIBUTE_PROPERTY_GETTER(ClassName, PropertyName) \
	GAMEPLAYATTRIBUTE_VALUE_GETTER(PropertyName) \
	GAMEPLAYATTRIBUTE_VALUE_SETTER(PropertyName) \
	GAMEPLAYATTRIBUTE_VALUE_INITTER(PropertyName)

class UObject;
struct FFrame;

/**
 * UMordecaiAttributeSet
 *
 * Core attribute set for Project Mordecai. Extends UAttributeSet directly (fresh set,
 * not derived from ULyraAttributeSet) per architecture spec.
 *
 * Contains:
 * - Vital stats: Health, Stamina, SpellPoints, Posture (with max counterparts)
 * - D&D ability scores: STR, DEX, CON, INT, WIS, CHA (stubbed, not wired to derived stats)
 */
UCLASS(BlueprintType)
class UMordecaiAttributeSet : public UAttributeSet
{
	GENERATED_BODY()

public:
	UMordecaiAttributeSet();

	//~ UAttributeSet interface
	virtual void PreAttributeBaseChange(const FGameplayAttribute& Attribute, float& NewValue) const override;
	virtual void PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue) override;
	virtual void GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const override;
	//~ End UAttributeSet interface

	// ---- Vital Attributes ----

	MORDECAI_ATTRIBUTE_ACCESSORS(ThisClass, Health);
	MORDECAI_ATTRIBUTE_ACCESSORS(ThisClass, MaxHealth);
	MORDECAI_ATTRIBUTE_ACCESSORS(ThisClass, Stamina);
	MORDECAI_ATTRIBUTE_ACCESSORS(ThisClass, MaxStamina);
	MORDECAI_ATTRIBUTE_ACCESSORS(ThisClass, SpellPoints);
	MORDECAI_ATTRIBUTE_ACCESSORS(ThisClass, MaxSpellPoints);
	MORDECAI_ATTRIBUTE_ACCESSORS(ThisClass, Posture);
	MORDECAI_ATTRIBUTE_ACCESSORS(ThisClass, MaxPosture);

	// ---- D&D Ability Scores (stubbed) ----

	MORDECAI_ATTRIBUTE_ACCESSORS(ThisClass, Strength);
	MORDECAI_ATTRIBUTE_ACCESSORS(ThisClass, Dexterity);
	MORDECAI_ATTRIBUTE_ACCESSORS(ThisClass, Constitution);
	MORDECAI_ATTRIBUTE_ACCESSORS(ThisClass, Intelligence);
	MORDECAI_ATTRIBUTE_ACCESSORS(ThisClass, Wisdom);
	MORDECAI_ATTRIBUTE_ACCESSORS(ThisClass, Charisma);

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
	void OnRep_Constitution(const FGameplayAttributeData& OldValue);
	UFUNCTION()
	void OnRep_Intelligence(const FGameplayAttributeData& OldValue);
	UFUNCTION()
	void OnRep_Wisdom(const FGameplayAttributeData& OldValue);
	UFUNCTION()
	void OnRep_Charisma(const FGameplayAttributeData& OldValue);

	void ClampAttribute(const FGameplayAttribute& Attribute, float& NewValue) const;

private:
	// ---- Vital Attributes ----

	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_Health, Category = "Mordecai|Vitals", Meta = (AllowPrivateAccess = true))
	FGameplayAttributeData Health;

	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_MaxHealth, Category = "Mordecai|Vitals", Meta = (AllowPrivateAccess = true))
	FGameplayAttributeData MaxHealth;

	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_Stamina, Category = "Mordecai|Vitals", Meta = (AllowPrivateAccess = true))
	FGameplayAttributeData Stamina;

	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_MaxStamina, Category = "Mordecai|Vitals", Meta = (AllowPrivateAccess = true))
	FGameplayAttributeData MaxStamina;

	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_SpellPoints, Category = "Mordecai|Vitals", Meta = (AllowPrivateAccess = true))
	FGameplayAttributeData SpellPoints;

	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_MaxSpellPoints, Category = "Mordecai|Vitals", Meta = (AllowPrivateAccess = true))
	FGameplayAttributeData MaxSpellPoints;

	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_Posture, Category = "Mordecai|Vitals", Meta = (AllowPrivateAccess = true))
	FGameplayAttributeData Posture;

	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_MaxPosture, Category = "Mordecai|Vitals", Meta = (AllowPrivateAccess = true))
	FGameplayAttributeData MaxPosture;

	// ---- D&D Ability Scores ----
	// TODO(DECISION): Determine how D&D stats map to derived gameplay stats (damage bonuses, dodge chance, etc.)

	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_Strength, Category = "Mordecai|AbilityScores", Meta = (AllowPrivateAccess = true))
	FGameplayAttributeData Strength;

	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_Dexterity, Category = "Mordecai|AbilityScores", Meta = (AllowPrivateAccess = true))
	FGameplayAttributeData Dexterity;

	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_Constitution, Category = "Mordecai|AbilityScores", Meta = (AllowPrivateAccess = true))
	FGameplayAttributeData Constitution;

	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_Intelligence, Category = "Mordecai|AbilityScores", Meta = (AllowPrivateAccess = true))
	FGameplayAttributeData Intelligence;

	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_Wisdom, Category = "Mordecai|AbilityScores", Meta = (AllowPrivateAccess = true))
	FGameplayAttributeData Wisdom;

	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_Charisma, Category = "Mordecai|AbilityScores", Meta = (AllowPrivateAccess = true))
	FGameplayAttributeData Charisma;
};
