// Project Mordecai — Weapon Framework Tests (US-024)
// All NullRHI-compatible headless tests covering DataAsset fields, equip/unequip,
// stat modifiers, ability granting, tags, two-hand slot management, melee integration,
// unarmed fallback, base damage, attack speed, and GetEquippedWeapon.

#include "Misc/AutomationTest.h"
#include "Engine/World.h"
#include "Engine/Engine.h"
#include "AbilitySystemComponent.h"
#include "Abilities/GameplayAbility.h"
#include "GameplayEffect.h"

#include "Mordecai/Weapons/MordecaiWeaponTypes.h"
#include "Mordecai/Weapons/MordecaiWeaponDataAsset.h"
#include "Mordecai/Weapons/MordecaiEquipmentComponent.h"
#include "Mordecai/Combat/MordecaiAttackProfileDataAsset.h"
#include "Mordecai/Combat/MordecaiCombatTypes.h"
#include "Mordecai/Combat/MordecaiGA_MeleeAttack.h"
#include "Mordecai/AbilitySystem/MordecaiAttributeSet.h"
#include "Mordecai/MordecaiGameplayTags.h"

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

namespace MordecaiWeaponTestHelpers
{
	static UMordecaiAttackProfileDataAsset* CreateProfile(float BasePower, float WindupMs = 200.f, float ActiveMs = 100.f, float RecoveryMs = 300.f)
	{
		UMordecaiAttackProfileDataAsset* Profile = NewObject<UMordecaiAttackProfileDataAsset>();
		Profile->DamageProfile.BasePower = BasePower;
		Profile->DamageProfile.DamageType = EMordecaiDamageType::Slash;
		Profile->WindupTimeMs = WindupMs;
		Profile->ActiveTimeMs = ActiveMs;
		Profile->RecoveryTimeMs = RecoveryMs;
		Profile->PostureDamageScalar = 0.5f;
		Profile->InputSlot = EMordecaiInputSlot::Light;
		return Profile;
	}

	static UMordecaiWeaponDataAsset* CreateSword(FName WeaponId = FName(TEXT("Longsword_Iron")))
	{
		UMordecaiWeaponDataAsset* Asset = NewObject<UMordecaiWeaponDataAsset>();
		Asset->WeaponId = WeaponId;
		Asset->DisplayName = FText::FromString(WeaponId.ToString());
		Asset->WeaponType = EMordecaiWeaponType::Longsword;
		Asset->EquipSlot = EMordecaiEquipSlot::MainHand;
		Asset->Rarity = EMordecaiItemRarity::Common;
		Asset->BaseDamage = 15.f;
		Asset->AttackSpeedMultiplier = 1.0f;
		Asset->Range = 150.f;
		Asset->PostureDamageBonus = 3.f;

		// 3-hit light combo
		Asset->LightAttackProfiles.Add(CreateProfile(20.f));
		Asset->LightAttackProfiles.Add(CreateProfile(25.f));
		Asset->LightAttackProfiles.Add(CreateProfile(35.f));

		// Heavy attack
		Asset->HeavyAttackProfile = CreateProfile(50.f, 400.f, 200.f, 500.f);

		// Sword tag
		Asset->GrantedTags.AddTag(MordecaiGameplayTags::Weapon_Type_Sword);

		return Asset;
	}

	static UMordecaiWeaponDataAsset* CreateGreatsword()
	{
		UMordecaiWeaponDataAsset* Asset = NewObject<UMordecaiWeaponDataAsset>();
		Asset->WeaponId = FName(TEXT("Greatsword_Iron"));
		Asset->DisplayName = FText::FromString(TEXT("Iron Greatsword"));
		Asset->WeaponType = EMordecaiWeaponType::Greatsword;
		Asset->EquipSlot = EMordecaiEquipSlot::TwoHand;
		Asset->Rarity = EMordecaiItemRarity::Green;
		Asset->BaseDamage = 30.f;
		Asset->AttackSpeedMultiplier = 0.8f;
		Asset->Range = 200.f;
		Asset->PostureDamageBonus = 8.f;

		// 2-hit light combo
		Asset->LightAttackProfiles.Add(CreateProfile(40.f, 300.f, 150.f, 400.f));
		Asset->LightAttackProfiles.Add(CreateProfile(55.f, 350.f, 200.f, 500.f));

		Asset->HeavyAttackProfile = CreateProfile(80.f, 600.f, 300.f, 700.f);

		Asset->GrantedTags.AddTag(MordecaiGameplayTags::Weapon_Type_Sword);
		Asset->GrantedTags.AddTag(MordecaiGameplayTags::Weapon_Slot_TwoHand);

		return Asset;
	}

	static UMordecaiWeaponDataAsset* CreateDagger()
	{
		UMordecaiWeaponDataAsset* Asset = NewObject<UMordecaiWeaponDataAsset>();
		Asset->WeaponId = FName(TEXT("Dagger_Iron"));
		Asset->DisplayName = FText::FromString(TEXT("Iron Dagger"));
		Asset->WeaponType = EMordecaiWeaponType::Dagger;
		Asset->EquipSlot = EMordecaiEquipSlot::OffHand;
		Asset->Rarity = EMordecaiItemRarity::Common;
		Asset->BaseDamage = 8.f;
		Asset->AttackSpeedMultiplier = 1.3f;
		Asset->Range = 80.f;
		Asset->PostureDamageBonus = 1.f;

		Asset->LightAttackProfiles.Add(CreateProfile(10.f, 100.f, 80.f, 150.f));

		Asset->GrantedTags.AddTag(MordecaiGameplayTags::Weapon_Type_Dagger);

		return Asset;
	}

	static FMordecaiWeaponInstance MakeInstance(UMordecaiWeaponDataAsset* DataAsset)
	{
		FMordecaiWeaponInstance Instance;
		Instance.InstanceId = FGuid::NewGuid();
		Instance.WeaponDataAsset = DataAsset;
		Instance.IsEquipped = false;
		return Instance;
	}

	static UMordecaiEquipmentComponent* CreateEquipmentComponent()
	{
		return NewObject<UMordecaiEquipmentComponent>();
	}

	/** Create a World + Actor + ASC with AttributeSet for GE testing. */
	struct FWeaponTestEnv
	{
		UWorld* World = nullptr;
		AActor* TestActor = nullptr;
		UAbilitySystemComponent* ASC = nullptr;
		UMordecaiAttributeSet* AttributeSet = nullptr;

		bool Setup()
		{
			if (!GEngine) return false;

			World = UWorld::CreateWorld(EWorldType::Game, false);
			if (!World) return false;

			FWorldContext& WorldContext = GEngine->CreateNewWorldContext(EWorldType::Game);
			WorldContext.SetCurrentWorld(World);

			FActorSpawnParameters SpawnParams;
			TestActor = World->SpawnActor<AActor>(AActor::StaticClass(), SpawnParams);
			if (!TestActor) { Teardown(); return false; }

			ASC = NewObject<UAbilitySystemComponent>(TestActor);
			ASC->RegisterComponent();

			AttributeSet = NewObject<UMordecaiAttributeSet>(TestActor);
			ASC->AddAttributeSetSubobject(AttributeSet);
			ASC->InitAbilityActorInfo(TestActor, TestActor);

			return true;
		}

		void Teardown()
		{
			if (World && GEngine)
			{
				GEngine->DestroyWorldContext(World);
				World->DestroyWorld(false);
			}
			World = nullptr;
			TestActor = nullptr;
			ASC = nullptr;
			AttributeSet = nullptr;
		}
	};
}

using namespace MordecaiWeaponTestHelpers;

// ===========================================================================
// 1. Mordecai.Weapon.DataAssetFieldsExist (AC-024.1)
// ===========================================================================
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_Weapon_DataAssetFieldsExist,
	"Mordecai.Weapon.DataAssetFieldsExist",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_Weapon_DataAssetFieldsExist::RunTest(const FString& Parameters)
{
	UMordecaiWeaponDataAsset* Asset = CreateSword();

	TestEqual("WeaponId set", Asset->WeaponId, FName(TEXT("Longsword_Iron")));
	TestFalse("DisplayName non-empty", Asset->DisplayName.IsEmpty());
	TestEqual("WeaponType is Longsword", Asset->WeaponType, EMordecaiWeaponType::Longsword);
	TestEqual("EquipSlot is MainHand", Asset->EquipSlot, EMordecaiEquipSlot::MainHand);
	TestEqual("Rarity is Common", Asset->Rarity, EMordecaiItemRarity::Common);
	TestTrue("BaseDamage > 0", Asset->BaseDamage > 0.f);
	TestTrue("AttackSpeedMultiplier > 0", Asset->AttackSpeedMultiplier > 0.f);
	TestTrue("Range > 0", Asset->Range > 0.f);
	TestTrue("PostureDamageBonus >= 0", Asset->PostureDamageBonus >= 0.f);
	TestEqual("3 light attack profiles", Asset->LightAttackProfiles.Num(), 3);
	TestTrue("Heavy attack profile exists", Asset->HeavyAttackProfile != nullptr);
	TestTrue("Has granted tags", Asset->GrantedTags.Num() > 0);

	return true;
}

// ===========================================================================
// 2. Mordecai.Weapon.EquipAppliesStatModifiers (AC-024.6)
// ===========================================================================
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_Weapon_EquipAppliesStatModifiers,
	"Mordecai.Weapon.EquipAppliesStatModifiers",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_Weapon_EquipAppliesStatModifiers::RunTest(const FString& Parameters)
{
	FWeaponTestEnv Env;
	if (!Env.Setup()) { AddError(TEXT("Failed to set up test env")); return false; }

	UMordecaiWeaponDataAsset* Asset = CreateSword();

	// Add a stat modifier: +5 Strength
	FMordecaiStatModifier Mod;
	Mod.Attribute = UMordecaiAttributeSet::GetStrengthAttribute();
	Mod.Operation = EMordecaiModifierOp::Add;
	Mod.Value = 5.f;
	Asset->StatModifiers.Add(Mod);

	const float OriginalStr = Env.AttributeSet->GetStrength();

	UMordecaiEquipmentComponent* EquipComp = CreateEquipmentComponent();
	EquipComp->SetASCOverride(Env.ASC);

	FMordecaiWeaponInstance Instance = MakeInstance(Asset);
	TestTrue("EquipWeapon succeeds", EquipComp->EquipWeapon(Instance, EMordecaiEquipSlot::MainHand));

	const float NewStr = Env.AttributeSet->GetStrength();
	TestTrue("Strength increased by 5", FMath::IsNearlyEqual(NewStr, OriginalStr + 5.f, 0.01f));

	Env.Teardown();
	return true;
}

// ===========================================================================
// 3. Mordecai.Weapon.EquipGrantsAbilities (AC-024.6)
// ===========================================================================
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_Weapon_EquipGrantsAbilities,
	"Mordecai.Weapon.EquipGrantsAbilities",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_Weapon_EquipGrantsAbilities::RunTest(const FString& Parameters)
{
	FWeaponTestEnv Env;
	if (!Env.Setup()) { AddError(TEXT("Failed to set up test env")); return false; }

	UMordecaiWeaponDataAsset* Asset = CreateSword();
	Asset->GrantedAbilities.Add(UGameplayAbility::StaticClass());

	UMordecaiEquipmentComponent* EquipComp = CreateEquipmentComponent();
	EquipComp->SetASCOverride(Env.ASC);

	FMordecaiWeaponInstance Instance = MakeInstance(Asset);
	EquipComp->EquipWeapon(Instance, EMordecaiEquipSlot::MainHand);

	// Check that the ability was granted
	const TArray<FGameplayAbilitySpec>& AllSpecs = Env.ASC->GetActivatableAbilities();
	TestTrue("At least 1 granted ability on ASC", AllSpecs.Num() >= 1);

	Env.Teardown();
	return true;
}

// ===========================================================================
// 4. Mordecai.Weapon.EquipGrantsTags (AC-024.6)
// ===========================================================================
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_Weapon_EquipGrantsTags,
	"Mordecai.Weapon.EquipGrantsTags",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_Weapon_EquipGrantsTags::RunTest(const FString& Parameters)
{
	FWeaponTestEnv Env;
	if (!Env.Setup()) { AddError(TEXT("Failed to set up test env")); return false; }

	UMordecaiWeaponDataAsset* Asset = CreateSword();

	UMordecaiEquipmentComponent* EquipComp = CreateEquipmentComponent();
	EquipComp->SetASCOverride(Env.ASC);

	FMordecaiWeaponInstance Instance = MakeInstance(Asset);
	EquipComp->EquipWeapon(Instance, EMordecaiEquipSlot::MainHand);

	TestTrue("Sword tag granted", Env.ASC->HasMatchingGameplayTag(MordecaiGameplayTags::Weapon_Type_Sword));

	Env.Teardown();
	return true;
}

// ===========================================================================
// 5. Mordecai.Weapon.UnequipRemovesAll (AC-024.7)
// ===========================================================================
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_Weapon_UnequipRemovesAll,
	"Mordecai.Weapon.UnequipRemovesAll",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_Weapon_UnequipRemovesAll::RunTest(const FString& Parameters)
{
	FWeaponTestEnv Env;
	if (!Env.Setup()) { AddError(TEXT("Failed to set up test env")); return false; }

	UMordecaiWeaponDataAsset* Asset = CreateSword();
	FMordecaiStatModifier Mod;
	Mod.Attribute = UMordecaiAttributeSet::GetStrengthAttribute();
	Mod.Operation = EMordecaiModifierOp::Add;
	Mod.Value = 5.f;
	Asset->StatModifiers.Add(Mod);
	Asset->GrantedAbilities.Add(UGameplayAbility::StaticClass());

	const float OriginalStr = Env.AttributeSet->GetStrength();

	UMordecaiEquipmentComponent* EquipComp = CreateEquipmentComponent();
	EquipComp->SetASCOverride(Env.ASC);

	FMordecaiWeaponInstance Instance = MakeInstance(Asset);
	EquipComp->EquipWeapon(Instance, EMordecaiEquipSlot::MainHand);

	// Verify effects applied
	TestTrue("Strength increased", Env.AttributeSet->GetStrength() > OriginalStr);
	TestTrue("Sword tag present", Env.ASC->HasMatchingGameplayTag(MordecaiGameplayTags::Weapon_Type_Sword));

	// Unequip
	TestTrue("UnequipWeapon succeeds", EquipComp->UnequipWeapon(EMordecaiEquipSlot::MainHand));

	// Verify all removed
	TestTrue("Strength restored", FMath::IsNearlyEqual(Env.AttributeSet->GetStrength(), OriginalStr, 0.01f));
	TestFalse("Sword tag removed", Env.ASC->HasMatchingGameplayTag(MordecaiGameplayTags::Weapon_Type_Sword));
	TestTrue("Slot is empty", EquipComp->GetEquippedWeapon(EMordecaiEquipSlot::MainHand) == nullptr);

	Env.Teardown();
	return true;
}

// ===========================================================================
// 6. Mordecai.Weapon.TwoHandClearsBothSlots (AC-024.8)
// ===========================================================================
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_Weapon_TwoHandClearsBothSlots,
	"Mordecai.Weapon.TwoHandClearsBothSlots",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_Weapon_TwoHandClearsBothSlots::RunTest(const FString& Parameters)
{
	FWeaponTestEnv Env;
	if (!Env.Setup()) { AddError(TEXT("Failed to set up test env")); return false; }

	UMordecaiEquipmentComponent* EquipComp = CreateEquipmentComponent();
	EquipComp->SetASCOverride(Env.ASC);

	// Equip MainHand + OffHand
	FMordecaiWeaponInstance SwordInst = MakeInstance(CreateSword());
	FMordecaiWeaponInstance DaggerInst = MakeInstance(CreateDagger());
	EquipComp->EquipWeapon(SwordInst, EMordecaiEquipSlot::MainHand);
	EquipComp->EquipWeapon(DaggerInst, EMordecaiEquipSlot::OffHand);

	TestTrue("MainHand occupied", EquipComp->GetEquippedWeapon(EMordecaiEquipSlot::MainHand) != nullptr);
	TestTrue("OffHand occupied", EquipComp->GetEquippedWeapon(EMordecaiEquipSlot::OffHand) != nullptr);

	// Equip TwoHand — should clear both
	FMordecaiWeaponInstance GreatInst = MakeInstance(CreateGreatsword());
	EquipComp->EquipWeapon(GreatInst, EMordecaiEquipSlot::TwoHand);

	TestTrue("MainHand has greatsword", EquipComp->GetEquippedWeapon(EMordecaiEquipSlot::MainHand) != nullptr);
	TestEqual("MainHand is greatsword", EquipComp->GetEquippedWeapon(EMordecaiEquipSlot::MainHand)->WeaponDataAsset->WeaponId, FName(TEXT("Greatsword_Iron")));
	TestTrue("OffHand cleared", EquipComp->GetEquippedWeapon(EMordecaiEquipSlot::OffHand) == nullptr);

	// Tags: old sword/dagger tags removed, greatsword tags present
	TestFalse("Dagger tag removed", Env.ASC->HasMatchingGameplayTag(MordecaiGameplayTags::Weapon_Type_Dagger));
	TestTrue("TwoHand tag present", Env.ASC->HasMatchingGameplayTag(MordecaiGameplayTags::Weapon_Slot_TwoHand));

	Env.Teardown();
	return true;
}

// ===========================================================================
// 7. Mordecai.Weapon.MeleeUsesEquippedProfile (AC-024.10)
// ===========================================================================
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_Weapon_MeleeUsesEquippedProfile,
	"Mordecai.Weapon.MeleeUsesEquippedProfile",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_Weapon_MeleeUsesEquippedProfile::RunTest(const FString& Parameters)
{
	UMordecaiWeaponDataAsset* SwordAsset = CreateSword();

	UMordecaiEquipmentComponent* EquipComp = CreateEquipmentComponent();
	FMordecaiWeaponInstance Instance = MakeInstance(SwordAsset);
	EquipComp->EquipWeapon(Instance, EMordecaiEquipSlot::MainHand);

	// Create melee ability and wire up equipment
	UMordecaiGA_MeleeAttack* Ability = NewObject<UMordecaiGA_MeleeAttack>();
	Ability->SetEquipmentComponentOverride(EquipComp);

	// Resolved profiles should come from the sword (3-hit chain)
	TArray<const UMordecaiAttackProfileDataAsset*> Profiles = Ability->GetResolvedAttackProfiles();
	TestEqual("3 profiles from equipped sword", Profiles.Num(), 3);

	// GetActiveProfile (combo index 0) should return the sword's first profile
	const UMordecaiAttackProfileDataAsset* Active = Ability->GetActiveProfile();
	TestTrue("Active profile not null", Active != nullptr);
	TestTrue("Active profile matches sword profile 0", Active == SwordAsset->LightAttackProfiles[0]);

	return true;
}

// ===========================================================================
// 8. Mordecai.Weapon.UnarmedFallbackWhenNoWeapon (AC-024.10)
// ===========================================================================
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_Weapon_UnarmedFallbackWhenNoWeapon,
	"Mordecai.Weapon.UnarmedFallbackWhenNoWeapon",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_Weapon_UnarmedFallbackWhenNoWeapon::RunTest(const FString& Parameters)
{
	UMordecaiEquipmentComponent* EquipComp = CreateEquipmentComponent();

	// Set up unarmed fallback profiles
	UMordecaiAttackProfileDataAsset* UnarmedProfile = CreateProfile(5.f, 150.f, 80.f, 200.f);
	EquipComp->UnarmedAttackProfiles.Add(UnarmedProfile);

	// No weapon equipped
	TestTrue("No MainHand weapon", EquipComp->GetEquippedWeapon(EMordecaiEquipSlot::MainHand) == nullptr);

	// Melee ability with equipment override
	UMordecaiGA_MeleeAttack* Ability = NewObject<UMordecaiGA_MeleeAttack>();
	Ability->SetEquipmentComponentOverride(EquipComp);

	TArray<const UMordecaiAttackProfileDataAsset*> Profiles = Ability->GetResolvedAttackProfiles();
	TestEqual("1 unarmed profile", Profiles.Num(), 1);
	TestTrue("Profile is unarmed fallback", Profiles[0] == UnarmedProfile);

	// Base damage should be 0 (no weapon)
	TestTrue("Weapon base damage is 0", FMath::IsNearlyEqual(Ability->GetWeaponBaseDamage(), 0.f));
	TestTrue("Weapon attack speed is 1.0", FMath::IsNearlyEqual(Ability->GetWeaponAttackSpeedMultiplier(), 1.f));

	return true;
}

// ===========================================================================
// 9. Mordecai.Weapon.BaseDamageAddsToAttack (AC-024.11)
// ===========================================================================
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_Weapon_BaseDamageAddsToAttack,
	"Mordecai.Weapon.BaseDamageAddsToAttack",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_Weapon_BaseDamageAddsToAttack::RunTest(const FString& Parameters)
{
	UMordecaiWeaponDataAsset* SwordAsset = CreateSword();
	// Sword BaseDamage = 15.0, first profile BasePower = 20.0

	UMordecaiEquipmentComponent* EquipComp = CreateEquipmentComponent();
	FMordecaiWeaponInstance Instance = MakeInstance(SwordAsset);
	EquipComp->EquipWeapon(Instance, EMordecaiEquipSlot::MainHand);

	UMordecaiGA_MeleeAttack* Ability = NewObject<UMordecaiGA_MeleeAttack>();
	Ability->SetEquipmentComponentOverride(EquipComp);

	// ComputeHealthDamage = -(BasePower + WeaponBaseDamage) = -(20 + 15) = -35
	float Damage = Ability->ComputeHealthDamage();
	TestTrue("Health damage includes weapon bonus", FMath::IsNearlyEqual(Damage, -35.f, 0.01f));

	// Without weapon: create ability with just the profile hardcoded
	UMordecaiGA_MeleeAttack* AbilityNoWeapon = NewObject<UMordecaiGA_MeleeAttack>();
	AbilityNoWeapon->AttackProfiles.Add(SwordAsset->LightAttackProfiles[0]);
	float DamageNoWeapon = AbilityNoWeapon->ComputeHealthDamage();
	// No equipment component → weapon base damage = 0 → damage = -20
	TestTrue("Without weapon, damage is profile-only", FMath::IsNearlyEqual(DamageNoWeapon, -20.f, 0.01f));

	return true;
}

// ===========================================================================
// 10. Mordecai.Weapon.AttackSpeedModifiesTiming (AC-024.12)
// ===========================================================================
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_Weapon_AttackSpeedModifiesTiming,
	"Mordecai.Weapon.AttackSpeedModifiesTiming",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_Weapon_AttackSpeedModifiesTiming::RunTest(const FString& Parameters)
{
	// Create a fast weapon (1.2x speed)
	UMordecaiWeaponDataAsset* FastAsset = CreateSword(FName(TEXT("Rapier_Iron")));
	FastAsset->AttackSpeedMultiplier = 1.2f;

	UMordecaiEquipmentComponent* EquipComp = CreateEquipmentComponent();
	FMordecaiWeaponInstance Instance = MakeInstance(FastAsset);
	EquipComp->EquipWeapon(Instance, EMordecaiEquipSlot::MainHand);

	UMordecaiGA_MeleeAttack* Ability = NewObject<UMordecaiGA_MeleeAttack>();
	Ability->SetEquipmentComponentOverride(EquipComp);

	// First profile: WindupTimeMs = 200, ActiveTimeMs = 100, RecoveryTimeMs = 300
	// With 1.2x speed: AdjustedTime = BaseTime / 1.2

	float WindupSec = Ability->GetPhaseDurationSeconds(EMordecaiAttackPhase::Windup);
	float ExpectedWindup = (200.f / 1.2f) / 1000.f;
	TestTrue("Windup reduced by speed mult", FMath::IsNearlyEqual(WindupSec, ExpectedWindup, 0.001f));

	float ActiveSec = Ability->GetPhaseDurationSeconds(EMordecaiAttackPhase::Active);
	float ExpectedActive = (100.f / 1.2f) / 1000.f;
	TestTrue("Active reduced by speed mult", FMath::IsNearlyEqual(ActiveSec, ExpectedActive, 0.001f));

	float RecoverySec = Ability->GetPhaseDurationSeconds(EMordecaiAttackPhase::Recovery);
	float ExpectedRecovery = (300.f / 1.2f) / 1000.f;
	TestTrue("Recovery reduced by speed mult", FMath::IsNearlyEqual(RecoverySec, ExpectedRecovery, 0.001f));

	return true;
}

// ===========================================================================
// 11. Mordecai.Weapon.GetEquippedWeaponReturnsCorrect (AC-024.9)
// ===========================================================================
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_Weapon_GetEquippedWeaponReturnsCorrect,
	"Mordecai.Weapon.GetEquippedWeaponReturnsCorrect",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_Weapon_GetEquippedWeaponReturnsCorrect::RunTest(const FString& Parameters)
{
	UMordecaiEquipmentComponent* EquipComp = CreateEquipmentComponent();

	// Initially empty
	TestTrue("MainHand empty", EquipComp->GetEquippedWeapon(EMordecaiEquipSlot::MainHand) == nullptr);
	TestTrue("OffHand empty", EquipComp->GetEquippedWeapon(EMordecaiEquipSlot::OffHand) == nullptr);

	// Equip sword in MainHand
	FMordecaiWeaponInstance SwordInst = MakeInstance(CreateSword());
	EquipComp->EquipWeapon(SwordInst, EMordecaiEquipSlot::MainHand);

	const FMordecaiWeaponInstance* MainHand = EquipComp->GetEquippedWeapon(EMordecaiEquipSlot::MainHand);
	TestTrue("MainHand has weapon", MainHand != nullptr);
	TestEqual("MainHand is the sword", MainHand->WeaponDataAsset->WeaponId, FName(TEXT("Longsword_Iron")));

	// Equip dagger in OffHand
	FMordecaiWeaponInstance DaggerInst = MakeInstance(CreateDagger());
	EquipComp->EquipWeapon(DaggerInst, EMordecaiEquipSlot::OffHand);

	const FMordecaiWeaponInstance* OffHand = EquipComp->GetEquippedWeapon(EMordecaiEquipSlot::OffHand);
	TestTrue("OffHand has weapon", OffHand != nullptr);
	TestEqual("OffHand is the dagger", OffHand->WeaponDataAsset->WeaponId, FName(TEXT("Dagger_Iron")));

	// MainHand still the sword
	TestEqual("MainHand unchanged", EquipComp->GetEquippedWeapon(EMordecaiEquipSlot::MainHand)->WeaponDataAsset->WeaponId, FName(TEXT("Longsword_Iron")));

	return true;
}
