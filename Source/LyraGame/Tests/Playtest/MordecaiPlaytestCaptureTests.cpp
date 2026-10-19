// Project Mordecai — Playtest capture tests
// Scripts/playtest_boot.py launches packaged builds with -PlaytestCapture so the game saves its own
// screenshots; desktop capture is black when the machine is locked or the window is covered.

#include "Misc/AutomationTest.h"
#include "Misc/Paths.h"
#include "Mordecai/Playtest/MordecaiPlaytestCaptureSubsystem.h"

// Mordecai.Playtest.Capture.DisabledWithoutFlag
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_Playtest_CaptureDisabledWithoutFlag,
	"Mordecai.Playtest.Capture.DisabledWithoutFlag",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_Playtest_CaptureDisabledWithoutFlag::RunTest(const FString& Parameters)
{
	TestFalse("Normal launch does not capture",
		FMordecaiPlaytestCaptureConfig::FromCommandLine(TEXT("/MordecaiCore/Game/Maps/DevTestMap -windowed -log")).bEnabled);
	TestFalse("Shot settings alone do not enable capture",
		FMordecaiPlaytestCaptureConfig::FromCommandLine(TEXT("-PlaytestShots=3 -PlaytestShotDir=C:/Shots")).bEnabled);
	return true;
}

// Mordecai.Playtest.Capture.ParsesSettings
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_Playtest_CaptureParsesSettings,
	"Mordecai.Playtest.Capture.ParsesSettings",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_Playtest_CaptureParsesSettings::RunTest(const FString& Parameters)
{
	const FMordecaiPlaytestCaptureConfig Config = FMordecaiPlaytestCaptureConfig::FromCommandLine(
		TEXT("/MordecaiCore/Game/Maps/DevTestMap -PlaytestCapture -PlaytestShots=5 -PlaytestShotDelay=4.5 -PlaytestShotInterval=1.5 -PlaytestShotDir=\"C:/Play Tests/Run 1\" -log"));
	TestTrue("Enabled by -PlaytestCapture", Config.bEnabled);
	TestEqual("Shot count", Config.NumShots, 5);
	TestEqual("Delay after experience load", Config.DelaySeconds, 4.5f);
	TestEqual("Interval between shots", Config.IntervalSeconds, 1.5f);
	TestEqual("Quoted directory with spaces", Config.ShotDir, FString(TEXT("C:/Play Tests/Run 1")));
	return true;
}

// Mordecai.Playtest.Capture.DefaultsAndClamps
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_Playtest_CaptureDefaultsAndClamps,
	"Mordecai.Playtest.Capture.DefaultsAndClamps",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_Playtest_CaptureDefaultsAndClamps::RunTest(const FString& Parameters)
{
	const FMordecaiPlaytestCaptureConfig Defaults = FMordecaiPlaytestCaptureConfig::FromCommandLine(TEXT("-PlaytestCapture"));
	TestEqual("Default shot count", Defaults.NumShots, 3);
	TestEqual("Default delay", Defaults.DelaySeconds, 6.f);
	TestEqual("Default interval", Defaults.IntervalSeconds, 2.f);
	TestTrue("Default directory is under Saved/Playtests/Capture",
		Defaults.ShotDir.EndsWith(TEXT("Playtests/Capture")) && Defaults.ShotDir.StartsWith(FPaths::ProjectSavedDir()));

	const FMordecaiPlaytestCaptureConfig Clamped = FMordecaiPlaytestCaptureConfig::FromCommandLine(
		TEXT("-PlaytestCapture -PlaytestShots=500 -PlaytestShotDelay=-3 -PlaytestShotInterval=0"));
	TestEqual("Shot count clamped to 20", Clamped.NumShots, 20);
	TestEqual("Negative delay clamped to 0", Clamped.DelaySeconds, 0.f);
	TestEqual("Interval clamped to 0.1s", Clamped.IntervalSeconds, 0.1f);
	return true;
}

// Mordecai.Playtest.Capture.ShotPathsAndSchedule
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_Playtest_CaptureShotPathsAndSchedule,
	"Mordecai.Playtest.Capture.ShotPathsAndSchedule",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_Playtest_CaptureShotPathsAndSchedule::RunTest(const FString& Parameters)
{
	const FMordecaiPlaytestCaptureConfig Config = FMordecaiPlaytestCaptureConfig::FromCommandLine(
		TEXT("-PlaytestCapture -PlaytestShotDelay=6 -PlaytestShotInterval=2 -PlaytestShotDir=C:/Shots"));
	TestEqual("First shot path", Config.GetShotPath(0), FString(TEXT("C:/Shots/engine_01.png")));
	TestEqual("Third shot path", Config.GetShotPath(2), FString(TEXT("C:/Shots/engine_03.png")));
	TestEqual("First shot after the delay", Config.GetShotTime(0), 6.f);
	TestEqual("Third shot two intervals later", Config.GetShotTime(2), 10.f);
	return true;
}
