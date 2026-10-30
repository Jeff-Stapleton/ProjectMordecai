// Project Mordecai — Recovery: the death screen is shown on death and removed on respawn (US-086, issue #7, N2)
//
// Nothing created UMordecaiDeathScreenWidget, so a dying player never saw "You Died". The player controller's
// UMordecaiDeathScreenComponent now pushes it onto the local player's UI.Layer.Game stack while Mordecai.State.Dead is
// on the PlayerState ASC, and removes it when the tag goes. These tests push through a real CommonUI layer stack in a
// test primary layout (the game's comes from the UI policy, which the headless world does not have), and read what
// the agent bridge reads: the non-empty TextBlocks in the widget's own tree.

#include "Misc/AutomationTest.h"
#include "Blueprint/WidgetTree.h"
#include "CommonInputModeTypes.h"
#include "Components/TextBlock.h"
#include "GameFramework/WorldSettings.h"
#include "Mordecai/MordecaiGameMode.h"
#include "Mordecai/MordecaiGameplayTags.h"
#include "Mordecai/UI/MordecaiDeathScreenComponent.h"
#include "Mordecai/UI/MordecaiDeathScreenWidget.h"
#include "Tests/Recovery/MordecaiRecoveryTestEnv.h"
#include "Tests/Recovery/MordecaiRecoveryTestLayout.h"

namespace MordecaiRecoveryDeathScreenTest
{
	/** The widget's non-empty TextBlock texts in tree order, as one string: "You Died | 3 | Kills: 0". */
	FString DrawnTexts(const UUserWidget* Widget)
	{
		TArray<FString> Texts;
		if (Widget && Widget->WidgetTree)
		{
			Widget->WidgetTree->ForEachWidget([&Texts](UWidget* Child)
			{
				if (const UTextBlock* Text = Cast<UTextBlock>(Child))
				{
					const FString String = Text->GetText().ToString();
					if (!String.IsEmpty())
					{
						Texts.Add(String);
					}
				}
			});
		}
		return FString::Join(Texts, TEXT(" | "));
	}

	/** The only death screen on the layer, or null (none, several, or another widget). */
	UMordecaiDeathScreenWidget* OnlyDeathScreen(const UCommonActivatableWidgetContainerBase* Layer)
	{
		const TArray<UCommonActivatableWidget*> Widgets = Layer->GetWidgetList();
		return Widgets.Num() == 1 ? Cast<UMordecaiDeathScreenWidget>(Widgets[0]) : nullptr;
	}
}

// ---------------------------------------------------------------------------
// 1. Mordecai.Recovery.DeathScreen.BuildsDefaultContent (AC-086.2)
// The native class has no designer tree: it builds "You Died", the countdown and the kill count itself.
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_Recovery_DeathScreen_BuildsDefaultContent,
	"Mordecai.Recovery.DeathScreen.BuildsDefaultContent",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_Recovery_DeathScreen_BuildsDefaultContent::RunTest(const FString& Parameters)
{
	using namespace MordecaiRecoveryTest;
	using namespace MordecaiRecoveryDeathScreenTest;

	FRecoveryControlledTestEnv Env;
	if (!TestTrue("test world with a controlled player set up", Env.SetupWithController()))
	{
		Env.TeardownWithController();
		return true;
	}

	UMordecaiDeathScreenWidget* Screen = Env.MakeWidget<UMordecaiDeathScreenWidget>();
	TestTrue("the widget has a tree with a root", Screen->WidgetTree && Screen->WidgetTree->RootWidget);
	if (Screen->WidgetTree)
	{
		TestNotNull("DeathText", Screen->WidgetTree->FindWidget<UTextBlock>(TEXT("DeathText")));
		TestNotNull("CountdownText", Screen->WidgetTree->FindWidget<UTextBlock>(TEXT("CountdownText")));
		TestNotNull("KillCountText", Screen->WidgetTree->FindWidget<UTextBlock>(TEXT("KillCountText")));
		if (Screen->WidgetTree->RootWidget)
		{
			TestTrue("an overlay: the game view keeps the clicks (root HitTestInvisible)",
				Screen->WidgetTree->RootWidget->GetVisibility() == ESlateVisibility::HitTestInvisible);
		}
	}
	TestEqualSensitive("built, before any setup", DrawnTexts(Screen), FString(TEXT("You Died")));

	Screen->InitializeDeathScreen(2.5f, 4);
	TestEqualSensitive("set up: the message, the ceiling countdown, the kills", DrawnTexts(Screen),
		FString(TEXT("You Died | 3 | Kills: 4")));

	Env.TeardownWithController();
	return true;
}

// ---------------------------------------------------------------------------
// 2. Mordecai.Recovery.DeathScreen.ShownOnDeathRemovedOnRespawn (AC-086.1)
// Lethal damage pushes one death screen onto UI.Layer.Game ("You Died", the game mode's respawn countdown and session
// kills); the respawn removes it. A later death reuses CommonUI's pooled instance, which must be set up again.
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_Recovery_DeathScreen_ShownOnDeathRemovedOnRespawn,
	"Mordecai.Recovery.DeathScreen.ShownOnDeathRemovedOnRespawn",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_Recovery_DeathScreen_ShownOnDeathRemovedOnRespawn::RunTest(const FString& Parameters)
{
	using namespace MordecaiRecoveryTest;
	using namespace MordecaiRecoveryDeathScreenTest;

	FRecoveryControlledTestEnv Env;
	if (!TestTrue("test world with a controlled player set up", Env.SetupWithController()))
	{
		Env.TeardownWithController();
		return true;
	}

	UMordecaiRecoveryTestLayout* Layout = Env.MakeWidget<UMordecaiRecoveryTestLayout>();
	UCommonActivatableWidgetContainerBase* GameLayer = Layout->AddTestLayer(FGameplayTag::RequestGameplayTag(TEXT("UI.Layer.Game")));

	UMordecaiDeathScreenComponent* Component = NewObject<UMordecaiDeathScreenComponent>(Env.PC);
	Component->RegisterComponent();
	Component->SetLayoutOverride(Layout);
	Component->BindToPlayerState();
	TestTrue("listening to the PlayerState ASC", Component->IsBoundToPlayerState());

	TestEqual("alive: nothing on the Game layer", GameLayer->GetNumWidgets(), 0);
	TestNull("alive: no death screen", Component->GetDeathScreenWidget());

	// Death. No game mode yet: the respawn delay is AMordecaiGameMode's default (3 s), the kill count 0.
	Env.ApplyPlayerHealthDelta(-1000.f);
	TestTrue("the player is dead", Env.Player->IsDead());
	UMordecaiDeathScreenWidget* Screen = OnlyDeathScreen(GameLayer);
	TestNotNull("dead: one death screen on the Game layer", Screen);
	TestTrue("the component's death screen is the pushed one", Screen && Component->GetDeathScreenWidget() == Screen);
	TestEqualSensitive("dead: the message, the countdown, the kills", DrawnTexts(Screen),
		FString(TEXT("You Died | 3 | Kills: 0")));

	Env.ApplyPlayerHealthDelta(-10.f);
	TestEqual("more damage while dead pushes nothing more", GameLayer->GetNumWidgets(), 1);

	Env.Player->HandlePlayerRespawn();
	TestFalse("the player is alive", Env.Player->IsDead());
	TestEqual("respawned: the Game layer is empty", GameLayer->GetNumWidgets(), 0);
	TestNull("respawned: no death screen", Component->GetDeathScreenWidget());

	// The second death reads the game mode: a 5 s respawn and 2 kills. The pool hands the first death's instance back,
	// so these new values also show that every push sets the screen up again.
	AMordecaiGameMode* GameMode = NewObject<AMordecaiGameMode>();
	GameMode->RespawnDelaySeconds = 5.f;
	GameMode->NotifyEnemyKilled(nullptr);
	GameMode->NotifyEnemyKilled(nullptr);
	Component->SetGameModeOverride(GameMode);
	Env.ApplyPlayerHealthDelta(-1000.f);
	UMordecaiDeathScreenWidget* SecondScreen = OnlyDeathScreen(GameLayer);
	TestNotNull("second death: one death screen", SecondScreen);
	TestEqualSensitive("second death: the game mode's respawn delay and kills", DrawnTexts(SecondScreen),
		FString(TEXT("You Died | 5 | Kills: 2")));
	Env.Player->HandlePlayerRespawn();
	TestEqual("second respawn: the Game layer is empty", GameLayer->GetNumWidgets(), 0);

	// Bound while already dead (possessed during a death): shown at once. Unbinding removes it.
	Component->Unbind();
	TestFalse("unbound", Component->IsBoundToPlayerState());
	Env.ApplyPlayerHealthDelta(-1000.f);
	TestEqual("unbound: a death pushes nothing", GameLayer->GetNumWidgets(), 0);
	Component->BindToPlayerState();
	TestNotNull("bound while dead: shown", OnlyDeathScreen(GameLayer));
	Component->Unbind();
	TestEqual("unbound: removed", GameLayer->GetNumWidgets(), 0);
	Env.Player->HandlePlayerRespawn();

	// The production lookup: a plain ULocalPlayer has no primary game layout, so nothing is shown and nothing asserts.
	Component->SetLayoutOverride(nullptr);
	Component->BindToPlayerState();
	Env.ApplyPlayerHealthDelta(-1000.f);
	TestNull("no primary layout: no death screen", Component->GetDeathScreenWidget());
	Env.Player->HandlePlayerRespawn();

	Component->Unbind();
	Env.TeardownWithController();
	return true;
}

// ---------------------------------------------------------------------------
// 3. Mordecai.Recovery.DeathScreen.CountdownHoldsWhilePaused (AC-062.1, issue #7 review)
// The pause menu can open while the player is dead. It pauses the world, and the respawn timer with it, but Slate
// keeps ticking the screen: the countdown must hold too, or it reaches nothing while the player is still dead.
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_Recovery_DeathScreen_CountdownHoldsWhilePaused,
	"Mordecai.Recovery.DeathScreen.CountdownHoldsWhilePaused",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_Recovery_DeathScreen_CountdownHoldsWhilePaused::RunTest(const FString& Parameters)
{
	using namespace MordecaiRecoveryTest;
	using namespace MordecaiRecoveryDeathScreenTest;

	FRecoveryControlledTestEnv Env;
	if (!TestTrue("test world with a controlled player set up", Env.SetupWithController()))
	{
		Env.TeardownWithController();
		return true;
	}

	UMordecaiDeathScreenWidget* Screen = Env.MakeWidget<UMordecaiDeathScreenWidget>();
	Screen->InitializeDeathScreen(3.f, 0);
	Screen->TickCountdown(1.5f);
	TestEqualSensitive("counts down while the game runs", DrawnTexts(Screen), FString(TEXT("You Died | 2 | Kills: 0")));

	Env.World->GetWorldSettings()->SetPauserPlayerState(Env.PlayerState);
	TestTrue("the world is paused", Env.World->IsPaused());
	Screen->TickCountdown(5.f);
	TestEqualSensitive("holds while paused", DrawnTexts(Screen), FString(TEXT("You Died | 2 | Kills: 0")));

	Env.World->GetWorldSettings()->SetPauserPlayerState(nullptr);
	Screen->TickCountdown(1.f);
	TestEqualSensitive("counts on after the pause", DrawnTexts(Screen), FString(TEXT("You Died | 1 | Kills: 0")));

	Env.TeardownWithController();
	return true;
}

// ---------------------------------------------------------------------------
// 4. Mordecai.Recovery.DeathScreen.KeepsTheViewportCaptureMode (AC-062.3, issue #7 review)
// The active screen's input config is applied to the game viewport, and CommonUI restores nothing when it goes: it
// must use the project's capture mode (DefaultInput.ini), or after a death the click that refocuses the game only
// captures the mouse and does not attack.
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_Recovery_DeathScreen_KeepsTheViewportCaptureMode,
	"Mordecai.Recovery.DeathScreen.KeepsTheViewportCaptureMode",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_Recovery_DeathScreen_KeepsTheViewportCaptureMode::RunTest(const FString& Parameters)
{
	const UMordecaiDeathScreenWidget* Screen = GetDefault<UMordecaiDeathScreenWidget>();
	const TOptional<FUIInputConfig> Config = Screen->GetDesiredInputConfig();
	if (TestTrue("the death screen asks for an input config", Config.IsSet()))
	{
		TestTrue("game and menu input both pass (respawn flow not blocked)", Config->GetInputMode() == ECommonInputMode::All);
		TestTrue("the project's viewport capture mode",
			Config->GetMouseCaptureMode() == EMouseCaptureMode::CapturePermanently_IncludingInitialMouseDown);
	}
	return true;
}
