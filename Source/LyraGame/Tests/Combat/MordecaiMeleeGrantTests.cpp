// Project Mordecai — Melee grant tests (issue #40)
// The melee ability receives the granting controller's MeleeAttackProfiles through the granted spec's
// SourceObject, never by writing them into the ability CDO: native CDOs are in the packaged build's
// disregard-for-GC pool, and a CDO pointing at a loaded data asset is a fatal GC verification error.

#include "Misc/AutomationTest.h"
#include "Misc/ScopeExit.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "GameplayAbilitySpec.h"
#include "GameModes/LyraGameState.h"
#include "Mordecai/Combat/MordecaiAttackProfileDataAsset.h"
#include "Mordecai/Combat/MordecaiGA_MeleeAttack.h"
#include "Mordecai/Enemy/MordecaiEnemyCharacter.h"
#include "Mordecai/MordecaiPlayerController.h"
#include "Mordecai/MordecaiPlayerState.h"

namespace MordecaiMeleeGrantTestHelpers
{
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

	/**
	 * A game world with a Lyra game state (Lyra player states require one), a Mordecai player state (it owns
	 * the ASC, as in the game) and a Mordecai player controller using it, so the controller's real grant path
	 * runs. The controller is not spawned: it needs no camera or cheat manager, and it still has authority.
	 */
	struct FMordecaiCombatGrantTestEnv
	{
		UWorld* World = nullptr;
		AMordecaiPlayerState* PlayerState = nullptr;
		AMordecaiPlayerController* PC = nullptr;

		bool Setup()
		{
			if (!GEngine)
			{
				return false;
			}
			World = UWorld::CreateWorld(EWorldType::Game, false);
			if (!World)
			{
				return false;
			}
			FWorldContext& WorldContext = GEngine->CreateNewWorldContext(EWorldType::Game);
			WorldContext.SetCurrentWorld(World);
			World->InitializeActorsForPlay(FURL());

			if (!World->SpawnActor<ALyraGameState>(ALyraGameState::StaticClass()))
			{
				return false;
			}
			PlayerState = World->SpawnActor<AMordecaiPlayerState>(AMordecaiPlayerState::StaticClass());
			if (!PlayerState)
			{
				return false;
			}
			PC = NewObject<AMordecaiPlayerController>();
			PC->PlayerState = PlayerState;
			return true;
		}

		/** Grant through the controller with only the melee ability and these profiles; returns the melee spec. */
		FGameplayAbilitySpec* GrantMeleeOnly(const TArray<UMordecaiAttackProfileDataAsset*>& Profiles)
		{
			PC->DefaultCombatAbilities.Reset();
			PC->DefaultCombatAbilities.Add(UMordecaiGA_MeleeAttack::StaticClass());
			PC->MeleeAttackProfiles.Reset();
			for (UMordecaiAttackProfileDataAsset* Profile : Profiles)
			{
				PC->MeleeAttackProfiles.Add(Profile);
			}
			// No spells: their grant loads DA_Spell_* content, which these tests are not about
			PC->DefaultSpellAbilities.Reset();

			PC->GrantCombatAbilities();

			UAbilitySystemComponent* ASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(PlayerState);
			return ASC ? ASC->FindAbilitySpecFromClass(UMordecaiGA_MeleeAttack::StaticClass()) : nullptr;
		}

		void Teardown()
		{
			if (PC)
			{
				PC->PlayerState = nullptr;
			}
			if (World && GEngine)
			{
				GEngine->DestroyWorldContext(World);
				World->DestroyWorld(false);
			}
			World = nullptr;
			PlayerState = nullptr;
			PC = nullptr;
		}
	};
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
