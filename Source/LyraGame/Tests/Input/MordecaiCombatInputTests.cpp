// Project Mordecai — Combat, jump and sprint input (US-083)
// UMordecaiHeroComponent::BuildAndBindRuntimeInput maps the combat keys in MordecaiIMC_Runtime and binds them. The
// headless tests below read those mappings and bindings directly and execute the bindings; they need no LocalPlayer.

#include "Misc/AutomationTest.h"
#include "Misc/ScopeExit.h"
#include "AbilitySystem/LyraAbilitySystemComponent.h"
#include "EnhancedInputComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameplayAbilitySpec.h"
#include "Input/LyraInputConfig.h"
#include "InputAction.h"
#include "InputMappingContext.h"
#include "InputTriggers.h"
#include "LyraGameplayTags.h"
#include "Mordecai/Combat/MordecaiGA_Block.h"
#include "Mordecai/Combat/MordecaiGA_Dodge.h"
#include "Mordecai/Combat/MordecaiGA_HeavyAttack.h"
#include "Mordecai/Combat/MordecaiGA_MeleeAttack.h"
#include "Mordecai/Combat/MordecaiGA_Parry.h"
#include "Mordecai/MordecaiCharacter.h"
#include "Mordecai/MordecaiGameplayTags.h"
#include "Mordecai/MordecaiHeroComponent.h"
#include "Tests/Combat/MordecaiCombatGrantTestEnv.h"
#include "UObject/SoftObjectPath.h"

namespace MordecaiCombatInputTestHelpers
{
	static UInputAction* LoadMordecaiInputAction(const TCHAR* Name)
	{
		return Cast<UInputAction>(FSoftObjectPath(FString::Printf(TEXT("/MordecaiCore/Input/Actions/%s.%s"), Name, Name)).TryLoad());
	}

	static const FEnhancedActionKeyMapping* FindKeyMapping(const UInputMappingContext* IMC, const UInputAction* Action, const FKey& Key)
	{
		for (const FEnhancedActionKeyMapping& Mapping : IMC->GetMappings())
		{
			if (Mapping.Action == Action && Mapping.Key == Key)
			{
				return &Mapping;
			}
		}
		return nullptr;
	}

	template <typename TTrigger>
	static const TTrigger* FindMappingTrigger(const FEnhancedActionKeyMapping* Mapping)
	{
		if (Mapping)
		{
			for (const TObjectPtr<UInputTrigger>& Trigger : Mapping->Triggers)
			{
				if (const TTrigger* Typed = ExactCast<TTrigger>(Trigger.Get()))
				{
					return Typed;
				}
			}
		}
		return nullptr;
	}

	/** The action mapped on Key with a chord on ChordAction (the runtime parry action has no asset). */
	static const UInputAction* FindChordedAction(const UInputMappingContext* IMC, const FKey& Key, const UInputAction* ChordAction)
	{
		for (const FEnhancedActionKeyMapping& Mapping : IMC->GetMappings())
		{
			const UInputTriggerChordAction* Chord = FindMappingTrigger<UInputTriggerChordAction>(&Mapping);
			if (Mapping.Key == Key && Chord && Chord->ChordAction == ChordAction)
			{
				return Mapping.Action;
			}
		}
		return nullptr;
	}

	static int32 FindBindingIndex(const UEnhancedInputComponent* EIC, const UInputAction* Action, ETriggerEvent Event, const UObject* BoundTo)
	{
		const TArray<TUniquePtr<FEnhancedInputActionEventBinding>>& Bindings = EIC->GetActionEventBindings();
		for (int32 Index = 0; Index < Bindings.Num(); ++Index)
		{
			const FEnhancedInputActionEventBinding* Binding = Bindings[Index].Get();
			if (Binding && Binding->GetAction() == Action && Binding->GetTriggerEvent() == Event && Binding->IsBoundToObject(BoundTo))
			{
				return Index;
			}
		}
		return INDEX_NONE;
	}

	/** Run a binding as Enhanced Input does when its event fires. Returns false if there is no such binding. */
	static bool ExecuteBinding(const UEnhancedInputComponent* EIC, const UInputAction* Action, ETriggerEvent Event, const UObject* BoundTo)
	{
		const int32 Index = FindBindingIndex(EIC, Action, Event, BoundTo);
		if (Index == INDEX_NONE)
		{
			return false;
		}
		EIC->GetActionEventBindings()[Index]->Execute(FInputActionInstance(Action));
		return true;
	}
}

// ---------------------------------------------------------------------------
// Mordecai.Recovery.Input.CombatActionsMappedAndBound (US-083 AC-083.3)
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_Recovery_InputCombatActionsMappedAndBound,
	"Mordecai.Recovery.Input.CombatActionsMappedAndBound",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_Recovery_InputCombatActionsMappedAndBound::RunTest(const FString& Parameters)
{
	using namespace MordecaiCombatInputTestHelpers;

	AMordecaiCharacter* Character = NewObject<AMordecaiCharacter>();
	UMordecaiHeroComponent* Hero = Character->FindComponentByClass<UMordecaiHeroComponent>();
	if (!TestNotNull("Hero component", Hero))
	{
		return true;
	}
	UEnhancedInputComponent* EIC = NewObject<UEnhancedInputComponent>(Character);
	const UInputMappingContext* IMC = Hero->BuildAndBindRuntimeInput(EIC);
	if (!TestNotNull("Runtime mapping context built", IMC))
	{
		return true;
	}
	TestTrue("The hero keeps the context", IMC == Hero->GetRuntimeMappingContext());
	TestEqualSensitive(TEXT("Context name"), IMC->GetName(), FString(TEXT("MordecaiIMC_Runtime")));

	const UInputAction* Light = LoadMordecaiInputAction(TEXT("IA_Mordecai_LightAttack"));
	const UInputAction* Heavy = LoadMordecaiInputAction(TEXT("IA_Mordecai_HeavyAttack"));
	const UInputAction* Block = LoadMordecaiInputAction(TEXT("IA_Mordecai_Block"));
	const UInputAction* Dodge = LoadMordecaiInputAction(TEXT("IA_Mordecai_Dodge"));
	const UInputAction* Jump = LoadMordecaiInputAction(TEXT("IA_Mordecai_Jump"));
	const UInputAction* Sprint = LoadMordecaiInputAction(TEXT("IA_Mordecai_Sprint"));
	if (!TestNotNull("IA_Mordecai_LightAttack loads", Light) || !TestNotNull("IA_Mordecai_HeavyAttack loads", Heavy)
		|| !TestNotNull("IA_Mordecai_Block loads", Block) || !TestNotNull("IA_Mordecai_Dodge loads", Dodge)
		|| !TestNotNull("IA_Mordecai_Jump loads", Jump) || !TestNotNull("IA_Mordecai_Sprint loads", Sprint))
	{
		return true;
	}

	// LMB / RT: released before the heavy hold time = light attack, held that long = heavy attack on release,
	// pressed while Block is triggered = parry
	const float HoldSeconds = Hero->HeavyAttackHoldSeconds;
	// TODO(DECISION) value in UMordecaiHeroComponent; above 0 (else no light attack) and below the 0.8 s hold of the
	// Tier 4 heavy_attack_hold check
	TestEqual("Heavy hold time default", HoldSeconds, 0.4f);
	const UInputAction* Parry = FindChordedAction(IMC, EKeys::LeftMouseButton, Block);
	if (!TestNotNull("LMB parry mapping chorded on Block", Parry))
	{
		return true;
	}
	TestEqualSensitive(TEXT("Parry action name"), Parry->GetName(), FString(TEXT("IA_Mordecai_Parry_Runtime")));
	TestFalse("Parry does not fire while paused", Parry->bTriggerWhenPaused);

	for (const FKey& Key : { EKeys::LeftMouseButton, EKeys::Gamepad_RightTriggerAxis })
	{
		const FString KeyName = Key.ToString();
		const UInputTriggerTap* Tap = FindMappingTrigger<UInputTriggerTap>(FindKeyMapping(IMC, Light, Key));
		if (TestNotNull(*FString::Printf(TEXT("%s maps light attack with a Tap trigger"), *KeyName), Tap))
		{
			TestEqual(*FString::Printf(TEXT("%s: tap window is the heavy hold time"), *KeyName), Tap->TapReleaseTimeThreshold, HoldSeconds);
		}
		const UInputTriggerHoldAndRelease* Hold = FindMappingTrigger<UInputTriggerHoldAndRelease>(FindKeyMapping(IMC, Heavy, Key));
		if (TestNotNull(*FString::Printf(TEXT("%s maps heavy attack with a HoldAndRelease trigger"), *KeyName), Hold))
		{
			TestEqual(*FString::Printf(TEXT("%s: heavy hold time"), *KeyName), Hold->HoldTimeThreshold, HoldSeconds);
		}
		const FEnhancedActionKeyMapping* ParryMapping = FindKeyMapping(IMC, Parry, Key);
		const UInputTriggerChordAction* Chord = FindMappingTrigger<UInputTriggerChordAction>(ParryMapping);
		TestTrue(*FString::Printf(TEXT("%s maps parry chorded on Block"), *KeyName), Chord && Chord->ChordAction == Block);
		TestNotNull(*FString::Printf(TEXT("%s parry needs a fresh press"), *KeyName), FindMappingTrigger<UInputTriggerPressed>(ParryMapping));
	}
	for (const FKey& Key : { EKeys::RightMouseButton, EKeys::Gamepad_LeftTriggerAxis })
	{
		TestNotNull(*FString::Printf(TEXT("%s maps block, held (Down trigger)"), *Key.ToString()),
			FindMappingTrigger<UInputTriggerDown>(FindKeyMapping(IMC, Block, Key)));
	}
	for (const FKey& Key : { EKeys::LeftAlt, EKeys::Gamepad_FaceButton_Right })
	{
		TestNotNull(*FString::Printf(TEXT("%s maps dodge, once per press (Pressed trigger)"), *Key.ToString()),
			FindMappingTrigger<UInputTriggerPressed>(FindKeyMapping(IMC, Dodge, Key)));
	}
	TestNotNull("SpaceBar maps jump", FindKeyMapping(IMC, Jump, EKeys::SpaceBar));
	TestNotNull("Gamepad A maps jump", FindKeyMapping(IMC, Jump, EKeys::Gamepad_FaceButton_Bottom));
	TestNotNull("LeftShift maps sprint", FindKeyMapping(IMC, Sprint, EKeys::LeftShift));

	// Bindings: ability input presses / releases a tag on the ASC (the hero's handlers); jump and sprint act on the character
	struct FExpectedBinding
	{
		const UInputAction* Action;
		ETriggerEvent Event;
		const UObject* BoundTo;
		const TCHAR* What;
	};
	const FExpectedBinding ExpectedBindings[] = {
		{ Light, ETriggerEvent::Triggered, Hero, TEXT("light attack press") },
		{ Light, ETriggerEvent::Completed, Hero, TEXT("light attack release") },
		{ Light, ETriggerEvent::Canceled, Hero, TEXT("light attack release (canceled)") },
		{ Heavy, ETriggerEvent::Triggered, Hero, TEXT("heavy attack press") },
		{ Heavy, ETriggerEvent::Completed, Hero, TEXT("heavy attack release") },
		{ Heavy, ETriggerEvent::Canceled, Hero, TEXT("heavy attack release (canceled)") },
		{ Block, ETriggerEvent::Triggered, Hero, TEXT("block press") },
		{ Block, ETriggerEvent::Completed, Hero, TEXT("block release") },
		{ Block, ETriggerEvent::Canceled, Hero, TEXT("block release (canceled)") },
		{ Parry, ETriggerEvent::Triggered, Hero, TEXT("parry press") },
		{ Parry, ETriggerEvent::Completed, Hero, TEXT("parry release") },
		{ Parry, ETriggerEvent::Canceled, Hero, TEXT("parry release (canceled)") },
		{ Dodge, ETriggerEvent::Triggered, Hero, TEXT("dodge press") },
		{ Dodge, ETriggerEvent::Completed, Hero, TEXT("dodge release") },
		{ Dodge, ETriggerEvent::Canceled, Hero, TEXT("dodge release (canceled)") },
		{ Jump, ETriggerEvent::Started, Character, TEXT("jump") },
		{ Jump, ETriggerEvent::Completed, Character, TEXT("stop jumping") },
		{ Sprint, ETriggerEvent::Started, Hero, TEXT("sprint toggle") },
	};
	for (const FExpectedBinding& Expected : ExpectedBindings)
	{
		TestTrue(*FString::Printf(TEXT("Bound: %s"), Expected.What),
			FindBindingIndex(EIC, Expected.Action, Expected.Event, Expected.BoundTo) != INDEX_NONE);
	}
	TestTrue("Light is bound before heavy, so a release exactly at the hold time is a light attack",
		FindBindingIndex(EIC, Light, ETriggerEvent::Triggered, Hero) < FindBindingIndex(EIC, Heavy, ETriggerEvent::Triggered, Hero));
	TestEqual("Sprint toggles once per press (Started only)", FindBindingIndex(EIC, Sprint, ETriggerEvent::Triggered, Hero), INDEX_NONE);
	// Started would fire when the block is raised over a held or partly pulled attack button (the chord alone makes
	// the parry mapping Ongoing); only Triggered needs the fresh press
	TestEqual("Parry does not press on Started", FindBindingIndex(EIC, Parry, ETriggerEvent::Started, Hero), INDEX_NONE);

	// Jump and sprint reach the character
	TestTrue("Jump binding runs", ExecuteBinding(EIC, Jump, ETriggerEvent::Started, Character));
	TestTrue("Jump pressed", Character->bPressedJump != 0);
	TestTrue("Stop-jumping binding runs", ExecuteBinding(EIC, Jump, ETriggerEvent::Completed, Character));
	TestTrue("Jump released", Character->bPressedJump == 0);

	const UCharacterMovementComponent* CMC = Character->GetCharacterMovement();
	TestTrue("Sprint binding runs", ExecuteBinding(EIC, Sprint, ETriggerEvent::Started, Hero));
	TestTrue("First press sprints", Character->bIsSprinting);
	TestEqual("Sprint speed", CMC->MaxWalkSpeed, Character->WalkSpeed * Character->SprintSpeedMultiplier);
	ExecuteBinding(EIC, Sprint, ETriggerEvent::Started, Hero);
	TestFalse("Second press toggles back", Character->bIsSprinting);
	TestEqual("Walk speed", CMC->MaxWalkSpeed, Character->WalkSpeed);

	// Building again (the pawn is possessed again) rebinds instead of stacking: a doubled toggle would cancel itself
	const int32 NumBindings = EIC->GetActionEventBindings().Num();
	TestTrue("Same context on a second build", Hero->BuildAndBindRuntimeInput(EIC) == IMC);
	TestEqual("Same number of bindings after a second build", EIC->GetActionEventBindings().Num(), NumBindings);
	TestTrue("Same parry action on a second build", FindChordedAction(IMC, EKeys::LeftMouseButton, Block) == Parry);
	return true;
}

// ---------------------------------------------------------------------------
// Mordecai.Recovery.Input.AbilityBindingsPressTheirInputTags
// Each combat binding presses and releases the input tag of its own ability's spec on the player's ASC.
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_Recovery_InputAbilityBindingsPressTheirInputTags,
	"Mordecai.Recovery.Input.AbilityBindingsPressTheirInputTags",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_Recovery_InputAbilityBindingsPressTheirInputTags::RunTest(const FString& Parameters)
{
	using namespace MordecaiCombatInputTestHelpers;

	// The real grant, without an avatar: activation attempts fail quietly, and each spec's InputPressed flag
	// shows which spec a binding pressed
	MordecaiCombatGrantTest::FMordecaiCombatGrantTestEnv Env;
	ON_SCOPE_EXIT { Env.Teardown(); };
	if (!TestTrue("Test world set up", Env.Setup()))
	{
		return true;
	}
	Env.GrantCombatOnly();
	ULyraAbilitySystemComponent* ASC = Env.GetASC();
	if (!TestNotNull("ASC", ASC))
	{
		return true;
	}

	AMordecaiCharacter* Character = NewObject<AMordecaiCharacter>();
	UMordecaiHeroComponent* Hero = Character->FindComponentByClass<UMordecaiHeroComponent>();
	if (!TestNotNull("Hero component", Hero))
	{
		return true;
	}
	Hero->SetAbilitySystemOverride(ASC);
	UEnhancedInputComponent* EIC = NewObject<UEnhancedInputComponent>(Character);
	const UInputMappingContext* IMC = Hero->BuildAndBindRuntimeInput(EIC);
	const UInputAction* Block = LoadMordecaiInputAction(TEXT("IA_Mordecai_Block"));
	if (!TestNotNull("Runtime mapping context built", IMC) || !TestNotNull("IA_Mordecai_Block loads", Block))
	{
		return true;
	}

	struct FCase
	{
		const UInputAction* Action;
		ETriggerEvent PressEvent;
		UClass* AbilityClass;
	};
	const FCase Cases[] = {
		{ LoadMordecaiInputAction(TEXT("IA_Mordecai_LightAttack")), ETriggerEvent::Triggered, UMordecaiGA_MeleeAttack::StaticClass() },
		{ LoadMordecaiInputAction(TEXT("IA_Mordecai_HeavyAttack")), ETriggerEvent::Triggered, UMordecaiGA_HeavyAttack::StaticClass() },
		{ Block, ETriggerEvent::Triggered, UMordecaiGA_Block::StaticClass() },
		{ FindChordedAction(IMC, EKeys::LeftMouseButton, Block), ETriggerEvent::Triggered, UMordecaiGA_Parry::StaticClass() },
		{ LoadMordecaiInputAction(TEXT("IA_Mordecai_Dodge")), ETriggerEvent::Triggered, UMordecaiGA_Dodge::StaticClass() },
	};

	for (const FCase& Case : Cases)
	{
		const FString Name = Case.AbilityClass->GetName();
		if (!TestNotNull(*FString::Printf(TEXT("%s: input action"), *Name), Case.Action)
			|| !TestTrue(*FString::Printf(TEXT("%s: press binding runs"), *Name), ExecuteBinding(EIC, Case.Action, Case.PressEvent, Hero)))
		{
			continue;
		}
		ASC->ProcessAbilityInput(0.f, false);
		for (const FCase& Other : Cases)
		{
			const FGameplayAbilitySpec* Spec = Env.FindSpec(Other.AbilityClass);
			const bool bPressed = Spec && Spec->InputPressed;
			TestTrue(*FString::Printf(TEXT("%s press -> %s spec %s"), *Name, *Other.AbilityClass->GetName(),
				Other.AbilityClass == Case.AbilityClass ? TEXT("pressed") : TEXT("not pressed")),
				bPressed == (Other.AbilityClass == Case.AbilityClass));
		}

		TestTrue(*FString::Printf(TEXT("%s: release binding runs"), *Name), ExecuteBinding(EIC, Case.Action, ETriggerEvent::Completed, Hero));
		ASC->ProcessAbilityInput(0.f, false);
		const FGameplayAbilitySpec* Spec = Env.FindSpec(Case.AbilityClass);
		TestTrue(*FString::Printf(TEXT("%s: released"), *Name), Spec && !Spec->InputPressed);

		// Canceled releases too (every heavy hold cancels the light Tap; a parry chord ends in Canceled)
		ExecuteBinding(EIC, Case.Action, Case.PressEvent, Hero);
		ASC->ProcessAbilityInput(0.f, false);
		TestTrue(*FString::Printf(TEXT("%s: canceled-release binding runs"), *Name), ExecuteBinding(EIC, Case.Action, ETriggerEvent::Canceled, Hero));
		ASC->ProcessAbilityInput(0.f, false);
		for (const FCase& Other : Cases)
		{
			const FGameplayAbilitySpec* OtherSpec = Env.FindSpec(Other.AbilityClass);
			TestTrue(*FString::Printf(TEXT("%s canceled -> %s spec not pressed"), *Name, *Other.AbilityClass->GetName()),
				OtherSpec && !OtherSpec->InputPressed);
		}
	}
	return true;
}

// ---------------------------------------------------------------------------
// Mordecai.Recovery.Input.LyraCrouchBindingRemoved
// InputData_Mordecai tags IA_Mordecai_Sprint with InputTag.Crouch, so Lyra bound the sprint key to its crouch
// toggle. The hero removes that binding after Lyra makes it (InitializePlayerInput); this checks the removal.
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_Recovery_InputLyraCrouchBindingRemoved,
	"Mordecai.Recovery.Input.LyraCrouchBindingRemoved",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_Recovery_InputLyraCrouchBindingRemoved::RunTest(const FString& Parameters)
{
	using namespace MordecaiCombatInputTestHelpers;

	UInputAction* SprintAction = NewObject<UInputAction>();
	UInputAction* OtherAction = NewObject<UInputAction>();
	ULyraInputConfig* Config = NewObject<ULyraInputConfig>();
	FLyraInputAction CrouchEntry;
	CrouchEntry.InputAction = SprintAction;
	CrouchEntry.InputTag = LyraGameplayTags::InputTag_Crouch;
	Config->NativeInputActions.Add(CrouchEntry);

	AMordecaiCharacter* Bound = NewObject<AMordecaiCharacter>();
	AMordecaiCharacter* Bystander = NewObject<AMordecaiCharacter>();
	UEnhancedInputComponent* EIC = NewObject<UEnhancedInputComponent>(Bound);
	EIC->BindAction(SprintAction, ETriggerEvent::Triggered, Bound, &AMordecaiCharacter::CycleNextWeapon);
	EIC->BindAction(SprintAction, ETriggerEvent::Triggered, Bystander, &AMordecaiCharacter::CycleNextWeapon);
	EIC->BindAction(OtherAction, ETriggerEvent::Triggered, Bound, &AMordecaiCharacter::CycleNextWeapon);

	TestEqual("Removes the crouch action's binding to that object",
		UMordecaiHeroComponent::RemoveNativeBindingsForTag(EIC, Config, LyraGameplayTags::InputTag_Crouch, Bound), 1);
	TestEqual("Crouch binding gone", FindBindingIndex(EIC, SprintAction, ETriggerEvent::Triggered, Bound), INDEX_NONE);
	TestTrue("Another object's binding of the action stays", FindBindingIndex(EIC, SprintAction, ETriggerEvent::Triggered, Bystander) != INDEX_NONE);
	TestTrue("The object's other bindings stay", FindBindingIndex(EIC, OtherAction, ETriggerEvent::Triggered, Bound) != INDEX_NONE);
	TestEqual("Two bindings left", EIC->GetActionEventBindings().Num(), 2);

	TestEqual("A tag the config does not use removes nothing",
		UMordecaiHeroComponent::RemoveNativeBindingsForTag(EIC, Config, LyraGameplayTags::InputTag_Move, Bystander), 0);
	TestEqual("No input config removes nothing",
		UMordecaiHeroComponent::RemoveNativeBindingsForTag(EIC, nullptr, LyraGameplayTags::InputTag_Crouch, Bystander), 0);
	TestEqual("No input component removes nothing",
		UMordecaiHeroComponent::RemoveNativeBindingsForTag(nullptr, Config, LyraGameplayTags::InputTag_Crouch, Bystander), 0);
	return true;
}
