// Copyright Project Mordecai. All Rights Reserved.

#include "MordecaiAttributeSet.h"
#include "Net/UnrealNetwork.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(MordecaiAttributeSet)

UMordecaiAttributeSet::UMordecaiAttributeSet()
	: Health(100.0f)
	, MaxHealth(100.0f)
	, Stamina(100.0f)
	, MaxStamina(100.0f)
	, SpellPoints(10.0f)
	, MaxSpellPoints(10.0f)
	, Posture(100.0f)
	, MaxPosture(100.0f)
	, Strength(10.0f)
	, Dexterity(10.0f)
	, Constitution(10.0f)
	, Intelligence(10.0f)
	, Wisdom(10.0f)
	, Charisma(10.0f)
{
}

void UMordecaiAttributeSet::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	// Vitals
	DOREPLIFETIME_CONDITION_NOTIFY(ThisClass, Health, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(ThisClass, MaxHealth, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(ThisClass, Stamina, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(ThisClass, MaxStamina, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(ThisClass, SpellPoints, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(ThisClass, MaxSpellPoints, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(ThisClass, Posture, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(ThisClass, MaxPosture, COND_None, REPNOTIFY_Always);

	// D&D Ability Scores
	DOREPLIFETIME_CONDITION_NOTIFY(ThisClass, Strength, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(ThisClass, Dexterity, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(ThisClass, Constitution, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(ThisClass, Intelligence, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(ThisClass, Wisdom, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(ThisClass, Charisma, COND_None, REPNOTIFY_Always);
}

// ---- OnRep functions ----

void UMordecaiAttributeSet::OnRep_Health(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(ThisClass, Health, OldValue);
}

void UMordecaiAttributeSet::OnRep_MaxHealth(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(ThisClass, MaxHealth, OldValue);
}

void UMordecaiAttributeSet::OnRep_Stamina(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(ThisClass, Stamina, OldValue);
}

void UMordecaiAttributeSet::OnRep_MaxStamina(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(ThisClass, MaxStamina, OldValue);
}

void UMordecaiAttributeSet::OnRep_SpellPoints(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(ThisClass, SpellPoints, OldValue);
}

void UMordecaiAttributeSet::OnRep_MaxSpellPoints(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(ThisClass, MaxSpellPoints, OldValue);
}

void UMordecaiAttributeSet::OnRep_Posture(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(ThisClass, Posture, OldValue);
}

void UMordecaiAttributeSet::OnRep_MaxPosture(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(ThisClass, MaxPosture, OldValue);
}

void UMordecaiAttributeSet::OnRep_Strength(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(ThisClass, Strength, OldValue);
}

void UMordecaiAttributeSet::OnRep_Dexterity(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(ThisClass, Dexterity, OldValue);
}

void UMordecaiAttributeSet::OnRep_Constitution(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(ThisClass, Constitution, OldValue);
}

void UMordecaiAttributeSet::OnRep_Intelligence(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(ThisClass, Intelligence, OldValue);
}

void UMordecaiAttributeSet::OnRep_Wisdom(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(ThisClass, Wisdom, OldValue);
}

void UMordecaiAttributeSet::OnRep_Charisma(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(ThisClass, Charisma, OldValue);
}

// ---- Clamping ----

void UMordecaiAttributeSet::ClampAttribute(const FGameplayAttribute& Attribute, float& NewValue) const
{
	// Clamp vitals to [0, Max].
	if (Attribute == GetHealthAttribute())
	{
		NewValue = FMath::Clamp(NewValue, 0.0f, GetMaxHealth());
	}
	else if (Attribute == GetMaxHealthAttribute())
	{
		NewValue = FMath::Max(NewValue, 1.0f);
	}
	else if (Attribute == GetStaminaAttribute())
	{
		NewValue = FMath::Clamp(NewValue, 0.0f, GetMaxStamina());
	}
	else if (Attribute == GetMaxStaminaAttribute())
	{
		NewValue = FMath::Max(NewValue, 1.0f);
	}
	else if (Attribute == GetSpellPointsAttribute())
	{
		NewValue = FMath::Clamp(NewValue, 0.0f, GetMaxSpellPoints());
	}
	else if (Attribute == GetMaxSpellPointsAttribute())
	{
		NewValue = FMath::Max(NewValue, 0.0f);
	}
	else if (Attribute == GetPostureAttribute())
	{
		NewValue = FMath::Clamp(NewValue, 0.0f, GetMaxPosture());
	}
	else if (Attribute == GetMaxPostureAttribute())
	{
		NewValue = FMath::Max(NewValue, 1.0f);
	}
	// D&D scores: clamp to [1, 30] (standard D&D range).
	else if (Attribute == GetStrengthAttribute() || Attribute == GetDexterityAttribute() ||
	         Attribute == GetConstitutionAttribute() || Attribute == GetIntelligenceAttribute() ||
	         Attribute == GetWisdomAttribute() || Attribute == GetCharismaAttribute())
	{
		NewValue = FMath::Clamp(NewValue, 1.0f, 30.0f);
	}
}

void UMordecaiAttributeSet::PreAttributeBaseChange(const FGameplayAttribute& Attribute, float& NewValue) const
{
	Super::PreAttributeBaseChange(Attribute, NewValue);
	ClampAttribute(Attribute, NewValue);
}

void UMordecaiAttributeSet::PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue)
{
	Super::PreAttributeChange(Attribute, NewValue);
	ClampAttribute(Attribute, NewValue);
}
