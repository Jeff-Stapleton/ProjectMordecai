// Project Mordecai — Loading Screen Tests (issue #39)
//
// The packaged game showed Lyra's loading screen (the LYRA logo and a debug line) on every load: Config/DefaultGame.ini
// pointed CommonLoadingScreenSettings.LoadingScreenWidget at Lyra's W_LoadingScreen_Host. The Mordecai loading screen
// is a native widget that builds its own tree. ULoadingScreenManager creates it with
// UUserWidget::CreateWidgetInstance(GameInstance), which gives it a player context only when the game instance's first
// local player has a player controller. Nothing guarantees that, so these tests create it the same way from a game
// instance without a local player: the case where NativeOnInitialized does not run.

#include "Misc/AutomationTest.h"
#include "Blueprint/UserWidget.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Engine/BlueprintGeneratedClass.h"
#include "Engine/GameInstance.h"
#include "Mordecai/UI/MordecaiLoadingScreenWidget.h"
#include "Tests/AutomationCommon.h"

namespace MordecaiLoadingScreenTest
{
	/** The widget's non-empty TextBlock texts in tree order, as one string: "Project Mordecai | Loading...". */
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

	/** A game instance with no local player: the boot case, where the widget gets no player context. */
	struct FLoadingScreenTestEnv
	{
		FTestWorldWrapper WorldWrapper;
		UGameInstance* GameInstance = nullptr;

		bool Setup(FAutomationTestBase& Test)
		{
			if (!WorldWrapper.CreateTestWorld(EWorldType::Game))
			{
				WorldWrapper.ForwardErrorMessages(&Test);
				return false;
			}
			UWorld* World = WorldWrapper.GetTestWorld();
			GameInstance = World ? World->GetGameInstance() : nullptr;
			return GameInstance != nullptr;
		}

		/** What ULoadingScreenManager::ShowLoadingScreen does (LoadingScreenManager.cpp). */
		UUserWidget* CreateLikeTheManager() const
		{
			return UUserWidget::CreateWidgetInstance(*GameInstance, UMordecaiLoadingScreenWidget::StaticClass(), NAME_None);
		}
	};
}

// ---------------------------------------------------------------------------
// 1. Mordecai.UI.LoadingScreen.IsTheConfiguredLoadingScreen
// CommonLoadingScreenSettings.LoadingScreenWidget names the native Mordecai widget: not Lyra's W_LoadingScreen_Host
// (the LYRA logo), and no Blueprint, so nothing has to be cooked for it.
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_UI_LoadingScreen_IsTheConfiguredLoadingScreen,
	"Mordecai.UI.LoadingScreen.IsTheConfiguredLoadingScreen",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_UI_LoadingScreen_IsTheConfiguredLoadingScreen::RunTest(const FString& Parameters)
{
	const TSubclassOf<UUserWidget> Configured = UMordecaiLoadingScreenWidget::GetConfiguredLoadingScreenWidgetClass();
	if (!TestNotNull("CommonLoadingScreenSettings.LoadingScreenWidget loads a widget class", Configured.Get()))
	{
		return true;
	}
	TestEqualSensitive("the loading screen manager shows the Mordecai loading screen", Configured->GetPathName(),
		UMordecaiLoadingScreenWidget::StaticClass()->GetPathName());
	TestNull("a native class, not a Blueprint", Cast<UBlueprintGeneratedClass>(Configured.Get()));
	return true;
}

// ---------------------------------------------------------------------------
// 2. Mordecai.UI.LoadingScreen.BuildsItsTreeWithoutAPlayer
// Created the manager's way from a game instance without a local player (no player context, so no
// NativeOnInitialized): the tree is still there, with the game's name and the status line.
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_UI_LoadingScreen_BuildsItsTreeWithoutAPlayer,
	"Mordecai.UI.LoadingScreen.BuildsItsTreeWithoutAPlayer",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_UI_LoadingScreen_BuildsItsTreeWithoutAPlayer::RunTest(const FString& Parameters)
{
	using namespace MordecaiLoadingScreenTest;

	FLoadingScreenTestEnv Env;
	if (!TestTrue("test world with a game instance", Env.Setup(*this)))
	{
		return true;
	}
	TestNull("the boot case: no local player yet", Env.GameInstance->GetFirstGamePlayer());

	UUserWidget* Screen = Env.CreateLikeTheManager();
	if (!TestNotNull("CreateWidgetInstance made the widget", Screen))
	{
		return true;
	}
	TestTrue("the widget has a tree with a root", Screen->WidgetTree && Screen->WidgetTree->RootWidget);
	if (Screen->WidgetTree)
	{
		TestNotNull("TitleText", Screen->WidgetTree->FindWidget<UTextBlock>(TEXT("TitleText")));
		TestNotNull("StatusText", Screen->WidgetTree->FindWidget<UTextBlock>(TEXT("StatusText")));
	}
	TestEqualSensitive("the game's name and the status line", DrawnTexts(Screen), FString(TEXT("Project Mordecai | Loading...")));
	TestEqualSensitive("the title comes from GetTitleText", UMordecaiLoadingScreenWidget::GetTitleText().ToString(),
		FString(TEXT("Project Mordecai")));
	TestEqualSensitive("the status line comes from GetStatusText", UMordecaiLoadingScreenWidget::GetStatusText().ToString(),
		FString(TEXT("Loading...")));
	return true;
}

// ---------------------------------------------------------------------------
// 3. Mordecai.UI.LoadingScreen.BuildsItsTreeOnce
// Initialize runs again for a widget that is already initialized: the tree is built once.
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_UI_LoadingScreen_BuildsItsTreeOnce,
	"Mordecai.UI.LoadingScreen.BuildsItsTreeOnce",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_UI_LoadingScreen_BuildsItsTreeOnce::RunTest(const FString& Parameters)
{
	using namespace MordecaiLoadingScreenTest;

	FLoadingScreenTestEnv Env;
	if (!TestTrue("test world with a game instance", Env.Setup(*this)))
	{
		return true;
	}
	UUserWidget* Screen = Env.CreateLikeTheManager();
	if (!TestNotNull("CreateWidgetInstance made the widget", Screen) || !TestNotNull("tree", Screen->WidgetTree.Get()))
	{
		return true;
	}
	UTextBlock* Title = Screen->WidgetTree->FindWidget<UTextBlock>(TEXT("TitleText"));
	if (!TestNotNull("TitleText", Title))
	{
		return true;
	}
	// A rebuild reuses the widgets' names, so it replaces each one in place (same address, same count): only the
	// state shows it. A second build would reset this text to the title.
	Title->SetText(FText::FromString(TEXT("built once")));

	TestFalse("a second Initialize reports nothing new to initialize", Screen->Initialize());
	TestEqualSensitive("the tree was not built again", DrawnTexts(Screen), FString(TEXT("built once | Loading...")));
	return true;
}

// ---------------------------------------------------------------------------
// 4. Mordecai.UI.LoadingScreen.OpaqueFullScreenBackground
// The first child of the root canvas, drawn under everything else, is an opaque border stretched over the whole
// screen: while the manager holds the screen up it renders the world again, and nothing of it may show through.
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_UI_LoadingScreen_OpaqueFullScreenBackground,
	"Mordecai.UI.LoadingScreen.OpaqueFullScreenBackground",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_UI_LoadingScreen_OpaqueFullScreenBackground::RunTest(const FString& Parameters)
{
	using namespace MordecaiLoadingScreenTest;

	FLoadingScreenTestEnv Env;
	if (!TestTrue("test world with a game instance", Env.Setup(*this)))
	{
		return true;
	}
	UUserWidget* Screen = Env.CreateLikeTheManager();
	const UCanvasPanel* Root = (Screen && Screen->WidgetTree) ? Cast<UCanvasPanel>(Screen->WidgetTree->RootWidget) : nullptr;
	if (!TestNotNull("the root is a canvas panel", Root) || !TestTrue("the canvas has children", Root->GetChildrenCount() > 0))
	{
		return true;
	}
	const UBorder* Background = Cast<UBorder>(Root->GetChildAt(0));
	if (!TestNotNull("the first (bottom) child is a border", Background))
	{
		return true;
	}
	TestEqual("the background is opaque", Background->GetBrushColor().A, 1.f);
	const UCanvasPanelSlot* BackgroundSlot = Cast<UCanvasPanelSlot>(Background->Slot);
	if (TestNotNull("the background sits in a canvas slot", BackgroundSlot))
	{
		const FAnchors Anchors = BackgroundSlot->GetAnchors();
		TestTrue("anchored to the whole screen",
			Anchors.Minimum == FVector2D(0.f, 0.f) && Anchors.Maximum == FVector2D(1.f, 1.f));
		TestTrue("no margin", BackgroundSlot->GetOffsets() == FMargin(0.f));
	}
	return true;
}

// ---------------------------------------------------------------------------
// 5. Mordecai.UI.LoadingScreen.NoLyraBranding
// Nothing of Lyra's screen: no nested user widget (the logo was W_LyraLogo_LoadingScreen), no image (the logo
// textures and materials), no text naming Lyra.
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_UI_LoadingScreen_NoLyraBranding,
	"Mordecai.UI.LoadingScreen.NoLyraBranding",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_UI_LoadingScreen_NoLyraBranding::RunTest(const FString& Parameters)
{
	using namespace MordecaiLoadingScreenTest;

	FLoadingScreenTestEnv Env;
	if (!TestTrue("test world with a game instance", Env.Setup(*this)))
	{
		return true;
	}
	UUserWidget* Screen = Env.CreateLikeTheManager();
	if (!TestNotNull("CreateWidgetInstance made the widget", Screen) || !TestNotNull("tree", Screen->WidgetTree.Get()))
	{
		return true;
	}
	TArray<UWidget*> Widgets;
	Screen->WidgetTree->GetAllWidgets(Widgets);
	TestTrue("the screen has content", Widgets.Num() > 0);
	for (const UWidget* Widget : Widgets)
	{
		const FString Name = Widget->GetName();
		TestFalse(*FString::Printf(TEXT("%s is not a nested user widget"), *Name), Widget->IsA<UUserWidget>());
		TestFalse(*FString::Printf(TEXT("%s is not an image"), *Name), Widget->IsA<UImage>());
		if (const UTextBlock* Text = Cast<UTextBlock>(Widget))
		{
			TestFalse(*FString::Printf(TEXT("%s does not name Lyra"), *Name),
				Text->GetText().ToString().Contains(TEXT("Lyra"), ESearchCase::IgnoreCase));
		}
	}
	return true;
}
