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

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue) override;
	virtual void PreAttributeBaseChange(const FGameplayAttribute& Attribute, float& NewValue) const override;

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
};
