// Project Mordecai — Item Pickup Actor Tests (US-079)
// NullRHI-compatible headless tests for AMordecaiItemPickup: press-to-pickup
// execution, destroy-on-pickup, null-safety, and server-authority gating.

#include "Misc/AutomationTest.h"
#include "Engine/World.h"
#include "Engine/Engine.h"

#include "Mordecai/Items/MordecaiInventoryComponent.h"
#include "Mordecai/Items/MordecaiItemDefinition.h"
#include "Mordecai/Items/MordecaiItemPickup.h"
#include "Mordecai/Items/MordecaiItemTypes.h"
#include "Mordecai/Items/MordecaiResourceLedger.h"

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

namespace MordecaiItemPickupTestHelpers
{
	static UMordecaiItemDefinition* MakeDef(
		FName ItemId,
		EMordecaiItemType Type = EMordecaiItemType::Consumable,
		const TCHAR* DisplayName = TEXT("Test Item"))
	{
		UMordecaiItemDefinition* Def = NewObject<UMordecaiItemDefinition>();
		Def->ItemId = ItemId;
		Def->ItemType = Type;
		Def->DisplayName = FText::FromString(DisplayName);
		return Def;
	}

	/** World + collector actor carrying an inventory component + ledger. */
	struct FPickupTestEnv
	{
		UWorld* World = nullptr;
		AActor* Collector = nullptr;
		UMordecaiInventoryComponent* Inventory = nullptr;
		UMordecaiResourceLedger* Ledger = nullptr;

		bool Setup()
		{
			if (!GEngine) return false;

			World = UWorld::CreateWorld(EWorldType::Game, false);
			if (!World) return false;

			FWorldContext& WorldContext = GEngine->CreateNewWorldContext(EWorldType::Game);
			WorldContext.SetCurrentWorld(World);

			FActorSpawnParameters SpawnParams;
			Collector = World->SpawnActor<AActor>(AActor::StaticClass(), SpawnParams);
			if (!Collector) { Teardown(); return false; }

			Inventory = NewObject<UMordecaiInventoryComponent>(Collector);
			Inventory->RegisterComponent();
			Ledger = NewObject<UMordecaiResourceLedger>(Collector);
			Ledger->RegisterComponent();
			Inventory->SetResourceLedger(Ledger);

			return true;
		}

		AMordecaiItemPickup* SpawnPickup(UMordecaiItemDefinition* Def, int32 Quantity = 1, FVector Location = FVector::ZeroVector)
		{
			FActorSpawnParameters SpawnParams;
			AMordecaiItemPickup* Pickup = World->SpawnActor<AMordecaiItemPickup>(
				AMordecaiItemPickup::StaticClass(), Location, FRotator::ZeroRotator, SpawnParams);
			if (Pickup)
			{
				Pickup->ItemDefinition = Def;
				Pickup->Quantity = Quantity;
			}
			return Pickup;
		}

		void Teardown()
		{
			if (World && GEngine)
			{
				GEngine->DestroyWorldContext(World);
				World->DestroyWorld(false);
			}
			World = nullptr;
			Collector = nullptr;
			Inventory = nullptr;
			Ledger = nullptr;
		}
	};
}


// ===========================================================================
// 1. Mordecai.Items.ItemPickup.ExecutePickupCallsPickupItemOnce (AC-079.2)
// ===========================================================================
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_Items_ItemPickup_ExecutePickupCallsPickupItemOnce,
	"Mordecai.Items.ItemPickup.ExecutePickupCallsPickupItemOnce",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_Items_ItemPickup_ExecutePickupCallsPickupItemOnce::RunTest(const FString& Parameters)
{
	using namespace MordecaiItemPickupTestHelpers;
	FPickupTestEnv Env;
	if (!Env.Setup()) { AddError(TEXT("Failed to set up test world")); return false; }

	UMordecaiItemDefinition* Def = MakeDef(FName("Potion_A"));
	AMordecaiItemPickup* Pickup = Env.SpawnPickup(Def, 3);
	TestNotNull("Pickup spawned", Pickup);

	const bool bPicked = Pickup->ExecutePickup(Env.Collector);
	TestTrue("ExecutePickup succeeded on authority", bPicked);
	TestEqual("Exactly the configured quantity was routed", Env.Inventory->GetTotalQuantityOfDefinition(Def), 3);
	TestEqual("Single instance created for non-stackable-merge case", Env.Inventory->GetAllItems().Num(), 3);

	Env.Teardown();
	return true;
}

// ===========================================================================
// 2. Mordecai.Items.ItemPickup.DestroysAfterPickup (AC-079.2)
// ===========================================================================
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_Items_ItemPickup_DestroysAfterPickup,
	"Mordecai.Items.ItemPickup.DestroysAfterPickup",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_Items_ItemPickup_DestroysAfterPickup::RunTest(const FString& Parameters)
{
	using namespace MordecaiItemPickupTestHelpers;
	FPickupTestEnv Env;
	if (!Env.Setup()) { AddError(TEXT("Failed to set up test world")); return false; }

	AMordecaiItemPickup* Pickup = Env.SpawnPickup(MakeDef(FName("Potion_B")));
	Pickup->bDestroyOnPickup = true;
	Pickup->ExecutePickup(Env.Collector);
	TestTrue("Actor destroyed after pickup", Pickup->IsActorBeingDestroyed());

	// bDestroyOnPickup = false leaves the actor alive (repeat-pickup test aid)
	AMordecaiItemPickup* Persistent = Env.SpawnPickup(MakeDef(FName("Potion_C")));
	Persistent->bDestroyOnPickup = false;
	Persistent->ExecutePickup(Env.Collector);
	TestFalse("Persistent pickup survives", Persistent->IsActorBeingDestroyed());

	Env.Teardown();
	return true;
}

// ===========================================================================
// 3. Mordecai.Items.ItemPickup.NullDefinitionNoOp (AC-079.2)
// ===========================================================================
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_Items_ItemPickup_NullDefinitionNoOp,
	"Mordecai.Items.ItemPickup.NullDefinitionNoOp",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_Items_ItemPickup_NullDefinitionNoOp::RunTest(const FString& Parameters)
{
	using namespace MordecaiItemPickupTestHelpers;
	FPickupTestEnv Env;
	if (!Env.Setup()) { AddError(TEXT("Failed to set up test world")); return false; }

	AMordecaiItemPickup* Pickup = Env.SpawnPickup(nullptr);
	const bool bPicked = Pickup->ExecutePickup(Env.Collector);

	TestFalse("Pickup with null definition returns false", bPicked);
	TestFalse("Actor not destroyed", Pickup->IsActorBeingDestroyed());
	TestEqual("Inventory untouched", Env.Inventory->GetAllItems().Num(), 0);

	// Collector without an inventory component is also a safe no-op
	AActor* BareActor = Env.World->SpawnActor<AActor>(AActor::StaticClass(), FActorSpawnParameters());
	AMordecaiItemPickup* Pickup2 = Env.SpawnPickup(MakeDef(FName("Potion_D")));
	TestFalse("No inventory component returns false", Pickup2->ExecutePickup(BareActor));
	TestFalse("Actor not destroyed without inventory", Pickup2->IsActorBeingDestroyed());

	// Null collector is a safe no-op
	TestFalse("Null collector returns false", Pickup2->ExecutePickup(nullptr));

	Env.Teardown();
	return true;
}

// ===========================================================================
// 4. Mordecai.Items.ItemPickup.NonAuthorityDoesNotPickup (AC-079.3)
// ===========================================================================
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_Items_ItemPickup_NonAuthorityDoesNotPickup,
	"Mordecai.Items.ItemPickup.NonAuthorityDoesNotPickup",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_Items_ItemPickup_NonAuthorityDoesNotPickup::RunTest(const FString& Parameters)
{
	using namespace MordecaiItemPickupTestHelpers;
	FPickupTestEnv Env;
	if (!Env.Setup()) { AddError(TEXT("Failed to set up test world")); return false; }

	UMordecaiItemDefinition* Def = MakeDef(FName("Potion_E"));
	AMordecaiItemPickup* Pickup = Env.SpawnPickup(Def);
	Pickup->SetRole(ROLE_SimulatedProxy);

	const bool bPicked = Pickup->ExecutePickup(Env.Collector);
	TestFalse("Non-authority ExecutePickup returns false", bPicked);
	TestEqual("Inventory untouched on non-authority", Env.Inventory->GetTotalQuantityOfDefinition(Def), 0);
	TestFalse("Actor not destroyed on non-authority", Pickup->IsActorBeingDestroyed());

	Env.Teardown();
	return true;
}

// ===========================================================================
// 5. Mordecai.Items.ItemPickup.AutoStoreRoutesToLedger (AC-079.2 + agent_rules §2)
// ===========================================================================
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_Items_ItemPickup_AutoStoreRoutesToLedger,
	"Mordecai.Items.ItemPickup.AutoStoreRoutesToLedger",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_Items_ItemPickup_AutoStoreRoutesToLedger::RunTest(const FString& Parameters)
{
	using namespace MordecaiItemPickupTestHelpers;
	FPickupTestEnv Env;
	if (!Env.Setup()) { AddError(TEXT("Failed to set up test world")); return false; }

	UMordecaiItemDefinition* Ore = MakeDef(FName("IronOre"), EMordecaiItemType::Material, TEXT("Iron Ore"));
	Ore->AutoStoreOnPickup = true;

	AMordecaiItemPickup* Pickup = Env.SpawnPickup(Ore, 2);
	TestTrue("Auto-store pickup succeeds", Pickup->ExecutePickup(Env.Collector));
	TestEqual("Flat inventory stays empty", Env.Inventory->GetAllItems().Num(), 0);
	TestEqual("Ledger holds the resource", Env.Ledger->GetResourceCount(FName("IronOre")), 2);
	TestTrue("Pickup consumed", Pickup->IsActorBeingDestroyed());

	Env.Teardown();
	return true;
}
