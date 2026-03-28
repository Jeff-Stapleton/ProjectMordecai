// Project Mordecai — Combat HUD Widget Tests (US-052)

#include "Misc/AutomationTest.h"
#include "Mordecai/UI/MordecaiHealthBarWidget.h"
#include "Mordecai/UI/MordecaiStaminaBarWidget.h"
#include "Mordecai/UI/MordecaiPostureBarWidget.h"

// ---------------------------------------------------------------------------
// 1. Mordecai.HUD.HealthPercentCalculation (AC-052.6)
// 75/100 -> 0.75, 0/100 -> 0.0, 100/100 -> 1.0
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_HUD_HealthPercentCalculation,
	"Mordecai.HUD.HealthPercentCalculation",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_HUD_HealthPercentCalculation::RunTest(const FString& Parameters)
{
	TestEqual("75/100 = 0.75", UMordecaiHealthBarWidget::CalcHealthPercent(75.f, 100.f), 0.75f);
	TestEqual("0/100 = 0.0", UMordecaiHealthBarWidget::CalcHealthPercent(0.f, 100.f), 0.0f);
	TestEqual("100/100 = 1.0", UMordecaiHealthBarWidget::CalcHealthPercent(100.f, 100.f), 1.0f);
	return true;
}

// ---------------------------------------------------------------------------
// 2. Mordecai.HUD.HealthPercentClampsAboveOne (AC-052.6)
// 150/100 -> 1.0
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_HUD_HealthPercentClampsAboveOne,
	"Mordecai.HUD.HealthPercentClampsAboveOne",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_HUD_HealthPercentClampsAboveOne::RunTest(const FString& Parameters)
{
	TestEqual("150/100 clamped to 1.0", UMordecaiHealthBarWidget::CalcHealthPercent(150.f, 100.f), 1.0f);
	return true;
}

// ---------------------------------------------------------------------------
// 3. Mordecai.HUD.HealthPercentZeroMaxHealth (AC-052.6)
// 50/0 -> 0.0
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_HUD_HealthPercentZeroMaxHealth,
	"Mordecai.HUD.HealthPercentZeroMaxHealth",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_HUD_HealthPercentZeroMaxHealth::RunTest(const FString& Parameters)
{
	TestEqual("50/0 = 0.0 (safe div-by-zero)", UMordecaiHealthBarWidget::CalcHealthPercent(50.f, 0.f), 0.0f);
	return true;
}

// ---------------------------------------------------------------------------
// 4. Mordecai.HUD.StaminaPercentCalculation (AC-052.7)
// 80/100 -> 0.8, 0/100 -> 0.0
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_HUD_StaminaPercentCalculation,
	"Mordecai.HUD.StaminaPercentCalculation",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_HUD_StaminaPercentCalculation::RunTest(const FString& Parameters)
{
	TestEqual("80/100 = 0.8", UMordecaiStaminaBarWidget::CalcStaminaPercent(80.f, 100.f), 0.8f);
	TestEqual("0/100 = 0.0", UMordecaiStaminaBarWidget::CalcStaminaPercent(0.f, 100.f), 0.0f);
	return true;
}

// ---------------------------------------------------------------------------
// 5. Mordecai.HUD.StaminaTierGreen (AC-052.8)
// 70% stamina -> Green
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_HUD_StaminaTierGreen,
	"Mordecai.HUD.StaminaTierGreen",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_HUD_StaminaTierGreen::RunTest(const FString& Parameters)
{
	TestEqual("70/100 = Green", UMordecaiStaminaBarWidget::CalcStaminaTier(70.f, 100.f), EMordecaiStaminaTier::Green);
	return true;
}

// ---------------------------------------------------------------------------
// 6. Mordecai.HUD.StaminaTierYellow (AC-052.8)
// 50% stamina -> Yellow
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_HUD_StaminaTierYellow,
	"Mordecai.HUD.StaminaTierYellow",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_HUD_StaminaTierYellow::RunTest(const FString& Parameters)
{
	TestEqual("50/100 = Yellow", UMordecaiStaminaBarWidget::CalcStaminaTier(50.f, 100.f), EMordecaiStaminaTier::Yellow);
	return true;
}

// ---------------------------------------------------------------------------
// 7. Mordecai.HUD.StaminaTierRed (AC-052.8)
// 20% stamina -> Red
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_HUD_StaminaTierRed,
	"Mordecai.HUD.StaminaTierRed",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_HUD_StaminaTierRed::RunTest(const FString& Parameters)
{
	TestEqual("20/100 = Red", UMordecaiStaminaBarWidget::CalcStaminaTier(20.f, 100.f), EMordecaiStaminaTier::Red);
	return true;
}

// ---------------------------------------------------------------------------
// 8. Mordecai.HUD.StaminaTierExhausted (AC-052.8)
// 0% stamina -> Exhausted
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_HUD_StaminaTierExhausted,
	"Mordecai.HUD.StaminaTierExhausted",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_HUD_StaminaTierExhausted::RunTest(const FString& Parameters)
{
	TestEqual("0/100 = Exhausted", UMordecaiStaminaBarWidget::CalcStaminaTier(0.f, 100.f), EMordecaiStaminaTier::Exhausted);
	return true;
}

// ---------------------------------------------------------------------------
// 9. Mordecai.HUD.StaminaTierBoundaryGreenYellow (AC-052.8)
// exactly 66% -> Green
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_HUD_StaminaTierBoundaryGreenYellow,
	"Mordecai.HUD.StaminaTierBoundaryGreenYellow",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_HUD_StaminaTierBoundaryGreenYellow::RunTest(const FString& Parameters)
{
	TestEqual("66/100 = Green (boundary)", UMordecaiStaminaBarWidget::CalcStaminaTier(66.f, 100.f), EMordecaiStaminaTier::Green);
	return true;
}

// ---------------------------------------------------------------------------
// 10. Mordecai.HUD.StaminaTierBoundaryYellowRed (AC-052.8)
// exactly 33% -> Yellow
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_HUD_StaminaTierBoundaryYellowRed,
	"Mordecai.HUD.StaminaTierBoundaryYellowRed",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_HUD_StaminaTierBoundaryYellowRed::RunTest(const FString& Parameters)
{
	TestEqual("33/100 = Yellow (boundary)", UMordecaiStaminaBarWidget::CalcStaminaTier(33.f, 100.f), EMordecaiStaminaTier::Yellow);
	return true;
}

// ---------------------------------------------------------------------------
// 11. Mordecai.HUD.PosturePercentCalculation (AC-052.9)
// 60/100 -> 0.6
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_HUD_PosturePercentCalculation,
	"Mordecai.HUD.PosturePercentCalculation",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_HUD_PosturePercentCalculation::RunTest(const FString& Parameters)
{
	TestEqual("60/100 = 0.6", UMordecaiPostureBarWidget::CalcPosturePercent(60.f, 100.f), 0.6f);
	return true;
}

// ---------------------------------------------------------------------------
// 12. Mordecai.HUD.PosturePercentZeroMaxPosture (AC-052.9)
// 50/0 -> 0.0
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_HUD_PosturePercentZeroMaxPosture,
	"Mordecai.HUD.PosturePercentZeroMaxPosture",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_HUD_PosturePercentZeroMaxPosture::RunTest(const FString& Parameters)
{
	TestEqual("50/0 = 0.0 (safe div-by-zero)", UMordecaiPostureBarWidget::CalcPosturePercent(50.f, 0.f), 0.0f);
	return true;
}
