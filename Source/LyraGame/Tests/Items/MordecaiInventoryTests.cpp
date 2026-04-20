// Project Mordecai — Inventory + Resource Ledger Tests (US-031)

#include "Misc/AutomationTest.h"

#include "Mordecai/Items/MordecaiInventoryComponent.h"
#include "Mordecai/Items/MordecaiResourceLedger.h"
#include "Mordecai/Items/MordecaiItemInstance.h"
#include "Mordecai/Items/MordecaiItemDefinition.h"
#include "Mordecai/Items/MordecaiItemTypes.h"
#include "Mordecai/MordecaiCharacter.h"
#include "MordecaiInventoryTestHelpers.h"

namespace MordecaiInventoryTestHelpers
{
	/** Build a transient UMordecaiItemDefinition with the specified fields. */
	static UMordecaiItemDefinition* MakeInvDef(
		FName ItemId,
		EMordecaiItemType Type,
		bool bAutoStored = false,
		bool bStackable = false,
		int32 MaxStackSize = 1,
		EMordecaiSortPriority Pri = EMordecaiSortPriority::Normal)
	{
		UMordecaiItemDefinition* Def = NewObject<UMordecaiItemDefinition>();
		Def->ItemId = ItemId;
		Def->ItemType = Type;
		Def->AutoStoreOnPickup = bAutoStored;
		Def->Stackable = bStackable;
		Def->MaxStackSize = MaxStackSize;
		Def->SortPriority = Pri;
		return Def;
	}

	static UMordecaiInventoryComponent* MakeInventory(UMordecaiResourceLedger** OutLedger = nullptr)
	{
		UMordecaiInventoryComponent* Inv = NewObject<UMordecaiInventoryComponent>();
		UMordecaiResourceLedger* Ledger = NewObject<UMordecaiResourceLedger>();
		Inv->SetResourceLedger(Ledger);
		if (OutLedger) *OutLedger = Ledger;
		return Inv;
	}
}

using namespace MordecaiInventoryTestHelpers;

// ===========================================================================
// 1. AddNonAutoStoredCreatesInstance (AC-031.4)
// ===========================================================================
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_Inventory_AddNonAutoStoredCreatesInstance,
	"Mordecai.Inventory.AddNonAutoStoredCreatesInstance",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_Inventory_AddNonAutoStoredCreatesInstance::RunTest(const FString& Parameters)
{
	UMordecaiResourceLedger* Ledger = nullptr;
	UMordecaiInventoryComponent* Inv = MakeInventory(&Ledger);
	UMordecaiItemDefinition* Sword = MakeInvDef(FName("Sword_Iron"), EMordecaiItemType::Weapon);

	FGuid Id = Inv->AddItem(Sword, 1);
	TestTrue("Returned valid InstanceId", Id.IsValid());
	TestEqual("Inventory has 1 item", Inv->GetAllItems().Num(), 1);
	TestEqual("Ledger untouched", Ledger->GetResourceCount(FName("Sword_Iron")), 0);
	return true;
}

// ===========================================================================
// 2. AddAutoStoredRoutesToLedger (AC-031.4, 031.19)
// ===========================================================================
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_Inventory_AddAutoStoredRoutesToLedger,
	"Mordecai.Inventory.AddAutoStoredRoutesToLedger",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_Inventory_AddAutoStoredRoutesToLedger::RunTest(const FString& Parameters)
{
	UMordecaiResourceLedger* Ledger = nullptr;
	UMordecaiInventoryComponent* Inv = MakeInventory(&Ledger);
	UMordecaiItemDefinition* IronOre = MakeInvDef(FName("Mat_IronOre"), EMordecaiItemType::Material, /*autoStored*/true);

	Inv->PickupItem(IronOre, 5);

	TestEqual("Inventory unchanged", Inv->GetAllItems().Num(), 0);
	TestEqual("Ledger has 5 iron ore", Ledger->GetResourceCount(FName("Mat_IronOre")), 5);

	// AddItem directly also routes to ledger, returns invalid FGuid
	FGuid Id = Inv->AddItem(IronOre, 3);
	TestFalse("Invalid FGuid returned for auto-stored add", Id.IsValid());
	TestEqual("Ledger now 8", Ledger->GetResourceCount(FName("Mat_IronOre")), 8);
	return true;
}

// ===========================================================================
// 3. AddStackableMergesIntoExistingStack (AC-031.4)
// ===========================================================================
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_Inventory_AddStackableMergesIntoExistingStack,
	"Mordecai.Inventory.AddStackableMergesIntoExistingStack",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_Inventory_AddStackableMergesIntoExistingStack::RunTest(const FString& Parameters)
{
	UMordecaiInventoryComponent* Inv = MakeInventory();
	UMordecaiItemDefinition* Potion = MakeInvDef(FName("Potion_Health"), EMordecaiItemType::Consumable,
		/*autoStored*/false, /*stackable*/true, /*maxStack*/10);

	Inv->AddItem(Potion, 1);
	Inv->AddItem(Potion, 1);

	TestEqual("1 instance after 2 adds", Inv->GetAllItems().Num(), 1);
	TestEqual("Quantity=2", Inv->GetAllItems()[0].Quantity, 2);
	return true;
}

// ===========================================================================
// 4. AddStackableOverflowsWhenAtMax (AC-031.4)
// ===========================================================================
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_Inventory_AddStackableOverflowsWhenAtMax,
	"Mordecai.Inventory.AddStackableOverflowsWhenAtMax",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_Inventory_AddStackableOverflowsWhenAtMax::RunTest(const FString& Parameters)
{
	UMordecaiInventoryComponent* Inv = MakeInventory();
	UMordecaiItemDefinition* Potion = MakeInvDef(FName("Potion_Health"), EMordecaiItemType::Consumable,
		/*autoStored*/false, /*stackable*/true, /*maxStack*/10);

	Inv->AddItem(Potion, 10);
	Inv->AddItem(Potion, 5);

	TestEqual("2 stacks", Inv->GetAllItems().Num(), 2);
	TestEqual("First stack full (10)", Inv->GetAllItems()[0].Quantity, 10);
	TestEqual("Second stack overflow (5)", Inv->GetAllItems()[1].Quantity, 5);
	return true;
}

// ===========================================================================
// 5. RemoveItemDecrementsQuantity (AC-031.5)
// ===========================================================================
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_Inventory_RemoveItemDecrementsQuantity,
	"Mordecai.Inventory.RemoveItemDecrementsQuantity",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_Inventory_RemoveItemDecrementsQuantity::RunTest(const FString& Parameters)
{
	UMordecaiInventoryComponent* Inv = MakeInventory();
	UMordecaiItemDefinition* Potion = MakeInvDef(FName("Potion"), EMordecaiItemType::Consumable,
		false, true, 10);

	FGuid Id = Inv->AddItem(Potion, 3);
	TestTrue("Remove 1 returns true", Inv->RemoveItem(Id, 1));
	TestEqual("Quantity=2", Inv->GetAllItems()[0].Quantity, 2);
	return true;
}

// ===========================================================================
// 6. RemoveItemDeletesInstanceAtZero (AC-031.5)
// ===========================================================================
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_Inventory_RemoveItemDeletesInstanceAtZero,
	"Mordecai.Inventory.RemoveItemDeletesInstanceAtZero",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_Inventory_RemoveItemDeletesInstanceAtZero::RunTest(const FString& Parameters)
{
	UMordecaiInventoryComponent* Inv = MakeInventory();
	UMordecaiItemDefinition* Sword = MakeInvDef(FName("Sword"), EMordecaiItemType::Weapon);
	FGuid Id = Inv->AddItem(Sword, 1);

	TestTrue("Remove 1 returns true", Inv->RemoveItem(Id, 1));
	TestEqual("Instance removed", Inv->GetAllItems().Num(), 0);

	// Removing more than present returns false
	TestFalse("Remove more than available returns false", Inv->RemoveItem(Id, 1));
	return true;
}

// ===========================================================================
// 7. ConsumeByDefinitionIsAtomic (AC-031.6)
// ===========================================================================
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_Inventory_ConsumeByDefinitionIsAtomic,
	"Mordecai.Inventory.ConsumeByDefinitionIsAtomic",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_Inventory_ConsumeByDefinitionIsAtomic::RunTest(const FString& Parameters)
{
	UMordecaiInventoryComponent* Inv = MakeInventory();
	UMordecaiItemDefinition* Potion = MakeInvDef(FName("Potion"), EMordecaiItemType::Consumable,
		false, true, 10);
	Inv->AddItem(Potion, 3);

	TestFalse("Consume 5 from 3 returns false", Inv->ConsumeByDefinition(Potion, 5));
	TestEqual("Quantity unchanged", Inv->GetAllItems()[0].Quantity, 3);
	return true;
}

// ===========================================================================
// 8. ConsumeByDefinitionPrefersLowestStack (AC-031.6)
// ===========================================================================
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_Inventory_ConsumeByDefinitionPrefersLowestStack,
	"Mordecai.Inventory.ConsumeByDefinitionPrefersLowestStack",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_Inventory_ConsumeByDefinitionPrefersLowestStack::RunTest(const FString& Parameters)
{
	UMordecaiInventoryComponent* Inv = MakeInventory();
	UMordecaiItemDefinition* Potion = MakeInvDef(FName("Potion"), EMordecaiItemType::Consumable,
		false, true, 10);

	// Two stacks: 5 and 2 (add the 5 first, then force a separate 2-stack by adding 2 after)
	FGuid A = Inv->AddItem(Potion, 5);
	// Second add will merge into the first up to Max=10 — we need to push it to a separate stack by
	// filling the first stack to max then adding. Easier: add a new stack via a second call after max.
	// Let's just manipulate directly via two adds:
	// First fill stack to 10 (extra 5)
	Inv->AddItem(Potion, 5);
	// Now the first stack is 10 (capped). Add 2 more - creates new stack.
	Inv->AddItem(Potion, 2);

	TestEqual("2 stacks exist", Inv->GetAllItems().Num(), 2);
	TestEqual("First stack 10", Inv->GetAllItems()[0].Quantity, 10);
	TestEqual("Second stack 2",  Inv->GetAllItems()[1].Quantity, 2);

	// Consume 3 — should drain the 2-stack entirely (2 removed) then pull 1 from the 10-stack
	TestTrue("Consume 3 succeeds", Inv->ConsumeByDefinition(Potion, 3));
	TestEqual("Now 1 stack remaining", Inv->GetAllItems().Num(), 1);
	TestEqual("Stack quantity = 9", Inv->GetAllItems()[0].Quantity, 9);
	return true;
}

// ===========================================================================
// 9. GetItemsByTypeFiltersCorrectly (AC-031.8)
// ===========================================================================
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_Inventory_GetItemsByTypeFiltersCorrectly,
	"Mordecai.Inventory.GetItemsByTypeFiltersCorrectly",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_Inventory_GetItemsByTypeFiltersCorrectly::RunTest(const FString& Parameters)
{
	UMordecaiInventoryComponent* Inv = MakeInventory();
	UMordecaiItemDefinition* Sword = MakeInvDef(FName("Sword"), EMordecaiItemType::Weapon);
	UMordecaiItemDefinition* Potion = MakeInvDef(FName("Potion"), EMordecaiItemType::Consumable);
	Inv->AddItem(Sword, 1);
	Inv->AddItem(Potion, 1);

	TArray<FMordecaiItemInstance> Weapons = Inv->GetItemsByType(EMordecaiItemType::Weapon);
	TestEqual("1 weapon", Weapons.Num(), 1);
	TestEqual("Weapon is Sword", Weapons[0].ItemDefinition->ItemId, FName("Sword"));
	return true;
}

// ===========================================================================
// 10. GetSortedItemsOrdersByPriority (AC-031.9)
// ===========================================================================
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_Inventory_GetSortedItemsOrdersByPriority,
	"Mordecai.Inventory.GetSortedItemsOrdersByPriority",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_Inventory_GetSortedItemsOrdersByPriority::RunTest(const FString& Parameters)
{
	UMordecaiInventoryComponent* Inv = MakeInventory();
	UMordecaiItemDefinition* Low = MakeInvDef(FName("Low"), EMordecaiItemType::Material,
		false, false, 1, EMordecaiSortPriority::Normal);
	UMordecaiItemDefinition* High = MakeInvDef(FName("High"), EMordecaiItemType::QuestItem,
		false, false, 1, EMordecaiSortPriority::Critical);
	Inv->AddItem(Low, 1);
	Inv->AddItem(High, 1);

	TArray<FMordecaiItemInstance> Sorted = Inv->GetSortedItems();
	TestEqual("Critical sorts first", Sorted[0].ItemDefinition->ItemId, FName("High"));
	TestEqual("Normal sorts second",   Sorted[1].ItemDefinition->ItemId, FName("Low"));
	return true;
}

// ===========================================================================
// 11. NoCapacityLimit (AC-031.20)
// ===========================================================================
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_Inventory_NoCapacityLimit,
	"Mordecai.Inventory.NoCapacityLimit",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_Inventory_NoCapacityLimit::RunTest(const FString& Parameters)
{
	UMordecaiInventoryComponent* Inv = MakeInventory();

	// Add 1000 distinct non-stackable items — no capacity rejection
	for (int32 i = 0; i < 1000; ++i)
	{
		UMordecaiItemDefinition* Def = MakeInvDef(FName(*FString::Printf(TEXT("Item_%d"), i)),
			EMordecaiItemType::Trinket);
		FGuid Id = Inv->AddItem(Def, 1);
		TestTrue("Valid id every time", Id.IsValid());
	}
	TestEqual("1000 items present", Inv->GetAllItems().Num(), 1000);
	return true;
}

// ===========================================================================
// 12-13. Delegate fires on add / remove (AC-031.17)
// (Spy classes live in MordecaiInventoryTestHelpers.h for UHT reflection)
// ===========================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_Inventory_ChangedDelegateFiresOnAdd,
	"Mordecai.Inventory.ChangedDelegateFiresOnAdd",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_Inventory_ChangedDelegateFiresOnAdd::RunTest(const FString& Parameters)
{
	UMordecaiInventoryComponent* Inv = MakeInventory();
	UMordecaiInventoryDelegateSpy* Spy = NewObject<UMordecaiInventoryDelegateSpy>();
	Inv->OnInventoryChanged.AddDynamic(Spy, &UMordecaiInventoryDelegateSpy::Handle);

	UMordecaiItemDefinition* Sword = MakeInvDef(FName("Sword"), EMordecaiItemType::Weapon);
	Inv->AddItem(Sword, 1);
	TestEqual("Add fired 1 event", Spy->AddEvents, 1);
	TestEqual("Delta = +1", Spy->LastDelta, 1);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_Inventory_ChangedDelegateFiresOnRemove,
	"Mordecai.Inventory.ChangedDelegateFiresOnRemove",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_Inventory_ChangedDelegateFiresOnRemove::RunTest(const FString& Parameters)
{
	UMordecaiInventoryComponent* Inv = MakeInventory();
	UMordecaiItemDefinition* Potion = MakeInvDef(FName("Potion"), EMordecaiItemType::Consumable,
		false, true, 10);
	FGuid Id = Inv->AddItem(Potion, 3);

	UMordecaiInventoryDelegateSpy* Spy = NewObject<UMordecaiInventoryDelegateSpy>();
	Inv->OnInventoryChanged.AddDynamic(Spy, &UMordecaiInventoryDelegateSpy::Handle);

	Inv->RemoveItem(Id, 2);
	TestEqual("Remove fired 1 event", Spy->RemoveEvents, 1);
	TestEqual("Delta = -2", Spy->LastDelta, -2);
	return true;
}

// ===========================================================================
// 14. Ledger add increments count (AC-031.13, 031.14)
// ===========================================================================
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_Ledger_AddResourceIncrementsCount,
	"Mordecai.Ledger.AddResourceIncrementsCount",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_Ledger_AddResourceIncrementsCount::RunTest(const FString& Parameters)
{
	UMordecaiResourceLedger* Ledger = NewObject<UMordecaiResourceLedger>();
	UMordecaiItemDefinition* Ore = MakeInvDef(FName("Mat_Ore"), EMordecaiItemType::Material, true);

	Ledger->AddResource(Ore, 7);
	TestEqual("Count=7 after add", Ledger->GetResourceCount(FName("Mat_Ore")), 7);

	Ledger->AddResource(Ore, 3);
	TestEqual("Count=10 after second add", Ledger->GetResourceCount(FName("Mat_Ore")), 10);
	return true;
}

// ===========================================================================
// 15. Ledger rejects non-auto-stored (AC-031.13)
// ===========================================================================
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_Ledger_AddResourceRejectsNonAutoStored,
	"Mordecai.Ledger.AddResourceRejectsNonAutoStored",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_Ledger_AddResourceRejectsNonAutoStored::RunTest(const FString& Parameters)
{
	UMordecaiResourceLedger* Ledger = NewObject<UMordecaiResourceLedger>();
	UMordecaiItemDefinition* Sword = MakeInvDef(FName("Sword"), EMordecaiItemType::Weapon, /*autostored*/false);

	AddExpectedError(TEXT("rejected non-auto-stored"), EAutomationExpectedErrorFlags::Contains, 0);
	Ledger->AddResource(Sword, 5);
	TestEqual("Count stays 0", Ledger->GetResourceCount(FName("Sword")), 0);
	return true;
}

// ===========================================================================
// 16. Ledger consume is atomic (AC-031.15)
// ===========================================================================
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_Ledger_ConsumeResourceIsAtomic,
	"Mordecai.Ledger.ConsumeResourceIsAtomic",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_Ledger_ConsumeResourceIsAtomic::RunTest(const FString& Parameters)
{
	UMordecaiResourceLedger* Ledger = NewObject<UMordecaiResourceLedger>();
	UMordecaiItemDefinition* Ore = MakeInvDef(FName("Mat_Ore"), EMordecaiItemType::Material, true);
	Ledger->AddResource(Ore, 4);

	TestFalse("Consume 5 from 4 returns false", Ledger->ConsumeResource(FName("Mat_Ore"), 5));
	TestEqual("Count unchanged (4)", Ledger->GetResourceCount(FName("Mat_Ore")), 4);

	TestTrue("Consume 3 from 4 returns true", Ledger->ConsumeResource(FName("Mat_Ore"), 3));
	TestEqual("Count=1 after consume", Ledger->GetResourceCount(FName("Mat_Ore")), 1);
	return true;
}

// ===========================================================================
// 17. Ledger delegate fires on add (AC-031.18)
// ===========================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_Ledger_ChangedDelegateFiresOnAdd,
	"Mordecai.Ledger.ChangedDelegateFiresOnAdd",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_Ledger_ChangedDelegateFiresOnAdd::RunTest(const FString& Parameters)
{
	UMordecaiResourceLedger* Ledger = NewObject<UMordecaiResourceLedger>();
	UMordecaiLedgerDelegateSpy* Spy = NewObject<UMordecaiLedgerDelegateSpy>();
	Ledger->OnResourceChanged.AddDynamic(Spy, &UMordecaiLedgerDelegateSpy::Handle);

	UMordecaiItemDefinition* Ore = MakeInvDef(FName("Mat_Ore"), EMordecaiItemType::Material, true);
	Ledger->AddResource(Ore, 5);

	TestEqual("Delegate fired once", Spy->EventCount, 1);
	TestEqual("ItemId passed through", Spy->LastItemId, FName("Mat_Ore"));
	TestEqual("New count = 5", Spy->LastCount, 5);
	return true;
}

// ===========================================================================
// 18. Character has inventory + ledger components (AC-031.21)
// ===========================================================================
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_Character_HasInventoryAndLedgerComponents,
	"Mordecai.Character.HasInventoryAndLedgerComponents",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_Character_HasInventoryAndLedgerComponents::RunTest(const FString& Parameters)
{
	// Verify defaults on the class CDO (no world/actor spawn required)
	AMordecaiCharacter* CDO = Cast<AMordecaiCharacter>(AMordecaiCharacter::StaticClass()->GetDefaultObject());
	TestTrue("CDO valid", CDO != nullptr);
	TestTrue("CDO has inventory component", CDO->GetInventoryComponent() != nullptr);
	TestTrue("CDO has resource ledger", CDO->GetResourceLedger() != nullptr);
	return true;
}
