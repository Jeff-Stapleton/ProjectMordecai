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

// Mordecai.Playtest.Capture.LoadingShotPath (issue #39)
// The frame taken when the loading screen first shows: it is gone before the first scheduled shot.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_Playtest_CaptureLoadingShotPath,
	"Mordecai.Playtest.Capture.LoadingShotPath",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_Playtest_CaptureLoadingShotPath::RunTest(const FString& Parameters)
{
	const FMordecaiPlaytestCaptureConfig Config = FMordecaiPlaytestCaptureConfig::FromCommandLine(
		TEXT("-PlaytestCapture -PlaytestShotDir=C:/Shots"));
	TestEqualSensitive("Loading screen shot path", Config.GetLoadingShotPath(), FString(TEXT("C:/Shots/engine_loading.png")));
	TestFalse("Not one of the scheduled shots", Config.GetLoadingShotPath() == Config.GetShotPath(0));
	return true;
}

// Mordecai.Playtest.Capture.MarketingFrameSettings
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_Playtest_CaptureMarketingFrameSettings,
	"Mordecai.Playtest.Capture.MarketingFrameSettings",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_Playtest_CaptureMarketingFrameSettings::RunTest(const FString& Parameters)
{
	const FMordecaiPlaytestCaptureConfig Config = FMordecaiPlaytestCaptureConfig::FromCommandLine(
		TEXT("-MarketingFrameCapture -MarketingFrameDir=\"C:/Wall Fade/Frames\" -MarketingFrameLimit=450"));
	TestTrue("Marketing frame flag enables the subsystem", Config.bEnabled);
	TestFalse("Marketing frame capture is distinct from boot capture", Config.bPlaytestCapture);
	TestTrue("Marketing frame capture parsed", Config.bMarketingFrameCapture);
	TestEqual("Marketing frame limit", Config.MarketingFrameLimit, 450);
	TestEqual("Marketing frame directory", Config.MarketingFrameDir, FString(TEXT("C:/Wall Fade/Frames")));
	TestEqual("First marketing frame path", Config.GetMarketingFramePath(0),
		FString(TEXT("C:/Wall Fade/Frames/frame_000001.png")));
	TestEqual("Frame 450 path", Config.GetMarketingFramePath(449),
		FString(TEXT("C:/Wall Fade/Frames/frame_000450.png")));
	return true;
}

// Mordecai.Playtest.Capture.RejectsCombinedCaptureModes
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_Playtest_CaptureRejectsCombinedModes,
	"Mordecai.Playtest.Capture.RejectsCombinedCaptureModes",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_Playtest_CaptureRejectsCombinedModes::RunTest(const FString& Parameters)
{
	const FMordecaiPlaytestCaptureConfig Config =
		FMordecaiPlaytestCaptureConfig::FromCommandLine(
			TEXT("-PlaytestCapture -MarketingFrameCapture"));
	TestTrue("Combined capture modes are identified as conflicting",
		Config.bConflictingCaptureModes);
	TestFalse("Combined capture modes are refused", Config.bEnabled);
	return true;
}
