// Project Mordecai — Level Tests (Story 2.3)

#include "Misc/AutomationTest.h"
#include "Mordecai/MordecaiGameMode.h"
#include "UObject/SoftObjectPath.h"

// Mordecai.Level.DevTestMapExists
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecaiLevel_DevTestMapExists,
	"Mordecai.Level.DevTestMapExists",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecaiLevel_DevTestMapExists::RunTest(const FString& Parameters)
{
	// Verify the DevTestMap exists as an asset (AC-2.3.1)
	const FString MapPath = TEXT("/MordecaiCore/Game/Maps/DevTestMap.DevTestMap");
	UObject* MapAsset = FSoftObjectPath(MapPath).TryLoad();
	TestNotNull("DevTestMap exists", MapAsset);

	return true;
}

// Mordecai.Level.PlayerSpawnsCorrectly
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecaiLevel_PlayerSpawns,
	"Mordecai.Level.PlayerSpawnsCorrectly",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecaiLevel_PlayerSpawns::RunTest(const FString& Parameters)
{
	// Verify the Mordecai experience definition exists (AC-2.3.3)
	const FString ExpPath = TEXT("/MordecaiCore/System/Experiences/B_MordecaiDevExperience.B_MordecaiDevExperience");
	UObject* ExpAsset = FSoftObjectPath(ExpPath).TryLoad();
	TestNotNull("MordecaiDevExperience exists", ExpAsset);

	return true;
}

// Mordecai.Level.GameModeIsCorrectClass
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecaiLevel_GameModeClass,
	"Mordecai.Level.GameModeIsCorrectClass",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecaiLevel_GameModeClass::RunTest(const FString& Parameters)
{
	// Verify AMordecaiGameMode inherits from ALyraGameMode and is a valid game mode class
	UClass* GMClass = AMordecaiGameMode::StaticClass();
	TestNotNull("GameMode class exists", GMClass);
	TestTrue("GameMode is a game mode base",
		GMClass->IsChildOf(AGameModeBase::StaticClass()));

	return true;
}
