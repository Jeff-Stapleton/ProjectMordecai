// Project Mordecai — Melee grant tests (issue #40)
// The melee ability receives the granting controller's MeleeAttackProfiles through the granted spec's
// SourceObject, never by writing them into the ability CDO: native CDOs are in the packaged build's
// disregard-for-GC pool, and a CDO pointing at a loaded data asset is a fatal GC verification error.

#include "Misc/AutomationTest.h"
#include "Misc/ScopeExit.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "GameplayAbilitySpec.h"
#include "Mordecai/Combat/MordecaiAttackProfileDataAsset.h"
#include "Mordecai/Combat/MordecaiGA_Block.h"
#include "Mordecai/Combat/MordecaiGA_Dodge.h"
#include "Mordecai/Combat/MordecaiGA_HeavyAttack.h"
#include "Mordecai/Combat/MordecaiGA_MeleeAttack.h"
#include "Mordecai/Combat/MordecaiGA_Parry.h"
#include "Mordecai/Enemy/MordecaiEnemyCharacter.h"
#include "Mordecai/MordecaiCharacter.h"
#include "Mordecai/MordecaiGameplayTags.h"
#include "Mordecai/MordecaiPlayerController.h"
#include "Mordecai/MordecaiPlayerState.h"
#include "Mordecai/Weapons/MordecaiEquipmentComponent.h"
#include "Mordecai/Weapons/MordecaiWeaponDataAsset.h"
#include "Mordecai/Weapons/MordecaiWeaponProfileFactory.h"
#include "Tests/Combat/MordecaiCombatGrantTestEnv.h"

namespace MordecaiMeleeGrantTestHelpers
{
	using MordecaiCombatGrantTest::FMordecaiCombatGrantTestEnv;

	/** A controller that grants melee with these profiles. A SourceObject needs no world. */
	static AMordecaiPlayerController* MakeGranter(const TArray<UMordecaiAttackProfileDataAsset*>& Profiles)
	{
		AMordecaiPlayerController* Granter = NewObject<AMordecaiPlayerController>();
		for (UMordecaiAttackProfileDataAsset* Profile : Profiles)
		{
			Granter->MeleeAttackProfiles.Add(Profile);
		}
		return Granter;
	}

	/** An unequipped weapon instance of this data asset, as the equipment component stores them. */
	static FMordecaiWeaponInstance MakeGrantTestWeapon(UMordecaiWeaponDataAsset* WeaponData)
	{
		FMordecaiWeaponInstance Weapon;
		Weapon.InstanceId = FGuid::NewGuid();
		Weapon.WeaponDataAsset = WeaponData;
		return Weapon;
	}
}

// Mordecai.Combat.MeleeGrant.InstanceTakesProfilesFromSpec
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_Combat_MeleeGrantInstanceTakesProfilesFromSpec,
	"Mordecai.Combat.MeleeGrant.InstanceTakesProfilesFromSpec",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_Combat_MeleeGrantInstanceTakesProfilesFromSpec::RunTest(const FString& Parameters)
{
	using namespace MordecaiMeleeGrantTestHelpers;

	UMordecaiAttackProfileDataAsset* P1 = NewObject<UMordecaiAttackProfileDataAsset>();
	UMordecaiAttackProfileDataAsset* P2 = NewObject<UMordecaiAttackProfileDataAsset>();
	UMordecaiGA_MeleeAttack* Instance = NewObject<UMordecaiGA_MeleeAttack>();
	const FGameplayAbilitySpec Spec(UMordecaiGA_MeleeAttack::StaticClass(), 1, INDEX_NONE, MakeGranter({ P1, P2 }));

	Instance->TakeAttackProfilesFromSpec(Spec);
	TestTrue("Instance uses the profiles of the controller that granted its spec",
		Instance->AttackProfiles.Num() == 2 && Instance->AttackProfiles[0] == P1 && Instance->AttackProfiles[1] == P2);
	TestTrue("The first combo step uses the first granted profile", Instance->GetActiveProfile() == P1);
	return true;
}

// Mordecai.Combat.MeleeGrant.ExistingProfilesKept
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_Combat_MeleeGrantExistingProfilesKept,
	"Mordecai.Combat.MeleeGrant.ExistingProfilesKept",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_Combat_MeleeGrantExistingProfilesKept::RunTest(const FString& Parameters)
{
	using namespace MordecaiMeleeGrantTestHelpers;

	UMordecaiAttackProfileDataAsset* P0 = NewObject<UMordecaiAttackProfileDataAsset>();
	UMordecaiAttackProfileDataAsset* P1 = NewObject<UMordecaiAttackProfileDataAsset>();
	UMordecaiAttackProfileDataAsset* P2 = NewObject<UMordecaiAttackProfileDataAsset>();

	// Profiles set on the ability class (e.g. a Blueprint subclass) win over the controller's
	UMordecaiGA_MeleeAttack* Configured = NewObject<UMordecaiGA_MeleeAttack>();
	Configured->AttackProfiles.Add(P0);
	Configured->TakeAttackProfilesFromSpec(FGameplayAbilitySpec(UMordecaiGA_MeleeAttack::StaticClass(), 1, INDEX_NONE, MakeGranter({ P1, P2 })));
	TestTrue("The ability's own profiles are kept", Configured->AttackProfiles.Num() == 1 && Configured->AttackProfiles[0] == P0);

	// The controller never clears them
	Configured->TakeAttackProfilesFromSpec(FGameplayAbilitySpec(UMordecaiGA_MeleeAttack::StaticClass(), 1, INDEX_NONE, MakeGranter({})));
	TestTrue("A controller without profiles leaves them", Configured->AttackProfiles.Num() == 1 && Configured->AttackProfiles[0] == P0);
	return true;
}

// Mordecai.Combat.MeleeGrant.NonControllerSourceIgnored
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_Combat_MeleeGrantNonControllerSourceIgnored,
	"Mordecai.Combat.MeleeGrant.NonControllerSourceIgnored",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_Combat_MeleeGrantNonControllerSourceIgnored::RunTest(const FString& Parameters)
{
	// Enemies grant with themselves as the SourceObject and set their profiles on the instance after granting
	AMordecaiEnemyCharacter* Enemy = NewObject<AMordecaiEnemyCharacter>();
	Enemy->EnemyAttackProfiles.Add(NewObject<UMordecaiAttackProfileDataAsset>());
	UMordecaiGA_MeleeAttack* EnemyInstance = NewObject<UMordecaiGA_MeleeAttack>();
	EnemyInstance->TakeAttackProfilesFromSpec(FGameplayAbilitySpec(UMordecaiGA_MeleeAttack::StaticClass(), 1, INDEX_NONE, Enemy));
	TestEqual("An enemy SourceObject gives no profiles", EnemyInstance->AttackProfiles.Num(), 0);

	// A grant without a SourceObject (or one not resolved yet on a client)
	UMordecaiGA_MeleeAttack* Unsourced = NewObject<UMordecaiGA_MeleeAttack>();
	Unsourced->TakeAttackProfilesFromSpec(FGameplayAbilitySpec(UMordecaiGA_MeleeAttack::StaticClass(), 1, INDEX_NONE, nullptr));
	TestEqual("No SourceObject gives no profiles", Unsourced->AttackProfiles.Num(), 0);
	return true;
}

// Mordecai.Combat.MeleeGrant.NeverWritesTheCDO
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_Combat_MeleeGrantNeverWritesTheCDO,
	"Mordecai.Combat.MeleeGrant.NeverWritesTheCDO",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_Combat_MeleeGrantNeverWritesTheCDO::RunTest(const FString& Parameters)
{
	using namespace MordecaiMeleeGrantTestHelpers;

	UMordecaiGA_MeleeAttack* CDO = GetMutableDefault<UMordecaiGA_MeleeAttack>();
	const TArray<TObjectPtr<UMordecaiAttackProfileDataAsset>> Before = CDO->AttackProfiles;
	ON_SCOPE_EXIT { CDO->AttackProfiles = Before; };
	if (!TestEqual("CDO starts without AttackProfiles", Before.Num(), 0))
	{
		return true;
	}

	// GAS calls OnGiveAbility on the CDO when there is no instance; that must not stick profiles on it
	CDO->TakeAttackProfilesFromSpec(FGameplayAbilitySpec(UMordecaiGA_MeleeAttack::StaticClass(), 1, INDEX_NONE,
		MakeGranter({ NewObject<UMordecaiAttackProfileDataAsset>() })));
	TestEqual("CDO AttackProfiles unchanged", CDO->AttackProfiles.Num(), 0);
	return true;
}

// Mordecai.Combat.MeleeGrant.GrantedInstanceTakesControllerProfiles
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_Combat_MeleeGrantGrantedInstanceTakesControllerProfiles,
	"Mordecai.Combat.MeleeGrant.GrantedInstanceTakesControllerProfiles",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_Combat_MeleeGrantGrantedInstanceTakesControllerProfiles::RunTest(const FString& Parameters)
{
	using namespace MordecaiMeleeGrantTestHelpers;

	// Restore the CDO even if a regression writes it again, so this test cannot leak into the other melee tests
	UMordecaiGA_MeleeAttack* CDO = GetMutableDefault<UMordecaiGA_MeleeAttack>();
	const TArray<TObjectPtr<UMordecaiAttackProfileDataAsset>> CDOBefore = CDO->AttackProfiles;
	ON_SCOPE_EXIT { CDO->AttackProfiles = CDOBefore; };

	FMordecaiCombatGrantTestEnv Env;
	ON_SCOPE_EXIT { Env.Teardown(); };
	if (!TestTrue("Test world set up", Env.Setup()))
	{
		return true;
	}

	// The real grant path: GiveAbility creates the instance, then calls its OnGiveAbility with the spec
	UMordecaiAttackProfileDataAsset* P1 = NewObject<UMordecaiAttackProfileDataAsset>();
	UMordecaiAttackProfileDataAsset* P2 = NewObject<UMordecaiAttackProfileDataAsset>();
	const FGameplayAbilitySpec* Spec = Env.GrantMeleeOnly({ P1, P2 });
	if (!TestNotNull("Melee ability granted", Spec))
	{
		return true;
	}

	const UMordecaiGA_MeleeAttack* Instance = Cast<UMordecaiGA_MeleeAttack>(Spec->GetPrimaryInstance());
	if (!TestNotNull("Melee ability is instanced", Instance))
	{
		return true;
	}
	TestFalse("Instance is not the CDO", Instance->HasAnyFlags(RF_ClassDefaultObject));
	TestTrue("Instance has the controller's profiles",
		Instance->AttackProfiles.Num() == 2 && Instance->AttackProfiles[0] == P1 && Instance->AttackProfiles[1] == P2);
	return true;
}

// Mordecai.Recovery.Combat.NoMeleeCDOWrite (US-083 AC-083.2)
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_Recovery_CombatNoMeleeCDOWrite,
	"Mordecai.Recovery.Combat.NoMeleeCDOWrite",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_Recovery_CombatNoMeleeCDOWrite::RunTest(const FString& Parameters)
{
	using namespace MordecaiMeleeGrantTestHelpers;

	// Restore the CDO whatever the grant does, so a failure here cannot leak into the other melee tests
	UMordecaiGA_MeleeAttack* CDO = GetMutableDefault<UMordecaiGA_MeleeAttack>();
	const TArray<TObjectPtr<UMordecaiAttackProfileDataAsset>> CDOBefore = CDO->AttackProfiles;
	ON_SCOPE_EXIT { CDO->AttackProfiles = CDOBefore; };
	if (!TestEqual("CDO starts without AttackProfiles", CDOBefore.Num(), 0))
	{
		return true;
	}

	FMordecaiCombatGrantTestEnv Env;
	ON_SCOPE_EXIT { Env.Teardown(); };
	if (!TestTrue("Test world set up", Env.Setup()))
	{
		return true;
	}

	const FGameplayAbilitySpec* Spec = Env.GrantMeleeOnly({ NewObject<UMordecaiAttackProfileDataAsset>(), NewObject<UMordecaiAttackProfileDataAsset>() });

	TestEqual("CDO AttackProfiles still empty after granting", GetDefault<UMordecaiGA_MeleeAttack>()->AttackProfiles.Num(), 0);
	if (!TestNotNull("Melee ability granted", Spec))
	{
		return true;
	}
	TestTrue("The controller is the grant's SourceObject", Spec->SourceObject.Get() == Env.PC);
	return true;
}

// Mordecai.Recovery.Combat.AbilitiesGrantedFromNativeDefaults (US-083 AC-083.1)
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_Recovery_CombatAbilitiesGrantedFromNativeDefaults,
	"Mordecai.Recovery.Combat.AbilitiesGrantedFromNativeDefaults",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_Recovery_CombatAbilitiesGrantedFromNativeDefaults::RunTest(const FString& Parameters)
{
	using namespace MordecaiMeleeGrantTestHelpers;

	// Restore the CDOs whatever the grant does, so a failure here cannot leak into the other melee tests
	UMordecaiGA_MeleeAttack* MeleeCDO = GetMutableDefault<UMordecaiGA_MeleeAttack>();
	UMordecaiGA_HeavyAttack* HeavyCDO = GetMutableDefault<UMordecaiGA_HeavyAttack>();
	const TArray<TObjectPtr<UMordecaiAttackProfileDataAsset>> MeleeCDOBefore = MeleeCDO->AttackProfiles;
	const TArray<TObjectPtr<UMordecaiAttackProfileDataAsset>> HeavyCDOBefore = HeavyCDO->AttackProfiles;
	ON_SCOPE_EXIT
	{
		MeleeCDO->AttackProfiles = MeleeCDOBefore;
		HeavyCDO->AttackProfiles = HeavyCDOBefore;
	};

	FMordecaiCombatGrantTestEnv Env;
	ON_SCOPE_EXIT { Env.Teardown(); };
	if (!TestTrue("Test world set up", Env.Setup()))
	{
		return true;
	}

	struct FExpectedGrant
	{
		UClass* AbilityClass;
		FGameplayTag InputTag;
	};
	const FExpectedGrant Expected[] = {
		{ UMordecaiGA_MeleeAttack::StaticClass(), MordecaiGameplayTags::Input_LightAttack },
		{ UMordecaiGA_HeavyAttack::StaticClass(), MordecaiGameplayTags::Input_HeavyAttack },
		{ UMordecaiGA_Dodge::StaticClass(), MordecaiGameplayTags::Input_Dodge },
		{ UMordecaiGA_Block::StaticClass(), MordecaiGameplayTags::Input_Block },
		{ UMordecaiGA_Parry::StaticClass(), MordecaiGameplayTags::Input_Parry },
	};

	// A fresh controller holds only what its C++ constructor set: an instance of a native class copies no
	// script-edited CDO values, and headless runs never ran Scripts/us054_10_configure_all.py anyway
	for (const FExpectedGrant& Grant : Expected)
	{
		TestTrue(*FString::Printf(TEXT("DefaultCombatAbilities holds %s"), *Grant.AbilityClass->GetName()),
			Env.PC->DefaultCombatAbilities.Contains(Grant.AbilityClass));
	}

	Env.GrantCombatOnly();
	if (!TestNotNull("Player state has an ASC", Env.GetASC()))
	{
		return true;
	}

	for (const FExpectedGrant& Grant : Expected)
	{
		const FString Name = Grant.AbilityClass->GetName();
		const FGameplayAbilitySpec* Spec = Env.FindSpec(Grant.AbilityClass);
		if (!TestNotNull(*FString::Printf(TEXT("ASC holds a %s spec"), *Name), Spec))
		{
			continue;
		}
		TestTrue(*FString::Printf(TEXT("%s: the controller is the grant's SourceObject"), *Name), Spec->SourceObject.Get() == Env.PC);
		TestTrue(*FString::Printf(TEXT("%s: spec carries %s"), *Name, *Grant.InputTag.ToString()),
			Spec->GetDynamicSpecSourceTags().HasTagExact(Grant.InputTag));
		TestEqual(*FString::Printf(TEXT("%s: spec carries one input tag"), *Name), Spec->GetDynamicSpecSourceTags().Num(), 1);
	}

	TestEqual("Melee CDO AttackProfiles still empty", GetDefault<UMordecaiGA_MeleeAttack>()->AttackProfiles.Num(), 0);
	TestEqual("Heavy CDO AttackProfiles still empty", GetDefault<UMordecaiGA_HeavyAttack>()->AttackProfiles.Num(), 0);
	return true;
}

// Mordecai.Recovery.Combat.MeleeUsesEquippedWeaponProfiles (US-083 AC-083.2)
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_Recovery_CombatMeleeUsesEquippedWeaponProfiles,
	"Mordecai.Recovery.Combat.MeleeUsesEquippedWeaponProfiles",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_Recovery_CombatMeleeUsesEquippedWeaponProfiles::RunTest(const FString& Parameters)
{
	using namespace MordecaiMeleeGrantTestHelpers;

	UMordecaiWeaponDataAsset* Longsword = UMordecaiWeaponProfileFactory::CreateLongsword(GetTransientPackage());
	if (!TestNotNull("Factory longsword", Longsword)
		|| !TestTrue("Longsword has light profiles", Longsword->LightAttackProfiles.Num() > 0)
		|| !TestNotNull("Longsword has a heavy profile", Longsword->HeavyAttackProfile.Get()))
	{
		return true;
	}
	const UMordecaiAttackProfileDataAsset* LongswordL1 = Longsword->LightAttackProfiles[0];
	const UMordecaiAttackProfileDataAsset* LongswordH = Longsword->HeavyAttackProfile;

	// Equipped weapon: light reads its light combo, heavy its heavy attack
	{
		UMordecaiEquipmentComponent* Equipment = NewObject<UMordecaiEquipmentComponent>();
		FMordecaiWeaponInstance Weapon = MakeGrantTestWeapon(Longsword);
		TestTrue("Longsword equipped", Equipment->EquipWeapon(Weapon, EMordecaiEquipSlot::MainHand));

		UMordecaiGA_MeleeAttack* Light = NewObject<UMordecaiGA_MeleeAttack>();
		Light->SetEquipmentComponentOverride(Equipment);
		UMordecaiGA_HeavyAttack* Heavy = NewObject<UMordecaiGA_HeavyAttack>();
		Heavy->SetEquipmentComponentOverride(Equipment);

		TestTrue("Light attack uses the weapon's first light profile", Light->GetActiveProfile() == LongswordL1);
		TestTrue("Heavy attack uses the weapon's heavy profile", Heavy->GetActiveProfile() == LongswordH);
		TestEqual("Heavy attack is a single step", Heavy->GetResolvedAttackProfiles().Num(), 1);
		TestTrue("Heavy attack owns Mordecai.Attack.Heavy", Heavy->HasActivationOwnedTag(MordecaiGameplayTags::Attack_Heavy));
		TestFalse("Light attack does not own Mordecai.Attack.Heavy", Light->HasActivationOwnedTag(MordecaiGameplayTags::Attack_Heavy));
	}

	// Nothing equipped: light falls back to the unarmed profiles; there is no unarmed heavy
	{
		UMordecaiEquipmentComponent* Bare = NewObject<UMordecaiEquipmentComponent>();
		UMordecaiAttackProfileDataAsset* Punch = NewObject<UMordecaiAttackProfileDataAsset>();
		Bare->UnarmedAttackProfiles.Add(Punch);

		UMordecaiGA_MeleeAttack* Light = NewObject<UMordecaiGA_MeleeAttack>();
		Light->SetEquipmentComponentOverride(Bare);
		UMordecaiGA_HeavyAttack* Heavy = NewObject<UMordecaiGA_HeavyAttack>();
		Heavy->SetEquipmentComponentOverride(Bare);

		TestTrue("Unarmed light attack uses the unarmed profile", Light->GetActiveProfile() == Punch);
		TestNull("Unarmed heavy attack has no profile", Heavy->GetActiveProfile());
	}

	// The real grant path: the granted instances read the avatar's equipped weapon
	FMordecaiCombatGrantTestEnv Env;
	ON_SCOPE_EXIT { Env.Teardown(); };
	if (!TestTrue("Test world set up", Env.Setup()))
	{
		return true;
	}
	UMordecaiAttackProfileDataAsset* ControllerProfile = NewObject<UMordecaiAttackProfileDataAsset>();
	Env.PC->MeleeAttackProfiles.Add(ControllerProfile);
	Env.GrantCombatOnly();
	AMordecaiCharacter* Avatar = Env.AttachAvatar();
	UMordecaiEquipmentComponent* AvatarEquipment = Avatar ? Avatar->GetEquipmentComponent() : nullptr;
	if (!TestNotNull("Avatar has an equipment component", AvatarEquipment))
	{
		return true;
	}
	FMordecaiWeaponInstance AvatarWeapon = MakeGrantTestWeapon(Longsword);
	AvatarEquipment->EquipWeapon(AvatarWeapon, EMordecaiEquipSlot::MainHand);

	const FGameplayAbilitySpec* LightSpec = Env.FindSpec(UMordecaiGA_MeleeAttack::StaticClass());
	const FGameplayAbilitySpec* HeavySpec = Env.FindSpec(UMordecaiGA_HeavyAttack::StaticClass());
	const UMordecaiGA_MeleeAttack* LightInstance = LightSpec ? Cast<UMordecaiGA_MeleeAttack>(LightSpec->GetPrimaryInstance()) : nullptr;
	const UMordecaiGA_HeavyAttack* HeavyInstance = HeavySpec ? Cast<UMordecaiGA_HeavyAttack>(HeavySpec->GetPrimaryInstance()) : nullptr;
	if (!TestNotNull("Granted light instance", LightInstance) || !TestNotNull("Granted heavy instance", HeavyInstance))
	{
		return true;
	}
	TestTrue("Granted light attack uses the equipped weapon", LightInstance->GetActiveProfile() == LongswordL1);
	TestTrue("Granted heavy attack uses the equipped weapon's heavy profile", HeavyInstance->GetActiveProfile() == LongswordH);

	// The controller's MeleeAttackProfiles are the light combo's fallback only
	TestTrue("Light instance took the controller's profiles as its fallback",
		LightInstance->AttackProfiles.Num() == 1 && LightInstance->AttackProfiles[0] == ControllerProfile);
	TestEqual("Heavy instance did not take the controller's light profiles", HeavyInstance->AttackProfiles.Num(), 0);
	return true;
}
