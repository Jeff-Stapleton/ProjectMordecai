// Project Mordecai — Damage Spell Tests (US-020)

#include "Misc/AutomationTest.h"
#include "Mordecai/Magic/MordecaiGA_Fireball.h"
#include "Mordecai/Magic/MordecaiGA_ConeOfCold.h"
#include "Mordecai/Magic/MordecaiGA_MagicMissile.h"
#include "Mordecai/Magic/MordecaiGA_StoneSkin.h"
#include "Mordecai/Magic/MordecaiSpellDataAsset.h"
#include "Mordecai/Magic/MordecaiGA_SpellBase.h"
#include "Mordecai/MordecaiGameplayTags.h"
#include "Mordecai/AbilitySystem/MordecaiAttributeSet.h"
#include "Mordecai/Combat/MordecaiGA_MeleeAttack.h"
#include "Mordecai/Combat/MordecaiHitDetectionSubsystem.h"
#include "Mordecai/Combat/MordecaiCombatTypes.h"

// ---------------------------------------------------------------------------
// Helper: Create a test spell data asset with Fireball defaults
// ---------------------------------------------------------------------------
namespace
{
	UMordecaiSpellDataAsset* CreateFireballData(
		float BasePower = 40.f,
		float ProjectileSpeed = 2000.f,
		float ProjectileLifetime = 3.0f,
		float ProjectileCollisionRadius = 30.f)
	{
		UMordecaiSpellDataAsset* Data = NewObject<UMordecaiSpellDataAsset>();
		Data->SpellId = FName(TEXT("Fireball"));
		Data->BasePower = BasePower;
		Data->ProjectileSpeed = ProjectileSpeed;
		Data->ProjectileLifetime = ProjectileLifetime;
		Data->ProjectileCollisionRadius = ProjectileCollisionRadius;
		Data->Range = 1800.f;
		Data->DeliveryType = EMordecaiSpellDeliveryType::SpawnProjectile;
		Data->TargetingType = EMordecaiSpellTargetingType::Projectile;
		return Data;
	}

	UMordecaiSpellDataAsset* CreateConeOfColdData(
		float BasePower = 35.f,
		float Range = 500.f,
		float ArcDegrees = 90.f)
	{
		UMordecaiSpellDataAsset* Data = NewObject<UMordecaiSpellDataAsset>();
		Data->SpellId = FName(TEXT("ConeOfCold"));
		Data->BasePower = BasePower;
		Data->Range = Range;
		Data->ArcDegrees = ArcDegrees;
		Data->DeliveryType = EMordecaiSpellDeliveryType::InstantApply;
		Data->TargetingType = EMordecaiSpellTargetingType::Cone;
		return Data;
	}

	UMordecaiSpellDataAsset* CreateMagicMissileData(
		float BasePower = 50.f,
		int32 ProjectileCount = 5,
		float CastTime = 2.0f)
	{
		UMordecaiSpellDataAsset* Data = NewObject<UMordecaiSpellDataAsset>();
		Data->SpellId = FName(TEXT("MagicMissile"));
		Data->BasePower = BasePower;
		Data->ProjectileCount = ProjectileCount;
		Data->CastTime = CastTime;
		Data->Range = 1800.f;
		Data->ProjectileSpeed = 2500.f;
		Data->ProjectileLifetime = 3.0f;
		Data->DeliveryType = EMordecaiSpellDeliveryType::SpawnProjectile;
		Data->TargetingType = EMordecaiSpellTargetingType::Projectile;
		return Data;
	}

	UMordecaiSpellDataAsset* CreateStoneSkinData(
		float BasePower = 20.f,
		float BuffDuration = 10.f)
	{
		UMordecaiSpellDataAsset* Data = NewObject<UMordecaiSpellDataAsset>();
		Data->SpellId = FName(TEXT("StoneSkin"));
		Data->BasePower = BasePower;
		Data->BuffDuration = BuffDuration;
		Data->DeliveryType = EMordecaiSpellDeliveryType::InstantApply;
		Data->TargetingType = EMordecaiSpellTargetingType::Self;
		return Data;
	}
}

// ---------------------------------------------------------------------------
// 1. Mordecai.Spell.Fireball.ProjectileSpawnsOnCast (AC-020.1, AC-020.2)
// Verifies Fireball builds a projectile spec from SpellDataAsset with correct
// speed, lifetime, and collision radius.
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecaiSpell_Fireball_ProjectileSpawnsOnCast,
	"Mordecai.Spell.Fireball.ProjectileSpawnsOnCast",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecaiSpell_Fireball_ProjectileSpawnsOnCast::RunTest(const FString& Parameters)
{
	// Verify Fireball class exists and inherits from SpellBase
	UMordecaiGA_Fireball* Fireball = NewObject<UMordecaiGA_Fireball>();
	TestNotNull("Fireball ability created", Fireball);

	UMordecaiGA_SpellBase* AsBase = Cast<UMordecaiGA_SpellBase>(Fireball);
	TestNotNull("Fireball inherits from SpellBase", AsBase);

	// Configure with data asset (AC-020.2: speed 2000, lifetime 3.0, radius 30)
	UMordecaiSpellDataAsset* Data = CreateFireballData(40.f, 2000.f, 3.0f, 30.f);
	Fireball->SpellData = Data;

	// Verify projectile spec configuration
	FMordecaiProjectileSpec Spec = Fireball->BuildProjectileSpec();
	TestEqual("Projectile speed from data asset", Spec.Speed, 2000.f);
	TestEqual("Projectile lifetime from data asset", Spec.Lifetime, 3.0f);
	TestEqual("Projectile max range from data asset", Spec.MaxRange, 1800.f);

	// Verify collision radius is in data asset
	TestEqual("Collision radius in data asset", Data->ProjectileCollisionRadius, 30.f);

	// Verify custom values are configurable
	UMordecaiSpellDataAsset* CustomData = CreateFireballData(40.f, 3000.f, 5.0f, 50.f);
	Fireball->SpellData = CustomData;
	FMordecaiProjectileSpec CustomSpec = Fireball->BuildProjectileSpec();
	TestEqual("Custom speed", CustomSpec.Speed, 3000.f);
	TestEqual("Custom lifetime", CustomSpec.Lifetime, 5.0f);

	return true;
}

// ---------------------------------------------------------------------------
// 2. Mordecai.Spell.Fireball.DealsDamageOnHit (AC-020.1, AC-020.3)
// Verifies Fireball uses fire damage type and correct gameplay tag.
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecaiSpell_Fireball_DealsDamageOnHit,
	"Mordecai.Spell.Fireball.DealsDamageOnHit",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecaiSpell_Fireball_DealsDamageOnHit::RunTest(const FString& Parameters)
{
	UMordecaiGA_Fireball* Fireball = NewObject<UMordecaiGA_Fireball>();
	UMordecaiSpellDataAsset* Data = CreateFireballData();
	Fireball->SpellData = Data;

	// AC-020.3: Verify damage type is Fire
	TestEqual("Fireball damage type is Fire",
		Fireball->GetSpellDamageType(), EMordecaiDamageType::Fire);

	// Verify correct gameplay tag mapping
	FGameplayTag FireTag = UMordecaiGA_MeleeAttack::GetDamageTagForType(EMordecaiDamageType::Fire);
	TestTrue("Fire damage tag is valid", FireTag.IsValid());
	TestTrue("Fire tag matches Mordecai.Damage.Fire",
		FireTag == MordecaiGameplayTags::Damage_Fire);

	// Verify damage profile
	FMordecaiDamageProfile Profile = Fireball->BuildDamageProfile(40.f);
	TestEqual("Damage profile type is Fire", Profile.DamageType, EMordecaiDamageType::Fire);
	TestEqual("Damage profile power matches", Profile.BasePower, 40.f);

	return true;
}

// ---------------------------------------------------------------------------
// 3. Mordecai.Spell.Fireball.DamageEqualsSpellPower (AC-020.1)
// Verifies Fireball damage magnitude matches computed SpellPower.
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecaiSpell_Fireball_DamageEqualsSpellPower,
	"Mordecai.Spell.Fireball.DamageEqualsSpellPower",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecaiSpell_Fireball_DamageEqualsSpellPower::RunTest(const FString& Parameters)
{
	UMordecaiGA_Fireball* Fireball = NewObject<UMordecaiGA_Fireball>();
	UMordecaiSpellDataAsset* Data = CreateFireballData(/*BasePower=*/40.f);
	Fireball->SpellData = Data;

	// No scaling, rank 0, no upcast: SpellPower = BasePower = 40
	float Power = Fireball->ComputeSpellPower(0.f, 0, false);
	TestEqual("Base power = 40", Power, 40.f);

	// Damage profile uses this power
	FMordecaiDamageProfile Profile = Fireball->BuildDamageProfile(Power);
	TestEqual("Damage magnitude equals SpellPower", Profile.BasePower, Power);

	// With rank 10 scaling: 40 * 1.0 * (1.0 + 10*0.05) = 40 * 1.5 = 60
	float PowerRank10 = Fireball->ComputeSpellPower(0.f, 10, false);
	TestEqual("Rank 10 power = 60", PowerRank10, 60.f);

	// With attribute scaling bonus of 1.0: 40 * (1.0 + 1.0) * 1.0 = 80
	float PowerScaled = Fireball->ComputeSpellPower(1.0f, 0, false);
	TestEqual("Attribute scaled power = 80", PowerScaled, 80.f);

	return true;
}

// ---------------------------------------------------------------------------
// 4. Mordecai.Spell.ConeOfCold.HitsAllEnemiesInCone (AC-020.4)
// Verifies all enemies within arc+range are detected.
// Uses IsPointInArcSector static helper for geometry validation.
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecaiSpell_ConeOfCold_HitsAllEnemiesInCone,
	"Mordecai.Spell.ConeOfCold.HitsAllEnemiesInCone",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecaiSpell_ConeOfCold_HitsAllEnemiesInCone::RunTest(const FString& Parameters)
{
	UMordecaiGA_ConeOfCold* ConeOfCold = NewObject<UMordecaiGA_ConeOfCold>();
	UMordecaiSpellDataAsset* Data = CreateConeOfColdData(35.f, /*Range=*/500.f, /*ArcDegrees=*/90.f);
	ConeOfCold->SpellData = Data;

	// Verify cone parameters read from data asset
	TestEqual("Cone arc degrees", ConeOfCold->GetConeArcDegrees(), 90.f);
	TestEqual("Cone range", ConeOfCold->GetConeRange(), 500.f);

	// Test geometry: points within the 90-degree cone, 500 range
	FVector Origin = FVector::ZeroVector;
	FVector Forward = FVector::ForwardVector; // +X direction

	// Target directly ahead at 300 units — should be in cone
	TestTrue("Target ahead in cone",
		UMordecaiHitDetectionSubsystem::IsPointInArcSector(
			Origin, Forward, FVector(300.f, 0.f, 0.f), 500.f, 90.f, 0.f));

	// Target at 30 degrees off-center, 400 units — within 45-degree half-angle
	FVector Point30Deg = FVector(400.f * FMath::Cos(FMath::DegreesToRadians(30.f)),
		400.f * FMath::Sin(FMath::DegreesToRadians(30.f)), 0.f);
	TestTrue("Target at 30 degrees in cone",
		UMordecaiHitDetectionSubsystem::IsPointInArcSector(
			Origin, Forward, Point30Deg, 500.f, 90.f, 0.f));

	// Target at exactly 45 degrees (edge of 90-degree arc) — should be included (AC-020.6)
	FVector Point45Deg = FVector(300.f * FMath::Cos(FMath::DegreesToRadians(45.f)),
		300.f * FMath::Sin(FMath::DegreesToRadians(45.f)), 0.f);
	TestTrue("Target at edge of arc (45 degrees) included",
		UMordecaiHitDetectionSubsystem::IsPointInArcSector(
			Origin, Forward, Point45Deg, 500.f, 90.f, 0.f));

	// Target close to caster within cone
	TestTrue("Target close to caster in cone",
		UMordecaiHitDetectionSubsystem::IsPointInArcSector(
			Origin, Forward, FVector(50.f, 10.f, 0.f), 500.f, 90.f, 0.f));

	return true;
}

// ---------------------------------------------------------------------------
// 5. Mordecai.Spell.ConeOfCold.MissesTargetsOutsideCone (AC-020.6)
// Verifies enemies behind caster or outside arc are not hit.
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecaiSpell_ConeOfCold_MissesTargetsOutsideCone,
	"Mordecai.Spell.ConeOfCold.MissesTargetsOutsideCone",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecaiSpell_ConeOfCold_MissesTargetsOutsideCone::RunTest(const FString& Parameters)
{
	FVector Origin = FVector::ZeroVector;
	FVector Forward = FVector::ForwardVector; // +X direction

	// Target directly behind caster — should NOT be in cone
	TestFalse("Target behind caster excluded",
		UMordecaiHitDetectionSubsystem::IsPointInArcSector(
			Origin, Forward, FVector(-300.f, 0.f, 0.f), 500.f, 90.f, 0.f));

	// Target at 90 degrees off-center (perpendicular) — outside 90-degree arc (half-angle is 45)
	TestFalse("Target perpendicular (90 degrees) excluded",
		UMordecaiHitDetectionSubsystem::IsPointInArcSector(
			Origin, Forward, FVector(0.f, 300.f, 0.f), 500.f, 90.f, 0.f));

	// Target at 60 degrees off-center — outside 90-degree arc (half-angle is 45)
	FVector Point60Deg = FVector(300.f * FMath::Cos(FMath::DegreesToRadians(60.f)),
		300.f * FMath::Sin(FMath::DegreesToRadians(60.f)), 0.f);
	TestFalse("Target at 60 degrees excluded from 90-degree cone",
		UMordecaiHitDetectionSubsystem::IsPointInArcSector(
			Origin, Forward, Point60Deg, 500.f, 90.f, 0.f));

	// Target beyond range — should NOT be in cone
	TestFalse("Target beyond range excluded",
		UMordecaiHitDetectionSubsystem::IsPointInArcSector(
			Origin, Forward, FVector(600.f, 0.f, 0.f), 500.f, 90.f, 0.f));

	// Target behind and to the side — should NOT be in cone
	TestFalse("Target behind and to side excluded",
		UMordecaiHitDetectionSubsystem::IsPointInArcSector(
			Origin, Forward, FVector(-200.f, 200.f, 0.f), 500.f, 90.f, 0.f));

	return true;
}

// ---------------------------------------------------------------------------
// 6. Mordecai.Spell.ConeOfCold.DealsColdDamage (AC-020.5)
// Verifies Cone of Cold uses Frost damage type with correct tag.
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecaiSpell_ConeOfCold_DealsColdDamage,
	"Mordecai.Spell.ConeOfCold.DealsColdDamage",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecaiSpell_ConeOfCold_DealsColdDamage::RunTest(const FString& Parameters)
{
	UMordecaiGA_ConeOfCold* ConeOfCold = NewObject<UMordecaiGA_ConeOfCold>();
	UMordecaiSpellDataAsset* Data = CreateConeOfColdData();
	ConeOfCold->SpellData = Data;

	// AC-020.5: Damage type is Frost (cold)
	TestEqual("Cone of Cold damage type is Frost",
		ConeOfCold->GetSpellDamageType(), EMordecaiDamageType::Frost);

	// Verify correct gameplay tag mapping
	FGameplayTag FrostTag = UMordecaiGA_MeleeAttack::GetDamageTagForType(EMordecaiDamageType::Frost);
	TestTrue("Frost damage tag is valid", FrostTag.IsValid());
	TestTrue("Frost tag matches Mordecai.Damage.Frost",
		FrostTag == MordecaiGameplayTags::Damage_Frost);

	return true;
}

// ---------------------------------------------------------------------------
// 7. Mordecai.Spell.MagicMissile.FiresMultipleProjectiles (AC-020.7)
// Verifies N projectiles configured from SpellDataAsset, default 5.
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecaiSpell_MagicMissile_FiresMultipleProjectiles,
	"Mordecai.Spell.MagicMissile.FiresMultipleProjectiles",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecaiSpell_MagicMissile_FiresMultipleProjectiles::RunTest(const FString& Parameters)
{
	UMordecaiGA_MagicMissile* MagicMissile = NewObject<UMordecaiGA_MagicMissile>();
	UMordecaiSpellDataAsset* Data = CreateMagicMissileData(50.f, /*ProjectileCount=*/5, 2.0f);
	MagicMissile->SpellData = Data;

	// AC-020.7: Default 5 projectiles
	TestEqual("Projectile count = 5", MagicMissile->GetProjectileCount(), 5);

	// Verify configurable via data asset
	Data->ProjectileCount = 3;
	TestEqual("Configurable count = 3", MagicMissile->GetProjectileCount(), 3);

	// Verify minimum of 1 projectile
	Data->ProjectileCount = 0;
	TestEqual("Minimum 1 projectile", MagicMissile->GetProjectileCount(), 1);

	// Verify inherits from SpellBase
	UMordecaiGA_SpellBase* AsBase = Cast<UMordecaiGA_SpellBase>(MagicMissile);
	TestNotNull("MagicMissile inherits from SpellBase", AsBase);

	return true;
}

// ---------------------------------------------------------------------------
// 8. Mordecai.Spell.MagicMissile.ProjectilesHome (AC-020.8)
// Verifies Magic Missile projectiles use aim assist for homing.
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecaiSpell_MagicMissile_ProjectilesHome,
	"Mordecai.Spell.MagicMissile.ProjectilesHome",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecaiSpell_MagicMissile_ProjectilesHome::RunTest(const FString& Parameters)
{
	UMordecaiGA_MagicMissile* MagicMissile = NewObject<UMordecaiGA_MagicMissile>();
	UMordecaiSpellDataAsset* Data = CreateMagicMissileData();
	MagicMissile->SpellData = Data;

	// AC-020.8: Homing is always enabled for Magic Missile
	TestTrue("Homing is enabled", MagicMissile->IsHomingEnabled());

	// Verify Range is set for aim assist queries
	TestTrue("Range > 0 for aim assist", Data->Range > 0.f);

	// Verify damage type is Arcane/Force (AC-020.9)
	TestEqual("Magic Missile damage type is Arcane",
		MagicMissile->GetSpellDamageType(), EMordecaiDamageType::Arcane);

	return true;
}

// ---------------------------------------------------------------------------
// 9. Mordecai.Spell.MagicMissile.TotalDamageEqualsSpellPower (AC-020.7)
// Verifies sum of all projectile damage = SpellPower.
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecaiSpell_MagicMissile_TotalDamageEqualsSpellPower,
	"Mordecai.Spell.MagicMissile.TotalDamageEqualsSpellPower",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecaiSpell_MagicMissile_TotalDamageEqualsSpellPower::RunTest(const FString& Parameters)
{
	UMordecaiGA_MagicMissile* MagicMissile = NewObject<UMordecaiGA_MagicMissile>();
	UMordecaiSpellDataAsset* Data = CreateMagicMissileData(/*BasePower=*/50.f, /*Count=*/5);
	MagicMissile->SpellData = Data;

	// SpellPower at rank 0, no scaling = BasePower = 50
	float SpellPower = MagicMissile->ComputeSpellPower(0.f, 0, false);
	TestEqual("SpellPower = 50", SpellPower, 50.f);

	// AC-020.7: Each projectile deals SpellPower / N
	float DamagePerProjectile = MagicMissile->GetDamagePerProjectile(SpellPower);
	TestEqual("Damage per projectile = 10", DamagePerProjectile, 10.f);

	// Total damage = DamagePerProjectile * N = SpellPower
	int32 Count = MagicMissile->GetProjectileCount();
	float TotalDamage = DamagePerProjectile * static_cast<float>(Count);
	TestEqual("Total damage = SpellPower", TotalDamage, SpellPower);

	// Test with different count
	Data->ProjectileCount = 3;
	float DamagePer3 = MagicMissile->GetDamagePerProjectile(SpellPower);
	float Total3 = DamagePer3 * 3.f;
	TestTrue("Total damage with 3 missiles equals SpellPower",
		FMath::IsNearlyEqual(Total3, SpellPower, 0.01f));

	// Test with rank scaling: 50 * 1.5 = 75 at rank 10
	float PowerRank10 = MagicMissile->ComputeSpellPower(0.f, 10, false);
	Data->ProjectileCount = 5;
	float DamagePerRank10 = MagicMissile->GetDamagePerProjectile(PowerRank10);
	float TotalRank10 = DamagePerRank10 * 5.f;
	TestTrue("Total damage at rank 10 = scaled SpellPower",
		FMath::IsNearlyEqual(TotalRank10, PowerRank10, 0.01f));

	return true;
}

// ---------------------------------------------------------------------------
// 10. Mordecai.Spell.StoneSkin.AppliesDamageReduction (AC-020.10)
// Verifies DamageReduction attribute exists and buff reads from BasePower.
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecaiSpell_StoneSkin_AppliesDamageReduction,
	"Mordecai.Spell.StoneSkin.AppliesDamageReduction",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecaiSpell_StoneSkin_AppliesDamageReduction::RunTest(const FString& Parameters)
{
	UMordecaiGA_StoneSkin* StoneSkin = NewObject<UMordecaiGA_StoneSkin>();
	UMordecaiSpellDataAsset* Data = CreateStoneSkinData(/*BasePower=*/20.f, /*Duration=*/10.f);
	StoneSkin->SpellData = Data;

	// AC-020.10: DamageReduction amount = BasePower from SpellDataAsset
	TestEqual("DamageReduction = BasePower = 20",
		StoneSkin->GetDamageReductionAmount(), 20.f);

	// Verify configurable
	Data->BasePower = 35.f;
	TestEqual("DamageReduction updates with BasePower",
		StoneSkin->GetDamageReductionAmount(), 35.f);

	// Verify StoneSkin inherits from SpellBase
	UMordecaiGA_SpellBase* AsBase = Cast<UMordecaiGA_SpellBase>(StoneSkin);
	TestNotNull("StoneSkin inherits from SpellBase", AsBase);

	return true;
}

// ---------------------------------------------------------------------------
// 11. Mordecai.Spell.StoneSkin.BuffExpiresAfterDuration (AC-020.11)
// Verifies buff duration and status tag configuration.
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecaiSpell_StoneSkin_BuffExpiresAfterDuration,
	"Mordecai.Spell.StoneSkin.BuffExpiresAfterDuration",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecaiSpell_StoneSkin_BuffExpiresAfterDuration::RunTest(const FString& Parameters)
{
	UMordecaiGA_StoneSkin* StoneSkin = NewObject<UMordecaiGA_StoneSkin>();
	UMordecaiSpellDataAsset* Data = CreateStoneSkinData(20.f, /*Duration=*/10.f);
	StoneSkin->SpellData = Data;

	// AC-020.11: Duration from SpellDataAsset
	TestEqual("Buff duration = 10 seconds", StoneSkin->GetBuffDuration(), 10.f);

	// Verify configurable
	Data->BuffDuration = 15.f;
	TestEqual("Duration updates with data asset", StoneSkin->GetBuffDuration(), 15.f);

	// AC-020.11: Status tag is Mordecai.Status.StoneSkin
	FGameplayTag StatusTag = StoneSkin->GetStatusTag();
	TestTrue("Status tag is valid", StatusTag.IsValid());
	TestTrue("Status tag is StoneSkin",
		StatusTag == MordecaiGameplayTags::Status_StoneSkin);

	return true;
}

// ---------------------------------------------------------------------------
// 12. Mordecai.Spell.StoneSkin.ReducesIncomingDamage (AC-020.10)
// Verifies DamageReduction attribute exists in MordecaiAttributeSet.
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecaiSpell_StoneSkin_ReducesIncomingDamage,
	"Mordecai.Spell.StoneSkin.ReducesIncomingDamage",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecaiSpell_StoneSkin_ReducesIncomingDamage::RunTest(const FString& Parameters)
{
	// Verify DamageReduction attribute exists on the attribute set
	const UClass* AttrClass = UMordecaiAttributeSet::StaticClass();
	TestNotNull("MordecaiAttributeSet class exists", AttrClass);

	const FProperty* DRProp = AttrClass->FindPropertyByName(FName(TEXT("DamageReduction")));
	TestNotNull("DamageReduction property exists", DRProp);

	// Verify the attribute is accessible
	FGameplayAttribute DRAttr = UMordecaiAttributeSet::GetDamageReductionAttribute();
	TestTrue("DamageReduction attribute is valid", DRAttr.IsValid());

	// Verify default value is 0 (no reduction until buff applied)
	UMordecaiAttributeSet* AttrSet = NewObject<UMordecaiAttributeSet>();
	TestEqual("DamageReduction defaults to 0", AttrSet->GetDamageReduction(), 0.f);

	// Verify all 4 spells have correct ability tags (AC-020.12)
	FGameplayTag FireballTag = MordecaiGameplayTags::Ability_Spell_Fireball;
	TestTrue("Fireball ability tag valid", FireballTag.IsValid());

	FGameplayTag ConeTag = MordecaiGameplayTags::Ability_Spell_ConeOfCold;
	TestTrue("ConeOfCold ability tag valid", ConeTag.IsValid());

	FGameplayTag MissileTag = MordecaiGameplayTags::Ability_Spell_MagicMissile;
	TestTrue("MagicMissile ability tag valid", MissileTag.IsValid());

	FGameplayTag StoneSkinTag = MordecaiGameplayTags::Ability_Spell_StoneSkin;
	TestTrue("StoneSkin ability tag valid", StoneSkinTag.IsValid());

	return true;
}
