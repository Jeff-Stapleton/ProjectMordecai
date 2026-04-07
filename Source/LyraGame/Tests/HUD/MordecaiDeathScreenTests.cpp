// Project Mordecai — Death Screen & Respawn Countdown Tests (US-062)

#include "Misc/AutomationTest.h"
#include "Mordecai/UI/MordecaiDeathScreenWidget.h"

// ---------------------------------------------------------------------------
// 1. Mordecai.UI.DeathScreen.FormatCountdownNormal (AC-062.1)
// 2.5s -> "3", 1.1s -> "2", 0.5s -> "1"
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_UI_DeathScreen_FormatCountdownNormal,
	"Mordecai.UI.DeathScreen.FormatCountdownNormal",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_UI_DeathScreen_FormatCountdownNormal::RunTest(const FString& Parameters)
{
	TestEqual("2.5s = 3", UMordecaiDeathScreenWidget::FormatRespawnCountdown(2.5f), FString(TEXT("3")));
	TestEqual("1.1s = 2", UMordecaiDeathScreenWidget::FormatRespawnCountdown(1.1f), FString(TEXT("2")));
	TestEqual("0.5s = 1", UMordecaiDeathScreenWidget::FormatRespawnCountdown(0.5f), FString(TEXT("1")));
	TestEqual("3.0s = 3", UMordecaiDeathScreenWidget::FormatRespawnCountdown(3.0f), FString(TEXT("3")));
	return true;
}

// ---------------------------------------------------------------------------
// 2. Mordecai.UI.DeathScreen.FormatCountdownZero (AC-062.1)
// 0 or negative -> empty (respawn imminent)
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_UI_DeathScreen_FormatCountdownZero,
	"Mordecai.UI.DeathScreen.FormatCountdownZero",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_UI_DeathScreen_FormatCountdownZero::RunTest(const FString& Parameters)
{
	TestEqual("0.0s = empty", UMordecaiDeathScreenWidget::FormatRespawnCountdown(0.0f), FString(TEXT("")));
	TestEqual("-1.0s = empty", UMordecaiDeathScreenWidget::FormatRespawnCountdown(-1.0f), FString(TEXT("")));
	return true;
}

// ---------------------------------------------------------------------------
// 3. Mordecai.UI.DeathScreen.FormatKillCount (AC-062.2)
// Formats kill count for death screen display
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_UI_DeathScreen_FormatKillCount,
	"Mordecai.UI.DeathScreen.FormatKillCount",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_UI_DeathScreen_FormatKillCount::RunTest(const FString& Parameters)
{
	TestEqual("0 kills", UMordecaiDeathScreenWidget::FormatKillCount(0), FString(TEXT("Kills: 0")));
	TestEqual("5 kills", UMordecaiDeathScreenWidget::FormatKillCount(5), FString(TEXT("Kills: 5")));
	TestEqual("42 kills", UMordecaiDeathScreenWidget::FormatKillCount(42), FString(TEXT("Kills: 42")));
	return true;
}
