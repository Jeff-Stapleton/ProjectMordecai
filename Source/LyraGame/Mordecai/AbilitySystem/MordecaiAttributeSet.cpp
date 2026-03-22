// Project Mordecai

#include "MordecaiAttributeSet.h"
#include "MordecaiAttributeScaling.h"
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
	// Derived: multipliers default 1.0 (neutral), crit chances default 0.0 (AC-010.25)
	, PhysicalDamageMultiplier(1.0f)
	, AttackSpeedMultiplier(1.0f)
	, AfflictionResistMultiplier(1.0f)
	, MagicDamageMultiplier(1.0f)
	, CastSpeedMultiplier(1.0f)
	, ArmorPenetrationMultiplier(1.0f)
	, PhysicalCritChance(0.0f)
	, StaminaRegenMultiplier(1.0f)
	, HealthRegenMultiplier(1.0f)
	, AfflictionRecoveryMultiplier(1.0f)
	, PostureRecoveryMultiplier(1.0f)
	, SpellPointsRegenMultiplier(1.0f)
	, ResistancePenetrationMultiplier(1.0f)
	, MagicCritChance(0.0f)
{
}

void UMordecaiAttributeSet::RecalculateDerivedStats()
{
	const float EffStr = FMordecaiAttributeScaling::CalculateEffectiveMod(GetStrength());
	const float EffDex = FMordecaiAttributeScaling::CalculateEffectiveMod(GetDexterity());
	const float EffEnd = FMordecaiAttributeScaling::CalculateEffectiveMod(GetEndurance());
	const float EffCon = FMordecaiAttributeScaling::CalculateEffectiveMod(GetConstitution());
	const float EffRes = FMordecaiAttributeScaling::CalculateEffectiveMod(GetResistance());
	const float EffDis = FMordecaiAttributeScaling::CalculateEffectiveMod(GetDiscipline());
	const float EffInt = FMordecaiAttributeScaling::CalculateEffectiveMod(GetIntelligence());
	const float EffWis = FMordecaiAttributeScaling::CalculateEffectiveMod(GetWisdom());
	const float EffCha = FMordecaiAttributeScaling::CalculateEffectiveMod(GetCharisma());

	// Primary effects (character_attributes_v1 primary_stats table)
	PhysicalDamageMultiplier.SetBaseValue(1.0f + 0.03f * EffStr);
	PhysicalDamageMultiplier.SetCurrentValue(1.0f + 0.03f * EffStr);

	AttackSpeedMultiplier.SetBaseValue(1.0f + 0.015f * EffDex);
	AttackSpeedMultiplier.SetCurrentValue(1.0f + 0.015f * EffDex);

	MaxStamina.SetBaseValue(BaseMaxStaminaValue * (1.0f + 0.04f * EffEnd));
	MaxStamina.SetCurrentValue(BaseMaxStaminaValue * (1.0f + 0.04f * EffEnd));

	MaxHealth.SetBaseValue(BaseMaxHealthValue * (1.0f + 0.06f * EffCon));
	MaxHealth.SetCurrentValue(BaseMaxHealthValue * (1.0f + 0.06f * EffCon));

	AfflictionResistMultiplier.SetBaseValue(1.0f + 0.03f * EffRes);
	AfflictionResistMultiplier.SetCurrentValue(1.0f + 0.03f * EffRes);

	MaxPosture.SetBaseValue(BaseMaxPostureValue * (1.0f + 0.04f * EffDis));
	MaxPosture.SetCurrentValue(BaseMaxPostureValue * (1.0f + 0.04f * EffDis));

	MaxSpellPoints.SetBaseValue(BaseMaxSpellPointsValue * (1.0f + 0.04f * EffInt));
	MaxSpellPoints.SetCurrentValue(BaseMaxSpellPointsValue * (1.0f + 0.04f * EffInt));

	MagicDamageMultiplier.SetBaseValue(1.0f + 0.03f * EffWis);
	MagicDamageMultiplier.SetCurrentValue(1.0f + 0.03f * EffWis);

	CastSpeedMultiplier.SetBaseValue(1.0f + 0.015f * EffCha);
	CastSpeedMultiplier.SetCurrentValue(1.0f + 0.015f * EffCha);

	// Secondary effects (character_attributes_v1 secondary_stats table)
	ArmorPenetrationMultiplier.SetBaseValue(1.0f + 0.015f * EffStr);
	ArmorPenetrationMultiplier.SetCurrentValue(1.0f + 0.015f * EffStr);

	PhysicalCritChance.SetBaseValue(0.0075f * EffDex);
	PhysicalCritChance.SetCurrentValue(0.0075f * EffDex);

	StaminaRegenMultiplier.SetBaseValue(1.0f + 0.02f * EffEnd);
	StaminaRegenMultiplier.SetCurrentValue(1.0f + 0.02f * EffEnd);

	HealthRegenMultiplier.SetBaseValue(1.0f + 0.015f * EffCon);
	HealthRegenMultiplier.SetCurrentValue(1.0f + 0.015f * EffCon);

	AfflictionRecoveryMultiplier.SetBaseValue(1.0f + 0.015f * EffRes);
	AfflictionRecoveryMultiplier.SetCurrentValue(1.0f + 0.015f * EffRes);

	PostureRecoveryMultiplier.SetBaseValue(1.0f + 0.02f * EffDis);
	PostureRecoveryMultiplier.SetCurrentValue(1.0f + 0.02f * EffDis);

	SpellPointsRegenMultiplier.SetBaseValue(1.0f + 0.02f * EffInt);
	SpellPointsRegenMultiplier.SetCurrentValue(1.0f + 0.02f * EffInt);

	ResistancePenetrationMultiplier.SetBaseValue(1.0f + 0.0125f * EffWis);
	ResistancePenetrationMultiplier.SetCurrentValue(1.0f + 0.0125f * EffWis);

	MagicCritChance.SetBaseValue(0.0075f * EffCha);
	MagicCritChance.SetCurrentValue(0.0075f * EffCha);
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

	// Derived attributes
	DOREPLIFETIME_CONDITION_NOTIFY(UMordecaiAttributeSet, PhysicalDamageMultiplier, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UMordecaiAttributeSet, AttackSpeedMultiplier, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UMordecaiAttributeSet, AfflictionResistMultiplier, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UMordecaiAttributeSet, MagicDamageMultiplier, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UMordecaiAttributeSet, CastSpeedMultiplier, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UMordecaiAttributeSet, ArmorPenetrationMultiplier, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UMordecaiAttributeSet, PhysicalCritChance, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UMordecaiAttributeSet, StaminaRegenMultiplier, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UMordecaiAttributeSet, HealthRegenMultiplier, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UMordecaiAttributeSet, AfflictionRecoveryMultiplier, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UMordecaiAttributeSet, PostureRecoveryMultiplier, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UMordecaiAttributeSet, SpellPointsRegenMultiplier, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UMordecaiAttributeSet, ResistancePenetrationMultiplier, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UMordecaiAttributeSet, MagicCritChance, COND_None, REPNOTIFY_Always);
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

void UMordecaiAttributeSet::PostAttributeChange(const FGameplayAttribute& Attribute, float OldValue, float NewValue)
{
	Super::PostAttributeChange(Attribute, OldValue, NewValue);

	// When a primary attribute changes, recalculate all derived stats (AC-010.24)
	if (Attribute == GetStrengthAttribute()
		|| Attribute == GetDexterityAttribute()
		|| Attribute == GetEnduranceAttribute()
		|| Attribute == GetConstitutionAttribute()
		|| Attribute == GetResistanceAttribute()
		|| Attribute == GetDisciplineAttribute()
		|| Attribute == GetIntelligenceAttribute()
		|| Attribute == GetWisdomAttribute()
		|| Attribute == GetCharismaAttribute())
	{
		RecalculateDerivedStats();
	}
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

// --- Derived attribute OnRep functions ---

void UMordecaiAttributeSet::OnRep_PhysicalDamageMultiplier(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UMordecaiAttributeSet, PhysicalDamageMultiplier, OldValue);
}

void UMordecaiAttributeSet::OnRep_AttackSpeedMultiplier(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UMordecaiAttributeSet, AttackSpeedMultiplier, OldValue);
}

void UMordecaiAttributeSet::OnRep_AfflictionResistMultiplier(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UMordecaiAttributeSet, AfflictionResistMultiplier, OldValue);
}

void UMordecaiAttributeSet::OnRep_MagicDamageMultiplier(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UMordecaiAttributeSet, MagicDamageMultiplier, OldValue);
}

void UMordecaiAttributeSet::OnRep_CastSpeedMultiplier(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UMordecaiAttributeSet, CastSpeedMultiplier, OldValue);
}

void UMordecaiAttributeSet::OnRep_ArmorPenetrationMultiplier(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UMordecaiAttributeSet, ArmorPenetrationMultiplier, OldValue);
}

void UMordecaiAttributeSet::OnRep_PhysicalCritChance(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UMordecaiAttributeSet, PhysicalCritChance, OldValue);
}

void UMordecaiAttributeSet::OnRep_StaminaRegenMultiplier(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UMordecaiAttributeSet, StaminaRegenMultiplier, OldValue);
}

void UMordecaiAttributeSet::OnRep_HealthRegenMultiplier(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UMordecaiAttributeSet, HealthRegenMultiplier, OldValue);
}

void UMordecaiAttributeSet::OnRep_AfflictionRecoveryMultiplier(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UMordecaiAttributeSet, AfflictionRecoveryMultiplier, OldValue);
}

void UMordecaiAttributeSet::OnRep_PostureRecoveryMultiplier(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UMordecaiAttributeSet, PostureRecoveryMultiplier, OldValue);
}

void UMordecaiAttributeSet::OnRep_SpellPointsRegenMultiplier(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UMordecaiAttributeSet, SpellPointsRegenMultiplier, OldValue);
}

void UMordecaiAttributeSet::OnRep_ResistancePenetrationMultiplier(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UMordecaiAttributeSet, ResistancePenetrationMultiplier, OldValue);
}

void UMordecaiAttributeSet::OnRep_MagicCritChance(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UMordecaiAttributeSet, MagicCritChance, OldValue);
}
