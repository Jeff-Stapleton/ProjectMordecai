// Project Mordecai — Pickup Prompt Widget Tests (US-079)
// NullRHI-compatible headless tests for UMordecaiPickupPromptWidget: binding
// lifecycle, prompt composition ("[F] Pick up <name>"), identification-aware
// names, hidden-when-unfocused, and the persistent inventory assist hint.

#include "Misc/AutomationTest.h"
#include "Engine/World.h"
#include "Engine/Engine.h"

#include "Mordecai/Items/MordecaiItemDefinition.h"
#include "Mordecai/Items/MordecaiItemPickup.h"
#include "Mordecai/Items/MordecaiItemTypes.h"
#include "Mordecai/Items/MordecaiPickupInteractionComponent.h"
#include "Mordecai/UI/MordecaiPickupPromptWidget.h"

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

namespace MordecaiPickupPromptTestHelpers
{
	static UMordecaiItemDefinition* MakeDef(FName ItemId, const TCHAR* DisplayName)
	{
		UMordecaiItemDefinition* Def = NewObject<UMordecaiItemDefinition>();
		Def->ItemId = ItemId;
		Def->ItemType = EMordecaiItemType::Consumable;
		Def->DisplayName = FText::FromString(DisplayName);
		return Def;
	}

	struct FPromptTestEnv
	{
		UWorld* World = nullptr;
		AActor* Owner = nullptr;
		UMordecaiPickupInteractionComponent* Interaction = nullptr;

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
			return true;
		}

		AMordecaiItemPickup* SpawnPickup(UMordecaiItemDefinition* Def, FVector Location = FVector(100, 0, 0))
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
		}
	};

	static UMordecaiPickupPromptWidget* MakeWidget()
	{
		return NewObject<UMordecaiPickupPromptWidget>(GetTransientPackage());
	}
}


// ===========================================================================
// 1. Mordecai.UI.PickupPrompt.BindsAndUnbinds (AC-079.18)
// ===========================================================================
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_UI_PickupPrompt_BindsAndUnbinds,
	"Mordecai.UI.PickupPrompt.BindsAndUnbinds",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_UI_PickupPrompt_BindsAndUnbinds::RunTest(const FString& Parameters)
{
	using namespace MordecaiPickupPromptTestHelpers;
	FPromptTestEnv Env;
	if (!Env.Setup()) { AddError(TEXT("Failed to set up test world")); return false; }

	UMordecaiPickupPromptWidget* Widget = MakeWidget();
	Widget->BindToInteraction(Env.Interaction);
	TestTrue("Bound to interaction component", Widget->IsBoundToInteraction());

	// Focus change while bound propagates
	AMordecaiItemPickup* Pickup = Env.SpawnPickup(MakeDef(FName("Ore"), TEXT("Iron Ore")));
	Env.Interaction->RegisterPickup(Pickup);
	TestTrue("Prompt visible after focus gained", Widget->IsPromptVisible());

	// Unbind — further focus changes do NOT propagate
	Widget->Unbind();
	TestFalse("Not bound after Unbind", Widget->IsBoundToInteraction());
	Env.Interaction->UnregisterPickup(Pickup);
	TestTrue("Prompt state frozen after unbind", Widget->IsPromptVisible());

	Env.Teardown();
	return true;
}

// ===========================================================================
// 2. Mordecai.UI.PickupPrompt.PromptTextIncludesKeyAndName (AC-079.18)
// ===========================================================================
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_UI_PickupPrompt_PromptTextIncludesKeyAndName,
	"Mordecai.UI.PickupPrompt.PromptTextIncludesKeyAndName",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_UI_PickupPrompt_PromptTextIncludesKeyAndName::RunTest(const FString& Parameters)
{
	using namespace MordecaiPickupPromptTestHelpers;
	FPromptTestEnv Env;
	if (!Env.Setup()) { AddError(TEXT("Failed to set up test world")); return false; }

	UMordecaiPickupPromptWidget* Widget = MakeWidget();
	Widget->BindToInteraction(Env.Interaction);

	AMordecaiItemPickup* Pickup = Env.SpawnPickup(MakeDef(FName("Ore"), TEXT("Iron Ore")));
	Env.Interaction->RegisterPickup(Pickup);

	const FString Prompt = Widget->GetPromptText().ToString();
	TestEqual("Prompt composes key + verb + item name", Prompt, FString(TEXT("[F] Pick up Iron Ore")));
	TestTrue("Prompt visible", Widget->IsPromptVisible());

	Env.Teardown();
	return true;
}

// ===========================================================================
// 3. Mordecai.UI.PickupPrompt.UnidentifiedShowsPartialName (AC-079.18)
// ===========================================================================
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_UI_PickupPrompt_UnidentifiedShowsPartialName,
	"Mordecai.UI.PickupPrompt.UnidentifiedShowsPartialName",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_UI_PickupPrompt_UnidentifiedShowsPartialName::RunTest(const FString& Parameters)
{
	using namespace MordecaiPickupPromptTestHelpers;
	FPromptTestEnv Env;
	if (!Env.Setup()) { AddError(TEXT("Failed to set up test world")); return false; }

	UMordecaiItemDefinition* Amulet = NewObject<UMordecaiItemDefinition>();
	Amulet->ItemId = FName("MysteriousAmulet");
	Amulet->ItemType = EMordecaiItemType::MagicalItem;
	Amulet->Rarity = EMordecaiItemRarity::Purple;
	Amulet->Subtype = FName("Amulet");
	Amulet->DisplayName = FText::FromString(TEXT("Mysterious Amulet"));
	Amulet->UsesIdentification = true;
	Amulet->DefaultIdentificationState = EMordecaiIdentificationState::Unidentified;

	AMordecaiItemPickup* Pickup = Env.SpawnPickup(Amulet);

	// The pickup's own display name must be identification-aware
	const FString PickupName = Pickup->GetItemDisplayName().ToString();
	TestFalse("Unidentified pickup hides its true name", PickupName.Contains(TEXT("Mysterious Amulet")));
	TestTrue("Unidentified pickup shows partial info", PickupName.Contains(TEXT("Unidentified")));

	// ...and the prompt uses that partial name
	UMordecaiPickupPromptWidget* Widget = MakeWidget();
	Widget->BindToInteraction(Env.Interaction);
	Env.Interaction->RegisterPickup(Pickup);

	const FString Prompt = Widget->GetPromptText().ToString();
	TestTrue("Prompt uses partial name", Prompt.Contains(TEXT("Unidentified")));
	TestFalse("Prompt does not leak identified name", Prompt.Contains(TEXT("Mysterious Amulet")));

	Env.Teardown();
	return true;
}

// ===========================================================================
// 4. Mordecai.UI.PickupPrompt.HiddenWhenNoFocus (AC-079.18)
// ===========================================================================
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_UI_PickupPrompt_HiddenWhenNoFocus,
	"Mordecai.UI.PickupPrompt.HiddenWhenNoFocus",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_UI_PickupPrompt_HiddenWhenNoFocus::RunTest(const FString& Parameters)
{
	using namespace MordecaiPickupPromptTestHelpers;
	FPromptTestEnv Env;
	if (!Env.Setup()) { AddError(TEXT("Failed to set up test world")); return false; }

	// Unbound widget: hidden prompt, but the assist hint is always available
	UMordecaiPickupPromptWidget* Widget = MakeWidget();
	TestFalse("Prompt hidden while unbound", Widget->IsPromptVisible());
	TestTrue("Prompt text empty while unbound", Widget->GetPromptText().IsEmpty());
	TestEqual("Inventory assist hint always present",
		UMordecaiPickupPromptWidget::GetInventoryHintText().ToString(), FString(TEXT("[I] Inventory")));

	// Bound but nothing focused: still hidden
	Widget->BindToInteraction(Env.Interaction);
	TestFalse("Prompt hidden with no focus", Widget->IsPromptVisible());

	// Focus gained then lost: prompt hides again
	AMordecaiItemPickup* Pickup = Env.SpawnPickup(MakeDef(FName("Ore"), TEXT("Iron Ore")));
	Env.Interaction->RegisterPickup(Pickup);
	TestTrue("Prompt visible while focused", Widget->IsPromptVisible());
	Env.Interaction->UnregisterPickup(Pickup);
	TestFalse("Prompt hidden after focus lost", Widget->IsPromptVisible());
	TestTrue("Prompt text cleared after focus lost", Widget->GetPromptText().IsEmpty());

	Env.Teardown();
	return true;
}
