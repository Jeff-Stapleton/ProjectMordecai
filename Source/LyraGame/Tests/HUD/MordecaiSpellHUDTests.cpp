// Project Mordecai — Spell HUD & Status Effect Indicator Tests (US-055)

#include "Misc/AutomationTest.h"
#include "Mordecai/UI/MordecaiSpellPointsBarWidget.h"
#include "Mordecai/UI/MordecaiStatusEffectBarWidget.h"
#include "Mordecai/UI/MordecaiStatusEffectIndicatorWidget.h"
#include "Mordecai/UI/MordecaiSpellCooldownWidget.h"
#include "GameplayTagContainer.h"

// ---------------------------------------------------------------------------
// 1. Mordecai.UI.SpellPointsBar.CalcPercentNormal (AC-055.1)
// 12/20 -> 0.6, 0/20 -> 0.0, 20/20 -> 1.0
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_UI_SpellPointsBar_CalcPercentNormal,
	"Mordecai.UI.SpellPointsBar.CalcPercentNormal",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_UI_SpellPointsBar_CalcPercentNormal::RunTest(const FString& Parameters)
{
	TestEqual("12/20 = 0.6", UMordecaiSpellPointsBarWidget::CalcSpellPointsPercent(12.f, 20.f), 0.6f);
	TestEqual("0/20 = 0.0", UMordecaiSpellPointsBarWidget::CalcSpellPointsPercent(0.f, 20.f), 0.0f);
	TestEqual("20/20 = 1.0", UMordecaiSpellPointsBarWidget::CalcSpellPointsPercent(20.f, 20.f), 1.0f);
	return true;
}

// ---------------------------------------------------------------------------
// 2. Mordecai.UI.SpellPointsBar.CalcPercentClampsAboveOne (AC-055.1)
// 25/20 -> 1.0
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_UI_SpellPointsBar_CalcPercentClampsAboveOne,
	"Mordecai.UI.SpellPointsBar.CalcPercentClampsAboveOne",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_UI_SpellPointsBar_CalcPercentClampsAboveOne::RunTest(const FString& Parameters)
{
	TestEqual("25/20 clamped to 1.0", UMordecaiSpellPointsBarWidget::CalcSpellPointsPercent(25.f, 20.f), 1.0f);
	return true;
}

// ---------------------------------------------------------------------------
// 3. Mordecai.UI.SpellPointsBar.CalcPercentZeroMax (AC-055.1)
// 5/0 -> 0.0 (safe division)
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_UI_SpellPointsBar_CalcPercentZeroMax,
	"Mordecai.UI.SpellPointsBar.CalcPercentZeroMax",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_UI_SpellPointsBar_CalcPercentZeroMax::RunTest(const FString& Parameters)
{
	TestEqual("5/0 = 0.0 (safe div-by-zero)", UMordecaiSpellPointsBarWidget::CalcSpellPointsPercent(5.f, 0.f), 0.0f);
	return true;
}

// ---------------------------------------------------------------------------
// 4. Mordecai.UI.SpellPointsBar.FormatNumericDisplay (AC-055.3)
// Displays "12 / 20" style text
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_UI_SpellPointsBar_FormatNumericDisplay,
	"Mordecai.UI.SpellPointsBar.FormatNumericDisplay",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_UI_SpellPointsBar_FormatNumericDisplay::RunTest(const FString& Parameters)
{
	TestEqual("12/20 format", UMordecaiSpellPointsBarWidget::FormatSpellPoints(12.f, 20.f), FString(TEXT("12 / 20")));
	TestEqual("0/10 format", UMordecaiSpellPointsBarWidget::FormatSpellPoints(0.f, 10.f), FString(TEXT("0 / 10")));
	TestEqual("5/5 format", UMordecaiSpellPointsBarWidget::FormatSpellPoints(5.f, 5.f), FString(TEXT("5 / 5")));
	return true;
}

// ---------------------------------------------------------------------------
// 5. Mordecai.UI.StatusEffectBar.IsBuffTag (AC-055.7)
// Buff tags return true, debuff tags return false
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_UI_StatusEffectBar_IsBuffTag,
	"Mordecai.UI.StatusEffectBar.IsBuffTag",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_UI_StatusEffectBar_IsBuffTag::RunTest(const FString& Parameters)
{
	// Focused, Blessed, StoneSkin, FireWard, Restoration are buffs
	const FGameplayTag FocusedTag = FGameplayTag::RequestGameplayTag(FName(TEXT("Mordecai.Status.Focused")));
	const FGameplayTag BlessedTag = FGameplayTag::RequestGameplayTag(FName(TEXT("Mordecai.Status.Blessed")));
	const FGameplayTag StoneSkinTag = FGameplayTag::RequestGameplayTag(FName(TEXT("Mordecai.Status.StoneSkin")));

	TestTrue("Focused is buff", UMordecaiStatusEffectBarWidget::IsBuffTag(FocusedTag));
	TestTrue("Blessed is buff", UMordecaiStatusEffectBarWidget::IsBuffTag(BlessedTag));
	TestTrue("StoneSkin is buff", UMordecaiStatusEffectBarWidget::IsBuffTag(StoneSkinTag));

	// Burning, Bleeding, Poisoned are debuffs
	const FGameplayTag BurningTag = FGameplayTag::RequestGameplayTag(FName(TEXT("Mordecai.Status.Burning")));
	const FGameplayTag BleedingTag = FGameplayTag::RequestGameplayTag(FName(TEXT("Mordecai.Status.Bleeding")));

	TestFalse("Burning is not buff", UMordecaiStatusEffectBarWidget::IsBuffTag(BurningTag));
	TestFalse("Bleeding is not buff", UMordecaiStatusEffectBarWidget::IsBuffTag(BleedingTag));
	return true;
}

// ---------------------------------------------------------------------------
// 6. Mordecai.UI.StatusEffectBar.GetTintForStatus (AC-055.7)
// Buffs get green tint, debuffs get red tint
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_UI_StatusEffectBar_GetTintForStatus,
	"Mordecai.UI.StatusEffectBar.GetTintForStatus",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_UI_StatusEffectBar_GetTintForStatus::RunTest(const FString& Parameters)
{
	const FGameplayTag FocusedTag = FGameplayTag::RequestGameplayTag(FName(TEXT("Mordecai.Status.Focused")));
	const FGameplayTag BurningTag = FGameplayTag::RequestGameplayTag(FName(TEXT("Mordecai.Status.Burning")));

	const FLinearColor BuffColor = UMordecaiStatusEffectBarWidget::GetTintForStatusTag(FocusedTag);
	const FLinearColor DebuffColor = UMordecaiStatusEffectBarWidget::GetTintForStatusTag(BurningTag);

	// Buff = green, Debuff = red
	TestEqual("Buff tint is green", BuffColor, FLinearColor(0.f, 1.f, 0.f, 1.f));
	TestEqual("Debuff tint is red", DebuffColor, FLinearColor(1.f, 0.f, 0.f, 1.f));
	return true;
}

// ---------------------------------------------------------------------------
// 7. Mordecai.UI.SpellCooldown.CalcCooldownPercent (AC-055.8)
// Elapsed/Total -> fraction
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_UI_SpellCooldown_CalcCooldownPercent,
	"Mordecai.UI.SpellCooldown.CalcCooldownPercent",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_UI_SpellCooldown_CalcCooldownPercent::RunTest(const FString& Parameters)
{
	// 2.5s elapsed of 5s total = 0.5
	TestEqual("2.5/5.0 = 0.5", UMordecaiSpellCooldownWidget::CalcCooldownPercent(2.5f, 5.0f), 0.5f);
	// 0s elapsed = 0.0 (just started cooldown)
	TestEqual("0/5.0 = 0.0", UMordecaiSpellCooldownWidget::CalcCooldownPercent(0.f, 5.0f), 0.0f);
	// 5s elapsed = 1.0 (cooldown finished)
	TestEqual("5.0/5.0 = 1.0", UMordecaiSpellCooldownWidget::CalcCooldownPercent(5.0f, 5.0f), 1.0f);
	// Over-elapsed clamped
	TestEqual("6.0/5.0 = 1.0 (clamped)", UMordecaiSpellCooldownWidget::CalcCooldownPercent(6.0f, 5.0f), 1.0f);
	return true;
}

// ---------------------------------------------------------------------------
// 8. Mordecai.UI.SpellCooldown.CalcCooldownPercentZeroDuration (AC-055.8)
// 0 duration -> 1.0 (ready)
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_UI_SpellCooldown_CalcCooldownPercentZeroDuration,
	"Mordecai.UI.SpellCooldown.CalcCooldownPercentZeroDuration",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_UI_SpellCooldown_CalcCooldownPercentZeroDuration::RunTest(const FString& Parameters)
{
	TestEqual("3.0/0.0 = 1.0 (zero duration = ready)", UMordecaiSpellCooldownWidget::CalcCooldownPercent(3.0f, 0.0f), 1.0f);
	return true;
}

// ---------------------------------------------------------------------------
// 9. Mordecai.UI.SpellCooldown.FormatRemainingTime (AC-055.8)
// Formats seconds remaining as display text
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_UI_SpellCooldown_FormatRemainingTime,
	"Mordecai.UI.SpellCooldown.FormatRemainingTime",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_UI_SpellCooldown_FormatRemainingTime::RunTest(const FString& Parameters)
{
	TestEqual("3.7s remaining", UMordecaiSpellCooldownWidget::FormatRemainingTime(3.7f), FString(TEXT("3.7")));
	TestEqual("0.0s = ready", UMordecaiSpellCooldownWidget::FormatRemainingTime(0.0f), FString(TEXT("")));
	TestEqual("negative = ready", UMordecaiSpellCooldownWidget::FormatRemainingTime(-1.0f), FString(TEXT("")));
	return true;
}
