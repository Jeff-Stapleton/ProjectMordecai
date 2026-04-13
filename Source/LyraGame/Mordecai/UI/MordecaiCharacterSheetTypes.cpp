// Project Mordecai — Character Sheet Types (US-066)

#include "Mordecai/UI/MordecaiCharacterSheetTypes.h"

#define LOCTEXT_NAMESPACE "MordecaiCharacterSheet"

// ---------------------------------------------------------------------------
// FMordecaiCharacterSheetData
// ---------------------------------------------------------------------------

void FMordecaiCharacterSheetData::InitDefaults()
{
	bIsBound = false;

	PhysicalGroupLabel   = LOCTEXT("GroupPhysical",   "Physical");
	ResilienceGroupLabel = LOCTEXT("GroupResilience", "Resilience");
	MagicalGroupLabel    = LOCTEXT("GroupMagical",    "Magical");

	// --- Primary attributes: STR, DEX, END, CON, RES, DIS, INT, WIS, CHA ---
	PrimaryAttributes.Reset();
	PrimaryAttributes.Add({ FName("STR"), LOCTEXT("AttrStrength",     "Strength"),     0.f, 0.f, false });
	PrimaryAttributes.Add({ FName("DEX"), LOCTEXT("AttrDexterity",    "Dexterity"),    0.f, 0.f, false });
	PrimaryAttributes.Add({ FName("END"), LOCTEXT("AttrEndurance",    "Endurance"),    0.f, 0.f, false });
	PrimaryAttributes.Add({ FName("CON"), LOCTEXT("AttrConstitution", "Constitution"), 0.f, 0.f, false });
	PrimaryAttributes.Add({ FName("RES"), LOCTEXT("AttrResistance",   "Resistance"),   0.f, 0.f, false });
	PrimaryAttributes.Add({ FName("DIS"), LOCTEXT("AttrDiscipline",   "Discipline"),   0.f, 0.f, false });
	PrimaryAttributes.Add({ FName("INT"), LOCTEXT("AttrIntelligence", "Intelligence"), 0.f, 0.f, false });
	PrimaryAttributes.Add({ FName("WIS"), LOCTEXT("AttrWisdom",       "Wisdom"),       0.f, 0.f, false });
	PrimaryAttributes.Add({ FName("CHA"), LOCTEXT("AttrCharisma",     "Charisma"),     0.f, 0.f, false });

	// --- Primary-derived multipliers (default 1.0) ---
	PrimaryDerivedStats.Reset();
	PrimaryDerivedStats.Add({ LOCTEXT("DerivPhysicalDmg",      "Physical Damage"),    1.f, false });
	PrimaryDerivedStats.Add({ LOCTEXT("DerivAttackSpeed",      "Attack Speed"),       1.f, false });
	PrimaryDerivedStats.Add({ LOCTEXT("DerivAfflictionResist", "Affliction Resist"),  1.f, false });
	PrimaryDerivedStats.Add({ LOCTEXT("DerivMagicDmg",         "Magic Damage"),       1.f, false });
	PrimaryDerivedStats.Add({ LOCTEXT("DerivCastSpeed",        "Cast Speed"),         1.f, false });

	// --- Secondary-derived multipliers (default 1.0) ---
	SecondaryDerivedStats.Reset();
	SecondaryDerivedStats.Add({ LOCTEXT("DerivArmorPen",             "Armor Penetration"),     1.f, false });
	SecondaryDerivedStats.Add({ LOCTEXT("DerivPhysicalCrit",         "Physical Crit Chance"),  1.f, false });
	SecondaryDerivedStats.Add({ LOCTEXT("DerivStaminaRegen",         "Stamina Regen"),         1.f, false });
	SecondaryDerivedStats.Add({ LOCTEXT("DerivHealthRegen",          "Health Regen"),          1.f, false });
	SecondaryDerivedStats.Add({ LOCTEXT("DerivAfflictionRecovery",   "Affliction Recovery"),   1.f, false });
	SecondaryDerivedStats.Add({ LOCTEXT("DerivPostureRecovery",      "Posture Recovery"),      1.f, false });
	SecondaryDerivedStats.Add({ LOCTEXT("DerivSPRegen",              "Spell Points Regen"),    1.f, false });
	SecondaryDerivedStats.Add({ LOCTEXT("DerivResistancePen",        "Resistance Penetration"),1.f, false });
	SecondaryDerivedStats.Add({ LOCTEXT("DerivMagicCrit",            "Magic Crit Chance"),     1.f, false });

	// --- Core resources ---
	CoreResources.Reset();
	CoreResources.Add({ FName("health"),      LOCTEXT("ResHealth",   "Health"),       0.f, 0.f, false });
	CoreResources.Add({ FName("stamina"),     LOCTEXT("ResStamina",  "Stamina"),      0.f, 0.f, false });
	CoreResources.Add({ FName("spellpoints"), LOCTEXT("ResSpellPts", "Spell Points"), 0.f, 0.f, false });
	CoreResources.Add({ FName("posture"),     LOCTEXT("ResPosture",  "Posture"),      0.f, 0.f, false });
}

EMordecaiAttributeGroup FMordecaiCharacterSheetData::GetGroupForAttribute(int32 PrimaryIndex)
{
	if (PrimaryIndex <= 2) { return EMordecaiAttributeGroup::Physical; }
	if (PrimaryIndex <= 5) { return EMordecaiAttributeGroup::Resilience; }
	return EMordecaiAttributeGroup::Magical;
}

FString FMordecaiCharacterSheetData::FormatPercentBonus(float MultiplierValue)
{
	const float Percent = (MultiplierValue - 1.0f) * 100.0f;
	const TCHAR* Sign = (Percent >= 0.f) ? TEXT("+") : TEXT("-");
	return FString::Printf(TEXT("%s%.1f%%"), Sign, FMath::Abs(Percent));
}

FString FMordecaiCharacterSheetData::FormatEffectiveModBonus(float EffectiveMod)
{
	const TCHAR* Sign = (EffectiveMod >= 0.f) ? TEXT("+") : TEXT("-");
	return FString::Printf(TEXT("%s%.1f%%"), Sign, FMath::Abs(EffectiveMod));
}

FString FMordecaiCharacterSheetData::FormatResource(float Current, float Max)
{
	return FString::Printf(TEXT("%d / %d"),
		FMath::FloorToInt(Current),
		FMath::FloorToInt(Max));
}

#undef LOCTEXT_NAMESPACE
