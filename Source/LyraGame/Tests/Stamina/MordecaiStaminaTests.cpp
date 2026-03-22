// Project Mordecai — Stamina Tier System Tests (US-008)

#include "Misc/AutomationTest.h"
#include "Mordecai/Combat/MordecaiStaminaSystem.h"
#include "Mordecai/Combat/MordecaiGA_MeleeAttack.h"
#include "Mordecai/Combat/MordecaiGA_Block.h"
#include "Mordecai/Combat/MordecaiGA_Dodge.h"
#include "Mordecai/Combat/MordecaiGA_Parry.h"
#include "Mordecai/AbilitySystem/MordecaiAttributeSet.h"
#include "Mordecai/MordecaiGameplayTags.h"

// ---------------------------------------------------------------------------
// 1. Mordecai.Stamina.GreenTierAboveThreshold (AC-008.3, AC-008.4)
// Set Stamina to 80/100, verify Green tier
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_Stamina_GreenTierAboveThreshold,
	"Mordecai.Stamina.GreenTierAboveThreshold",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_Stamina_GreenTierAboveThreshold::RunTest(const FString& Parameters)
{
	UMordecaiStaminaSystem* System = NewObject<UMordecaiStaminaSystem>();

	// AC-008.3: Green tier when Stamina > 66% of Max
	EMordecaiStaminaTier Tier = System->GetTierForStamina(80.f, 100.f);
	TestEqual("80/100 = Green tier", Tier, EMordecaiStaminaTier::Green);

	// AC-008.4: Green tier tag is valid
	FGameplayTag GreenTag = MordecaiGameplayTags::Stamina_Tier_Green;
	TestTrue("Mordecai.Stamina.Tier.Green tag is valid", GreenTag.IsValid());

	// Default threshold is 0.66
	TestEqual("Default green threshold is 0.66", System->GreenThresholdPercent, 0.66f);

	// Edge case: exactly at threshold boundary (66% = 66/100) is Yellow, above is Green
	EMordecaiStaminaTier AtBoundary = System->GetTierForStamina(66.f, 100.f);
	TestEqual("66/100 = Yellow (not above 66%)", AtBoundary, EMordecaiStaminaTier::Yellow);

	EMordecaiStaminaTier JustAbove = System->GetTierForStamina(67.f, 100.f);
	TestEqual("67/100 = Green (above 66%)", JustAbove, EMordecaiStaminaTier::Green);

	return true;
}

// ---------------------------------------------------------------------------
// 2. Mordecai.Stamina.YellowTierInMiddleRange (AC-008.3, AC-008.4)
// Set Stamina to 50/100, verify Yellow tier
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_Stamina_YellowTierInMiddleRange,
	"Mordecai.Stamina.YellowTierInMiddleRange",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_Stamina_YellowTierInMiddleRange::RunTest(const FString& Parameters)
{
	UMordecaiStaminaSystem* System = NewObject<UMordecaiStaminaSystem>();

	// AC-008.3: Yellow tier when Stamina 33%-66% of Max
	EMordecaiStaminaTier Tier = System->GetTierForStamina(50.f, 100.f);
	TestEqual("50/100 = Yellow tier", Tier, EMordecaiStaminaTier::Yellow);

	// AC-008.4: Yellow tier tag is valid
	FGameplayTag YellowTag = MordecaiGameplayTags::Stamina_Tier_Yellow;
	TestTrue("Mordecai.Stamina.Tier.Yellow tag is valid", YellowTag.IsValid());

	// Default threshold is 0.33
	TestEqual("Default yellow threshold is 0.33", System->YellowThresholdPercent, 0.33f);

	// Edge case: at 33% boundary is Red, above is Yellow
	EMordecaiStaminaTier AtLower = System->GetTierForStamina(33.f, 100.f);
	TestEqual("33/100 = Yellow (at 33%)", AtLower, EMordecaiStaminaTier::Yellow);

	EMordecaiStaminaTier BelowLower = System->GetTierForStamina(32.f, 100.f);
	TestEqual("32/100 = Red (below 33%)", BelowLower, EMordecaiStaminaTier::Red);

	return true;
}

// ---------------------------------------------------------------------------
// 3. Mordecai.Stamina.RedTierBelowThreshold (AC-008.3, AC-008.4)
// Set Stamina to 20/100, verify Red tier
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_Stamina_RedTierBelowThreshold,
	"Mordecai.Stamina.RedTierBelowThreshold",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_Stamina_RedTierBelowThreshold::RunTest(const FString& Parameters)
{
	UMordecaiStaminaSystem* System = NewObject<UMordecaiStaminaSystem>();

	// AC-008.3: Red tier when Stamina < 33% of Max
	EMordecaiStaminaTier Tier = System->GetTierForStamina(20.f, 100.f);
	TestEqual("20/100 = Red tier", Tier, EMordecaiStaminaTier::Red);

	// AC-008.4: Red tier tag is valid
	FGameplayTag RedTag = MordecaiGameplayTags::Stamina_Tier_Red;
	TestTrue("Mordecai.Stamina.Tier.Red tag is valid", RedTag.IsValid());

	// Low positive value is still Red
	EMordecaiStaminaTier LowPositive = System->GetTierForStamina(1.f, 100.f);
	TestEqual("1/100 = Red tier", LowPositive, EMordecaiStaminaTier::Red);

	return true;
}

// ---------------------------------------------------------------------------
// 4. Mordecai.Stamina.ExhaustedAtZero (AC-008.6)
// Set Stamina to 0, verify Exhausted tag active
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_Stamina_ExhaustedAtZero,
	"Mordecai.Stamina.ExhaustedAtZero",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_Stamina_ExhaustedAtZero::RunTest(const FString& Parameters)
{
	UMordecaiStaminaSystem* System = NewObject<UMordecaiStaminaSystem>();

	// AC-008.6: Exhausted when Stamina <= 0
	TestTrue("Exhausted at Stamina 0", System->IsExhausted(0.f));
	TestTrue("Exhausted at negative Stamina", System->IsExhausted(-10.f));
	TestFalse("Not exhausted at Stamina 1", System->IsExhausted(1.f));

	// AC-008.6: Exhausted tag is valid
	FGameplayTag ExhaustedTag = MordecaiGameplayTags::Stamina_Tier_Exhausted;
	TestTrue("Mordecai.Stamina.Tier.Exhausted tag is valid", ExhaustedTag.IsValid());

	// Exhausted overlaps with Red (both active simultaneously)
	EMordecaiStaminaTier TierAtZero = System->GetTierForStamina(0.f, 100.f);
	TestEqual("Tier at 0 is Red (Exhausted is an overlay)", TierAtZero, EMordecaiStaminaTier::Red);

	EMordecaiStaminaTier TierNegative = System->GetTierForStamina(-20.f, 100.f);
	TestEqual("Tier at -20 is Red (Exhausted is an overlay)", TierNegative, EMordecaiStaminaTier::Red);

	return true;
}

// ---------------------------------------------------------------------------
// 5. Mordecai.Stamina.TierUpdatesOnChange (AC-008.5)
// Start at Green, consume stamina to Yellow range, verify tag switches
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_Stamina_TierUpdatesOnChange,
	"Mordecai.Stamina.TierUpdatesOnChange",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_Stamina_TierUpdatesOnChange::RunTest(const FString& Parameters)
{
	UMordecaiStaminaSystem* System = NewObject<UMordecaiStaminaSystem>();

	// AC-008.5: Tier updates immediately when Stamina changes
	const float MaxStamina = 100.f;

	// Start at full (Green)
	EMordecaiStaminaTier TierFull = System->GetTierForStamina(100.f, MaxStamina);
	TestEqual("Full stamina = Green", TierFull, EMordecaiStaminaTier::Green);

	// Drop to 50 (Yellow)
	EMordecaiStaminaTier TierMid = System->GetTierForStamina(50.f, MaxStamina);
	TestEqual("Half stamina = Yellow", TierMid, EMordecaiStaminaTier::Yellow);

	// Drop to 20 (Red)
	EMordecaiStaminaTier TierLow = System->GetTierForStamina(20.f, MaxStamina);
	TestEqual("Low stamina = Red", TierLow, EMordecaiStaminaTier::Red);

	// Drop to 0 (Red + Exhausted)
	EMordecaiStaminaTier TierZero = System->GetTierForStamina(0.f, MaxStamina);
	TestEqual("Zero stamina = Red", TierZero, EMordecaiStaminaTier::Red);
	TestTrue("Zero stamina = Exhausted overlay", System->IsExhausted(0.f));

	// Recover back to Green
	EMordecaiStaminaTier TierRecovered = System->GetTierForStamina(90.f, MaxStamina);
	TestEqual("Recovered stamina = Green", TierRecovered, EMordecaiStaminaTier::Green);
	TestFalse("Not exhausted after recovery", System->IsExhausted(90.f));

	return true;
}

// ---------------------------------------------------------------------------
// 6. Mordecai.Stamina.TierMultiplierReturnsCorrectValue (AC-008.7, AC-008.8)
// Verify multiplier for each tier
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_Stamina_TierMultiplierReturnsCorrectValue,
	"Mordecai.Stamina.TierMultiplierReturnsCorrectValue",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_Stamina_TierMultiplierReturnsCorrectValue::RunTest(const FString& Parameters)
{
	UMordecaiStaminaSystem* System = NewObject<UMordecaiStaminaSystem>();

	// AC-008.7: Default multiplier values
	TestEqual("Green multiplier is 1.25", System->GreenMultiplier, 1.25f);
	TestEqual("Yellow multiplier is 1.0", System->YellowMultiplier, 1.0f);
	TestEqual("Red multiplier is 0.75", System->RedMultiplier, 0.75f);
	TestEqual("Exhausted multiplier is 0.5", System->ExhaustedMultiplier, 0.5f);

	// AC-008.8: GetTierMultiplier returns correct value per tier
	TestEqual("Green tier -> 1.25",
		System->GetTierMultiplier(80.f, 100.f), 1.25f);
	TestEqual("Yellow tier -> 1.0",
		System->GetTierMultiplier(50.f, 100.f), 1.0f);
	TestEqual("Red tier (positive) -> 0.75",
		System->GetTierMultiplier(20.f, 100.f), 0.75f);
	TestEqual("Exhausted (at 0) -> 0.5",
		System->GetTierMultiplier(0.f, 100.f), 0.5f);
	TestEqual("Exhausted (negative) -> 0.5",
		System->GetTierMultiplier(-10.f, 100.f), 0.5f);

	// Multipliers are configurable
	System->GreenMultiplier = 1.5f;
	TestEqual("Custom green multiplier 1.5",
		System->GetMultiplierForTier(EMordecaiStaminaTier::Green), 1.5f);

	return true;
}

// ---------------------------------------------------------------------------
// 7. Mordecai.Stamina.ActionsNotBlockedAtZero (AC-008.9)
// Verify that no combat ability has stamina-related activation blocked tags
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_Stamina_ActionsNotBlockedAtZero,
	"Mordecai.Stamina.ActionsNotBlockedAtZero",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_Stamina_ActionsNotBlockedAtZero::RunTest(const FString& Parameters)
{
	// AC-008.9: Actions are NEVER blocked regardless of stamina level
	FGameplayTag ExhaustedTag = MordecaiGameplayTags::Stamina_Tier_Exhausted;
	FGameplayTag RedTag = MordecaiGameplayTags::Stamina_Tier_Red;
	FGameplayTag YellowTag = MordecaiGameplayTags::Stamina_Tier_Yellow;

	// Melee Attack not blocked by stamina tags
	UMordecaiGA_MeleeAttack* MeleeAbility = NewObject<UMordecaiGA_MeleeAttack>();
	TestFalse("Melee NOT blocked by Exhausted",
		MeleeAbility->HasActivationBlockedTag(ExhaustedTag));
	TestFalse("Melee NOT blocked by Red tier",
		MeleeAbility->HasActivationBlockedTag(RedTag));

	// Dodge not blocked by stamina tags
	UMordecaiGA_Dodge* DodgeAbility = NewObject<UMordecaiGA_Dodge>();
	TestFalse("Dodge NOT blocked by Exhausted",
		DodgeAbility->HasActivationBlockedTag(ExhaustedTag));
	TestFalse("Dodge NOT blocked by Red tier",
		DodgeAbility->HasActivationBlockedTag(RedTag));

	// Block not blocked by stamina tags
	UMordecaiGA_Block* BlockAbility = NewObject<UMordecaiGA_Block>();
	TestFalse("Block NOT blocked by Exhausted",
		BlockAbility->HasActivationBlockedTag(ExhaustedTag));
	TestFalse("Block NOT blocked by Red tier",
		BlockAbility->HasActivationBlockedTag(RedTag));

	// Parry not blocked by stamina tags
	UMordecaiGA_Parry* ParryAbility = NewObject<UMordecaiGA_Parry>();
	TestFalse("Parry NOT blocked by Exhausted",
		ParryAbility->HasActivationBlockedTag(ExhaustedTag));
	TestFalse("Parry NOT blocked by Red tier",
		ParryAbility->HasActivationBlockedTag(RedTag));

	return true;
}

// ---------------------------------------------------------------------------
// 8. Mordecai.Stamina.RegenerationOccursOverTime (AC-008.10)
// Verify stamina regen computation
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_Stamina_RegenerationOccursOverTime,
	"Mordecai.Stamina.RegenerationOccursOverTime",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_Stamina_RegenerationOccursOverTime::RunTest(const FString& Parameters)
{
	UMordecaiStaminaSystem* System = NewObject<UMordecaiStaminaSystem>();

	// AC-008.10: Default regen rate is 15.0 units per second
	TestEqual("Default StaminaRegenRate is 15.0", System->StaminaRegenRate, 15.f);

	// Compute regen for 1 second
	float Regen1s = System->ComputeRegenAmount(1.0f);
	TestEqual("1 second of regen = 15.0", Regen1s, 15.f);

	// Compute regen for 0.1 seconds (typical tick interval)
	float RegenTick = System->ComputeRegenAmount(0.1f);
	TestEqual("0.1 second of regen = 1.5", RegenTick, 1.5f);

	// After consumption, regen is allowed past delay
	System->NotifyStaminaConsumed(10.0); // consumed at t=10
	TestTrue("Regen allowed after delay (t=12, past 1000ms delay)",
		System->IsRegenAllowed(12.0));

	// Custom rate
	System->StaminaRegenRate = 25.f;
	TestEqual("Custom rate 25: 1 second = 25.0", System->ComputeRegenAmount(1.0f), 25.f);

	return true;
}

// ---------------------------------------------------------------------------
// 9. Mordecai.Stamina.RegenPausedDuringDelay (AC-008.11)
// Consume stamina, verify no regen during delay window
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_Stamina_RegenPausedDuringDelay,
	"Mordecai.Stamina.RegenPausedDuringDelay",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_Stamina_RegenPausedDuringDelay::RunTest(const FString& Parameters)
{
	UMordecaiStaminaSystem* System = NewObject<UMordecaiStaminaSystem>();

	// AC-008.11: Default regen delay is 1000ms
	TestEqual("Default StaminaRegenDelayMs is 1000.0", System->StaminaRegenDelayMs, 1000.f);
	TestEqual("Regen delay in seconds is 1.0", System->GetStaminaRegenDelaySeconds(), 1.0f);

	// Regen pause tag is valid
	FGameplayTag RegenPausedTag = MordecaiGameplayTags::State_StaminaRegenPaused;
	TestTrue("Mordecai.State.StaminaRegenPaused tag is valid", RegenPausedTag.IsValid());

	// After consuming stamina, regen is paused
	System->NotifyStaminaConsumed(10.0); // consumed at t=10s

	// Immediately after: no regen
	TestFalse("No regen immediately after consumption (t=10.0)",
		System->IsRegenAllowed(10.0));

	// During delay: no regen
	TestFalse("No regen during delay (t=10.5, 0.5s after)",
		System->IsRegenAllowed(10.5));

	// At exact delay boundary: still paused
	TestFalse("No regen at exact delay boundary (t=11.0, 1.0s after)",
		System->IsRegenAllowed(11.0));

	// Past delay: regen resumes
	TestTrue("Regen allowed past delay (t=11.1, 1.1s after)",
		System->IsRegenAllowed(11.1));

	return true;
}

// ---------------------------------------------------------------------------
// 10. Mordecai.Stamina.RegenDelayResetsOnConsumption (AC-008.12)
// Consume, wait half delay, consume again, verify delay restarted
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_Stamina_RegenDelayResetsOnConsumption,
	"Mordecai.Stamina.RegenDelayResetsOnConsumption",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_Stamina_RegenDelayResetsOnConsumption::RunTest(const FString& Parameters)
{
	UMordecaiStaminaSystem* System = NewObject<UMordecaiStaminaSystem>();

	// AC-008.12: Regen delay resets on each new consumption
	System->NotifyStaminaConsumed(10.0); // consumed at t=10

	// Wait half the delay (0.5s of 1.0s)
	TestFalse("No regen at t=10.5 (mid-delay)", System->IsRegenAllowed(10.5));

	// Consume again at t=10.5, resetting the delay
	System->NotifyStaminaConsumed(10.5);

	// The original delay end (t=11.0) should still be paused because delay was reset
	TestFalse("No regen at t=11.0 (delay reset by second consumption)",
		System->IsRegenAllowed(11.0));

	// New delay expires at t=11.5
	TestFalse("No regen at t=11.5 (exact boundary of new delay)",
		System->IsRegenAllowed(11.5));

	TestTrue("Regen allowed at t=11.6 (past new delay)",
		System->IsRegenAllowed(11.6));

	return true;
}

// ---------------------------------------------------------------------------
// 11. Mordecai.Stamina.SprintDrainsPerSecond (AC-008.14, AC-008.15)
// Verify sprint drain computation
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_Stamina_SprintDrainsPerSecond,
	"Mordecai.Stamina.SprintDrainsPerSecond",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_Stamina_SprintDrainsPerSecond::RunTest(const FString& Parameters)
{
	UMordecaiStaminaSystem* System = NewObject<UMordecaiStaminaSystem>();

	// AC-008.14: Default sprint drain rate is 10.0/sec
	TestEqual("Default SprintStaminaDrainRate is 10.0", System->SprintStaminaDrainRate, 10.f);

	// AC-008.15: Sprint drain is continuous (per-second), not lump
	float Drain1s = System->ComputeSprintDrain(1.0f);
	TestEqual("1 second sprint drain = 10.0", Drain1s, 10.f);

	float Drain05s = System->ComputeSprintDrain(0.5f);
	TestEqual("0.5 second sprint drain = 5.0", Drain05s, 5.f);

	float Drain01s = System->ComputeSprintDrain(0.1f);
	TestTrue("0.1 second sprint drain ~ 1.0", FMath::IsNearlyEqual(Drain01s, 1.0f, 0.01f));

	// Sprinting state tag is valid
	FGameplayTag SprintingTag = MordecaiGameplayTags::State_Sprinting;
	TestTrue("Mordecai.State.Sprinting tag is valid", SprintingTag.IsValid());

	// Custom drain rate
	System->SprintStaminaDrainRate = 20.f;
	TestEqual("Custom 20/sec: 1 second = 20.0", System->ComputeSprintDrain(1.0f), 20.f);

	return true;
}

// ---------------------------------------------------------------------------
// 12. Mordecai.Stamina.SprintContinuesBelowZero (AC-008.16)
// Verify sprint doesn't stop at zero — stamina can go negative
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_Stamina_SprintContinuesBelowZero,
	"Mordecai.Stamina.SprintContinuesBelowZero",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_Stamina_SprintContinuesBelowZero::RunTest(const FString& Parameters)
{
	UMordecaiStaminaSystem* System = NewObject<UMordecaiStaminaSystem>();

	// AC-008.16: Sprint does NOT stop when stamina reaches 0
	// Tier still applies multiplier even at negative values
	EMordecaiStaminaTier TierNeg = System->GetTierForStamina(-5.f, 100.f);
	TestEqual("Negative stamina is Red tier", TierNeg, EMordecaiStaminaTier::Red);
	TestTrue("Negative stamina is Exhausted", System->IsExhausted(-5.f));

	// Exhausted multiplier applies (reduced effectiveness, but sprint continues)
	float Mult = System->GetTierMultiplier(-5.f, 100.f);
	TestEqual("Exhausted multiplier 0.5 at negative stamina", Mult, 0.5f);

	// Sprint drain computation works at negative values (continues draining)
	float Drain = System->ComputeSprintDrain(1.0f);
	TestTrue("Sprint drain is positive even when stamina is negative", Drain > 0.f);

	// Verify the stamina floor allows negative values
	TestEqual("Default stamina floor is -50.0", System->StaminaFloor, -50.f);

	return true;
}

// ---------------------------------------------------------------------------
// 13. Mordecai.Stamina.CanGoNegative (AC-008.1)
// Verify stamina clamping allows negative values
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_Stamina_CanGoNegative,
	"Mordecai.Stamina.CanGoNegative",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_Stamina_CanGoNegative::RunTest(const FString& Parameters)
{
	// AC-008.1: Stamina can go below 0
	// The stamina system defines a configurable floor
	UMordecaiStaminaSystem* System = NewObject<UMordecaiStaminaSystem>();
	TestEqual("Stamina floor is -50.0", System->StaminaFloor, -50.f);

	// Tier computation handles negative values correctly
	EMordecaiStaminaTier TierNeg10 = System->GetTierForStamina(-10.f, 100.f);
	TestEqual("-10/100 = Red tier", TierNeg10, EMordecaiStaminaTier::Red);
	TestTrue("-10 is Exhausted", System->IsExhausted(-10.f));

	// Exhausted multiplier at negative
	float Mult = System->GetTierMultiplier(-25.f, 100.f);
	TestEqual("Exhausted multiplier at -25", Mult, 0.5f);

	// AC-008.2: MaxStamina default is 100.0
	const UMordecaiAttributeSet* CDO = GetDefault<UMordecaiAttributeSet>();
	TestNotNull("Attribute set CDO exists", CDO);
	TestEqual("Default MaxStamina is 100.0", CDO->GetMaxStamina(), 100.0f);
	TestEqual("Default Stamina is 100.0", CDO->GetStamina(), 100.0f);

	return true;
}

// ---------------------------------------------------------------------------
// 14. Mordecai.Stamina.StandardizedConsumptionGE (AC-008.17, AC-008.18)
// Verify SetByCaller tag for stamina cost exists and system tracks consumption
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_Stamina_StandardizedConsumptionGE,
	"Mordecai.Stamina.StandardizedConsumptionGE",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_Stamina_StandardizedConsumptionGE::RunTest(const FString& Parameters)
{
	// AC-008.17: SetByCaller tag for stamina cost
	FGameplayTag StaminaCostTag = MordecaiGameplayTags::SetByCaller_StaminaCost;
	TestTrue("Mordecai.Data.StaminaCost SetByCaller tag is valid", StaminaCostTag.IsValid());

	// AC-008.18: Consumption triggers regen delay
	UMordecaiStaminaSystem* System = NewObject<UMordecaiStaminaSystem>();

	// Before any consumption, regen is allowed (no delay active)
	TestTrue("Regen allowed before any consumption", System->IsRegenAllowed(0.0));

	// Notify consumption
	System->NotifyStaminaConsumed(5.0);

	// Regen paused after consumption
	TestFalse("Regen paused after consumption (t=5.0)", System->IsRegenAllowed(5.0));
	TestFalse("Regen paused during delay (t=5.5)", System->IsRegenAllowed(5.5));

	// Regen resumes after delay
	TestTrue("Regen resumes after delay (t=6.1)", System->IsRegenAllowed(6.1));

	// AC-008.13: Regen rate scaling with END is TODO(DECISION)
	// Verify the interface exists for future scaling
	TestEqual("Default StaminaRegenRate is base value (15.0)", System->StaminaRegenRate, 15.f);

	return true;
}
