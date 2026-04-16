// Project Mordecai — Blunt & Polearm Weapon Profile Tests (US-026)
// NullRHI-compatible tests for Axe, Mace, Spear, Quarterstaff, Unarmed factory
// profiles. Verifies shapes, timings, damage values, airborne rules, full 9-weapon
// ordering, equip integration, and cancel rules.

#include "Misc/AutomationTest.h"

#include "Mordecai/Weapons/MordecaiWeaponProfileFactory.h"
#include "Mordecai/Weapons/MordecaiWeaponDataAsset.h"
#include "Mordecai/Weapons/MordecaiWeaponTypes.h"
#include "Mordecai/Weapons/MordecaiEquipmentComponent.h"
#include "Mordecai/Combat/MordecaiAttackProfileDataAsset.h"
#include "Mordecai/Combat/MordecaiCombatTypes.h"
#include "Mordecai/MordecaiGameplayTags.h"

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

namespace MordecaiBluntPolearmTestHelpers
{
	static FMordecaiWeaponInstance MakeBluntInstance(UMordecaiWeaponDataAsset* DA)
	{
		FMordecaiWeaponInstance Inst;
		Inst.InstanceId = FGuid::NewGuid();
		Inst.WeaponDataAsset = DA;
		Inst.IsEquipped = false;
		return Inst;
	}

	/** Collect all attack profiles (light + heavy) from a weapon. */
	static TArray<const UMordecaiAttackProfileDataAsset*> AllProfiles(const UMordecaiWeaponDataAsset* W)
	{
		TArray<const UMordecaiAttackProfileDataAsset*> Result;
		for (const auto& P : W->LightAttackProfiles) { Result.Add(P); }
		if (W->HeavyAttackProfile) { Result.Add(W->HeavyAttackProfile); }
		return Result;
	}
}

using namespace MordecaiBluntPolearmTestHelpers;

// ===========================================================================
// 1. Mordecai.Weapon.Axe.ProfilesMatchSpec (AC-026.2)
// ===========================================================================
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_Weapon_Axe_ProfilesMatchSpec,
	"Mordecai.Weapon.Axe.ProfilesMatchSpec",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_Weapon_Axe_ProfilesMatchSpec::RunTest(const FString& Parameters)
{
	UMordecaiWeaponDataAsset* W = UMordecaiWeaponProfileFactory::CreateAxe(GetTransientPackage());
	TestTrue("Axe created", W != nullptr);

	TestEqual("WeaponType Axe", W->WeaponType, EMordecaiWeaponType::Axe);
	TestEqual("EquipSlot MainHand", W->EquipSlot, EMordecaiEquipSlot::MainHand);
	TestTrue("BaseDamage=6", FMath::IsNearlyEqual(W->BaseDamage, 6.f));
	TestTrue("AttackSpeed=0.85", FMath::IsNearlyEqual(W->AttackSpeedMultiplier, 0.85f));
	TestTrue("Range=180", FMath::IsNearlyEqual(W->Range, 180.f));
	TestTrue("PostureBonus=3", FMath::IsNearlyEqual(W->PostureDamageBonus, 3.f));
	TestTrue("Has Axe tag", W->GrantedTags.HasTag(MordecaiGameplayTags::Weapon_Type_Axe));

	// 2 light sweeps
	TestEqual("2 light profiles", W->LightAttackProfiles.Num(), 2);
	const auto* L1 = W->LightAttackProfiles[0].Get();
	TestEqual("L1 type sweep", L1->AttackType, EMordecaiAttackType::MeleeSweep);
	TestTrue("L1 radius=180", FMath::IsNearlyEqual(L1->HitShapeParams.Radius, 180.f));
	TestTrue("L1 angle=140", FMath::IsNearlyEqual(L1->HitShapeParams.Angle, 140.f));
	TestTrue("L1 basePower=16", FMath::IsNearlyEqual(L1->DamageProfile.BasePower, 16.f));
	TestEqual("L1 Slash", L1->DamageProfile.DamageType, EMordecaiDamageType::Slash);

	const auto* L2 = W->LightAttackProfiles[1].Get();
	TestTrue("L2 basePower=22", FMath::IsNearlyEqual(L2->DamageProfile.BasePower, 22.f));

	// Heavy: MeleeSlam, Circle(160)
	const auto* H = W->HeavyAttackProfile.Get();
	TestEqual("Heavy Slam", H->AttackType, EMordecaiAttackType::MeleeSlam);
	TestEqual("Heavy Circle", H->HitShapeType, EMordecaiHitShapeType::Circle);
	TestTrue("Heavy radius=160", FMath::IsNearlyEqual(H->HitShapeParams.Radius, 160.f));
	TestTrue("Heavy basePower=30", FMath::IsNearlyEqual(H->DamageProfile.BasePower, 30.f));
	TestTrue("Heavy postureScalar=1.8", FMath::IsNearlyEqual(H->PostureDamageScalar, 1.8f));
	TestEqual("Heavy rooted", H->RootedDuring, EMordecaiRootedMode::Active);
	TestEqual("Heavy Slash", H->DamageProfile.DamageType, EMordecaiDamageType::Slash);

	return true;
}

// ===========================================================================
// 2. Mordecai.Weapon.Mace.ProfilesMatchSpec (AC-026.3)
// ===========================================================================
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_Weapon_Mace_ProfilesMatchSpec,
	"Mordecai.Weapon.Mace.ProfilesMatchSpec",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_Weapon_Mace_ProfilesMatchSpec::RunTest(const FString& Parameters)
{
	UMordecaiWeaponDataAsset* W = UMordecaiWeaponProfileFactory::CreateMace(GetTransientPackage());
	TestTrue("Mace created", W != nullptr);

	TestEqual("WeaponType Mace", W->WeaponType, EMordecaiWeaponType::Mace);
	TestEqual("EquipSlot MainHand", W->EquipSlot, EMordecaiEquipSlot::MainHand);
	TestTrue("BaseDamage=5", FMath::IsNearlyEqual(W->BaseDamage, 5.f));
	TestTrue("AttackSpeed=0.8", FMath::IsNearlyEqual(W->AttackSpeedMultiplier, 0.8f));
	TestTrue("PostureBonus=5", FMath::IsNearlyEqual(W->PostureDamageBonus, 5.f));
	TestTrue("Has Mace tag", W->GrantedTags.HasTag(MordecaiGameplayTags::Weapon_Type_Mace));

	// L1: sweep, L2: slam
	TestEqual("2 light profiles", W->LightAttackProfiles.Num(), 2);
	TestEqual("L1 sweep", W->LightAttackProfiles[0]->AttackType, EMordecaiAttackType::MeleeSweep);
	TestEqual("L1 ArcSector", W->LightAttackProfiles[0]->HitShapeType, EMordecaiHitShapeType::ArcSector);
	TestEqual("L1 Blunt", W->LightAttackProfiles[0]->DamageProfile.DamageType, EMordecaiDamageType::Blunt);
	TestTrue("L1 basePower=14", FMath::IsNearlyEqual(W->LightAttackProfiles[0]->DamageProfile.BasePower, 14.f));

	TestEqual("L2 slam", W->LightAttackProfiles[1]->AttackType, EMordecaiAttackType::MeleeSlam);
	TestEqual("L2 Circle", W->LightAttackProfiles[1]->HitShapeType, EMordecaiHitShapeType::Circle);
	TestTrue("L2 basePower=18", FMath::IsNearlyEqual(W->LightAttackProfiles[1]->DamageProfile.BasePower, 18.f));
	TestTrue("L2 postureScalar=1.2", FMath::IsNearlyEqual(W->LightAttackProfiles[1]->PostureDamageScalar, 1.2f));

	// Heavy: ground pound
	const auto* H = W->HeavyAttackProfile.Get();
	TestEqual("Heavy Slam", H->AttackType, EMordecaiAttackType::MeleeSlam);
	TestTrue("Heavy radius=180", FMath::IsNearlyEqual(H->HitShapeParams.Radius, 180.f));
	TestTrue("Heavy basePower=28", FMath::IsNearlyEqual(H->DamageProfile.BasePower, 28.f));
	TestTrue("Heavy postureScalar=2.5", FMath::IsNearlyEqual(H->PostureDamageScalar, 2.5f));
	TestEqual("Heavy Blunt", H->DamageProfile.DamageType, EMordecaiDamageType::Blunt);

	return true;
}

// ===========================================================================
// 3. Mordecai.Weapon.Spear.ProfilesMatchSpec (AC-026.4)
// ===========================================================================
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_Weapon_Spear_ProfilesMatchSpec,
	"Mordecai.Weapon.Spear.ProfilesMatchSpec",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_Weapon_Spear_ProfilesMatchSpec::RunTest(const FString& Parameters)
{
	UMordecaiWeaponDataAsset* W = UMordecaiWeaponProfileFactory::CreateSpear(GetTransientPackage());
	TestTrue("Spear created", W != nullptr);

	TestEqual("WeaponType Spear", W->WeaponType, EMordecaiWeaponType::Spear);
	TestEqual("EquipSlot TwoHand", W->EquipSlot, EMordecaiEquipSlot::TwoHand);
	TestTrue("BaseDamage=5", FMath::IsNearlyEqual(W->BaseDamage, 5.f));
	TestTrue("AttackSpeed=0.9", FMath::IsNearlyEqual(W->AttackSpeedMultiplier, 0.9f));
	TestTrue("Range=280", FMath::IsNearlyEqual(W->Range, 280.f));
	TestTrue("Has Spear tag", W->GrantedTags.HasTag(MordecaiGameplayTags::Weapon_Type_Spear));

	// 3 lights: thrust, thrust, sweep finisher
	TestEqual("3 light profiles", W->LightAttackProfiles.Num(), 3);
	TestEqual("L1 thrust", W->LightAttackProfiles[0]->AttackType, EMordecaiAttackType::MeleeThrust);
	TestEqual("L2 thrust", W->LightAttackProfiles[1]->AttackType, EMordecaiAttackType::MeleeThrust);
	TestEqual("L3 sweep", W->LightAttackProfiles[2]->AttackType, EMordecaiAttackType::MeleeSweep);

	TestTrue("L1 length=280", FMath::IsNearlyEqual(W->LightAttackProfiles[0]->HitShapeParams.Length, 280.f));
	TestTrue("L3 radius=250", FMath::IsNearlyEqual(W->LightAttackProfiles[2]->HitShapeParams.Radius, 250.f));

	// All Pierce
	for (int32 i = 0; i < 3; ++i)
	{
		TestEqual(FString::Printf(TEXT("L%d Pierce"), i + 1),
			W->LightAttackProfiles[i]->DamageProfile.DamageType, EMordecaiDamageType::Pierce);
	}

	// Heavy: charging lunge thrust
	const auto* H = W->HeavyAttackProfile.Get();
	TestEqual("Heavy thrust", H->AttackType, EMordecaiAttackType::MeleeThrust);
	TestTrue("Heavy length=320", FMath::IsNearlyEqual(H->HitShapeParams.Length, 320.f));
	TestTrue("Heavy basePower=26", FMath::IsNearlyEqual(H->DamageProfile.BasePower, 26.f));
	TestEqual("Heavy Pierce", H->DamageProfile.DamageType, EMordecaiDamageType::Pierce);

	return true;
}

// ===========================================================================
// 4. Mordecai.Weapon.Quarterstaff.ProfilesMatchSpec (AC-026.5)
// ===========================================================================
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_Weapon_Quarterstaff_ProfilesMatchSpec,
	"Mordecai.Weapon.Quarterstaff.ProfilesMatchSpec",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_Weapon_Quarterstaff_ProfilesMatchSpec::RunTest(const FString& Parameters)
{
	UMordecaiWeaponDataAsset* W = UMordecaiWeaponProfileFactory::CreateQuarterstaff(GetTransientPackage());
	TestTrue("Quarterstaff created", W != nullptr);

	TestEqual("WeaponType Quarterstaff", W->WeaponType, EMordecaiWeaponType::Quarterstaff);
	TestEqual("EquipSlot TwoHand", W->EquipSlot, EMordecaiEquipSlot::TwoHand);
	TestTrue("BaseDamage=3", FMath::IsNearlyEqual(W->BaseDamage, 3.f));
	TestTrue("AttackSpeed=1.0", FMath::IsNearlyEqual(W->AttackSpeedMultiplier, 1.f));
	TestTrue("Range=240", FMath::IsNearlyEqual(W->Range, 240.f));
	TestTrue("Has Staff tag", W->GrantedTags.HasTag(MordecaiGameplayTags::Weapon_Type_Staff));

	// 3 lights: sweep, thrust, sweep
	TestEqual("3 light profiles", W->LightAttackProfiles.Num(), 3);
	TestEqual("L1 sweep", W->LightAttackProfiles[0]->AttackType, EMordecaiAttackType::MeleeSweep);
	TestEqual("L2 thrust", W->LightAttackProfiles[1]->AttackType, EMordecaiAttackType::MeleeThrust);
	TestEqual("L3 sweep", W->LightAttackProfiles[2]->AttackType, EMordecaiAttackType::MeleeSweep);

	// All Blunt
	for (int32 i = 0; i < 3; ++i)
	{
		TestEqual(FString::Printf(TEXT("L%d Blunt"), i + 1),
			W->LightAttackProfiles[i]->DamageProfile.DamageType, EMordecaiDamageType::Blunt);
	}

	// Heavy: 360° spinning sweep
	const auto* H = W->HeavyAttackProfile.Get();
	TestEqual("Heavy sweep", H->AttackType, EMordecaiAttackType::MeleeSweep);
	TestTrue("Heavy angle=360", FMath::IsNearlyEqual(H->HitShapeParams.Angle, 360.f));
	TestTrue("Heavy basePower=22", FMath::IsNearlyEqual(H->DamageProfile.BasePower, 22.f));
	TestEqual("Heavy Blunt", H->DamageProfile.DamageType, EMordecaiDamageType::Blunt);

	return true;
}

// ===========================================================================
// 5. Mordecai.Weapon.Unarmed.ProfilesMatchSpec (AC-026.6)
// ===========================================================================
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_Weapon_Unarmed_ProfilesMatchSpec,
	"Mordecai.Weapon.Unarmed.ProfilesMatchSpec",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_Weapon_Unarmed_ProfilesMatchSpec::RunTest(const FString& Parameters)
{
	UMordecaiWeaponDataAsset* W = UMordecaiWeaponProfileFactory::CreateUnarmed(GetTransientPackage());
	TestTrue("Unarmed created", W != nullptr);

	TestEqual("WeaponType Unarmed", W->WeaponType, EMordecaiWeaponType::Unarmed);
	TestEqual("EquipSlot MainHand", W->EquipSlot, EMordecaiEquipSlot::MainHand);
	TestTrue("BaseDamage=0", FMath::IsNearlyEqual(W->BaseDamage, 0.f));
	TestTrue("AttackSpeed=1.3", FMath::IsNearlyEqual(W->AttackSpeedMultiplier, 1.3f));
	TestTrue("Range=100", FMath::IsNearlyEqual(W->Range, 100.f));
	TestTrue("PostureBonus=0", FMath::IsNearlyEqual(W->PostureDamageBonus, 0.f));
	TestTrue("Has Unarmed tag", W->GrantedTags.HasTag(MordecaiGameplayTags::Weapon_Type_Unarmed));

	// 3 light thrusts, all Blunt
	TestEqual("3 light profiles", W->LightAttackProfiles.Num(), 3);
	for (int32 i = 0; i < 3; ++i)
	{
		TestEqual(FString::Printf(TEXT("L%d thrust"), i + 1), W->LightAttackProfiles[i]->AttackType, EMordecaiAttackType::MeleeThrust);
		TestEqual(FString::Printf(TEXT("L%d Blunt"), i + 1), W->LightAttackProfiles[i]->DamageProfile.DamageType, EMordecaiDamageType::Blunt);
	}
	TestTrue("L1 basePower=3", FMath::IsNearlyEqual(W->LightAttackProfiles[0]->DamageProfile.BasePower, 3.f));

	// Heavy: slam
	const auto* H = W->HeavyAttackProfile.Get();
	TestEqual("Heavy slam", H->AttackType, EMordecaiAttackType::MeleeSlam);
	TestTrue("Heavy basePower=8", FMath::IsNearlyEqual(H->DamageProfile.BasePower, 8.f));
	TestTrue("Heavy radius=80", FMath::IsNearlyEqual(H->HitShapeParams.Radius, 80.f));
	TestEqual("Heavy Blunt", H->DamageProfile.DamageType, EMordecaiDamageType::Blunt);

	return true;
}

// ===========================================================================
// 6. Mordecai.Weapon.BluntPolearm.SweepAttacksJumpAvoidable (AC-026.7)
// ===========================================================================
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_Weapon_BluntPolearm_SweepAttacksJumpAvoidable,
	"Mordecai.Weapon.BluntPolearm.SweepAttacksJumpAvoidable",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_Weapon_BluntPolearm_SweepAttacksJumpAvoidable::RunTest(const FString& Parameters)
{
	UObject* Outer = GetTransientPackage();
	TArray<UMordecaiWeaponDataAsset*> Weapons = {
		UMordecaiWeaponProfileFactory::CreateAxe(Outer),
		UMordecaiWeaponProfileFactory::CreateMace(Outer),
		UMordecaiWeaponProfileFactory::CreateSpear(Outer),
		UMordecaiWeaponProfileFactory::CreateQuarterstaff(Outer),
		UMordecaiWeaponProfileFactory::CreateUnarmed(Outer)
	};

	for (const UMordecaiWeaponDataAsset* W : Weapons)
	{
		for (const auto& P : AllProfiles(W))
		{
			if (P->AttackType == EMordecaiAttackType::MeleeSweep)
			{
				TestTrue(FString::Printf(TEXT("%s sweep JA"), *W->WeaponId.ToString()), P->JumpAvoidable);
				TestFalse(FString::Printf(TEXT("%s sweep !HA"), *W->WeaponId.ToString()), P->HitsAirborne);
			}
		}
	}
	return true;
}

// ===========================================================================
// 7. Mordecai.Weapon.BluntPolearm.ThrustSlamAttacksHitAirborne (AC-026.7)
// ===========================================================================
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_Weapon_BluntPolearm_ThrustSlamAttacksHitAirborne,
	"Mordecai.Weapon.BluntPolearm.ThrustSlamAttacksHitAirborne",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_Weapon_BluntPolearm_ThrustSlamAttacksHitAirborne::RunTest(const FString& Parameters)
{
	UObject* Outer = GetTransientPackage();
	TArray<UMordecaiWeaponDataAsset*> Weapons = {
		UMordecaiWeaponProfileFactory::CreateAxe(Outer),
		UMordecaiWeaponProfileFactory::CreateMace(Outer),
		UMordecaiWeaponProfileFactory::CreateSpear(Outer),
		UMordecaiWeaponProfileFactory::CreateQuarterstaff(Outer),
		UMordecaiWeaponProfileFactory::CreateUnarmed(Outer)
	};

	for (const UMordecaiWeaponDataAsset* W : Weapons)
	{
		for (const auto& P : AllProfiles(W))
		{
			if (P->AttackType == EMordecaiAttackType::MeleeThrust || P->AttackType == EMordecaiAttackType::MeleeSlam)
			{
				TestTrue(FString::Printf(TEXT("%s thrust/slam HA"), *W->WeaponId.ToString()), P->HitsAirborne);
				TestFalse(FString::Printf(TEXT("%s thrust/slam !JA"), *W->WeaponId.ToString()), P->JumpAvoidable);
			}
		}
	}
	return true;
}

// ===========================================================================
// 8–11. Full 9-weapon ordering tests (AC-026.8)
// ===========================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_Weapon_AllMelee_RelativeSpeedOrdering,
	"Mordecai.Weapon.AllMelee.RelativeSpeedOrdering",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_Weapon_AllMelee_RelativeSpeedOrdering::RunTest(const FString& Parameters)
{
	UObject* O = GetTransientPackage();
	float Dagger = UMordecaiWeaponProfileFactory::CreateDagger(O)->AttackSpeedMultiplier;
	float Unarmed = UMordecaiWeaponProfileFactory::CreateUnarmed(O)->AttackSpeedMultiplier;
	float Short = UMordecaiWeaponProfileFactory::CreateShortsword(O)->AttackSpeedMultiplier;
	float Long = UMordecaiWeaponProfileFactory::CreateLongsword(O)->AttackSpeedMultiplier;
	float Staff = UMordecaiWeaponProfileFactory::CreateQuarterstaff(O)->AttackSpeedMultiplier;
	float Spear = UMordecaiWeaponProfileFactory::CreateSpear(O)->AttackSpeedMultiplier;
	float Axe = UMordecaiWeaponProfileFactory::CreateAxe(O)->AttackSpeedMultiplier;
	float Mace = UMordecaiWeaponProfileFactory::CreateMace(O)->AttackSpeedMultiplier;
	float Great = UMordecaiWeaponProfileFactory::CreateGreatsword(O)->AttackSpeedMultiplier;

	TestTrue("Dagger > Unarmed", Dagger > Unarmed);
	TestTrue("Unarmed > Shortsword", Unarmed > Short);
	TestTrue("Shortsword > Longsword", Short > Long);
	TestTrue("Longsword = Quarterstaff", FMath::IsNearlyEqual(Long, Staff));
	TestTrue("Quarterstaff > Spear", Staff > Spear);
	TestTrue("Spear > Axe", Spear > Axe);
	TestTrue("Axe > Mace", Axe > Mace);
	TestTrue("Mace > Greatsword", Mace > Great);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_Weapon_AllMelee_RelativeDamageOrdering,
	"Mordecai.Weapon.AllMelee.RelativeDamageOrdering",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_Weapon_AllMelee_RelativeDamageOrdering::RunTest(const FString& Parameters)
{
	UObject* O = GetTransientPackage();
	float Great = UMordecaiWeaponProfileFactory::CreateGreatsword(O)->BaseDamage;
	float Axe = UMordecaiWeaponProfileFactory::CreateAxe(O)->BaseDamage;
	float Mace = UMordecaiWeaponProfileFactory::CreateMace(O)->BaseDamage;
	float Spear = UMordecaiWeaponProfileFactory::CreateSpear(O)->BaseDamage;
	float Long = UMordecaiWeaponProfileFactory::CreateLongsword(O)->BaseDamage;
	float Staff = UMordecaiWeaponProfileFactory::CreateQuarterstaff(O)->BaseDamage;
	float Short = UMordecaiWeaponProfileFactory::CreateShortsword(O)->BaseDamage;
	float Dagger = UMordecaiWeaponProfileFactory::CreateDagger(O)->BaseDamage;
	float Unarmed = UMordecaiWeaponProfileFactory::CreateUnarmed(O)->BaseDamage;

	TestTrue("Great > Axe", Great > Axe);
	TestTrue("Axe > Mace", Axe > Mace);
	TestTrue("Mace = Spear", FMath::IsNearlyEqual(Mace, Spear));
	TestTrue("Spear > Longsword", Spear > Long);
	TestTrue("Long > Staff", Long > Staff);
	TestTrue("Staff > Short", Staff > Short);
	TestTrue("Short > Dagger", Short > Dagger);
	TestTrue("Dagger > Unarmed", Dagger > Unarmed);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_Weapon_AllMelee_RelativeRangeOrdering,
	"Mordecai.Weapon.AllMelee.RelativeRangeOrdering",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_Weapon_AllMelee_RelativeRangeOrdering::RunTest(const FString& Parameters)
{
	UObject* O = GetTransientPackage();
	float Spear = UMordecaiWeaponProfileFactory::CreateSpear(O)->Range;
	float Great = UMordecaiWeaponProfileFactory::CreateGreatsword(O)->Range;
	float Staff = UMordecaiWeaponProfileFactory::CreateQuarterstaff(O)->Range;
	float Long = UMordecaiWeaponProfileFactory::CreateLongsword(O)->Range;
	float Axe = UMordecaiWeaponProfileFactory::CreateAxe(O)->Range;
	float Mace = UMordecaiWeaponProfileFactory::CreateMace(O)->Range;
	float Short = UMordecaiWeaponProfileFactory::CreateShortsword(O)->Range;
	float Dagger = UMordecaiWeaponProfileFactory::CreateDagger(O)->Range;
	float Unarmed = UMordecaiWeaponProfileFactory::CreateUnarmed(O)->Range;

	TestTrue("Spear > Great", Spear > Great);
	TestTrue("Great > Staff", Great > Staff);
	TestTrue("Staff > Long", Staff > Long);
	TestTrue("Long > Axe", Long > Axe);
	TestTrue("Axe > Mace", Axe > Mace);
	TestTrue("Mace > Short", Mace > Short);
	TestTrue("Short > Dagger", Short > Dagger);
	TestTrue("Dagger > Unarmed", Dagger > Unarmed);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_Weapon_AllMelee_RelativePostureOrdering,
	"Mordecai.Weapon.AllMelee.RelativePostureOrdering",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_Weapon_AllMelee_RelativePostureOrdering::RunTest(const FString& Parameters)
{
	UObject* O = GetTransientPackage();
	float Mace = UMordecaiWeaponProfileFactory::CreateMace(O)->PostureDamageBonus;
	float Great = UMordecaiWeaponProfileFactory::CreateGreatsword(O)->PostureDamageBonus;
	float Axe = UMordecaiWeaponProfileFactory::CreateAxe(O)->PostureDamageBonus;
	float Spear = UMordecaiWeaponProfileFactory::CreateSpear(O)->PostureDamageBonus;
	float Staff = UMordecaiWeaponProfileFactory::CreateQuarterstaff(O)->PostureDamageBonus;
	float Long = UMordecaiWeaponProfileFactory::CreateLongsword(O)->PostureDamageBonus;
	float Short = UMordecaiWeaponProfileFactory::CreateShortsword(O)->PostureDamageBonus;
	float Dagger = UMordecaiWeaponProfileFactory::CreateDagger(O)->PostureDamageBonus;
	float Unarmed = UMordecaiWeaponProfileFactory::CreateUnarmed(O)->PostureDamageBonus;

	TestTrue("Mace > Great", Mace > Great);
	TestTrue("Great > Axe", Great > Axe);
	TestTrue("Axe > Spear", Axe > Spear);
	TestTrue("Spear = Staff", FMath::IsNearlyEqual(Spear, Staff));
	TestTrue("Staff > Long", Staff > Long);
	TestTrue("Long > Short", Long > Short);
	TestTrue("Short = Dagger", FMath::IsNearlyEqual(Short, Dagger));
	TestTrue("Dagger = Unarmed", FMath::IsNearlyEqual(Dagger, Unarmed));
	return true;
}

// ===========================================================================
// 12. Mordecai.Weapon.BluntPolearm.EquipSetsActiveProfiles (AC-026.9)
// ===========================================================================
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_Weapon_BluntPolearm_EquipSetsActiveProfiles,
	"Mordecai.Weapon.BluntPolearm.EquipSetsActiveProfiles",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_Weapon_BluntPolearm_EquipSetsActiveProfiles::RunTest(const FString& Parameters)
{
	UObject* Outer = GetTransientPackage();
	UMordecaiEquipmentComponent* EquipComp = NewObject<UMordecaiEquipmentComponent>();

	auto TestWeapon = [&](UMordecaiWeaponDataAsset* W, EMordecaiEquipSlot Slot, int32 ExpectedCount)
	{
		FMordecaiWeaponInstance Inst = MakeBluntInstance(W);
		EquipComp->EquipWeapon(Inst, Slot);
		TArray<UMordecaiAttackProfileDataAsset*> Active = EquipComp->GetActiveLightAttackProfiles();
		TestEqual(FString::Printf(TEXT("%s active count"), *W->WeaponId.ToString()), Active.Num(), ExpectedCount);
		EquipComp->UnequipWeapon(Slot);
	};

	TestWeapon(UMordecaiWeaponProfileFactory::CreateAxe(Outer), EMordecaiEquipSlot::MainHand, 2);
	TestWeapon(UMordecaiWeaponProfileFactory::CreateMace(Outer), EMordecaiEquipSlot::MainHand, 2);
	TestWeapon(UMordecaiWeaponProfileFactory::CreateSpear(Outer), EMordecaiEquipSlot::TwoHand, 3);
	TestWeapon(UMordecaiWeaponProfileFactory::CreateQuarterstaff(Outer), EMordecaiEquipSlot::TwoHand, 3);
	TestWeapon(UMordecaiWeaponProfileFactory::CreateUnarmed(Outer), EMordecaiEquipSlot::MainHand, 3);

	return true;
}

// ===========================================================================
// 13. Mordecai.Weapon.BluntPolearm.HeavyAttacksNotCancelable (AC-026.10)
// ===========================================================================
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_Weapon_BluntPolearm_HeavyAttacksNotCancelable,
	"Mordecai.Weapon.BluntPolearm.HeavyAttacksNotCancelable",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_Weapon_BluntPolearm_HeavyAttacksNotCancelable::RunTest(const FString& Parameters)
{
	UObject* O = GetTransientPackage();
	TArray<UMordecaiWeaponDataAsset*> Weapons = {
		UMordecaiWeaponProfileFactory::CreateAxe(O),
		UMordecaiWeaponProfileFactory::CreateMace(O),
		UMordecaiWeaponProfileFactory::CreateSpear(O),
		UMordecaiWeaponProfileFactory::CreateQuarterstaff(O),
		UMordecaiWeaponProfileFactory::CreateUnarmed(O)
	};
	for (const UMordecaiWeaponDataAsset* W : Weapons)
	{
		if (W->HeavyAttackProfile)
		{
			TestFalse(FString::Printf(TEXT("%s heavy !CancelDodge"), *W->WeaponId.ToString()),
				W->HeavyAttackProfile->CancelableIntoDodge);
		}
	}
	return true;
}

// ===========================================================================
// 14. Mordecai.Weapon.BluntPolearm.LightAttacksCancelIntoDodge (AC-026.10)
// ===========================================================================
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_Weapon_BluntPolearm_LightAttacksCancelIntoDodge,
	"Mordecai.Weapon.BluntPolearm.LightAttacksCancelIntoDodge",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_Weapon_BluntPolearm_LightAttacksCancelIntoDodge::RunTest(const FString& Parameters)
{
	UObject* O = GetTransientPackage();
	TArray<UMordecaiWeaponDataAsset*> Weapons = {
		UMordecaiWeaponProfileFactory::CreateAxe(O),
		UMordecaiWeaponProfileFactory::CreateMace(O),
		UMordecaiWeaponProfileFactory::CreateSpear(O),
		UMordecaiWeaponProfileFactory::CreateQuarterstaff(O),
		UMordecaiWeaponProfileFactory::CreateUnarmed(O)
	};
	for (const UMordecaiWeaponDataAsset* W : Weapons)
	{
		for (const auto& P : W->LightAttackProfiles)
		{
			TestTrue(FString::Printf(TEXT("%s light CancelDodge"), *W->WeaponId.ToString()),
				P->CancelableIntoDodge);
		}
	}
	return true;
}
