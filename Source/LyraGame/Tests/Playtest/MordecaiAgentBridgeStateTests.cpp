// Project Mordecai — Agent bridge state serialization tests (Specs/agent_bridge_v1.md §4 state)
// Headless tests for the read-only state gatherers, driven by NewObject'd components and actors.

#include "Misc/AutomationTest.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "Blueprint/WidgetTree.h"
#include "Components/TextBlock.h"
#include "Dom/JsonObject.h"
#include "Dom/JsonValue.h"
#include "Engine/Engine.h"
#include "Engine/LocalPlayer.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "Layout/Geometry.h"
#include "Misc/Guid.h"

#include "AbilitySystem/Attributes/LyraHealthSet.h"
#include "GameModes/LyraGameState.h"
#include "Mordecai/AbilitySystem/MordecaiAttributeSet.h"
#include "Mordecai/Combat/MordecaiProjectile.h"
#include "Mordecai/Enemy/MordecaiEnemyCharacter.h"
#include "Mordecai/Items/MordecaiInventoryComponent.h"
#include "Mordecai/Items/MordecaiItemDefinition.h"
#include "Mordecai/Items/MordecaiItemPickup.h"
#include "Mordecai/Items/MordecaiPickupInteractionComponent.h"
#include "Mordecai/Items/MordecaiResourceLedger.h"
#include "Mordecai/Magic/MordecaiIllusionActor.h"
#include "Mordecai/MordecaiPlayerState.h"
#include "Mordecai/Playtest/MordecaiAgentBridgeProtocol.h"
#include "Mordecai/Playtest/MordecaiAgentBridgeState.h"
#include "Mordecai/UI/MordecaiInventoryWidget.h"
#include "Mordecai/UI/MordecaiPauseMenuWidget.h"
#include "Mordecai/UI/MordecaiPickupPromptWidget.h"
#include "Mordecai/UI/MordecaiTabButton.h"

namespace MordecaiAgentBridgeStateTestHelpers
{
	static UMordecaiItemDefinition* MakeDef(FName ItemId, const TCHAR* DisplayName, EMordecaiItemType Type,
		EMordecaiItemRarity Rarity = EMordecaiItemRarity::Common)
	{
		UMordecaiItemDefinition* Def = NewObject<UMordecaiItemDefinition>();
		Def->ItemId = ItemId;
		Def->DisplayName = FText::FromString(DisplayName);
		Def->ItemType = Type;
		Def->Rarity = Rarity;
		return Def;
	}

	static double Number(const TSharedPtr<FJsonObject>& Object, const TCHAR* Field)
	{
		double Value = -1.0;
		return Object.IsValid() && Object->TryGetNumberField(Field, Value) ? Value : -12345.0;
	}

	static FVector VectorField(const TSharedPtr<FJsonObject>& Object, const TCHAR* Field)
	{
		const TArray<TSharedPtr<FJsonValue>>* Array = nullptr;
		if (!Object.IsValid() || !Object->TryGetArrayField(Field, Array) || Array->Num() != 3)
		{
			return FVector(-12345.0);
		}
		return FVector((*Array)[0]->AsNumber(), (*Array)[1]->AsNumber(), (*Array)[2]->AsNumber());
	}

	static bool IsNullField(const TSharedPtr<FJsonObject>& Object, const TCHAR* Field)
	{
		return Object.IsValid() && Object->HasTypedField<EJson::Null>(Field);
	}

	/** A game world for actors that must be spawned (pickups). */
	struct FBridgeStateTestWorld
	{
		UWorld* World = nullptr;

		bool Setup()
		{
			if (!GEngine)
			{
				return false;
			}
			World = UWorld::CreateWorld(EWorldType::Game, false);
			if (!World)
			{
				return false;
			}
			FWorldContext& WorldContext = GEngine->CreateNewWorldContext(EWorldType::Game);
			WorldContext.SetCurrentWorld(World);
			return true;
		}

		void Teardown()
		{
			if (World && GEngine)
			{
				GEngine->DestroyWorldContext(World);
				World->DestroyWorld(false);
			}
			World = nullptr;
		}
	};

	/**
	 * A game world whose actors are initialized as on a map load (InitializeActorsForPlay, so spawned
	 * actors run PostInitializeComponents and their ASCs register their attribute sets), an optional
	 * Lyra game state (Lyra player states require one), and a player controller with a ULocalPlayer so
	 * native user widgets get a valid player context and build their trees in NativeOnInitialized.
	 */
	struct FBridgeGameTestEnv
	{
		UWorld* World = nullptr;
		APlayerController* PC = nullptr;
		ULocalPlayer* LocalPlayer = nullptr;

		bool Setup(bool bWithGameState)
		{
			if (!GEngine)
			{
				return false;
			}
			World = UWorld::CreateWorld(EWorldType::Game, false);
			if (!World)
			{
				return false;
			}
			FWorldContext& WorldContext = GEngine->CreateNewWorldContext(EWorldType::Game);
			WorldContext.SetCurrentWorld(World);
			World->InitializeActorsForPlay(FURL());

			if (bWithGameState && !World->SpawnActor<ALyraGameState>(ALyraGameState::StaticClass()))
			{
				return false;
			}
			PC = World->SpawnActor<APlayerController>(APlayerController::StaticClass());
			if (!PC)
			{
				return false;
			}
			LocalPlayer = NewObject<ULocalPlayer>(GEngine, ULocalPlayer::StaticClass());
			PC->Player = LocalPlayer;
			LocalPlayer->PlayerController = PC;
			return FLocalPlayerContext(LocalPlayer, World).IsValid();
		}

		/** CreateWidget's path for a native widget: player context, then Initialize (runs NativeOnInitialized). */
		template <typename TWidget>
		TWidget* MakeUserWidget()
		{
			TWidget* Widget = NewObject<TWidget>(World);
			Widget->SetPlayerContext(FLocalPlayerContext(LocalPlayer, World));
			Widget->Initialize();
			return Widget;
		}

		void Teardown()
		{
			if (PC)
			{
				PC->Player = nullptr;
			}
			if (LocalPlayer)
			{
				LocalPlayer->PlayerController = nullptr;
			}
			if (World && GEngine)
			{
				GEngine->DestroyWorldContext(World);
				World->DestroyWorld(false);
			}
			World = nullptr;
			PC = nullptr;
			LocalPlayer = nullptr;
		}
	};

	static TSharedPtr<FJsonObject> ObjectAt(const TSharedPtr<FJsonObject>& Object, const TCHAR* Field)
	{
		const TSharedPtr<FJsonObject>* Out = nullptr;
		return Object.IsValid() && Object->TryGetObjectField(Field, Out) ? *Out : nullptr;
	}

	static TArray<TSharedPtr<FJsonValue>> ArrayAt(const TSharedPtr<FJsonObject>& Object, const TCHAR* Field)
	{
		const TArray<TSharedPtr<FJsonValue>>* Out = nullptr;
		return Object.IsValid() && Object->TryGetArrayField(Field, Out) ? *Out : TArray<TSharedPtr<FJsonValue>>();
	}

	static FString StringAt(const TSharedPtr<FJsonObject>& Object, const TCHAR* Field)
	{
		FString Out;
		return Object.IsValid() && Object->TryGetStringField(Field, Out) ? Out : FString(TEXT("<missing>"));
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_Playtest_AgentBridge_State_InventoryAndLedger,
	"Mordecai.Playtest.AgentBridge.State.InventoryAndLedger",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_Playtest_AgentBridge_State_InventoryAndLedger::RunTest(const FString& Parameters)
{
	using namespace MordecaiAgentBridgeStateTestHelpers;

	UMordecaiInventoryComponent* Inventory = NewObject<UMordecaiInventoryComponent>();
	UMordecaiResourceLedger* Ledger = NewObject<UMordecaiResourceLedger>();
	Inventory->SetResourceLedger(Ledger);

	UMordecaiItemDefinition* Potion = MakeDef(TEXT("TestItem_HealthPotion"), TEXT("Health Potion"), EMordecaiItemType::Consumable);
	Potion->Stackable = true;
	Potion->MaxStackSize = 10;
	UMordecaiItemDefinition* Sword = MakeDef(TEXT("TestItem_RustySword"), TEXT("Rusty Sword"), EMordecaiItemType::Weapon);
	UMordecaiItemDefinition* Amulet = MakeDef(TEXT("TestItem_MysteriousAmulet"), TEXT("Mysterious Amulet"), EMordecaiItemType::MagicalItem, EMordecaiItemRarity::Purple);
	Amulet->Subtype = TEXT("Amulet");
	Amulet->UsesIdentification = true;
	Amulet->DefaultIdentificationState = EMordecaiIdentificationState::Unidentified;
	UMordecaiItemDefinition* Ore = MakeDef(TEXT("TestItem_IronOre"), TEXT("Iron Ore"), EMordecaiItemType::Material);
	Ore->AutoStoreOnPickup = true;
	Ore->Stackable = true;
	Ore->MaxStackSize = 99;
	UMordecaiItemDefinition* Stone = MakeDef(TEXT("TestItem_TownStone"), TEXT("Town Stone"), EMordecaiItemType::TownResource);
	Stone->AutoStoreOnPickup = true;
	Stone->Stackable = true;
	Stone->MaxStackSize = 99;

	Inventory->PickupItem(Potion, 2);
	Inventory->PickupItem(Sword, 1);
	Inventory->PickupItem(Amulet, 1);
	Inventory->PickupItem(Ore, 3);
	Inventory->PickupItem(Stone, 1);

	const TSharedRef<FJsonObject> Section = MordecaiAgentBridge::State::GatherInventory(Inventory, Ledger);
	TestTrue("valid", Section->GetBoolField(TEXT("valid")));
	TestEqual("count (instances)", Number(Section, TEXT("count")), 3.0);
	TestEqual("ledger_count", Number(Section, TEXT("ledger_count")), 2.0);

	const TArray<TSharedPtr<FJsonValue>>* Items = nullptr;
	if (TestTrue("items array", Section->TryGetArrayField(TEXT("items"), Items)) && TestEqual("three items", Items->Num(), 3))
	{
		const TArray<FMordecaiItemInstance> Sorted = Inventory->GetSortedItems();
		// UI order (GetSortedItems): weapon, magical item, consumable
		const TSharedPtr<FJsonObject> First = (*Items)[0]->AsObject();
		TestEqual("first item_id", First->GetStringField(TEXT("item_id")), FString(TEXT("TestItem_RustySword")));
		TestEqual("first display_name", First->GetStringField(TEXT("display_name")), FString(TEXT("Rusty Sword")));
		TestEqual("first type", First->GetStringField(TEXT("type")), FString(TEXT("Weapon")));
		TestEqual("first rarity", First->GetStringField(TEXT("rarity")), FString(TEXT("Common")));
		TestTrue("first identified", First->GetBoolField(TEXT("identified")));
		TestFalse("first not equipped", First->GetBoolField(TEXT("equipped")));
		TestEqual("first quantity", Number(First, TEXT("quantity")), 1.0);
		TestEqual("instance_id is the 32-digit guid", First->GetStringField(TEXT("instance_id")), Sorted[0].InstanceId.ToString());

		const TSharedPtr<FJsonObject> Second = (*Items)[1]->AsObject();
		TestEqual("amulet item_id", Second->GetStringField(TEXT("item_id")), FString(TEXT("TestItem_MysteriousAmulet")));
		TestEqual("amulet shows the identification-aware name", Second->GetStringField(TEXT("display_name")), FString(TEXT("Unidentified Purple Amulet")));
		TestFalse("amulet unidentified", Second->GetBoolField(TEXT("identified")));
		TestEqual("amulet rarity", Second->GetStringField(TEXT("rarity")), FString(TEXT("Purple")));
		TestEqual("amulet type", Second->GetStringField(TEXT("type")), FString(TEXT("MagicalItem")));

		const TSharedPtr<FJsonObject> Third = (*Items)[2]->AsObject();
		TestEqual("potion item_id", Third->GetStringField(TEXT("item_id")), FString(TEXT("TestItem_HealthPotion")));
		TestEqual("potion stack quantity", Number(Third, TEXT("quantity")), 2.0);
	}

	const TArray<TSharedPtr<FJsonValue>>* LedgerEntries = nullptr;
	if (TestTrue("ledger array", Section->TryGetArrayField(TEXT("ledger"), LedgerEntries)) && TestEqual("two ledger entries", LedgerEntries->Num(), 2))
	{
		bool bFoundOre = false;
		for (const TSharedPtr<FJsonValue>& Entry : *LedgerEntries)
		{
			const TSharedPtr<FJsonObject> Object = Entry->AsObject();
			if (Object->GetStringField(TEXT("item_id")) == TEXT("TestItem_IronOre"))
			{
				bFoundOre = true;
				TestEqual("ore display_name", Object->GetStringField(TEXT("display_name")), FString(TEXT("Iron Ore")));
				TestEqual("ore count", Number(Object, TEXT("count")), 3.0);
			}
		}
		TestTrue("ore in the ledger", bFoundOre);
	}

	// Identification changes what is reported
	Inventory->SetInstanceIdentificationState(Inventory->GetSortedItems()[1].InstanceId, EMordecaiIdentificationState::Identified);
	const TSharedRef<FJsonObject> After = MordecaiAgentBridge::State::GatherInventory(Inventory, Ledger);
	const TArray<TSharedPtr<FJsonValue>>* AfterItems = nullptr;
	if (After->TryGetArrayField(TEXT("items"), AfterItems) && AfterItems->Num() == 3)
	{
		const TSharedPtr<FJsonObject> Amulet2 = (*AfterItems)[1]->AsObject();
		TestEqual("identified name", Amulet2->GetStringField(TEXT("display_name")), FString(TEXT("Mysterious Amulet")));
		TestTrue("identified", Amulet2->GetBoolField(TEXT("identified")));
	}

	// Missing sources are invalid, never invented
	const TSharedRef<FJsonObject> Missing = MordecaiAgentBridge::State::GatherInventory(nullptr, nullptr);
	TestFalse("no inventory -> invalid", Missing->GetBoolField(TEXT("valid")));
	TestFalse("invalid has a reason", Missing->GetStringField(TEXT("reason")).IsEmpty());
	TestFalse("invalid invents no count", Missing->HasField(TEXT("count")));

	const TSharedRef<FJsonObject> NoLedger = MordecaiAgentBridge::State::GatherInventory(Inventory, nullptr);
	TestTrue("inventory without a ledger is valid", NoLedger->GetBoolField(TEXT("valid")));
	TestTrue("ledger_count is null without a ledger", IsNullField(NoLedger, TEXT("ledger_count")));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_Playtest_AgentBridge_State_Attributes,
	"Mordecai.Playtest.AgentBridge.State.Attributes",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_Playtest_AgentBridge_State_Attributes::RunTest(const FString& Parameters)
{
	using namespace MordecaiAgentBridgeStateTestHelpers;

	UMordecaiAttributeSet* Attributes = NewObject<UMordecaiAttributeSet>();
	Attributes->InitHealth(75.f);
	Attributes->InitMaxHealth(120.f);
	Attributes->InitStamina(-12.5f);
	Attributes->InitMaxStamina(100.f);
	Attributes->InitPosture(40.f);
	Attributes->InitMaxPosture(100.f);
	Attributes->InitSpellPoints(7.f);
	Attributes->InitMaxSpellPoints(10.f);

	const TSharedRef<FJsonObject> Json = MordecaiAgentBridge::State::MakeAttributes(Attributes);
	TestEqual("health", Number(Json, TEXT("health")), 75.0);
	TestEqual("max_health", Number(Json, TEXT("max_health")), 120.0);
	TestEqual("negative stamina is allowed", Number(Json, TEXT("stamina")), -12.5);
	TestEqual("max_stamina", Number(Json, TEXT("max_stamina")), 100.0);
	TestEqual("posture", Number(Json, TEXT("posture")), 40.0);
	TestEqual("max_posture", Number(Json, TEXT("max_posture")), 100.0);
	TestEqual("spell_points", Number(Json, TEXT("spell_points")), 7.0);
	TestEqual("max_spell_points", Number(Json, TEXT("max_spell_points")), 10.0);
	TestEqual("exactly the eight resource fields", Json->Values.Num(), 8);

	const TSharedRef<FJsonObject> Missing = MordecaiAgentBridge::State::MakeAttributes(nullptr);
	TestFalse("no attribute set -> invalid", Missing->GetBoolField(TEXT("valid")));
	TestFalse("no invented health", Missing->HasField(TEXT("health")));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_Playtest_AgentBridge_State_PickupEntries,
	"Mordecai.Playtest.AgentBridge.State.PickupEntries",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_Playtest_AgentBridge_State_PickupEntries::RunTest(const FString& Parameters)
{
	using namespace MordecaiAgentBridgeStateTestHelpers;

	FBridgeStateTestWorld Env;
	if (!TestTrue("world created", Env.Setup()))
	{
		return true;
	}

	AMordecaiItemPickup* Pickup = Env.World->SpawnActor<AMordecaiItemPickup>(
		AMordecaiItemPickup::StaticClass(), FVector(400.0, 0.0, 30.0), FRotator::ZeroRotator, FActorSpawnParameters());
	if (!TestNotNull("pickup spawned", Pickup))
	{
		Env.Teardown();
		return true;
	}
	Pickup->ItemDefinition = MakeDef(TEXT("TestItem_HealthPotion"), TEXT("Health Potion"), EMordecaiItemType::Consumable);
#if WITH_EDITOR
	Pickup->SetActorLabel(TEXT("TestPickup_HealthPotion"), /*bMarkDirty*/ false);
	const FString ExpectedLabel = TEXT("TestPickup_HealthPotion");
#else
	const FString ExpectedLabel = Pickup->GetActorNameOrLabel();
#endif

	const FVector Origin(0.0, 0.0, 92.0);
	const TSharedRef<FJsonObject> Entry = MordecaiAgentBridge::State::MakePickupEntry(Pickup, &Origin);
	TestEqual("label is GetActorNameOrLabel", Entry->GetStringField(TEXT("label")), ExpectedLabel);
	TestEqual("item_id", Entry->GetStringField(TEXT("item_id")), FString(TEXT("TestItem_HealthPotion")));
	TestEqual("display_name", Entry->GetStringField(TEXT("display_name")), FString(TEXT("Health Potion")));
	TestEqual("location", VectorField(Entry, TEXT("location")), FVector(400.0, 0.0, 30.0));
	TestEqual("distance from the origin", Number(Entry, TEXT("distance")), FVector::Dist(Origin, FVector(400.0, 0.0, 30.0)), 0.001);

	const TSharedRef<FJsonObject> NoOrigin = MordecaiAgentBridge::State::MakePickupEntry(Pickup, nullptr);
	TestTrue("distance is null without an origin", IsNullField(NoOrigin, TEXT("distance")));

	// The same pickup as an actors[] entry
	const TSharedRef<FJsonObject> Actor = MordecaiAgentBridge::State::MakeActorEntry(Pickup, &Origin);
	TestEqual("actor label", Actor->GetStringField(TEXT("label")), ExpectedLabel);
	TestEqual("actor kind", Actor->GetStringField(TEXT("kind")), FString(TEXT("pickup")));
	TestEqual("actor native class", Actor->GetStringField(TEXT("native_class")), FString(TEXT("MordecaiItemPickup")));
	TestEqual("actor name", Actor->GetStringField(TEXT("name")), Pickup->GetName());
	TestTrue("pickup has no health", IsNullField(Actor, TEXT("health")));
	TestTrue("pickup has no is_dead", IsNullField(Actor, TEXT("is_dead")));
	TestTrue("pickup has no ai_state", IsNullField(Actor, TEXT("ai_state")));
	TestFalse("not hidden", Actor->GetBoolField(TEXT("hidden")));
	TestFalse("display mesh without a mesh is not visible", Actor->GetBoolField(TEXT("has_visible_mesh")));
	TestTrue("pickups are listed by default", MordecaiAgentBridge::State::IsInterestingActor(Pickup));

	AActor* Plain = Env.World->SpawnActor<AActor>(AActor::StaticClass(), FTransform::Identity, FActorSpawnParameters());
	TestEqual("plain actor kind", MordecaiAgentBridge::State::ClassifyActorKind(Plain), FString(TEXT("other")));
	TestFalse("plain actors are level noise", MordecaiAgentBridge::State::IsInterestingActor(Plain));

	Env.Teardown();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_Playtest_AgentBridge_State_EnemyActorEntry,
	"Mordecai.Playtest.AgentBridge.State.EnemyActorEntry",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_Playtest_AgentBridge_State_EnemyActorEntry::RunTest(const FString& Parameters)
{
	using namespace MordecaiAgentBridgeStateTestHelpers;

	// A NewObject'd enemy never ran InitializeComponent, so its ASC has no registered sets: this covers
	// the GetMordecaiAttributeSet() fallback. The ASC branch is EnemyHealthFromItsOwnASC below.
	AMordecaiEnemyCharacter* Enemy = NewObject<AMordecaiEnemyCharacter>();
	if (!TestNotNull("enemy created", Enemy) || !TestNotNull("enemy attribute set", Enemy->GetMordecaiAttributeSet()))
	{
		return true;
	}
	Enemy->GetMordecaiAttributeSet()->InitHealth(150.f);
	Enemy->GetMordecaiAttributeSet()->InitMaxHealth(200.f);
	TestNull("no set registered on the uninitialized ASC (fallback path)", Enemy->GetAbilitySystemComponent()->GetSet<UMordecaiAttributeSet>());

	float Current = 0.f;
	float Max = 0.f;
	TestTrue("enemy health found", MordecaiAgentBridge::State::GetActorHealth(Enemy, Current, Max));
	TestEqual("current health from the fallback set", Current, 150.f);
	TestEqual("max health from the fallback set", Max, 200.f);

	const FVector Origin(100.0, 0.0, 0.0);
	const TSharedRef<FJsonObject> Entry = MordecaiAgentBridge::State::MakeActorEntry(Enemy, &Origin);
	TestEqual("kind", Entry->GetStringField(TEXT("kind")), FString(TEXT("enemy")));
	TestEqual("native_class", Entry->GetStringField(TEXT("native_class")), FString(TEXT("MordecaiEnemyCharacter")));
	TestEqual("class", Entry->GetStringField(TEXT("class")), FString(TEXT("MordecaiEnemyCharacter")));
	TestEqual("distance", Number(Entry, TEXT("distance")), 100.0);
	const TSharedPtr<FJsonObject>* Health = nullptr;
	if (TestTrue("health object", Entry->TryGetObjectField(TEXT("health"), Health)))
	{
		TestEqual("health.current", Number(*Health, TEXT("current")), 150.0);
		TestEqual("health.max", Number(*Health, TEXT("max")), 200.0);
	}
	TestTrue("is_dead is a bool for enemies", Entry->HasTypedField<EJson::Boolean>(TEXT("is_dead")) && !Entry->GetBoolField(TEXT("is_dead")));
	TestTrue("no AI controller -> ai_state null", IsNullField(Entry, TEXT("ai_state")));
	TestFalse("no mesh asset -> not visible", Entry->GetBoolField(TEXT("has_visible_mesh")));
	TestTrue("tags array", Entry->HasTypedField<EJson::Array>(TEXT("tags")));
	TestTrue("widgets array", Entry->HasTypedField<EJson::Array>(TEXT("widgets")));
	TestTrue("enemies are listed by default", MordecaiAgentBridge::State::IsInterestingActor(Enemy));

	Enemy->EnterDeathState();
	const TSharedRef<FJsonObject> Dead = MordecaiAgentBridge::State::MakeActorEntry(Enemy, &Origin);
	TestTrue("is_dead after death", Dead->GetBoolField(TEXT("is_dead")));
	bool bDeadTag = false;
	for (const TSharedPtr<FJsonValue>& Tag : Dead->GetArrayField(TEXT("tags")))
	{
		bDeadTag |= Tag->AsString() == TEXT("Mordecai.State.Dead");
	}
	TestTrue("dead tag reported", bDeadTag);

	TestEqual("illusion kind", MordecaiAgentBridge::State::ClassifyActorKind(NewObject<AMordecaiIllusionActor>()), FString(TEXT("illusion")));
	TestEqual("projectile kind", MordecaiAgentBridge::State::ClassifyActorKind(NewObject<AMordecaiProjectile>()), FString(TEXT("projectile")));
	TestEqual("native class of a native class", MordecaiAgentBridge::State::GetNativeClassName(AMordecaiEnemyCharacter::StaticClass()), FString(TEXT("MordecaiEnemyCharacter")));
	TestFalse("null actor has no health", MordecaiAgentBridge::State::GetActorHealth(nullptr, Current, Max));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_Playtest_AgentBridge_State_SectionsWithoutAGame,
	"Mordecai.Playtest.AgentBridge.State.SectionsWithoutAGame",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_Playtest_AgentBridge_State_SectionsWithoutAGame::RunTest(const FString& Parameters)
{
	const FMordecaiAgentBridgeStateContext Empty;

	const TSharedRef<FJsonObject> All = MordecaiAgentBridge::State::GatherState(Empty, {}, 1500.0, false);
	for (const FString& Section : MordecaiAgentBridge::GetStateSectionNames())
	{
		const TSharedPtr<FJsonObject>* Object = nullptr;
		TestTrue(FString::Printf(TEXT("'%s' present as an object"), *Section), All->TryGetObjectField(Section, Object));
		if (Object)
		{
			TestFalse(FString::Printf(TEXT("'%s' invalid without a game"), *Section), (*Object)->GetBoolField(TEXT("valid")));
			TestFalse(FString::Printf(TEXT("'%s' says why"), *Section), (*Object)->GetStringField(TEXT("reason")).IsEmpty());
		}
	}
	TestEqual("all nine sections", All->Values.Num(), 9);

	const TSharedRef<FJsonObject> Subset = MordecaiAgentBridge::State::GatherState(Empty, { TEXT("inventory"), TEXT("game") }, 1500.0, false);
	TestEqual("only the requested sections", Subset->Values.Num(), 2);
	TestTrue("inventory present", Subset->HasField(TEXT("inventory")));
	TestTrue("game present", Subset->HasField(TEXT("game")));

	const TSharedRef<FJsonObject> Invalid = MordecaiAgentBridge::State::MakeInvalidSection(TEXT("no pawn"));
	TestFalse("invalid section", Invalid->GetBoolField(TEXT("valid")));
	TestEqual("reason", Invalid->GetStringField(TEXT("reason")), FString(TEXT("no pawn")));

	TestFalse("not ready without a game", MordecaiAgentBridge::State::GetReadiness(Empty).IsReady());
	TestFalse("no viewport transform without a viewport", FMordecaiAgentBridgeViewportTransform::FromViewportClient(nullptr).bValid);
	TestFalse("null widget is not on screen", MordecaiAgentBridge::State::IsWidgetOnScreen(nullptr));
	return true;
}

// ===========================================================================
// Ids: a cooked game prints FNames in the first-registered case (§4)
// ===========================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_Playtest_AgentBridge_State_PauseMenuTabIdsAreLowercase,
	"Mordecai.Playtest.AgentBridge.State.PauseMenuTabIdsAreLowercase",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_Playtest_AgentBridge_State_PauseMenuTabIdsAreLowercase::RunTest(const FString& Parameters)
{
	using namespace MordecaiAgentBridgeStateTestHelpers;

	// The packaged game's tab id FName("inventory") prints "Inventory" because the UHT parameter
	// "Inventory" (BindToInventory) registered that spelling first; "character" likewise prints
	// "Character". This editor build preserves case, so the tabs are registered under the spellings
	// a cooked build hands back, and the bridge must still emit the canonical lowercase ids.
	const FName CookedInventory(TEXT("Inventory"));
	const FName CookedCharacter(TEXT("Character"));
	TestTrue("same FName as the tab id", CookedInventory == UMordecaiInventoryWidget::GetInventoryTabId());

	UMordecaiPauseMenuWidget* Menu = NewObject<UMordecaiPauseMenuWidget>(GetTransientPackage());
	Menu->RegisterTab(CookedCharacter, FText::FromString(TEXT("Character")), UUserWidget::StaticClass());
	Menu->RegisterTab(CookedInventory, FText::FromString(TEXT("Inventory")), UMordecaiInventoryWidget::StaticClass());
	Menu->SetActiveTab(CookedInventory);
	TestEqualSensitive("the widget reports the cooked spelling", Menu->GetActiveTabId().ToString(), FString(TEXT("Inventory")));

	const TSharedRef<FJsonObject> PauseMenu = MordecaiAgentBridge::State::GatherPauseMenuWidget(Menu, /*bOpen*/ true, FMordecaiAgentBridgeViewportTransform());
	TestTrue("valid", PauseMenu->GetBoolField(TEXT("valid")));
	TestTrue("open", PauseMenu->GetBoolField(TEXT("open")));
	TestEqualSensitive("active_tab is lowercase", StringAt(PauseMenu, TEXT("active_tab")), FString(TEXT("inventory")));
	const TArray<TSharedPtr<FJsonValue>> Tabs = ArrayAt(PauseMenu, TEXT("tabs"));
	if (TestEqual("two tabs", Tabs.Num(), 2))
	{
		TestEqualSensitive("tabs[0].id", StringAt(Tabs[0]->AsObject(), TEXT("id")), FString(TEXT("character")));
		TestEqualSensitive("tabs[0].name keeps its display case", StringAt(Tabs[0]->AsObject(), TEXT("name")), FString(TEXT("Character")));
		TestEqualSensitive("tabs[1].id", StringAt(Tabs[1]->AsObject(), TEXT("id")), FString(TEXT("inventory")));
	}

	const TSharedRef<FJsonObject> Closed = MordecaiAgentBridge::State::GatherPauseMenuWidget(nullptr, /*bOpen*/ false, FMordecaiAgentBridgeViewportTransform());
	TestFalse("closed", Closed->GetBoolField(TEXT("open")));
	TestTrue("no active tab while closed", IsNullField(Closed, TEXT("active_tab")));
	TestTrue("no inventory tab while closed", IsNullField(Closed, TEXT("inventory_tab")));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_Playtest_AgentBridge_State_TabButtonIdsAreCanonical,
	"Mordecai.Playtest.AgentBridge.State.TabButtonIdsAreCanonical",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_Playtest_AgentBridge_State_TabButtonIdsAreCanonical::RunTest(const FString& Parameters)
{
	using namespace MordecaiAgentBridgeStateTestHelpers;

	UMordecaiTabButton* TabButton = NewObject<UMordecaiTabButton>(GetTransientPackage());
	TabButton->InitButton(FName(TEXT("Inventory")));
	TestEqualSensitive("tab button id lowercase", StringAt(MordecaiAgentBridge::State::MakeButtonEntry(TabButton, FMordecaiAgentBridgeViewportTransform()), TEXT("id")), FString(TEXT("inventory")));

	// Identify buttons carry the instance GUID; whatever case the FName prints in, the id is the instance_id form
	const FGuid Guid = FGuid::NewGuid();
	UMordecaiTabButton* Identify = NewObject<UMordecaiTabButton>(GetTransientPackage());
	Identify->InitButton(FName(*Guid.ToString().ToLower()));
	const TSharedRef<FJsonObject> Entry = MordecaiAgentBridge::State::MakeButtonEntry(Identify, FMordecaiAgentBridgeViewportTransform());
	TestEqualSensitive("identify button id == instance_id", StringAt(Entry, TEXT("id")), Guid.ToString());
	TestTrue("no label child -> null label", IsNullField(Entry, TEXT("label")));
	TestTrue("never painted -> null center", IsNullField(Entry, TEXT("center")));
	TestTrue("enabled", Entry->GetBoolField(TEXT("enabled")));
	return true;
}

// ===========================================================================
// Player attributes come from the PlayerState ASC's UMordecaiAttributeSet (§4 notes)
// ===========================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_Playtest_AgentBridge_State_PlayerAttributesFromPlayerStateASC,
	"Mordecai.Playtest.AgentBridge.State.PlayerAttributesFromPlayerStateASC",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_Playtest_AgentBridge_State_PlayerAttributesFromPlayerStateASC::RunTest(const FString& Parameters)
{
	using namespace MordecaiAgentBridgeStateTestHelpers;
	FBridgeGameTestEnv Env;
	if (!TestTrue("test world with a game state and a local player controller", Env.Setup(/*bWithGameState*/ true)))
	{
		Env.Teardown();
		return true;
	}

	AMordecaiPlayerState* PlayerState = Env.World->SpawnActor<AMordecaiPlayerState>(AMordecaiPlayerState::StaticClass());
	APawn* Pawn = Env.World->SpawnActor<APawn>(APawn::StaticClass(), FVector(10.0, 20.0, 92.0), FRotator::ZeroRotator);
	if (!TestNotNull("player state spawned", PlayerState) || !TestNotNull("pawn spawned", Pawn))
	{
		Env.Teardown();
		return true;
	}
	Env.PC->PlayerState = PlayerState;

	UAbilitySystemComponent* ASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(PlayerState);
	const UMordecaiAttributeSet* MordecaiSet = ASC ? ASC->GetSet<UMordecaiAttributeSet>() : nullptr;
	const ULyraHealthSet* LyraSet = ASC ? ASC->GetSet<ULyraHealthSet>() : nullptr;
	if (!TestNotNull("PlayerState ASC", ASC) || !TestNotNull("the ASC registered UMordecaiAttributeSet", MordecaiSet) || !TestNotNull("and Lyra's health set", LyraSet))
	{
		Env.Teardown();
		return true;
	}

	ASC->SetNumericAttributeBase(UMordecaiAttributeSet::GetMaxHealthAttribute(), 150.f);
	ASC->SetNumericAttributeBase(UMordecaiAttributeSet::GetHealthAttribute(), 37.f);
	ASC->SetNumericAttributeBase(UMordecaiAttributeSet::GetSpellPointsAttribute(), 4.f);
	TestNotEqual("the Lyra health set reads something else", LyraSet->GetHealth(), 37.f);

	FMordecaiAgentBridgeStateContext Context;
	Context.World = Env.World;
	Context.PlayerController = Env.PC;
	Context.Pawn = Pawn;

	TSharedPtr<FJsonObject> Player = MordecaiAgentBridge::State::GatherSection(Context, TEXT("player"), 1500.0, false)->AsObject();
	TestTrue("player section valid", Player.IsValid() && Player->GetBoolField(TEXT("valid")));
	TSharedPtr<FJsonObject> Attributes = ObjectAt(Player, TEXT("attributes"));
	TestEqual("health from the Mordecai set", Number(Attributes, TEXT("health")), 37.0);
	TestEqual("max_health from the Mordecai set", Number(Attributes, TEXT("max_health")), 150.0);
	TestEqual("spell_points", Number(Attributes, TEXT("spell_points")), 4.0);

	// The section follows the set
	ASC->SetNumericAttributeBase(UMordecaiAttributeSet::GetHealthAttribute(), 12.f);
	Player = MordecaiAgentBridge::State::GatherSection(Context, TEXT("player"), 1500.0, false)->AsObject();
	TestEqual("health follows the set", Number(ObjectAt(Player, TEXT("attributes")), TEXT("health")), 12.0);

	// No PlayerState: attributes are invalid, never read from elsewhere
	Env.PC->PlayerState = nullptr;
	Player = MordecaiAgentBridge::State::GatherSection(Context, TEXT("player"), 1500.0, false)->AsObject();
	const TSharedPtr<FJsonObject> NoAttributes = ObjectAt(Player, TEXT("attributes"));
	TestTrue("no PlayerState -> attributes invalid", NoAttributes.IsValid() && !NoAttributes->GetBoolField(TEXT("valid")));

	Env.Teardown();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_Playtest_AgentBridge_State_EnemyHealthFromItsOwnASC,
	"Mordecai.Playtest.AgentBridge.State.EnemyHealthFromItsOwnASC",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_Playtest_AgentBridge_State_EnemyHealthFromItsOwnASC::RunTest(const FString& Parameters)
{
	using namespace MordecaiAgentBridgeStateTestHelpers;
	FBridgeGameTestEnv Env;
	if (!TestTrue("test world", Env.Setup(/*bWithGameState*/ false)))
	{
		Env.Teardown();
		return true;
	}

	// Spawned (so InitializeComponent lets the ASC register its attribute set), without an AI controller
	const FTransform SpawnTransform(FVector(300.0, 0.0, 100.0));
	AMordecaiEnemyCharacter* Enemy = Env.World->SpawnActorDeferred<AMordecaiEnemyCharacter>(AMordecaiEnemyCharacter::StaticClass(), SpawnTransform);
	if (!TestNotNull("enemy spawned", Enemy))
	{
		Env.Teardown();
		return true;
	}
	Enemy->AutoPossessAI = EAutoPossessAI::Disabled;
	Enemy->FinishSpawning(SpawnTransform);

	UAbilitySystemComponent* ASC = Enemy->GetAbilitySystemComponent();
	const UMordecaiAttributeSet* Set = ASC ? ASC->GetSet<UMordecaiAttributeSet>() : nullptr;
	if (!TestNotNull("the enemy's own ASC registered UMordecaiAttributeSet", Set))
	{
		Env.Teardown();
		return true;
	}
	ASC->SetNumericAttributeBase(UMordecaiAttributeSet::GetMaxHealthAttribute(), 200.f);
	ASC->SetNumericAttributeBase(UMordecaiAttributeSet::GetHealthAttribute(), 150.f);

	float Current = 0.f;
	float Max = 0.f;
	TestTrue("health found", MordecaiAgentBridge::State::GetActorHealth(Enemy, Current, Max));
	TestEqual("current from the ASC set", Current, 150.f);
	TestEqual("max from the ASC set", Max, 200.f);

	const FVector Origin(0.0, 0.0, 100.0);
	const TSharedRef<FJsonObject> Entry = MordecaiAgentBridge::State::MakeActorEntry(Enemy, &Origin);
	TestEqualSensitive("kind", StringAt(Entry, TEXT("kind")), FString(TEXT("enemy")));
	TestEqual("health.current", Number(ObjectAt(Entry, TEXT("health")), TEXT("current")), 150.0);
	TestEqual("health.max", Number(ObjectAt(Entry, TEXT("health")), TEXT("max")), 200.0);
	TestEqual("distance", Number(Entry, TEXT("distance")), 300.0);

	ASC->SetNumericAttributeBase(UMordecaiAttributeSet::GetHealthAttribute(), 25.f);
	TestEqual("health follows the ASC", Number(ObjectAt(MordecaiAgentBridge::State::MakeActorEntry(Enemy, &Origin), TEXT("health")), TEXT("current")), 25.0);

	Env.Teardown();
	return true;
}

// ===========================================================================
// pickups.prompt is read from the prompt widget's rendered text blocks (§4 notes)
// ===========================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_Playtest_AgentBridge_State_PromptReadsRenderedTextBlocks,
	"Mordecai.Playtest.AgentBridge.State.PromptReadsRenderedTextBlocks",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_Playtest_AgentBridge_State_PromptReadsRenderedTextBlocks::RunTest(const FString& Parameters)
{
	using namespace MordecaiAgentBridgeStateTestHelpers;
	FBridgeGameTestEnv Env;
	if (!TestTrue("test world with a local player", Env.Setup(/*bWithGameState*/ false)))
	{
		Env.Teardown();
		return true;
	}

	UMordecaiPickupPromptWidget* Prompt = Env.MakeUserWidget<UMordecaiPickupPromptWidget>();
	if (!TestNotNull("prompt widget", Prompt) || !TestNotNull("its tree was built", Prompt->WidgetTree ? Prompt->WidgetTree->FindWidget<UTextBlock>(TEXT("PromptText")) : nullptr))
	{
		Env.Teardown();
		return true;
	}

	AActor* Owner = Env.World->SpawnActor<AActor>(AActor::StaticClass(), FVector::ZeroVector, FRotator::ZeroRotator);
	UMordecaiPickupInteractionComponent* Interaction = NewObject<UMordecaiPickupInteractionComponent>(Owner);
	Interaction->RegisterComponent();
	AMordecaiItemPickup* Pickup = Env.World->SpawnActor<AMordecaiItemPickup>(AMordecaiItemPickup::StaticClass(), FVector(100.0, 0.0, 0.0), FRotator::ZeroRotator);
	Pickup->ItemDefinition = MakeDef(TEXT("TestItem_IronOre"), TEXT("Iron Ore"), EMordecaiItemType::Material);
	Prompt->BindToInteraction(Interaction);

	TSharedRef<FJsonObject> Section = MordecaiAgentBridge::State::GatherPromptWidget(Prompt);
	TestTrue("valid", Section->GetBoolField(TEXT("valid")));
	TestEqualSensitive("nothing focused -> empty prompt text", StringAt(Section, TEXT("text")), FString());
	TestEqualSensitive("hint text from its text block", StringAt(Section, TEXT("hint_text")), FString(TEXT("[I] Inventory")));

	Interaction->RegisterPickup(Pickup);
	Section = MordecaiAgentBridge::State::GatherPromptWidget(Prompt);
	TestEqualSensitive("focused -> rendered prompt", StringAt(Section, TEXT("text")), FString(TEXT("[F] Pick up Iron Ore")));
	// Headless there is no SWindow, so nothing is on screen; the real visibility is proven at Tier 4
	TestFalse("not on screen without a window", Section->GetBoolField(TEXT("visible")));
	TestFalse("hint not on screen without a window", Section->GetBoolField(TEXT("hint_visible")));

	// The text comes from the PromptText block, not from the widget's model
	Prompt->WidgetTree->FindWidget<UTextBlock>(TEXT("PromptText"))->SetText(FText::FromString(TEXT("drawn text")));
	TestEqual("the model still says the prompt", Prompt->GetPromptText().ToString(), FString(TEXT("[F] Pick up Iron Ore")));
	TestEqualSensitive("state reports what the text block draws", StringAt(MordecaiAgentBridge::State::GatherPromptWidget(Prompt), TEXT("text")), FString(TEXT("drawn text")));

	TestFalse("no widget -> invalid", MordecaiAgentBridge::State::GatherPromptWidget(nullptr)->GetBoolField(TEXT("valid")));
	UMordecaiPickupPromptWidget* Bare = NewObject<UMordecaiPickupPromptWidget>(GetTransientPackage());
	TestFalse("no tree -> invalid", MordecaiAgentBridge::State::GatherPromptWidget(Bare)->GetBoolField(TEXT("valid")));

	Prompt->Unbind();
	Env.Teardown();
	return true;
}

// ===========================================================================
// pause_menu.inventory_tab rows come from the rendered ItemListBox (§4 notes)
// ===========================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_Playtest_AgentBridge_State_InventoryTabRowsFromBoundWidget,
	"Mordecai.Playtest.AgentBridge.State.InventoryTabRowsFromBoundWidget",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_Playtest_AgentBridge_State_InventoryTabRowsFromBoundWidget::RunTest(const FString& Parameters)
{
	using namespace MordecaiAgentBridgeStateTestHelpers;
	FBridgeGameTestEnv Env;
	if (!TestTrue("test world with a local player", Env.Setup(/*bWithGameState*/ false)))
	{
		Env.Teardown();
		return true;
	}

	UMordecaiInventoryComponent* Inventory = NewObject<UMordecaiInventoryComponent>();
	UMordecaiResourceLedger* Ledger = NewObject<UMordecaiResourceLedger>();
	Inventory->SetResourceLedger(Ledger);
	UMordecaiItemDefinition* Sword = MakeDef(TEXT("TestItem_RustySword"), TEXT("Rusty Sword"), EMordecaiItemType::Weapon);
	UMordecaiItemDefinition* Amulet = MakeDef(TEXT("TestItem_MysteriousAmulet"), TEXT("Mysterious Amulet"), EMordecaiItemType::MagicalItem, EMordecaiItemRarity::Purple);
	Amulet->Subtype = TEXT("Amulet");
	Amulet->UsesIdentification = true;
	Amulet->DefaultIdentificationState = EMordecaiIdentificationState::Unidentified;
	UMordecaiItemDefinition* Potion = MakeDef(TEXT("TestItem_HealthPotion"), TEXT("Health Potion"), EMordecaiItemType::Consumable);
	UMordecaiItemDefinition* Ore = MakeDef(TEXT("TestItem_IronOre"), TEXT("Iron Ore"), EMordecaiItemType::Material);
	Ore->AutoStoreOnPickup = true;
	Ore->Stackable = true;
	Ore->MaxStackSize = 99;
	Inventory->PickupItem(Sword, 1);
	Inventory->PickupItem(Amulet, 1);
	Inventory->PickupItem(Potion, 1);
	Inventory->PickupItem(Ore, 3);

	UMordecaiInventoryWidget* Widget = Env.MakeUserWidget<UMordecaiInventoryWidget>();
	if (!TestNotNull("inventory widget", Widget) || !TestTrue("its tree was built", Widget->WidgetTree && Widget->WidgetTree->RootWidget))
	{
		Env.Teardown();
		return true;
	}
	Widget->BindToInventory(Inventory, Ledger);

	const TSharedPtr<FJsonObject> Items = MordecaiAgentBridge::State::GatherInventory(Inventory, Ledger);
	const TArray<TSharedPtr<FJsonValue>> ItemList = ArrayAt(Items, TEXT("items"));
	TSharedPtr<FJsonObject> Tab = MordecaiAgentBridge::State::GatherInventoryTab(Widget, FMordecaiAgentBridgeViewportTransform())->AsObject();
	if (!TestTrue("inventory_tab object", Tab.IsValid()))
	{
		Env.Teardown();
		return true;
	}
	TestFalse("not on screen without a window", Tab->GetBoolField(TEXT("visible")));
	const TArray<TSharedPtr<FJsonValue>> Rows = ArrayAt(Tab, TEXT("rows"));
	if (TestEqual("three rendered rows", Rows.Num(), 3) && TestEqual("three items", ItemList.Num(), 3))
	{
		const TSharedPtr<FJsonObject> Row0 = Rows[0]->AsObject();
		const TSharedPtr<FJsonObject> Row1 = Rows[1]->AsObject();
		const TSharedPtr<FJsonObject> Row2 = Rows[2]->AsObject();
		TestEqualSensitive("row 0 text as drawn", StringAt(Row0, TEXT("text")), FString(TEXT("Rusty Sword")));
		TestEqualSensitive("row 1 text as drawn (badge + unidentified name)", StringAt(Row1, TEXT("text")), FString(TEXT("? Unidentified Purple Amulet")));
		TestEqualSensitive("row 2 text", StringAt(Row2, TEXT("text")), FString(TEXT("Health Potion")));
		TestFalse("row 0 identified", Row0->GetBoolField(TEXT("unidentified")));
		TestTrue("row 1 unidentified", Row1->GetBoolField(TEXT("unidentified")));
		TestTrue("identified rows have no button", IsNullField(Row0, TEXT("identify_button")));
		for (int32 Index = 0; Index < 3; ++Index)
		{
			TestEqualSensitive(*FString::Printf(TEXT("row %d instance_id == inventory.items[%d].instance_id"), Index, Index),
				StringAt(Rows[Index]->AsObject(), TEXT("instance_id")), StringAt(ItemList[Index]->AsObject(), TEXT("instance_id")));
		}
		const TSharedPtr<FJsonObject> Button = ObjectAt(Row1, TEXT("identify_button"));
		TestEqualSensitive("Identify label", StringAt(Button, TEXT("label")), FString(TEXT("Identify")));
		TestTrue("Identify enabled", Button.IsValid() && Button->GetBoolField(TEXT("enabled")));
		TestTrue("center null before the first paint", IsNullField(Button, TEXT("center")));
	}
	const TArray<TSharedPtr<FJsonValue>> LedgerRows = ArrayAt(Tab, TEXT("ledger_rows"));
	TestTrue("ledger row as drawn", LedgerRows.Num() == 1 && LedgerRows[0]->AsString() == TEXT("Iron Ore  x3"));

	// Identifying rebuilds the rows: the amulet row reads its real name and loses its button
	if (ItemList.Num() == 3)
	{
		const FString AmuletId = StringAt(ItemList[1]->AsObject(), TEXT("instance_id"));
		FGuid AmuletGuid;
		FGuid::Parse(AmuletId, AmuletGuid);
		Inventory->SetInstanceIdentificationState(AmuletGuid, EMordecaiIdentificationState::Identified);
		Tab = MordecaiAgentBridge::State::GatherInventoryTab(Widget, FMordecaiAgentBridgeViewportTransform())->AsObject();
		const TArray<TSharedPtr<FJsonValue>> After = ArrayAt(Tab, TEXT("rows"));
		if (TestEqual("still three rows", After.Num(), 3))
		{
			TestEqualSensitive("identified name drawn", StringAt(After[1]->AsObject(), TEXT("text")), FString(TEXT("Mysterious Amulet")));
			TestFalse("identified", After[1]->AsObject()->GetBoolField(TEXT("unidentified")));
			TestTrue("no Identify button", IsNullField(After[1]->AsObject(), TEXT("identify_button")));
			TestEqualSensitive("instance_id unchanged", StringAt(After[1]->AsObject(), TEXT("instance_id")), AmuletId);
		}
	}

	TestTrue("no widget -> null", MordecaiAgentBridge::State::GatherInventoryTab(nullptr, FMordecaiAgentBridgeViewportTransform())->IsNull());
	Widget->Unbind();
	Env.Teardown();
	return true;
}

// ===========================================================================
// Screen coordinates: painted geometry -> viewport pixels at DPI scale 1.5 (§4 units)
// ===========================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_Playtest_AgentBridge_State_GeometryToViewportPixelsAtDpi150,
	"Mordecai.Playtest.AgentBridge.State.GeometryToViewportPixelsAtDpi150",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_Playtest_AgentBridge_State_GeometryToViewportPixelsAtDpi150::RunTest(const FString& Parameters)
{
	// The playtest window: a 1280x720 viewport drawn into a 1920x1080 client area at desktop (320,224)
	FMordecaiAgentBridgeViewportTransform Transform;
	Transform.bValid = true;
	Transform.AbsolutePosition = FVector2D(320.0, 224.0);
	Transform.AbsoluteSize = FVector2D(1920.0, 1080.0);
	Transform.ViewportSize = FIntPoint(1280, 720);

	// A 120x40 (local units) button laid out at scale 1.5 so its centre is viewport pixel (812, 402)
	const FVector2D LocalSize(120.0, 40.0);
	const FVector2D AbsoluteTopLeft = Transform.ToAbsolute(FVector2D(812.0 - 60.0, 402.0 - 20.0));
	const FGeometry Geometry = FGeometry::MakeRoot(LocalSize, FSlateLayoutTransform(1.5f, AbsoluteTopLeft));
	TestTrue("absolute size is scaled", FVector2D(Geometry.GetAbsoluteSize()).Equals(FVector2D(180.0, 60.0), 1e-3));

	const TSharedRef<FJsonValue> Center = MordecaiAgentBridge::State::GeometryCenter(Geometry, Transform);
	const TArray<TSharedPtr<FJsonValue>>* CenterArray = nullptr;
	if (TestTrue("center is [x,y]", Center->TryGetArray(CenterArray) && CenterArray->Num() == 2))
	{
		TestEqual("center x in viewport pixels", (*CenterArray)[0]->AsNumber(), 812.0, 0.05);
		TestEqual("center y in viewport pixels", (*CenterArray)[1]->AsNumber(), 402.0, 0.05);
	}
	const TSharedRef<FJsonValue> Rect = MordecaiAgentBridge::State::GeometryRect(Geometry, Transform);
	const TArray<TSharedPtr<FJsonValue>>* RectArray = nullptr;
	if (TestTrue("rect is [x,y,w,h]", Rect->TryGetArray(RectArray) && RectArray->Num() == 4))
	{
		TestEqual("rect x", (*RectArray)[0]->AsNumber(), 752.0, 0.05);
		TestEqual("rect y", (*RectArray)[1]->AsNumber(), 382.0, 0.05);
		TestEqual("rect w in viewport pixels (local size)", (*RectArray)[2]->AsNumber(), 120.0, 0.05);
		TestEqual("rect h", (*RectArray)[3]->AsNumber(), 40.0, 0.05);
	}

	// Round trip with the click path: the reported centre maps back to the absolute centre
	const FVector2D AbsoluteCenter = FVector2D(Geometry.GetAbsolutePosition()) + FVector2D(Geometry.GetAbsoluteSize()) * 0.5;
	TestTrue("click at the reported centre lands on the absolute centre",
		Transform.ToAbsolute(FVector2D(812.0, 402.0)).Equals(AbsoluteCenter, 1e-3));

	TestTrue("unpainted (zero size) -> null", MordecaiAgentBridge::State::GeometryCenter(FGeometry(), Transform)->IsNull());
	TestTrue("no viewport transform -> null", MordecaiAgentBridge::State::GeometryCenter(Geometry, FMordecaiAgentBridgeViewportTransform())->IsNull());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_Playtest_AgentBridge_State_HeldInputsReportHolders,
	"Mordecai.Playtest.AgentBridge.State.HeldInputsReportHolders",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_Playtest_AgentBridge_State_HeldInputsReportHolders::RunTest(const FString& Parameters)
{
	using namespace MordecaiAgentBridgeStateTestHelpers;
	FMordecaiAgentBridgeHeldInput Held;
	Held.Key = TEXT("W");
	Held.Kind = TEXT("key");
	Held.Value = 1.0;
	Held.Holders = 2;
	const TSharedRef<FJsonObject> Entry = MordecaiAgentBridge::State::MakeHeldInputEntry(Held);
	TestEqualSensitive("key", StringAt(Entry, TEXT("key")), FString(TEXT("W")));
	TestEqualSensitive("kind", StringAt(Entry, TEXT("kind")), FString(TEXT("key")));
	TestEqual("value", Number(Entry, TEXT("value")), 1.0);
	TestEqual("holders", Number(Entry, TEXT("holders")), 2.0);
	return true;
}
