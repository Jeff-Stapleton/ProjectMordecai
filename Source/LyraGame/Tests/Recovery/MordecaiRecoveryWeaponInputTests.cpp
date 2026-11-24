// Project Mordecai — previous weapon input recovery (US-096, issue #14)

#include "Misc/AutomationTest.h"

#include "EnhancedInputComponent.h"
#include "EnhancedPlayerInput.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "InputAction.h"
#include "InputMappingContext.h"
#include "InputTriggers.h"
#include "Mordecai/MordecaiCharacter.h"
#include "Mordecai/MordecaiHeroComponent.h"
#include "Mordecai/Input/MordecaiInputTriggers.h"
#include "Mordecai/Weapons/MordecaiEquipmentComponent.h"
#include "Mordecai/Weapons/MordecaiWeaponProfileFactory.h"
#include "UObject/SoftObjectPath.h"

namespace MordecaiRecoveryWeaponInputTest
{
	static UInputAction* LoadAction(const TCHAR* Name)
	{
		return Cast<UInputAction>(FSoftObjectPath(FString::Printf(
			TEXT("/MordecaiCore/Input/Actions/%s.%s"), Name, Name)).TryLoad());
	}

	template <typename TTrigger>
	static const TTrigger* FindTrigger(const FEnhancedActionKeyMapping& Mapping)
	{
		for (const TObjectPtr<UInputTrigger>& Trigger : Mapping.Triggers)
		{
			if (const TTrigger* Typed = ExactCast<TTrigger>(Trigger.Get()))
			{
				return Typed;
			}
		}
		return nullptr;
	}

	static int32 FindMapping(
		const UInputMappingContext* Context,
		const UInputAction* Action,
		const FKey& Key)
	{
		const TArray<FEnhancedActionKeyMapping>& Mappings = Context->GetMappings();
		for (int32 Index = 0; Index < Mappings.Num(); ++Index)
		{
			if (Mappings[Index].Action == Action && Mappings[Index].Key == Key)
			{
				return Index;
			}
		}
		return INDEX_NONE;
	}

	static int32 FindShiftTab(const UInputMappingContext* Context)
	{
		const TArray<FEnhancedActionKeyMapping>& Mappings = Context->GetMappings();
		for (int32 Index = 0; Index < Mappings.Num(); ++Index)
		{
			if (Mappings[Index].Key == EKeys::Tab
				&& FindTrigger<UMordecaiInputTriggerShiftPressed>(Mappings[Index]))
			{
				return Index;
			}
		}
		return INDEX_NONE;
	}

	static bool ExecuteBinding(
		const UEnhancedInputComponent* EIC,
		const UInputAction* Action,
		const ETriggerEvent Event,
		const UObject* BoundTo)
	{
		for (const TUniquePtr<FEnhancedInputActionEventBinding>& Binding :
			EIC->GetActionEventBindings())
		{
			if (Binding && Binding->GetAction() == Action
				&& Binding->GetTriggerEvent() == Event
				&& Binding->IsBoundToObject(BoundTo))
			{
				Binding->Execute(FInputActionInstance(Action));
				return true;
			}
		}
		return false;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FMordecai_Recovery_Weapons_PreviousWeaponInput,
	"Mordecai.Recovery.Weapons.PreviousWeaponInput",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_Recovery_Weapons_PreviousWeaponInput::RunTest(
	const FString& Parameters)
{
	using namespace MordecaiRecoveryWeaponInputTest;

	AMordecaiCharacter* Character = NewObject<AMordecaiCharacter>();
	UMordecaiHeroComponent* Hero =
		Character->FindComponentByClass<UMordecaiHeroComponent>();
	UMordecaiEquipmentComponent* Equipment = Character->GetEquipmentComponent();
	if (!TestNotNull(TEXT("Hero component"), Hero)
		|| !TestNotNull(TEXT("Equipment component"), Equipment))
	{
		return true;
	}

	UEnhancedInputComponent* EIC = NewObject<UEnhancedInputComponent>(Character);
	const UInputMappingContext* Context = Hero->BuildAndBindRuntimeInput(EIC);
	const UInputAction* NextAction = LoadAction(TEXT("IA_Mordecai_WeaponSwap"));
	const UInputAction* BackslashAction =
		LoadAction(TEXT("IA_Mordecai_WeaponCyclePrev"));
	const UInputAction* SprintAction = LoadAction(TEXT("IA_Mordecai_Sprint"));
	if (!TestNotNull(TEXT("Runtime input context"), Context)
		|| !TestNotNull(TEXT("Next weapon action"), NextAction)
		|| !TestNotNull(TEXT("Previous weapon action"), BackslashAction)
		|| !TestNotNull(TEXT("Sprint action"), SprintAction))
	{
		return true;
	}

	Equipment->AddAvailableWeapon(
		UMordecaiWeaponProfileFactory::CreateLongsword(Character));
	Equipment->AddAvailableWeapon(
		UMordecaiWeaponProfileFactory::CreateDagger(Character));
	Equipment->AddAvailableWeapon(
		UMordecaiWeaponProfileFactory::CreateSpear(Character));
	Equipment->CycleNextWeapon();
	TestEqual(TEXT("Starts on weapon zero"),
		Equipment->GetCurrentWeaponIndex(), 0);

	const int32 PlainTabIndex = FindMapping(Context, NextAction, EKeys::Tab);
	if (!TestTrue(TEXT("Tab alone maps next weapon"), PlainTabIndex != INDEX_NONE))
	{
		return true;
	}
	const TArray<FEnhancedActionKeyMapping>& Mappings = Context->GetMappings();
	TestNotNull(TEXT("Plain Tab is blocked while Left Shift is held"),
		FindTrigger<UMordecaiInputTriggerShiftBlocker>(Mappings[PlainTabIndex]));
	TestTrue(TEXT("Tab binding executes"),
		ExecuteBinding(EIC, NextAction, ETriggerEvent::Started, Hero));
	TestEqual(TEXT("Tab alone selects the next weapon"),
		Equipment->GetCurrentWeaponIndex(), 1);

	const int32 ShiftTabIndex = FindShiftTab(Context);
	if (!TestTrue(TEXT("Shift+Tab has a Shift-gated Tab mapping"),
		ShiftTabIndex != INDEX_NONE))
	{
		return true;
	}

	const UInputAction* ShiftTabAction = Mappings[ShiftTabIndex].Action;
	const int32 SprintShiftIndex =
		FindMapping(Context, SprintAction, EKeys::LeftShift);
	TestTrue(TEXT("Sprint action still maps Left Shift"), SprintShiftIndex != INDEX_NONE);
	TestNotNull(TEXT("Shift+Tab requires Left Shift and a fresh Tab press"),
		FindTrigger<UMordecaiInputTriggerShiftPressed>(Mappings[ShiftTabIndex]));
	TestTrue(TEXT("Shift+Tab mapping precedes plain Tab"),
		ShiftTabIndex < PlainTabIndex);
	TestTrue(TEXT("BackSlash remains an extra previous-weapon binding"),
		FindMapping(Context, BackslashAction, EKeys::Backslash) != INDEX_NONE);

	TestFalse(TEXT("Sprint begins off"), Character->bIsSprinting);
	TestTrue(TEXT("Left Shift sprint binding executes immediately"),
		ExecuteBinding(EIC, SprintAction, ETriggerEvent::Started, Hero));
	TestTrue(TEXT("Left Shift still toggles sprint without release delay"),
		Character->bIsSprinting);
	TestTrue(TEXT("Shift+Tab binding executes"),
		ExecuteBinding(EIC, ShiftTabAction, ETriggerEvent::Triggered, Hero));
	TestEqual(TEXT("Shift+Tab selects the previous weapon"),
		Equipment->GetCurrentWeaponIndex(), 0);
	TestFalse(TEXT("Shift+Tab restores sprint-off state"),
		Character->bIsSprinting);

	Character->SetSprinting(true);
	ExecuteBinding(EIC, NextAction, ETriggerEvent::Started, Hero);
	TestTrue(TEXT("Precondition: sprint begins on"), Character->bIsSprinting);
	ExecuteBinding(EIC, SprintAction, ETriggerEvent::Started, Hero);
	TestFalse(TEXT("Shift press temporarily toggles sprint off"),
		Character->bIsSprinting);
	ExecuteBinding(EIC, ShiftTabAction, ETriggerEvent::Triggered, Hero);
	TestEqual(TEXT("Second Shift+Tab also selects the previous weapon"),
		Equipment->GetCurrentWeaponIndex(), 0);
	TestTrue(TEXT("Shift+Tab restores sprint-on state"),
		Character->bIsSprinting);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FMordecai_Recovery_Weapons_ShiftTabSameFrameOrdering,
	"Mordecai.Recovery.Weapons.ShiftTabSameFrameOrdering",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_Recovery_Weapons_ShiftTabSameFrameOrdering::RunTest(
	const FString& Parameters)
{
	using namespace MordecaiRecoveryWeaponInputTest;

	auto RunOrder = [this](const bool bChordBindingFirst, const bool bStartsSprinting)
	{
		AMordecaiCharacter* Character = NewObject<AMordecaiCharacter>();
		UMordecaiHeroComponent* Hero =
			Character->FindComponentByClass<UMordecaiHeroComponent>();
		UMordecaiEquipmentComponent* Equipment = Character->GetEquipmentComponent();
		UEnhancedInputComponent* EIC = NewObject<UEnhancedInputComponent>(Character);
		const UInputMappingContext* Context = Hero->BuildAndBindRuntimeInput(EIC);
		const UInputAction* SprintAction = LoadAction(TEXT("IA_Mordecai_Sprint"));
		if (!Hero || !Equipment || !Context || !SprintAction)
		{
			AddError(TEXT("Same-frame fixture did not initialize"));
			return;
		}

		Equipment->AddAvailableWeapon(
			UMordecaiWeaponProfileFactory::CreateLongsword(Character));
		Equipment->AddAvailableWeapon(
			UMordecaiWeaponProfileFactory::CreateDagger(Character));
		Equipment->AddAvailableWeapon(
			UMordecaiWeaponProfileFactory::CreateSpear(Character));
		Equipment->CycleNextWeapon();
		Equipment->CycleNextWeapon();
		Character->SetSprinting(bStartsSprinting);

		const int32 ShiftTabIndex = FindShiftTab(Context);
		if (ShiftTabIndex == INDEX_NONE)
		{
			AddError(TEXT("Shift+Tab mapping is missing"));
			return;
		}
		const UInputAction* ShiftTabAction = Context->GetMappings()[ShiftTabIndex].Action;
		const bool bFirstExecuted = bChordBindingFirst
			? ExecuteBinding(EIC, ShiftTabAction, ETriggerEvent::Triggered, Hero)
			: ExecuteBinding(EIC, SprintAction, ETriggerEvent::Started, Hero);
		const bool bSecondExecuted = bChordBindingFirst
			? ExecuteBinding(EIC, SprintAction, ETriggerEvent::Started, Hero)
			: ExecuteBinding(EIC, ShiftTabAction, ETriggerEvent::Triggered, Hero);

		const FString Order = bChordBindingFirst
			? TEXT("chord-before-sprint")
			: TEXT("sprint-before-chord");
		TestTrue(*FString::Printf(TEXT("%s: both same-frame bindings execute"), *Order),
			bFirstExecuted && bSecondExecuted);
		TestEqual(*FString::Printf(TEXT("%s: exactly one previous cycle"), *Order),
			Equipment->GetCurrentWeaponIndex(), 0);
		TestEqual(*FString::Printf(TEXT("%s: sprint state is unchanged"), *Order),
			Character->bIsSprinting, bStartsSprinting);
	};

	for (const bool bStartsSprinting : {false, true})
	{
		RunOrder(false, bStartsSprinting);
		RunOrder(true, bStartsSprinting);
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FMordecai_Recovery_Weapons_TabRequiresFreshPressThroughShiftTransitions,
	"Mordecai.Recovery.Weapons.TabRequiresFreshPressThroughShiftTransitions",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_Recovery_Weapons_TabRequiresFreshPressThroughShiftTransitions::RunTest(
	const FString& Parameters)
{
	using namespace MordecaiRecoveryWeaponInputTest;

	AMordecaiCharacter* Character = NewObject<AMordecaiCharacter>();
	UMordecaiHeroComponent* Hero =
		Character->FindComponentByClass<UMordecaiHeroComponent>();
	UEnhancedInputComponent* EIC = NewObject<UEnhancedInputComponent>(Character);
	const UInputMappingContext* Context = Hero->BuildAndBindRuntimeInput(EIC);
	const UInputAction* NextAction = LoadAction(TEXT("IA_Mordecai_WeaponSwap"));
	if (!Hero || !Context || !NextAction)
	{
		AddError(TEXT("Fresh-Tab fixture did not initialize"));
		return false;
	}

	const int32 TabIndex = FindMapping(Context, NextAction, EKeys::Tab);
	if (TabIndex == INDEX_NONE)
	{
		AddError(TEXT("Plain Tab next-weapon mapping is missing"));
		return false;
	}
	const FEnhancedActionKeyMapping& TabMapping = Context->GetMappings()[TabIndex];
	UInputTriggerPressed* FreshPress = nullptr;
	UMordecaiInputTriggerShiftBlocker* ShiftBlocker = nullptr;
	for (const TObjectPtr<UInputTrigger>& Trigger : TabMapping.Triggers)
	{
		FreshPress = FreshPress
			? FreshPress
			: ExactCast<UInputTriggerPressed>(Trigger.Get());
		ShiftBlocker = ShiftBlocker
			? ShiftBlocker
			: ExactCast<UMordecaiInputTriggerShiftBlocker>(Trigger.Get());
	}
	TestNotNull(TEXT("Plain Tab has a fresh-press trigger"), FreshPress);
	TestNotNull(TEXT("Plain Tab has a Shift blocker"), ShiftBlocker);
	if (!ShiftBlocker)
	{
		return false;
	}

	// Keep exercising the key-state behavior in RED even before the mapping owns
	// its Pressed trigger, so this test proves more than trigger class presence.
	if (!FreshPress)
	{
		FreshPress = NewObject<UInputTriggerPressed>();
	}

	UWorld* InputWorld = UWorld::CreateWorld(EWorldType::Game, false);
	if (!GEngine || !InputWorld)
	{
		AddError(TEXT("Could not create the key-injection world"));
		return false;
	}
	FWorldContext& WorldContext =
		GEngine->CreateNewWorldContext(EWorldType::Game);
	WorldContext.SetCurrentWorld(InputWorld);
	InputWorld->InitializeActorsForPlay(FURL());
	APlayerController* PlayerController =
		InputWorld->SpawnActor<APlayerController>();
	if (!PlayerController)
	{
		GEngine->DestroyWorldContext(InputWorld);
		InputWorld->DestroyWorld(false);
		AddError(TEXT("Could not create the key-injection player controller"));
		return false;
	}
	UEnhancedPlayerInput* PlayerInput =
		NewObject<UEnhancedPlayerInput>(PlayerController);
	TArray<UInputComponent*> EmptyStack;
	auto InputFrame = [PlayerInput, &EmptyStack](
		const FKey& Key, const EInputEvent Event)
	{
		const float Value = Event == IE_Released ? 0.0f : 1.0f;
		PlayerInput->InputKey(
			FInputKeyEventArgs::CreateSimulated(Key, Event, Value));
		PlayerInput->ProcessInputStack(EmptyStack, 1.0f / 60.0f, false);
	};
	auto MappingTriggers = [PlayerInput, FreshPress, ShiftBlocker]()
	{
		const FInputActionValue TabValue(PlayerInput->IsPressed(EKeys::Tab));
		const ETriggerState FreshState =
			FreshPress->UpdateState(PlayerInput, TabValue, 1.0f / 60.0f);
		const ETriggerState BlockerState =
			ShiftBlocker->UpdateState(PlayerInput, TabValue, 1.0f / 60.0f);
		// Enhanced Player Input normally performs this bookkeeping after it
		// evaluates each mapping trigger.
		FreshPress->LastValue = TabValue;
		return FreshState == ETriggerState::Triggered
			&& BlockerState != ETriggerState::Triggered;
	};

	// A fresh unmodified Tab press fires once. Pressing and releasing Shift
	// while Tab remains held must neither re-fire Next nor treat release as a
	// new Tab press.
	InputFrame(EKeys::Tab, IE_Pressed);
	TestTrue(TEXT("Fresh Tab key state reaches the next-weapon mapping"),
		PlayerInput->WasJustPressed(EKeys::Tab) && MappingTriggers());
	InputFrame(EKeys::LeftShift, IE_Pressed);
	TestFalse(TEXT("Left Shift blocks held Tab"), MappingTriggers());
	InputFrame(EKeys::LeftShift, IE_Released);
	TestFalse(TEXT("Left Shift release does not re-fire held Tab"), MappingTriggers());
	InputFrame(EKeys::Tab, IE_Released);
	TestFalse(TEXT("Tab release does not fire next weapon"), MappingTriggers());

	// Shift-first then Tab exercises the actual blocker. Releasing Shift while
	// Tab remains down must leave the fresh-press trigger non-triggered.
	InputFrame(EKeys::LeftShift, IE_Pressed);
	TestFalse(TEXT("Left Shift alone does not fire Tab"), MappingTriggers());
	InputFrame(EKeys::Tab, IE_Pressed);
	TestFalse(TEXT("Left Shift blocks a fresh Tab press"), MappingTriggers());
	InputFrame(EKeys::LeftShift, IE_Released);
	TestFalse(TEXT("Releasing Left Shift first does not re-fire Tab"),
		MappingTriggers());
	InputFrame(EKeys::Tab, IE_Released);
	MappingTriggers();

	// Right Shift is also a previous-weapon chord modifier, but it is not the
	// sprint key. It must block plain Tab exactly as Left Shift does.
	InputFrame(EKeys::RightShift, IE_Pressed);
	MappingTriggers();
	InputFrame(EKeys::Tab, IE_Pressed);
	TestFalse(TEXT("Right Shift blocks a fresh Tab press"), MappingTriggers());
	InputFrame(EKeys::RightShift, IE_Released);
	TestFalse(TEXT("Releasing Right Shift first does not re-fire Tab"),
		MappingTriggers());
	InputFrame(EKeys::Tab, IE_Released);
	MappingTriggers();

	GEngine->DestroyWorldContext(InputWorld);
	InputWorld->DestroyWorld(false);
	return true;
}
