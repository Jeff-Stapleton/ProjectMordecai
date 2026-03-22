// Project Mordecai

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
	, Endurance(10.0f)
	, Constitution(10.0f)
	, Resistance(10.0f)
	, Discipline(10.0f)
	, Intelligence(10.0f)
	, Wisdom(10.0f)
	, Charisma(10.0f)
{
}

void UMordecaiAttributeSet::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	// Core resources
	DOREPLIFETIME_CONDITION_NOTIFY(UMordecaiAttributeSet, Health, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UMordecaiAttributeSet, MaxHealth, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UMordecaiAttributeSet, Stamina, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UMordecaiAttributeSet, MaxStamina, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UMordecaiAttributeSet, SpellPoints, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UMordecaiAttributeSet, MaxSpellPoints, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UMordecaiAttributeSet, Posture, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UMordecaiAttributeSet, MaxPosture, COND_None, REPNOTIFY_Always);

	// Primary attributes
	DOREPLIFETIME_CONDITION_NOTIFY(UMordecaiAttributeSet, Strength, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UMordecaiAttributeSet, Dexterity, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UMordecaiAttributeSet, Endurance, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UMordecaiAttributeSet, Constitution, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UMordecaiAttributeSet, Resistance, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UMordecaiAttributeSet, Discipline, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UMordecaiAttributeSet, Intelligence, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UMordecaiAttributeSet, Wisdom, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UMordecaiAttributeSet, Charisma, COND_None, REPNOTIFY_Always);
}

void UMordecaiAttributeSet::PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue)
{
	Super::PreAttributeChange(Attribute, NewValue);
	ClampAttribute(Attribute, NewValue);
}

void UMordecaiAttributeSet::PreAttributeBaseChange(const FGameplayAttribute& Attribute, float& NewValue) const
{
	Super::PreAttributeBaseChange(Attribute, NewValue);
	ClampAttribute(Attribute, NewValue);
}

void UMordecaiAttributeSet::ClampAttribute(const FGameplayAttribute& Attribute, float& NewValue) const
{
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
		// AC-008.1: Stamina can go negative (floor at -50 to prevent infinite debt)
		NewValue = FMath::Clamp(NewValue, -50.0f, GetMaxStamina());
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
		NewValue = FMath::Max(NewValue, 1.0f);
	}
	else if (Attribute == GetPostureAttribute())
	{
		NewValue = FMath::Clamp(NewValue, 0.0f, GetMaxPosture());
	}
	else if (Attribute == GetMaxPostureAttribute())
	{
		NewValue = FMath::Max(NewValue, 1.0f);
	}
}

// --- OnRep functions ---

void UMordecaiAttributeSet::OnRep_Health(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UMordecaiAttributeSet, Health, OldValue);
}

void UMordecaiAttributeSet::OnRep_MaxHealth(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UMordecaiAttributeSet, MaxHealth, OldValue);
}

void UMordecaiAttributeSet::OnRep_Stamina(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UMordecaiAttributeSet, Stamina, OldValue);
}

void UMordecaiAttributeSet::OnRep_MaxStamina(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UMordecaiAttributeSet, MaxStamina, OldValue);
}

void UMordecaiAttributeSet::OnRep_SpellPoints(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UMordecaiAttributeSet, SpellPoints, OldValue);
}

void UMordecaiAttributeSet::OnRep_MaxSpellPoints(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UMordecaiAttributeSet, MaxSpellPoints, OldValue);
}

void UMordecaiAttributeSet::OnRep_Posture(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UMordecaiAttributeSet, Posture, OldValue);
}

void UMordecaiAttributeSet::OnRep_MaxPosture(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UMordecaiAttributeSet, MaxPosture, OldValue);
}

void UMordecaiAttributeSet::OnRep_Strength(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UMordecaiAttributeSet, Strength, OldValue);
}

void UMordecaiAttributeSet::OnRep_Dexterity(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UMordecaiAttributeSet, Dexterity, OldValue);
}

void UMordecaiAttributeSet::OnRep_Endurance(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UMordecaiAttributeSet, Endurance, OldValue);
}

void UMordecaiAttributeSet::OnRep_Constitution(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UMordecaiAttributeSet, Constitution, OldValue);
}

void UMordecaiAttributeSet::OnRep_Resistance(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UMordecaiAttributeSet, Resistance, OldValue);
}

void UMordecaiAttributeSet::OnRep_Discipline(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UMordecaiAttributeSet, Discipline, OldValue);
}

void UMordecaiAttributeSet::OnRep_Intelligence(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UMordecaiAttributeSet, Intelligence, OldValue);
}

void UMordecaiAttributeSet::OnRep_Wisdom(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UMordecaiAttributeSet, Wisdom, OldValue);
}

void UMordecaiAttributeSet::OnRep_Charisma(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UMordecaiAttributeSet, Charisma, OldValue);
}
