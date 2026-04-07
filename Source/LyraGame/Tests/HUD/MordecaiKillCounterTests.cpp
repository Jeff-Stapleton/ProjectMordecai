// Project Mordecai — Kill Counter Tests (US-063)

#include "Misc/AutomationTest.h"
#include "Mordecai/UI/MordecaiKillCounterWidget.h"

// ---------------------------------------------------------------------------
// 1. Mordecai.UI.KillCounter.FormatKillCount
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_UI_KillCounter_FormatKillCount,
	"Mordecai.UI.KillCounter.FormatKillCount",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_UI_KillCounter_FormatKillCount::RunTest(const FString& Parameters)
{
	TestEqual("0 kills", UMordecaiKillCounterWidget::FormatKillCount(0), FString(TEXT("Kills: 0")));
	TestEqual("1 kill", UMordecaiKillCounterWidget::FormatKillCount(1), FString(TEXT("Kills: 1")));
	TestEqual("99 kills", UMordecaiKillCounterWidget::FormatKillCount(99), FString(TEXT("Kills: 99")));
	return true;
}
