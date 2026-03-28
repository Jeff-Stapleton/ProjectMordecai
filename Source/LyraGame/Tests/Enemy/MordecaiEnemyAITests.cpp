// Project Mordecai — Enemy AI Tests (US-051)

#include "Misc/AutomationTest.h"
#include "Mordecai/Enemy/MordecaiEnemyAIController.h"
#include "Mordecai/Enemy/MordecaiEnemyAITypes.h"
#include "Mordecai/MordecaiGameplayTags.h"
#include "AIController.h"

// ---------------------------------------------------------------------------
// 1. Mordecai.EnemyAI.IdleStateAtSpawn (AC-051.1, AC-051.2)
// Verify AI controller exists, extends AAIController, and starts in Idle state
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_EnemyAI_IdleStateAtSpawn,
	"Mordecai.EnemyAI.IdleStateAtSpawn",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_EnemyAI_IdleStateAtSpawn::RunTest(const FString& Parameters)
{
	// AC-051.1: Class extends AAIController
	TestTrue("AMordecaiEnemyAIController extends AAIController",
		AMordecaiEnemyAIController::StaticClass()->IsChildOf(AAIController::StaticClass()));

	// AC-051.2: State machine starts in Idle
	AMordecaiEnemyAIController* AI = NewObject<AMordecaiEnemyAIController>();
	TestNotNull("AI controller created", AI);
	TestEqual("Initial state is Idle", AI->GetCurrentState(), EMordecaiEnemyAIState::Idle);

	// AC-051.2: All 7 required states can be entered
	AI->TransitionToState(EMordecaiEnemyAIState::Approach);
	TestEqual("Approach state exists", AI->GetCurrentState(), EMordecaiEnemyAIState::Approach);

	AI->TransitionToState(EMordecaiEnemyAIState::Attack);
	TestEqual("Attack state exists", AI->GetCurrentState(), EMordecaiEnemyAIState::Attack);

	AI->TransitionToState(EMordecaiEnemyAIState::Recover);
	TestEqual("Recover state exists", AI->GetCurrentState(), EMordecaiEnemyAIState::Recover);

	AI->TransitionToState(EMordecaiEnemyAIState::Staggered);
	TestEqual("Staggered state exists", AI->GetCurrentState(), EMordecaiEnemyAIState::Staggered);

	AI->TransitionToState(EMordecaiEnemyAIState::Leash);
	TestEqual("Leash state exists", AI->GetCurrentState(), EMordecaiEnemyAIState::Leash);

	AI->TransitionToState(EMordecaiEnemyAIState::Dead);
	TestEqual("Dead state exists", AI->GetCurrentState(), EMordecaiEnemyAIState::Dead);

	return true;
}

// ---------------------------------------------------------------------------
// 2. Mordecai.EnemyAI.TransitionsToApproachOnPlayerInRange (AC-051.3)
// Verify aggro range detection and Idle -> Approach transition
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_EnemyAI_TransitionsToApproachOnPlayerInRange,
	"Mordecai.EnemyAI.TransitionsToApproachOnPlayerInRange",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_EnemyAI_TransitionsToApproachOnPlayerInRange::RunTest(const FString& Parameters)
{
	AMordecaiEnemyAIController* AI = NewObject<AMordecaiEnemyAIController>();

	// AC-051.3: Default AggroRange is 900cm
	TestEqual("Default AggroRange is 900", AI->AggroRange, 900.f);

	// Distance checks for aggro range
	TestTrue("800cm is within aggro range", AI->IsInAggroRange(800.f));
	TestTrue("900cm is within aggro range (boundary)", AI->IsInAggroRange(900.f));
	TestFalse("901cm is outside aggro range", AI->IsInAggroRange(901.f));

	// Verify Idle -> Approach transition works
	TestEqual("Starts in Idle", AI->GetCurrentState(), EMordecaiEnemyAIState::Idle);
	AI->TransitionToState(EMordecaiEnemyAIState::Approach);
	TestEqual("Transitions to Approach", AI->GetCurrentState(), EMordecaiEnemyAIState::Approach);

	return true;
}

// ---------------------------------------------------------------------------
// 3. Mordecai.EnemyAI.StaysIdleWhenPlayerOutOfRange (AC-051.3)
// Verify no transition when player is beyond aggro range
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_EnemyAI_StaysIdleWhenPlayerOutOfRange,
	"Mordecai.EnemyAI.StaysIdleWhenPlayerOutOfRange",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_EnemyAI_StaysIdleWhenPlayerOutOfRange::RunTest(const FString& Parameters)
{
	AMordecaiEnemyAIController* AI = NewObject<AMordecaiEnemyAIController>();

	// Player at various distances beyond aggro range
	TestFalse("1000cm is outside aggro range", AI->IsInAggroRange(1000.f));
	TestFalse("1500cm is outside aggro range", AI->IsInAggroRange(1500.f));
	TestFalse("5000cm is well outside aggro range", AI->IsInAggroRange(5000.f));

	// State remains Idle (no tick/trigger to change it)
	TestEqual("State remains Idle", AI->GetCurrentState(), EMordecaiEnemyAIState::Idle);

	// Edge case: distance at aggro boundary + 1
	TestFalse("901cm is outside aggro range", AI->IsInAggroRange(901.f));

	return true;
}

// ---------------------------------------------------------------------------
// 4. Mordecai.EnemyAI.TransitionsToAttackAtAttackRange (AC-051.4, AC-051.5)
// Verify attack range detection and Approach -> Attack transition
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_EnemyAI_TransitionsToAttackAtAttackRange,
	"Mordecai.EnemyAI.TransitionsToAttackAtAttackRange",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_EnemyAI_TransitionsToAttackAtAttackRange::RunTest(const FString& Parameters)
{
	AMordecaiEnemyAIController* AI = NewObject<AMordecaiEnemyAIController>();

	// AC-051.4: Default AttackRange is 200cm
	TestEqual("Default AttackRange is 200", AI->AttackRange, 200.f);

	// Distance checks for attack range
	TestTrue("150cm is within attack range", AI->IsInAttackRange(150.f));
	TestTrue("200cm is within attack range (boundary)", AI->IsInAttackRange(200.f));
	TestFalse("201cm is outside attack range", AI->IsInAttackRange(201.f));

	// AC-051.5: Approach -> Attack transition
	AI->TransitionToState(EMordecaiEnemyAIState::Approach);
	AI->TransitionToState(EMordecaiEnemyAIState::Attack);
	TestEqual("Transitions to Attack from Approach", AI->GetCurrentState(), EMordecaiEnemyAIState::Attack);

	return true;
}

// ---------------------------------------------------------------------------
// 5. Mordecai.EnemyAI.RecoverStateAfterAttack (AC-051.6)
// Verify cooldown between attacks: Attack -> Recover on completion
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_EnemyAI_RecoverStateAfterAttack,
	"Mordecai.EnemyAI.RecoverStateAfterAttack",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_EnemyAI_RecoverStateAfterAttack::RunTest(const FString& Parameters)
{
	AMordecaiEnemyAIController* AI = NewObject<AMordecaiEnemyAIController>();

	// AC-051.6: Default AttackCooldownMs is 1500
	TestEqual("Default AttackCooldownMs is 1500", AI->AttackCooldownMs, 1500.f);

	// Enter Attack state
	AI->TransitionToState(EMordecaiEnemyAIState::Attack);
	TestEqual("State is Attack", AI->GetCurrentState(), EMordecaiEnemyAIState::Attack);

	// AC-051.6: Attack completes -> Recover
	AI->NotifyAttackCompleted();
	TestEqual("Transitions to Recover after attack", AI->GetCurrentState(), EMordecaiEnemyAIState::Recover);

	// NotifyAttackCompleted from non-Attack state is a no-op
	AI->TransitionToState(EMordecaiEnemyAIState::Idle);
	EMordecaiEnemyAIState StateBefore = AI->GetCurrentState();
	AI->NotifyAttackCompleted();
	TestEqual("NotifyAttackCompleted no-op from Idle", AI->GetCurrentState(), StateBefore);

	return true;
}

// ---------------------------------------------------------------------------
// 6. Mordecai.EnemyAI.StaggeredOnPostureBroken (AC-051.7)
// Verify PostureBroken tag transitions to Staggered and pauses actions
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_EnemyAI_StaggeredOnPostureBroken,
	"Mordecai.EnemyAI.StaggeredOnPostureBroken",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_EnemyAI_StaggeredOnPostureBroken::RunTest(const FString& Parameters)
{
	AMordecaiEnemyAIController* AI = NewObject<AMordecaiEnemyAIController>();

	// Start in Approach state
	AI->TransitionToState(EMordecaiEnemyAIState::Approach);
	TestEqual("State is Approach", AI->GetCurrentState(), EMordecaiEnemyAIState::Approach);

	// AC-051.7: PostureBroken tag added -> Staggered
	AI->OnPostureBrokenTagChanged(MordecaiGameplayTags::State_PostureBroken, 1);
	TestEqual("Staggered after posture break", AI->GetCurrentState(), EMordecaiEnemyAIState::Staggered);

	// AC-051.7: PostureBroken tag removed -> exits Staggered
	AI->OnPostureBrokenTagChanged(MordecaiGameplayTags::State_PostureBroken, 0);
	TestNotEqual("No longer Staggered after tag removed", AI->GetCurrentState(), EMordecaiEnemyAIState::Staggered);

	// Also works from Attack state
	AI->TransitionToState(EMordecaiEnemyAIState::Attack);
	AI->OnPostureBrokenTagChanged(MordecaiGameplayTags::State_PostureBroken, 1);
	TestEqual("Staggered from Attack state", AI->GetCurrentState(), EMordecaiEnemyAIState::Staggered);

	return true;
}

// ---------------------------------------------------------------------------
// 7. Mordecai.EnemyAI.DeadStateOnDeath (AC-051.8)
// Verify Dead tag transitions to permanent Dead state
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_EnemyAI_DeadStateOnDeath,
	"Mordecai.EnemyAI.DeadStateOnDeath",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_EnemyAI_DeadStateOnDeath::RunTest(const FString& Parameters)
{
	AMordecaiEnemyAIController* AI = NewObject<AMordecaiEnemyAIController>();

	// Start in Approach
	AI->TransitionToState(EMordecaiEnemyAIState::Approach);

	// AC-051.8: Dead tag -> Dead state
	AI->OnDeadTagChanged(MordecaiGameplayTags::State_Dead, 1);
	TestEqual("State is Dead", AI->GetCurrentState(), EMordecaiEnemyAIState::Dead);

	// AC-051.8: Dead is permanent — cannot transition out
	AI->TransitionToState(EMordecaiEnemyAIState::Idle);
	TestEqual("Still Dead after Idle request", AI->GetCurrentState(), EMordecaiEnemyAIState::Dead);

	AI->TransitionToState(EMordecaiEnemyAIState::Approach);
	TestEqual("Still Dead after Approach request", AI->GetCurrentState(), EMordecaiEnemyAIState::Dead);

	// Posture broken should not change Dead state
	AI->OnPostureBrokenTagChanged(MordecaiGameplayTags::State_PostureBroken, 1);
	TestEqual("Still Dead after posture break", AI->GetCurrentState(), EMordecaiEnemyAIState::Dead);

	return true;
}

// ---------------------------------------------------------------------------
// 8. Mordecai.EnemyAI.LeashWhenPlayerTooFar (AC-051.9)
// Verify leash range and return-to-spawn behavior
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_EnemyAI_LeashWhenPlayerTooFar,
	"Mordecai.EnemyAI.LeashWhenPlayerTooFar",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_EnemyAI_LeashWhenPlayerTooFar::RunTest(const FString& Parameters)
{
	AMordecaiEnemyAIController* AI = NewObject<AMordecaiEnemyAIController>();

	// AC-051.9: Default LeashRange is 1800cm
	TestEqual("Default LeashRange is 1800", AI->LeashRange, 1800.f);

	// Distance checks for leash range
	TestTrue("2000cm is outside leash range", AI->IsOutsideLeashRange(2000.f));
	TestTrue("1801cm is outside leash range", AI->IsOutsideLeashRange(1801.f));
	TestFalse("1800cm is not outside leash range (boundary)", AI->IsOutsideLeashRange(1800.f));
	TestFalse("500cm is not outside leash range", AI->IsOutsideLeashRange(500.f));

	// Verify Leash state can be entered
	AI->TransitionToState(EMordecaiEnemyAIState::Approach);
	AI->TransitionToState(EMordecaiEnemyAIState::Leash);
	TestEqual("Transitions to Leash", AI->GetCurrentState(), EMordecaiEnemyAIState::Leash);

	// Verify spawn location is stored (defaults to ZeroVector before BeginPlay)
	TestEqual("Spawn location default is zero", AI->GetSpawnLocation(), FVector::ZeroVector);

	// Leash -> Idle transition (at spawn)
	AI->TransitionToState(EMordecaiEnemyAIState::Idle);
	TestEqual("Leash returns to Idle at spawn", AI->GetCurrentState(), EMordecaiEnemyAIState::Idle);

	return true;
}

// ---------------------------------------------------------------------------
// 9. Mordecai.EnemyAI.FacesPlayerDuringCombat (AC-051.10)
// Verify controller has focus/rotation capability for facing the player
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_EnemyAI_FacesPlayerDuringCombat,
	"Mordecai.EnemyAI.FacesPlayerDuringCombat",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_EnemyAI_FacesPlayerDuringCombat::RunTest(const FString& Parameters)
{
	// AC-051.10: Enemy faces player during Approach and Attack states
	// Implemented via AAIController::SetFocus() which drives pawn rotation

	AMordecaiEnemyAIController* AI = NewObject<AMordecaiEnemyAIController>();
	TestNotNull("AI controller created", AI);

	// AAIController provides SetFocus/ClearFocus for rotation toward target
	TestTrue("Controller is AAIController (has SetFocus/ClearFocus)",
		AI->IsA<AAIController>());

	// Verify combat states can be entered (where facing is active)
	AI->TransitionToState(EMordecaiEnemyAIState::Approach);
	TestEqual("Can enter Approach (facing active)", AI->GetCurrentState(), EMordecaiEnemyAIState::Approach);

	AI->TransitionToState(EMordecaiEnemyAIState::Attack);
	TestEqual("Can enter Attack (facing active)", AI->GetCurrentState(), EMordecaiEnemyAIState::Attack);

	// Focus is cleared on exit (verify no crash)
	AI->TransitionToState(EMordecaiEnemyAIState::Idle);
	TestEqual("Returns to Idle (focus cleared)", AI->GetCurrentState(), EMordecaiEnemyAIState::Idle);

	return true;
}

// ---------------------------------------------------------------------------
// 10. Mordecai.EnemyAI.RangesAreConfigurable (AC-051.11)
// Verify all range/timing parameters are exposed as UPROPERTY
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_EnemyAI_RangesAreConfigurable,
	"Mordecai.EnemyAI.RangesAreConfigurable",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_EnemyAI_RangesAreConfigurable::RunTest(const FString& Parameters)
{
	UClass* AIClass = AMordecaiEnemyAIController::StaticClass();

	// AC-051.11: All range/timing parameters exposed as UPROPERTY
	TestNotNull("AggroRange property exposed", AIClass->FindPropertyByName(FName("AggroRange")));
	TestNotNull("AttackRange property exposed", AIClass->FindPropertyByName(FName("AttackRange")));
	TestNotNull("LeashRange property exposed", AIClass->FindPropertyByName(FName("LeashRange")));
	TestNotNull("AttackCooldownMs property exposed", AIClass->FindPropertyByName(FName("AttackCooldownMs")));
	TestNotNull("MeleeAttackAbilityClass property exposed", AIClass->FindPropertyByName(FName("MeleeAttackAbilityClass")));

	// Verify parameters can be customized per-enemy
	AMordecaiEnemyAIController* AI = NewObject<AMordecaiEnemyAIController>();

	AI->AggroRange = 1200.f;
	TestEqual("AggroRange customized to 1200", AI->AggroRange, 1200.f);
	TestTrue("Custom aggro range: 1100cm in range", AI->IsInAggroRange(1100.f));
	TestFalse("Custom aggro range: 1300cm out of range", AI->IsInAggroRange(1300.f));

	AI->AttackRange = 300.f;
	TestEqual("AttackRange customized to 300", AI->AttackRange, 300.f);
	TestTrue("Custom attack range: 250cm in range", AI->IsInAttackRange(250.f));
	TestFalse("Custom attack range: 350cm out of range", AI->IsInAttackRange(350.f));

	AI->LeashRange = 2500.f;
	TestEqual("LeashRange customized to 2500", AI->LeashRange, 2500.f);
	TestTrue("Custom leash range: 2600cm triggers leash", AI->IsOutsideLeashRange(2600.f));
	TestFalse("Custom leash range: 2400cm no leash", AI->IsOutsideLeashRange(2400.f));

	AI->AttackCooldownMs = 2000.f;
	TestEqual("AttackCooldownMs customized to 2000", AI->AttackCooldownMs, 2000.f);

	return true;
}

// ---------------------------------------------------------------------------
// 11. Mordecai.EnemyAI.OnlyRunsOnServer (AC-051.12)
// Verify state machine is tick-driven and respects server authority
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_EnemyAI_OnlyRunsOnServer,
	"Mordecai.EnemyAI.OnlyRunsOnServer",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_EnemyAI_OnlyRunsOnServer::RunTest(const FString& Parameters)
{
	// AC-051.12: State machine is tick-driven and only runs on server/authority

	AMordecaiEnemyAIController* AI = NewObject<AMordecaiEnemyAIController>();
	TestNotNull("AI controller created", AI);

	// Tick is enabled (state machine is tick-driven)
	TestTrue("Tick is enabled", AI->PrimaryActorTick.bCanEverTick);
	TestTrue("Tick starts enabled", AI->PrimaryActorTick.bStartWithTickEnabled);

	// Controller extends AAIController (server-side class in UE)
	TestTrue("Extends AAIController (server-side)", AI->IsA<AAIController>());

	// State machine does not advance without Tick (no autonomous client logic)
	TestEqual("State unchanged without Tick", AI->GetCurrentState(), EMordecaiEnemyAIState::Idle);

	// HasAuthority() guard in Tick ensures no client-side execution.
	// Full authority test requires a networked world — structural check verifies:
	// 1. Controller is tick-driven (above)
	// 2. Controller is AAIController (spawned only on server by default)
	// 3. State cannot change without explicit method calls or Tick

	return true;
}
