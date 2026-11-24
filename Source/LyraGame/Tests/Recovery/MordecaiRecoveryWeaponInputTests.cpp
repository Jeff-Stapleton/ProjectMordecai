// Project Mordecai — previous weapon input recovery (US-096, issue #14)

#include "Misc/AutomationTest.h"

#include "EnhancedInputComponent.h"
#include "InputAction.h"
#include "InputMappingContext.h"
#include "InputTriggers.h"
#include "Mordecai/MordecaiCharacter.h"
#include "Mordecai/MordecaiHeroComponent.h"
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

	static int32 FindChordedTab(
		const UInputMappingContext* Context,
		const UInputAction*& OutChordAction)
	{
		const TArray<FEnhancedActionKeyMapping>& Mappings = Context->GetMappings();
		for (int32 Index = 0; Index < Mappings.Num(); ++Index)
		{
			if (Mappings[Index].Key != EKeys::Tab)
			{
				continue;
			}
			if (const UInputTriggerChordAction* Chord =
				FindTrigger<UInputTriggerChordAction>(Mappings[Index]))
			{
				OutChordAction = Chord->ChordAction;
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
	TestTrue(TEXT("Tab alone maps next weapon"), PlainTabIndex != INDEX_NONE);
	TestTrue(TEXT("Tab binding executes"),
		ExecuteBinding(EIC, NextAction, ETriggerEvent::Started, Hero));
	TestEqual(TEXT("Tab alone selects the next weapon"),
		Equipment->GetCurrentWeaponIndex(), 1);

	const UInputAction* ShiftChordAction = nullptr;
	const int32 ShiftTabIndex = FindChordedTab(Context, ShiftChordAction);
	if (!TestTrue(TEXT("Shift+Tab has a chorded mapping"),
		ShiftTabIndex != INDEX_NONE)
		|| !TestNotNull(TEXT("Shift+Tab names a chord action"),
			ShiftChordAction))
	{
		return true;
	}

	const TArray<FEnhancedActionKeyMapping>& Mappings = Context->GetMappings();
	const UInputAction* ShiftTabAction = Mappings[ShiftTabIndex].Action;
	const int32 ShiftChordIndex =
		FindMapping(Context, ShiftChordAction, EKeys::LeftShift);
	const int32 SprintShiftIndex =
		FindMapping(Context, SprintAction, EKeys::LeftShift);
	TestTrue(TEXT("Chord action maps Left Shift"), ShiftChordIndex != INDEX_NONE);
	TestTrue(TEXT("Sprint action still maps Left Shift"), SprintShiftIndex != INDEX_NONE);
	TestFalse(TEXT("Chord source does not consume sprint input"),
		ShiftChordAction->bConsumeInput);
	TestTrue(TEXT("Chord source maps before the consuming sprint action"),
		ShiftChordIndex < SprintShiftIndex);
	TestNotNull(TEXT("Shift+Tab requires a fresh Tab press"),
		FindTrigger<UInputTriggerPressed>(Mappings[ShiftTabIndex]));
	TestTrue(TEXT("Chord mapping precedes plain Tab for automatic chord blocking"),
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
