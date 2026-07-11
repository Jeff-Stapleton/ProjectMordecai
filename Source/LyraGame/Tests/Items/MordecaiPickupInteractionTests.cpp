// Project Mordecai — Pickup Interaction Component Tests (US-079)
// NullRHI-compatible headless tests for UMordecaiPickupInteractionComponent:
// nearest-pickup focus, stale-pointer pruning, focused-changed delegate, and
// TryPickupFocused dispatch.

#include "Misc/AutomationTest.h"
#include "Engine/World.h"
#include "Engine/Engine.h"

#include "Mordecai/Items/MordecaiInventoryComponent.h"
#include "Mordecai/Items/MordecaiItemDefinition.h"
#include "Mordecai/Items/MordecaiItemPickup.h"
#include "Mordecai/Items/MordecaiItemTypes.h"
#include "Mordecai/Items/MordecaiPickupInteractionComponent.h"
#include "MordecaiPickupTestHelpers.h"

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

namespace MordecaiPickupInteractionTestHelpers
{
	static UMordecaiItemDefinition* MakeDef(FName ItemId, const TCHAR* DisplayName = TEXT("Test Item"))
	{
		UMordecaiItemDefinition* Def = NewObject<UMordecaiItemDefinition>();
		Def->ItemId = ItemId;
		Def->ItemType = EMordecaiItemType::Consumable;
		Def->DisplayName = FText::FromString(DisplayName);
		return Def;
	}

	/** World + owner actor at origin carrying interaction + inventory components. */
	struct FInteractionTestEnv
	{
		UWorld* World = nullptr;
		AActor* Owner = nullptr;
		UMordecaiPickupInteractionComponent* Interaction = nullptr;
		UMordecaiInventoryComponent* Inventory = nullptr;

		bool Setup()
		{
			if (!GEngine) return false;

			World = UWorld::CreateWorld(EWorldType::Game, false);
			if (!World) return false;

			FWorldContext& WorldContext = GEngine->CreateNewWorldContext(EWorldType::Game);
			WorldContext.SetCurrentWorld(World);

			FActorSpawnParameters SpawnParams;
			Owner = World->SpawnActor<AActor>(AActor::StaticClass(), FVector::ZeroVector, FRotator::ZeroRotator, SpawnParams);
			if (!Owner) { Teardown(); return false; }

			Interaction = NewObject<UMordecaiPickupInteractionComponent>(Owner);
			Interaction->RegisterComponent();
			Inventory = NewObject<UMordecaiInventoryComponent>(Owner);
			Inventory->RegisterComponent();

			return true;
		}

		AMordecaiItemPickup* SpawnPickup(UMordecaiItemDefinition* Def, FVector Location)
		{
			FActorSpawnParameters SpawnParams;
			AMordecaiItemPickup* Pickup = World->SpawnActor<AMordecaiItemPickup>(
				AMordecaiItemPickup::StaticClass(), Location, FRotator::ZeroRotator, SpawnParams);
			if (Pickup)
			{
				Pickup->ItemDefinition = Def;
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
			Owner = nullptr;
			Interaction = nullptr;
			Inventory = nullptr;
		}
	};
}


// ===========================================================================
// 1. Mordecai.Items.PickupInteraction.NearestPickupIsFocused (AC-079.14)
// ===========================================================================
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_Items_PickupInteraction_NearestPickupIsFocused,
	"Mordecai.Items.PickupInteraction.NearestPickupIsFocused",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_Items_PickupInteraction_NearestPickupIsFocused::RunTest(const FString& Parameters)
{
	using namespace MordecaiPickupInteractionTestHelpers;
	FInteractionTestEnv Env;
	if (!Env.Setup()) { AddError(TEXT("Failed to set up test world")); return false; }

	AMordecaiItemPickup* Near = Env.SpawnPickup(MakeDef(FName("Near")), FVector(100, 0, 0));
	AMordecaiItemPickup* Far = Env.SpawnPickup(MakeDef(FName("Far")), FVector(500, 0, 0));

	// Register far first — focus starts on it, then swaps when a closer one arrives
	Env.Interaction->RegisterPickup(Far);
	TestTrue("Far focused while alone", Env.Interaction->GetFocusedPickup() == Far);

	Env.Interaction->RegisterPickup(Near);
	TestTrue("Near takes focus when registered", Env.Interaction->GetFocusedPickup() == Near);
	TestEqual("Both tracked in range", Env.Interaction->GetInRangePickupCount(), 2);

	// Duplicate registration does not double-track
	Env.Interaction->RegisterPickup(Near);
	TestEqual("Duplicate register ignored", Env.Interaction->GetInRangePickupCount(), 2);

	Env.Teardown();
	return true;
}

// ===========================================================================
// 2. Mordecai.Items.PickupInteraction.FocusClearsOnUnregisterAndDestroy (AC-079.14)
// ===========================================================================
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_Items_PickupInteraction_FocusClearsOnUnregisterAndDestroy,
	"Mordecai.Items.PickupInteraction.FocusClearsOnUnregisterAndDestroy",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_Items_PickupInteraction_FocusClearsOnUnregisterAndDestroy::RunTest(const FString& Parameters)
{
	using namespace MordecaiPickupInteractionTestHelpers;
	FInteractionTestEnv Env;
	if (!Env.Setup()) { AddError(TEXT("Failed to set up test world")); return false; }

	AMordecaiItemPickup* A = Env.SpawnPickup(MakeDef(FName("A")), FVector(100, 0, 0));
	AMordecaiItemPickup* B = Env.SpawnPickup(MakeDef(FName("B")), FVector(300, 0, 0));
	Env.Interaction->RegisterPickup(A);
	Env.Interaction->RegisterPickup(B);

	// Unregister the focused pickup — focus advances to the survivor
	Env.Interaction->UnregisterPickup(A);
	TestTrue("Focus advances to B after unregister", Env.Interaction->GetFocusedPickup() == B);

	// Destroy the focused pickup externally — stale weak pointer is pruned
	Env.World->DestroyActor(B);
	Env.Interaction->EvaluateFocus();
	TestNull("Focus cleared after focused pickup destroyed", Env.Interaction->GetFocusedPickup());
	TestEqual("Stale entries pruned", Env.Interaction->GetInRangePickupCount(), 0);

	Env.Teardown();
	return true;
}

// ===========================================================================
// 3. Mordecai.Items.PickupInteraction.TryPickupFocusedNoOpWithNoFocus (AC-079.15)
// ===========================================================================
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_Items_PickupInteraction_TryPickupFocusedNoOpWithNoFocus,
	"Mordecai.Items.PickupInteraction.TryPickupFocusedNoOpWithNoFocus",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_Items_PickupInteraction_TryPickupFocusedNoOpWithNoFocus::RunTest(const FString& Parameters)
{
	using namespace MordecaiPickupInteractionTestHelpers;
	FInteractionTestEnv Env;
	if (!Env.Setup()) { AddError(TEXT("Failed to set up test world")); return false; }

	TestFalse("TryPickupFocused with no focus returns false", Env.Interaction->TryPickupFocused());
	TestEqual("Inventory untouched", Env.Inventory->GetAllItems().Num(), 0);

	Env.Teardown();
	return true;
}

// ===========================================================================
// 4. Mordecai.Items.PickupInteraction.FocusedChangedDelegateFires (AC-079.15)
// ===========================================================================
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_Items_PickupInteraction_FocusedChangedDelegateFires,
	"Mordecai.Items.PickupInteraction.FocusedChangedDelegateFires",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_Items_PickupInteraction_FocusedChangedDelegateFires::RunTest(const FString& Parameters)
{
	using namespace MordecaiPickupInteractionTestHelpers;
	FInteractionTestEnv Env;
	if (!Env.Setup()) { AddError(TEXT("Failed to set up test world")); return false; }

	UMordecaiFocusedPickupDelegateSpy* Spy = NewObject<UMordecaiFocusedPickupDelegateSpy>();
	Env.Interaction->OnFocusedPickupChanged.AddDynamic(Spy, &UMordecaiFocusedPickupDelegateSpy::Handle);

	AMordecaiItemPickup* Pickup = Env.SpawnPickup(MakeDef(FName("Ore"), TEXT("Iron Ore")), FVector(100, 0, 0));
	Env.Interaction->RegisterPickup(Pickup);

	TestEqual("Delegate fired on focus gain", Spy->EventCount, 1);
	TestTrue("Delegate carries focused pickup", Spy->LastFocusedPickup.Get() == Pickup);
	TestEqual("Delegate carries item display name", Spy->LastItemDisplayName.ToString(), FString(TEXT("Iron Ore")));

	// Re-evaluating with no change must NOT re-broadcast
	Env.Interaction->EvaluateFocus();
	TestEqual("No re-broadcast without focus change", Spy->EventCount, 1);

	Env.Interaction->UnregisterPickup(Pickup);
	TestEqual("Delegate fired on focus loss", Spy->EventCount, 2);
	TestNull("Focus loss carries null pickup", Spy->LastFocusedPickup.Get());
	TestTrue("Focus loss carries empty display name", Spy->LastItemDisplayName.IsEmpty());

	Env.Teardown();
	return true;
}

// ===========================================================================
// 5. Mordecai.Items.PickupInteraction.TryPickupFocusedExecutesPickup (AC-079.15)
// ===========================================================================
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_Items_PickupInteraction_TryPickupFocusedExecutesPickup,
	"Mordecai.Items.PickupInteraction.TryPickupFocusedExecutesPickup",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_Items_PickupInteraction_TryPickupFocusedExecutesPickup::RunTest(const FString& Parameters)
{
	using namespace MordecaiPickupInteractionTestHelpers;
	FInteractionTestEnv Env;
	if (!Env.Setup()) { AddError(TEXT("Failed to set up test world")); return false; }

	UMordecaiItemDefinition* Def = MakeDef(FName("Sword"), TEXT("Rusty Sword"));
	AMordecaiItemPickup* Pickup = Env.SpawnPickup(Def, FVector(100, 0, 0));
	Env.Interaction->RegisterPickup(Pickup);

	TestTrue("TryPickupFocused succeeds", Env.Interaction->TryPickupFocused());
	TestEqual("Item routed into the inventory", Env.Inventory->GetTotalQuantityOfDefinition(Def), 1);
	TestTrue("Pickup consumed", Pickup->IsActorBeingDestroyed());
	TestNull("Focus cleared after consume", Env.Interaction->GetFocusedPickup());
	TestEqual("Consumed pickup unregistered", Env.Interaction->GetInRangePickupCount(), 0);

	Env.Teardown();
	return true;
}
