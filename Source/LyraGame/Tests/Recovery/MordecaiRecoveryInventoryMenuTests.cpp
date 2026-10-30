// Project Mordecai — Recovery: the pause menu's inventory tab shows the current items on every open (US-094, issue #28)
//
// CommonUI pools the pause menu on UI.Layer.Menu. A reopen reuses the same UMordecaiPauseMenuWidget: NativeOnInitialized
// does not run again and SetActiveTab keeps the existing content instance, while the previous close released the
// menu's Slate tree, whose ~SObjectWidget ran the content's NativeDestruct -> Unbind. These tests build the real menu
// (player context + Initialize, so NativeOnInitialized builds the tree and the content widgets), drive it the way the
// stack does (ActivateWidget on push, DeactivateWidget on close, the same instance on reopen) and read the rendered
// rows the agent bridge reads. Headless there is no Slate tree to release, so EmulateSlateTeardown() runs the
// content's unbind that ~SObjectWidget runs in the game.

#include "Misc/AutomationTest.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/HorizontalBox.h"
#include "Components/TextBlock.h"
#include "Dom/JsonObject.h"
#include "Dom/JsonValue.h"
#include "Engine/LocalPlayer.h"
#include "GameFramework/PlayerController.h"
#include "Mordecai/Items/MordecaiInventoryComponent.h"
#include "Mordecai/Items/MordecaiItemDefinition.h"
#include "Mordecai/Items/MordecaiResourceLedger.h"
#include "Mordecai/Playtest/MordecaiAgentBridgeState.h"
#include "Mordecai/UI/MordecaiCharacterSheetWidget.h"
#include "Mordecai/UI/MordecaiInventoryWidget.h"
#include "Mordecai/UI/MordecaiPauseMenuWidget.h"
#include "Mordecai/UI/MordecaiTabButton.h"
#include "Tests/Recovery/MordecaiRecoveryTestEnv.h"

using namespace MordecaiRecoveryTest;

namespace MordecaiRecoveryInventoryMenuTest
{
	/** The controlled recovery env, building the pause menu the way the game's pause menu is built. */
	struct FMenuTestEnv : FRecoveryControlledTestEnv
	{
		UMordecaiPauseMenuWidget* MakeMenu() const
		{
			return MakeWidget<UMordecaiPauseMenuWidget>();
		}
	};

	struct FItemDefs
	{
		UMordecaiItemDefinition* Sword = nullptr;
		UMordecaiItemDefinition* Amulet = nullptr;
		UMordecaiItemDefinition* Potion = nullptr;
		UMordecaiItemDefinition* Ore = nullptr;
		UMordecaiItemDefinition* Stone = nullptr;
	};

	UMordecaiItemDefinition* MakeMenuTestDef(FName ItemId, const TCHAR* DisplayName, EMordecaiItemType Type,
		EMordecaiItemRarity Rarity = EMordecaiItemRarity::Common)
	{
		UMordecaiItemDefinition* Def = NewObject<UMordecaiItemDefinition>();
		Def->ItemId = ItemId;
		Def->DisplayName = FText::FromString(DisplayName);
		Def->ItemType = Type;
		Def->Rarity = Rarity;
		return Def;
	}

	/** The us079 arena items: a weapon, an unidentified amulet, a consumable and two auto-stored resources. */
	FItemDefs MakeArenaItems()
	{
		FItemDefs Defs;
		Defs.Sword = MakeMenuTestDef(TEXT("TestItem_RustySword"), TEXT("Rusty Sword"), EMordecaiItemType::Weapon);
		Defs.Amulet = MakeMenuTestDef(TEXT("TestItem_MysteriousAmulet"), TEXT("Mysterious Amulet"), EMordecaiItemType::MagicalItem, EMordecaiItemRarity::Purple);
		Defs.Amulet->Subtype = TEXT("Amulet");
		Defs.Amulet->UsesIdentification = true;
		Defs.Amulet->DefaultIdentificationState = EMordecaiIdentificationState::Unidentified;
		Defs.Potion = MakeMenuTestDef(TEXT("TestItem_HealthPotion"), TEXT("Health Potion"), EMordecaiItemType::Consumable);
		Defs.Ore = MakeMenuTestDef(TEXT("TestItem_IronOre"), TEXT("Iron Ore"), EMordecaiItemType::Material);
		Defs.Ore->AutoStoreOnPickup = true;
		Defs.Ore->Stackable = true;
		Defs.Ore->MaxStackSize = 99;
		Defs.Stone = MakeMenuTestDef(TEXT("TestItem_TownStone"), TEXT("Town Stone"), EMordecaiItemType::TownResource);
		Defs.Stone->AutoStoreOnPickup = true;
		Defs.Stone->Stackable = true;
		Defs.Stone->MaxStackSize = 99;
		return Defs;
	}

	/** The stack's push: CommonUI activates the widget, then the subsystem restores / selects the tab. */
	void OpenMenuTo(UMordecaiPauseMenuWidget* Menu, FName TabId)
	{
		Menu->ActivateWidget();
		Menu->SetActiveTab(TabId);
	}

	/**
	 * What the game's ~SObjectWidget does to the content after a close: the menu's Slate tree is released one core
	 * tick later and the content's NativeDestruct unbinds it (MordecaiInventoryWidget / MordecaiCharacterSheetWidget).
	 */
	void EmulateSlateTeardown(UUserWidget* Content)
	{
		if (UMordecaiInventoryWidget* Inventory = Cast<UMordecaiInventoryWidget>(Content))
		{
			Inventory->Unbind();
		}
		else if (UMordecaiCharacterSheetWidget* Sheet = Cast<UMordecaiCharacterSheetWidget>(Content))
		{
			Sheet->UnbindFromASC();
		}
	}

	/** The close: the back action (or the subsystem's pop) deactivates the widget, then the Slate tree goes away. */
	void CloseMenu(UMordecaiPauseMenuWidget* Menu)
	{
		Menu->DeactivateWidget();
		EmulateSlateTeardown(Menu->GetCurrentContentWidget());
	}

	/** Rows as one string, so a mismatch prints what is drawn: "Rusty Sword | Health Potion". */
	FString JoinRows(const TArray<FString>& Rows)
	{
		return FString::Join(Rows, TEXT(" | "));
	}

	/** The rendered item rows' text, as the agent bridge reports pause_menu.inventory_tab.rows. */
	TArray<FString> DrawnRows(const UMordecaiInventoryWidget* Content)
	{
		TArray<FString> Rows;
		const TSharedPtr<FJsonObject> Tab = MordecaiAgentBridge::State::GatherInventoryTab(Content, FMordecaiAgentBridgeViewportTransform())->AsObject();
		const TArray<TSharedPtr<FJsonValue>>* Array = nullptr;
		if (Tab.IsValid() && Tab->TryGetArrayField(TEXT("rows"), Array))
		{
			for (const TSharedPtr<FJsonValue>& Row : *Array)
			{
				FString Text;
				Row->AsObject()->TryGetStringField(TEXT("text"), Text);
				Rows.Add(Text);
			}
		}
		return Rows;
	}

	/** The rendered ledger rows, as the agent bridge reports pause_menu.inventory_tab.ledger_rows. */
	TArray<FString> DrawnLedgerRows(const UMordecaiInventoryWidget* Content)
	{
		TArray<FString> Rows;
		const TSharedPtr<FJsonObject> Tab = MordecaiAgentBridge::State::GatherInventoryTab(Content, FMordecaiAgentBridgeViewportTransform())->AsObject();
		const TArray<TSharedPtr<FJsonValue>>* Array = nullptr;
		if (Tab.IsValid() && Tab->TryGetArrayField(TEXT("ledger_rows"), Array))
		{
			for (const TSharedPtr<FJsonValue>& Row : *Array)
			{
				Rows.Add(Row->AsString());
			}
		}
		return Rows;
	}

	/** The text the named placeholder TextBlock draws (empty when collapsed). */
	FString DrawnPlaceholder(const UMordecaiInventoryWidget* Content, const TCHAR* Name)
	{
		const UTextBlock* Text = Content && Content->WidgetTree ? Content->WidgetTree->FindWidget<UTextBlock>(FName(Name)) : nullptr;
		if (!Text)
		{
			return TEXT("<missing>");
		}
		return Text->GetVisibility() == ESlateVisibility::Collapsed ? FString() : Text->GetText().ToString();
	}

	/** Click a filter button the way the bridge's click does: its UButton OnClicked, relayed with the filter id. */
	bool ClickFilterButton(const UMordecaiInventoryWidget* Content, FName FilterId)
	{
		const UHorizontalBox* FilterBar = Content && Content->WidgetTree ? Content->WidgetTree->FindWidget<UHorizontalBox>(TEXT("FilterBar")) : nullptr;
		if (!FilterBar)
		{
			return false;
		}
		for (UWidget* Child : FilterBar->GetAllChildren())
		{
			UMordecaiTabButton* Button = Cast<UMordecaiTabButton>(Child);
			if (Button && Button->GetButtonId() == FilterId)
			{
				Button->OnClicked.Broadcast();
				return true;
			}
		}
		return false;
	}

	/** How many times Listener is bound to the delegate (0 = not listening, 1 = bound once, more = duplicates). */
	int32 CountListeners(const TArray<UObject*>& BoundObjects, const UObject* Listener)
	{
		int32 Count = 0;
		for (const UObject* Object : BoundObjects)
		{
			Count += Object == Listener ? 1 : 0;
		}
		return Count;
	}
}

// ---------------------------------------------------------------------------
// 1. Mordecai.Recovery.PauseMenu.RebindsContentOnReactivate (AC-094.1)
// Every activation of the pooled menu rebinds the active tab's content to the current pawn's components (and player
// state ASC) and redraws it, before and independently of the subsystem's SetActiveTab.
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_Recovery_PauseMenu_RebindsContentOnReactivate,
	"Mordecai.Recovery.PauseMenu.RebindsContentOnReactivate",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_Recovery_PauseMenu_RebindsContentOnReactivate::RunTest(const FString& Parameters)
{
	using namespace MordecaiRecoveryInventoryMenuTest;
	FMenuTestEnv Env;
	if (!TestTrue("test world with a controlled player", Env.SetupWithController()))
	{
		Env.TeardownWithController();
		return true;
	}
	UMordecaiPauseMenuWidget* Menu = Env.MakeMenu();
	if (!TestTrue("the menu built its tree and content area", Menu && Menu->WidgetTree && Menu->WidgetTree->FindWidget<UBorder>(TEXT("TabContentBorder"))))
	{
		Env.TeardownWithController();
		return true;
	}
	UMordecaiInventoryComponent* Inventory = Env.Player->GetInventoryComponent();
	UMordecaiResourceLedger* Ledger = Env.Player->GetResourceLedger();
	const FItemDefs Defs = MakeArenaItems();
	UMordecaiCharacterSheetWidget* DefaultSheet = Cast<UMordecaiCharacterSheetWidget>(Menu->GetCurrentContentWidget());
	TestTrue("the default character tab is bound to the ASC", DefaultSheet && DefaultSheet->IsBoundToASC());

	// Open 1, before any pickup: the freshly created tab is bound and empty
	OpenMenuTo(Menu, UMordecaiInventoryWidget::GetInventoryTabId());
	UMordecaiInventoryWidget* Tab = Cast<UMordecaiInventoryWidget>(Menu->GetCurrentContentWidget());
	if (!TestNotNull("open 1: the inventory tab content", Tab))
	{
		Env.TeardownWithController();
		return true;
	}
	TestTrue("open 1: the replaced character tab stopped listening",
		DefaultSheet && !DefaultSheet->IsBoundToASC() && DefaultSheet->GetRegisteredDelegateCount() == 0);
	TestTrue("open 1: bound to the pawn's inventory", Tab->IsBoundToInventory());
	TestTrue("open 1: bound to the pawn's ledger", Tab->IsBoundToLedger());
	TestEqualSensitive("open 1: 'Inventory is empty.'", DrawnPlaceholder(Tab, TEXT("ListPlaceholder")), FString(TEXT("Inventory is empty.")));
	TestEqualSensitive("open 1: 'No resources stored.'", DrawnPlaceholder(Tab, TEXT("LedgerPlaceholder")), FString(TEXT("No resources stored.")));

	// Close, then pick everything up while the menu is closed
	CloseMenu(Menu);
	Inventory->PickupItem(Defs.Sword, 1);
	Inventory->PickupItem(Defs.Amulet, 1);
	Inventory->PickupItem(Defs.Potion, 1);
	Inventory->PickupItem(Defs.Ore, 1);
	Inventory->PickupItem(Defs.Stone, 1);
	TestEqual("closed: the pawn holds 3 items", Inventory->GetSortedItems().Num(), 3);
	TestEqual("closed: the pawn stores 2 resources", Ledger->GetAllResources().Num(), 2);

	// Reopen: the stack activates the SAME pooled instance; activation alone must rebind and redraw the tab
	Menu->ActivateWidget();
	TestTrue("reopen: the pooled content instance is kept", Menu->GetCurrentContentWidget() == Tab);
	TestTrue("reopen: activation rebinds the inventory", Tab->IsBoundToInventory());
	TestTrue("reopen: activation rebinds the ledger", Tab->IsBoundToLedger());
	TestEqualSensitive("reopen: 3 rendered item rows",
		JoinRows(DrawnRows(Tab)), FString(TEXT("Rusty Sword | ? Unidentified Purple Amulet | Health Potion")));
	TestEqualSensitive("reopen: 2 rendered ledger rows",
		JoinRows(DrawnLedgerRows(Tab)), FString(TEXT("Iron Ore  x1 | Town Stone  x1")));
	TestEqualSensitive("reopen: no list placeholder", DrawnPlaceholder(Tab, TEXT("ListPlaceholder")), FString());
	TestEqualSensitive("reopen: no ledger placeholder", DrawnPlaceholder(Tab, TEXT("LedgerPlaceholder")), FString());

	// The subsystem then restores the remembered tab: same class, same instance, still current
	Menu->SetActiveTab(UMordecaiInventoryWidget::GetInventoryTabId());
	TestTrue("reopen + SetActiveTab: same content instance", Menu->GetCurrentContentWidget() == Tab);
	TestEqual("reopen + SetActiveTab: still 3 rendered rows", DrawnRows(Tab).Num(), 3);

	// The CURRENT pawn: the controller now controls another player; the next open shows that pawn's items
	CloseMenu(Menu);
	AMordecaiPlayerState* PlayerState2 = nullptr;
	AMordecaiCharacter* Player2 = nullptr;
	if (TestTrue("a second player", Env.SpawnPlayer(FVector(500.f, 0.f, 0.f), PlayerState2, Player2)))
	{
		Env.PC->SetPawn(Player2);
		Player2->GetInventoryComponent()->PickupItem(Defs.Potion, 1);
		OpenMenuTo(Menu, UMordecaiInventoryWidget::GetInventoryTabId());
		TestTrue("new pawn: same pooled content instance", Menu->GetCurrentContentWidget() == Tab);
		TestTrue("new pawn: bound", Tab->IsBoundToInventory() && Tab->IsBoundToLedger());
		TestEqualSensitive("new pawn: its 1 item is drawn", JoinRows(DrawnRows(Tab)), FString(TEXT("Health Potion")));
		TestEqualSensitive("new pawn: it stores nothing", DrawnPlaceholder(Tab, TEXT("LedgerPlaceholder")), FString(TEXT("No resources stored.")));
		Inventory->PickupItem(Defs.Sword, 1);
		TestEqual("the old pawn's pickups no longer reach the tab", DrawnRows(Tab).Num(), 1);
		CloseMenu(Menu);
		Env.PC->SetPawn(Env.Player);
	}

	// The character tab is rebound the same way (to the player state's ASC)
	OpenMenuTo(Menu, UMordecaiCharacterSheetWidget::GetCharacterTabId());
	TestFalse("tab switch: the replaced inventory tab stopped listening", Tab->IsBoundToInventory() || Tab->IsBoundToLedger());
	TestEqual("tab switch: no inventory listener left", CountListeners(Inventory->OnInventoryChanged.GetAllObjects(), Tab), 0);
	TestEqual("tab switch: no ledger listener left", CountListeners(Ledger->OnResourceChanged.GetAllObjects(), Tab), 0);
	UMordecaiCharacterSheetWidget* Sheet = Cast<UMordecaiCharacterSheetWidget>(Menu->GetCurrentContentWidget());
	if (TestNotNull("the character tab content", Sheet))
	{
		TestTrue("character open: bound to the ASC", Sheet->IsBoundToASC());
		CloseMenu(Menu);
		Menu->ActivateWidget();
		TestTrue("character reopen: the pooled content instance is kept", Menu->GetCurrentContentWidget() == Sheet);
		TestTrue("character reopen: activation rebinds the ASC", Sheet->IsBoundToASC());
		TestTrue("character reopen: attribute delegates registered", Sheet->GetRegisteredDelegateCount() > 0);
		Menu->DeactivateWidget();
	}

	Env.TeardownWithController();
	return true;
}

// ---------------------------------------------------------------------------
// 2. Mordecai.Recovery.Inventory.WidgetRefreshesOnChange (AC-094.2)
// On a reopened (pooled) menu, the inventory tab redraws its rows and ledger rows when the inventory or the ledger
// changes while the menu is open.
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_Recovery_Inventory_WidgetRefreshesOnChange,
	"Mordecai.Recovery.Inventory.WidgetRefreshesOnChange",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_Recovery_Inventory_WidgetRefreshesOnChange::RunTest(const FString& Parameters)
{
	using namespace MordecaiRecoveryInventoryMenuTest;
	FMenuTestEnv Env;
	if (!TestTrue("test world with a controlled player", Env.SetupWithController()))
	{
		Env.TeardownWithController();
		return true;
	}
	UMordecaiPauseMenuWidget* Menu = Env.MakeMenu();
	if (!TestTrue("the menu built its tree and content area", Menu && Menu->WidgetTree && Menu->WidgetTree->FindWidget<UBorder>(TEXT("TabContentBorder"))))
	{
		Env.TeardownWithController();
		return true;
	}
	UMordecaiInventoryComponent* Inventory = Env.Player->GetInventoryComponent();
	UMordecaiResourceLedger* Ledger = Env.Player->GetResourceLedger();
	const FItemDefs Defs = MakeArenaItems();

	// Open once, close, reopen: the tab under test is the pooled one
	OpenMenuTo(Menu, UMordecaiInventoryWidget::GetInventoryTabId());
	UMordecaiInventoryWidget* Tab = Cast<UMordecaiInventoryWidget>(Menu->GetCurrentContentWidget());
	if (!TestNotNull("the inventory tab content", Tab))
	{
		Env.TeardownWithController();
		return true;
	}
	CloseMenu(Menu);
	OpenMenuTo(Menu, UMordecaiInventoryWidget::GetInventoryTabId());
	TestTrue("reopen: same pooled content instance", Menu->GetCurrentContentWidget() == Tab);
	TestEqualSensitive("reopen: 'Inventory is empty.'", DrawnPlaceholder(Tab, TEXT("ListPlaceholder")), FString(TEXT("Inventory is empty.")));

	// Changes while the menu is open redraw the tab
	Inventory->PickupItem(Defs.Sword, 1);
	TestEqualSensitive("pickup sword: drawn", JoinRows(DrawnRows(Tab)), FString(TEXT("Rusty Sword")));
	TestEqualSensitive("pickup sword: no list placeholder", DrawnPlaceholder(Tab, TEXT("ListPlaceholder")), FString());

	Inventory->PickupItem(Defs.Ore, 2);
	TestEqualSensitive("pickup ore: ledger row drawn", JoinRows(DrawnLedgerRows(Tab)), FString(TEXT("Iron Ore  x2")));
	TestEqualSensitive("pickup ore: no ledger placeholder", DrawnPlaceholder(Tab, TEXT("LedgerPlaceholder")), FString());

	Inventory->PickupItem(Defs.Amulet, 1);
	TestEqualSensitive("pickup amulet: drawn unidentified",
		JoinRows(DrawnRows(Tab)), FString(TEXT("Rusty Sword | ? Unidentified Purple Amulet")));

	FGuid AmuletId;
	for (const FMordecaiItemInstance& Instance : Inventory->GetSortedItems())
	{
		AmuletId = Instance.ItemDefinition == Defs.Amulet ? Instance.InstanceId : AmuletId;
	}
	TestTrue("identify the amulet", Inventory->SetInstanceIdentificationState(AmuletId, EMordecaiIdentificationState::Identified));
	TestEqualSensitive("identified: the real name is drawn",
		JoinRows(DrawnRows(Tab)), FString(TEXT("Rusty Sword | Mysterious Amulet")));

	TestTrue("consume 1 ore", Ledger->ConsumeResource(Defs.Ore->ItemId, 1));
	TestEqualSensitive("consume ore: ledger row redrawn", JoinRows(DrawnLedgerRows(Tab)), FString(TEXT("Iron Ore  x1")));
	TestTrue("consume the last ore", Ledger->ConsumeResource(Defs.Ore->ItemId, 1));
	TestEqual("no ore left: no ledger rows", DrawnLedgerRows(Tab).Num(), 0);
	TestEqualSensitive("no ore left: 'No resources stored.'", DrawnPlaceholder(Tab, TEXT("LedgerPlaceholder")), FString(TEXT("No resources stored.")));

	FGuid SwordId;
	for (const FMordecaiItemInstance& Instance : Inventory->GetSortedItems())
	{
		SwordId = Instance.ItemDefinition == Defs.Sword ? Instance.InstanceId : SwordId;
	}
	TestTrue("drop the sword", Inventory->RemoveItem(SwordId, 1));
	TestEqualSensitive("sword removed: its row is gone", JoinRows(DrawnRows(Tab)), FString(TEXT("Mysterious Amulet")));

	Menu->DeactivateWidget();
	Env.TeardownWithController();
	return true;
}

// ---------------------------------------------------------------------------
// 3. Mordecai.Recovery.Inventory.RepeatedOpenStaysBound (AC-094.2)
// Open/close the pooled menu repeatedly: each open is bound exactly once (no duplicate subscriptions), shows every
// item picked up while closed and never the unbound "--" placeholder, even after a filter click; each close unbinds
// the tab cleanly (no listener left on the inventory or the ledger).
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_Recovery_Inventory_RepeatedOpenStaysBound,
	"Mordecai.Recovery.Inventory.RepeatedOpenStaysBound",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_Recovery_Inventory_RepeatedOpenStaysBound::RunTest(const FString& Parameters)
{
	using namespace MordecaiRecoveryInventoryMenuTest;
	FMenuTestEnv Env;
	if (!TestTrue("test world with a controlled player", Env.SetupWithController()))
	{
		Env.TeardownWithController();
		return true;
	}
	UMordecaiPauseMenuWidget* Menu = Env.MakeMenu();
	if (!TestTrue("the menu built its tree and content area", Menu && Menu->WidgetTree && Menu->WidgetTree->FindWidget<UBorder>(TEXT("TabContentBorder"))))
	{
		Env.TeardownWithController();
		return true;
	}
	UMordecaiInventoryComponent* Inventory = Env.Player->GetInventoryComponent();
	UMordecaiResourceLedger* Ledger = Env.Player->GetResourceLedger();
	const FItemDefs Defs = MakeArenaItems();

	UMordecaiInventoryWidget* FirstTab = nullptr;
	for (int32 OpenIndex = 1; OpenIndex <= 3; ++OpenIndex)
	{
		const FString At = FString::Printf(TEXT("open %d: "), OpenIndex);
		const int32 Expected = OpenIndex - 1; // one sword picked up during each earlier close

		OpenMenuTo(Menu, UMordecaiInventoryWidget::GetInventoryTabId());
		UMordecaiInventoryWidget* Tab = Cast<UMordecaiInventoryWidget>(Menu->GetCurrentContentWidget());
		if (!TestNotNull(*(At + TEXT("the inventory tab content")), Tab))
		{
			break;
		}
		FirstTab = FirstTab ? FirstTab : Tab;
		TestTrue(*(At + TEXT("the pooled content instance")), Tab == FirstTab);
		TestTrue(*(At + TEXT("bound to the inventory")), Tab->IsBoundToInventory());
		TestTrue(*(At + TEXT("bound to the ledger")), Tab->IsBoundToLedger());
		TestEqual(*(At + TEXT("subscribed to the inventory once")), CountListeners(Inventory->OnInventoryChanged.GetAllObjects(), Tab), 1);
		TestEqual(*(At + TEXT("subscribed to the ledger once")), CountListeners(Ledger->OnResourceChanged.GetAllObjects(), Tab), 1);
		TestEqual(*(At + TEXT("every sword picked up so far is drawn")), DrawnRows(Tab).Num(), Expected);

		// A filter click redraws the list: never the unbound "--"
		TestTrue(*(At + TEXT("click Armor")), ClickFilterButton(Tab, TEXT("Armor")));
		TestEqual(*(At + TEXT("Armor: no rows")), DrawnRows(Tab).Num(), 0);
		TestEqualSensitive(*(At + TEXT("Armor: placeholder")), DrawnPlaceholder(Tab, TEXT("ListPlaceholder")),
			FString(Expected > 0 ? TEXT("No items match this filter.") : TEXT("Inventory is empty.")));
		TestEqualSensitive(*(At + TEXT("Armor: ledger placeholder")), DrawnPlaceholder(Tab, TEXT("LedgerPlaceholder")), FString(TEXT("No resources stored.")));
		TestTrue(*(At + TEXT("click All")), ClickFilterButton(Tab, TEXT("All")));
		TestEqual(*(At + TEXT("All: every sword drawn")), DrawnRows(Tab).Num(), Expected);

		// Close: the tab lets go of the pawn's components, and a pickup while closed does not reach it
		Menu->DeactivateWidget();
		TestFalse(*(At + TEXT("closed: unbound from the inventory")), Tab->IsBoundToInventory());
		TestFalse(*(At + TEXT("closed: unbound from the ledger")), Tab->IsBoundToLedger());
		TestEqual(*(At + TEXT("closed: no inventory listener left")), CountListeners(Inventory->OnInventoryChanged.GetAllObjects(), Tab), 0);
		TestEqual(*(At + TEXT("closed: no ledger listener left")), CountListeners(Ledger->OnResourceChanged.GetAllObjects(), Tab), 0);
		EmulateSlateTeardown(Tab);
		Inventory->PickupItem(Defs.Sword, 1);
		TestEqual(*(At + TEXT("closed: a pickup does not touch the hidden tab")), Tab->GetCachedRowCount(), Expected);
	}

	Env.TeardownWithController();
	return true;
}
