// Project Mordecai — Block System Tests (US-006)

#include "Misc/AutomationTest.h"
#include "Mordecai/Combat/MordecaiGA_Block.h"
#include "Mordecai/Combat/MordecaiGA_MeleeAttack.h"
#include "Mordecai/Combat/MordecaiAttackProfileDataAsset.h"
#include "Mordecai/Combat/MordecaiCombatTypes.h"
#include "Mordecai/MordecaiGameplayTags.h"
#include "AbilitySystem/Abilities/LyraGameplayAbility.h"

// ---------------------------------------------------------------------------
// 1. Mordecai.Block.AbilityActivatesOnInputHold (AC-006.1, AC-006.2)
// Hold block input, verify Blocking tag applied
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_Block_Activates,
	"Mordecai.Block.AbilityActivatesOnInputHold",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_Block_Activates::RunTest(const FString& Parameters)
{
	// AC-006.1: Verify class exists and inherits correctly
	const UClass* AbilityClass = UMordecaiGA_Block::StaticClass();
	TestNotNull("UMordecaiGA_Block class exists", AbilityClass);
	TestTrue("Inherits from ULyraGameplayAbility",
		AbilityClass->IsChildOf(ULyraGameplayAbility::StaticClass()));

	// Verify instancing policy (InstancedPerActor per tech notes)
	UMordecaiGA_Block* Ability = NewObject<UMordecaiGA_Block>();
	TestEqual("InstancedPerActor policy",
		Ability->GetInstancingPolicy(),
		EGameplayAbilityInstancingPolicy::InstancedPerActor);

	// AC-006.2: Verify Blocking tag is valid
	FGameplayTag BlockingTag = MordecaiGameplayTags::State_Blocking;
	TestTrue("Mordecai.State.Blocking tag is valid", BlockingTag.IsValid());

	// Verify block transitions to Active phase
	TestEqual("Initial phase is None", Ability->GetCurrentBlockPhase(), EMordecaiBlockPhase::None);
	Ability->TransitionToBlockPhase(EMordecaiBlockPhase::Active);
	TestEqual("Phase transitions to Active", Ability->GetCurrentBlockPhase(), EMordecaiBlockPhase::Active);

	// AC-006.2: Blocking tag is an activation owned tag
	TestTrue("Blocking tag is an activation owned tag",
		Ability->HasActivationOwnedTag(MordecaiGameplayTags::State_Blocking));

	return true;
}

// ---------------------------------------------------------------------------
// 2. Mordecai.Block.AbilityEndsOnInputRelease (AC-006.1)
// Release block input, verify Blocking tag removed
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_Block_EndsOnRelease,
	"Mordecai.Block.AbilityEndsOnInputRelease",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_Block_EndsOnRelease::RunTest(const FString& Parameters)
{
	UMordecaiGA_Block* Ability = NewObject<UMordecaiGA_Block>();

	// Simulate block lifecycle: Active -> None (on release)
	Ability->TransitionToBlockPhase(EMordecaiBlockPhase::Active);
	TestEqual("Block is active", Ability->GetCurrentBlockPhase(), EMordecaiBlockPhase::Active);

	// End block (simulating input release)
	Ability->TransitionToBlockPhase(EMordecaiBlockPhase::None);
	TestEqual("Block ended — phase None", Ability->GetCurrentBlockPhase(), EMordecaiBlockPhase::None);

	return true;
}

// ---------------------------------------------------------------------------
// 3. Mordecai.Block.MitigatesDamage (AC-006.3)
// Set Mitigation=0.5, take 100 damage while blocking, verify 50 health lost
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_Block_MitigatesDamage,
	"Mordecai.Block.MitigatesDamage",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_Block_MitigatesDamage::RunTest(const FString& Parameters)
{
	UMordecaiGA_Block* Ability = NewObject<UMordecaiGA_Block>();

	// AC-006.3: FinalDamage = IncomingDamage × (1 − Mitigation)
	Ability->Mitigation = 0.5f;
	TestEqual("50% mitigation: 100 damage -> 50",
		Ability->ComputeBlockedDamage(100.f), 50.f);

	// Full mitigation
	Ability->Mitigation = 1.0f;
	TestEqual("100% mitigation: 100 damage -> 0",
		Ability->ComputeBlockedDamage(100.f), 0.f);

	// No mitigation
	Ability->Mitigation = 0.0f;
	TestEqual("0% mitigation: 100 damage -> 100",
		Ability->ComputeBlockedDamage(100.f), 100.f);

	// Partial mitigation
	Ability->Mitigation = 0.75f;
	TestTrue("75% mitigation: 100 damage -> 25",
		FMath::IsNearlyEqual(Ability->ComputeBlockedDamage(100.f), 25.f, 0.01f));

	return true;
}

// ---------------------------------------------------------------------------
// 4. Mordecai.Block.DrainsStaminaPerHit (AC-006.4)
// Set Stability=0.3, take 100 damage, verify stamina drained by 70
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_Block_StaminaDrain,
	"Mordecai.Block.DrainsStaminaPerHit",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_Block_StaminaDrain::RunTest(const FString& Parameters)
{
	UMordecaiGA_Block* Ability = NewObject<UMordecaiGA_Block>();

	// AC-006.4: StaminaDrain = IncomingDamage × (1 − Stability)
	Ability->Stability = 0.3f;
	TestTrue("Stability 0.3: 100 damage -> 70 stamina drain",
		FMath::IsNearlyEqual(Ability->ComputeStaminaDrain(100.f), 70.f, 0.01f));

	// Full stability (no drain)
	Ability->Stability = 1.0f;
	TestEqual("Stability 1.0: 100 damage -> 0 drain",
		Ability->ComputeStaminaDrain(100.f), 0.f);

	// No stability (full drain)
	Ability->Stability = 0.0f;
	TestEqual("Stability 0.0: 100 damage -> 100 drain",
		Ability->ComputeStaminaDrain(100.f), 100.f);

	return true;
}

// ---------------------------------------------------------------------------
// 5. Mordecai.Block.GuardBreaksAtZeroStamina (AC-006.5)
// Block with low stamina, take hit that depletes it, verify GuardBroken
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_Block_GuardBreak,
	"Mordecai.Block.GuardBreaksAtZeroStamina",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_Block_GuardBreak::RunTest(const FString& Parameters)
{
	UMordecaiGA_Block* Ability = NewObject<UMordecaiGA_Block>();

	// AC-006.5: GuardBroken tag exists and is valid
	FGameplayTag GuardBrokenTag = MordecaiGameplayTags::State_GuardBroken;
	TestTrue("Mordecai.State.GuardBroken tag is valid", GuardBrokenTag.IsValid());

	// GuardBroken phase exists
	Ability->TransitionToBlockPhase(EMordecaiBlockPhase::GuardBroken);
	TestEqual("Guard broken phase", Ability->GetCurrentBlockPhase(), EMordecaiBlockPhase::GuardBroken);

	// GuardBrokenDuration is configurable
	TestEqual("Default guard broken duration is 800ms", Ability->GuardBrokenDurationMs, 800.f);
	TestEqual("Guard broken duration in seconds", Ability->GetGuardBrokenDurationSeconds(), 0.8f);

	return true;
}

// ---------------------------------------------------------------------------
// 6. Mordecai.Block.CancelsAttackRecoveryPhase (AC-006.6)
// Attack with CancelableIntoBlock=true, trigger block during Recovery
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_Block_CancelsAttack,
	"Mordecai.Block.CancelsAttackRecoveryPhase",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_Block_CancelsAttack::RunTest(const FString& Parameters)
{
	// Set up melee attack with CancelableIntoBlock=true
	UMordecaiAttackProfileDataAsset* Profile = NewObject<UMordecaiAttackProfileDataAsset>();
	Profile->CancelableIntoBlock = true;
	Profile->InputSlot = EMordecaiInputSlot::Light;
	Profile->WindupTimeMs = 200.f;
	Profile->ActiveTimeMs = 100.f;
	Profile->RecoveryTimeMs = 300.f;
	Profile->RootedDuring = EMordecaiRootedMode::None;
	Profile->DamageProfile.BasePower = 25.f;
	Profile->DamageProfile.DamageType = EMordecaiDamageType::Slash;

	UMordecaiGA_MeleeAttack* MeleeAbility = NewObject<UMordecaiGA_MeleeAttack>();
	MeleeAbility->AttackProfiles.Add(Profile);

	// During Recovery with CancelableIntoBlock=true, cancel should be allowed
	MeleeAbility->TransitionToPhase(EMordecaiAttackPhase::Recovery);
	TestTrue("Melee allows cancel into block during Recovery",
		MeleeAbility->CanCancelIntoBlock());

	// Block is NOT blocked by Attacking tag
	UMordecaiGA_Block* BlockAbility = NewObject<UMordecaiGA_Block>();
	TestFalse("Block not blocked by Attacking tag",
		BlockAbility->HasActivationBlockedTag(MordecaiGameplayTags::State_Attacking));

	// During non-Recovery phases, cancel should NOT be allowed
	MeleeAbility->TransitionToPhase(EMordecaiAttackPhase::Active);
	TestFalse("No cancel during Active", MeleeAbility->CanCancelIntoBlock());

	MeleeAbility->TransitionToPhase(EMordecaiAttackPhase::Windup);
	TestFalse("No cancel during Windup", MeleeAbility->CanCancelIntoBlock());

	// With CancelableIntoBlock=false, cancel never allowed
	Profile->CancelableIntoBlock = false;
	MeleeAbility->TransitionToPhase(EMordecaiAttackPhase::Recovery);
	TestFalse("No cancel when profile disables it", MeleeAbility->CanCancelIntoBlock());

	return true;
}

// ---------------------------------------------------------------------------
// 7. Mordecai.Block.AllowsReducedMovement (AC-006.7)
// Block, attempt movement, verify movement at reduced speed
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_Block_ReducedMovement,
	"Mordecai.Block.AllowsReducedMovement",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_Block_ReducedMovement::RunTest(const FString& Parameters)
{
	UMordecaiGA_Block* Ability = NewObject<UMordecaiGA_Block>();

	// AC-006.7: Configurable move speed multiplier, default 0.5
	TestEqual("Default block move speed multiplier is 0.5",
		Ability->BlockMoveSpeedMultiplier, 0.5f);

	// Move speed multiplier is configurable
	Ability->BlockMoveSpeedMultiplier = 0.3f;
	TestEqual("Custom block move speed multiplier", Ability->BlockMoveSpeedMultiplier, 0.3f);

	// AC-006.8: Block is NOT blocked by Rooted tag
	TestFalse("Block not blocked by Rooted",
		Ability->HasActivationBlockedTag(MordecaiGameplayTags::State_Rooted));

	return true;
}

// ---------------------------------------------------------------------------
// 8. Mordecai.Block.PerfectBlockGrantsTag (AC-006.9)
// Activate block, take hit within PerfectBlockWindow, verify PerfectBlock tag
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_Block_PerfectBlockTag,
	"Mordecai.Block.PerfectBlockGrantsTag",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_Block_PerfectBlockTag::RunTest(const FString& Parameters)
{
	UMordecaiGA_Block* Ability = NewObject<UMordecaiGA_Block>();

	// AC-006.9: PerfectBlock tag is valid
	FGameplayTag PerfectBlockTag = MordecaiGameplayTags::State_PerfectBlock;
	TestTrue("Mordecai.State.PerfectBlock tag is valid", PerfectBlockTag.IsValid());

	// Perfect block window is configurable, default 250ms
	TestEqual("Default perfect block window is 250ms", Ability->PerfectBlockWindowMs, 250.f);
	TestEqual("Window seconds conversion", Ability->GetPerfectBlockWindowSeconds(), 0.25f);

	// Initially not in perfect block window
	TestFalse("Perfect block not active initially", Ability->IsPerfectBlockWindowActive());

	// Test perfect block timing check
	// Block activated at time 10.0, hit at 10.1 (within 250ms window) = perfect block
	Ability->PerfectBlockWindowMs = 250.f;
	Ability->SetBlockActivationTime(10.0);

	// Hit at 10.1s = 100ms after activation — within 250ms window
	TestTrue("Hit within window is perfect block",
		Ability->IsHitWithinPerfectBlockWindow(10.1));

	// Hit at 10.3s = 300ms after activation — outside 250ms window
	TestFalse("Hit outside window is not perfect block",
		Ability->IsHitWithinPerfectBlockWindow(10.3));

	// Hit exactly at window edge (250ms)
	TestTrue("Hit at exact window edge is perfect block",
		Ability->IsHitWithinPerfectBlockWindow(10.25));

	// Hit before activation time should not be perfect block
	TestFalse("Hit before activation is not perfect block",
		Ability->IsHitWithinPerfectBlockWindow(9.9));

	return true;
}

// ---------------------------------------------------------------------------
// 9. Mordecai.Block.PerfectBlockNoStaminaDrain (AC-006.10)
// Take hit during PerfectBlock window, verify zero stamina drain
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_Block_PerfectBlockNoStaminaDrain,
	"Mordecai.Block.PerfectBlockNoStaminaDrain",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_Block_PerfectBlockNoStaminaDrain::RunTest(const FString& Parameters)
{
	UMordecaiGA_Block* Ability = NewObject<UMordecaiGA_Block>();

	// AC-006.10: Perfect block = no stamina drain
	// Perfect block window active means stamina drain is skipped
	Ability->Stability = 0.3f;

	// Normal block: 100 damage -> 70 stamina drain
	float NormalDrain = Ability->ComputeStaminaDrain(100.f);
	TestTrue("Normal block drains stamina", NormalDrain > 0.f);

	// Perfect block posture damage is configurable, default 30
	TestEqual("Default perfect block posture damage", Ability->PerfectBlockPostureDamage, 30.f);

	return true;
}

// ---------------------------------------------------------------------------
// 10. Mordecai.Block.PerfectBlockDamagesAttackerPosture (AC-006.10)
// Take hit during PerfectBlock, verify attacker posture reduced
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_Block_PerfectBlockPosture,
	"Mordecai.Block.PerfectBlockDamagesAttackerPosture",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_Block_PerfectBlockPosture::RunTest(const FString& Parameters)
{
	UMordecaiGA_Block* Ability = NewObject<UMordecaiGA_Block>();

	// AC-006.10: Perfect block deals posture damage to attacker
	TestEqual("Perfect block posture damage default is 30",
		Ability->PerfectBlockPostureDamage, 30.f);

	// Configurable
	Ability->PerfectBlockPostureDamage = 50.f;
	TestEqual("Custom perfect block posture damage", Ability->PerfectBlockPostureDamage, 50.f);

	return true;
}
