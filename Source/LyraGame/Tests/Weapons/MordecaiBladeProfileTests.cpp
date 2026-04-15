// Project Mordecai — Blade Family Weapon Profile Tests (US-025)
// NullRHI-compatible tests for Longsword, Greatsword, Shortsword, Dagger factory
// profiles. Verifies shapes, timings, damage values, airborne rules, relative
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

namespace MordecaiBladeTestHelpers
{
	static FMordecaiWeaponInstance MakeInstance(UMordecaiWeaponDataAsset* DA)
	{
		FMordecaiWeaponInstance Inst;
		Inst.InstanceId = FGuid::NewGuid();
		Inst.WeaponDataAsset = DA;
		Inst.IsEquipped = false;
		return Inst;
	}
}

using namespace MordecaiBladeTestHelpers;

// ===========================================================================
// 1. Mordecai.Weapon.Longsword.ProfilesMatchSpec (AC-025.2)
// ===========================================================================
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_Weapon_Longsword_ProfilesMatchSpec,
	"Mordecai.Weapon.Longsword.ProfilesMatchSpec",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_Weapon_Longsword_ProfilesMatchSpec::RunTest(const FString& Parameters)
{
	UMordecaiWeaponDataAsset* W = UMordecaiWeaponProfileFactory::CreateLongsword(GetTransientPackage());
	TestTrue("Longsword created", W != nullptr);

	// Weapon stats
	TestEqual("WeaponType", W->WeaponType, EMordecaiWeaponType::Longsword);
	TestEqual("EquipSlot", W->EquipSlot, EMordecaiEquipSlot::MainHand);
	TestTrue("BaseDamage=4", FMath::IsNearlyEqual(W->BaseDamage, 4.f));
	TestTrue("AttackSpeed=1.0", FMath::IsNearlyEqual(W->AttackSpeedMultiplier, 1.f));
	TestTrue("Range=200", FMath::IsNearlyEqual(W->Range, 200.f));
	TestTrue("PostureBonus=1", FMath::IsNearlyEqual(W->PostureDamageBonus, 1.f));
	TestTrue("Has Sword tag", W->GrantedTags.HasTag(MordecaiGameplayTags::Weapon_Type_Sword));

	// 3 light profiles
	TestEqual("3 light profiles", W->LightAttackProfiles.Num(), 3);

	// L1: MeleeSweep, ArcSector(200, 120), Windup=200, Active=150, Recovery=250, BP=12, Stam=8, PS=0.5
	const auto* L1 = W->LightAttackProfiles[0].Get();
	TestEqual("L1 type", L1->AttackType, EMordecaiAttackType::MeleeSweep);
	TestEqual("L1 shape", L1->HitShapeType, EMordecaiHitShapeType::ArcSector);
	TestTrue("L1 radius", FMath::IsNearlyEqual(L1->HitShapeParams.Radius, 200.f));
	TestTrue("L1 angle", FMath::IsNearlyEqual(L1->HitShapeParams.Angle, 120.f));
	TestTrue("L1 windup", FMath::IsNearlyEqual(L1->WindupTimeMs, 200.f));
	TestTrue("L1 active", FMath::IsNearlyEqual(L1->ActiveTimeMs, 150.f));
	TestTrue("L1 recovery", FMath::IsNearlyEqual(L1->RecoveryTimeMs, 250.f));
	TestTrue("L1 basePower", FMath::IsNearlyEqual(L1->DamageProfile.BasePower, 12.f));
	TestTrue("L1 stamina", FMath::IsNearlyEqual(L1->StaminaCost, 8.f));
	TestTrue("L1 postureScalar", FMath::IsNearlyEqual(L1->PostureDamageScalar, 0.5f));
	TestEqual("L1 damageType", L1->DamageProfile.DamageType, EMordecaiDamageType::Slash);

	// L3: bigger finisher
	const auto* L3 = W->LightAttackProfiles[2].Get();
	TestTrue("L3 basePower=18", FMath::IsNearlyEqual(L3->DamageProfile.BasePower, 18.f));
	TestTrue("L3 angle=180", FMath::IsNearlyEqual(L3->HitShapeParams.Angle, 180.f));

	// Heavy: MeleeSweep, ArcSector(220, 360), Windup=500, Active=250, Recovery=500, BP=28, Stam=25, PS=1.5, Rooted=Active
	TestTrue("Heavy exists", W->HeavyAttackProfile != nullptr);
	const auto* H = W->HeavyAttackProfile.Get();
	TestEqual("Heavy type", H->AttackType, EMordecaiAttackType::MeleeSweep);
	TestTrue("Heavy angle=360", FMath::IsNearlyEqual(H->HitShapeParams.Angle, 360.f));
	TestTrue("Heavy basePower=28", FMath::IsNearlyEqual(H->DamageProfile.BasePower, 28.f));
	TestTrue("Heavy staminaCost=25", FMath::IsNearlyEqual(H->StaminaCost, 25.f));
	TestTrue("Heavy postureScalar=1.5", FMath::IsNearlyEqual(H->PostureDamageScalar, 1.5f));
	TestEqual("Heavy rooted", H->RootedDuring, EMordecaiRootedMode::Active);

	return true;
}

// ===========================================================================
// 2. Mordecai.Weapon.Greatsword.ProfilesMatchSpec (AC-025.3)
// ===========================================================================
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_Weapon_Greatsword_ProfilesMatchSpec,
	"Mordecai.Weapon.Greatsword.ProfilesMatchSpec",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_Weapon_Greatsword_ProfilesMatchSpec::RunTest(const FString& Parameters)
{
	UMordecaiWeaponDataAsset* W = UMordecaiWeaponProfileFactory::CreateGreatsword(GetTransientPackage());
	TestTrue("Greatsword created", W != nullptr);

	TestEqual("WeaponType", W->WeaponType, EMordecaiWeaponType::Greatsword);
	TestEqual("EquipSlot TwoHand", W->EquipSlot, EMordecaiEquipSlot::TwoHand);
	TestTrue("BaseDamage=8", FMath::IsNearlyEqual(W->BaseDamage, 8.f));
	TestTrue("AttackSpeed=0.75", FMath::IsNearlyEqual(W->AttackSpeedMultiplier, 0.75f));
	TestTrue("Range=250", FMath::IsNearlyEqual(W->Range, 250.f));
	TestTrue("PostureBonus=4", FMath::IsNearlyEqual(W->PostureDamageBonus, 4.f));

	// 2 light sweeps
	TestEqual("2 light profiles", W->LightAttackProfiles.Num(), 2);
	const auto* L1 = W->LightAttackProfiles[0].Get();
	TestEqual("L1 type sweep", L1->AttackType, EMordecaiAttackType::MeleeSweep);
	TestTrue("L1 basePower=20", FMath::IsNearlyEqual(L1->DamageProfile.BasePower, 20.f));

	const auto* L2 = W->LightAttackProfiles[1].Get();
	TestTrue("L2 basePower=26", FMath::IsNearlyEqual(L2->DamageProfile.BasePower, 26.f));
	TestTrue("L2 angle=200", FMath::IsNearlyEqual(L2->HitShapeParams.Angle, 200.f));

	// Heavy: MeleeSlam, Circle(180)
	const auto* H = W->HeavyAttackProfile.Get();
	TestEqual("Heavy is Slam", H->AttackType, EMordecaiAttackType::MeleeSlam);
	TestEqual("Heavy shape Circle", H->HitShapeType, EMordecaiHitShapeType::Circle);
	TestTrue("Heavy radius=180", FMath::IsNearlyEqual(H->HitShapeParams.Radius, 180.f));
	TestTrue("Heavy basePower=35", FMath::IsNearlyEqual(H->DamageProfile.BasePower, 35.f));
	TestTrue("Heavy postureScalar=2.0", FMath::IsNearlyEqual(H->PostureDamageScalar, 2.f));
	TestEqual("Heavy rooted", H->RootedDuring, EMordecaiRootedMode::Active);
	TestEqual("Heavy damageType Slash", H->DamageProfile.DamageType, EMordecaiDamageType::Slash);

	return true;
}

// ===========================================================================
// 3. Mordecai.Weapon.Shortsword.ProfilesMatchSpec (AC-025.4)
// ===========================================================================
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_Weapon_Shortsword_ProfilesMatchSpec,
	"Mordecai.Weapon.Shortsword.ProfilesMatchSpec",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_Weapon_Shortsword_ProfilesMatchSpec::RunTest(const FString& Parameters)
{
	UMordecaiWeaponDataAsset* W = UMordecaiWeaponProfileFactory::CreateShortsword(GetTransientPackage());
	TestTrue("Shortsword created", W != nullptr);

	TestEqual("WeaponType", W->WeaponType, EMordecaiWeaponType::Shortsword);
	TestEqual("EquipSlot MainHand", W->EquipSlot, EMordecaiEquipSlot::MainHand);
	TestTrue("AttackSpeed=1.2", FMath::IsNearlyEqual(W->AttackSpeedMultiplier, 1.2f));

	// 4 light: 3 sweeps + 1 thrust
	TestEqual("4 light profiles", W->LightAttackProfiles.Num(), 4);

	for (int32 i = 0; i < 3; ++i)
	{
		TestEqual(FString::Printf(TEXT("L%d is sweep"), i + 1),
			W->LightAttackProfiles[i]->AttackType, EMordecaiAttackType::MeleeSweep);
		TestEqual(FString::Printf(TEXT("L%d shape ArcSector"), i + 1),
			W->LightAttackProfiles[i]->HitShapeType, EMordecaiHitShapeType::ArcSector);
	}

	// L4: thrust finisher
	const auto* L4 = W->LightAttackProfiles[3].Get();
	TestEqual("L4 is thrust", L4->AttackType, EMordecaiAttackType::MeleeThrust);
	TestEqual("L4 shape Capsule", L4->HitShapeType, EMordecaiHitShapeType::Capsule);
	TestTrue("L4 basePower=14", FMath::IsNearlyEqual(L4->DamageProfile.BasePower, 14.f));
	TestTrue("L4 length=170", FMath::IsNearlyEqual(L4->HitShapeParams.Length, 170.f));
	TestTrue("L4 width=60", FMath::IsNearlyEqual(L4->HitShapeParams.Width, 60.f));

	// Heavy: thrust
	const auto* H = W->HeavyAttackProfile.Get();
	TestEqual("Heavy is thrust", H->AttackType, EMordecaiAttackType::MeleeThrust);
	TestEqual("Heavy shape Capsule", H->HitShapeType, EMordecaiHitShapeType::Capsule);
	TestTrue("Heavy basePower=20", FMath::IsNearlyEqual(H->DamageProfile.BasePower, 20.f));

	return true;
}

// ===========================================================================
// 4. Mordecai.Weapon.Dagger.ProfilesMatchSpec (AC-025.5)
// ===========================================================================
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_Weapon_Dagger_ProfilesMatchSpec,
	"Mordecai.Weapon.Dagger.ProfilesMatchSpec",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_Weapon_Dagger_ProfilesMatchSpec::RunTest(const FString& Parameters)
{
	UMordecaiWeaponDataAsset* W = UMordecaiWeaponProfileFactory::CreateDagger(GetTransientPackage());
	TestTrue("Dagger created", W != nullptr);

	TestEqual("WeaponType Dagger", W->WeaponType, EMordecaiWeaponType::Dagger);
	TestEqual("EquipSlot MainHand", W->EquipSlot, EMordecaiEquipSlot::MainHand);
	TestTrue("BaseDamage=1", FMath::IsNearlyEqual(W->BaseDamage, 1.f));
	TestTrue("AttackSpeed=1.4", FMath::IsNearlyEqual(W->AttackSpeedMultiplier, 1.4f));
	TestTrue("Has Dagger tag", W->GrantedTags.HasTag(MordecaiGameplayTags::Weapon_Type_Dagger));

	// 5 light thrusts, all Pierce
	TestEqual("5 light profiles", W->LightAttackProfiles.Num(), 5);
	for (int32 i = 0; i < 5; ++i)
	{
		const auto* Li = W->LightAttackProfiles[i].Get();
		TestEqual(FString::Printf(TEXT("L%d is thrust"), i + 1), Li->AttackType, EMordecaiAttackType::MeleeThrust);
		TestEqual(FString::Printf(TEXT("L%d shape Capsule"), i + 1), Li->HitShapeType, EMordecaiHitShapeType::Capsule);
		TestEqual(FString::Printf(TEXT("L%d Pierce"), i + 1), Li->DamageProfile.DamageType, EMordecaiDamageType::Pierce);
	}

	// L5 finisher: bigger
	const auto* L5 = W->LightAttackProfiles[4].Get();
	TestTrue("L5 basePower=10", FMath::IsNearlyEqual(L5->DamageProfile.BasePower, 10.f));
	TestTrue("L5 length=150", FMath::IsNearlyEqual(L5->HitShapeParams.Length, 150.f));

	// Heavy: lunge thrust, Pierce
	const auto* H = W->HeavyAttackProfile.Get();
	TestEqual("Heavy is thrust", H->AttackType, EMordecaiAttackType::MeleeThrust);
	TestEqual("Heavy Pierce", H->DamageProfile.DamageType, EMordecaiDamageType::Pierce);
	TestTrue("Heavy basePower=22", FMath::IsNearlyEqual(H->DamageProfile.BasePower, 22.f));

	return true;
}

// ===========================================================================
// 5. Mordecai.Weapon.Blades.SweepAttacksJumpAvoidable (AC-025.6)
// ===========================================================================
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_Weapon_Blades_SweepAttacksJumpAvoidable,
	"Mordecai.Weapon.Blades.SweepAttacksJumpAvoidable",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_Weapon_Blades_SweepAttacksJumpAvoidable::RunTest(const FString& Parameters)
{
	UObject* Outer = GetTransientPackage();
	TArray<UMordecaiWeaponDataAsset*> Weapons = {
		UMordecaiWeaponProfileFactory::CreateLongsword(Outer),
		UMordecaiWeaponProfileFactory::CreateGreatsword(Outer),
		UMordecaiWeaponProfileFactory::CreateShortsword(Outer),
		UMordecaiWeaponProfileFactory::CreateDagger(Outer)
	};

	for (const UMordecaiWeaponDataAsset* W : Weapons)
	{
		for (const auto& Profile : W->LightAttackProfiles)
		{
			if (Profile->AttackType == EMordecaiAttackType::MeleeSweep)
			{
				TestTrue(FString::Printf(TEXT("%s sweep JumpAvoidable"), *W->WeaponId.ToString()),
					Profile->JumpAvoidable);
				TestFalse(FString::Printf(TEXT("%s sweep !HitsAirborne"), *W->WeaponId.ToString()),
					Profile->HitsAirborne);
			}
		}
		if (W->HeavyAttackProfile && W->HeavyAttackProfile->AttackType == EMordecaiAttackType::MeleeSweep)
		{
			TestTrue(FString::Printf(TEXT("%s heavy sweep JA"), *W->WeaponId.ToString()),
				W->HeavyAttackProfile->JumpAvoidable);
			TestFalse(FString::Printf(TEXT("%s heavy sweep !HA"), *W->WeaponId.ToString()),
				W->HeavyAttackProfile->HitsAirborne);
		}
	}

	return true;
}

// ===========================================================================
// 6. Mordecai.Weapon.Blades.ThrustSlamAttacksHitAirborne (AC-025.6)
// ===========================================================================
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_Weapon_Blades_ThrustSlamAttacksHitAirborne,
	"Mordecai.Weapon.Blades.ThrustSlamAttacksHitAirborne",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_Weapon_Blades_ThrustSlamAttacksHitAirborne::RunTest(const FString& Parameters)
{
	UObject* Outer = GetTransientPackage();
	TArray<UMordecaiWeaponDataAsset*> Weapons = {
		UMordecaiWeaponProfileFactory::CreateLongsword(Outer),
		UMordecaiWeaponProfileFactory::CreateGreatsword(Outer),
		UMordecaiWeaponProfileFactory::CreateShortsword(Outer),
		UMordecaiWeaponProfileFactory::CreateDagger(Outer)
	};

	for (const UMordecaiWeaponDataAsset* W : Weapons)
	{
		for (const auto& Profile : W->LightAttackProfiles)
		{
			if (Profile->AttackType == EMordecaiAttackType::MeleeThrust || Profile->AttackType == EMordecaiAttackType::MeleeSlam)
			{
				TestTrue(FString::Printf(TEXT("%s thrust/slam HitsAirborne"), *W->WeaponId.ToString()),
					Profile->HitsAirborne);
				TestFalse(FString::Printf(TEXT("%s thrust/slam !JumpAvoidable"), *W->WeaponId.ToString()),
					Profile->JumpAvoidable);
			}
		}
		if (W->HeavyAttackProfile &&
			(W->HeavyAttackProfile->AttackType == EMordecaiAttackType::MeleeThrust ||
			 W->HeavyAttackProfile->AttackType == EMordecaiAttackType::MeleeSlam))
		{
			TestTrue(FString::Printf(TEXT("%s heavy thrust/slam HA"), *W->WeaponId.ToString()),
				W->HeavyAttackProfile->HitsAirborne);
			TestFalse(FString::Printf(TEXT("%s heavy thrust/slam !JA"), *W->WeaponId.ToString()),
				W->HeavyAttackProfile->JumpAvoidable);
		}
	}

	return true;
}

// ===========================================================================
// 7. Mordecai.Weapon.Blades.RelativeSpeedOrdering (AC-025.7)
// ===========================================================================
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_Weapon_Blades_RelativeSpeedOrdering,
	"Mordecai.Weapon.Blades.RelativeSpeedOrdering",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_Weapon_Blades_RelativeSpeedOrdering::RunTest(const FString& Parameters)
{
	UObject* Outer = GetTransientPackage();
	float DaggerSpd   = UMordecaiWeaponProfileFactory::CreateDagger(Outer)->AttackSpeedMultiplier;
	float ShortSpd    = UMordecaiWeaponProfileFactory::CreateShortsword(Outer)->AttackSpeedMultiplier;
	float LongSpd     = UMordecaiWeaponProfileFactory::CreateLongsword(Outer)->AttackSpeedMultiplier;
	float GreatSpd    = UMordecaiWeaponProfileFactory::CreateGreatsword(Outer)->AttackSpeedMultiplier;

	TestTrue("Dagger > Shortsword speed", DaggerSpd > ShortSpd);
	TestTrue("Shortsword > Longsword speed", ShortSpd > LongSpd);
	TestTrue("Longsword > Greatsword speed", LongSpd > GreatSpd);
	return true;
}

// ===========================================================================
// 8. Mordecai.Weapon.Blades.RelativeDamageOrdering (AC-025.7)
// ===========================================================================
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_Weapon_Blades_RelativeDamageOrdering,
	"Mordecai.Weapon.Blades.RelativeDamageOrdering",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_Weapon_Blades_RelativeDamageOrdering::RunTest(const FString& Parameters)
{
	UObject* Outer = GetTransientPackage();
	float GreatDmg  = UMordecaiWeaponProfileFactory::CreateGreatsword(Outer)->BaseDamage;
	float LongDmg   = UMordecaiWeaponProfileFactory::CreateLongsword(Outer)->BaseDamage;
	float ShortDmg  = UMordecaiWeaponProfileFactory::CreateShortsword(Outer)->BaseDamage;
	float DaggerDmg = UMordecaiWeaponProfileFactory::CreateDagger(Outer)->BaseDamage;

	TestTrue("Greatsword > Longsword dmg", GreatDmg > LongDmg);
	TestTrue("Longsword > Shortsword dmg", LongDmg > ShortDmg);
	TestTrue("Shortsword > Dagger dmg", ShortDmg > DaggerDmg);
	return true;
}

// ===========================================================================
// 9. Mordecai.Weapon.Blades.RelativeRangeOrdering (AC-025.7)
// ===========================================================================
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_Weapon_Blades_RelativeRangeOrdering,
	"Mordecai.Weapon.Blades.RelativeRangeOrdering",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_Weapon_Blades_RelativeRangeOrdering::RunTest(const FString& Parameters)
{
	UObject* Outer = GetTransientPackage();
	float GreatRng  = UMordecaiWeaponProfileFactory::CreateGreatsword(Outer)->Range;
	float LongRng   = UMordecaiWeaponProfileFactory::CreateLongsword(Outer)->Range;
	float ShortRng  = UMordecaiWeaponProfileFactory::CreateShortsword(Outer)->Range;
	float DaggerRng = UMordecaiWeaponProfileFactory::CreateDagger(Outer)->Range;

	TestTrue("Greatsword > Longsword range", GreatRng > LongRng);
	TestTrue("Longsword > Shortsword range", LongRng > ShortRng);
	TestTrue("Shortsword > Dagger range", ShortRng > DaggerRng);
	return true;
}

// ===========================================================================
// 10. Mordecai.Weapon.Blades.EquipSetsActiveProfiles (AC-025.8)
// ===========================================================================
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_Weapon_Blades_EquipSetsActiveProfiles,
	"Mordecai.Weapon.Blades.EquipSetsActiveProfiles",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_Weapon_Blades_EquipSetsActiveProfiles::RunTest(const FString& Parameters)
{
	UObject* Outer = GetTransientPackage();
	UMordecaiEquipmentComponent* EquipComp = NewObject<UMordecaiEquipmentComponent>();

	// Equip each blade and check active profiles
	auto TestWeapon = [&](UMordecaiWeaponDataAsset* W, EMordecaiEquipSlot Slot, int32 ExpectedCount)
	{
		FMordecaiWeaponInstance Inst = MakeInstance(W);
		EquipComp->EquipWeapon(Inst, Slot);
		TArray<UMordecaiAttackProfileDataAsset*> Active = EquipComp->GetActiveLightAttackProfiles();
		TestEqual(FString::Printf(TEXT("%s active profiles count"), *W->WeaponId.ToString()),
			Active.Num(), ExpectedCount);
		EquipComp->UnequipWeapon(Slot);
	};

	TestWeapon(UMordecaiWeaponProfileFactory::CreateLongsword(Outer), EMordecaiEquipSlot::MainHand, 3);
	TestWeapon(UMordecaiWeaponProfileFactory::CreateGreatsword(Outer), EMordecaiEquipSlot::TwoHand, 2);
	TestWeapon(UMordecaiWeaponProfileFactory::CreateShortsword(Outer), EMordecaiEquipSlot::MainHand, 4);
	TestWeapon(UMordecaiWeaponProfileFactory::CreateDagger(Outer), EMordecaiEquipSlot::MainHand, 5);

	// Unequipped = unarmed fallback (empty by default on bare component)
	TArray<UMordecaiAttackProfileDataAsset*> Unarmed = EquipComp->GetActiveLightAttackProfiles();
	TestEqual("Unarmed fallback (empty default)", Unarmed.Num(), 0);

	return true;
}

// ===========================================================================
// 11. Mordecai.Weapon.Blades.HeavyAttacksNotCancelable (AC-025.9)
// ===========================================================================
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_Weapon_Blades_HeavyAttacksNotCancelable,
	"Mordecai.Weapon.Blades.HeavyAttacksNotCancelable",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_Weapon_Blades_HeavyAttacksNotCancelable::RunTest(const FString& Parameters)
{
	UObject* Outer = GetTransientPackage();
	TArray<UMordecaiWeaponDataAsset*> Weapons = {
		UMordecaiWeaponProfileFactory::CreateLongsword(Outer),
		UMordecaiWeaponProfileFactory::CreateGreatsword(Outer),
		UMordecaiWeaponProfileFactory::CreateShortsword(Outer),
		UMordecaiWeaponProfileFactory::CreateDagger(Outer)
	};

	for (const UMordecaiWeaponDataAsset* W : Weapons)
	{
		if (W->HeavyAttackProfile)
		{
			TestFalse(FString::Printf(TEXT("%s heavy !CancelIntoDodge"), *W->WeaponId.ToString()),
				W->HeavyAttackProfile->CancelableIntoDodge);
		}
	}
	return true;
}

// ===========================================================================
// 12. Mordecai.Weapon.Blades.LightAttacksCancelIntoDodge (AC-025.9)
// ===========================================================================
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_Weapon_Blades_LightAttacksCancelIntoDodge,
	"Mordecai.Weapon.Blades.LightAttacksCancelIntoDodge",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_Weapon_Blades_LightAttacksCancelIntoDodge::RunTest(const FString& Parameters)
{
	UObject* Outer = GetTransientPackage();
	TArray<UMordecaiWeaponDataAsset*> Weapons = {
		UMordecaiWeaponProfileFactory::CreateLongsword(Outer),
		UMordecaiWeaponProfileFactory::CreateGreatsword(Outer),
		UMordecaiWeaponProfileFactory::CreateShortsword(Outer),
		UMordecaiWeaponProfileFactory::CreateDagger(Outer)
	};

	for (const UMordecaiWeaponDataAsset* W : Weapons)
	{
		for (const auto& Profile : W->LightAttackProfiles)
		{
			TestTrue(FString::Printf(TEXT("%s light CancelIntoDodge"), *W->WeaponId.ToString()),
				Profile->CancelableIntoDodge);
		}
	}
	return true;
}
