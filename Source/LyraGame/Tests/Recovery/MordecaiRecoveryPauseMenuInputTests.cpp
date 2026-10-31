// Project Mordecai — Recovery: pause menu input — I closes the menu, LB/RB switch tabs (US-095, issues #23 and #32)
//
// While the pause menu is open, CommonUI's Menu input mode blocks every key that has no UI action binding: the Enhanced
// Input actions (I = Inventory, LB/RB = spells) never fire. The menu registers CommonUI UI action bindings for them in
// NativeOnInitialized. Headless there is no CommonUI action router (the test local player runs no PlayerAdded, so it
// has no local player subsystems), so RegisterUIActionBinding returns invalid handles. These tests create exactly the
// bindings the menu registers through FUIActionBinding::TryCreate, the call the router makes, and drive them with an
// emulation of the router's key matching (FActionRouterBindingCollection::ProcessNormalInput). The live router, Slate
// routing and the packaged config are proven by the Tier 4 checks i_closes, shoulder_switches_tab and
// shoulder_rb_switches_tab of Scripts/playtests/us069_pause_menu.py.

#include "Misc/AutomationTest.h"
#include "Blueprint/WidgetTree.h"
#include "Components/HorizontalBox.h"
#include "Engine/EngineBaseTypes.h"
#include "Input/CommonUIInputSettings.h"
#include "Input/CommonUIInputTypes.h"
#include "Input/UIActionBinding.h"
#include "InputCoreTypes.h"
#include "Mordecai/MordecaiGameplayTags.h"
#include "Mordecai/UI/MordecaiCharacterSheetWidget.h"
#include "Mordecai/UI/MordecaiInventoryWidget.h"
#include "Mordecai/UI/MordecaiPauseMenuPlaceholderWidget.h"
#include "Mordecai/UI/MordecaiPauseMenuSubsystem.h"
#include "Mordecai/UI/MordecaiPauseMenuWidget.h"
#include "Mordecai/UI/MordecaiTabButton.h"
#include "Tests/AutomationCommon.h"
#include "Tests/Recovery/MordecaiRecoveryTestEnv.h"
#include "UITag.h"

namespace MordecaiRecoveryPauseMenuInputTest
{
	/** Tab-bar order (FMordecaiPauseMenuTabModel::InitDefaults, AC-069.2). */
	const TArray<FName>& TabOrder()
	{
		static const TArray<FName> Order = { TEXT("character"), TEXT("skills"), TEXT("feats"), TEXT("inventory"), TEXT("settings") };
		return Order;
	}

	/**
	 * The pause menu's UI action bindings, created the way UCommonUIActionRouterBase::RegisterUIActionBinding creates
	 * them (FUIActionBinding::TryCreate for local player 0), and unregistered when this goes out of scope so the static
	 * registration map holds nothing for the next test.
	 */
	struct FMenuActionBindings
	{
		TArray<FUIActionBindingHandle> Handles;

		explicit FMenuActionBindings(UMordecaiPauseMenuWidget& Menu)
		{
			for (const FBindUIActionArgs& BindArgs : Menu.MakeUIActionBindingArgs())
			{
				Handles.Add(FUIActionBinding::TryCreate(Menu, BindArgs, /*UserIndex*/ 0));
			}
		}

		~FMenuActionBindings()
		{
			for (FUIActionBindingHandle& Handle : Handles)
			{
				Handle.Unregister();
			}
		}

		FMenuActionBindings(const FMenuActionBindings&) = delete;
		FMenuActionBindings& operator=(const FMenuActionBindings&) = delete;

		int32 NumValid() const
		{
			int32 Count = 0;
			for (const FUIActionBindingHandle& Handle : Handles)
			{
				Count += FUIActionBinding::FindBinding(Handle).IsValid() ? 1 : 0;
			}
			return Count;
		}

		TSharedPtr<FUIActionBinding> Find(const FGameplayTag& ActionTag, EInputEvent InputEvent = IE_Pressed) const
		{
			for (const FUIActionBindingHandle& Handle : Handles)
			{
				TSharedPtr<FUIActionBinding> Binding = FUIActionBinding::FindBinding(Handle);
				if (Binding && Binding->ActionName == ActionTag.GetTagName() && Binding->InputEvent == InputEvent)
				{
					return Binding;
				}
			}
			return nullptr;
		}

		/**
		 * Emulation of FActionRouterBindingCollection::ProcessNormalInput (UIActionRouterTypes.cpp) for a Menu-mode root:
		 * a binding fires when the active input mode accepts it (Menu == its InputMode), one of its normal key mappings
		 * is the key and its input event is the event; a consuming binding ends the search. Returns what fired.
		 */
		TArray<FName> Dispatch(const FKey& Key, EInputEvent InputEvent) const
		{
			TArray<FName> Fired;
			for (const FUIActionBindingHandle& Handle : Handles)
			{
				TSharedPtr<FUIActionBinding> Binding = FUIActionBinding::FindBinding(Handle);
				if (!Binding || Binding->InputMode != ECommonInputMode::Menu || Binding->InputEvent != InputEvent)
				{
					continue;
				}
				for (const FUIActionKeyMapping& Mapping : Binding->NormalMappings)
				{
					if (Mapping.Key == Key)
					{
						Fired.Add(Binding->ActionName);
						Binding->OnExecuteAction.ExecuteIfBound();
						if (Binding->bConsumesInput)
						{
							return Fired;
						}
						break;
					}
				}
			}
			return Fired;
		}
	};

	FString Names(const TArray<FName>& InNames)
	{
		return FString::JoinBy(InNames, TEXT(", "), [](const FName& Name) { return Name.ToString(); });
	}

	/** The keys a binding's normal (press, no hold) mappings name. */
	TArray<FKey> NormalKeys(const TSharedPtr<FUIActionBinding>& Binding)
	{
		TArray<FKey> Keys;
		if (Binding)
		{
			for (const FUIActionKeyMapping& Mapping : Binding->NormalMappings)
			{
				Keys.Add(Mapping.Key);
			}
		}
		return Keys;
	}

	/** The tab the tab bar highlights: the one button whose background differs from all the others ('' if none or several). */
	FName HighlightedTab(const UMordecaiPauseMenuWidget* Menu)
	{
		const UHorizontalBox* TabBar = Menu && Menu->WidgetTree ? Menu->WidgetTree->FindWidget<UHorizontalBox>(TEXT("TabBar")) : nullptr;
		if (!TabBar)
		{
			return NAME_None;
		}
		TArray<const UMordecaiTabButton*> Buttons;
		for (UWidget* Child : TabBar->GetAllChildren())
		{
			if (const UMordecaiTabButton* Button = Cast<UMordecaiTabButton>(Child))
			{
				Buttons.Add(Button);
			}
		}
		FName Highlighted = NAME_None;
		for (const UMordecaiTabButton* Button : Buttons)
		{
			int32 SameColor = 0;
			for (const UMordecaiTabButton* Other : Buttons)
			{
				SameColor += Other->GetBackgroundColor().Equals(Button->GetBackgroundColor()) ? 1 : 0;
			}
			if (SameColor == 1)
			{
				if (!Highlighted.IsNone())
				{
					return NAME_None; // more than one stands out: no single highlight
				}
				Highlighted = Button->GetButtonId();
			}
		}
		return Highlighted;
	}

	/** The content class a tab draws (US-066 character sheet, US-079 inventory, placeholders for the rest). */
	const UClass* ExpectedContentClass(FName TabId)
	{
		if (TabId == UMordecaiCharacterSheetWidget::GetCharacterTabId())
		{
			return UMordecaiCharacterSheetWidget::StaticClass();
		}
		if (TabId == UMordecaiInventoryWidget::GetInventoryTabId())
		{
			return UMordecaiInventoryWidget::StaticClass();
		}
		return UMordecaiPauseMenuPlaceholderWidget::StaticClass();
	}
}

// ---------------------------------------------------------------------------
// 1. Mordecai.Recovery.PauseMenu.UIActionKeysConfigured (AC-095.1, AC-095.3: project config)
// Each UI action the menu binds maps to exactly its key in CommonUIInputSettings (DefaultInput.ini), as a press
// (no hold time), and none takes a back-action key, so Escape and gamepad B keep closing the menu.
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_Recovery_PauseMenu_UIActionKeysConfigured,
	"Mordecai.Recovery.PauseMenu.UIActionKeysConfigured",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_Recovery_PauseMenu_UIActionKeysConfigured::RunTest(const FString& Parameters)
{
	struct FExpected
	{
		FGameplayTag Tag;
		FKey Key;
	};
	const FExpected Expected[] = {
		{ MordecaiGameplayTags::UIAction_PauseMenu_PreviousTab, EKeys::Gamepad_LeftShoulder },
		{ MordecaiGameplayTags::UIAction_PauseMenu_NextTab, EKeys::Gamepad_RightShoulder },
		{ MordecaiGameplayTags::UIAction_PauseMenu_Inventory, EKeys::I },
	};
	const FKey BackKeys[] = { EKeys::Escape, EKeys::Gamepad_FaceButton_Right, EKeys::Gamepad_Special_Right };

	for (const FExpected& Entry : Expected)
	{
		const FString Tag = Entry.Tag.ToString();
		const FUIActionTag ActionTag = FUIActionTag::TryConvert(Entry.Tag);
		if (!TestTrue(FString::Printf(TEXT("%s is a UI action tag (under UI.Action)"), *Tag), ActionTag.IsValid()))
		{
			continue;
		}
		const FUIInputAction* Action = UCommonUIInputSettings::Get().FindAction(ActionTag);
		if (!TestNotNull(FString::Printf(TEXT("%s has a CommonUIInputSettings InputActions entry"), *Tag), Action))
		{
			continue;
		}
		TestEqual(FString::Printf(TEXT("%s maps one key"), *Tag), Action->KeyMappings.Num(), 1);
		for (const FUIActionKeyMapping& Mapping : Action->KeyMappings)
		{
			TestTrue(FString::Printf(TEXT("%s key is %s (got %s)"), *Tag, *Entry.Key.ToString(), *Mapping.Key.ToString()), Mapping.Key == Entry.Key);
			TestEqual(FString::Printf(TEXT("%s is a press, not a hold"), *Tag), Mapping.HoldTime, 0.f);
			for (const FKey& BackKey : BackKeys)
			{
				TestFalse(FString::Printf(TEXT("%s does not take the back key %s"), *Tag, *BackKey.ToString()), Mapping.Key == BackKey);
			}
		}
	}
	return true;
}

// ---------------------------------------------------------------------------
// 2. Mordecai.Recovery.PauseMenu.TabModelCyclesWithWrap (AC-095.3)
// The tab model steps the active tab along the tab bar in both directions and wraps at either end.
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_Recovery_PauseMenu_TabModelCyclesWithWrap,
	"Mordecai.Recovery.PauseMenu.TabModelCyclesWithWrap",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_Recovery_PauseMenu_TabModelCyclesWithWrap::RunTest(const FString& Parameters)
{
	using namespace MordecaiRecoveryPauseMenuInputTest;
	FMordecaiPauseMenuTabModel Model;
	Model.InitDefaults(UMordecaiPauseMenuPlaceholderWidget::StaticClass());
	const TArray<FName>& Order = TabOrder();
	TestEqual("starts on character", Model.ActiveTabId, Order[0]);

	// Forward through every tab and around to the first
	for (int32 Step = 1; Step <= Order.Num(); ++Step)
	{
		const FName Want = Order[Step % Order.Num()];
		TestTrue(FString::Printf(TEXT("next #%d changes the tab"), Step), Model.CycleActiveTab(1));
		TestEqual(FString::Printf(TEXT("next #%d -> %s"), Step, *Want.ToString()), Model.ActiveTabId, Want);
	}

	// Backward: from the first tab around to the last, then down to the first
	for (int32 Step = 1; Step <= Order.Num(); ++Step)
	{
		const FName Want = Order[(Order.Num() - Step) % Order.Num()];
		TestTrue(FString::Printf(TEXT("previous #%d changes the tab"), Step), Model.CycleActiveTab(-1));
		TestEqual(FString::Printf(TEXT("previous #%d -> %s"), Step, *Want.ToString()), Model.ActiveTabId, Want);
	}

	// An unknown active tab starts from the first tab
	Model.ActiveTabId = TEXT("nonexistent");
	TestTrue("unknown active tab: next selects a tab", Model.CycleActiveTab(1));
	TestEqual("unknown active tab: next selects the first tab", Model.ActiveTabId, Order[0]);

	// One tab: nothing to switch to
	FMordecaiPauseMenuTabModel Single;
	Single.RegisterTab(TEXT("only"), FText::FromString(TEXT("Only")), UMordecaiPauseMenuPlaceholderWidget::StaticClass());
	Single.SetActiveTab(TEXT("only"));
	TestFalse("one tab: next changes nothing", Single.CycleActiveTab(1));
	TestFalse("one tab: previous changes nothing", Single.CycleActiveTab(-1));
	TestEqual("one tab: still active", Single.ActiveTabId, FName(TEXT("only")));

	// No tabs
	FMordecaiPauseMenuTabModel Empty;
	TestFalse("no tabs: next changes nothing", Empty.CycleActiveTab(1));
	TestTrue("no tabs: no active tab", Empty.ActiveTabId.IsNone());
	return true;
}

// ---------------------------------------------------------------------------
// 3. Mordecai.Recovery.PauseMenu.InventoryKeyClosesMenu (AC-095.1)
// While the menu is open, pressing I closes it through the same path as Escape: the widget deactivates and
// NativeOnDeactivated closes the menu through the subsystem (which unpauses; this test world has no player controller
// to pause). The menu consumes the whole press and closes on its key-up, so a held I's repeats never reach the game
// (whose Inventory action would reopen the menu). The key-up and repeats of the I press that opened the menu do not
// close it, and the back keys stay with the back action.
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_Recovery_PauseMenu_InventoryKeyClosesMenu,
	"Mordecai.Recovery.PauseMenu.InventoryKeyClosesMenu",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_Recovery_PauseMenu_InventoryKeyClosesMenu::RunTest(const FString& Parameters)
{
	using namespace MordecaiRecoveryPauseMenuInputTest;
	FTestWorldWrapper WorldWrapper;
	if (!TestTrue("test world with an initialized game instance", WorldWrapper.CreateTestWorld(EWorldType::Game)))
	{
		WorldWrapper.ForwardErrorMessages(this);
		return true;
	}
	UWorld* World = WorldWrapper.GetTestWorld();
	UGameInstance* GameInstance = World ? World->GetGameInstance() : nullptr;
	UMordecaiPauseMenuSubsystem* Subsystem = GameInstance ? GameInstance->GetSubsystem<UMordecaiPauseMenuSubsystem>() : nullptr;
	if (!TestNotNull("pause menu subsystem", Subsystem))
	{
		return true;
	}

	// What I does while the menu is closed (Enhanced Input, MordecaiHeroComponent): open on the inventory tab
	Subsystem->TogglePauseMenuToTab(UMordecaiInventoryWidget::GetInventoryTabId());
	TestTrue("I opened the menu", Subsystem->IsMenuOpen());

	UMordecaiPauseMenuWidget* Menu = NewObject<UMordecaiPauseMenuWidget>(World);
	Menu->InitializeTabs();
	Menu->SetActiveTab(UMordecaiInventoryWidget::GetInventoryTabId());
	TestTrue("the widget resolves the game instance the subsystem lives on", Menu->GetGameInstance() == GameInstance);
	Menu->ActivateWidget();
	TestTrue("activated", Menu->IsActivated());

	const FMenuActionBindings Bindings(*Menu);
	TestEqual("the menu binds 4 UI actions (LB, RB, I press, I release)", Bindings.NumValid(), 4);
	const TSharedPtr<FUIActionBinding> InventoryPress = Bindings.Find(MordecaiGameplayTags::UIAction_PauseMenu_Inventory, IE_Pressed);
	const TSharedPtr<FUIActionBinding> InventoryRelease = Bindings.Find(MordecaiGameplayTags::UIAction_PauseMenu_Inventory, IE_Released);
	if (!TestTrue("the I press and I release UI action bindings", InventoryPress.IsValid() && InventoryRelease.IsValid()))
	{
		Menu->DeactivateWidget();
		return true;
	}
	for (const TSharedPtr<FUIActionBinding>& Binding : { InventoryPress, InventoryRelease })
	{
		const FString Event = Binding->InputEvent == IE_Pressed ? TEXT("press") : TEXT("release");
		TestTrue(FString::Printf(TEXT("I %s: I is its only key"), *Event), NormalKeys(Binding) == TArray<FKey>({ EKeys::I }));
		TestTrue(FString::Printf(TEXT("I %s: acts in Menu input mode"), *Event), Binding->InputMode == ECommonInputMode::Menu);
		TestTrue(FString::Printf(TEXT("I %s: consumes the key (the game never sees it)"), *Event), Binding->bConsumesInput);
		TestFalse(FString::Printf(TEXT("I %s: not persistent (only while the menu is the active root)"), *Event), Binding->bIsPersistent);
	}
	const FString InventoryAction = MordecaiGameplayTags::UIAction_PauseMenu_Inventory.GetTag().ToString();
	auto IsOpen = [&]() { return Menu->IsActivated() && Subsystem->IsMenuOpen(); };

	// The rest of the I press that opened the menu reaches the menu: its repeats and its key-up must not close it
	TestEqual("an I repeat fires nothing", Names(Bindings.Dispatch(EKeys::I, IE_Repeat)), FString());
	Bindings.Dispatch(EKeys::I, IE_Released);
	TestTrue("the key-up of the opening press leaves the menu open", IsOpen());

	// The back keys belong to the back action (Mordecai.UI.PauseMenu.IsCommonUIBackHandler), not to these bindings
	TestEqual("Escape fires none of these bindings", Names(Bindings.Dispatch(EKeys::Escape, IE_Pressed)), FString());
	TestEqual("Gamepad B fires none of these bindings", Names(Bindings.Dispatch(EKeys::Gamepad_FaceButton_Right, IE_Pressed)), FString());
	TestTrue("still open after the back keys reached these bindings", IsOpen());

	// A new I press, held: the menu consumes the press and the repeats and stays open until the key-up
	TestEqual("an I press fires the I action", Names(Bindings.Dispatch(EKeys::I, IE_Pressed)), InventoryAction);
	TestTrue("the press alone leaves the menu open", IsOpen());
	TestEqual("its repeats fire nothing", Names(Bindings.Dispatch(EKeys::I, IE_Repeat)), FString());
	TestTrue("held: still open", IsOpen());

	// A deactivation in between (Escape) drops the armed press: the next activation needs a new I press
	Menu->DeactivateWidget();
	Subsystem->TogglePauseMenuToTab(UMordecaiInventoryWidget::GetInventoryTabId());
	Menu->ActivateWidget();
	Bindings.Dispatch(EKeys::I, IE_Released);
	TestTrue("a key-up whose press came before the last activation leaves the menu open", IsOpen());

	// Press and release closes the menu, on the key-up
	TestEqual("I press", Names(Bindings.Dispatch(EKeys::I, IE_Pressed)), InventoryAction);
	TestEqual("I key-up fires the I action", Names(Bindings.Dispatch(EKeys::I, IE_Released)), InventoryAction);
	TestFalse("the widget deactivated (the back action's close)", Menu->IsActivated());
	TestFalse("the menu closed through the subsystem", Subsystem->IsMenuOpen());

	// The game sees I again once the menu is closed: Enhanced Input's toggle opens it again
	Subsystem->TogglePauseMenuToTab(UMordecaiInventoryWidget::GetInventoryTabId());
	TestTrue("I reopens the closed menu", Subsystem->IsMenuOpen());
	Subsystem->ClosePauseMenu();
	return true;
}

// ---------------------------------------------------------------------------
// 4. Mordecai.Recovery.PauseMenu.ShoulderButtonsCycleTabs (AC-095.3)
// While the menu is open, LB selects the previous tab and RB the next, wrapping at either end; each switch moves the
// tab-bar highlight and swaps the content area like a tab click. Key-ups do nothing and the menu stays open.
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_Recovery_PauseMenu_ShoulderButtonsCycleTabs,
	"Mordecai.Recovery.PauseMenu.ShoulderButtonsCycleTabs",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_Recovery_PauseMenu_ShoulderButtonsCycleTabs::RunTest(const FString& Parameters)
{
	using namespace MordecaiRecoveryTest;
	using namespace MordecaiRecoveryPauseMenuInputTest;
	FRecoveryControlledTestEnv Env;
	if (!TestTrue("test world with a controlled player", Env.SetupWithController()))
	{
		Env.TeardownWithController();
		return true;
	}
	UMordecaiPauseMenuWidget* Menu = Env.MakeWidget<UMordecaiPauseMenuWidget>();
	if (!TestTrue("the menu built its tab bar", Menu && Menu->WidgetTree && Menu->WidgetTree->FindWidget<UHorizontalBox>(TEXT("TabBar"))))
	{
		Env.TeardownWithController();
		return true;
	}
	Menu->ActivateWidget();

	{
		const FMenuActionBindings Bindings(*Menu);
		TestEqual("the menu binds 4 UI actions (LB, RB, I press, I release)", Bindings.NumValid(), 4);
		const TSharedPtr<FUIActionBinding> Previous = Bindings.Find(MordecaiGameplayTags::UIAction_PauseMenu_PreviousTab);
		const TSharedPtr<FUIActionBinding> Next = Bindings.Find(MordecaiGameplayTags::UIAction_PauseMenu_NextTab);
		TestTrue("LB is the previous-tab action's only key", NormalKeys(Previous) == TArray<FKey>({ EKeys::Gamepad_LeftShoulder }));
		TestTrue("RB is the next-tab action's only key", NormalKeys(Next) == TArray<FKey>({ EKeys::Gamepad_RightShoulder }));
		for (const TSharedPtr<FUIActionBinding>& Binding : { Previous, Next })
		{
			if (Binding)
			{
				TestTrue(FString::Printf(TEXT("%s fires on the press, in Menu mode, and consumes the key"), *Binding->ActionName.ToString()),
					Binding->InputEvent == IE_Pressed && Binding->InputMode == ECommonInputMode::Menu && Binding->bConsumesInput);
			}
		}

		const TArray<FName>& Order = TabOrder();
		const int32 Num = Order.Num();
		auto ExpectOn = [&](const FString& What, FName Want)
		{
			TestEqual(What + TEXT(": active tab"), Menu->GetActiveTabId(), Want);
			TestEqual(What + TEXT(": the tab bar highlights it"), HighlightedTab(Menu), Want);
			const UUserWidget* Content = Menu->GetCurrentContentWidget();
			TestTrue(What + FString::Printf(TEXT(": the content area draws %s"), *GetNameSafe(ExpectedContentClass(Want))),
				Content && Content->GetClass() == ExpectedContentClass(Want));
			TestTrue(What + TEXT(": the menu stays open"), Menu->IsActivated());
		};
		ExpectOn(TEXT("start"), Order[0]);

		// RB through every tab and around (settings -> character)
		for (int32 Step = 1; Step <= Num; ++Step)
		{
			const FString At = FString::Printf(TEXT("RB #%d"), Step);
			TestEqual(At + TEXT(" fires the next-tab action"), Names(Bindings.Dispatch(EKeys::Gamepad_RightShoulder, IE_Pressed)),
				MordecaiGameplayTags::UIAction_PauseMenu_NextTab.GetTag().ToString());
			ExpectOn(At, Order[Step % Num]);
		}

		// LB around the other way (character -> settings), then down to character
		for (int32 Step = 1; Step <= Num; ++Step)
		{
			const FString At = FString::Printf(TEXT("LB #%d"), Step);
			TestEqual(At + TEXT(" fires the previous-tab action"), Names(Bindings.Dispatch(EKeys::Gamepad_LeftShoulder, IE_Pressed)),
				MordecaiGameplayTags::UIAction_PauseMenu_PreviousTab.GetTag().ToString());
			ExpectOn(At, Order[(Num - Step) % Num]);
		}

		// Key-ups and repeats do not switch
		TestEqual("an RB key-up fires nothing", Names(Bindings.Dispatch(EKeys::Gamepad_RightShoulder, IE_Released)), FString());
		TestEqual("an LB repeat fires nothing", Names(Bindings.Dispatch(EKeys::Gamepad_LeftShoulder, IE_Repeat)), FString());
		ExpectOn(TEXT("after key-ups and repeats"), Order[0]);

		// The us069 Tier 4 path: on Settings, LB -> Inventory (bound to the pawn), RB -> Settings
		Menu->SetActiveTab(TEXT("settings"));
		Bindings.Dispatch(EKeys::Gamepad_LeftShoulder, IE_Pressed);
		ExpectOn(TEXT("Settings + LB"), TEXT("inventory"));
		const UMordecaiInventoryWidget* InventoryTab = Cast<UMordecaiInventoryWidget>(Menu->GetCurrentContentWidget());
		TestTrue("Settings + LB: the inventory tab is bound to the pawn's inventory", InventoryTab && InventoryTab->IsBoundToInventory());
		Bindings.Dispatch(EKeys::Gamepad_RightShoulder, IE_Pressed);
		ExpectOn(TEXT("Inventory + RB"), TEXT("settings"));
	}

	Menu->DeactivateWidget();
	Env.TeardownWithController();
	return true;
}
