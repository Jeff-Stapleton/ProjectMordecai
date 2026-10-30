// Project Mordecai — Recovery: enemy attacks reach the player (US-084, issue #12 / N7)

#include "Misc/AutomationTest.h"
#include "Components/CapsuleComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "GameplayAbilitySpec.h"
#include "TimerManager.h"
#include "UObject/SoftObjectPath.h"
#include "Mordecai/Combat/MordecaiHitDetectionSubsystem.h"
#include "Mordecai/MordecaiGameplayTags.h"
#include "Tests/Recovery/MordecaiRecoveryTestEnv.h"

using namespace MordecaiRecoveryTest;

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
	FRecoveryTestEnv Env;
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
// 1b. Mordecai.Recovery.Enemy.ClosesToWeaponReachBeforeAttacking (AC-084.1)
// The Frontliners' slash reaches 150 cm but their AttackRange is 200 (AC-054.2 vs AC-054.3). An enemy that
// roots into Attack between the two swings at air forever (N7 at the spawn: enemies at ~195 cm, player at
// 100/100). The AI attacks from its profile's reach (issue #47), so it keeps approaching until the target is
// inside it, then attacks and hits.
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_Recovery_Enemy_ClosesToWeaponReach,
	"Mordecai.Recovery.Enemy.ClosesToWeaponReachBeforeAttacking",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_Recovery_Enemy_ClosesToWeaponReach::RunTest(const FString& Parameters)
{
	FRecoveryTestEnv Env;
	if (!TestTrue("test world with a player set up", Env.Setup(FVector(180.0, 0.0, 0.0))))
	{
		Env.Teardown();
		return true;
	}

	// DA_EnemyAttack_BasicSlash's shape: a 90 degree arc of 150 cm
	UMordecaiAttackProfileDataAsset* Slash = MakeStubProfile();
	Slash->HitShapeParams.Radius = 150.f;
	Slash->HitShapeParams.Angle = 90.f;
	AMordecaiEnemyCharacter* Enemy = Env.SpawnEnemy(FVector::ZeroVector, { Slash });
	AMordecaiEnemyAIController* AI = Enemy ? Cast<AMordecaiEnemyAIController>(Enemy->GetController()) : nullptr;
	if (!TestNotNull("enemy spawned", Enemy) || !TestNotNull("enemy AI controller possesses it", AI))
	{
		Env.Teardown();
		return true;
	}
	TestEqual("AttackRange is the AC-054.2 default", AI->AttackRange, 200.f);
	TestEqual("effective attack range is the slash's reach", AI->GetEffectiveAttackRange(), 150.f);

	// 180 cm: inside AttackRange, outside the slash: keep approaching
	for (int32 Step = 0; Step < 5; ++Step)
	{
		AI->UpdateStateMachine(0.016f, Env.Player);
	}
	TestEqual("at 180 cm the enemy still approaches", AI->GetCurrentState(), EMordecaiEnemyAIState::Approach);

	// 140 cm: inside the slash: attack, and the swing lands
	Env.Player->SetActorLocation(FVector(140.0, 0.0, 0.0));
	AI->UpdateStateMachine(0.016f, Env.Player);
	TestEqual("at 140 cm the enemy attacks", AI->GetCurrentState(), EMordecaiEnemyAIState::Attack);
	AI->UpdateStateMachine(0.016f, Env.Player);
	for (int32 Frame = 0; Frame < 2; ++Frame)
	{
		++GFrameCounter;
		Env.World->GetTimerManager().Tick(0.15f);
	}
	TestTrue(FString::Printf(TEXT("the swing lands (health %.1f)"), Env.GetPlayerHealth()), Env.GetPlayerHealth() < 100.f);

	Env.Teardown();
	return true;
}

// ---------------------------------------------------------------------------
// 1c. Mordecai.Recovery.Enemy.AttackReachFollowsHitShape (AC-084.1)
// The reach the AI closes to is the distance the hit query tests for each shape.
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_Recovery_Enemy_AttackReachFollowsHitShape,
	"Mordecai.Recovery.Enemy.AttackReachFollowsHitShape",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_Recovery_Enemy_AttackReachFollowsHitShape::RunTest(const FString& Parameters)
{
	UMordecaiGA_MeleeAttack* Melee = NewObject<UMordecaiGA_MeleeAttack>();
	TestEqual("no profile, no reach", Melee->GetAttackReach(), 0.f);

	UMordecaiAttackProfileDataAsset* Profile = MakeStubProfile();
	Profile->HitShapeParams.Radius = 150.f;
	Profile->HitShapeParams.Length = 120.f;
	Melee->AttackProfiles.Add(Profile);

	Profile->HitShapeType = EMordecaiHitShapeType::ArcSector;
	TestEqual("arc: its radius", Melee->GetAttackReach(), 150.f);
	Profile->HitShapeType = EMordecaiHitShapeType::Circle;
	TestEqual("circle: its offset plus its radius", Melee->GetAttackReach(), 270.f);
	Profile->HitShapeType = EMordecaiHitShapeType::Capsule;
	TestEqual("capsule: its length", Melee->GetAttackReach(), 120.f);
	Profile->HitShapeType = EMordecaiHitShapeType::Box;
	TestEqual("box: its length", Melee->GetAttackReach(), 120.f);
	return true;
}

// ---------------------------------------------------------------------------
// 1d. Mordecai.Recovery.Enemy.AttackRangeDerivedFromProfile (issue #47)
// The engage distance comes from the attack profile, not from AttackRange: a swing that reaches farther than
// AttackRange is swung from its own reach too. AttackRange is only the fallback for a melee attack without a profile.
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_Recovery_Enemy_AttackRangeDerivedFromProfile,
	"Mordecai.Recovery.Enemy.AttackRangeDerivedFromProfile",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_Recovery_Enemy_AttackRangeDerivedFromProfile::RunTest(const FString& Parameters)
{
	FRecoveryTestEnv Env;
	if (!TestTrue("test world with a player set up", Env.Setup(FVector(230.0, 0.0, 0.0))))
	{
		Env.Teardown();
		return true;
	}

	// The stub sweep reaches 250 cm, past the 200 cm AttackRange
	AMordecaiEnemyCharacter* Enemy = Env.SpawnEnemy(FVector::ZeroVector, { MakeStubProfile() });
	AMordecaiEnemyCharacter* Bare = Env.SpawnEnemy(FVector(0.0, 1000.0, 0.0), {});
	AMordecaiEnemyAIController* AI = Enemy ? Cast<AMordecaiEnemyAIController>(Enemy->GetController()) : nullptr;
	AMordecaiEnemyAIController* BareAI = Bare ? Cast<AMordecaiEnemyAIController>(Bare->GetController()) : nullptr;
	if (!TestNotNull("enemy AI", AI) || !TestNotNull("profile-less enemy AI", BareAI))
	{
		Env.Teardown();
		return true;
	}

	TestEqual("AttackRange is the AC-054.2 default", AI->AttackRange, 200.f);
	TestEqual("effective attack range is the profile's reach, past AttackRange", AI->GetEffectiveAttackRange(), 250.f);
	TestEqual("without a profile it falls back to AttackRange", BareAI->GetEffectiveAttackRange(), BareAI->AttackRange);

	// 230 cm: outside AttackRange, inside the sweep: attack
	AI->UpdateStateMachine(0.016f, Env.Player);
	AI->UpdateStateMachine(0.016f, Env.Player);
	TestEqual("at 230 cm the enemy attacks", AI->GetCurrentState(), EMordecaiEnemyAIState::Attack);

	Env.Teardown();
	return true;
}

// ---------------------------------------------------------------------------
// 1e. Mordecai.Recovery.Enemy.BlueprintAttackRangeWithinReach (issue #47)
// The saved enemy Blueprints, spawned as the game spawns them: the range their AI attacks from is no longer than
// the reach of any of their attacks, so they never root into Attack where their swing cannot land.
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_Recovery_Enemy_BlueprintAttackRangeWithinReach,
	"Mordecai.Recovery.Enemy.BlueprintAttackRangeWithinReach",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_Recovery_Enemy_BlueprintAttackRangeWithinReach::RunTest(const FString& Parameters)
{
	const TCHAR* const ClassPaths[] = {
		TEXT("/MordecaiCore/Enemies/BP_MordecaiEnemy_Frontliner.BP_MordecaiEnemy_Frontliner_C"),
		TEXT("/MordecaiCore/Enemies/BP_MordecaiEnemy_TestAttacker.BP_MordecaiEnemy_TestAttacker_C"),
	};

	FRecoveryTestEnv Env;
	if (!TestTrue("test world with a player set up", Env.Setup(FVector(5000.0, 0.0, 0.0))))
	{
		Env.Teardown();
		return true;
	}

	double SpawnY = 0.0;
	for (const TCHAR* ClassPath : ClassPaths)
	{
		UClass* EnemyClass = FSoftClassPath(ClassPath).TryLoadClass<AMordecaiEnemyCharacter>();
		if (!TestNotNull(FString::Printf(TEXT("%s loads as an AMordecaiEnemyCharacter class"), ClassPath), EnemyClass))
		{
			continue;
		}
		AMordecaiEnemyCharacter* Enemy = Env.SpawnEnemyOfClass(EnemyClass, FVector(0.0, SpawnY, 0.0));
		SpawnY += 1000.0;
		AMordecaiEnemyAIController* AI = Enemy ? Cast<AMordecaiEnemyAIController>(Enemy->GetController()) : nullptr;
		if (!TestNotNull(FString::Printf(TEXT("%s spawns with an enemy AI controller"), *EnemyClass->GetName()), AI))
		{
			continue;
		}
		if (!TestTrue(FString::Printf(TEXT("%s has attack profiles"), *EnemyClass->GetName()), Enemy->EnemyAttackProfiles.Num() > 0))
		{
			continue;
		}

		const float Range = AI->GetEffectiveAttackRange();
		UMordecaiGA_MeleeAttack* Melee = NewObject<UMordecaiGA_MeleeAttack>();
		for (const TObjectPtr<UMordecaiAttackProfileDataAsset>& Profile : Enemy->EnemyAttackProfiles)
		{
			Melee->AttackProfiles = { Profile };
			const float Reach = Melee->GetAttackReach();
			TestTrue(FString::Printf(TEXT("%s's attack range %.0f is no longer than %s's reach %.0f"),
					*EnemyClass->GetName(), Range, *GetNameSafe(Profile), Reach),
				Reach > 0.f && Range <= Reach);
		}
	}

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
	FRecoveryTestEnv Env;
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
// 3. Mordecai.Recovery.Enemy.PlayerSweepsSpareOtherPlayers (AC-084.2, AC-003.9)
// The enemy tag decides only when an enemy is involved: two players (both NoTeam, no enemy tag) stay friendly,
// so a player's sweep does not hit a co-op partner with friendly fire off, and does hit an enemy.
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_Recovery_Enemy_PlayerSweepsSpareOtherPlayers,
	"Mordecai.Recovery.Enemy.PlayerSweepsSpareOtherPlayers",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_Recovery_Enemy_PlayerSweepsSpareOtherPlayers::RunTest(const FString& Parameters)
{
	FRecoveryTestEnv Env;
	if (!TestTrue("test world with a player set up", Env.Setup(FVector::ZeroVector)))
	{
		Env.Teardown();
		return true;
	}

	AMordecaiPlayerState* PartnerState = nullptr;
	AMordecaiCharacter* Partner = nullptr;
	UMordecaiAttackProfileDataAsset* Profile = MakeStubProfile();
	AMordecaiEnemyCharacter* Enemy = Env.SpawnEnemy(FVector(150.0, -80.0, 0.0), { Profile });
	UMordecaiHitDetectionSubsystem* HitDetection = Env.World->GetSubsystem<UMordecaiHitDetectionSubsystem>();
	if (!TestTrue("partner player spawned", Env.SpawnPlayer(FVector(150.0, 80.0, 0.0), PartnerState, Partner))
		|| !TestNotNull("enemy spawned", Enemy) || !TestNotNull("hit detection", HitDetection))
	{
		Env.Teardown();
		return true;
	}

	TestTrue("two players are friendly", UMordecaiHitDetectionSubsystem::AreActorsFriendly(Env.Player, Partner));

	TArray<AActor*> Hits;
	for (const FMordecaiHitResult& Hit : HitDetection->PerformMeleeHitDetection(
		Profile, Env.Player->GetActorLocation(), Env.Player->GetActorForwardVector(), Env.Player))
	{
		Hits.Add(Hit.HitActor.Get());
	}
	TestFalse("the player's sweep does not hit the partner", Hits.Contains(Partner));
	TestTrue("the player's sweep hits the enemy", Hits.Contains(Enemy));

	Env.Teardown();
	return true;
}

// ---------------------------------------------------------------------------
// 4. Mordecai.Recovery.Enemy.LogsMissingContent (AC-084.3)
// An enemy that begins play without attack profiles, or without a visible mesh, logs a LogMordecaiEnemy error
// naming it, exactly once; a complete enemy begins play silently.
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_Recovery_Enemy_LogsMissingContent,
	"Mordecai.Recovery.Enemy.LogsMissingContent",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_Recovery_Enemy_LogsMissingContent::RunTest(const FString& Parameters)
{
	FRecoveryTestEnv Env;
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
	TestFalse("bare enemy has no visible mesh", Bare->HasVisibleMesh());
	AddExpectedErrorPlain(FString::Printf(TEXT("enemy %s has no attack profiles"), *Bare->GetName()), EAutomationExpectedErrorFlags::Contains, 1);
	AddExpectedErrorPlain(FString::Printf(TEXT("enemy %s has no visible mesh"), *Bare->GetName()), EAutomationExpectedErrorFlags::Contains, 1);

	// The test world never begins play, so start the enemy's play the way the world would
	FRecoveryTestEnv::BeginEnemyPlay(Bare);
	TestTrue("bare enemy began play", Bare->HasActorBegunPlay());

	// With a profile and a visible mesh it begins play silently (any error would fail this test)
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
	FRecoveryTestEnv::BeginEnemyPlay(Complete);
	TestTrue("complete enemy began play", Complete->HasActorBegunPlay());
	TestTrue("complete enemy passes content validation", Complete->ValidateContent());

	// A mesh hidden in game is not visible
	Body->SetHiddenInGame(true);
	TestFalse("a mesh hidden in game does not count", Complete->HasVisibleMesh());

	Env.Teardown();
	return true;
}

// ---------------------------------------------------------------------------
// 4b. Mordecai.Recovery.Enemy.LogsInstanceStatOverride (US-085 AC-085.2/AC-085.5, issue #18 / N13)
// DevTestMap saved DefaultMaxHealth = 100 on each placed Frontliner, so they spawned with 100 health against their
// Blueprint's 200, with nothing in the log. An enemy whose base stats differ from its class's logs a LogMordecaiEnemy
// error naming the stat when it begins play; one whose stats are its class's begins play silently.
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_Recovery_Enemy_LogsInstanceStatOverride,
	"Mordecai.Recovery.Enemy.LogsInstanceStatOverride",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_Recovery_Enemy_LogsInstanceStatOverride::RunTest(const FString& Parameters)
{
	FRecoveryTestEnv Env;
	if (!TestTrue("test world with a player set up", Env.Setup(FVector(2000.0, 0.0, 0.0))))
	{
		Env.Teardown();
		return true;
	}

	// Profiles and a visible mesh, so a stat override is the only content error possible
	auto SpawnComplete = [&Env](const FVector& Location)
	{
		AMordecaiEnemyCharacter* Enemy = Env.SpawnEnemy(Location, { MakeStubProfile() });
		if (Enemy)
		{
			UStaticMeshComponent* Body = NewObject<UStaticMeshComponent>(Enemy);
			Body->SetStaticMesh(NewObject<UStaticMesh>(GetTransientPackage()));
			Body->SetupAttachment(Enemy->GetRootComponent());
			Body->RegisterComponent();
		}
		return Enemy;
	};

	AMordecaiEnemyCharacter* Matching = SpawnComplete(FVector::ZeroVector);
	AMordecaiEnemyCharacter* Overridden = SpawnComplete(FVector(0.0, 500.0, 0.0));
	if (!TestNotNull("matching enemy spawned", Matching) || !TestNotNull("overriding enemy spawned", Overridden))
	{
		Env.Teardown();
		return true;
	}
	const AMordecaiEnemyCharacter* ClassDefaults = GetDefault<AMordecaiEnemyCharacter>();

	// Stats equal to the class's: begins play silently (any error would fail this test)
	FRecoveryTestEnv::BeginEnemyPlay(Matching);
	TestTrue("matching enemy began play", Matching->HasActorBegunPlay());
	TestTrue("an enemy whose stats are its class's passes content validation", Matching->ValidateContent());

	// The N13 mismatch: the instance holds another max health than its class (logged by BeginPlay, then by the call)
	Overridden->DefaultMaxHealth = ClassDefaults->DefaultMaxHealth + 100.f;
	AddExpectedErrorPlain(FString::Printf(TEXT("enemy %s overrides DefaultMaxHealth (%.1f on the instance, %.1f on MordecaiEnemyCharacter)"),
		*Overridden->GetName(), Overridden->DefaultMaxHealth, ClassDefaults->DefaultMaxHealth), EAutomationExpectedErrorFlags::Contains, 2);
	FRecoveryTestEnv::BeginEnemyPlay(Overridden);
	TestTrue("overriding enemy began play", Overridden->HasActorBegunPlay());
	TestFalse("a max health override fails content validation", Overridden->ValidateContent());
	Overridden->DefaultMaxHealth = ClassDefaults->DefaultMaxHealth;

	Overridden->DefaultMaxPosture = ClassDefaults->DefaultMaxPosture + 50.f;
	AddExpectedErrorPlain(FString::Printf(TEXT("enemy %s overrides DefaultMaxPosture"), *Overridden->GetName()), EAutomationExpectedErrorFlags::Contains, 1);
	TestFalse("a max posture override fails content validation", Overridden->ValidateContent());
	Overridden->DefaultMaxPosture = ClassDefaults->DefaultMaxPosture;

	Overridden->DefaultMoveSpeed = ClassDefaults->DefaultMoveSpeed + 50.f;
	AddExpectedErrorPlain(FString::Printf(TEXT("enemy %s overrides DefaultMoveSpeed"), *Overridden->GetName()), EAutomationExpectedErrorFlags::Contains, 1);
	TestFalse("a move speed override fails content validation", Overridden->ValidateContent());
	Overridden->DefaultMoveSpeed = ClassDefaults->DefaultMoveSpeed;

	TestTrue("with its class's stats again it passes content validation", Overridden->ValidateContent());

	Env.Teardown();
	return true;
}

// ---------------------------------------------------------------------------
// 4c. Mordecai.Recovery.Enemy.BodyMeshSlot (US-085 AC-085.1, issue #19 / N14)
// The packaged Frontliners had no mesh: only their red health bars showed, and their capsules made an invisible
// wall. Every enemy has an empty BodyMesh for its Blueprint to fill: it rides the capsule, never collides (the
// capsule alone blocks, as before), and makes the enemy visible once it has a mesh. A native enemy leaves it empty,
// so an enemy Blueprint without a body still logs the "no visible mesh" content error.
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_Recovery_Enemy_BodyMeshSlot,
	"Mordecai.Recovery.Enemy.BodyMeshSlot",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_Recovery_Enemy_BodyMeshSlot::RunTest(const FString& Parameters)
{
	FRecoveryTestEnv Env;
	if (!TestTrue("test world with a player set up", Env.Setup(FVector(2000.0, 0.0, 0.0))))
	{
		Env.Teardown();
		return true;
	}

	AMordecaiEnemyCharacter* Enemy = Env.SpawnEnemy(FVector::ZeroVector, { MakeStubProfile() });
	if (!TestNotNull("enemy spawned", Enemy))
	{
		Env.Teardown();
		return true;
	}

	// Found by name: the Frontliner Blueprint saves its mesh on the component called BodyMesh
	UStaticMeshComponent* Body = FindObject<UStaticMeshComponent>(Enemy, TEXT("BodyMesh"));
	if (!TestNotNull("the enemy has a BodyMesh static mesh component", Body))
	{
		Env.Teardown();
		return true;
	}

	TestTrue("BodyMesh rides the capsule", Body->GetAttachParent() == Enemy->GetCapsuleComponent());
	TestNull("a native enemy's BodyMesh is empty: its Blueprint sets the mesh", Body->GetStaticMesh());
	TestFalse("so a native enemy has no visible mesh", Enemy->HasVisibleMesh());

	TestEqual("BodyMesh never collides", Body->GetCollisionEnabled(), ECollisionEnabled::NoCollision);
	TestFalse("BodyMesh generates no overlaps", Body->GetGenerateOverlapEvents());
	TestFalse("BodyMesh does not affect navigation", Body->CanEverAffectNavigation());
	TestEqual("the capsule still blocks pawns", Enemy->GetCapsuleComponent()->GetCollisionResponseToChannel(ECC_Pawn), ECR_Block);

	TestTrue("BodyMesh is visible in game", Body->IsVisible() && !Body->bHiddenInGame);
	Body->SetStaticMesh(NewObject<UStaticMesh>(GetTransientPackage()));
	TestTrue("with a mesh in BodyMesh the enemy is visible", Enemy->HasVisibleMesh());

	Env.Teardown();
	return true;
}

// ---------------------------------------------------------------------------
// 4d. Mordecai.Recovery.Enemy.BodyHiddenWhileDead (issue #19 / N14)
// A dead enemy stops colliding (AC-050.6). Its body goes with it, so the player never sees a standing enemy it can
// walk through; the arena reset brings both back.
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_Recovery_Enemy_BodyHiddenWhileDead,
	"Mordecai.Recovery.Enemy.BodyHiddenWhileDead",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_Recovery_Enemy_BodyHiddenWhileDead::RunTest(const FString& Parameters)
{
	FRecoveryTestEnv Env;
	if (!TestTrue("test world with a player set up", Env.Setup(FVector(2000.0, 0.0, 0.0))))
	{
		Env.Teardown();
		return true;
	}

	AMordecaiEnemyCharacter* Enemy = Env.SpawnEnemy(FVector::ZeroVector, { MakeStubProfile() });
	UStaticMeshComponent* Body = Enemy ? FindObject<UStaticMeshComponent>(Enemy, TEXT("BodyMesh")) : nullptr;
	if (!TestNotNull("enemy spawned", Enemy) || !TestNotNull("the enemy has a BodyMesh", Body))
	{
		Env.Teardown();
		return true;
	}
	Body->SetStaticMesh(NewObject<UStaticMesh>(GetTransientPackage()));
	TestTrue("a living enemy shows its body", Enemy->HasVisibleMesh());

	Enemy->EnterDeathState();
	TestTrue("the enemy is dead", Enemy->IsDead());
	TestFalse("a dead enemy no longer collides", Enemy->GetActorEnableCollision());
	TestFalse("a dead enemy shows no body", Enemy->HasVisibleMesh());

	Enemy->ResetForArena();
	TestFalse("the reset enemy is alive", Enemy->IsDead());
	TestTrue("the reset enemy collides again", Enemy->GetActorEnableCollision());
	TestTrue("the reset enemy shows its body again", Enemy->HasVisibleMesh());

	Env.Teardown();
	return true;
}

// ---------------------------------------------------------------------------
// 5. Mordecai.Recovery.Enemy.NoSilentAttackLoop (AC-084.4)
// An enemy whose melee attack has no profile does not cycle Attack -> Recover: it logs once and stays in
// Approach (chase) at the player.
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_Recovery_Enemy_NoSilentAttackLoop,
	"Mordecai.Recovery.Enemy.NoSilentAttackLoop",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_Recovery_Enemy_NoSilentAttackLoop::RunTest(const FString& Parameters)
{
	FRecoveryTestEnv Env;
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
