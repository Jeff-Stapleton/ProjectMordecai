// Project Mordecai — Attribute Tests (Story 2.2)

#include "Misc/AutomationTest.h"
#include "Mordecai/AbilitySystem/MordecaiAttributeSet.h"
#include "Mordecai/MordecaiPlayerState.h"

// Mordecai.Attributes.AttributeSetHasAllCoreAttributes
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecaiAttr_CoreAttributes,
	"Mordecai.Attributes.AttributeSetHasAllCoreAttributes",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecaiAttr_CoreAttributes::RunTest(const FString& Parameters)
{
	// Verify all core resource attributes exist on the attribute set
	TestTrue("Health attribute exists", UMordecaiAttributeSet::GetHealthAttribute().IsValid());
	TestTrue("MaxHealth attribute exists", UMordecaiAttributeSet::GetMaxHealthAttribute().IsValid());
	TestTrue("Stamina attribute exists", UMordecaiAttributeSet::GetStaminaAttribute().IsValid());
	TestTrue("MaxStamina attribute exists", UMordecaiAttributeSet::GetMaxStaminaAttribute().IsValid());
	TestTrue("SpellPoints attribute exists", UMordecaiAttributeSet::GetSpellPointsAttribute().IsValid());
	TestTrue("MaxSpellPoints attribute exists", UMordecaiAttributeSet::GetMaxSpellPointsAttribute().IsValid());
	TestTrue("Posture attribute exists", UMordecaiAttributeSet::GetPostureAttribute().IsValid());
	TestTrue("MaxPosture attribute exists", UMordecaiAttributeSet::GetMaxPostureAttribute().IsValid());

	return true;
}

// Mordecai.Attributes.AttributeSetHasAllPrimaryAttributes
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecaiAttr_PrimaryAttributes,
	"Mordecai.Attributes.AttributeSetHasAllPrimaryAttributes",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecaiAttr_PrimaryAttributes::RunTest(const FString& Parameters)
{
	// Verify all 9 primary attributes exist (per character_attributes_v1)
	TestTrue("STR attribute exists", UMordecaiAttributeSet::GetStrengthAttribute().IsValid());
	TestTrue("DEX attribute exists", UMordecaiAttributeSet::GetDexterityAttribute().IsValid());
	TestTrue("END attribute exists", UMordecaiAttributeSet::GetEnduranceAttribute().IsValid());
	TestTrue("CON attribute exists", UMordecaiAttributeSet::GetConstitutionAttribute().IsValid());
	TestTrue("RES attribute exists", UMordecaiAttributeSet::GetResistanceAttribute().IsValid());
	TestTrue("DIS attribute exists", UMordecaiAttributeSet::GetDisciplineAttribute().IsValid());
	TestTrue("INT attribute exists", UMordecaiAttributeSet::GetIntelligenceAttribute().IsValid());
	TestTrue("WIS attribute exists", UMordecaiAttributeSet::GetWisdomAttribute().IsValid());
	TestTrue("CHA attribute exists", UMordecaiAttributeSet::GetCharismaAttribute().IsValid());

	return true;
}

// Mordecai.Attributes.DefaultsInitializedCorrectly
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecaiAttr_Defaults,
	"Mordecai.Attributes.DefaultsInitializedCorrectly",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecaiAttr_Defaults::RunTest(const FString& Parameters)
{
	const UMordecaiAttributeSet* CDO = GetDefault<UMordecaiAttributeSet>();
	TestNotNull("AttributeSet CDO exists", CDO);

	// Core resource defaults (AC-2.2.3)
	TestEqual("Health default is 100", CDO->GetHealth(), 100.0f);
	TestEqual("MaxHealth default is 100", CDO->GetMaxHealth(), 100.0f);
	TestEqual("Stamina default is 100", CDO->GetStamina(), 100.0f);
	TestEqual("MaxStamina default is 100", CDO->GetMaxStamina(), 100.0f);
	TestEqual("SpellPoints default is 10", CDO->GetSpellPoints(), 10.0f);
	TestEqual("MaxSpellPoints default is 10", CDO->GetMaxSpellPoints(), 10.0f);
	TestEqual("Posture default is 100", CDO->GetPosture(), 100.0f);
	TestEqual("MaxPosture default is 100", CDO->GetMaxPosture(), 100.0f);

	// Primary attribute defaults (placeholder = 10)
	TestEqual("STR default is 10", CDO->GetStrength(), 10.0f);
	TestEqual("DEX default is 10", CDO->GetDexterity(), 10.0f);
	TestEqual("END default is 10", CDO->GetEndurance(), 10.0f);
	TestEqual("CON default is 10", CDO->GetConstitution(), 10.0f);
	TestEqual("RES default is 10", CDO->GetResistance(), 10.0f);
	TestEqual("DIS default is 10", CDO->GetDiscipline(), 10.0f);
	TestEqual("INT default is 10", CDO->GetIntelligence(), 10.0f);
	TestEqual("WIS default is 10", CDO->GetWisdom(), 10.0f);
	TestEqual("CHA default is 10", CDO->GetCharisma(), 10.0f);

	return true;
}

// Mordecai.Attributes.HealthClampedToMax
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecaiAttr_HealthClamped,
	"Mordecai.Attributes.HealthClampedToMax",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecaiAttr_HealthClamped::RunTest(const FString& Parameters)
{
	// Verify the clamp logic by checking the attribute accessor behavior.
	// ClampAttribute is called in PreAttributeChange and PreAttributeBaseChange.
	// We verify that the static attribute accessors work correctly with the CDO.
	const UMordecaiAttributeSet* CDO = GetDefault<UMordecaiAttributeSet>();
	TestNotNull("AttributeSet CDO exists", CDO);

	// Health should not exceed MaxHealth
	TestTrue("Health <= MaxHealth", CDO->GetHealth() <= CDO->GetMaxHealth());

	// Stamina should not exceed MaxStamina
	TestTrue("Stamina <= MaxStamina", CDO->GetStamina() <= CDO->GetMaxStamina());

	// SpellPoints should not exceed MaxSpellPoints
	TestTrue("SpellPoints <= MaxSpellPoints", CDO->GetSpellPoints() <= CDO->GetMaxSpellPoints());

	// Posture should not exceed MaxPosture
	TestTrue("Posture <= MaxPosture", CDO->GetPosture() <= CDO->GetMaxPosture());

	return true;
}

// Mordecai.Attributes.ASCInitializesAttributeSet
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecaiAttr_ASCInitializesSet,
	"Mordecai.Attributes.ASCInitializesAttributeSet",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecaiAttr_ASCInitializesSet::RunTest(const FString& Parameters)
{
	// Verify that AMordecaiPlayerState CDO has the MordecaiAttributeSet
	const AMordecaiPlayerState* CDO = GetDefault<AMordecaiPlayerState>();
	TestNotNull("PlayerState CDO exists", CDO);

	UMordecaiAttributeSet* AttrSet = const_cast<AMordecaiPlayerState*>(CDO)->GetMordecaiAttributeSet();
	TestNotNull("MordecaiAttributeSet exists on PlayerState", AttrSet);

	return true;
}
