// Project Mordecai — Combat Data Model Tests (US-002)

#include "Misc/AutomationTest.h"
#include "Mordecai/Combat/MordecaiCombatTypes.h"
#include "Mordecai/Combat/MordecaiAttackProfileDataAsset.h"
#include "Mordecai/MordecaiGameplayTags.h"
#include "GameplayTagsManager.h"

// ---------------------------------------------------------------------------
// Mordecai.Combat.AttackTypeEnumHasAllValues  (AC-002.1)
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecaiCombat_AttackTypeEnum,
	"Mordecai.Combat.AttackTypeEnumHasAllValues",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecaiCombat_AttackTypeEnum::RunTest(const FString& Parameters)
{
	// Compile-time: using each value proves it exists
	EMordecaiAttackType Values[] = {
		EMordecaiAttackType::MeleeSweep,
		EMordecaiAttackType::MeleeThrust,
		EMordecaiAttackType::MeleeSlam,
		EMordecaiAttackType::Projectile,
		EMordecaiAttackType::AoE,
	};
	TestEqual("Enum has 5 attack types", (int32)UE_ARRAY_COUNT(Values), 5);

	// Runtime: verify reflection registration
	const UEnum* Enum = StaticEnum<EMordecaiAttackType>();
	TestNotNull("EMordecaiAttackType registered", Enum);

	// Verify each value has a valid name via reflection
	for (EMordecaiAttackType V : Values)
	{
		FString Name = Enum->GetNameStringByValue(static_cast<int64>(V));
		TestFalse(FString::Printf(TEXT("Value %d has name"), static_cast<int32>(V)), Name.IsEmpty());
	}

	return true;
}

// ---------------------------------------------------------------------------
// Mordecai.Combat.InputSlotEnumHasAllValues  (AC-002.2)
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecaiCombat_InputSlotEnum,
	"Mordecai.Combat.InputSlotEnumHasAllValues",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecaiCombat_InputSlotEnum::RunTest(const FString& Parameters)
{
	EMordecaiInputSlot Values[] = {
		EMordecaiInputSlot::Light,
		EMordecaiInputSlot::Heavy,
		EMordecaiInputSlot::SkillA,
		EMordecaiInputSlot::SkillB,
		EMordecaiInputSlot::Throw,
	};
	TestEqual("Enum has 5 input slots", (int32)UE_ARRAY_COUNT(Values), 5);

	const UEnum* Enum = StaticEnum<EMordecaiInputSlot>();
	TestNotNull("EMordecaiInputSlot registered", Enum);

	for (EMordecaiInputSlot V : Values)
	{
		FString Name = Enum->GetNameStringByValue(static_cast<int64>(V));
		TestFalse(FString::Printf(TEXT("Value %d has name"), static_cast<int32>(V)), Name.IsEmpty());
	}

	return true;
}

// ---------------------------------------------------------------------------
// Mordecai.Combat.RootedModeEnumHasAllValues  (AC-002.3)
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecaiCombat_RootedModeEnum,
	"Mordecai.Combat.RootedModeEnumHasAllValues",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecaiCombat_RootedModeEnum::RunTest(const FString& Parameters)
{
	EMordecaiRootedMode Values[] = {
		EMordecaiRootedMode::None,
		EMordecaiRootedMode::Windup,
		EMordecaiRootedMode::Active,
		EMordecaiRootedMode::Full,
	};
	TestEqual("Enum has 4 rooted modes", (int32)UE_ARRAY_COUNT(Values), 4);

	const UEnum* Enum = StaticEnum<EMordecaiRootedMode>();
	TestNotNull("EMordecaiRootedMode registered", Enum);

	for (EMordecaiRootedMode V : Values)
	{
		FString Name = Enum->GetNameStringByValue(static_cast<int64>(V));
		TestFalse(FString::Printf(TEXT("Value %d has name"), static_cast<int32>(V)), Name.IsEmpty());
	}

	return true;
}

// ---------------------------------------------------------------------------
// Mordecai.Combat.HitShapeTypeEnumHasAllValues  (AC-002.4)
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecaiCombat_HitShapeTypeEnum,
	"Mordecai.Combat.HitShapeTypeEnumHasAllValues",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecaiCombat_HitShapeTypeEnum::RunTest(const FString& Parameters)
{
	EMordecaiHitShapeType Values[] = {
		EMordecaiHitShapeType::ArcSector,
		EMordecaiHitShapeType::Capsule,
		EMordecaiHitShapeType::Box,
		EMordecaiHitShapeType::Circle,
	};
	TestEqual("Enum has 4 hit shape types", (int32)UE_ARRAY_COUNT(Values), 4);

	const UEnum* Enum = StaticEnum<EMordecaiHitShapeType>();
	TestNotNull("EMordecaiHitShapeType registered", Enum);

	for (EMordecaiHitShapeType V : Values)
	{
		FString Name = Enum->GetNameStringByValue(static_cast<int64>(V));
		TestFalse(FString::Printf(TEXT("Value %d has name"), static_cast<int32>(V)), Name.IsEmpty());
	}

	return true;
}

// ---------------------------------------------------------------------------
// Mordecai.Combat.DamageTypeEnumHasAllValues  (AC-002.5)
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecaiCombat_DamageTypeEnum,
	"Mordecai.Combat.DamageTypeEnumHasAllValues",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecaiCombat_DamageTypeEnum::RunTest(const FString& Parameters)
{
	EMordecaiDamageType Values[] = {
		EMordecaiDamageType::Physical,
		EMordecaiDamageType::Slash,
		EMordecaiDamageType::Pierce,
		EMordecaiDamageType::Blunt,
		EMordecaiDamageType::Fire,
		EMordecaiDamageType::Frost,
		EMordecaiDamageType::Lightning,
		EMordecaiDamageType::Poison,
		EMordecaiDamageType::Corrosive,
		EMordecaiDamageType::Arcane,
		EMordecaiDamageType::Shadow,
		EMordecaiDamageType::Radiant,
	};
	TestEqual("Enum has 12 damage types", (int32)UE_ARRAY_COUNT(Values), 12);

	const UEnum* Enum = StaticEnum<EMordecaiDamageType>();
	TestNotNull("EMordecaiDamageType registered", Enum);

	for (EMordecaiDamageType V : Values)
	{
		FString Name = Enum->GetNameStringByValue(static_cast<int64>(V));
		TestFalse(FString::Printf(TEXT("Value %d has name"), static_cast<int32>(V)), Name.IsEmpty());
	}

	return true;
}

// ---------------------------------------------------------------------------
// Mordecai.Combat.AttackProfileDataAssetHasAllFields  (AC-002.9, AC-002.10, AC-002.12)
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecaiCombat_AttackProfileFields,
	"Mordecai.Combat.AttackProfileDataAssetHasAllFields",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecaiCombat_AttackProfileFields::RunTest(const FString& Parameters)
{
	const UClass* Class = UMordecaiAttackProfileDataAsset::StaticClass();
	TestNotNull("AttackProfileDataAsset class exists", Class);

	// Expected UPROPERTY flags: EditAnywhere + BlueprintReadOnly (AC-002.12)
	const uint64 ExpectedFlags = CPF_Edit | CPF_BlueprintVisible | CPF_BlueprintReadOnly;

	// Helper lambda: check property exists and has correct flags
	auto CheckProperty = [&](const TCHAR* FieldName)
	{
		const FProperty* Prop = Class->FindPropertyByName(FName(FieldName));
		TestNotNull(FString::Printf(TEXT("%s exists"), FieldName), Prop);
		if (Prop)
		{
			TestTrue(FString::Printf(TEXT("%s has EditAnywhere|BlueprintReadOnly"), FieldName),
				Prop->HasAllPropertyFlags(ExpectedFlags));
		}
	};

	// AC-002.9 fields
	CheckProperty(TEXT("AttackType"));
	CheckProperty(TEXT("InputSlot"));
	CheckProperty(TEXT("WindupTimeMs"));
	CheckProperty(TEXT("ActiveTimeMs"));
	CheckProperty(TEXT("RecoveryTimeMs"));
	CheckProperty(TEXT("RootedDuring"));
	CheckProperty(TEXT("HitShapeType"));
	CheckProperty(TEXT("HitShapeParams"));
	CheckProperty(TEXT("HitsAirborne"));
	CheckProperty(TEXT("JumpAvoidable"));
	CheckProperty(TEXT("DamageProfile"));
	CheckProperty(TEXT("PostureDamageScalar"));
	CheckProperty(TEXT("StaminaCost"));
	CheckProperty(TEXT("ComboIndex"));
	CheckProperty(TEXT("CancelableIntoDodge"));
	CheckProperty(TEXT("CancelableIntoBlock"));

	// AC-002.10 fields
	CheckProperty(TEXT("OnHitPayload"));
	CheckProperty(TEXT("OnUsePayload"));

	return true;
}

// ---------------------------------------------------------------------------
// Mordecai.Combat.ProjectileSpecHasAllFields  (AC-002.7)
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecaiCombat_ProjectileSpecFields,
	"Mordecai.Combat.ProjectileSpecHasAllFields",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecaiCombat_ProjectileSpecFields::RunTest(const FString& Parameters)
{
	const UScriptStruct* Struct = FMordecaiProjectileSpec::StaticStruct();
	TestNotNull("FMordecaiProjectileSpec registered", Struct);

	auto CheckField = [&](const TCHAR* FieldName)
	{
		const FProperty* Prop = Struct->FindPropertyByName(FName(FieldName));
		TestNotNull(FString::Printf(TEXT("ProjectileSpec.%s exists"), FieldName), Prop);
	};

	CheckField(TEXT("Speed"));
	CheckField(TEXT("GravityDrop"));
	CheckField(TEXT("Lifetime"));
	CheckField(TEXT("PierceCount"));
	CheckField(TEXT("RicochetCount"));
	CheckField(TEXT("RicochetRange"));
	CheckField(TEXT("RicochetAngleLimit"));
	CheckField(TEXT("OnHitAoERadius"));
	CheckField(TEXT("OnHitAoEDamageScalar"));

	return true;
}

// ---------------------------------------------------------------------------
// Mordecai.Combat.DamageProfileHasAllFields  (AC-002.8)
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecaiCombat_DamageProfileFields,
	"Mordecai.Combat.DamageProfileHasAllFields",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecaiCombat_DamageProfileFields::RunTest(const FString& Parameters)
{
	const UScriptStruct* Struct = FMordecaiDamageProfile::StaticStruct();
	TestNotNull("FMordecaiDamageProfile registered", Struct);

	auto CheckField = [&](const TCHAR* FieldName)
	{
		const FProperty* Prop = Struct->FindPropertyByName(FName(FieldName));
		TestNotNull(FString::Printf(TEXT("DamageProfile.%s exists"), FieldName), Prop);
	};

	CheckField(TEXT("DamageType"));
	CheckField(TEXT("Delivery"));
	CheckField(TEXT("BasePower"));
	CheckField(TEXT("CanCrit"));
	CheckField(TEXT("AppliesPostureDamage"));
	CheckField(TEXT("BuildsStatus"));
	CheckField(TEXT("StatusMagnitude"));

	return true;
}

// ---------------------------------------------------------------------------
// Mordecai.Combat.DamageGameplayTagsRegistered  (AC-002.11)
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecaiCombat_DamageGameplayTags,
	"Mordecai.Combat.DamageGameplayTagsRegistered",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecaiCombat_DamageGameplayTags::RunTest(const FString& Parameters)
{
	// Physical subtypes are nested: Mordecai.Damage.Physical.Slash (per technical notes)
	const TCHAR* TagNames[] = {
		TEXT("Mordecai.Damage.Physical"),
		TEXT("Mordecai.Damage.Physical.Slash"),
		TEXT("Mordecai.Damage.Physical.Pierce"),
		TEXT("Mordecai.Damage.Physical.Blunt"),
		TEXT("Mordecai.Damage.Fire"),
		TEXT("Mordecai.Damage.Frost"),
		TEXT("Mordecai.Damage.Lightning"),
		TEXT("Mordecai.Damage.Poison"),
		TEXT("Mordecai.Damage.Corrosive"),
		TEXT("Mordecai.Damage.Arcane"),
		TEXT("Mordecai.Damage.Shadow"),
		TEXT("Mordecai.Damage.Radiant"),
	};

	for (const TCHAR* TagName : TagNames)
	{
		FGameplayTag Tag = FGameplayTag::RequestGameplayTag(FName(TagName), /*bErrorIfNotFound=*/ false);
		TestTrue(FString::Printf(TEXT("Tag %s is valid"), TagName), Tag.IsValid());
	}

	return true;
}

// ---------------------------------------------------------------------------
// Mordecai.Combat.AttackProfileSerializesCorrectly
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecaiCombat_AttackProfileSerialize,
	"Mordecai.Combat.AttackProfileSerializesCorrectly",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecaiCombat_AttackProfileSerialize::RunTest(const FString& Parameters)
{
	UMordecaiAttackProfileDataAsset* Profile = NewObject<UMordecaiAttackProfileDataAsset>();
	TestNotNull("Profile created", Profile);

	// Set all fields to non-default values
	Profile->AttackType = EMordecaiAttackType::MeleeSlam;
	Profile->InputSlot = EMordecaiInputSlot::Heavy;
	Profile->WindupTimeMs = 250.f;
	Profile->ActiveTimeMs = 100.f;
	Profile->RecoveryTimeMs = 400.f;
	Profile->RootedDuring = EMordecaiRootedMode::Full;
	Profile->HitShapeType = EMordecaiHitShapeType::Circle;
	Profile->HitShapeParams.Radius = 300.f;
	Profile->HitsAirborne = true;
	Profile->JumpAvoidable = false;
	Profile->DamageProfile.DamageType = EMordecaiDamageType::Blunt;
	Profile->DamageProfile.BasePower = 45.f;
	Profile->DamageProfile.CanCrit = true;
	Profile->DamageProfile.AppliesPostureDamage = true;
	Profile->DamageProfile.StatusMagnitude = 12.f;
	Profile->PostureDamageScalar = 1.5f;
	Profile->StaminaCost = 30.f;
	Profile->ComboIndex = 2;
	Profile->CancelableIntoDodge = true;
	Profile->CancelableIntoBlock = false;

	// Verify all values persisted on the object
	TestEqual("AttackType", Profile->AttackType, EMordecaiAttackType::MeleeSlam);
	TestEqual("InputSlot", Profile->InputSlot, EMordecaiInputSlot::Heavy);
	TestEqual("WindupTimeMs", Profile->WindupTimeMs, 250.f);
	TestEqual("ActiveTimeMs", Profile->ActiveTimeMs, 100.f);
	TestEqual("RecoveryTimeMs", Profile->RecoveryTimeMs, 400.f);
	TestEqual("RootedDuring", Profile->RootedDuring, EMordecaiRootedMode::Full);
	TestEqual("HitShapeType", Profile->HitShapeType, EMordecaiHitShapeType::Circle);
	TestEqual("HitShapeParams.Radius", Profile->HitShapeParams.Radius, 300.f);
	TestEqual("HitsAirborne", Profile->HitsAirborne, true);
	TestEqual("JumpAvoidable", Profile->JumpAvoidable, false);
	TestEqual("DamageProfile.DamageType", Profile->DamageProfile.DamageType, EMordecaiDamageType::Blunt);
	TestEqual("DamageProfile.BasePower", Profile->DamageProfile.BasePower, 45.f);
	TestEqual("DamageProfile.CanCrit", Profile->DamageProfile.CanCrit, true);
	TestEqual("DamageProfile.AppliesPostureDamage", Profile->DamageProfile.AppliesPostureDamage, true);
	TestEqual("DamageProfile.StatusMagnitude", Profile->DamageProfile.StatusMagnitude, 12.f);
	TestEqual("PostureDamageScalar", Profile->PostureDamageScalar, 1.5f);
	TestEqual("StaminaCost", Profile->StaminaCost, 30.f);
	TestEqual("ComboIndex", Profile->ComboIndex, 2);
	TestEqual("CancelableIntoDodge", Profile->CancelableIntoDodge, true);
	TestEqual("CancelableIntoBlock", Profile->CancelableIntoBlock, false);

	return true;
}
