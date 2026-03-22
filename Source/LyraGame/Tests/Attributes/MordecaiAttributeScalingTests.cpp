// Project Mordecai — Attribute Scaling Tests (US-010)

#include "Misc/AutomationTest.h"
#include "Mordecai/AbilitySystem/MordecaiAttributeScaling.h"
#include "Mordecai/AbilitySystem/MordecaiAttributeSet.h"

// =============================================================================
// Effective Mod Formula Tests (AC-010.1 through AC-010.5)
// =============================================================================

// Mordecai.AttributeScaling.EffectiveModBand1 — AC-010.1
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecaiScaling_EffModBand1,
	"Mordecai.AttributeScaling.EffectiveModBand1",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecaiScaling_EffModBand1::RunTest(const FString& Parameters)
{
	// Band 1: Mods 1-5 at 100%. EffMod(5) == 5.0
	const float Result = FMordecaiAttributeScaling::CalculateEffectiveMod(5.0f);
	TestEqual("EffectiveMod(5) == 5.0", Result, 5.0f);
	return true;
}

// Mordecai.AttributeScaling.EffectiveModBand2 — AC-010.2
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecaiScaling_EffModBand2,
	"Mordecai.AttributeScaling.EffectiveModBand2",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecaiScaling_EffModBand2::RunTest(const FString& Parameters)
{
	// Band 2: Mods 6-10 at 70%. EffMod(10) == 5 + 5*0.7 == 8.5
	const float Result = FMordecaiAttributeScaling::CalculateEffectiveMod(10.0f);
	TestEqual("EffectiveMod(10) == 8.5", Result, 8.5f);
	return true;
}

// Mordecai.AttributeScaling.EffectiveModBand3 — AC-010.3
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecaiScaling_EffModBand3,
	"Mordecai.AttributeScaling.EffectiveModBand3",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecaiScaling_EffModBand3::RunTest(const FString& Parameters)
{
	// Band 3: Mods 11-15 at 45%. EffMod(12) == 5 + 5*0.7 + 2*0.45 == 9.4
	const float Result = FMordecaiAttributeScaling::CalculateEffectiveMod(12.0f);
	TestEqual("EffectiveMod(12) == 9.4", Result, 9.4f);
	return true;
}

// Mordecai.AttributeScaling.EffectiveModAllBands — AC-010.4
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecaiScaling_EffModAllBands,
	"Mordecai.AttributeScaling.EffectiveModAllBands",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecaiScaling_EffModAllBands::RunTest(const FString& Parameters)
{
	// All 4 bands: EffMod(20) == 5 + 5*0.7 + 5*0.45 + 5*0.25 == 12.0
	const float Result = FMordecaiAttributeScaling::CalculateEffectiveMod(20.0f);
	TestEqual("EffectiveMod(20) == 12.0", Result, 12.0f);
	return true;
}

// Mordecai.AttributeScaling.EffectiveModZero — AC-010.5
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecaiScaling_EffModZero,
	"Mordecai.AttributeScaling.EffectiveModZero",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecaiScaling_EffModZero::RunTest(const FString& Parameters)
{
	// Zero input: EffMod(0) == 0.0
	const float Result = FMordecaiAttributeScaling::CalculateEffectiveMod(0.0f);
	TestEqual("EffectiveMod(0) == 0.0", Result, 0.0f);
	return true;
}

// =============================================================================
// Helper: Create an attribute set, set a primary stat, recalculate derived stats
// =============================================================================

namespace MordecaiScalingTestHelpers
{
	static constexpr float TestStatValue = 12.0f;
	// EffMod(12) = 5 + 5*0.7 + 2*0.45 = 9.4
	static constexpr float ExpectedEffMod12 = 9.4f;
	static constexpr float Tolerance = 0.001f;

	// Base resource values (must match UMordecaiAttributeSet constants)
	static constexpr float BaseMaxHealth = 100.0f;
	static constexpr float BaseMaxStamina = 100.0f;
	static constexpr float BaseMaxPosture = 100.0f;
	static constexpr float BaseMaxSpellPoints = 10.0f;
}

// =============================================================================
// Primary Stat Effect Tests (AC-010.6 through AC-010.14)
// =============================================================================

// Mordecai.AttributeScaling.STRScalesPhysicalDamage — AC-010.6
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecaiScaling_STRPhysDmg,
	"Mordecai.AttributeScaling.STRScalesPhysicalDamage",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecaiScaling_STRPhysDmg::RunTest(const FString& Parameters)
{
	using namespace MordecaiScalingTestHelpers;
	UMordecaiAttributeSet* Set = NewObject<UMordecaiAttributeSet>(GetTransientPackage());
	Set->InitStrength(TestStatValue);
	Set->RecalculateDerivedStats();

	// PhysicalDamageMultiplier = 1.0 + 0.03 * 9.4 = 1.282
	const float Expected = 1.0f + 0.03f * ExpectedEffMod12;
	TestNearlyEqual("STR 12 -> PhysicalDamageMultiplier", Set->GetPhysicalDamageMultiplier(), Expected, Tolerance);
	return true;
}

// Mordecai.AttributeScaling.DEXScalesAttackSpeed — AC-010.7
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecaiScaling_DEXAtkSpd,
	"Mordecai.AttributeScaling.DEXScalesAttackSpeed",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecaiScaling_DEXAtkSpd::RunTest(const FString& Parameters)
{
	using namespace MordecaiScalingTestHelpers;
	UMordecaiAttributeSet* Set = NewObject<UMordecaiAttributeSet>(GetTransientPackage());
	Set->InitDexterity(TestStatValue);
	Set->RecalculateDerivedStats();

	// AttackSpeedMultiplier = 1.0 + 0.015 * 9.4 = 1.141
	const float Expected = 1.0f + 0.015f * ExpectedEffMod12;
	TestNearlyEqual("DEX 12 -> AttackSpeedMultiplier", Set->GetAttackSpeedMultiplier(), Expected, Tolerance);
	return true;
}

// Mordecai.AttributeScaling.ENDScalesMaxStamina — AC-010.8
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecaiScaling_ENDMaxStam,
	"Mordecai.AttributeScaling.ENDScalesMaxStamina",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecaiScaling_ENDMaxStam::RunTest(const FString& Parameters)
{
	using namespace MordecaiScalingTestHelpers;
	UMordecaiAttributeSet* Set = NewObject<UMordecaiAttributeSet>(GetTransientPackage());
	Set->InitEndurance(TestStatValue);
	Set->RecalculateDerivedStats();

	// MaxStamina = BaseMaxStamina * (1 + 0.04 * 9.4) = 100 * 1.376 = 137.6
	const float Expected = BaseMaxStamina * (1.0f + 0.04f * ExpectedEffMod12);
	TestNearlyEqual("END 12 -> MaxStamina", Set->GetMaxStamina(), Expected, Tolerance);
	return true;
}

// Mordecai.AttributeScaling.CONScalesMaxHealth — AC-010.9
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecaiScaling_CONMaxHP,
	"Mordecai.AttributeScaling.CONScalesMaxHealth",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecaiScaling_CONMaxHP::RunTest(const FString& Parameters)
{
	using namespace MordecaiScalingTestHelpers;
	UMordecaiAttributeSet* Set = NewObject<UMordecaiAttributeSet>(GetTransientPackage());
	Set->InitConstitution(TestStatValue);
	Set->RecalculateDerivedStats();

	// MaxHealth = BaseMaxHealth * (1 + 0.06 * 9.4) = 100 * 1.564 = 156.4
	const float Expected = BaseMaxHealth * (1.0f + 0.06f * ExpectedEffMod12);
	TestNearlyEqual("CON 12 -> MaxHealth", Set->GetMaxHealth(), Expected, Tolerance);
	return true;
}

// Mordecai.AttributeScaling.RESScalesAfflictionResist — AC-010.10
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecaiScaling_RESAfflRes,
	"Mordecai.AttributeScaling.RESScalesAfflictionResist",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecaiScaling_RESAfflRes::RunTest(const FString& Parameters)
{
	using namespace MordecaiScalingTestHelpers;
	UMordecaiAttributeSet* Set = NewObject<UMordecaiAttributeSet>(GetTransientPackage());
	Set->InitResistance(TestStatValue);
	Set->RecalculateDerivedStats();

	// AfflictionResistMultiplier = 1.0 + 0.03 * 9.4 = 1.282
	const float Expected = 1.0f + 0.03f * ExpectedEffMod12;
	TestNearlyEqual("RES 12 -> AfflictionResistMultiplier", Set->GetAfflictionResistMultiplier(), Expected, Tolerance);
	return true;
}

// Mordecai.AttributeScaling.DISScalesMaxPosture — AC-010.11
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecaiScaling_DISMaxPost,
	"Mordecai.AttributeScaling.DISScalesMaxPosture",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecaiScaling_DISMaxPost::RunTest(const FString& Parameters)
{
	using namespace MordecaiScalingTestHelpers;
	UMordecaiAttributeSet* Set = NewObject<UMordecaiAttributeSet>(GetTransientPackage());
	Set->InitDiscipline(TestStatValue);
	Set->RecalculateDerivedStats();

	// MaxPosture = BaseMaxPosture * (1 + 0.04 * 9.4) = 100 * 1.376 = 137.6
	const float Expected = BaseMaxPosture * (1.0f + 0.04f * ExpectedEffMod12);
	TestNearlyEqual("DIS 12 -> MaxPosture", Set->GetMaxPosture(), Expected, Tolerance);
	return true;
}

// Mordecai.AttributeScaling.INTScalesMaxSpellPoints — AC-010.12
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecaiScaling_INTMaxSP,
	"Mordecai.AttributeScaling.INTScalesMaxSpellPoints",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecaiScaling_INTMaxSP::RunTest(const FString& Parameters)
{
	using namespace MordecaiScalingTestHelpers;
	UMordecaiAttributeSet* Set = NewObject<UMordecaiAttributeSet>(GetTransientPackage());
	Set->InitIntelligence(TestStatValue);
	Set->RecalculateDerivedStats();

	// MaxSpellPoints = BaseMaxSpellPoints * (1 + 0.04 * 9.4) = 10 * 1.376 = 13.76
	const float Expected = BaseMaxSpellPoints * (1.0f + 0.04f * ExpectedEffMod12);
	TestNearlyEqual("INT 12 -> MaxSpellPoints", Set->GetMaxSpellPoints(), Expected, Tolerance);
	return true;
}

// Mordecai.AttributeScaling.WISScalesMagicDamage — AC-010.13
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecaiScaling_WISMagDmg,
	"Mordecai.AttributeScaling.WISScalesMagicDamage",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecaiScaling_WISMagDmg::RunTest(const FString& Parameters)
{
	using namespace MordecaiScalingTestHelpers;
	UMordecaiAttributeSet* Set = NewObject<UMordecaiAttributeSet>(GetTransientPackage());
	Set->InitWisdom(TestStatValue);
	Set->RecalculateDerivedStats();

	// MagicDamageMultiplier = 1.0 + 0.03 * 9.4 = 1.282
	const float Expected = 1.0f + 0.03f * ExpectedEffMod12;
	TestNearlyEqual("WIS 12 -> MagicDamageMultiplier", Set->GetMagicDamageMultiplier(), Expected, Tolerance);
	return true;
}

// Mordecai.AttributeScaling.CHAScalesCastSpeed — AC-010.14
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecaiScaling_CHACastSpd,
	"Mordecai.AttributeScaling.CHAScalesCastSpeed",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecaiScaling_CHACastSpd::RunTest(const FString& Parameters)
{
	using namespace MordecaiScalingTestHelpers;
	UMordecaiAttributeSet* Set = NewObject<UMordecaiAttributeSet>(GetTransientPackage());
	Set->InitCharisma(TestStatValue);
	Set->RecalculateDerivedStats();

	// CastSpeedMultiplier = 1.0 + 0.015 * 9.4 = 1.141
	const float Expected = 1.0f + 0.015f * ExpectedEffMod12;
	TestNearlyEqual("CHA 12 -> CastSpeedMultiplier", Set->GetCastSpeedMultiplier(), Expected, Tolerance);
	return true;
}

// =============================================================================
// Secondary Stat Effect Tests (AC-010.15 through AC-010.23)
// =============================================================================

// Mordecai.AttributeScaling.STRScalesArmorPenetration — AC-010.15
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecaiScaling_STRArmorPen,
	"Mordecai.AttributeScaling.STRScalesArmorPenetration",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecaiScaling_STRArmorPen::RunTest(const FString& Parameters)
{
	using namespace MordecaiScalingTestHelpers;
	UMordecaiAttributeSet* Set = NewObject<UMordecaiAttributeSet>(GetTransientPackage());
	Set->InitStrength(TestStatValue);
	Set->RecalculateDerivedStats();

	// ArmorPenetrationMultiplier = 1.0 + 0.015 * 9.4 = 1.141
	const float Expected = 1.0f + 0.015f * ExpectedEffMod12;
	TestNearlyEqual("STR 12 -> ArmorPenetrationMultiplier", Set->GetArmorPenetrationMultiplier(), Expected, Tolerance);
	return true;
}

// Mordecai.AttributeScaling.DEXScalesPhysicalCritChance — AC-010.16
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecaiScaling_DEXPhysCrit,
	"Mordecai.AttributeScaling.DEXScalesPhysicalCritChance",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecaiScaling_DEXPhysCrit::RunTest(const FString& Parameters)
{
	using namespace MordecaiScalingTestHelpers;
	UMordecaiAttributeSet* Set = NewObject<UMordecaiAttributeSet>(GetTransientPackage());
	Set->InitDexterity(TestStatValue);
	Set->RecalculateDerivedStats();

	// PhysicalCritChance = 0.0075 * 9.4 = 0.0705
	const float Expected = 0.0075f * ExpectedEffMod12;
	TestNearlyEqual("DEX 12 -> PhysicalCritChance", Set->GetPhysicalCritChance(), Expected, Tolerance);
	return true;
}

// Mordecai.AttributeScaling.ENDScalesStaminaRegen — AC-010.17
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecaiScaling_ENDStamRegen,
	"Mordecai.AttributeScaling.ENDScalesStaminaRegen",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecaiScaling_ENDStamRegen::RunTest(const FString& Parameters)
{
	using namespace MordecaiScalingTestHelpers;
	UMordecaiAttributeSet* Set = NewObject<UMordecaiAttributeSet>(GetTransientPackage());
	Set->InitEndurance(TestStatValue);
	Set->RecalculateDerivedStats();

	// StaminaRegenMultiplier = 1.0 + 0.02 * 9.4 = 1.188
	const float Expected = 1.0f + 0.02f * ExpectedEffMod12;
	TestNearlyEqual("END 12 -> StaminaRegenMultiplier", Set->GetStaminaRegenMultiplier(), Expected, Tolerance);
	return true;
}

// Mordecai.AttributeScaling.CONScalesHealthRegen — AC-010.18
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecaiScaling_CONHPRegen,
	"Mordecai.AttributeScaling.CONScalesHealthRegen",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecaiScaling_CONHPRegen::RunTest(const FString& Parameters)
{
	using namespace MordecaiScalingTestHelpers;
	UMordecaiAttributeSet* Set = NewObject<UMordecaiAttributeSet>(GetTransientPackage());
	Set->InitConstitution(TestStatValue);
	Set->RecalculateDerivedStats();

	// HealthRegenMultiplier = 1.0 + 0.015 * 9.4 = 1.141
	const float Expected = 1.0f + 0.015f * ExpectedEffMod12;
	TestNearlyEqual("CON 12 -> HealthRegenMultiplier", Set->GetHealthRegenMultiplier(), Expected, Tolerance);
	return true;
}

// Mordecai.AttributeScaling.RESScalesAfflictionRecovery — AC-010.19
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecaiScaling_RESAfflRecov,
	"Mordecai.AttributeScaling.RESScalesAfflictionRecovery",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecaiScaling_RESAfflRecov::RunTest(const FString& Parameters)
{
	using namespace MordecaiScalingTestHelpers;
	UMordecaiAttributeSet* Set = NewObject<UMordecaiAttributeSet>(GetTransientPackage());
	Set->InitResistance(TestStatValue);
	Set->RecalculateDerivedStats();

	// AfflictionRecoveryMultiplier = 1.0 + 0.015 * 9.4 = 1.141
	const float Expected = 1.0f + 0.015f * ExpectedEffMod12;
	TestNearlyEqual("RES 12 -> AfflictionRecoveryMultiplier", Set->GetAfflictionRecoveryMultiplier(), Expected, Tolerance);
	return true;
}

// Mordecai.AttributeScaling.DISScalesPostureRecovery — AC-010.20
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecaiScaling_DISPostRecov,
	"Mordecai.AttributeScaling.DISScalesPostureRecovery",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecaiScaling_DISPostRecov::RunTest(const FString& Parameters)
{
	using namespace MordecaiScalingTestHelpers;
	UMordecaiAttributeSet* Set = NewObject<UMordecaiAttributeSet>(GetTransientPackage());
	Set->InitDiscipline(TestStatValue);
	Set->RecalculateDerivedStats();

	// PostureRecoveryMultiplier = 1.0 + 0.02 * 9.4 = 1.188
	const float Expected = 1.0f + 0.02f * ExpectedEffMod12;
	TestNearlyEqual("DIS 12 -> PostureRecoveryMultiplier", Set->GetPostureRecoveryMultiplier(), Expected, Tolerance);
	return true;
}

// Mordecai.AttributeScaling.INTScalesSpellPointsRegen — AC-010.21
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecaiScaling_INTSPRegen,
	"Mordecai.AttributeScaling.INTScalesSpellPointsRegen",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecaiScaling_INTSPRegen::RunTest(const FString& Parameters)
{
	using namespace MordecaiScalingTestHelpers;
	UMordecaiAttributeSet* Set = NewObject<UMordecaiAttributeSet>(GetTransientPackage());
	Set->InitIntelligence(TestStatValue);
	Set->RecalculateDerivedStats();

	// SpellPointsRegenMultiplier = 1.0 + 0.02 * 9.4 = 1.188
	const float Expected = 1.0f + 0.02f * ExpectedEffMod12;
	TestNearlyEqual("INT 12 -> SpellPointsRegenMultiplier", Set->GetSpellPointsRegenMultiplier(), Expected, Tolerance);
	return true;
}

// Mordecai.AttributeScaling.WISScalesResistancePenetration — AC-010.22
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecaiScaling_WISResPen,
	"Mordecai.AttributeScaling.WISScalesResistancePenetration",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecaiScaling_WISResPen::RunTest(const FString& Parameters)
{
	using namespace MordecaiScalingTestHelpers;
	UMordecaiAttributeSet* Set = NewObject<UMordecaiAttributeSet>(GetTransientPackage());
	Set->InitWisdom(TestStatValue);
	Set->RecalculateDerivedStats();

	// ResistancePenetrationMultiplier = 1.0 + 0.0125 * 9.4 = 1.1175
	const float Expected = 1.0f + 0.0125f * ExpectedEffMod12;
	TestNearlyEqual("WIS 12 -> ResistancePenetrationMultiplier", Set->GetResistancePenetrationMultiplier(), Expected, Tolerance);
	return true;
}

// Mordecai.AttributeScaling.CHAScalesMagicCritChance — AC-010.23
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecaiScaling_CHAMagCrit,
	"Mordecai.AttributeScaling.CHAScalesMagicCritChance",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecaiScaling_CHAMagCrit::RunTest(const FString& Parameters)
{
	using namespace MordecaiScalingTestHelpers;
	UMordecaiAttributeSet* Set = NewObject<UMordecaiAttributeSet>(GetTransientPackage());
	Set->InitCharisma(TestStatValue);
	Set->RecalculateDerivedStats();

	// MagicCritChance = 0.0075 * 9.4 = 0.0705
	const float Expected = 0.0075f * ExpectedEffMod12;
	TestNearlyEqual("CHA 12 -> MagicCritChance", Set->GetMagicCritChance(), Expected, Tolerance);
	return true;
}

// =============================================================================
// GAS Integration Tests (AC-010.24 through AC-010.26)
// =============================================================================

// Mordecai.AttributeScaling.DerivedStatsRecalcOnPrimaryChange — AC-010.24
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecaiScaling_DerivedRecalc,
	"Mordecai.AttributeScaling.DerivedStatsRecalcOnPrimaryChange",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecaiScaling_DerivedRecalc::RunTest(const FString& Parameters)
{
	using namespace MordecaiScalingTestHelpers;
	UMordecaiAttributeSet* Set = NewObject<UMordecaiAttributeSet>(GetTransientPackage());

	// At default STR=10, before recalculation, PhysicalDamageMultiplier should be 1.0 (constructor default)
	TestEqual("PhysDmgMult starts at 1.0", Set->GetPhysicalDamageMultiplier(), 1.0f);

	// Recalculate with STR=10: EffMod(10)=8.5, PhysDmgMult = 1.0 + 0.03*8.5 = 1.255
	Set->RecalculateDerivedStats();
	const float AfterFirst = 1.0f + 0.03f * 8.5f;
	TestNearlyEqual("After recalc with STR 10", Set->GetPhysicalDamageMultiplier(), AfterFirst, Tolerance);

	// Change STR to 15 and recalculate: EffMod(15)=10.75, PhysDmgMult = 1.0 + 0.03*10.75 = 1.3225
	Set->InitStrength(15.0f);
	Set->RecalculateDerivedStats();
	const float EffMod15 = FMordecaiAttributeScaling::CalculateEffectiveMod(15.0f);
	const float AfterSecond = 1.0f + 0.03f * EffMod15;
	TestNearlyEqual("After recalc with STR 15", Set->GetPhysicalDamageMultiplier(), AfterSecond, Tolerance);

	// Verify derived stat actually changed
	TestTrue("Derived stat changed after primary stat changed", !FMath::IsNearlyEqual(AfterFirst, AfterSecond));

	return true;
}

// Mordecai.AttributeScaling.DefaultMultiplierValues — AC-010.25
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecaiScaling_Defaults,
	"Mordecai.AttributeScaling.DefaultMultiplierValues",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecaiScaling_Defaults::RunTest(const FString& Parameters)
{
	const UMordecaiAttributeSet* CDO = GetDefault<UMordecaiAttributeSet>();
	TestNotNull("AttributeSet CDO exists", CDO);

	// Multiplier attributes default to 1.0 (neutral)
	TestEqual("PhysicalDamageMultiplier default 1.0", CDO->GetPhysicalDamageMultiplier(), 1.0f);
	TestEqual("AttackSpeedMultiplier default 1.0", CDO->GetAttackSpeedMultiplier(), 1.0f);
	TestEqual("AfflictionResistMultiplier default 1.0", CDO->GetAfflictionResistMultiplier(), 1.0f);
	TestEqual("MagicDamageMultiplier default 1.0", CDO->GetMagicDamageMultiplier(), 1.0f);
	TestEqual("CastSpeedMultiplier default 1.0", CDO->GetCastSpeedMultiplier(), 1.0f);
	TestEqual("ArmorPenetrationMultiplier default 1.0", CDO->GetArmorPenetrationMultiplier(), 1.0f);
	TestEqual("StaminaRegenMultiplier default 1.0", CDO->GetStaminaRegenMultiplier(), 1.0f);
	TestEqual("HealthRegenMultiplier default 1.0", CDO->GetHealthRegenMultiplier(), 1.0f);
	TestEqual("AfflictionRecoveryMultiplier default 1.0", CDO->GetAfflictionRecoveryMultiplier(), 1.0f);
	TestEqual("PostureRecoveryMultiplier default 1.0", CDO->GetPostureRecoveryMultiplier(), 1.0f);
	TestEqual("SpellPointsRegenMultiplier default 1.0", CDO->GetSpellPointsRegenMultiplier(), 1.0f);
	TestEqual("ResistancePenetrationMultiplier default 1.0", CDO->GetResistancePenetrationMultiplier(), 1.0f);

	// Additive chance attributes default to 0.0
	TestEqual("PhysicalCritChance default 0.0", CDO->GetPhysicalCritChance(), 0.0f);
	TestEqual("MagicCritChance default 0.0", CDO->GetMagicCritChance(), 0.0f);

	return true;
}

// Mordecai.AttributeScaling.EndToEndSTR12Example — AC-010.26
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecaiScaling_EndToEndSTR12,
	"Mordecai.AttributeScaling.EndToEndSTR12Example",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecaiScaling_EndToEndSTR12::RunTest(const FString& Parameters)
{
	using namespace MordecaiScalingTestHelpers;
	UMordecaiAttributeSet* Set = NewObject<UMordecaiAttributeSet>(GetTransientPackage());
	Set->InitStrength(12.0f);
	Set->RecalculateDerivedStats();

	// EffMod(12) = 9.4
	// PhysicalDamageMultiplier = 1.0 + (0.03 * 9.4) = 1.282
	TestNearlyEqual("STR 12 -> PhysDmgMult == 1.282",
		Set->GetPhysicalDamageMultiplier(), 1.282f, Tolerance);

	// ArmorPenetrationMultiplier = 1.0 + (0.015 * 9.4) = 1.141
	TestNearlyEqual("STR 12 -> ArmorPenMult == 1.141",
		Set->GetArmorPenetrationMultiplier(), 1.141f, Tolerance);

	return true;
}
