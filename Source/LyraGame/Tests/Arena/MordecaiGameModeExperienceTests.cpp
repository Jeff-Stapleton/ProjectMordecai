// Project Mordecai — Game mode experience selection tests
// The arena experience must load in packaged builds, not only in PIE. Lyra falls back to
// B_LyraDefaultExperience (the shooter) when nothing names an experience, so AMordecaiGameMode
// supplies its own default — but only when no higher-priority source already chose one.

#include "Misc/AutomationTest.h"
#include "Mordecai/MordecaiGameMode.h"

// Mordecai.GameMode.DefaultExperience.UsedWhenNothingSpecified
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_GameMode_DefaultExperienceUsedWhenNothingSpecified,
	"Mordecai.GameMode.DefaultExperience.UsedWhenNothingSpecified",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_GameMode_DefaultExperienceUsedWhenNothingSpecified::RunTest(const FString& Parameters)
{
	// Packaged build launched straight into DevTestMap: no URL option, no command line, no PIE, no World Settings experience
	TestTrue("Default experience is used when no source names one",
		AMordecaiGameMode::ShouldUseDefaultExperience(TEXT("?Name=Player"), TEXT("ProjectMordecai /MordecaiCore/Game/Maps/DevTestMap -log"), false, false));
	TestTrue("Empty options and command line also use the default",
		AMordecaiGameMode::ShouldUseDefaultExperience(TEXT(""), TEXT(""), false, false));
	return true;
}

// Mordecai.GameMode.DefaultExperience.UrlOptionWins
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_GameMode_DefaultExperienceUrlOptionWins,
	"Mordecai.GameMode.DefaultExperience.UrlOptionWins",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_GameMode_DefaultExperienceUrlOptionWins::RunTest(const FString& Parameters)
{
	// Front-end playlists travel with ?Experience=... in the URL
	TestFalse("URL Experience option is respected",
		AMordecaiGameMode::ShouldUseDefaultExperience(TEXT("?Experience=B_SomeOtherExperience"), TEXT(""), false, false));
	return true;
}

// Mordecai.GameMode.DefaultExperience.CommandLineWins
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_GameMode_DefaultExperienceCommandLineWins,
	"Mordecai.GameMode.DefaultExperience.CommandLineWins",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_GameMode_DefaultExperienceCommandLineWins::RunTest(const FString& Parameters)
{
	TestFalse("Command line Experience= is respected",
		AMordecaiGameMode::ShouldUseDefaultExperience(TEXT(""), TEXT("ProjectMordecai -log Experience=B_SomeOtherExperience"), false, false));
	return true;
}

// Mordecai.GameMode.DefaultExperience.PIEDeveloperOverrideWins
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_GameMode_DefaultExperiencePIEDeveloperOverrideWins,
	"Mordecai.GameMode.DefaultExperience.PIEDeveloperOverrideWins",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_GameMode_DefaultExperiencePIEDeveloperOverrideWins::RunTest(const FString& Parameters)
{
	TestFalse("PIE ExperienceOverride in developer settings is respected",
		AMordecaiGameMode::ShouldUseDefaultExperience(TEXT(""), TEXT(""), true, false));
	return true;
}

// Mordecai.GameMode.DefaultExperience.WorldSettingsWins
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_GameMode_DefaultExperienceWorldSettingsWins,
	"Mordecai.GameMode.DefaultExperience.WorldSettingsWins",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_GameMode_DefaultExperienceWorldSettingsWins::RunTest(const FString& Parameters)
{
	TestFalse("A map's World Settings DefaultGameplayExperience is respected",
		AMordecaiGameMode::ShouldUseDefaultExperience(TEXT(""), TEXT(""), false, true));
	return true;
}

// Mordecai.GameMode.DefaultExperience.IsArenaExperience
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_GameMode_DefaultExperienceIsArenaExperience,
	"Mordecai.GameMode.DefaultExperience.IsArenaExperience",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_GameMode_DefaultExperienceIsArenaExperience::RunTest(const FString& Parameters)
{
	const AMordecaiGameMode* CDO = GetDefault<AMordecaiGameMode>();
	TestNotNull("Game mode CDO exists", CDO);
	if (CDO)
	{
		TestEqual("Default experience type", CDO->DefaultExperience.PrimaryAssetType, FPrimaryAssetType(TEXT("LyraExperienceDefinition")));
		TestEqual("Default experience name", CDO->DefaultExperience.PrimaryAssetName, FName(TEXT("B_MordecaiArenaExperience")));
	}
	return true;
}
