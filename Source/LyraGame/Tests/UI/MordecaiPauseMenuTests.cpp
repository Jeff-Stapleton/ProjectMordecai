// Project Mordecai — Pause Menu Tests (US-069)
// Tests the pure data models (FMordecaiPauseMenuState, FMordecaiPauseMenuTabModel)
// and subsystem state management. All NullRHI-compatible.

#include "Misc/AutomationTest.h"
#include "Mordecai/UI/MordecaiPauseMenuTypes.h"
#include "Mordecai/UI/MordecaiPauseMenuPlaceholderWidget.h"

// ---------------------------------------------------------------------------
// 1. Mordecai.UI.PauseMenu.OpensOnToggle (AC-069.5, AC-069.6)
// TogglePauseMenu when closed -> menu opens
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_UI_PauseMenu_OpensOnToggle,
	"Mordecai.UI.PauseMenu.OpensOnToggle",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_UI_PauseMenu_OpensOnToggle::RunTest(const FString& Parameters)
{
	FMordecaiPauseMenuState State;
	TestFalse("Initially closed", State.bIsOpen);

	const bool bChanged = State.Toggle(NAME_None);
	TestTrue("Toggle returned true (state changed)", bChanged);
	TestTrue("Menu is now open", State.bIsOpen);

	return true;
}

// ---------------------------------------------------------------------------
// 2. Mordecai.UI.PauseMenu.ClosesOnToggle (AC-069.6)
// TogglePauseMenu when open -> menu closes
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_UI_PauseMenu_ClosesOnToggle,
	"Mordecai.UI.PauseMenu.ClosesOnToggle",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_UI_PauseMenu_ClosesOnToggle::RunTest(const FString& Parameters)
{
	FMordecaiPauseMenuState State;
	State.Open();
	TestTrue("Menu is open", State.bIsOpen);

	const bool bChanged = State.Toggle(FName("skills"));
	TestTrue("Toggle returned true (state changed)", bChanged);
	TestFalse("Menu is now closed", State.bIsOpen);

	return true;
}

// ---------------------------------------------------------------------------
// 3. Mordecai.UI.PauseMenu.PausesGameOnOpen (AC-069.6)
// When menu opens with bCanPauseGame=true, game should be paused
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_UI_PauseMenu_PausesGameOnOpen,
	"Mordecai.UI.PauseMenu.PausesGameOnOpen",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_UI_PauseMenu_PausesGameOnOpen::RunTest(const FString& Parameters)
{
	FMordecaiPauseMenuState State;
	State.bCanPauseGame = true;

	TestFalse("Not paused initially", State.ShouldBePaused());

	State.Open();
	TestTrue("Should be paused after open", State.ShouldBePaused());

	return true;
}

// ---------------------------------------------------------------------------
// 4. Mordecai.UI.PauseMenu.UnpausesGameOnClose (AC-069.6)
// When menu closes, game should be unpaused
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_UI_PauseMenu_UnpausesGameOnClose,
	"Mordecai.UI.PauseMenu.UnpausesGameOnClose",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_UI_PauseMenu_UnpausesGameOnClose::RunTest(const FString& Parameters)
{
	FMordecaiPauseMenuState State;
	State.bCanPauseGame = true;

	State.Open();
	TestTrue("Paused while open", State.ShouldBePaused());

	State.Close(NAME_None);
	TestFalse("Not paused after close", State.ShouldBePaused());

	return true;
}

// ---------------------------------------------------------------------------
// 5. Mordecai.UI.PauseMenu.HasDefaultTabs (AC-069.2)
// Menu has 5 default tabs with correct ids
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_UI_PauseMenu_HasDefaultTabs,
	"Mordecai.UI.PauseMenu.HasDefaultTabs",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_UI_PauseMenu_HasDefaultTabs::RunTest(const FString& Parameters)
{
	FMordecaiPauseMenuTabModel Model;
	Model.InitDefaults(UMordecaiPauseMenuPlaceholderWidget::StaticClass());

	TestEqual("5 default tabs", Model.GetTabCount(), 5);
	TestTrue("Has character tab", Model.HasTab(FName("character")));
	TestTrue("Has skills tab", Model.HasTab(FName("skills")));
	TestTrue("Has feats tab", Model.HasTab(FName("feats")));
	TestTrue("Has inventory tab", Model.HasTab(FName("inventory")));
	TestTrue("Has settings tab", Model.HasTab(FName("settings")));

	// First tab is active by default
	TestEqual("character is active", Model.ActiveTabId, FName("character"));

	// All default tabs use placeholder content
	for (const FMordecaiPauseMenuTabEntry& Tab : Model.Tabs)
	{
		TestEqual(
			FString::Printf(TEXT("Tab '%s' uses placeholder"), *Tab.TabId.ToString()),
			Tab.ContentWidgetClass.Get(),
			UMordecaiPauseMenuPlaceholderWidget::StaticClass());
	}

	return true;
}

// ---------------------------------------------------------------------------
// 6. Mordecai.UI.PauseMenu.TabSwitchChangesContent (AC-069.3, AC-069.4)
// Switching active tab changes the tracked active tab
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_UI_PauseMenu_TabSwitchChangesContent,
	"Mordecai.UI.PauseMenu.TabSwitchChangesContent",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_UI_PauseMenu_TabSwitchChangesContent::RunTest(const FString& Parameters)
{
	FMordecaiPauseMenuTabModel Model;
	Model.InitDefaults(UMordecaiPauseMenuPlaceholderWidget::StaticClass());

	TestEqual("Initially on character", Model.ActiveTabId, FName("character"));

	const bool bSwitched = Model.SetActiveTab(FName("skills"));
	TestTrue("SetActiveTab returned true", bSwitched);
	TestEqual("Now on skills", Model.ActiveTabId, FName("skills"));

	const bool bSwitched2 = Model.SetActiveTab(FName("settings"));
	TestTrue("SetActiveTab returned true", bSwitched2);
	TestEqual("Now on settings", Model.ActiveTabId, FName("settings"));

	// Switching to non-existent tab should fail
	const bool bBadSwitch = Model.SetActiveTab(FName("nonexistent"));
	TestFalse("Bad tab switch returns false", bBadSwitch);
	TestEqual("Still on settings", Model.ActiveTabId, FName("settings"));

	return true;
}

// ---------------------------------------------------------------------------
// 7. Mordecai.UI.PauseMenu.RegisterTabReplacesPlaceholder (AC-069.8)
// Registering a content class for a tab replaces its placeholder
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_UI_PauseMenu_RegisterTabReplacesPlaceholder,
	"Mordecai.UI.PauseMenu.RegisterTabReplacesPlaceholder",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_UI_PauseMenu_RegisterTabReplacesPlaceholder::RunTest(const FString& Parameters)
{
	FMordecaiPauseMenuTabModel Model;
	Model.InitDefaults(UMordecaiPauseMenuPlaceholderWidget::StaticClass());

	// Before registration: placeholder
	const FMordecaiPauseMenuTabEntry* Tab = Model.FindTab(FName("character"));
	TestNotNull("character tab exists", Tab);
	if (Tab)
	{
		TestEqual("Initially placeholder", Tab->ContentWidgetClass.Get(), UMordecaiPauseMenuPlaceholderWidget::StaticClass());
	}

	// Register with a different class (UUserWidget as stand-in)
	Model.RegisterTab(FName("character"), FText::FromString(TEXT("Character")), UUserWidget::StaticClass());

	// After registration: UUserWidget
	Tab = Model.FindTab(FName("character"));
	TestNotNull("character tab still exists", Tab);
	if (Tab)
	{
		TestEqual("Now uses custom class", Tab->ContentWidgetClass.Get(), UUserWidget::StaticClass());
	}

	// Tab count unchanged
	TestEqual("Still 5 tabs", Model.GetTabCount(), 5);

	return true;
}

// ---------------------------------------------------------------------------
// 8. Mordecai.UI.PauseMenu.UnregisterTabRemoves (AC-069.9)
// Unregistering a tab removes it from the tab bar
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_UI_PauseMenu_UnregisterTabRemoves,
	"Mordecai.UI.PauseMenu.UnregisterTabRemoves",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_UI_PauseMenu_UnregisterTabRemoves::RunTest(const FString& Parameters)
{
	FMordecaiPauseMenuTabModel Model;
	Model.InitDefaults(UMordecaiPauseMenuPlaceholderWidget::StaticClass());

	TestEqual("5 tabs initially", Model.GetTabCount(), 5);
	TestTrue("settings exists", Model.HasTab(FName("settings")));

	const bool bRemoved = Model.UnregisterTab(FName("settings"));
	TestTrue("UnregisterTab returned true", bRemoved);
	TestEqual("4 tabs after removal", Model.GetTabCount(), 4);
	TestFalse("settings no longer exists", Model.HasTab(FName("settings")));

	// Removing non-existent tab returns false
	const bool bBadRemove = Model.UnregisterTab(FName("nonexistent"));
	TestFalse("Removing non-existent returns false", bBadRemove);
	TestEqual("Still 4 tabs", Model.GetTabCount(), 4);

	return true;
}

// ---------------------------------------------------------------------------
// 9. Mordecai.UI.PauseMenu.RemembersLastTab (AC-069.11)
// Reopening the menu restores the last active tab
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_UI_PauseMenu_RemembersLastTab,
	"Mordecai.UI.PauseMenu.RemembersLastTab",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_UI_PauseMenu_RemembersLastTab::RunTest(const FString& Parameters)
{
	FMordecaiPauseMenuState State;

	// Open, "switch to skills", close
	State.Open();
	State.Close(FName("skills"));
	TestEqual("Last tab is skills", State.LastActiveTabId, FName("skills"));

	// Open again, "switch to feats", close
	State.Open();
	State.Close(FName("feats"));
	TestEqual("Last tab is feats", State.LastActiveTabId, FName("feats"));

	return true;
}

// ---------------------------------------------------------------------------
// 10. Mordecai.UI.PauseMenu.DoubleOpenIsNoOp (AC-069.12)
// Opening when already open does nothing
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_UI_PauseMenu_DoubleOpenIsNoOp,
	"Mordecai.UI.PauseMenu.DoubleOpenIsNoOp",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_UI_PauseMenu_DoubleOpenIsNoOp::RunTest(const FString& Parameters)
{
	FMordecaiPauseMenuState State;

	const bool bFirst = State.Open();
	TestTrue("First open succeeds", bFirst);
	TestTrue("Is open", State.bIsOpen);

	const bool bSecond = State.Open();
	TestFalse("Second open is no-op", bSecond);
	TestTrue("Still open", State.bIsOpen);

	return true;
}

// ---------------------------------------------------------------------------
// 11. Mordecai.UI.PauseMenu.DoubleCloseIsNoOp (AC-069.12)
// Closing when already closed does nothing
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_UI_PauseMenu_DoubleCloseIsNoOp,
	"Mordecai.UI.PauseMenu.DoubleCloseIsNoOp",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_UI_PauseMenu_DoubleCloseIsNoOp::RunTest(const FString& Parameters)
{
	FMordecaiPauseMenuState State;

	// Open first, then close twice
	State.Open();
	TestTrue("Is open", State.bIsOpen);

	const bool bFirst = State.Close(NAME_None);
	TestTrue("First close succeeds", bFirst);
	TestFalse("Is closed", State.bIsOpen);

	const bool bSecond = State.Close(NAME_None);
	TestFalse("Second close is no-op", bSecond);
	TestFalse("Still closed", State.bIsOpen);

	return true;
}

// ---------------------------------------------------------------------------
// 12. Mordecai.UI.PauseMenu.InventoryTabRegistered (US-079, AC-079.8)
// InitializeTabs (the NativeOnInitialized hook) registers the real inventory
// widget as the "inventory" tab content, alongside the character sheet.
// ---------------------------------------------------------------------------
#include "Mordecai/UI/MordecaiCharacterSheetWidget.h"
#include "Mordecai/UI/MordecaiInventoryWidget.h"
#include "Mordecai/UI/MordecaiPauseMenuWidget.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_UI_PauseMenu_InventoryTabRegistered,
	"Mordecai.UI.PauseMenu.InventoryTabRegistered",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_UI_PauseMenu_InventoryTabRegistered::RunTest(const FString& Parameters)
{
	UMordecaiPauseMenuWidget* Menu = NewObject<UMordecaiPauseMenuWidget>(GetTransientPackage());
	Menu->InitializeTabs();

	const FMordecaiPauseMenuTabEntry* InventoryTab = Menu->GetTabModel().FindTab(UMordecaiInventoryWidget::GetInventoryTabId());
	TestNotNull("'inventory' tab exists", InventoryTab);
	if (InventoryTab)
	{
		TestTrue("'inventory' tab content is the real inventory widget class",
			InventoryTab->ContentWidgetClass == UMordecaiInventoryWidget::StaticClass());
	}

	const FMordecaiPauseMenuTabEntry* CharacterTab = Menu->GetTabModel().FindTab(UMordecaiCharacterSheetWidget::GetCharacterTabId());
	TestNotNull("'character' tab still registered", CharacterTab);
	if (CharacterTab)
	{
		TestTrue("'character' tab content is the character sheet",
			CharacterTab->ContentWidgetClass == UMordecaiCharacterSheetWidget::StaticClass());
	}

	return true;
}
