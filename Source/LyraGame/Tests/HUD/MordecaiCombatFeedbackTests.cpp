// Project Mordecai — Combat Feedback & Combo Counter Tests (US-061)

#include "Misc/AutomationTest.h"
#include "Mordecai/UI/MordecaiComboCounterWidget.h"
#include "Mordecai/UI/MordecaiCombatFeedbackWidget.h"

// ---------------------------------------------------------------------------
// 1. Mordecai.UI.ComboCounter.FormatComboHit (AC-061.1)
// Combo index 0 -> "Hit 1", index 1 -> "Hit 2", index 2 -> "Hit 3"
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_UI_ComboCounter_FormatComboHit,
	"Mordecai.UI.ComboCounter.FormatComboHit",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_UI_ComboCounter_FormatComboHit::RunTest(const FString& Parameters)
{
	TestEqual("Index 0 = Hit 1", UMordecaiComboCounterWidget::FormatComboHit(0), FString(TEXT("Hit 1")));
	TestEqual("Index 1 = Hit 2", UMordecaiComboCounterWidget::FormatComboHit(1), FString(TEXT("Hit 2")));
	TestEqual("Index 2 = Hit 3", UMordecaiComboCounterWidget::FormatComboHit(2), FString(TEXT("Hit 3")));
	return true;
}

// ---------------------------------------------------------------------------
// 2. Mordecai.UI.ComboCounter.FormatComboHitNegative (AC-061.1)
// Negative index returns empty string (no combo)
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_UI_ComboCounter_FormatComboHitNegative,
	"Mordecai.UI.ComboCounter.FormatComboHitNegative",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_UI_ComboCounter_FormatComboHitNegative::RunTest(const FString& Parameters)
{
	TestEqual("Negative index = empty", UMordecaiComboCounterWidget::FormatComboHit(-1), FString(TEXT("")));
	return true;
}

// ---------------------------------------------------------------------------
// 3. Mordecai.UI.CombatFeedback.GetFeedbackTextForTag (AC-061.2)
// Maps gameplay tags to display text
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_UI_CombatFeedback_GetFeedbackTextForTag,
	"Mordecai.UI.CombatFeedback.GetFeedbackTextForTag",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_UI_CombatFeedback_GetFeedbackTextForTag::RunTest(const FString& Parameters)
{
	const FGameplayTag PerfectDodge = FGameplayTag::RequestGameplayTag(FName(TEXT("Mordecai.State.PerfectDodge")));
	const FGameplayTag PerfectBlock = FGameplayTag::RequestGameplayTag(FName(TEXT("Mordecai.State.PerfectBlock")));
	const FGameplayTag Parried = FGameplayTag::RequestGameplayTag(FName(TEXT("Mordecai.State.Parried")));
	const FGameplayTag ParryWhiff = FGameplayTag::RequestGameplayTag(FName(TEXT("Mordecai.State.ParryWhiff")));
	const FGameplayTag PostureBroken = FGameplayTag::RequestGameplayTag(FName(TEXT("Mordecai.State.PostureBroken")));

	TestEqual("PerfectDodge text", UMordecaiCombatFeedbackWidget::GetFeedbackTextForTag(PerfectDodge), FString(TEXT("Perfect Dodge!")));
	TestEqual("PerfectBlock text", UMordecaiCombatFeedbackWidget::GetFeedbackTextForTag(PerfectBlock), FString(TEXT("Perfect Block!")));
	TestEqual("Parried text", UMordecaiCombatFeedbackWidget::GetFeedbackTextForTag(Parried), FString(TEXT("Parried!")));
	TestEqual("ParryWhiff text", UMordecaiCombatFeedbackWidget::GetFeedbackTextForTag(ParryWhiff), FString(TEXT("Parry Failed!")));
	TestEqual("PostureBroken text", UMordecaiCombatFeedbackWidget::GetFeedbackTextForTag(PostureBroken), FString(TEXT("Posture Broken!")));
	return true;
}

// ---------------------------------------------------------------------------
// 4. Mordecai.UI.CombatFeedback.GetFeedbackColorForTag (AC-061.2)
// Positive feedback = gold, negative = red
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_UI_CombatFeedback_GetFeedbackColorForTag,
	"Mordecai.UI.CombatFeedback.GetFeedbackColorForTag",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_UI_CombatFeedback_GetFeedbackColorForTag::RunTest(const FString& Parameters)
{
	const FGameplayTag PerfectDodge = FGameplayTag::RequestGameplayTag(FName(TEXT("Mordecai.State.PerfectDodge")));
	const FGameplayTag ParryWhiff = FGameplayTag::RequestGameplayTag(FName(TEXT("Mordecai.State.ParryWhiff")));

	const FLinearColor PositiveColor = UMordecaiCombatFeedbackWidget::GetFeedbackColorForTag(PerfectDodge);
	const FLinearColor NegativeColor = UMordecaiCombatFeedbackWidget::GetFeedbackColorForTag(ParryWhiff);

	// Positive actions get gold/yellow, negative get red
	TestEqual("PerfectDodge = gold", PositiveColor, FLinearColor(1.f, 0.84f, 0.f, 1.f));
	TestEqual("ParryWhiff = red", NegativeColor, FLinearColor(1.f, 0.2f, 0.2f, 1.f));
	return true;
}

// ---------------------------------------------------------------------------
// 5. Mordecai.UI.CombatFeedback.UnknownTagReturnsEmpty (AC-061.2)
// Tags not in the feedback map return empty string
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_UI_CombatFeedback_UnknownTagReturnsEmpty,
	"Mordecai.UI.CombatFeedback.UnknownTagReturnsEmpty",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_UI_CombatFeedback_UnknownTagReturnsEmpty::RunTest(const FString& Parameters)
{
	const FGameplayTag Unknown = FGameplayTag::RequestGameplayTag(FName(TEXT("Mordecai.State.Dodging")));
	TestEqual("Unknown tag = empty", UMordecaiCombatFeedbackWidget::GetFeedbackTextForTag(Unknown), FString(TEXT("")));
	return true;
}
