// Project Mordecai — Item Pickup Actor Tests (US-079)
// NullRHI-compatible headless tests for AMordecaiItemPickup: press-to-pickup
// execution, destroy-on-pickup, null-safety, server-authority gating, and the
// name label that faces the camera (issues #30, #38).

#include "Misc/AutomationTest.h"
#include "Camera/PlayerCameraManager.h"
#include "Components/TextRenderComponent.h"
#include "Engine/World.h"
#include "Engine/Engine.h"
#include "Engine/LocalPlayer.h"
#include "GameFramework/PlayerController.h"

#include "Mordecai/Camera/MordecaiCameraMode_Diorama.h"
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

		AMordecaiItemPickup* SpawnPickup(UMordecaiItemDefinition* Def, int32 Quantity = 1, FVector Location = FVector::ZeroVector,
			FRotator Rotation = FRotator::ZeroRotator)
		{
			FActorSpawnParameters SpawnParams;
			AMordecaiItemPickup* Pickup = World->SpawnActor<AMordecaiItemPickup>(
				AMordecaiItemPickup::StaticClass(), Location, Rotation, SpawnParams);
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

	/**
	 * How squarely a TextRender at LabelRotation faces a camera at CameraRotation: its readable face
	 * (local +X; engine TextRenderComponent.cpp lays glyphs out at local (0, -x, -y)) against the
	 * camera's backward vector. 1 = square to the screen, below 0 = the camera sees its back.
	 */
	static double LabelFacing(const FRotator& LabelRotation, const FRotator& CameraRotation)
	{
		return FVector::DotProduct(FRotationMatrix(LabelRotation).GetUnitAxis(EAxis::X), -FRotationMatrix(CameraRotation).GetUnitAxis(EAxis::X));
	}

	/** How level its text reads on screen: the way it runs (local -Y) against the camera's right. 1 = left to right, 0 = turned 90 degrees. */
	static double LabelUpright(const FRotator& LabelRotation, const FRotator& CameraRotation)
	{
		return FVector::DotProduct(-FRotationMatrix(LabelRotation).GetUnitAxis(EAxis::Y), FRotationMatrix(CameraRotation).GetUnitAxis(EAxis::Y));
	}

	/**
	 * A world initialized as on a map load, so spawned player controllers are in its controller list with camera
	 * managers: first a remote player's controller (no ULocalPlayer, as on a listen server), then the local one.
	 */
	struct FLocalCameraTestEnv
	{
		UWorld* World = nullptr;
		APlayerController* RemotePC = nullptr;
		APlayerController* PC = nullptr;
		ULocalPlayer* LocalPlayer = nullptr;

		bool Setup()
		{
			if (!GEngine) return false;

			World = UWorld::CreateWorld(EWorldType::Game, false);
			if (!World) return false;

			FWorldContext& WorldContext = GEngine->CreateNewWorldContext(EWorldType::Game);
			WorldContext.SetCurrentWorld(World);
			World->InitializeActorsForPlay(FURL());

			// APlayerController::PostInitializeComponents spawns its camera manager
			RemotePC = World->SpawnActor<APlayerController>(APlayerController::StaticClass());
			PC = World->SpawnActor<APlayerController>(APlayerController::StaticClass());
			if (!RemotePC || !PC) return false;

			// Without a net driver a controller is local only when it has a ULocalPlayer (APlayerController::IsLocalController)
			LocalPlayer = NewObject<ULocalPlayer>(GEngine, ULocalPlayer::StaticClass());
			PC->Player = LocalPlayer;
			LocalPlayer->PlayerController = PC;
			return PC->IsLocalController() && !RemotePC->IsLocalController() && PC->PlayerCameraManager && RemotePC->PlayerCameraManager;
		}

		/** What a camera manager's per-frame update does: cache the view at the current world time. */
		void SetCameraView(APlayerController* Controller, const FRotator& Rotation)
		{
			World->TimeSeconds += 1.0;
			FMinimalViewInfo View;
			View.Rotation = Rotation;
			Controller->PlayerCameraManager->FillCameraCache(View);
		}

		void Teardown()
		{
			if (PC) PC->Player = nullptr;
			if (LocalPlayer) LocalPlayer->PlayerController = nullptr;
			if (World && GEngine)
			{
				GEngine->DestroyWorldContext(World);
				World->DestroyWorld(false);
			}
			World = nullptr;
			RemotePC = nullptr;
			PC = nullptr;
			LocalPlayer = nullptr;
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

// ===========================================================================
// 6. Mordecai.Items.ItemPickup.NameLabelRotationFacesAnyCamera (AC-079.10, issue #38)
// ===========================================================================
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_Items_ItemPickup_NameLabelRotationFacesAnyCamera,
	"Mordecai.Items.ItemPickup.NameLabelRotationFacesAnyCamera",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_Items_ItemPickup_NameLabelRotationFacesAnyCamera::RunTest(const FString& Parameters)
{
	using namespace MordecaiItemPickupTestHelpers;

	// The diorama camera (pitch -50, yaw 0, looking along +X): the axes the engine's glyph layout needs
	const FRotationMatrix Diorama(AMordecaiItemPickup::ComputeNameLabelRotation(FRotator(-50.0, 0.0, 0.0)));
	TestTrue("face (+X) points back up the view", Diorama.GetUnitAxis(EAxis::X).Equals(FVector(-0.6428, 0.0, 0.7660), 1e-3));
	TestTrue("text (-Y) runs toward screen right (+Y)", (-Diorama.GetUnitAxis(EAxis::Y)).Equals(FVector(0.0, 1.0, 0.0), 1e-3));
	TestTrue("text up (+Z) is the camera's up", Diorama.GetUnitAxis(EAxis::Z).Equals(FVector(0.7660, 0.0, 0.6428), 1e-3));

	// Wherever the camera is placed, the label is square to it and reads left to right
	const TArray<FRotator> Cameras = {
		FRotator(-50.0, 0.0, 0.0), FRotator(-30.0, 45.0, 0.0), FRotator(-89.0, 0.0, 0.0),
		FRotator(0.0, -135.0, 0.0), FRotator(-60.0, 200.0, -10.0), FRotator(-50.0, 0.0, 20.0) };
	for (const FRotator& Camera : Cameras)
	{
		const FRotator Label = AMordecaiItemPickup::ComputeNameLabelRotation(Camera);
		const double Facing = LabelFacing(Label, Camera);
		const double Upright = LabelUpright(Label, Camera);
		TestTrue(*FString::Printf(TEXT("square to camera %s (facing %.3f)"), *Camera.ToCompactString(), Facing), Facing > 0.999);
		TestTrue(*FString::Printf(TEXT("reads left to right for camera %s (upright %.3f)"), *Camera.ToCompactString(), Upright), Upright > 0.999);
	}
	return true;
}

// ===========================================================================
// 7. Mordecai.Items.ItemPickup.NameLabelFacesTheDioramaCameraWhateverTheActorRotation (AC-079.10, issue #38)
// ===========================================================================
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_Items_ItemPickup_NameLabelFacesTheDioramaCameraWhateverTheActorRotation,
	"Mordecai.Items.ItemPickup.NameLabelFacesTheDioramaCameraWhateverTheActorRotation",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_Items_ItemPickup_NameLabelFacesTheDioramaCameraWhateverTheActorRotation::RunTest(const FString& Parameters)
{
	using namespace MordecaiItemPickupTestHelpers;
	FPickupTestEnv Env;
	if (!Env.Setup()) { AddError(TEXT("Failed to set up test world")); return false; }

	// Before any camera update (and on a machine without one) the label faces the default diorama camera
	const FRotator DioramaCamera(GetDefault<UMordecaiCameraMode_Diorama>()->PitchAngle, 0.0, 0.0);
	const TArray<FRotator> ActorRotations = {
		FRotator::ZeroRotator, FRotator(0.0, 90.0, 0.0), FRotator(0.0, -135.0, 0.0), FRotator(20.0, 45.0, 10.0) };
	for (const FRotator& ActorRotation : ActorRotations)
	{
		AMordecaiItemPickup* Pickup = Env.SpawnPickup(MakeDef(FName("Potion_F")), 1, FVector(200.0, 100.0, 30.0), ActorRotation);
		const UTextRenderComponent* Label = Pickup ? Pickup->GetNameLabel() : nullptr;
		if (!TestNotNull("pickup with a name label", Label))
		{
			break;
		}
		const FString Actor = ActorRotation.ToCompactString();
		TestTrue(*FString::Printf(TEXT("actor %s: the label ignores the actor's rotation"), *Actor), Label->IsUsingAbsoluteRotation());
		const double Facing = LabelFacing(Label->GetComponentRotation(), DioramaCamera);
		const double Upright = LabelUpright(Label->GetComponentRotation(), DioramaCamera);
		TestTrue(*FString::Printf(TEXT("actor %s: square to the diorama camera (facing %.3f)"), *Actor, Facing), Facing > 0.999);
		TestTrue(*FString::Printf(TEXT("actor %s: reads left to right (upright %.3f)"), *Actor, Upright), Upright > 0.999);
	}

	Env.Teardown();
	return true;
}

// ===========================================================================
// 8. Mordecai.Items.ItemPickup.NameLabelFollowsTheLocalCamera (AC-079.10, issue #38)
// ===========================================================================
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_Items_ItemPickup_NameLabelFollowsTheLocalCamera,
	"Mordecai.Items.ItemPickup.NameLabelFollowsTheLocalCamera",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_Items_ItemPickup_NameLabelFollowsTheLocalCamera::RunTest(const FString& Parameters)
{
	using namespace MordecaiItemPickupTestHelpers;
	FLocalCameraTestEnv Env;
	if (!Env.Setup()) { AddError(TEXT("Failed to set up a world with a local camera")); Env.Teardown(); return false; }

	AMordecaiItemPickup* Pickup = Env.World->SpawnActor<AMordecaiItemPickup>(
		AMordecaiItemPickup::StaticClass(), FVector(300.0, 200.0, 30.0), FRotator(0.0, 90.0, 0.0), FActorSpawnParameters());
	UTextRenderComponent* Label = Pickup ? Pickup->GetNameLabel() : nullptr;
	if (!TestNotNull("pickup with a name label", Label)) { Env.Teardown(); return false; }

	// No view yet: a camera that has never updated, or none at all, leaves the label alone
	const FRotator Unchanged(10.0, 20.0, 30.0);
	Label->SetWorldRotation(Unchanged);
	TestFalse("no view before the camera's first update", Pickup->FaceNameLabelTo(Env.PC->PlayerCameraManager));
	TestFalse("no camera", Pickup->FaceNameLabelTo(nullptr));
	Pickup->Tick(1.f / 60.f);
	TestTrue("rotation untouched without a view", Label->GetComponentRotation().Equals(Unchanged, 1e-3));

	// Every frame the label turns to wherever the local camera looks, never to a remote player's camera
	Env.SetCameraView(Env.RemotePC, FRotator(-20.0, -90.0, 0.0));
	const TArray<FRotator> Cameras = {
		FRotator(-50.0, 0.0, 0.0), FRotator(-30.0, 45.0, 0.0), FRotator(-70.0, -120.0, 0.0), FRotator(-40.0, 160.0, 15.0) };
	for (const FRotator& Camera : Cameras)
	{
		Env.SetCameraView(Env.PC, Camera);
		Pickup->Tick(1.f / 60.f);
		const double Facing = LabelFacing(Label->GetComponentRotation(), Camera);
		const double Upright = LabelUpright(Label->GetComponentRotation(), Camera);
		TestTrue(*FString::Printf(TEXT("square to camera %s (facing %.3f)"), *Camera.ToCompactString(), Facing), Facing > 0.999);
		TestTrue(*FString::Printf(TEXT("reads left to right for camera %s (upright %.3f)"), *Camera.ToCompactString(), Upright), Upright > 0.999);
	}
	TestTrue("FaceNameLabelTo reports a known view", Pickup->FaceNameLabelTo(Env.PC->PlayerCameraManager));

	Env.Teardown();
	return true;
}

// ===========================================================================
// 9. Mordecai.Items.ItemPickup.NameLabelTicksOnClientsAfterTheCamera (issue #38)
// ===========================================================================
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_Items_ItemPickup_NameLabelTicksOnClientsAfterTheCamera,
	"Mordecai.Items.ItemPickup.NameLabelTicksOnClientsAfterTheCamera",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_Items_ItemPickup_NameLabelTicksOnClientsAfterTheCamera::RunTest(const FString& Parameters)
{
	const AMordecaiItemPickup* CDO = GetDefault<AMordecaiItemPickup>();
	TestTrue("pickups tick (the label follows the camera)", CDO->PrimaryActorTick.bCanEverTick);
	// The camera updates after TG_PostPhysics and before TG_PostUpdateWork (engine LevelTick.cpp): earlier is a frame late
	TestTrue("ticks in TG_PostUpdateWork, after the camera update", CDO->PrimaryActorTick.TickGroup == TG_PostUpdateWork);
	TestFalse("never ticks on a dedicated server (no local camera)", CDO->PrimaryActorTick.bAllowTickOnDedicatedServer);
	return true;
}
