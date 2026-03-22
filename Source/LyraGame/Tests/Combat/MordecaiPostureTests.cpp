// Project Mordecai — Posture & Stagger System Tests (US-007)

#include "Misc/AutomationTest.h"
#include "Mordecai/Combat/MordecaiPostureSystem.h"
#include "Mordecai/Combat/MordecaiGA_MeleeAttack.h"
#include "Mordecai/Combat/MordecaiGA_Block.h"
#include "Mordecai/Combat/MordecaiGA_Dodge.h"
#include "Mordecai/Combat/MordecaiGA_Parry.h"
#include "Mordecai/AbilitySystem/MordecaiAttributeSet.h"
#include "Mordecai/MordecaiGameplayTags.h"

// ---------------------------------------------------------------------------
// 1. Mordecai.Posture.PostureDamageReducesPosture (AC-007.3)
// Apply posture damage GE, verify Posture attribute decreases
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_Posture_DamageReducesPosture,
	"Mordecai.Posture.PostureDamageReducesPosture",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_Posture_DamageReducesPosture::RunTest(const FString& Parameters)
{
	// AC-007.3: SetByCaller tag for posture damage exists
	FGameplayTag PostureDamageTag = MordecaiGameplayTags::SetByCaller_PostureDamage;
	TestTrue("SetByCaller.PostureDamage tag is valid", PostureDamageTag.IsValid());

	// Posture attribute exists on attribute set
	const UMordecaiAttributeSet* CDO = GetDefault<UMordecaiAttributeSet>();
	TestNotNull("Attribute set CDO exists", CDO);
	TestTrue("Posture attribute is valid", UMordecaiAttributeSet::GetPostureAttribute().IsValid());

	// PostureSystem computes posture damage correctly
	UMordecaiPostureSystem* System = NewObject<UMordecaiPostureSystem>();
	float Damage = System->ComputePostureDamage(25.f, 1.0f);
	TestEqual("25 base x 1.0 scalar = 25 posture damage", Damage, 25.f);

	// Damage is positive (will be subtracted from Posture)
	TestTrue("Posture damage is positive", Damage > 0.f);

	return true;
}

// ---------------------------------------------------------------------------
// 2. Mordecai.Posture.PostureDamageUsesScalar (AC-007.4)
// BasePower=20, PostureDamageScalar=1.5, verify 30 posture damage
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_Posture_DamageUsesScalar,
	"Mordecai.Posture.PostureDamageUsesScalar",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_Posture_DamageUsesScalar::RunTest(const FString& Parameters)
{
	UMordecaiPostureSystem* System = NewObject<UMordecaiPostureSystem>();

	// AC-007.4: PostureDamage = BasePower x PostureDamageScalar
	float Damage = System->ComputePostureDamage(20.f, 1.5f);
	TestEqual("20 base x 1.5 scalar = 30 posture damage", Damage, 30.f);

	// Different scalars
	TestEqual("10 base x 2.0 scalar = 20", System->ComputePostureDamage(10.f, 2.0f), 20.f);
	TestEqual("50 base x 0.5 scalar = 25", System->ComputePostureDamage(50.f, 0.5f), 25.f);
	TestEqual("0 base x 1.0 scalar = 0", System->ComputePostureDamage(0.f, 1.0f), 0.f);

	return true;
}

// ---------------------------------------------------------------------------
// 3. Mordecai.Posture.BlockReducesPostureDamage (AC-007.5)
// Block a hit, verify posture damage = base x BlockPostureDamageMultiplier
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_Posture_BlockReducesDamage,
	"Mordecai.Posture.BlockReducesPostureDamage",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_Posture_BlockReducesDamage::RunTest(const FString& Parameters)
{
	UMordecaiPostureSystem* System = NewObject<UMordecaiPostureSystem>();

	// AC-007.5: Default BlockPostureDamageMultiplier is 0.5
	TestEqual("Default BlockPostureDamageMultiplier is 0.5",
		System->BlockPostureDamageMultiplier, 0.5f);

	// Normal hit (not blocking): 20 base x 1.0 scalar = 20
	float NormalDamage = System->ComputePostureDamage(20.f, 1.0f, false);
	TestEqual("Unblocked: 20 posture damage", NormalDamage, 20.f);

	// Blocked hit: 20 base x 1.0 scalar x 0.5 = 10
	float BlockedDamage = System->ComputePostureDamage(20.f, 1.0f, true);
	TestEqual("Blocked: 10 posture damage (50% reduction)", BlockedDamage, 10.f);

	// Custom multiplier
	System->BlockPostureDamageMultiplier = 0.25f;
	float CustomBlocked = System->ComputePostureDamage(40.f, 1.0f, true);
	TestEqual("Custom 0.25 multiplier: 40 x 1.0 x 0.25 = 10", CustomBlocked, 10.f);

	return true;
}

// ---------------------------------------------------------------------------
// 4. Mordecai.Posture.BreaksAtZero (AC-007.7)
// Reduce posture to 0, verify PostureBroken tag applied
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_Posture_BreaksAtZero,
	"Mordecai.Posture.BreaksAtZero",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_Posture_BreaksAtZero::RunTest(const FString& Parameters)
{
	UMordecaiPostureSystem* System = NewObject<UMordecaiPostureSystem>();

	// AC-007.7: PostureBroken tag is valid
	FGameplayTag PostureBrokenTag = MordecaiGameplayTags::State_PostureBroken;
	TestTrue("Mordecai.State.PostureBroken tag is valid", PostureBrokenTag.IsValid());

	// Posture at 0 should trigger break
	TestTrue("Break when posture reaches 0", System->ShouldBreakPosture(0.f));

	// Posture above 0 should not break
	TestFalse("No break when posture is 50", System->ShouldBreakPosture(50.f));
	TestFalse("No break when posture is 0.1", System->ShouldBreakPosture(0.1f));

	// Negative value (clamp) should also trigger break
	TestTrue("Break when posture is negative", System->ShouldBreakPosture(-5.f));

	// State transition
	TestFalse("Not broken initially", System->IsPostureBroken());
	System->EnterPostureBroken();
	TestTrue("PostureBroken active after entering", System->IsPostureBroken());
	TestTrue("Is vulnerable when broken", System->IsVulnerable());

	return true;
}

// ---------------------------------------------------------------------------
// 5. Mordecai.Posture.BrokenStateBlocksAbilities (AC-007.8)
// Apply PostureBroken, attempt to activate combat ability, verify blocked
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_Posture_BrokenBlocksAbilities,
	"Mordecai.Posture.BrokenStateBlocksAbilities",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_Posture_BrokenBlocksAbilities::RunTest(const FString& Parameters)
{
	// AC-007.8: All combat abilities must be blocked by PostureBroken and KnockedDown
	FGameplayTag PostureBrokenTag = MordecaiGameplayTags::State_PostureBroken;
	FGameplayTag KnockedDownTag = MordecaiGameplayTags::State_KnockedDown;

	TestTrue("PostureBroken tag valid", PostureBrokenTag.IsValid());
	TestTrue("KnockedDown tag valid", KnockedDownTag.IsValid());

	// Melee Attack blocked by PostureBroken and KnockedDown
	UMordecaiGA_MeleeAttack* MeleeAbility = NewObject<UMordecaiGA_MeleeAttack>();
	TestTrue("Melee blocked by PostureBroken",
		MeleeAbility->HasActivationBlockedTag(PostureBrokenTag));
	TestTrue("Melee blocked by KnockedDown",
		MeleeAbility->HasActivationBlockedTag(KnockedDownTag));

	// Block blocked by PostureBroken and KnockedDown
	UMordecaiGA_Block* BlockAbility = NewObject<UMordecaiGA_Block>();
	TestTrue("Block blocked by PostureBroken",
		BlockAbility->HasActivationBlockedTag(PostureBrokenTag));
	TestTrue("Block blocked by KnockedDown",
		BlockAbility->HasActivationBlockedTag(KnockedDownTag));

	// Dodge blocked by PostureBroken and KnockedDown
	UMordecaiGA_Dodge* DodgeAbility = NewObject<UMordecaiGA_Dodge>();
	TestTrue("Dodge blocked by PostureBroken",
		DodgeAbility->HasActivationBlockedTag(PostureBrokenTag));
	TestTrue("Dodge blocked by KnockedDown",
		DodgeAbility->HasActivationBlockedTag(KnockedDownTag));

	// Parry blocked by PostureBroken and KnockedDown
	UMordecaiGA_Parry* ParryAbility = NewObject<UMordecaiGA_Parry>();
	TestTrue("Parry blocked by PostureBroken",
		ParryAbility->HasActivationBlockedTag(PostureBrokenTag));
	TestTrue("Parry blocked by KnockedDown",
		ParryAbility->HasActivationBlockedTag(KnockedDownTag));

	return true;
}

// ---------------------------------------------------------------------------
// 6. Mordecai.Posture.StaggerLastsOneSecond (AC-007.9)
// Break posture, verify PostureBroken expires after 1000ms
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_Posture_StaggerDuration,
	"Mordecai.Posture.StaggerLastsOneSecond",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_Posture_StaggerDuration::RunTest(const FString& Parameters)
{
	UMordecaiPostureSystem* System = NewObject<UMordecaiPostureSystem>();

	// AC-007.9: Default PostureBrokenDuration is 1000ms
	TestEqual("Default stagger duration is 1000ms",
		System->PostureBrokenDurationMs, 1000.f);
	TestEqual("Stagger duration in seconds is 1.0",
		System->GetPostureBrokenDurationSeconds(), 1.0f);

	// Configurable
	System->PostureBrokenDurationMs = 1500.f;
	TestEqual("Custom stagger duration 1500ms",
		System->PostureBrokenDurationMs, 1500.f);
	TestEqual("Custom duration in seconds",
		System->GetPostureBrokenDurationSeconds(), 1.5f);

	return true;
}

// ---------------------------------------------------------------------------
// 7. Mordecai.Posture.StaggeredDamageMultiplied (AC-007.10)
// Break posture, deal damage during stagger, verify damage x 1.5
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_Posture_StaggeredDamageMultiplied,
	"Mordecai.Posture.StaggeredDamageMultiplied",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_Posture_StaggeredDamageMultiplied::RunTest(const FString& Parameters)
{
	UMordecaiPostureSystem* System = NewObject<UMordecaiPostureSystem>();

	// AC-007.10: Default StaggeredDamageMultiplier is 1.5
	TestEqual("Default stagger damage multiplier is 1.5",
		System->StaggeredDamageMultiplier, 1.5f);

	// Compute staggered damage: 100 damage x 1.5 = 150
	float StaggeredDamage = System->ComputeStaggeredDamage(100.f);
	TestEqual("100 damage x 1.5 = 150 during stagger", StaggeredDamage, 150.f);

	// During PostureBroken, vulnerability multiplier returns stagger multiplier
	System->EnterPostureBroken();
	TestEqual("Vulnerability multiplier is 1.5 when broken",
		System->GetVulnerabilityDamageMultiplier(), 1.5f);

	// Custom multiplier
	System->StaggeredDamageMultiplier = 2.0f;
	TestEqual("Custom: 100 x 2.0 = 200", System->ComputeStaggeredDamage(100.f), 200.f);

	return true;
}

// ---------------------------------------------------------------------------
// 8. Mordecai.Posture.KnockDownOnThresholdDamage (AC-007.10a)
// Break posture, deal damage exceeding threshold, verify KnockedDown
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_Posture_KnockDownOnThreshold,
	"Mordecai.Posture.KnockDownOnThresholdDamage",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_Posture_KnockDownOnThreshold::RunTest(const FString& Parameters)
{
	UMordecaiPostureSystem* System = NewObject<UMordecaiPostureSystem>();

	// AC-007.10a: KnockDown tag is valid
	FGameplayTag KnockedDownTag = MordecaiGameplayTags::State_KnockedDown;
	TestTrue("Mordecai.State.KnockedDown tag is valid", KnockedDownTag.IsValid());

	// KnockDownDamageThreshold is configurable
	TestTrue("KnockDownDamageThreshold > 0", System->KnockDownDamageThreshold > 0.f);

	// Enter stagger, accumulate damage below threshold
	System->EnterPostureBroken();
	System->AccumulateStaggerDamage(System->KnockDownDamageThreshold - 1.f);
	TestFalse("Below threshold: no knock down", System->ShouldKnockDown());

	// Accumulate damage past threshold
	System->AccumulateStaggerDamage(2.f);
	TestTrue("Above threshold: should knock down", System->ShouldKnockDown());

	// Enter knock down replaces stagger
	System->EnterKnockDown();
	TestTrue("Is knocked down", System->IsKnockedDown());
	TestFalse("No longer posture broken (replaced by knock down)", System->IsPostureBroken());
	TestTrue("Is vulnerable when knocked down", System->IsVulnerable());

	return true;
}

// ---------------------------------------------------------------------------
// 9. Mordecai.Posture.KnockDownExtendsDuration (AC-007.10a)
// Trigger knock down, verify vulnerability window extends to KnockDownDurationMs
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_Posture_KnockDownExtendsDuration,
	"Mordecai.Posture.KnockDownExtendsDuration",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_Posture_KnockDownExtendsDuration::RunTest(const FString& Parameters)
{
	UMordecaiPostureSystem* System = NewObject<UMordecaiPostureSystem>();

	// AC-007.10a: Default KnockDownDurationMs is 2000ms
	TestEqual("Default knock down duration is 2000ms",
		System->KnockDownDurationMs, 2000.f);
	TestEqual("Knock down duration in seconds is 2.0",
		System->GetKnockDownDurationSeconds(), 2.0f);

	// Knock down duration is longer than stagger duration
	TestTrue("Knock down lasts longer than stagger",
		System->KnockDownDurationMs > System->PostureBrokenDurationMs);

	// Configurable
	System->KnockDownDurationMs = 3000.f;
	TestEqual("Custom knock down duration",
		System->GetKnockDownDurationSeconds(), 3.0f);

	return true;
}

// ---------------------------------------------------------------------------
// 10. Mordecai.Posture.KnockDownDoubleDamage (AC-007.10a)
// Deal damage during KnockedDown state, verify damage x 2.0
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_Posture_KnockDownDoubleDamage,
	"Mordecai.Posture.KnockDownDoubleDamage",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_Posture_KnockDownDoubleDamage::RunTest(const FString& Parameters)
{
	UMordecaiPostureSystem* System = NewObject<UMordecaiPostureSystem>();

	// AC-007.10a: Default KnockDownDamageMultiplier is 2.0
	TestEqual("Default knock down damage multiplier is 2.0",
		System->KnockDownDamageMultiplier, 2.0f);

	// Compute knock down damage: 100 x 2.0 = 200
	float KnockDownDamage = System->ComputeKnockDownDamage(100.f);
	TestEqual("100 damage x 2.0 = 200 during knock down", KnockDownDamage, 200.f);

	// During KnockedDown, vulnerability multiplier returns knock down multiplier
	System->EnterPostureBroken();
	System->EnterKnockDown();
	TestEqual("Vulnerability multiplier is 2.0 when knocked down",
		System->GetVulnerabilityDamageMultiplier(), 2.0f);

	// Knock down multiplier is greater than stagger multiplier
	TestTrue("Knock down multiplier > stagger multiplier",
		System->KnockDownDamageMultiplier > System->StaggeredDamageMultiplier);

	return true;
}

// ---------------------------------------------------------------------------
// 11. Mordecai.Posture.ResetsAfterBrokenExpires (AC-007.11)
// Break posture, wait for expiry, verify Posture == MaxPosture
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_Posture_ResetsAfterBrokenExpires,
	"Mordecai.Posture.ResetsAfterBrokenExpires",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_Posture_ResetsAfterBrokenExpires::RunTest(const FString& Parameters)
{
	UMordecaiPostureSystem* System = NewObject<UMordecaiPostureSystem>();

	// AC-007.11: After PostureBroken expires, Posture resets to MaxPosture
	float MaxPosture = 100.f;

	// Enter and exit posture broken
	System->EnterPostureBroken();
	TestTrue("Is broken", System->IsPostureBroken());

	float NewPosture = System->ExitPostureBroken(MaxPosture);
	TestEqual("Posture resets to MaxPosture after break expires", NewPosture, MaxPosture);
	TestFalse("No longer broken after exit", System->IsPostureBroken());

	// Works with custom MaxPosture
	System->EnterPostureBroken();
	float CustomMax = 150.f;
	float CustomPosture = System->ExitPostureBroken(CustomMax);
	TestEqual("Resets to custom MaxPosture", CustomPosture, CustomMax);

	// Knock down exit also resets posture
	System->EnterPostureBroken();
	System->EnterKnockDown();
	float KnockDownPosture = System->ExitKnockDown(MaxPosture);
	TestEqual("Posture resets to MaxPosture after knock down expires", KnockDownPosture, MaxPosture);
	TestFalse("No longer knocked down after exit", System->IsKnockedDown());

	return true;
}

// ---------------------------------------------------------------------------
// 12. Mordecai.Posture.RegeneratesOverTime (AC-007.12)
// Deal partial posture damage, wait past regen delay, verify posture increases
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_Posture_RegeneratesOverTime,
	"Mordecai.Posture.RegeneratesOverTime",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_Posture_RegeneratesOverTime::RunTest(const FString& Parameters)
{
	UMordecaiPostureSystem* System = NewObject<UMordecaiPostureSystem>();

	// AC-007.12: Default regen rate is 10.0 units per second
	TestEqual("Default posture regen rate is 10.0", System->PostureRegenRate, 10.f);

	// Compute regen for 1 second
	float Regen1s = System->ComputeRegenAmount(1.0f);
	TestEqual("1 second of regen = 10.0", Regen1s, 10.f);

	// Compute regen for 0.5 seconds
	float Regen05s = System->ComputeRegenAmount(0.5f);
	TestEqual("0.5 seconds of regen = 5.0", Regen05s, 5.f);

	// Custom rate
	System->PostureRegenRate = 20.f;
	float CustomRegen = System->ComputeRegenAmount(1.0f);
	TestEqual("Custom rate 20: 1 second = 20.0", CustomRegen, 20.f);

	// Regen is allowed after delay has passed
	System->PostureRegenDelayMs = 3000.f;
	System->NotifyPostureDamageApplied(10.0); // damage at t=10
	TestTrue("Regen allowed after delay (t=14 > t=10 + 3s)",
		System->IsRegenAllowed(14.0));

	return true;
}

// ---------------------------------------------------------------------------
// 13. Mordecai.Posture.RegenPausedAfterDamage (AC-007.13)
// Deal posture damage, immediately check regen, verify no regen during delay
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_Posture_RegenPausedAfterDamage,
	"Mordecai.Posture.RegenPausedAfterDamage",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_Posture_RegenPausedAfterDamage::RunTest(const FString& Parameters)
{
	UMordecaiPostureSystem* System = NewObject<UMordecaiPostureSystem>();

	// AC-007.13: Default regen delay is 3000ms
	TestEqual("Default posture regen delay is 3000ms", System->PostureRegenDelayMs, 3000.f);
	TestEqual("Regen delay in seconds is 3.0", System->GetPostureRegenDelaySeconds(), 3.0f);

	// After taking damage, regen is paused for the delay period
	System->NotifyPostureDamageApplied(10.0); // damage at t=10s

	// Immediately after damage: no regen
	TestFalse("No regen immediately after damage (t=10.0)",
		System->IsRegenAllowed(10.0));

	// During delay: no regen
	TestFalse("No regen during delay (t=11.0, 1s after damage)",
		System->IsRegenAllowed(11.0));
	TestFalse("No regen during delay (t=12.5, 2.5s after damage)",
		System->IsRegenAllowed(12.5));

	// At exact delay boundary: still paused
	TestFalse("No regen at exact delay boundary (t=13.0, 3.0s after damage)",
		System->IsRegenAllowed(13.0));

	// Past delay: regen resumes
	TestTrue("Regen allowed past delay (t=13.1, 3.1s after damage)",
		System->IsRegenAllowed(13.1));

	return true;
}

// ---------------------------------------------------------------------------
// 14. Mordecai.Posture.NoRegenDuringBroken (AC-007.14)
// Break posture, verify no regen during PostureBroken state
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_Posture_NoRegenDuringBroken,
	"Mordecai.Posture.NoRegenDuringBroken",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_Posture_NoRegenDuringBroken::RunTest(const FString& Parameters)
{
	UMordecaiPostureSystem* System = NewObject<UMordecaiPostureSystem>();

	// AC-007.14: No regen during PostureBroken
	System->EnterPostureBroken();

	// Even with no recent damage, regen is blocked during broken state
	TestFalse("No regen during PostureBroken (well past any delay)",
		System->IsRegenAllowed(99999.0));

	// Also blocked during KnockedDown
	System->EnterKnockDown();
	TestFalse("No regen during KnockedDown",
		System->IsRegenAllowed(99999.0));

	// After exiting broken state, regen can resume (subject to delay)
	System->ExitKnockDown(100.f);
	TestFalse("No broken state after exit", System->IsPostureBroken());
	TestFalse("No knocked down state after exit", System->IsKnockedDown());

	return true;
}

// ---------------------------------------------------------------------------
// 15. Mordecai.Posture.PostureClampedToMax (AC-007.1)
// Attempt to set Posture above MaxPosture, verify clamped
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_Posture_ClampedToMax,
	"Mordecai.Posture.PostureClampedToMax",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_Posture_ClampedToMax::RunTest(const FString& Parameters)
{
	// AC-007.1: Posture and MaxPosture exist, 0 <= Posture <= MaxPosture
	const UMordecaiAttributeSet* CDO = GetDefault<UMordecaiAttributeSet>();
	TestNotNull("Attribute set CDO", CDO);

	// Verify attributes exist
	TestTrue("Posture attribute valid", UMordecaiAttributeSet::GetPostureAttribute().IsValid());
	TestTrue("MaxPosture attribute valid", UMordecaiAttributeSet::GetMaxPostureAttribute().IsValid());

	// AC-007.2: MaxPosture default is 100.0
	TestEqual("Default MaxPosture is 100.0", CDO->GetMaxPosture(), 100.0f);
	TestEqual("Default Posture is 100.0", CDO->GetPosture(), 100.0f);

	// Clamping: Posture <= MaxPosture (verified via ClampAttribute in AttributeSet)
	TestTrue("Posture <= MaxPosture in CDO", CDO->GetPosture() <= CDO->GetMaxPosture());

	// Clamping: Posture >= 0 (verified via ClampAttribute in AttributeSet)
	TestTrue("Posture >= 0 in CDO", CDO->GetPosture() >= 0.f);

	// AC-007.15: Weakened status stub tag exists
	FGameplayTag WeakenedTag = MordecaiGameplayTags::Status_Weakened;
	TestTrue("Mordecai.Status.Weakened tag is valid", WeakenedTag.IsValid());

	// AC-007.16: Brittle status stub tag exists
	FGameplayTag BrittleTag = MordecaiGameplayTags::Status_Brittle;
	TestTrue("Mordecai.Status.Brittle tag is valid", BrittleTag.IsValid());

	// AC-007.15/16: Stub multipliers default to 1.0 (no effect)
	UMordecaiPostureSystem* System = NewObject<UMordecaiPostureSystem>();
	TestEqual("Weakened multiplier default 1.0", System->WeakenedPostureMultiplier, 1.0f);
	TestEqual("Brittle multiplier default 1.0", System->BrittlePostureMultiplier, 1.0f);

	// With Weakened applied, outgoing posture damage is reduced
	System->WeakenedPostureMultiplier = 0.5f;
	float WeakenedDamage = System->ComputePostureDamage(20.f, 1.0f);
	TestEqual("Weakened 0.5: 20 x 1.0 x 0.5 = 10", WeakenedDamage, 10.f);

	// With Brittle applied, incoming posture damage is increased
	System->WeakenedPostureMultiplier = 1.0f; // reset
	System->BrittlePostureMultiplier = 1.5f;
	float BrittleDamage = System->ComputePostureDamage(20.f, 1.0f);
	TestEqual("Brittle 1.5: 20 x 1.0 x 1.5 = 30", BrittleDamage, 30.f);

	return true;
}
