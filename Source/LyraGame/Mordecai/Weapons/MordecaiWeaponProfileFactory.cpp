// Project Mordecai — Weapon Profile Factory (US-025)

#include "Mordecai/Weapons/MordecaiWeaponProfileFactory.h"
#include "Mordecai/Weapons/MordecaiWeaponDataAsset.h"
#include "Mordecai/Weapons/MordecaiWeaponTypes.h"
#include "Mordecai/Combat/MordecaiAttackProfileDataAsset.h"
#include "Mordecai/Combat/MordecaiCombatTypes.h"
#include "Mordecai/MordecaiGameplayTags.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(MordecaiWeaponProfileFactory)

// ---------------------------------------------------------------------------
// Internal helpers
// ---------------------------------------------------------------------------

namespace
{
	/** Create a sweep (ArcSector) attack profile. */
	UMordecaiAttackProfileDataAsset* MakeSweep(
		UObject* Outer,
		float Radius, float Angle,
		float WindupMs, float ActiveMs, float RecoveryMs,
		float BasePower, float StaminaCost, float PostureScalar,
		EMordecaiDamageType DmgType,
		EMordecaiRootedMode Rooted = EMordecaiRootedMode::None,
		bool bCancelIntoDodge = true)
	{
		UMordecaiAttackProfileDataAsset* P = NewObject<UMordecaiAttackProfileDataAsset>(Outer);
		P->AttackType = EMordecaiAttackType::MeleeSweep;
		P->InputSlot = EMordecaiInputSlot::Light;
		P->HitShapeType = EMordecaiHitShapeType::ArcSector;
		P->HitShapeParams.Radius = Radius;
		P->HitShapeParams.Angle = Angle;
		P->WindupTimeMs = WindupMs;
		P->ActiveTimeMs = ActiveMs;
		P->RecoveryTimeMs = RecoveryMs;
		P->DamageProfile.BasePower = BasePower;
		P->DamageProfile.DamageType = DmgType;
		P->StaminaCost = StaminaCost;
		P->PostureDamageScalar = PostureScalar;
		P->RootedDuring = Rooted;
		P->JumpAvoidable = true;
		P->HitsAirborne = false;
		P->CancelableIntoDodge = bCancelIntoDodge;
		P->CancelableIntoBlock = bCancelIntoDodge;
		return P;
	}

	/** Create a thrust (Capsule) attack profile. */
	UMordecaiAttackProfileDataAsset* MakeThrust(
		UObject* Outer,
		float Length, float Width,
		float WindupMs, float ActiveMs, float RecoveryMs,
		float BasePower, float StaminaCost, float PostureScalar,
		EMordecaiDamageType DmgType,
		EMordecaiRootedMode Rooted = EMordecaiRootedMode::None,
		bool bCancelIntoDodge = true)
	{
		UMordecaiAttackProfileDataAsset* P = NewObject<UMordecaiAttackProfileDataAsset>(Outer);
		P->AttackType = EMordecaiAttackType::MeleeThrust;
		P->InputSlot = EMordecaiInputSlot::Light;
		P->HitShapeType = EMordecaiHitShapeType::Capsule;
		P->HitShapeParams.Length = Length;
		P->HitShapeParams.Width = Width;
		P->WindupTimeMs = WindupMs;
		P->ActiveTimeMs = ActiveMs;
		P->RecoveryTimeMs = RecoveryMs;
		P->DamageProfile.BasePower = BasePower;
		P->DamageProfile.DamageType = DmgType;
		P->StaminaCost = StaminaCost;
		P->PostureDamageScalar = PostureScalar;
		P->RootedDuring = Rooted;
		P->HitsAirborne = true;
		P->JumpAvoidable = false;
		P->CancelableIntoDodge = bCancelIntoDodge;
		P->CancelableIntoBlock = bCancelIntoDodge;
		return P;
	}

	/** Create a slam (Circle) attack profile. */
	UMordecaiAttackProfileDataAsset* MakeSlam(
		UObject* Outer,
		float Radius,
		float WindupMs, float ActiveMs, float RecoveryMs,
		float BasePower, float StaminaCost, float PostureScalar,
		EMordecaiDamageType DmgType,
		EMordecaiRootedMode Rooted = EMordecaiRootedMode::None,
		bool bCancelIntoDodge = false)
	{
		UMordecaiAttackProfileDataAsset* P = NewObject<UMordecaiAttackProfileDataAsset>(Outer);
		P->AttackType = EMordecaiAttackType::MeleeSlam;
		P->InputSlot = EMordecaiInputSlot::Heavy;
		P->HitShapeType = EMordecaiHitShapeType::Circle;
		P->HitShapeParams.Radius = Radius;
		P->WindupTimeMs = WindupMs;
		P->ActiveTimeMs = ActiveMs;
		P->RecoveryTimeMs = RecoveryMs;
		P->DamageProfile.BasePower = BasePower;
		P->DamageProfile.DamageType = DmgType;
		P->StaminaCost = StaminaCost;
		P->PostureDamageScalar = PostureScalar;
		P->RootedDuring = Rooted;
		P->HitsAirborne = true;
		P->JumpAvoidable = false;
		P->CancelableIntoDodge = bCancelIntoDodge;
		P->CancelableIntoBlock = false;
		return P;
	}
}

// ---------------------------------------------------------------------------
// Longsword — Balanced sweep sword (MainHand)
// ---------------------------------------------------------------------------

UMordecaiWeaponDataAsset* UMordecaiWeaponProfileFactory::CreateLongsword(UObject* Outer)
{
	UMordecaiWeaponDataAsset* W = NewObject<UMordecaiWeaponDataAsset>(Outer);
	W->WeaponId = FName(TEXT("Longsword"));
	W->DisplayName = FText::FromString(TEXT("Longsword"));
	W->WeaponType = EMordecaiWeaponType::Longsword;
	W->EquipSlot = EMordecaiEquipSlot::MainHand;
	W->Rarity = EMordecaiItemRarity::Common;
	W->BaseDamage = 4.f;
	W->AttackSpeedMultiplier = 1.f;
	W->Range = 200.f;
	W->PostureDamageBonus = 1.f;
	W->GrantedTags.AddTag(MordecaiGameplayTags::Weapon_Type_Sword);

	const EMordecaiDamageType Slash = EMordecaiDamageType::Slash;

	// L1: ArcSector(200, 120), 200/150/250, BP=12, Stam=8, PS=0.5
	W->LightAttackProfiles.Add(MakeSweep(Outer, 200.f, 120.f, 200.f, 150.f, 250.f, 12.f, 8.f, 0.5f, Slash));
	// L2: ArcSector(200, 150), 180/170/250, BP=14, Stam=10, PS=0.5
	W->LightAttackProfiles.Add(MakeSweep(Outer, 200.f, 150.f, 180.f, 170.f, 250.f, 14.f, 10.f, 0.5f, Slash));
	// L3: ArcSector(220, 180), 250/200/350, BP=18, Stam=14, PS=0.8
	W->LightAttackProfiles.Add(MakeSweep(Outer, 220.f, 180.f, 250.f, 200.f, 350.f, 18.f, 14.f, 0.8f, Slash));

	// Heavy: ArcSector(220, 360), 500/250/500, BP=28, Stam=25, PS=1.5, Rooted=Active
	auto* Heavy = MakeSweep(Outer, 220.f, 360.f, 500.f, 250.f, 500.f, 28.f, 25.f, 1.5f, Slash, EMordecaiRootedMode::Active, /*bCancel=*/false);
	Heavy->InputSlot = EMordecaiInputSlot::Heavy;
	W->HeavyAttackProfile = Heavy;

	return W;
}

// ---------------------------------------------------------------------------
// Greatsword — Slow, powerful two-hander (TwoHand)
// ---------------------------------------------------------------------------

UMordecaiWeaponDataAsset* UMordecaiWeaponProfileFactory::CreateGreatsword(UObject* Outer)
{
	UMordecaiWeaponDataAsset* W = NewObject<UMordecaiWeaponDataAsset>(Outer);
	W->WeaponId = FName(TEXT("Greatsword"));
	W->DisplayName = FText::FromString(TEXT("Greatsword"));
	W->WeaponType = EMordecaiWeaponType::Greatsword;
	W->EquipSlot = EMordecaiEquipSlot::TwoHand;
	W->Rarity = EMordecaiItemRarity::Common;
	W->BaseDamage = 8.f;
	W->AttackSpeedMultiplier = 0.75f;
	W->Range = 250.f;
	W->PostureDamageBonus = 4.f;
	W->GrantedTags.AddTag(MordecaiGameplayTags::Weapon_Type_Sword);

	const EMordecaiDamageType Slash = EMordecaiDamageType::Slash;

	// L1: ArcSector(250, 180), 350/200/400, BP=20, Stam=14, PS=0.8
	W->LightAttackProfiles.Add(MakeSweep(Outer, 250.f, 180.f, 350.f, 200.f, 400.f, 20.f, 14.f, 0.8f, Slash));
	// L2: ArcSector(260, 200), 400/250/500, BP=26, Stam=18, PS=1.0
	W->LightAttackProfiles.Add(MakeSweep(Outer, 260.f, 200.f, 400.f, 250.f, 500.f, 26.f, 18.f, 1.0f, Slash));

	// Heavy: MeleeSlam, Circle(180), 600/200/600, BP=35, Stam=30, PS=2.0, Rooted=Active, HitsAirborne=true
	W->HeavyAttackProfile = MakeSlam(Outer, 180.f, 600.f, 200.f, 600.f, 35.f, 30.f, 2.0f, Slash, EMordecaiRootedMode::Active);

	return W;
}

// ---------------------------------------------------------------------------
// Shortsword — Fast, low commitment (MainHand)
// ---------------------------------------------------------------------------

UMordecaiWeaponDataAsset* UMordecaiWeaponProfileFactory::CreateShortsword(UObject* Outer)
{
	UMordecaiWeaponDataAsset* W = NewObject<UMordecaiWeaponDataAsset>(Outer);
	W->WeaponId = FName(TEXT("Shortsword"));
	W->DisplayName = FText::FromString(TEXT("Shortsword"));
	W->WeaponType = EMordecaiWeaponType::Shortsword;
	W->EquipSlot = EMordecaiEquipSlot::MainHand;
	W->Rarity = EMordecaiItemRarity::Common;
	W->BaseDamage = 2.f;
	W->AttackSpeedMultiplier = 1.2f;
	W->Range = 150.f;
	W->PostureDamageBonus = 0.f;
	W->GrantedTags.AddTag(MordecaiGameplayTags::Weapon_Type_Sword);

	const EMordecaiDamageType Slash = EMordecaiDamageType::Slash;

	// L1: ArcSector(150, 90), 140/120/180, BP=8, Stam=5, PS=0.3
	W->LightAttackProfiles.Add(MakeSweep(Outer, 150.f, 90.f, 140.f, 120.f, 180.f, 8.f, 5.f, 0.3f, Slash));
	// L2: ArcSector(150, 100), 130/120/170, BP=9, Stam=6, PS=0.3
	W->LightAttackProfiles.Add(MakeSweep(Outer, 150.f, 100.f, 130.f, 120.f, 170.f, 9.f, 6.f, 0.3f, Slash));
	// L3: ArcSector(160, 110), 140/130/190, BP=10, Stam=7, PS=0.4
	W->LightAttackProfiles.Add(MakeSweep(Outer, 160.f, 110.f, 140.f, 130.f, 190.f, 10.f, 7.f, 0.4f, Slash));
	// L4: MeleeThrust, Capsule(170, 60), 180/150/250, BP=14, Stam=10, PS=0.6, HA=true
	W->LightAttackProfiles.Add(MakeThrust(Outer, 170.f, 60.f, 180.f, 150.f, 250.f, 14.f, 10.f, 0.6f, Slash));

	// Heavy: MeleeThrust, Capsule(180, 70), 300/150/350, BP=20, Stam=18, PS=1.0, HA=true
	auto* Heavy = MakeThrust(Outer, 180.f, 70.f, 300.f, 150.f, 350.f, 20.f, 18.f, 1.0f, Slash, EMordecaiRootedMode::None, /*bCancel=*/false);
	Heavy->InputSlot = EMordecaiInputSlot::Heavy;
	W->HeavyAttackProfile = Heavy;

	return W;
}

// ---------------------------------------------------------------------------
// Dagger — Fastest, combo-oriented (MainHand)
// ---------------------------------------------------------------------------

UMordecaiWeaponDataAsset* UMordecaiWeaponProfileFactory::CreateDagger(UObject* Outer)
{
	UMordecaiWeaponDataAsset* W = NewObject<UMordecaiWeaponDataAsset>(Outer);
	W->WeaponId = FName(TEXT("Dagger"));
	W->DisplayName = FText::FromString(TEXT("Dagger"));
	W->WeaponType = EMordecaiWeaponType::Dagger;
	W->EquipSlot = EMordecaiEquipSlot::MainHand;
	W->Rarity = EMordecaiItemRarity::Common;
	W->BaseDamage = 1.f;
	W->AttackSpeedMultiplier = 1.4f;
	W->Range = 120.f;
	W->PostureDamageBonus = 0.f;
	W->GrantedTags.AddTag(MordecaiGameplayTags::Weapon_Type_Dagger);

	const EMordecaiDamageType Pierce = EMordecaiDamageType::Pierce;

	// L1: Capsule(120, 40), 100/80/120, BP=5, Stam=3, PS=0.2
	W->LightAttackProfiles.Add(MakeThrust(Outer, 120.f, 40.f, 100.f, 80.f, 120.f, 5.f, 3.f, 0.2f, Pierce));
	// L2: Capsule(120, 40), 90/80/120, BP=5, Stam=3, PS=0.2
	W->LightAttackProfiles.Add(MakeThrust(Outer, 120.f, 40.f, 90.f, 80.f, 120.f, 5.f, 3.f, 0.2f, Pierce));
	// L3: Capsule(120, 45), 80/80/110, BP=6, Stam=4, PS=0.2
	W->LightAttackProfiles.Add(MakeThrust(Outer, 120.f, 45.f, 80.f, 80.f, 110.f, 6.f, 4.f, 0.2f, Pierce));
	// L4: Capsule(125, 45), 80/80/110, BP=6, Stam=4, PS=0.3
	W->LightAttackProfiles.Add(MakeThrust(Outer, 125.f, 45.f, 80.f, 80.f, 110.f, 6.f, 4.f, 0.3f, Pierce));
	// L5: Capsule(150, 50), 120/100/200, BP=10, Stam=8, PS=0.5
	W->LightAttackProfiles.Add(MakeThrust(Outer, 150.f, 50.f, 120.f, 100.f, 200.f, 10.f, 8.f, 0.5f, Pierce));

	// Heavy: Capsule(160, 50), 250/120/300, BP=22, Stam=15, PS=1.2, HA=true
	auto* Heavy = MakeThrust(Outer, 160.f, 50.f, 250.f, 120.f, 300.f, 22.f, 15.f, 1.2f, Pierce, EMordecaiRootedMode::None, /*bCancel=*/false);
	Heavy->InputSlot = EMordecaiInputSlot::Heavy;
	W->HeavyAttackProfile = Heavy;

	return W;
}

// ---------------------------------------------------------------------------
// Axe — Aggressive slash weapon, frontloaded burst (MainHand)
// ---------------------------------------------------------------------------

UMordecaiWeaponDataAsset* UMordecaiWeaponProfileFactory::CreateAxe(UObject* Outer)
{
	UMordecaiWeaponDataAsset* W = NewObject<UMordecaiWeaponDataAsset>(Outer);
	W->WeaponId = FName(TEXT("Axe"));
	W->DisplayName = FText::FromString(TEXT("Axe"));
	W->WeaponType = EMordecaiWeaponType::Axe;
	W->EquipSlot = EMordecaiEquipSlot::MainHand;
	W->Rarity = EMordecaiItemRarity::Common;
	W->BaseDamage = 6.f;
	W->AttackSpeedMultiplier = 0.85f;
	W->Range = 180.f;
	W->PostureDamageBonus = 3.f;
	W->GrantedTags.AddTag(MordecaiGameplayTags::Weapon_Type_Axe);

	const EMordecaiDamageType Slash = EMordecaiDamageType::Slash;

	// L1: ArcSector(180, 140), 280/180/350, BP=16, Stam=12, PS=0.7
	W->LightAttackProfiles.Add(MakeSweep(Outer, 180.f, 140.f, 280.f, 180.f, 350.f, 16.f, 12.f, 0.7f, Slash));
	// L2: ArcSector(190, 160), 320/200/400, BP=22, Stam=16, PS=0.9
	W->LightAttackProfiles.Add(MakeSweep(Outer, 190.f, 160.f, 320.f, 200.f, 400.f, 22.f, 16.f, 0.9f, Slash));

	// Heavy: MeleeSlam, Circle(160), 500/200/550, BP=30, Stam=24, PS=1.8, Rooted=Active
	W->HeavyAttackProfile = MakeSlam(Outer, 160.f, 500.f, 200.f, 550.f, 30.f, 24.f, 1.8f, Slash, EMordecaiRootedMode::Active);

	return W;
}

// ---------------------------------------------------------------------------
// Mace — Blunt posture-breaker (MainHand)
// ---------------------------------------------------------------------------

UMordecaiWeaponDataAsset* UMordecaiWeaponProfileFactory::CreateMace(UObject* Outer)
{
	UMordecaiWeaponDataAsset* W = NewObject<UMordecaiWeaponDataAsset>(Outer);
	W->WeaponId = FName(TEXT("Mace"));
	W->DisplayName = FText::FromString(TEXT("Mace"));
	W->WeaponType = EMordecaiWeaponType::Mace;
	W->EquipSlot = EMordecaiEquipSlot::MainHand;
	W->Rarity = EMordecaiItemRarity::Common;
	W->BaseDamage = 5.f;
	W->AttackSpeedMultiplier = 0.8f;
	W->Range = 160.f;
	W->PostureDamageBonus = 5.f;
	W->GrantedTags.AddTag(MordecaiGameplayTags::Weapon_Type_Mace);

	const EMordecaiDamageType Blunt = EMordecaiDamageType::Blunt;

	// L1: MeleeSweep, ArcSector(160, 150), 300/180/400, BP=14, Stam=12, PS=1.0
	W->LightAttackProfiles.Add(MakeSweep(Outer, 160.f, 150.f, 300.f, 180.f, 400.f, 14.f, 12.f, 1.0f, Blunt));
	// L2: MeleeSlam, Circle(140), 320/200/450, BP=18, Stam=14, PS=1.2, HA=true
	auto* L2 = MakeSlam(Outer, 140.f, 320.f, 200.f, 450.f, 18.f, 14.f, 1.2f, Blunt);
	L2->InputSlot = EMordecaiInputSlot::Light;
	L2->CancelableIntoDodge = true;
	L2->CancelableIntoBlock = true;
	W->LightAttackProfiles.Add(L2);

	// Heavy: MeleeSlam, Circle(180), 600/200/600, BP=28, Stam=28, PS=2.5, Rooted=Active
	W->HeavyAttackProfile = MakeSlam(Outer, 180.f, 600.f, 200.f, 600.f, 28.f, 28.f, 2.5f, Blunt, EMordecaiRootedMode::Active);

	return W;
}

// ---------------------------------------------------------------------------
// Spear — Pierce polearm, longest melee reach (TwoHand)
// ---------------------------------------------------------------------------

UMordecaiWeaponDataAsset* UMordecaiWeaponProfileFactory::CreateSpear(UObject* Outer)
{
	UMordecaiWeaponDataAsset* W = NewObject<UMordecaiWeaponDataAsset>(Outer);
	W->WeaponId = FName(TEXT("Spear"));
	W->DisplayName = FText::FromString(TEXT("Spear"));
	W->WeaponType = EMordecaiWeaponType::Spear;
	W->EquipSlot = EMordecaiEquipSlot::TwoHand;
	W->Rarity = EMordecaiItemRarity::Common;
	W->BaseDamage = 5.f;
	W->AttackSpeedMultiplier = 0.9f;
	W->Range = 280.f;
	W->PostureDamageBonus = 2.f;
	W->GrantedTags.AddTag(MordecaiGameplayTags::Weapon_Type_Spear);

	const EMordecaiDamageType Pierce = EMordecaiDamageType::Pierce;

	// L1: MeleeThrust, Capsule(280, 50), 200/150/280, BP=12, Stam=8, PS=0.5
	W->LightAttackProfiles.Add(MakeThrust(Outer, 280.f, 50.f, 200.f, 150.f, 280.f, 12.f, 8.f, 0.5f, Pierce));
	// L2: MeleeThrust, Capsule(290, 55), 220/160/280, BP=14, Stam=10, PS=0.6
	W->LightAttackProfiles.Add(MakeThrust(Outer, 290.f, 55.f, 220.f, 160.f, 280.f, 14.f, 10.f, 0.6f, Pierce));
	// L3: MeleeSweep, ArcSector(250, 160), 280/200/350, BP=18, Stam=14, PS=0.8
	W->LightAttackProfiles.Add(MakeSweep(Outer, 250.f, 160.f, 280.f, 200.f, 350.f, 18.f, 14.f, 0.8f, Pierce));

	// Heavy: MeleeThrust, Capsule(320, 60), 400/180/450, BP=26, Stam=22, PS=1.3, Rooted=Active
	auto* Heavy = MakeThrust(Outer, 320.f, 60.f, 400.f, 180.f, 450.f, 26.f, 22.f, 1.3f, Pierce, EMordecaiRootedMode::Active, /*bCancel=*/false);
	Heavy->InputSlot = EMordecaiInputSlot::Heavy;
	W->HeavyAttackProfile = Heavy;

	return W;
}

// ---------------------------------------------------------------------------
// Quarterstaff — Versatile two-handed blunt (TwoHand)
// ---------------------------------------------------------------------------

UMordecaiWeaponDataAsset* UMordecaiWeaponProfileFactory::CreateQuarterstaff(UObject* Outer)
{
	UMordecaiWeaponDataAsset* W = NewObject<UMordecaiWeaponDataAsset>(Outer);
	W->WeaponId = FName(TEXT("Quarterstaff"));
	W->DisplayName = FText::FromString(TEXT("Quarterstaff"));
	W->WeaponType = EMordecaiWeaponType::Quarterstaff;
	W->EquipSlot = EMordecaiEquipSlot::TwoHand;
	W->Rarity = EMordecaiItemRarity::Common;
	W->BaseDamage = 3.f;
	W->AttackSpeedMultiplier = 1.f;
	W->Range = 240.f;
	W->PostureDamageBonus = 2.f;
	W->GrantedTags.AddTag(MordecaiGameplayTags::Weapon_Type_Staff);

	const EMordecaiDamageType Blunt = EMordecaiDamageType::Blunt;

	// L1: MeleeSweep, ArcSector(240, 130), 200/160/250, BP=10, Stam=7, PS=0.5
	W->LightAttackProfiles.Add(MakeSweep(Outer, 240.f, 130.f, 200.f, 160.f, 250.f, 10.f, 7.f, 0.5f, Blunt));
	// L2: MeleeThrust, Capsule(250, 50), 180/140/240, BP=11, Stam=8, PS=0.5
	W->LightAttackProfiles.Add(MakeThrust(Outer, 250.f, 50.f, 180.f, 140.f, 240.f, 11.f, 8.f, 0.5f, Blunt));
	// L3: MeleeSweep, ArcSector(250, 200), 250/180/320, BP=15, Stam=12, PS=0.7
	W->LightAttackProfiles.Add(MakeSweep(Outer, 250.f, 200.f, 250.f, 180.f, 320.f, 15.f, 12.f, 0.7f, Blunt));

	// Heavy: MeleeSweep, ArcSector(260, 360), 400/220/450, BP=22, Stam=20, PS=1.2, Rooted=Active
	auto* Heavy = MakeSweep(Outer, 260.f, 360.f, 400.f, 220.f, 450.f, 22.f, 20.f, 1.2f, Blunt, EMordecaiRootedMode::Active, /*bCancel=*/false);
	Heavy->InputSlot = EMordecaiInputSlot::Heavy;
	W->HeavyAttackProfile = Heavy;

	return W;
}

// ---------------------------------------------------------------------------
// Unarmed — Weakest but always available (MainHand fallback)
// ---------------------------------------------------------------------------

UMordecaiWeaponDataAsset* UMordecaiWeaponProfileFactory::CreateUnarmed(UObject* Outer)
{
	UMordecaiWeaponDataAsset* W = NewObject<UMordecaiWeaponDataAsset>(Outer);
	W->WeaponId = FName(TEXT("Unarmed"));
	W->DisplayName = FText::FromString(TEXT("Unarmed"));
	W->WeaponType = EMordecaiWeaponType::Unarmed;
	W->EquipSlot = EMordecaiEquipSlot::MainHand;
	W->Rarity = EMordecaiItemRarity::Common;
	W->BaseDamage = 0.f;
	W->AttackSpeedMultiplier = 1.3f;
	W->Range = 100.f;
	W->PostureDamageBonus = 0.f;
	W->GrantedTags.AddTag(MordecaiGameplayTags::Weapon_Type_Unarmed);

	const EMordecaiDamageType Blunt = EMordecaiDamageType::Blunt;

	// L1: Capsule(100, 40), 80/60/100, BP=3, Stam=2, PS=0.1
	W->LightAttackProfiles.Add(MakeThrust(Outer, 100.f, 40.f, 80.f, 60.f, 100.f, 3.f, 2.f, 0.1f, Blunt));
	// L2: Capsule(100, 40), 80/60/100, BP=3, Stam=2, PS=0.1
	W->LightAttackProfiles.Add(MakeThrust(Outer, 100.f, 40.f, 80.f, 60.f, 100.f, 3.f, 2.f, 0.1f, Blunt));
	// L3: Capsule(110, 45), 100/80/140, BP=5, Stam=4, PS=0.2
	W->LightAttackProfiles.Add(MakeThrust(Outer, 110.f, 45.f, 100.f, 80.f, 140.f, 5.f, 4.f, 0.2f, Blunt));

	// Heavy: MeleeSlam, Circle(80), 200/100/250, BP=8, Stam=8, PS=0.5, Rooted=Active
	W->HeavyAttackProfile = MakeSlam(Outer, 80.f, 200.f, 100.f, 250.f, 8.f, 8.f, 0.5f, Blunt, EMordecaiRootedMode::Active);

	return W;
}
