// Project Mordecai — Recovery: enemy attacks reach the player (US-084, issue #12 / N7)

#include "Misc/AutomationTest.h"
#include "AbilitySystem/LyraAbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "Character/LyraPawnData.h"
#include "Character/LyraPawnExtensionComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/Engine.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "GameModes/LyraGameState.h"
#include "GameplayAbilitySpec.h"
#include "TimerManager.h"
#include "Mordecai/AbilitySystem/MordecaiAttributeSet.h"
#include "Mordecai/Combat/MordecaiAttackProfileDataAsset.h"
#include "Mordecai/Combat/MordecaiGA_MeleeAttack.h"
#include "Mordecai/Combat/MordecaiHitDetectionSubsystem.h"
#include "Mordecai/Enemy/MordecaiEnemyAIController.h"
#include "Mordecai/Enemy/MordecaiEnemyCharacter.h"
#include "Mordecai/MordecaiCharacter.h"
#include "Mordecai/MordecaiGameplayTags.h"
#include "Mordecai/MordecaiPlayerState.h"

namespace MordecaiRecoveryEnemyTest
{
	/** A quick arc sweep in front of the attacker: 120 degrees, 250 cm, 10 slash damage, 0.1 s per phase. */
	UMordecaiAttackProfileDataAsset* MakeStubProfile()
	{
		UMordecaiAttackProfileDataAsset* Profile = NewObject<UMordecaiAttackProfileDataAsset>();
		Profile->InputSlot = EMordecaiInputSlot::Light;
		Profile->WindupTimeMs = 100.f;
		Profile->ActiveTimeMs = 100.f;
		Profile->RecoveryTimeMs = 100.f;
		Profile->HitShapeType = EMordecaiHitShapeType::ArcSector;
		Profile->HitShapeParams.Radius = 250.f;
		Profile->HitShapeParams.Angle = 120.f;
		Profile->DamageProfile.DamageType = EMordecaiDamageType::Slash;
		Profile->DamageProfile.BasePower = 10.f;
		return Profile;
	}

	/**
	 * A game world with the player as the game builds it: a Mordecai player state owning the ASC (Lyra pattern)
	 * and a Mordecai character whose pawn extension uses that ASC. No Lyra teams are created, as in the Mordecai
	 * experiences, so the player and the enemies all have FGenericTeamId::NoTeam. Nothing ticks.
	 */
	struct FEnemyAttackTestEnv
	{
		UWorld* World = nullptr;
		AMordecaiPlayerState* PlayerState = nullptr;
		AMordecaiCharacter* Player = nullptr;

		bool Setup(const FVector& PlayerLocation)
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
			Player = World->SpawnActor<AMordecaiCharacter>(AMordecaiCharacter::StaticClass(), PlayerLocation, FRotator::ZeroRotator, AlwaysSpawn());
			if (!PlayerState || !Player)
			{
				return false;
			}

			ULyraPawnExtensionComponent* PawnExt = ULyraPawnExtensionComponent::FindPawnExtensionComponent(Player);
			if (!PawnExt || !PlayerState->GetLyraAbilitySystemComponent())
			{
				return false;
			}
			PawnExt->SetPawnData(NewObject<ULyraPawnData>(GetTransientPackage()));
			PawnExt->InitializeAbilitySystem(PlayerState->GetLyraAbilitySystemComponent(), PlayerState);
			return true;
		}

		/** An enemy spawned like a placed one (its AI controller possesses it) with the melee ability and these profiles. */
		AMordecaiEnemyCharacter* SpawnEnemy(const FVector& Location, const TArray<UMordecaiAttackProfileDataAsset*>& Profiles) const
		{
			AMordecaiEnemyCharacter* Enemy = World->SpawnActorDeferred<AMordecaiEnemyCharacter>(
				AMordecaiEnemyCharacter::StaticClass(), FTransform(FRotator::ZeroRotator, Location), nullptr, nullptr,
				ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
			if (!Enemy)
			{
				return nullptr;
			}
			Enemy->DefaultAbilities.Add(UMordecaiGA_MeleeAttack::StaticClass());
			for (UMordecaiAttackProfileDataAsset* Profile : Profiles)
			{
				Enemy->EnemyAttackProfiles.Add(Profile);
			}
			Enemy->FinishSpawning(FTransform(FRotator::ZeroRotator, Location));

			if (!Enemy->GetController())
			{
				if (AMordecaiEnemyAIController* AI = World->SpawnActor<AMordecaiEnemyAIController>())
				{
					AI->Possess(Enemy);
				}
			}
			Enemy->InitializeAbilitySystem();
			return Enemy;
		}

		UAbilitySystemComponent* GetPlayerASC() const
		{
			return UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(Player);
		}

		float GetPlayerHealth() const
		{
			const UAbilitySystemComponent* ASC = GetPlayerASC();
			return ASC ? ASC->GetNumericAttribute(UMordecaiAttributeSet::GetHealthAttribute()) : -1.f;
		}

		void Teardown()
		{
			if (World && GEngine)
			{
				GEngine->DestroyWorldContext(World);
				World->DestroyWorld(false);
			}
			World = nullptr;
			PlayerState = nullptr;
			Player = nullptr;
		}

		static FActorSpawnParameters AlwaysSpawn()
		{
			FActorSpawnParameters Params;
			Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
			return Params;
		}
	};
}

using namespace MordecaiRecoveryEnemyTest;

// ---------------------------------------------------------------------------
// 1. Mordecai.Recovery.Enemy.AttackDamagesPlayerWithPlayerStateASC (AC-084.1)
// An enemy with one profile, in Attack state within AttackRange of the player, damages the player's
// PlayerState ASC. Runs the AI state machine and the melee ability's real windup -> active hit.
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_Recovery_Enemy_AttackDamagesPlayer,
	"Mordecai.Recovery.Enemy.AttackDamagesPlayerWithPlayerStateASC",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_Recovery_Enemy_AttackDamagesPlayer::RunTest(const FString& Parameters)
{
	FEnemyAttackTestEnv Env;
	if (!TestTrue("test world with a player set up", Env.Setup(FVector(150.0, 0.0, 0.0))))
	{
		Env.Teardown();
		return true;
	}

	AMordecaiEnemyCharacter* Enemy = Env.SpawnEnemy(FVector::ZeroVector, { MakeStubProfile() });
	AMordecaiEnemyAIController* AI = Enemy ? Cast<AMordecaiEnemyAIController>(Enemy->GetController()) : nullptr;
	if (!TestNotNull("enemy spawned", Enemy) || !TestNotNull("enemy AI controller possesses it", AI))
	{
		Env.Teardown();
		return true;
	}

	TestTrue("the player's ASC lives on the PlayerState", Env.GetPlayerASC() && Env.GetPlayerASC()->GetOwnerActor() == Env.PlayerState);
	const float HealthBefore = Env.GetPlayerHealth();
	TestEqual("player starts at full health", HealthBefore, 100.f);

	// Idle -> Approach (aggro) -> Attack (in range) -> the melee ability activates
	AI->UpdateStateMachine(0.016f, Env.Player);
	AI->UpdateStateMachine(0.016f, Env.Player);
	TestEqual("enemy is in Attack within AttackRange", AI->GetCurrentState(), EMordecaiEnemyAIState::Attack);
	AI->UpdateStateMachine(0.016f, Env.Player);

	UAbilitySystemComponent* EnemyASC = Enemy->GetAbilitySystemComponent();
	FGameplayAbilitySpec* MeleeSpec = EnemyASC ? EnemyASC->FindAbilitySpecFromClass(UMordecaiGA_MeleeAttack::StaticClass()) : nullptr;
	TestTrue("enemy melee attack is active", MeleeSpec && MeleeSpec->IsActive());
	const UMordecaiGA_MeleeAttack* Melee = MeleeSpec ? Cast<UMordecaiGA_MeleeAttack>(MeleeSpec->GetPrimaryInstance()) : nullptr;
	TestTrue("enemy attack is in Windup", Melee && Melee->GetCurrentPhase() == EMordecaiAttackPhase::Windup);

	// Windup (0.1 s) ends: the Active phase runs the hit query and applies damage. A timer set before the timer
	// manager's first tick of a frame is pending until that tick ends, and it ticks once per frame: two frames.
	for (int32 Frame = 0; Frame < 2; ++Frame)
	{
		++GFrameCounter;
		Env.World->GetTimerManager().Tick(0.15f);
	}

	TestTrue(FString::Printf(TEXT("player health dropped (%.1f -> %.1f)"), HealthBefore, Env.GetPlayerHealth()),
		Env.GetPlayerHealth() < HealthBefore);

	Env.Teardown();
	return true;
}

// ---------------------------------------------------------------------------
// 2. Mordecai.Recovery.Enemy.AttackTargetsPlayerNotAllies (AC-084.2)
// The enemy's melee hit query is hostile-filtered: it hits the player, not another enemy, with friendly fire
// off; airborne rules still apply to the player.
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_Recovery_Enemy_AttackTargetsPlayerNotAllies,
	"Mordecai.Recovery.Enemy.AttackTargetsPlayerNotAllies",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_Recovery_Enemy_AttackTargetsPlayerNotAllies::RunTest(const FString& Parameters)
{
	FEnemyAttackTestEnv Env;
	if (!TestTrue("test world with a player set up", Env.Setup(FVector(150.0, 80.0, 0.0))))
	{
		Env.Teardown();
		return true;
	}

	UMordecaiAttackProfileDataAsset* Profile = MakeStubProfile();
	AMordecaiEnemyCharacter* Attacker = Env.SpawnEnemy(FVector::ZeroVector, { Profile });
	AMordecaiEnemyCharacter* Ally = Env.SpawnEnemy(FVector(150.0, -80.0, 0.0), { Profile });
	UMordecaiHitDetectionSubsystem* HitDetection = Env.World->GetSubsystem<UMordecaiHitDetectionSubsystem>();
	if (!TestNotNull("attacker spawned", Attacker) || !TestNotNull("ally spawned", Ally) || !TestNotNull("hit detection", HitDetection))
	{
		Env.Teardown();
		return true;
	}

	// Lyra teams are not created in Mordecai experiences: everyone is NoTeam, which must not make them allies
	TestFalse("enemy and player are not friendly", UMordecaiHitDetectionSubsystem::AreActorsFriendly(Attacker, Env.Player));
	TestFalse("player and enemy are not friendly", UMordecaiHitDetectionSubsystem::AreActorsFriendly(Env.Player, Attacker));
	TestTrue("two enemies are friendly", UMordecaiHitDetectionSubsystem::AreActorsFriendly(Attacker, Ally));

	auto HitActors = [&]()
	{
		TArray<AActor*> Actors;
		for (const FMordecaiHitResult& Hit : HitDetection->PerformMeleeHitDetection(
			Profile, Attacker->GetActorLocation(), Attacker->GetActorForwardVector(), Attacker))
		{
			Actors.Add(Hit.HitActor.Get());
		}
		return Actors;
	};

	TArray<AActor*> Hits = HitActors();
	TestTrue("the enemy's sweep hits the player", Hits.Contains(Env.Player));
	TestFalse("the enemy's sweep does not hit the other enemy", Hits.Contains(Ally));
	TestFalse("the enemy's sweep does not hit itself", Hits.Contains(Attacker));

	// Airborne rules (AC-003.7/003.8): a jump-avoidable sweep misses an airborne player, unless it hits airborne
	Env.GetPlayerASC()->AddLooseGameplayTag(MordecaiGameplayTags::State_Airborne);
	Profile->JumpAvoidable = true;
	TestFalse("a jump-avoidable sweep misses the airborne player", HitActors().Contains(Env.Player));
	Profile->HitsAirborne = true;
	TestTrue("a sweep that hits airborne hits the airborne player", HitActors().Contains(Env.Player));

	Env.Teardown();
	return true;
}

// ---------------------------------------------------------------------------
// 3. Mordecai.Recovery.Enemy.LogsMissingContent (AC-084.3)
// An enemy without attack profiles, or without a visible mesh, logs a LogMordecaiEnemy error naming it.
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_Recovery_Enemy_LogsMissingContent,
	"Mordecai.Recovery.Enemy.LogsMissingContent",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_Recovery_Enemy_LogsMissingContent::RunTest(const FString& Parameters)
{
	FEnemyAttackTestEnv Env;
	if (!TestTrue("test world with a player set up", Env.Setup(FVector(2000.0, 0.0, 0.0))))
	{
		Env.Teardown();
		return true;
	}

	// The native enemy has neither: no profiles, and its skeletal mesh component has no mesh asset
	AMordecaiEnemyCharacter* Bare = Env.SpawnEnemy(FVector::ZeroVector, {});
	if (!TestNotNull("bare enemy spawned", Bare))
	{
		Env.Teardown();
		return true;
	}
	AddExpectedErrorPlain(FString::Printf(TEXT("enemy %s has no attack profiles"), *Bare->GetName()), EAutomationExpectedErrorFlags::Contains, 1);
	AddExpectedErrorPlain(FString::Printf(TEXT("enemy %s has no visible mesh"), *Bare->GetName()), EAutomationExpectedErrorFlags::Contains, 1);
	TestFalse("bare enemy has no visible mesh", Bare->HasVisibleMesh());
	TestFalse("bare enemy fails content validation", Bare->ValidateContent());

	// With a profile and a visible mesh it validates, silently
	AMordecaiEnemyCharacter* Complete = Env.SpawnEnemy(FVector(0.0, 500.0, 0.0), { MakeStubProfile() });
	if (!TestNotNull("complete enemy spawned", Complete))
	{
		Env.Teardown();
		return true;
	}
	UStaticMeshComponent* Body = NewObject<UStaticMeshComponent>(Complete);
	Body->SetStaticMesh(NewObject<UStaticMesh>(GetTransientPackage()));
	Body->SetupAttachment(Complete->GetRootComponent());
	Body->RegisterComponent();
	TestTrue("complete enemy has a visible mesh", Complete->HasVisibleMesh());
	TestTrue("complete enemy passes content validation", Complete->ValidateContent());

	// A mesh hidden in game is not visible
	Body->SetHiddenInGame(true);
	TestFalse("a mesh hidden in game does not count", Complete->HasVisibleMesh());

	Env.Teardown();
	return true;
}

// ---------------------------------------------------------------------------
// 4. Mordecai.Recovery.Enemy.NoSilentAttackLoop (AC-084.4)
// An enemy whose melee attack has no profile does not cycle Attack -> Recover: it logs once and stays in
// Approach (chase) at the player.
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_Recovery_Enemy_NoSilentAttackLoop,
	"Mordecai.Recovery.Enemy.NoSilentAttackLoop",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_Recovery_Enemy_NoSilentAttackLoop::RunTest(const FString& Parameters)
{
	FEnemyAttackTestEnv Env;
	if (!TestTrue("test world with a player set up", Env.Setup(FVector(150.0, 0.0, 0.0))))
	{
		Env.Teardown();
		return true;
	}

	AMordecaiEnemyCharacter* Enemy = Env.SpawnEnemy(FVector::ZeroVector, {});
	AMordecaiEnemyAIController* AI = Enemy ? Cast<AMordecaiEnemyAIController>(Enemy->GetController()) : nullptr;
	if (!TestNotNull("enemy spawned", Enemy) || !TestNotNull("enemy AI controller possesses it", AI))
	{
		Env.Teardown();
		return true;
	}

	AddExpectedErrorPlain(FString::Printf(TEXT("enemy %s cannot attack"), *Enemy->GetName()), EAutomationExpectedErrorFlags::Contains, 1);
	TestFalse("melee attack without a profile cannot be performed", AI->CanPerformMeleeAttack());

	for (int32 Step = 0; Step < 30; ++Step)
	{
		AI->UpdateStateMachine(0.1f, Env.Player);
		const EMordecaiEnemyAIState State = AI->GetCurrentState();
		if (State == EMordecaiEnemyAIState::Attack || State == EMordecaiEnemyAIState::Recover)
		{
			AddError(FString::Printf(TEXT("step %d: entered %s without an attack that can hit"), Step,
				State == EMordecaiEnemyAIState::Attack ? TEXT("Attack") : TEXT("Recover")));
			break;
		}
	}
	TestEqual("enemy stays in Approach at the player", AI->GetCurrentState(), EMordecaiEnemyAIState::Approach);
	TestEqual("player is untouched", Env.GetPlayerHealth(), 100.f);

	// With a profile, the same enemy attacks
	AMordecaiEnemyCharacter* Armed = Env.SpawnEnemy(FVector(0.0, 300.0, 0.0), { MakeStubProfile() });
	AMordecaiEnemyAIController* ArmedAI = Armed ? Cast<AMordecaiEnemyAIController>(Armed->GetController()) : nullptr;
	if (TestNotNull("armed enemy AI", ArmedAI))
	{
		TestTrue("melee attack with a profile can be performed", ArmedAI->CanPerformMeleeAttack());
	}

	Env.Teardown();
	return true;
}
