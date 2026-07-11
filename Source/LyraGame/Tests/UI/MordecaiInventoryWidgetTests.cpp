// Project Mordecai — Inventory Widget Tests (US-071)
// NullRHI-compatible headless tests covering binding lifecycle, flat list
// display, category filtering, ledger panel, identify dispatch, empty states,
// and rebuild-on-delegate behavior.

#include "Misc/AutomationTest.h"
#include "Engine/GameInstance.h"

#include "Mordecai/Items/MordecaiIdentificationService.h"
#include "Mordecai/Items/MordecaiInventoryComponent.h"
#include "Mordecai/Items/MordecaiItemDefinition.h"
#include "Mordecai/Items/MordecaiItemInstance.h"
#include "Mordecai/Items/MordecaiItemLibrary.h"
#include "Mordecai/Items/MordecaiItemTypes.h"
#include "Mordecai/Items/MordecaiResourceLedger.h"
#include "Mordecai/UI/MordecaiInventoryWidget.h"
#include "Mordecai/UI/MordecaiPauseMenuWidget.h"
#include "MordecaiInventoryWidgetTestHelpers.h"

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

namespace MordecaiInventoryWidgetTestHelpers
{
	static UMordecaiItemDefinition* MakeDef(
		FName ItemId,
		EMordecaiItemType Type,
		EMordecaiItemRarity Rarity = EMordecaiItemRarity::Common,
		const TCHAR* DisplayName = TEXT("Test Item"))
	{
		UMordecaiItemDefinition* Def = NewObject<UMordecaiItemDefinition>();
		Def->ItemId = ItemId;
		Def->ItemType = Type;
		Def->Rarity = Rarity;
		Def->DisplayName = FText::FromString(DisplayName);
		Def->Description = FText::FromString(FString::Printf(TEXT("%s description"), DisplayName));
		return Def;
	}

	static UMordecaiItemDefinition* MakeStackableDef(FName ItemId, EMordecaiItemType Type, int32 MaxStack = 20)
	{
		UMordecaiItemDefinition* Def = MakeDef(ItemId, Type);
		Def->Stackable = true;
		Def->MaxStackSize = MaxStack;
		return Def;
	}

	static UMordecaiItemDefinition* MakeAutoStoreDef(FName ItemId, const TCHAR* DisplayName)
	{
		UMordecaiItemDefinition* Def = MakeDef(ItemId, EMordecaiItemType::Material, EMordecaiItemRarity::Common, DisplayName);
		Def->AutoStoreOnPickup = true;
		return Def;
	}

	/** Magical item that uses identification and starts Unidentified. */
	static UMordecaiItemDefinition* MakeUnidentifiedDef(FName ItemId)
	{
		UMordecaiItemDefinition* Def = MakeDef(ItemId, EMordecaiItemType::MagicalItem, EMordecaiItemRarity::Purple, TEXT("Amulet of Testing"));
		Def->Subtype = FName(TEXT("Amulet"));
		Def->ShortDescription = FText::FromString(TEXT("An amulet."));
		Def->UsesIdentification = true;
		Def->DefaultIdentificationState = EMordecaiIdentificationState::Unidentified;
		return Def;
	}

	/** Inventory component with a wired sibling ledger. */
	static UMordecaiInventoryComponent* MakeInv(UMordecaiResourceLedger** OutLedger = nullptr)
	{
		UMordecaiInventoryComponent* Inv = NewObject<UMordecaiInventoryComponent>();
		UMordecaiResourceLedger* Led = NewObject<UMordecaiResourceLedger>();
		Inv->SetResourceLedger(Led);
		if (OutLedger)
		{
			*OutLedger = Led;
		}
		return Inv;
	}

	static UMordecaiInventoryWidget* MakeWidget()
	{
		return NewObject<UMordecaiInventoryWidget>(GetTransientPackage());
	}

	/** Identification service with a transient GameInstance outer (subsystem requires it). */
	static UMordecaiIdentificationService* MakeService()
	{
		UGameInstance* GI = NewObject<UGameInstance>();
		return NewObject<UMordecaiIdentificationService>(GI);
	}
}


// ===========================================================================
// 1. Mordecai.UI.Inventory.BindsAndUnbinds (AC-071.2, AC-071.3)
// ===========================================================================
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_UI_Inventory_BindsAndUnbinds,
	"Mordecai.UI.Inventory.BindsAndUnbinds",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_UI_Inventory_BindsAndUnbinds::RunTest(const FString& Parameters)
{
	using namespace MordecaiInventoryWidgetTestHelpers;
	UMordecaiResourceLedger* Ledger = nullptr;
	UMordecaiInventoryComponent* Inv = MakeInv(&Ledger);
	UMordecaiInventoryWidget* Widget = MakeWidget();

	Widget->BindToInventory(Inv, Ledger);
	TestTrue("Bound to inventory", Widget->IsBoundToInventory());
	TestTrue("Bound to ledger", Widget->IsBoundToLedger());

	// Change while bound propagates
	Inv->AddItem(MakeDef(FName("Sword_A"), EMordecaiItemType::Weapon), 1);
	TestEqual("1 row after add while bound", Widget->GetCachedRowCount(), 1);

	// Unbind clears handles — further changes do NOT propagate
	Widget->Unbind();
	TestFalse("Not bound to inventory after Unbind", Widget->IsBoundToInventory());
	TestFalse("Not bound to ledger after Unbind", Widget->IsBoundToLedger());

	Inv->AddItem(MakeDef(FName("Sword_B"), EMordecaiItemType::Weapon), 1);
	TestEqual("Row cache unchanged after unbound add", Widget->GetCachedRowCount(), 1);

	return true;
}

// ===========================================================================
// 2. Mordecai.UI.Inventory.GracefulWithoutBinding (AC-071.2, AC-071.11)
// ===========================================================================
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_UI_Inventory_GracefulWithoutBinding,
	"Mordecai.UI.Inventory.GracefulWithoutBinding",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_UI_Inventory_GracefulWithoutBinding::RunTest(const FString& Parameters)
{
	using namespace MordecaiInventoryWidgetTestHelpers;
	UMordecaiInventoryWidget* Widget = MakeWidget();
	Widget->BindToInventory(nullptr, nullptr);

	TestFalse("Not bound to inventory", Widget->IsBoundToInventory());
	TestFalse("Not bound to ledger", Widget->IsBoundToLedger());
	TestEqual("List placeholder is --", Widget->GetListPlaceholderText().ToString(), FString(TEXT("--")));
	TestEqual("Ledger placeholder is --", Widget->GetLedgerPlaceholderText().ToString(), FString(TEXT("--")));

	// Filter changes must not crash and produce zero rows
	Widget->SetFilter(EMordecaiInventoryFilter::Weapons);
	TestEqual("Zero visible rows unbound", Widget->GetVisibleRowCount(), 0);
	Widget->SetFilter(EMordecaiInventoryFilter::All);
	TestEqual("Still zero visible rows", Widget->GetVisibleRowCount(), 0);

	return true;
}

// ===========================================================================
// 3. Mordecai.UI.Inventory.DisplaysFlatList (AC-071.4)
// ===========================================================================
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_UI_Inventory_DisplaysFlatList,
	"Mordecai.UI.Inventory.DisplaysFlatList",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_UI_Inventory_DisplaysFlatList::RunTest(const FString& Parameters)
{
	using namespace MordecaiInventoryWidgetTestHelpers;
	UMordecaiResourceLedger* Ledger = nullptr;
	UMordecaiInventoryComponent* Inv = MakeInv(&Ledger);

	// Different types/sort buckets so GetSortedItems() order is non-trivial
	Inv->AddItem(MakeDef(FName("Ore_A"),    EMordecaiItemType::Material, EMordecaiItemRarity::Common, TEXT("Iron Ore")), 1);
	Inv->AddItem(MakeDef(FName("Sword_A"),  EMordecaiItemType::Weapon,   EMordecaiItemRarity::Blue,   TEXT("Test Sword")), 1);
	Inv->AddItem(MakeDef(FName("Potion_A"), EMordecaiItemType::Consumable, EMordecaiItemRarity::Green, TEXT("Test Potion")), 1);

	UMordecaiInventoryWidget* Widget = MakeWidget();
	Widget->BindToInventory(Inv, Ledger);

	const TArray<FMordecaiItemInstance> Sorted = Inv->GetSortedItems();
	const TArray<FMordecaiInventoryRowModel> Rows = Widget->GetVisibleRows();

	TestEqual("3 rows for 3 instances", Rows.Num(), 3);
	if (Rows.Num() == Sorted.Num())
	{
		for (int32 i = 0; i < Rows.Num(); ++i)
		{
			TestEqual(FString::Printf(TEXT("Row %d matches GetSortedItems order"), i),
				Rows[i].InstanceId, Sorted[i].InstanceId);
			TestEqual(FString::Printf(TEXT("Row %d type matches"), i),
				Rows[i].Type, Sorted[i].ItemDefinition->ItemType);
			TestEqual(FString::Printf(TEXT("Row %d rarity matches"), i),
				Rows[i].Rarity, Sorted[i].ItemDefinition->Rarity);
			TestFalse(FString::Printf(TEXT("Row %d has display name"), i), Rows[i].DisplayName.IsEmpty());
		}
	}

	// Quantity badge: shown only when IsStackable() and Quantity > 1
	Inv->AddItem(MakeStackableDef(FName("Arrow_A"), EMordecaiItemType::Consumable), 5);
	const TArray<FMordecaiInventoryRowModel> Rows2 = Widget->GetVisibleRows();
	bool bFoundStack = false;
	for (const FMordecaiInventoryRowModel& Row : Rows2)
	{
		if (Row.Quantity == 5)
		{
			bFoundStack = true;
			TestTrue("Stack of 5 shows quantity", Row.bShowQuantity);
		}
		else
		{
			TestFalse("Singletons hide quantity", Row.bShowQuantity);
		}
	}
	TestTrue("Stackable row present", bFoundStack);

	return true;
}

// ===========================================================================
// 4. Mordecai.UI.Inventory.RowUsesLibraryDisplayName (AC-071.5)
// ===========================================================================
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_UI_Inventory_RowUsesLibraryDisplayName,
	"Mordecai.UI.Inventory.RowUsesLibraryDisplayName",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_UI_Inventory_RowUsesLibraryDisplayName::RunTest(const FString& Parameters)
{
	using namespace MordecaiInventoryWidgetTestHelpers;
	UMordecaiResourceLedger* Ledger = nullptr;
	UMordecaiInventoryComponent* Inv = MakeInv(&Ledger);
	const FGuid Id = Inv->AddItem(MakeUnidentifiedDef(FName("Amulet_A")), 1);

	UMordecaiInventoryWidget* Widget = MakeWidget();
	Widget->BindToInventory(Inv, Ledger);

	const FMordecaiItemInstance* Inst = Inv->FindInstance(Id);
	TestNotNull("Instance exists", Inst);

	const TArray<FMordecaiInventoryRowModel> Rows = Widget->GetVisibleRows();
	TestEqual("1 row", Rows.Num(), 1);
	if (Rows.Num() == 1 && Inst)
	{
		// Must match the library exactly (partial-info form for unidentified)
		TestEqual("Row display name = library display name",
			Rows[0].DisplayName.ToString(), UMordecaiItemLibrary::GetDisplayName(*Inst).ToString());
		TestTrue("Partial-info name contains 'Unidentified'",
			Rows[0].DisplayName.ToString().Contains(TEXT("Unidentified")));
		TestFalse("Real name hidden while unidentified",
			Rows[0].DisplayName.ToString().Contains(TEXT("Amulet of Testing")));

		TestEqual("Row description = library description",
			Rows[0].Description.ToString(), UMordecaiItemLibrary::GetDescription(*Inst).ToString());
		TestTrue("Unidentified badge set", Rows[0].bIsUnidentified);
	}

	return true;
}

// ===========================================================================
// 5. Mordecai.UI.Inventory.RarityColorMap (AC-071.6)
// ===========================================================================
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_UI_Inventory_RarityColorMap,
	"Mordecai.UI.Inventory.RarityColorMap",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_UI_Inventory_RarityColorMap::RunTest(const FString& Parameters)
{
	using namespace MordecaiInventoryWidgetTestHelpers;
	const TArray<EMordecaiItemRarity> Rarities = {
		EMordecaiItemRarity::Common, EMordecaiItemRarity::Green, EMordecaiItemRarity::Blue,
		EMordecaiItemRarity::Purple, EMordecaiItemRarity::Red, EMordecaiItemRarity::Gold
	};

	// All 6 rarities map to distinct colors
	TArray<FLinearColor> Colors;
	for (EMordecaiItemRarity Rarity : Rarities)
	{
		Colors.Add(UMordecaiInventoryWidget::GetRarityColor(Rarity));
	}
	for (int32 i = 0; i < Colors.Num(); ++i)
	{
		for (int32 j = i + 1; j < Colors.Num(); ++j)
		{
			TestFalse(FString::Printf(TEXT("Colors %d and %d are distinct"), i, j), Colors[i] == Colors[j]);
		}
	}

	// Common is Gray: equal RGB channels, not black, not white
	const FLinearColor Common = UMordecaiInventoryWidget::GetRarityColor(EMordecaiItemRarity::Common);
	TestEqual("Common R == G", Common.R, Common.G);
	TestEqual("Common G == B", Common.G, Common.B);
	TestTrue("Common not black", Common.R > 0.0f);
	TestTrue("Common not white", Common.R < 1.0f);

	// Accessor agrees with the constexpr palette in the header
	TestEqual("Common matches header constant", Common, UMordecaiInventoryWidget::RarityColorCommon);
	TestEqual("Gold matches header constant",
		UMordecaiInventoryWidget::GetRarityColor(EMordecaiItemRarity::Gold), UMordecaiInventoryWidget::RarityColorGold);

	return true;
}

// ===========================================================================
// 6. Mordecai.UI.Inventory.FilterRestrictsList (AC-071.7, AC-071.8)
// ===========================================================================
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_UI_Inventory_FilterRestrictsList,
	"Mordecai.UI.Inventory.FilterRestrictsList",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_UI_Inventory_FilterRestrictsList::RunTest(const FString& Parameters)
{
	using namespace MordecaiInventoryWidgetTestHelpers;
	UMordecaiResourceLedger* Ledger = nullptr;
	UMordecaiInventoryComponent* Inv = MakeInv(&Ledger);
	Inv->AddItem(MakeDef(FName("Sword_A"),  EMordecaiItemType::Weapon), 1);
	Inv->AddItem(MakeDef(FName("Helm_A"),   EMordecaiItemType::Armor), 1);
	// Materials in inventory (NOT auto-stored) — both Material and TownResource
	Inv->AddItem(MakeDef(FName("Ore_A"),    EMordecaiItemType::Material), 1);
	Inv->AddItem(MakeDef(FName("Lumber_A"), EMordecaiItemType::TownResource), 1);

	UMordecaiInventoryWidget* Widget = MakeWidget();
	Widget->BindToInventory(Inv, Ledger);

	TestEqual("Default filter is All", Widget->GetFilter(), EMordecaiInventoryFilter::All);
	TestEqual("All shows 4 rows", Widget->GetVisibleRowCount(), 4);

	Widget->SetFilter(EMordecaiInventoryFilter::Weapons);
	const TArray<FMordecaiInventoryRowModel> WeaponRows = Widget->GetVisibleRows();
	TestEqual("Weapons filter shows 1 row", WeaponRows.Num(), 1);
	if (WeaponRows.Num() == 1)
	{
		TestEqual("Weapon row type", WeaponRows[0].Type, EMordecaiItemType::Weapon);
	}

	// Materials filter includes both Material and TownResource
	Widget->SetFilter(EMordecaiInventoryFilter::Materials);
	const TArray<FMordecaiInventoryRowModel> MaterialRows = Widget->GetVisibleRows();
	TestEqual("Materials filter shows 2 rows", MaterialRows.Num(), 2);
	for (const FMordecaiInventoryRowModel& Row : MaterialRows)
	{
		TestTrue("Material row is Material or TownResource",
			Row.Type == EMordecaiItemType::Material || Row.Type == EMordecaiItemType::TownResource);
	}

	// Static mapping: Magical includes MagicalItem and UpgradeKey; Quest includes QuestItem
	TestTrue("Magical matches MagicalItem",
		UMordecaiInventoryWidget::FilterMatchesType(EMordecaiInventoryFilter::Magical, EMordecaiItemType::MagicalItem));
	TestTrue("Magical matches UpgradeKey",
		UMordecaiInventoryWidget::FilterMatchesType(EMordecaiInventoryFilter::Magical, EMordecaiItemType::UpgradeKey));
	TestTrue("Quest matches QuestItem",
		UMordecaiInventoryWidget::FilterMatchesType(EMordecaiInventoryFilter::Quest, EMordecaiItemType::QuestItem));
	TestFalse("Weapons does not match Armor",
		UMordecaiInventoryWidget::FilterMatchesType(EMordecaiInventoryFilter::Weapons, EMordecaiItemType::Armor));

	return true;
}

// ===========================================================================
// 7. Mordecai.UI.Inventory.FilterFiresDelegate (AC-071.9)
// ===========================================================================
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_UI_Inventory_FilterFiresDelegate,
	"Mordecai.UI.Inventory.FilterFiresDelegate",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_UI_Inventory_FilterFiresDelegate::RunTest(const FString& Parameters)
{
	using namespace MordecaiInventoryWidgetTestHelpers;
	UMordecaiInventoryWidget* Widget = MakeWidget();
	UMordecaiInventoryFilterDelegateSpy* Spy = NewObject<UMordecaiInventoryFilterDelegateSpy>();
	Widget->OnFilterChanged.AddDynamic(Spy, &UMordecaiInventoryFilterDelegateSpy::Handle);

	Widget->SetFilter(EMordecaiInventoryFilter::Weapons);
	TestEqual("Fires exactly once per call", Spy->EventCount, 1);
	TestEqual("Filter id is Weapons", Spy->LastFilterId, FName(TEXT("Weapons")));

	Widget->SetFilter(EMordecaiInventoryFilter::Materials);
	TestEqual("Second call fires once more", Spy->EventCount, 2);
	TestEqual("Filter id is Materials", Spy->LastFilterId, FName(TEXT("Materials")));

	return true;
}

// ===========================================================================
// 8. Mordecai.UI.Inventory.LedgerPanelShowsResources (AC-071.10)
// ===========================================================================
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_UI_Inventory_LedgerPanelShowsResources,
	"Mordecai.UI.Inventory.LedgerPanelShowsResources",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_UI_Inventory_LedgerPanelShowsResources::RunTest(const FString& Parameters)
{
	using namespace MordecaiInventoryWidgetTestHelpers;
	UMordecaiResourceLedger* Ledger = nullptr;
	UMordecaiInventoryComponent* Inv = MakeInv(&Ledger);

	UMordecaiInventoryWidget* Widget = MakeWidget();
	Widget->BindToInventory(Inv, Ledger);

	// Names chosen so alphabetical order differs from insertion order
	Inv->PickupItem(MakeAutoStoreDef(FName("Zinc_A"),  TEXT("Zinc Ore")), 3);
	Inv->PickupItem(MakeAutoStoreDef(FName("Amber_A"), TEXT("Amber Resin")), 7);

	const TArray<FMordecaiInventoryLedgerRow> Rows = Widget->GetLedgerRows();
	TestEqual("2 ledger rows", Rows.Num(), 2);
	if (Rows.Num() == 2)
	{
		TestEqual("Alphabetical first: Amber Resin", Rows[0].DisplayName.ToString(), FString(TEXT("Amber Resin")));
		TestEqual("Amber count 7", Rows[0].Count, 7);
		TestEqual("Alphabetical second: Zinc Ore", Rows[1].DisplayName.ToString(), FString(TEXT("Zinc Ore")));
		TestEqual("Zinc count 3", Rows[1].Count, 3);
	}

	// Auto-stored items never appear in the item list
	TestEqual("Item list stays empty", Widget->GetCachedRowCount(), 0);

	return true;
}

// ===========================================================================
// 9. Mordecai.UI.Inventory.LedgerEmptyStateMessage (AC-071.11)
// ===========================================================================
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_UI_Inventory_LedgerEmptyStateMessage,
	"Mordecai.UI.Inventory.LedgerEmptyStateMessage",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_UI_Inventory_LedgerEmptyStateMessage::RunTest(const FString& Parameters)
{
	using namespace MordecaiInventoryWidgetTestHelpers;
	UMordecaiResourceLedger* Ledger = nullptr;
	UMordecaiInventoryComponent* Inv = MakeInv(&Ledger);

	UMordecaiInventoryWidget* Widget = MakeWidget();
	Widget->BindToInventory(Inv, Ledger);

	TestEqual("Empty bound ledger placeholder",
		Widget->GetLedgerPlaceholderText().ToString(), FString(TEXT("No resources stored.")));

	// Once a resource lands, the placeholder clears
	Inv->PickupItem(MakeAutoStoreDef(FName("Ore_A"), TEXT("Iron Ore")), 1);
	TestTrue("Placeholder empty when ledger has entries", Widget->GetLedgerPlaceholderText().IsEmpty());

	return true;
}

// ===========================================================================
// 10. Mordecai.UI.Inventory.IdentifyActionFlipsState (AC-071.12, AC-071.13)
// ===========================================================================
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_UI_Inventory_IdentifyActionFlipsState,
	"Mordecai.UI.Inventory.IdentifyActionFlipsState",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_UI_Inventory_IdentifyActionFlipsState::RunTest(const FString& Parameters)
{
	using namespace MordecaiInventoryWidgetTestHelpers;
	UMordecaiResourceLedger* Ledger = nullptr;
	UMordecaiInventoryComponent* Inv = MakeInv(&Ledger);
	const FGuid Id = Inv->AddItem(MakeUnidentifiedDef(FName("Amulet_Id")), 1);

	UMordecaiInventoryWidget* Widget = MakeWidget();
	Widget->BindToInventory(Inv, Ledger);
	Widget->SetIdentificationServiceOverride(MakeService());

	// Pre-state: unidentified badge, partial-info name
	{
		const TArray<FMordecaiInventoryRowModel> Rows = Widget->GetVisibleRows();
		TestEqual("1 row before identify", Rows.Num(), 1);
		if (Rows.Num() == 1)
		{
			TestTrue("Row unidentified before", Rows[0].bIsUnidentified);
		}
	}

	TestTrue("TryIdentify returns true", Widget->TryIdentify(Id));

	const FMordecaiItemInstance* Inst = Inv->FindInstance(Id);
	TestNotNull("Instance still exists", Inst);
	if (Inst)
	{
		TestTrue("Instance now identified", Inst->IsIdentified());
	}

	// Row rebuilt automatically via OnInventoryChanged (delta=0) — no manual refresh
	{
		const TArray<FMordecaiInventoryRowModel> Rows = Widget->GetVisibleRows();
		TestEqual("1 row after identify", Rows.Num(), 1);
		if (Rows.Num() == 1)
		{
			TestFalse("Row identified after", Rows[0].bIsUnidentified);
			TestEqual("Row shows identified name",
				Rows[0].DisplayName.ToString(), FString(TEXT("Amulet of Testing")));
		}
	}

	return true;
}

// ===========================================================================
// 11. Mordecai.UI.Inventory.IdentifyNoopOnIdentified (AC-071.14)
// ===========================================================================
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_UI_Inventory_IdentifyNoopOnIdentified,
	"Mordecai.UI.Inventory.IdentifyNoopOnIdentified",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_UI_Inventory_IdentifyNoopOnIdentified::RunTest(const FString& Parameters)
{
	using namespace MordecaiInventoryWidgetTestHelpers;
	UMordecaiResourceLedger* Ledger = nullptr;
	UMordecaiInventoryComponent* Inv = MakeInv(&Ledger);
	// Plain identified weapon (no identification flow at all)
	const FGuid PlainId = Inv->AddItem(MakeDef(FName("Sword_Plain"), EMordecaiItemType::Weapon), 1);
	// Magical item, already identified via the service
	const FGuid MagicId = Inv->AddItem(MakeUnidentifiedDef(FName("Amulet_Done")), 1);

	UMordecaiInventoryWidget* Widget = MakeWidget();
	Widget->BindToInventory(Inv, Ledger);
	Widget->SetIdentificationServiceOverride(MakeService());

	TestTrue("Setup: identify magical item", Widget->TryIdentify(MagicId));

	TestFalse("Already-identified magical item rejected", Widget->TryIdentify(MagicId));
	TestFalse("Non-identification item rejected", Widget->TryIdentify(PlainId));

	const FMordecaiItemInstance* Inst = Inv->FindInstance(MagicId);
	if (Inst)
	{
		TestTrue("State unchanged (still identified)", Inst->IsIdentified());
	}

	return true;
}

// ===========================================================================
// 12. Mordecai.UI.Inventory.IdentifyNoopOnMissingInstance (AC-071.14)
// ===========================================================================
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_UI_Inventory_IdentifyNoopOnMissingInstance,
	"Mordecai.UI.Inventory.IdentifyNoopOnMissingInstance",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_UI_Inventory_IdentifyNoopOnMissingInstance::RunTest(const FString& Parameters)
{
	using namespace MordecaiInventoryWidgetTestHelpers;
	UMordecaiResourceLedger* Ledger = nullptr;
	UMordecaiInventoryComponent* Inv = MakeInv(&Ledger);
	Inv->AddItem(MakeUnidentifiedDef(FName("Amulet_X")), 1);

	UMordecaiInventoryWidget* Widget = MakeWidget();
	Widget->BindToInventory(Inv, Ledger);
	Widget->SetIdentificationServiceOverride(MakeService());

	TestFalse("Unknown InstanceId returns false", Widget->TryIdentify(FGuid::NewGuid()));
	TestFalse("Invalid (zero) guid returns false", Widget->TryIdentify(FGuid()));

	return true;
}

// ===========================================================================
// 13. Mordecai.UI.Inventory.IdentifyWithoutServiceSubsystem (AC-071.14)
// ===========================================================================
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_UI_Inventory_IdentifyWithoutServiceSubsystem,
	"Mordecai.UI.Inventory.IdentifyWithoutServiceSubsystem",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_UI_Inventory_IdentifyWithoutServiceSubsystem::RunTest(const FString& Parameters)
{
	using namespace MordecaiInventoryWidgetTestHelpers;
	UMordecaiResourceLedger* Ledger = nullptr;
	UMordecaiInventoryComponent* Inv = MakeInv(&Ledger);
	const FGuid Id = Inv->AddItem(MakeUnidentifiedDef(FName("Amulet_NoSvc")), 1);

	UMordecaiInventoryWidget* Widget = MakeWidget();
	Widget->BindToInventory(Inv, Ledger);
	// No service override and no world/GameInstance on this widget — the
	// subsystem lookup must fail gracefully.

	TestFalse("No service available returns false", Widget->TryIdentify(Id));

	const FMordecaiItemInstance* Inst = Inv->FindInstance(Id);
	if (Inst)
	{
		TestFalse("Instance stays unidentified", Inst->IsIdentified());
	}

	return true;
}

// ===========================================================================
// 14. Mordecai.UI.Inventory.EmptyInventoryShowsPlaceholder (AC-071.15)
// ===========================================================================
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_UI_Inventory_EmptyInventoryShowsPlaceholder,
	"Mordecai.UI.Inventory.EmptyInventoryShowsPlaceholder",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_UI_Inventory_EmptyInventoryShowsPlaceholder::RunTest(const FString& Parameters)
{
	using namespace MordecaiInventoryWidgetTestHelpers;
	UMordecaiResourceLedger* Ledger = nullptr;
	UMordecaiInventoryComponent* Inv = MakeInv(&Ledger);

	UMordecaiInventoryWidget* Widget = MakeWidget();
	Widget->BindToInventory(Inv, Ledger);

	TestEqual("Empty inventory + empty ledger placeholder",
		Widget->GetListPlaceholderText().ToString(), FString(TEXT("Inventory is empty.")));

	// Adding an item clears the placeholder
	Inv->AddItem(MakeDef(FName("Sword_A"), EMordecaiItemType::Weapon), 1);
	TestTrue("Placeholder empty when items exist", Widget->GetListPlaceholderText().IsEmpty());

	return true;
}

// ===========================================================================
// 15. Mordecai.UI.Inventory.FilterHidesAllShowsAltPlaceholder (AC-071.15)
// ===========================================================================
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_UI_Inventory_FilterHidesAllShowsAltPlaceholder,
	"Mordecai.UI.Inventory.FilterHidesAllShowsAltPlaceholder",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_UI_Inventory_FilterHidesAllShowsAltPlaceholder::RunTest(const FString& Parameters)
{
	using namespace MordecaiInventoryWidgetTestHelpers;
	UMordecaiResourceLedger* Ledger = nullptr;
	UMordecaiInventoryComponent* Inv = MakeInv(&Ledger);
	Inv->AddItem(MakeDef(FName("Sword_A"), EMordecaiItemType::Weapon), 1);

	UMordecaiInventoryWidget* Widget = MakeWidget();
	Widget->BindToInventory(Inv, Ledger);

	Widget->SetFilter(EMordecaiInventoryFilter::Consumables);
	TestEqual("Zero visible rows under filter", Widget->GetVisibleRowCount(), 0);
	TestEqual("Alt placeholder when filter hides all",
		Widget->GetListPlaceholderText().ToString(), FString(TEXT("No items match this filter.")));

	// Back to All restores the list, placeholder clears
	Widget->SetFilter(EMordecaiInventoryFilter::All);
	TestEqual("Row visible again", Widget->GetVisibleRowCount(), 1);
	TestTrue("Placeholder cleared", Widget->GetListPlaceholderText().IsEmpty());

	return true;
}

// ===========================================================================
// 16. Mordecai.UI.Inventory.RebuildsOnInventoryChanged (AC-071.2, AC-071.4)
// ===========================================================================
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_UI_Inventory_RebuildsOnInventoryChanged,
	"Mordecai.UI.Inventory.RebuildsOnInventoryChanged",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_UI_Inventory_RebuildsOnInventoryChanged::RunTest(const FString& Parameters)
{
	using namespace MordecaiInventoryWidgetTestHelpers;
	UMordecaiResourceLedger* Ledger = nullptr;
	UMordecaiInventoryComponent* Inv = MakeInv(&Ledger);

	UMordecaiInventoryWidget* Widget = MakeWidget();
	Widget->BindToInventory(Inv, Ledger);
	TestEqual("Starts empty", Widget->GetVisibleRowCount(), 0);

	// Add after binding → row appears without manual refresh
	const FGuid Id = Inv->AddItem(MakeDef(FName("Sword_A"), EMordecaiItemType::Weapon), 1);
	TestEqual("Row appears after add", Widget->GetVisibleRowCount(), 1);

	// Remove → row disappears
	TestTrue("Remove succeeds", Inv->RemoveItem(Id, 1));
	TestEqual("Row disappears after remove", Widget->GetVisibleRowCount(), 0);

	return true;
}

// ===========================================================================
// 17. Mordecai.UI.Inventory.RebuildsOnResourceChanged (AC-071.2, AC-071.10)
// ===========================================================================
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_UI_Inventory_RebuildsOnResourceChanged,
	"Mordecai.UI.Inventory.RebuildsOnResourceChanged",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_UI_Inventory_RebuildsOnResourceChanged::RunTest(const FString& Parameters)
{
	using namespace MordecaiInventoryWidgetTestHelpers;
	UMordecaiResourceLedger* Ledger = nullptr;
	UMordecaiInventoryComponent* Inv = MakeInv(&Ledger);

	UMordecaiInventoryWidget* Widget = MakeWidget();
	Widget->BindToInventory(Inv, Ledger);
	TestEqual("Ledger panel starts empty", Widget->GetLedgerRows().Num(), 0);

	UMordecaiItemDefinition* OreDef = MakeAutoStoreDef(FName("Ore_A"), TEXT("Iron Ore"));

	// Auto-store after binding → ledger panel updates without manual refresh
	Inv->PickupItem(OreDef, 2);
	{
		const TArray<FMordecaiInventoryLedgerRow> Rows = Widget->GetLedgerRows();
		TestEqual("1 ledger row after pickup", Rows.Num(), 1);
		if (Rows.Num() == 1)
		{
			TestEqual("Count is 2", Rows[0].Count, 2);
		}
	}

	// Second pickup accumulates
	Inv->PickupItem(OreDef, 3);
	{
		const TArray<FMordecaiInventoryLedgerRow> Rows = Widget->GetLedgerRows();
		TestEqual("Still 1 ledger row", Rows.Num(), 1);
		if (Rows.Num() == 1)
		{
			TestEqual("Count accumulated to 5", Rows[0].Count, 5);
		}
	}

	return true;
}

// ===========================================================================
// 18. Mordecai.UI.Inventory.RegistersWithPauseMenu (AC-071.1)
// ===========================================================================
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_UI_Inventory_RegistersWithPauseMenu,
	"Mordecai.UI.Inventory.RegistersWithPauseMenu",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_UI_Inventory_RegistersWithPauseMenu::RunTest(const FString& Parameters)
{
	using namespace MordecaiInventoryWidgetTestHelpers;
	UMordecaiPauseMenuWidget* PauseMenu = NewObject<UMordecaiPauseMenuWidget>(GetTransientPackage());

	UMordecaiInventoryWidget::RegisterWithPauseMenu(PauseMenu);

	const FMordecaiPauseMenuTabEntry* Tab = PauseMenu->GetTabModel().FindTab(UMordecaiInventoryWidget::GetInventoryTabId());
	TestNotNull("'inventory' tab registered", Tab);
	if (Tab)
	{
		TestEqual("Tab content class is the inventory widget",
			Tab->ContentWidgetClass.Get(), UMordecaiInventoryWidget::StaticClass());
	}

	// Null pause menu must not crash
	UMordecaiInventoryWidget::RegisterWithPauseMenu(nullptr);

	return true;
}
