// Project Mordecai — Recovery: the HUD kill counter follows the arena's authoritative count (US-086, issue #6, N1)

#include "Misc/AutomationTest.h"
#include "Mordecai/MordecaiCharacter.h"
#include "Mordecai/MordecaiGameMode.h"
#include "Mordecai/UI/MordecaiKillCounterWidget.h"

// ---------------------------------------------------------------------------
// 1. Mordecai.Recovery.KillCounter.IncrementsOnPlayerKill (AC-086.3)
// The widget does not keep a second count: each game-mode notification publishes the authoritative value.
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_Recovery_KillCounter_IncrementsOnPlayerKill,
	"Mordecai.Recovery.KillCounter.IncrementsOnPlayerKill",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_Recovery_KillCounter_IncrementsOnPlayerKill::RunTest(const FString& Parameters)
{
	AMordecaiGameMode* GameMode = NewObject<AMordecaiGameMode>();
	UMordecaiKillCounterWidget* Counter = NewObject<UMordecaiKillCounterWidget>();
	Counter->BindToGameMode(GameMode);

	TestEqual("the game mode starts at zero", GameMode->GetEnemyKillCount(), 0);
	TestEqual("the widget starts at the same value", Counter->GetKillCount(), GameMode->GetEnemyKillCount());

	GameMode->NotifyEnemyKilled(nullptr);
	TestEqual("the game mode records the first player kill", GameMode->GetEnemyKillCount(), 1);
	TestEqual("the widget shows the game mode's first kill", Counter->GetKillCount(), GameMode->GetEnemyKillCount());

	GameMode->NotifyEnemyKilled(nullptr);
	TestEqual("the game mode records the second player kill", GameMode->GetEnemyKillCount(), 2);
	TestEqual("the widget still shows the one source of truth", Counter->GetKillCount(), GameMode->GetEnemyKillCount());

	return true;
}

// ---------------------------------------------------------------------------
// 2. Mordecai.Recovery.KillCounter.ResetsOnPlayerDeath (AC-086.3 / AC-063.2)
// AMordecaiCharacter emits Mordecai.Event.PlayerDeath immediately before this game-mode notification.
// The life count and every bound counter reset together; the death screen already cached the completed life's count.
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_Recovery_KillCounter_ResetsOnPlayerDeath,
	"Mordecai.Recovery.KillCounter.ResetsOnPlayerDeath",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_Recovery_KillCounter_ResetsOnPlayerDeath::RunTest(const FString& Parameters)
{
	AMordecaiGameMode* GameMode = NewObject<AMordecaiGameMode>();
	GameMode->NotifyEnemyKilled(nullptr);
	GameMode->NotifyEnemyKilled(nullptr);

	UMordecaiKillCounterWidget* Counter = NewObject<UMordecaiKillCounterWidget>();
	Counter->BindToGameMode(GameMode);
	TestEqual("a late-bound widget reads the current life count", Counter->GetKillCount(), 2);

	AMordecaiCharacter* Player = NewObject<AMordecaiCharacter>();
	GameMode->NotifyPlayerDeath(Player);
	TestEqual("player death resets the authoritative life count", GameMode->GetEnemyKillCount(), 0);
	TestEqual("the widget resets from that same notification", Counter->GetKillCount(), GameMode->GetEnemyKillCount());

	return true;
}
