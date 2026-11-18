// Project Mordecai — Combat abilities driven by their input tags (US-083)
// The player's combat abilities activate through Lyra's input path: a binding presses a Mordecai.Input.* tag on the
// ASC, ProcessAbilityInput activates the spec carrying it, and a release reaches the active instance's InputReleased.
// These tests drive that path on the real grant (FMordecaiCombatGrantTestEnv) with a pawn avatar attached.

#include "Misc/AutomationTest.h"
#include "Misc/ScopeExit.h"
#include "AbilitySystem/LyraAbilitySystemComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameplayAbilitySpec.h"
#include "Mordecai/AbilitySystem/MordecaiAttributeSet.h"
#include "Mordecai/Combat/MordecaiCombatTypes.h"
#include "Mordecai/Combat/MordecaiGA_Block.h"
#include "Mordecai/Combat/MordecaiGA_Dodge.h"
#include "Mordecai/Combat/MordecaiGA_HeavyAttack.h"
#include "Mordecai/Combat/MordecaiGA_MeleeAttack.h"
#include "Mordecai/Combat/MordecaiGA_Parry.h"
#include "Mordecai/MordecaiCharacter.h"
#include "Mordecai/MordecaiGameplayTags.h"
#include "Mordecai/Weapons/MordecaiEquipmentComponent.h"
#include "Mordecai/Weapons/MordecaiWeaponDataAsset.h"
#include "Mordecai/Weapons/MordecaiWeaponProfileFactory.h"
#include "Tests/Combat/MordecaiCombatGrantTestEnv.h"

namespace MordecaiCombatInputAbilityTestHelpers
{
	using MordecaiCombatGrantTest::FMordecaiCombatGrantTestEnv;

	/** Grant the native combat abilities and attach an avatar holding a factory longsword (so attacks have profiles). */
	static bool SetupArmedPlayer(FAutomationTestBase& Test, FMordecaiCombatGrantTestEnv& Env)
	{
		if (!Test.TestTrue(TEXT("Test world set up"), Env.Setup()))
		{
			return false;
		}
		Env.GrantCombatOnly();
		AMordecaiCharacter* Avatar = Env.AttachAvatar();
		if (!Test.TestNotNull(TEXT("Avatar"), Avatar) || !Test.TestNotNull(TEXT("ASC"), Env.GetASC()))
		{
			return false;
		}
		FMordecaiWeaponInstance Weapon;
		Weapon.InstanceId = FGuid::NewGuid();
		Weapon.WeaponDataAsset = UMordecaiWeaponProfileFactory::CreateLongsword(GetTransientPackage());
		return Test.TestTrue(TEXT("Longsword equipped"), Avatar->GetEquipmentComponent()->EquipWeapon(Weapon, EMordecaiEquipSlot::MainHand));
	}

	static bool IsAbilityActive(const FMordecaiCombatGrantTestEnv& Env, TSubclassOf<UGameplayAbility> AbilityClass)
	{
		const FGameplayAbilitySpec* Spec = Env.FindSpec(AbilityClass);
		return Spec && Spec->IsActive();
	}
}

// ---------------------------------------------------------------------------
// Mordecai.Recovery.Combat.BlockEndsOnReleaseOnlyWhileActive
// Block is held: releasing its input ends it, but never cuts a guard-break stagger short.
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_Recovery_CombatBlockEndsOnRelease,
	"Mordecai.Recovery.Combat.BlockEndsOnReleaseOnlyWhileActive",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_Recovery_CombatBlockEndsOnRelease::RunTest(const FString& Parameters)
{
	using namespace MordecaiCombatInputAbilityTestHelpers;

	FMordecaiCombatGrantTestEnv Env;
	ON_SCOPE_EXIT { Env.Teardown(); };
	if (!SetupArmedPlayer(*this, Env))
	{
		return true;
	}
	ULyraAbilitySystemComponent* ASC = Env.GetASC();

	Env.PressInput(MordecaiGameplayTags::Input_Block);
	TestTrue("Block input activates block", IsAbilityActive(Env, UMordecaiGA_Block::StaticClass()));
	TestTrue("Blocking tag while held", ASC->HasMatchingGameplayTag(MordecaiGameplayTags::State_Blocking));

	Env.ReleaseInput(MordecaiGameplayTags::Input_Block);
	TestFalse("Releasing the block input ends block", IsAbilityActive(Env, UMordecaiGA_Block::StaticClass()));
	TestFalse("No Blocking tag after release", ASC->HasMatchingGameplayTag(MordecaiGameplayTags::State_Blocking));

	// Guard break: the stagger runs its course even if the player lets go
	Env.PressInput(MordecaiGameplayTags::Input_Block);
	const FGameplayAbilitySpec* BlockSpec = Env.FindSpec(UMordecaiGA_Block::StaticClass());
	UMordecaiGA_Block* Block = BlockSpec ? Cast<UMordecaiGA_Block>(BlockSpec->GetPrimaryInstance()) : nullptr;
	if (!TestNotNull("Block instance", Block) || !TestTrue("Block active again", BlockSpec->IsActive()))
	{
		return true;
	}
	ASC->SetNumericAttributeBase(UMordecaiAttributeSet::GetStaminaAttribute(), 1.f);
	// A hit after the perfect-block window, so it is an ordinary blocked hit that drains stamina
	Block->ReceiveBlockedHit(100.f, Block->GetBlockActivationTime() + Block->GetPerfectBlockWindowSeconds() + 0.1);
	if (!TestEqual("A hit at 1 stamina breaks the guard", Block->GetCurrentBlockPhase(), EMordecaiBlockPhase::GuardBroken))
	{
		return true;
	}
	Env.ReleaseInput(MordecaiGameplayTags::Input_Block);
	TestTrue("Releasing during guard break keeps the ability running", BlockSpec->IsActive());
	TestTrue("GuardBroken stagger still on", ASC->HasMatchingGameplayTag(MordecaiGameplayTags::State_GuardBroken));
	return true;
}

// ---------------------------------------------------------------------------
// Mordecai.Recovery.Combat.ParryCancelsActiveBlock (AC-006.17, AC-006.18 through real input)
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_Recovery_CombatParryCancelsBlock,
	"Mordecai.Recovery.Combat.ParryCancelsActiveBlock",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_Recovery_CombatParryCancelsBlock::RunTest(const FString& Parameters)
{
	using namespace MordecaiCombatInputAbilityTestHelpers;

	FMordecaiCombatGrantTestEnv Env;
	ON_SCOPE_EXIT { Env.Teardown(); };
	if (!SetupArmedPlayer(*this, Env))
	{
		return true;
	}
	ULyraAbilitySystemComponent* ASC = Env.GetASC();

	Env.PressInput(MordecaiGameplayTags::Input_Block);
	if (!TestTrue("Blocking", IsAbilityActive(Env, UMordecaiGA_Block::StaticClass())))
	{
		return true;
	}

	Env.PressInput(MordecaiGameplayTags::Input_Parry);
	TestTrue("Parry activates while blocking", IsAbilityActive(Env, UMordecaiGA_Parry::StaticClass()));
	TestTrue("Parrying tag", ASC->HasMatchingGameplayTag(MordecaiGameplayTags::State_Parrying));
	TestFalse("Parry dropped the block", IsAbilityActive(Env, UMordecaiGA_Block::StaticClass()));
	TestFalse("No Blocking tag during the parry", ASC->HasMatchingGameplayTag(MordecaiGameplayTags::State_Blocking));

	// The held block input re-presses every frame; it must not bring the guard back mid-parry
	Env.PressInput(MordecaiGameplayTags::Input_Block);
	TestFalse("Block cannot activate while parrying", IsAbilityActive(Env, UMordecaiGA_Block::StaticClass()));
	return true;
}

// ---------------------------------------------------------------------------
// Mordecai.Recovery.Combat.CombatInputBlockedWhileParryingOrDead
// Attacks cannot fire during a parry or its whiff lock, dodges only not during the whiff lock (AC-006.14), and a dead
// player does nothing.
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_Recovery_CombatInputBlockedWhileParryingOrDead,
	"Mordecai.Recovery.Combat.CombatInputBlockedWhileParryingOrDead",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_Recovery_CombatInputBlockedWhileParryingOrDead::RunTest(const FString& Parameters)
{
	using namespace MordecaiCombatInputAbilityTestHelpers;

	// The class defaults
	const UMordecaiGA_MeleeAttack* Melee = GetDefault<UMordecaiGA_MeleeAttack>();
	const UMordecaiGA_HeavyAttack* Heavy = GetDefault<UMordecaiGA_HeavyAttack>();
	const UMordecaiGA_Dodge* Dodge = GetDefault<UMordecaiGA_Dodge>();
	const UMordecaiGA_Block* Block = GetDefault<UMordecaiGA_Block>();
	const UMordecaiGA_Parry* Parry = GetDefault<UMordecaiGA_Parry>();
	TestTrue("Melee blocked while parrying", Melee->HasActivationBlockedTag(MordecaiGameplayTags::State_Parrying));
	TestTrue("Melee blocked during parry whiff", Melee->HasActivationBlockedTag(MordecaiGameplayTags::State_ParryWhiff));
	TestTrue("Melee blocked when dead", Melee->HasActivationBlockedTag(MordecaiGameplayTags::State_Dead));
	TestTrue("Heavy blocked while parrying", Heavy->HasActivationBlockedTag(MordecaiGameplayTags::State_Parrying));
	TestTrue("Heavy blocked when dead", Heavy->HasActivationBlockedTag(MordecaiGameplayTags::State_Dead));
	TestTrue("Dodge blocked during parry whiff", Dodge->HasActivationBlockedTag(MordecaiGameplayTags::State_ParryWhiff));
	TestTrue("Dodge blocked when dead", Dodge->HasActivationBlockedTag(MordecaiGameplayTags::State_Dead));
	TestTrue("Block blocked while parrying", Block->HasActivationBlockedTag(MordecaiGameplayTags::State_Parrying));
	TestTrue("Block blocked when dead", Block->HasActivationBlockedTag(MordecaiGameplayTags::State_Dead));
	TestTrue("Parry blocked when dead", Parry->HasActivationBlockedTag(MordecaiGameplayTags::State_Dead));

	// Through real input on the granted abilities
	FMordecaiCombatGrantTestEnv Env;
	ON_SCOPE_EXIT { Env.Teardown(); };
	if (!SetupArmedPlayer(*this, Env))
	{
		return true;
	}
	ULyraAbilitySystemComponent* ASC = Env.GetASC();

	ASC->AddLooseGameplayTag(MordecaiGameplayTags::State_Parrying);
	Env.PressInput(MordecaiGameplayTags::Input_LightAttack);
	TestFalse("No light attack mid-parry", IsAbilityActive(Env, UMordecaiGA_MeleeAttack::StaticClass()));
	Env.ReleaseInput(MordecaiGameplayTags::Input_LightAttack);
	ASC->RemoveLooseGameplayTag(MordecaiGameplayTags::State_Parrying);

	ASC->AddLooseGameplayTag(MordecaiGameplayTags::State_Dead);
	Env.PressInput(MordecaiGameplayTags::Input_LightAttack);
	Env.PressInput(MordecaiGameplayTags::Input_Dodge);
	Env.PressInput(MordecaiGameplayTags::Input_Block);
	TestFalse("Dead: no light attack", IsAbilityActive(Env, UMordecaiGA_MeleeAttack::StaticClass()));
	TestFalse("Dead: no dodge", IsAbilityActive(Env, UMordecaiGA_Dodge::StaticClass()));
	TestFalse("Dead: no block", IsAbilityActive(Env, UMordecaiGA_Block::StaticClass()));
	Env.ReleaseInput(MordecaiGameplayTags::Input_LightAttack);
	Env.ReleaseInput(MordecaiGameplayTags::Input_Dodge);
	Env.ReleaseInput(MordecaiGameplayTags::Input_Block);
	ASC->RemoveLooseGameplayTag(MordecaiGameplayTags::State_Dead);

	// Controls: the same presses act once nothing blocks them (nothing ticks, so each ability stays active)
	Env.PressInput(MordecaiGameplayTags::Input_LightAttack);
	TestTrue("Light attack activates from its input tag", IsAbilityActive(Env, UMordecaiGA_MeleeAttack::StaticClass()));
	TestTrue("Attacking tag", ASC->HasMatchingGameplayTag(MordecaiGameplayTags::State_Attacking));
	Env.PressInput(MordecaiGameplayTags::Input_Dodge);
	TestTrue("Dodge activates from its input tag", IsAbilityActive(Env, UMordecaiGA_Dodge::StaticClass()));
	TestTrue("Dodging tag", ASC->HasMatchingGameplayTag(MordecaiGameplayTags::State_Dodging));
	Env.PressInput(MordecaiGameplayTags::Input_Block);
	TestTrue("Block activates from its input tag", IsAbilityActive(Env, UMordecaiGA_Block::StaticClass()));
	return true;
}

// ---------------------------------------------------------------------------
// Mordecai.Recovery.Combat.HeavyInputStartsHeavyAttack
// The heavy input tag starts the heavy attack (its own spec), which owns Mordecai.Attack.Heavy.
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_Recovery_CombatHeavyInputStartsHeavyAttack,
	"Mordecai.Recovery.Combat.HeavyInputStartsHeavyAttack",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_Recovery_CombatHeavyInputStartsHeavyAttack::RunTest(const FString& Parameters)
{
	using namespace MordecaiCombatInputAbilityTestHelpers;

	FMordecaiCombatGrantTestEnv Env;
	ON_SCOPE_EXIT { Env.Teardown(); };
	if (!SetupArmedPlayer(*this, Env))
	{
		return true;
	}
	ULyraAbilitySystemComponent* ASC = Env.GetASC();

	Env.PressInput(MordecaiGameplayTags::Input_HeavyAttack);
	TestTrue("Heavy attack active", IsAbilityActive(Env, UMordecaiGA_HeavyAttack::StaticClass()));
	TestFalse("Light attack not active", IsAbilityActive(Env, UMordecaiGA_MeleeAttack::StaticClass()));
	TestTrue("Attacking tag", ASC->HasMatchingGameplayTag(MordecaiGameplayTags::State_Attacking));
	TestTrue("Mordecai.Attack.Heavy tag", ASC->HasMatchingGameplayTag(MordecaiGameplayTags::Attack_Heavy));

	const FGameplayAbilitySpec* HeavySpec = Env.FindSpec(UMordecaiGA_HeavyAttack::StaticClass());
	const UMordecaiGA_HeavyAttack* Heavy = HeavySpec ? Cast<UMordecaiGA_HeavyAttack>(HeavySpec->GetPrimaryInstance()) : nullptr;
	if (TestNotNull("Heavy instance", Heavy))
	{
		const UMordecaiWeaponDataAsset* Weapon = Env.Avatar->GetEquipmentComponent()->GetEquippedWeapon(EMordecaiEquipSlot::MainHand)->WeaponDataAsset;
		TestTrue("Swinging the equipped weapon's heavy profile", Heavy->GetActiveProfile() == Weapon->HeavyAttackProfile.Get());
		TestEqual("In windup", Heavy->GetCurrentPhase(), EMordecaiAttackPhase::Windup);
	}

	// A light press during the heavy attack does not start a second attack
	Env.ReleaseInput(MordecaiGameplayTags::Input_HeavyAttack);
	Env.PressInput(MordecaiGameplayTags::Input_LightAttack);
	TestFalse("No light attack during the heavy one", IsAbilityActive(Env, UMordecaiGA_MeleeAttack::StaticClass()));
	return true;
}

// ---------------------------------------------------------------------------
// Mordecai.Recovery.Combat.LightAndHeavyRemainExclusiveThroughRecovery (AC-004.14)
// Light and heavy are separate specs. Recovery remains part of the running swing, so the
// other spec must stay blocked until the first ability ends.
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_Recovery_CombatAttackRecoveryExclusivity,
	"Mordecai.Recovery.Combat.LightAndHeavyRemainExclusiveThroughRecovery",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_Recovery_CombatAttackRecoveryExclusivity::RunTest(const FString& Parameters)
{
	using namespace MordecaiCombatInputAbilityTestHelpers;

	{
		FMordecaiCombatGrantTestEnv Env;
		ON_SCOPE_EXIT { Env.Teardown(); };
		if (!SetupArmedPlayer(*this, Env))
		{
			return true;
		}

		Env.PressInput(MordecaiGameplayTags::Input_LightAttack);
		FGameplayAbilitySpec* LightSpec =
			Env.FindSpec(UMordecaiGA_MeleeAttack::StaticClass());
		UMordecaiGA_MeleeAttack* Light = LightSpec
			? Cast<UMordecaiGA_MeleeAttack>(LightSpec->GetPrimaryInstance())
			: nullptr;
		if (!TestNotNull("Light instance", Light)
			|| !TestTrue("Light is active", LightSpec->IsActive()))
		{
			return true;
		}
		Light->TransitionToPhase(EMordecaiAttackPhase::Recovery);
		Env.PressInput(MordecaiGameplayTags::Input_HeavyAttack);
		TestTrue("Light remains active in Recovery", LightSpec->IsActive());
		TestFalse(
			"Heavy cannot start during light Recovery",
			IsAbilityActive(Env, UMordecaiGA_HeavyAttack::StaticClass()));
	}

	{
		FMordecaiCombatGrantTestEnv Env;
		ON_SCOPE_EXIT { Env.Teardown(); };
		if (!SetupArmedPlayer(*this, Env))
		{
			return true;
		}

		Env.PressInput(MordecaiGameplayTags::Input_HeavyAttack);
		FGameplayAbilitySpec* HeavySpec =
			Env.FindSpec(UMordecaiGA_HeavyAttack::StaticClass());
		UMordecaiGA_HeavyAttack* Heavy = HeavySpec
			? Cast<UMordecaiGA_HeavyAttack>(HeavySpec->GetPrimaryInstance())
			: nullptr;
		if (!TestNotNull("Heavy instance", Heavy)
			|| !TestTrue("Heavy is active", HeavySpec->IsActive()))
		{
			return true;
		}
		Heavy->TransitionToPhase(EMordecaiAttackPhase::Recovery);
		Env.PressInput(MordecaiGameplayTags::Input_LightAttack);
		TestTrue("Heavy remains active in Recovery", HeavySpec->IsActive());
		TestFalse(
			"Light cannot start during heavy Recovery",
			IsAbilityActive(Env, UMordecaiGA_MeleeAttack::StaticClass()));
	}

	return true;
}

// ---------------------------------------------------------------------------
// Mordecai.Recovery.Combat.LightComboChainsHitTwo
// State.Attacking blocks the other attack spec, but it must not block a queued continuation
// inside the already-running light ability (PR137).
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_Recovery_CombatLightComboChainsHitTwo,
	"Mordecai.Recovery.Combat.LightComboChainsHitTwo",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_Recovery_CombatLightComboChainsHitTwo::RunTest(const FString& Parameters)
{
	using namespace MordecaiCombatInputAbilityTestHelpers;

	FMordecaiCombatGrantTestEnv Env;
	ON_SCOPE_EXIT { Env.Teardown(); };
	if (!SetupArmedPlayer(*this, Env))
	{
		return true;
	}

	Env.PressInput(MordecaiGameplayTags::Input_LightAttack);
	FGameplayAbilitySpec* LightSpec =
		Env.FindSpec(UMordecaiGA_MeleeAttack::StaticClass());
	UMordecaiGA_MeleeAttack* Light = LightSpec
		? Cast<UMordecaiGA_MeleeAttack>(LightSpec->GetPrimaryInstance())
		: nullptr;
	if (!TestNotNull("Light instance", Light)
		|| !TestTrue("Hit 1 is active", LightSpec->IsActive()))
	{
		return true;
	}

	Light->TransitionToPhase(EMordecaiAttackPhase::Recovery);
	Env.PressInput(MordecaiGameplayTags::Input_LightAttack);
	Light->OnRecoveryComplete();

	TestTrue("The same light ability remains active for Hit 2", LightSpec->IsActive());
	TestEqual("Hit 2 advances to combo index 1", Light->GetCurrentComboIndex(), 1);
	TestEqual("Hit 2 begins its windup", Light->GetCurrentPhase(), EMordecaiAttackPhase::Windup);
	TestTrue(
		"Whole-ability State.Attacking remains owned across the chain",
		Env.GetASC()->HasMatchingGameplayTag(MordecaiGameplayTags::State_Attacking));
	TestTrue(
		"Hit 2 commits facing again",
		Env.GetASC()->HasMatchingGameplayTag(MordecaiGameplayTags::State_AttackCommitted));
	TestFalse(
		"Combo chaining did not activate the heavy spec",
		IsAbilityActive(Env, UMordecaiGA_HeavyAttack::StaticClass()));
	return true;
}

// ---------------------------------------------------------------------------
// Mordecai.Recovery.Combat.SprintComposesWithBlockSpeed
// Sprint and block both scale the walk speed; toggling sprint mid-block must not leave a wrong speed.
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_Recovery_CombatSprintComposesWithBlockSpeed,
	"Mordecai.Recovery.Combat.SprintComposesWithBlockSpeed",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_Recovery_CombatSprintComposesWithBlockSpeed::RunTest(const FString& Parameters)
{
	using namespace MordecaiCombatInputAbilityTestHelpers;

	FMordecaiCombatGrantTestEnv Env;
	ON_SCOPE_EXIT { Env.Teardown(); };
	if (!SetupArmedPlayer(*this, Env))
	{
		return true;
	}
	AMordecaiCharacter* Character = Env.Avatar;
	const UCharacterMovementComponent* CMC = Character->GetCharacterMovement();
	const float Walk = Character->WalkSpeed;
	const float Sprint = Character->WalkSpeed * Character->SprintSpeedMultiplier;
	const float BlockMul = GetDefault<UMordecaiGA_Block>()->BlockMoveSpeedMultiplier;
	TestEqual("Walking", CMC->MaxWalkSpeed, Walk);

	Character->SetSprinting(true);
	TestEqual("Sprinting", CMC->MaxWalkSpeed, Sprint);
	Character->SetSprinting(true);
	TestEqual("Sprinting twice changes nothing", CMC->MaxWalkSpeed, Sprint);
	Character->SetSprinting(false);
	TestEqual("Walking again", CMC->MaxWalkSpeed, Walk);

	// A new sprint toggle is refused while block owns its interrupt tag.
	Env.PressInput(MordecaiGameplayTags::Input_Block);
	TestEqual("Blocking walk", CMC->MaxWalkSpeed, Walk * BlockMul);
	Character->SetSprinting(true);
	TestFalse("Sprint is refused while a real block is active", Character->bIsSprinting);
	TestEqual("Blocking speed remains unsprinted", CMC->MaxWalkSpeed, Walk * BlockMul);
	Env.ReleaseInput(MordecaiGameplayTags::Input_Block);
	TestFalse("Releasing block does not resurrect the refused sprint", Character->bIsSprinting);
	TestEqual("Walk after refused sprint and block release", CMC->MaxWalkSpeed, Walk);

	// Sprint on, block, sprint off while blocking, release
	Character->SetSprinting(true);
	Env.PressInput(MordecaiGameplayTags::Input_Block);
	Character->SetSprinting(false);
	TestEqual("Blocking walk after sprint off", CMC->MaxWalkSpeed, Walk * BlockMul);
	Env.ReleaseInput(MordecaiGameplayTags::Input_Block);
	TestEqual("Walk after block", CMC->MaxWalkSpeed, Walk);
	return true;
}
