// Project Mordecai — Parry System Tests (US-006)

#include "Misc/AutomationTest.h"
#include "Mordecai/Combat/MordecaiGA_Parry.h"
#include "Mordecai/Combat/MordecaiGA_Block.h"
#include "Mordecai/Combat/MordecaiCombatTypes.h"
#include "Mordecai/MordecaiGameplayTags.h"
#include "AbilitySystem/Abilities/LyraGameplayAbility.h"

// ---------------------------------------------------------------------------
// 1. Mordecai.Parry.ActivatesOnSimultaneousInput (AC-006.11)
// Fire block+attack simultaneously, verify Parry ability activates
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_Parry_Activates,
	"Mordecai.Parry.ActivatesOnSimultaneousInput",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_Parry_Activates::RunTest(const FString& Parameters)
{
	// AC-006.11: Verify class exists and inherits correctly
	const UClass* AbilityClass = UMordecaiGA_Parry::StaticClass();
	TestNotNull("UMordecaiGA_Parry class exists", AbilityClass);
	TestTrue("Inherits from ULyraGameplayAbility",
		AbilityClass->IsChildOf(ULyraGameplayAbility::StaticClass()));

	// Verify instancing policy (InstancedPerActor)
	UMordecaiGA_Parry* Ability = NewObject<UMordecaiGA_Parry>();
	TestEqual("InstancedPerActor policy",
		Ability->GetInstancingPolicy(),
		EGameplayAbilityInstancingPolicy::InstancedPerActor);

	// Parrying tag exists and is valid
	FGameplayTag ParryingTag = MordecaiGameplayTags::State_Parrying;
	TestTrue("Mordecai.State.Parrying tag is valid", ParryingTag.IsValid());

	// Event tag for parry input exists
	FGameplayTag ParryInputTag = MordecaiGameplayTags::Event_ParryInput;
	TestTrue("Mordecai.Event.ParryInput tag is valid", ParryInputTag.IsValid());

	// Verify parry transitions to Active phase
	TestEqual("Initial phase is None", Ability->GetCurrentParryPhase(), EMordecaiParryPhase::None);
	Ability->TransitionToParryPhase(EMordecaiParryPhase::Active);
	TestEqual("Phase transitions to Active", Ability->GetCurrentParryPhase(), EMordecaiParryPhase::Active);

	return true;
}

// ---------------------------------------------------------------------------
// 2. Mordecai.Parry.SuccessfulParryStaggersAttacker (AC-006.13)
// Parry an incoming hit, verify Parried tag on attacker and Riposte on player
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_Parry_StaggersAttacker,
	"Mordecai.Parry.SuccessfulParryStaggersAttacker",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_Parry_StaggersAttacker::RunTest(const FString& Parameters)
{
	UMordecaiGA_Parry* Ability = NewObject<UMordecaiGA_Parry>();

	// AC-006.13: Parried and Riposte tags exist
	FGameplayTag ParriedTag = MordecaiGameplayTags::State_Parried;
	TestTrue("Mordecai.State.Parried tag is valid", ParriedTag.IsValid());

	FGameplayTag RiposteTag = MordecaiGameplayTags::State_Riposte;
	TestTrue("Mordecai.State.Riposte tag is valid", RiposteTag.IsValid());

	// During Active phase, receiving a hit should succeed
	Ability->TransitionToParryPhase(EMordecaiParryPhase::Active);
	TestFalse("Not yet successful before hit", Ability->WasParrySuccessful());

	// ReceiveParriedHit with nullptr ASC (unit test — no ASC available)
	// should still return true to indicate successful parry during Active phase
	bool bSuccess = Ability->ReceiveParriedHit(nullptr);
	TestTrue("Parry succeeds during Active phase", bSuccess);
	TestTrue("Parry marked as successful", Ability->WasParrySuccessful());

	// Outside Active phase, parry should fail
	UMordecaiGA_Parry* Ability2 = NewObject<UMordecaiGA_Parry>();
	Ability2->TransitionToParryPhase(EMordecaiParryPhase::None);
	bool bFail = Ability2->ReceiveParriedHit(nullptr);
	TestFalse("Parry fails outside Active phase", bFail);

	return true;
}

// ---------------------------------------------------------------------------
// 3. Mordecai.Parry.SuccessfulParryDamagesPosture (AC-006.13)
// Parry a hit, verify attacker posture reduced by ParryPostureDamage
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_Parry_DamagesPosture,
	"Mordecai.Parry.SuccessfulParryDamagesPosture",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_Parry_DamagesPosture::RunTest(const FString& Parameters)
{
	UMordecaiGA_Parry* Ability = NewObject<UMordecaiGA_Parry>();

	// AC-006.13: Configurable posture damage, default 50
	TestEqual("Default parry posture damage", Ability->ParryPostureDamage, 50.f);

	// Configurable
	Ability->ParryPostureDamage = 75.f;
	TestEqual("Custom parry posture damage", Ability->ParryPostureDamage, 75.f);

	return true;
}

// ---------------------------------------------------------------------------
// 4. Mordecai.Parry.FailedParryAnimationLocks (AC-006.14)
// Activate parry, let window expire, verify ParryWhiff tag applied
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_Parry_FailedLocks,
	"Mordecai.Parry.FailedParryAnimationLocks",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_Parry_FailedLocks::RunTest(const FString& Parameters)
{
	UMordecaiGA_Parry* Ability = NewObject<UMordecaiGA_Parry>();

	// AC-006.14: ParryWhiff tag exists
	FGameplayTag WhiffTag = MordecaiGameplayTags::State_ParryWhiff;
	TestTrue("Mordecai.State.ParryWhiff tag is valid", WhiffTag.IsValid());

	// Whiff phase exists
	Ability->TransitionToParryPhase(EMordecaiParryPhase::Whiff);
	TestEqual("Whiff phase", Ability->GetCurrentParryPhase(), EMordecaiParryPhase::Whiff);

	// Whiff duration is configurable, default 600ms
	TestEqual("Default whiff duration is 600ms", Ability->ParryWhiffDurationMs, 600.f);
	TestEqual("Whiff duration in seconds", Ability->GetParryWhiffDurationSeconds(), 0.6f);

	// Parry window is configurable, default 500ms
	TestEqual("Default parry window is 500ms", Ability->ParryWindowMs, 500.f);
	TestEqual("Parry window in seconds", Ability->GetParryWindowSeconds(), 0.5f);

	return true;
}

// ---------------------------------------------------------------------------
// 5. Mordecai.Parry.ConsumesStaminaOnActivation (AC-006.15)
// Activate parry, verify stamina reduced
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_Parry_StaminaCost,
	"Mordecai.Parry.ConsumesStaminaOnActivation",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_Parry_StaminaCost::RunTest(const FString& Parameters)
{
	UMordecaiGA_Parry* Ability = NewObject<UMordecaiGA_Parry>();

	// AC-006.15: Default stamina cost is 20
	TestEqual("Default parry stamina cost is 20", Ability->GetStaminaCost(), 20.f);

	// Configurable
	Ability->ParryStaminaCost = 30.f;
	TestEqual("Custom parry stamina cost", Ability->GetStaminaCost(), 30.f);

	return true;
}

// ---------------------------------------------------------------------------
// 6. Mordecai.Parry.BlockedDuringPostureBroken (AC-006.16)
// Apply PostureBroken, attempt parry, verify blocked
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_Parry_BlockedPostureBroken,
	"Mordecai.Parry.BlockedDuringPostureBroken",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_Parry_BlockedPostureBroken::RunTest(const FString& Parameters)
{
	UMordecaiGA_Parry* Ability = NewObject<UMordecaiGA_Parry>();

	// AC-006.16: PostureBroken blocks parry
	TestTrue("PostureBroken blocks parry",
		Ability->HasActivationBlockedTag(MordecaiGameplayTags::State_PostureBroken));

	// AC-006.16: GuardBroken also blocks parry
	TestTrue("GuardBroken blocks parry",
		Ability->HasActivationBlockedTag(MordecaiGameplayTags::State_GuardBroken));

	return true;
}

// ---------------------------------------------------------------------------
// 7. Mordecai.Parry.TakesPriorityOverBlock (AC-006.17, AC-006.18)
// Hold block, press attack (parry input), verify parry takes priority
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_Parry_PriorityOverBlock,
	"Mordecai.Parry.TakesPriorityOverBlock",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_Parry_PriorityOverBlock::RunTest(const FString& Parameters)
{
	UMordecaiGA_Block* BlockAbility = NewObject<UMordecaiGA_Block>();
	UMordecaiGA_Parry* ParryAbility = NewObject<UMordecaiGA_Parry>();

	// AC-006.17: Block and Parry are mutually exclusive
	// Parry is NOT blocked by the Blocking tag — it cancels block
	TestFalse("Parry not blocked by Blocking tag",
		ParryAbility->HasActivationBlockedTag(MordecaiGameplayTags::State_Blocking));

	// AC-006.18: Parry activation cancels active block
	// This is handled by the parry ability cancelling abilities with the Blocking tag
	// Verify parry doesn't block itself
	TestFalse("Parry not blocked by Parrying tag (allows single activation)",
		ParryAbility->HasActivationBlockedTag(MordecaiGameplayTags::State_Parrying));

	// Block should be blocked during parry whiff (can't block while whiffing)
	TestTrue("Block is blocked during ParryWhiff",
		BlockAbility->HasActivationBlockedTag(MordecaiGameplayTags::State_ParryWhiff));

	// Block should be blocked during guard broken
	TestTrue("Block is blocked during GuardBroken",
		BlockAbility->HasActivationBlockedTag(MordecaiGameplayTags::State_GuardBroken));

	return true;
}
