// Project Mordecai

#include "MordecaiHeroComponent.h"
#include "Mordecai/MordecaiCharacter.h"
#include "Mordecai/MordecaiGameplayTags.h"
#include "Mordecai/MordecaiPlayerController.h"
#include "Mordecai/Items/MordecaiPickupInteractionComponent.h"
#include "Mordecai/UI/MordecaiInventoryWidget.h"
#include "Mordecai/UI/MordecaiPauseMenuSubsystem.h"
#include "Engine/GameInstance.h"
#include "GameFeatures/GameFeatureAction_AddInputContextMapping.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemInterface.h"
#include "AbilitySystem/LyraAbilitySystemComponent.h"
#include "Character/LyraPawnData.h"
#include "Character/LyraPawnExtensionComponent.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "Input/LyraInputConfig.h"
#include "InputAction.h"
#include "InputMappingContext.h"
#include "InputModifiers.h"
#include "InputTriggers.h"
#include "LyraGameplayTags.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerState.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(MordecaiHeroComponent)

UMordecaiHeroComponent::UMordecaiHeroComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	// Note: We no longer rely on DefaultInputMappings for the IMC_Mordecai .uasset
	// because its modifiers can't be serialized via Python. Instead, we create a
	// programmatic IMC in EnsureMordecaiInputBound with proper C++ modifiers.
}

void UMordecaiHeroComponent::EnsureMordecaiInputBound(APlayerController* PC)
{
	if (!PC)
	{
		return;
	}

	ULocalPlayer* LP = PC->GetLocalPlayer();
	if (!LP)
	{
		return;
	}

	UEnhancedInputLocalPlayerSubsystem* Subsystem = LP->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>();
	if (!Subsystem)
	{
		return;
	}

	UEnhancedInputComponent* EIC = Cast<UEnhancedInputComponent>(PC->InputComponent.Get());
	if (!EIC)
	{
		return;
	}

	UInputMappingContext* MordecaiIMC = BuildAndBindRuntimeInput(EIC);
	if (!MordecaiIMC)
	{
		return;
	}

	// Add the programmatic IMC to the subsystem
	Subsystem->AddMappingContext(MordecaiIMC, 2); // Priority 2 to override any broken .uasset IMC
}

UInputMappingContext* UMordecaiHeroComponent::BuildAndBindRuntimeInput(UEnhancedInputComponent* EIC)
{
	if (!EIC)
	{
		return nullptr;
	}

	// Load input actions from MordecaiCore
	auto LoadAction = [](const TCHAR* Path) -> UInputAction*
	{
		FSoftObjectPath SoftPath(Path);
		return Cast<UInputAction>(SoftPath.TryLoad());
	};

	UInputAction* MoveAction = LoadAction(TEXT("/MordecaiCore/Input/Actions/IA_Mordecai_Move.IA_Mordecai_Move"));
	UInputAction* LookMouseAction = LoadAction(TEXT("/MordecaiCore/Input/Actions/IA_Mordecai_Look_Mouse.IA_Mordecai_Look_Mouse"));
	UInputAction* LookStickAction = LoadAction(TEXT("/MordecaiCore/Input/Actions/IA_Mordecai_Look_Stick.IA_Mordecai_Look_Stick"));
	UInputAction* JumpAction = LoadAction(TEXT("/MordecaiCore/Input/Actions/IA_Mordecai_Jump.IA_Mordecai_Jump"));
	UInputAction* SprintAction = LoadAction(TEXT("/MordecaiCore/Input/Actions/IA_Mordecai_Sprint.IA_Mordecai_Sprint"));

	if (!MoveAction)
	{
		UE_LOG(LogTemp, Error, TEXT("MORDECAI: Failed to load IA_Mordecai_Move!"));
		return nullptr;
	}

	// --- AC-057.5: Spell Input Actions (Q/LB, E/RB, R, 1/DPadUp) ---
	UInputAction* Spell1Action = LoadAction(TEXT("/MordecaiCore/Input/Actions/IA_Mordecai_Spell1.IA_Mordecai_Spell1"));
	UInputAction* Spell2Action = LoadAction(TEXT("/MordecaiCore/Input/Actions/IA_Mordecai_Spell2.IA_Mordecai_Spell2"));
	UInputAction* Spell3Action = LoadAction(TEXT("/MordecaiCore/Input/Actions/IA_Mordecai_Spell3.IA_Mordecai_Spell3"));
	UInputAction* Spell4Action = LoadAction(TEXT("/MordecaiCore/Input/Actions/IA_Mordecai_Spell4.IA_Mordecai_Spell4"));

	// --- US-078: Weapon Cycling Input Actions (Tab / Shift+Tab / Gamepad Y) ---
	UInputAction* WeaponCycleNextAction = LoadAction(TEXT("/MordecaiCore/Input/Actions/IA_Mordecai_WeaponSwap.IA_Mordecai_WeaponSwap"));
	UInputAction* WeaponCyclePrevAction = LoadAction(TEXT("/MordecaiCore/Input/Actions/IA_Mordecai_WeaponCyclePrev.IA_Mordecai_WeaponCyclePrev"));

	// --- US-079: Interact (press-to-pickup) + Pause / Inventory menu actions ---
	UInputAction* InteractAction = LoadAction(TEXT("/MordecaiCore/Input/Actions/IA_Mordecai_Interact.IA_Mordecai_Interact"));
	UInputAction* PauseAction = LoadAction(TEXT("/MordecaiCore/Input/Actions/IA_Mordecai_Pause.IA_Mordecai_Pause"));
	if (!InventoryAction)
	{
		// No IA asset exists for direct-to-inventory; a programmatic action matches
		// the programmatic-IMC pattern and keeps the binding permanent in C++.
		InventoryAction = NewObject<UInputAction>(this, TEXT("IA_Mordecai_Inventory_Runtime"));
		InventoryAction->ValueType = EInputActionValueType::Boolean;
		InventoryAction->bTriggerWhenPaused = true;
	}
	if (PauseAction)
	{
		// Menu keys must fire while the game is paused so the toggle can close too
		PauseAction->bTriggerWhenPaused = true;
	}

	// --- US-083: Combat input actions (light / heavy attack, block, dodge; parry has no asset) ---
	UInputAction* LightAttackAction = LoadAction(TEXT("/MordecaiCore/Input/Actions/IA_Mordecai_LightAttack.IA_Mordecai_LightAttack"));
	UInputAction* HeavyAttackAction = LoadAction(TEXT("/MordecaiCore/Input/Actions/IA_Mordecai_HeavyAttack.IA_Mordecai_HeavyAttack"));
	UInputAction* BlockAction = LoadAction(TEXT("/MordecaiCore/Input/Actions/IA_Mordecai_Block.IA_Mordecai_Block"));
	UInputAction* DodgeAction = LoadAction(TEXT("/MordecaiCore/Input/Actions/IA_Mordecai_Dodge.IA_Mordecai_Dodge"));
	if (!ParryAction)
	{
		ParryAction = NewObject<UInputAction>(this, TEXT("IA_Mordecai_Parry_Runtime"));
		ParryAction->ValueType = EInputActionValueType::Boolean;
	}

	if (!RuntimeMappingContext)
	{
		// Create a programmatic IMC with proper modifiers (permanent C++ solution)
		UInputMappingContext* MordecaiIMC = NewObject<UInputMappingContext>(this, TEXT("MordecaiIMC_Runtime"));

		// --- WASD Movement ---
		{
			// W = Forward (+Y). Swizzle XY→YX so 1D input goes to Y axis
			FEnhancedActionKeyMapping& W = MordecaiIMC->MapKey(MoveAction, FKey(TEXT("W")));
			UInputModifierSwizzleAxis* SwizzleW = NewObject<UInputModifierSwizzleAxis>(MordecaiIMC);
			SwizzleW->Order = EInputAxisSwizzle::YXZ;
			W.Modifiers.Add(SwizzleW);

			// S = Backward (-Y). Negate then Swizzle
			FEnhancedActionKeyMapping& S = MordecaiIMC->MapKey(MoveAction, FKey(TEXT("S")));
			UInputModifierNegate* NegS = NewObject<UInputModifierNegate>(MordecaiIMC);
			UInputModifierSwizzleAxis* SwizzleS = NewObject<UInputModifierSwizzleAxis>(MordecaiIMC);
			SwizzleS->Order = EInputAxisSwizzle::YXZ;
			S.Modifiers.Add(NegS);
			S.Modifiers.Add(SwizzleS);

			// D = Right (+X). No modifiers
			MordecaiIMC->MapKey(MoveAction, FKey(TEXT("D")));

			// A = Left (-X). Negate
			FEnhancedActionKeyMapping& A = MordecaiIMC->MapKey(MoveAction, FKey(TEXT("A")));
			UInputModifierNegate* NegA = NewObject<UInputModifierNegate>(MordecaiIMC);
			A.Modifiers.Add(NegA);

			// Gamepad Left Stick
			FEnhancedActionKeyMapping& Stick = MordecaiIMC->MapKey(MoveAction, FKey(TEXT("Gamepad_LeftStick2D")));
			UInputModifierDeadZone* DZ = NewObject<UInputModifierDeadZone>(MordecaiIMC);
			DZ->LowerThreshold = 0.2f;
			Stick.Modifiers.Add(DZ);
		}

		// --- Look ---
		if (LookMouseAction)
		{
			MordecaiIMC->MapKey(LookMouseAction, FKey(TEXT("Mouse2D")));
		}
		if (LookStickAction)
		{
			FEnhancedActionKeyMapping& RS = MordecaiIMC->MapKey(LookStickAction, FKey(TEXT("Gamepad_RightStick2D")));
			UInputModifierDeadZone* DZ2 = NewObject<UInputModifierDeadZone>(MordecaiIMC);
			DZ2->LowerThreshold = 0.2f;
			RS.Modifiers.Add(DZ2);
		}

		// --- Jump (Space / Gamepad A) ---
		if (JumpAction)
		{
			MordecaiIMC->MapKey(JumpAction, FKey(TEXT("SpaceBar")));
			MordecaiIMC->MapKey(JumpAction, FKey(TEXT("Gamepad_FaceButton_Bottom")));
		}

		// --- Sprint (Shift / L3) ---
		if (SprintAction)
		{
			MordecaiIMC->MapKey(SprintAction, FKey(TEXT("LeftShift")));
			MordecaiIMC->MapKey(SprintAction, FKey(TEXT("Gamepad_LeftThumbstickButton")));
		}

		if (Spell1Action)
		{
			MordecaiIMC->MapKey(Spell1Action, FKey(TEXT("Q")));
			MordecaiIMC->MapKey(Spell1Action, FKey(TEXT("Gamepad_LeftShoulder")));
		}
		if (Spell2Action)
		{
			MordecaiIMC->MapKey(Spell2Action, FKey(TEXT("E")));
			MordecaiIMC->MapKey(Spell2Action, FKey(TEXT("Gamepad_RightShoulder")));
		}
		if (Spell3Action)
		{
			MordecaiIMC->MapKey(Spell3Action, FKey(TEXT("R")));
		}
		if (Spell4Action)
		{
			MordecaiIMC->MapKey(Spell4Action, FKey(TEXT("One")));
			MordecaiIMC->MapKey(Spell4Action, FKey(TEXT("Gamepad_DPad_Up")));
		}

		// --- US-078: Weapon cycling ---
		// AC-078.6: Next weapon — Tab (MKB) / Gamepad Y tap
		if (WeaponCycleNextAction)
		{
			MordecaiIMC->MapKey(WeaponCycleNextAction, FKey(TEXT("Tab")));
			MordecaiIMC->MapKey(WeaponCycleNextAction, FKey(TEXT("Gamepad_FaceButton_Top")));
		}
		// AC-078.7: Previous weapon — BackSlash (pragmatic alternative to Shift+Tab chord,
		// which requires a separate IA_Sprint chord-action setup in Enhanced Input).
		if (WeaponCyclePrevAction)
		{
			MordecaiIMC->MapKey(WeaponCyclePrevAction, FKey(TEXT("BackSlash")));
		}

		// --- US-079: Interact — F (MKB) / Gamepad X, per control_bindings_v1.1 ---
		if (InteractAction)
		{
			MordecaiIMC->MapKey(InteractAction, FKey(TEXT("F")));
			MordecaiIMC->MapKey(InteractAction, FKey(TEXT("Gamepad_FaceButton_Left")));
		}
		// --- US-079: Pause menu — Esc (MKB) / Start; I opens straight to Inventory ---
		if (PauseAction)
		{
			MordecaiIMC->MapKey(PauseAction, FKey(TEXT("Escape")));
			MordecaiIMC->MapKey(PauseAction, FKey(TEXT("Gamepad_Special_Right")));
		}
		MordecaiIMC->MapKey(InventoryAction, FKey(TEXT("I")));

		// --- US-083: Combat, per control_bindings_v1.1. Triggers go on the mappings, never on the shared IA assets ---
		// LMB / RT: released before HeavyAttackHoldSeconds is a light attack (Tap), held at least that long a heavy
		// attack on release (HoldAndRelease). Both time the same press, so every press is one or the other.
		// TODO(DECISION): the heavy attack fires on release (charge, then let go); firing at the hold time while the
		// button is still down is the alternative.
		// Parry is the same key pressed while Block is triggered. Its chord makes Enhanced Input block the light and
		// heavy mappings of that key on that frame; the abilities refuse to attack while Parrying.
		for (const FKey& AttackKey : { EKeys::LeftMouseButton, EKeys::Gamepad_RightTriggerAxis })
		{
			if (LightAttackAction)
			{
				FEnhancedActionKeyMapping& LightMapping = MordecaiIMC->MapKey(LightAttackAction, AttackKey);
				UInputTriggerTap* Tap = NewObject<UInputTriggerTap>(MordecaiIMC);
				Tap->TapReleaseTimeThreshold = HeavyAttackHoldSeconds;
				LightMapping.Triggers.Add(Tap);
			}
			if (HeavyAttackAction)
			{
				FEnhancedActionKeyMapping& HeavyMapping = MordecaiIMC->MapKey(HeavyAttackAction, AttackKey);
				UInputTriggerHoldAndRelease* Hold = NewObject<UInputTriggerHoldAndRelease>(MordecaiIMC);
				Hold->HoldTimeThreshold = HeavyAttackHoldSeconds;
				HeavyMapping.Triggers.Add(Hold);
			}
			if (BlockAction)
			{
				FEnhancedActionKeyMapping& ParryMapping = MordecaiIMC->MapKey(ParryAction, AttackKey);
				UInputTriggerChordAction* WhileBlocking = NewObject<UInputTriggerChordAction>(MordecaiIMC);
				WhileBlocking->ChordAction = BlockAction;
				ParryMapping.Triggers.Add(WhileBlocking);
				// A fresh press only: neither a held button nor a block raised while it is held parries
				ParryMapping.Triggers.Add(NewObject<UInputTriggerPressed>(MordecaiIMC));
			}
		}

		// Block: RMB / LT held. Down also makes an analog trigger count only past its 0.5 actuation threshold.
		if (BlockAction)
		{
			for (const FKey& BlockKey : { EKeys::RightMouseButton, EKeys::Gamepad_LeftTriggerAxis })
			{
				FEnhancedActionKeyMapping& BlockMapping = MordecaiIMC->MapKey(BlockAction, BlockKey);
				BlockMapping.Triggers.Add(NewObject<UInputTriggerDown>(MordecaiIMC));
			}
		}

		// Dodge: Left Alt / Gamepad B, once per press (a held key does not dodge again when the cooldown ends).
		// Gamepad B is also CommonUI's Back, but menus run in Menu input mode, which blocks game input.
		if (DodgeAction)
		{
			for (const FKey& DodgeKey : { EKeys::LeftAlt, EKeys::Gamepad_FaceButton_Right })
			{
				FEnhancedActionKeyMapping& DodgeMapping = MordecaiIMC->MapKey(DodgeAction, DodgeKey);
				DodgeMapping.Triggers.Add(NewObject<UInputTriggerPressed>(MordecaiIMC));
			}
		}

		RuntimeMappingContext = MordecaiIMC;
	}

	// A second call (the pawn possessed again) rebinds instead of stacking a second set of bindings: a doubled sprint
	// toggle would cancel itself
	EIC->ClearBindingsForObject(this);
	if (AActor* Owner = GetOwner())
	{
		EIC->ClearBindingsForObject(Owner);
	}

	// Bind input actions to handlers
	EIC->BindAction(MoveAction, ETriggerEvent::Triggered, this, &UMordecaiHeroComponent::Input_Move);
	if (LookMouseAction)
	{
		EIC->BindAction(LookMouseAction, ETriggerEvent::Triggered, this, &UMordecaiHeroComponent::Input_LookMouse);
	}
	if (LookStickAction)
	{
		EIC->BindAction(LookStickAction, ETriggerEvent::Triggered, this, &UMordecaiHeroComponent::Input_LookStick);
	}

	// Bind spell input actions (AC-057.5)
	if (Spell1Action)
	{
		EIC->BindAction(Spell1Action, ETriggerEvent::Started, this, &UMordecaiHeroComponent::HandleSpell1Input);
	}
	if (Spell2Action)
	{
		EIC->BindAction(Spell2Action, ETriggerEvent::Started, this, &UMordecaiHeroComponent::HandleSpell2Input);
	}
	if (Spell3Action)
	{
		EIC->BindAction(Spell3Action, ETriggerEvent::Started, this, &UMordecaiHeroComponent::HandleSpell3Input);
	}
	if (Spell4Action)
	{
		EIC->BindAction(Spell4Action, ETriggerEvent::Started, this, &UMordecaiHeroComponent::HandleSpell4Input);
	}

	// Bind weapon cycling input actions (US-078)
	if (WeaponCycleNextAction)
	{
		EIC->BindAction(WeaponCycleNextAction, ETriggerEvent::Started, this, &UMordecaiHeroComponent::HandleWeaponCycleNextInput);
	}
	if (WeaponCyclePrevAction)
	{
		EIC->BindAction(WeaponCyclePrevAction, ETriggerEvent::Started, this, &UMordecaiHeroComponent::HandleWeaponCyclePrevInput);
	}

	// Bind pickup + menu input actions (US-079)
	if (InteractAction)
	{
		EIC->BindAction(InteractAction, ETriggerEvent::Started, this, &UMordecaiHeroComponent::HandleInteractInput);
	}
	if (PauseAction)
	{
		EIC->BindAction(PauseAction, ETriggerEvent::Started, this, &UMordecaiHeroComponent::HandlePauseInput);
	}
	EIC->BindAction(InventoryAction, ETriggerEvent::Started, this, &UMordecaiHeroComponent::HandleInventoryInput);

	// --- US-083: Combat abilities activate through Lyra's input path: a press puts the ability's Mordecai.Input.* tag
	// on the ASC (ULyraAbilitySystemComponent::AbilityInputTagPressed), ProcessAbilityInput activates the spec carrying
	// it, and a release reaches the active instance (Block ends on it). Light is bound before heavy, so if a release
	// lands exactly on the hold time and both trigger, the light attack activates first and the heavy one is refused.
	auto BindAbilityInput = [this, EIC](const UInputAction* Action, ETriggerEvent PressEvent, const FGameplayTag& InputTag)
	{
		if (!Action)
		{
			return;
		}
		EIC->BindAction(Action, PressEvent, this, &UMordecaiHeroComponent::HandleAbilityInputPressed, InputTag);
		EIC->BindAction(Action, ETriggerEvent::Completed, this, &UMordecaiHeroComponent::HandleAbilityInputReleased, InputTag);
		EIC->BindAction(Action, ETriggerEvent::Canceled, this, &UMordecaiHeroComponent::HandleAbilityInputReleased, InputTag);
	};
	BindAbilityInput(LightAttackAction, ETriggerEvent::Triggered, MordecaiGameplayTags::Input_LightAttack);
	BindAbilityInput(HeavyAttackAction, ETriggerEvent::Triggered, MordecaiGameplayTags::Input_HeavyAttack);
	BindAbilityInput(BlockAction, ETriggerEvent::Triggered, MordecaiGameplayTags::Input_Block);
	// Parry presses on Triggered: only the frame a fresh LMB / RT press (Pressed) lands while Block is triggered.
	// Started would also fire when the block is raised over a held or partly pulled button: the chord alone makes the
	// mapping Ongoing (None -> Ongoing is Started).
	BindAbilityInput(ParryAction, ETriggerEvent::Triggered, MordecaiGameplayTags::Input_Parry);
	BindAbilityInput(DodgeAction, ETriggerEvent::Triggered, MordecaiGameplayTags::Input_Dodge);

	// --- US-083: Jump (Space / Gamepad A) and the sprint toggle (Left Shift; the L3 key name is US-097's) ---
	if (ACharacter* OwnerCharacter = Cast<ACharacter>(GetOwner()))
	{
		if (JumpAction)
		{
			EIC->BindAction(JumpAction, ETriggerEvent::Started, OwnerCharacter, &ACharacter::Jump);
			EIC->BindAction(JumpAction, ETriggerEvent::Completed, OwnerCharacter, &ACharacter::StopJumping);
		}
	}
	if (SprintAction)
	{
		// Started fires once per press, so a tap flips the toggle once
		EIC->BindAction(SprintAction, ETriggerEvent::Started, this, &UMordecaiHeroComponent::HandleSprintToggleInput);
	}

	UE_LOG(LogTemp, Warning, TEXT("MORDECAI: Programmatic IMC created and input bound successfully (including %d spell actions)."),
		(Spell1Action ? 1 : 0) + (Spell2Action ? 1 : 0) + (Spell3Action ? 1 : 0) + (Spell4Action ? 1 : 0));

	return RuntimeMappingContext;
}

void UMordecaiHeroComponent::InitializePlayerInput(UInputComponent* PlayerInputComponent)
{
	Super::InitializePlayerInput(PlayerInputComponent);

	// InputData_Mordecai tags IA_Mordecai_Sprint with InputTag.Crouch, so Super just bound the sprint key (Left Shift)
	// to Lyra's crouch toggle, firing every frame it is held: the player crouched (300 cm/s, no jumping) instead of
	// sprinting (N23). Mordecai has no crouch control; drop that binding. Sprint is bound in BuildAndBindRuntimeInput.
	const ULyraPawnExtensionComponent* PawnExt = ULyraPawnExtensionComponent::FindPawnExtensionComponent(GetOwner());
	const ULyraPawnData* PawnData = PawnExt ? PawnExt->GetPawnData<ULyraPawnData>() : nullptr;
	const int32 Removed = RemoveNativeBindingsForTag(Cast<UEnhancedInputComponent>(PlayerInputComponent),
		PawnData ? PawnData->InputConfig.Get() : nullptr, LyraGameplayTags::InputTag_Crouch, this);
	UE_LOG(LogTemp, Log, TEXT("MORDECAI: Removed %d Lyra crouch binding(s) from the pawn's input."), Removed);

	// Super cleared every mapping context; put MordecaiIMC_Runtime back if it was added before this ran
	if (RuntimeMappingContext)
	{
		const APlayerController* PC = GetController<APlayerController>();
		const ULocalPlayer* LP = PC ? PC->GetLocalPlayer() : nullptr;
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem = LP ? LP->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>() : nullptr)
		{
			Subsystem->AddMappingContext(RuntimeMappingContext, 2);
		}
	}
}

int32 UMordecaiHeroComponent::RemoveNativeBindingsForTag(UEnhancedInputComponent* EIC, const ULyraInputConfig* InputConfig,
	const FGameplayTag& InputTag, const UObject* BoundObject)
{
	if (!EIC || !InputConfig)
	{
		return 0;
	}
	const UInputAction* Action = InputConfig->FindNativeInputActionForTag(InputTag, /*bLogNotFound=*/ false);
	if (!Action)
	{
		return 0;
	}

	TArray<uint32> Handles;
	for (const TUniquePtr<FEnhancedInputActionEventBinding>& Binding : EIC->GetActionEventBindings())
	{
		if (Binding && Binding->GetAction() == Action && Binding->IsBoundToObject(BoundObject))
		{
			Handles.Add(Binding->GetHandle());
		}
	}
	for (const uint32 Handle : Handles)
	{
		EIC->RemoveBindingByHandle(Handle);
	}
	return Handles.Num();
}

void UMordecaiHeroComponent::SetAbilitySystemOverride(ULyraAbilitySystemComponent* InASC)
{
	AbilitySystemOverride = InASC;
}

// Unused stubs (movement is handled by base class Input_Move via the programmatic IMC with correct modifiers)
void UMordecaiHeroComponent::Input_MoveForward(const FInputActionValue& InputActionValue) {}
void UMordecaiHeroComponent::Input_MoveBackward(const FInputActionValue& InputActionValue) {}
void UMordecaiHeroComponent::Input_MoveRight(const FInputActionValue& InputActionValue) {}
void UMordecaiHeroComponent::Input_MoveLeft(const FInputActionValue& InputActionValue) {}

// ---------------------------------------------------------------------------
// Spell Input Handlers (AC-057.5)
// ---------------------------------------------------------------------------

void UMordecaiHeroComponent::HandleSpell1Input(const FInputActionValue& InputActionValue) { ActivateSpellBySlot(0); }
void UMordecaiHeroComponent::HandleSpell2Input(const FInputActionValue& InputActionValue) { ActivateSpellBySlot(1); }
void UMordecaiHeroComponent::HandleSpell3Input(const FInputActionValue& InputActionValue) { ActivateSpellBySlot(2); }
void UMordecaiHeroComponent::HandleSpell4Input(const FInputActionValue& InputActionValue) { ActivateSpellBySlot(3); }

void UMordecaiHeroComponent::ActivateSpellBySlot(int32 SlotIndex)
{
	APawn* OwnerPawn = Cast<APawn>(GetOwner());
	if (!OwnerPawn)
	{
		return;
	}

	AMordecaiPlayerController* MPC = Cast<AMordecaiPlayerController>(OwnerPawn->GetController());
	if (!MPC)
	{
		return;
	}

	TSubclassOf<UGameplayAbility> SpellClass = MPC->GetSpellAbilityClass(SlotIndex);
	if (!SpellClass)
	{
		return;
	}

	// Get ASC from PlayerState (Lyra pattern)
	UAbilitySystemComponent* ASC = nullptr;
	if (APlayerState* PS = MPC->PlayerState)
	{
		if (IAbilitySystemInterface* ASI = Cast<IAbilitySystemInterface>(PS))
		{
			ASC = ASI->GetAbilitySystemComponent();
		}
	}

	if (ASC)
	{
		ASC->TryActivateAbilityByClass(SpellClass);
	}
}

// ---------------------------------------------------------------------------
// US-083: Combat + Sprint Input Handlers
// ---------------------------------------------------------------------------

void UMordecaiHeroComponent::HandleAbilityInputPressed(FGameplayTag InputTag)
{
	if (ULyraAbilitySystemComponent* ASC = FindLyraAbilitySystemComponent())
	{
		ASC->AbilityInputTagPressed(InputTag);
	}
}

void UMordecaiHeroComponent::HandleAbilityInputReleased(FGameplayTag InputTag)
{
	if (ULyraAbilitySystemComponent* ASC = FindLyraAbilitySystemComponent())
	{
		ASC->AbilityInputTagReleased(InputTag);
	}
}

ULyraAbilitySystemComponent* UMordecaiHeroComponent::FindLyraAbilitySystemComponent() const
{
	if (AbilitySystemOverride)
	{
		return AbilitySystemOverride;
	}
	// The pawn's ability system (the player state's), as ULyraHeroComponent::Input_AbilityInputTagPressed finds it
	const ULyraPawnExtensionComponent* PawnExt = ULyraPawnExtensionComponent::FindPawnExtensionComponent(GetOwner());
	return PawnExt ? PawnExt->GetLyraAbilitySystemComponent() : nullptr;
}

void UMordecaiHeroComponent::HandleSprintToggleInput(const FInputActionValue& InputActionValue)
{
	if (AMordecaiCharacter* MC = Cast<AMordecaiCharacter>(GetOwner()))
	{
		MC->SetSprinting(!MC->bIsSprinting);
	}
}

// ---------------------------------------------------------------------------
// US-078: Weapon Cycling Input Handlers
// ---------------------------------------------------------------------------

void UMordecaiHeroComponent::HandleWeaponCycleNextInput(const FInputActionValue& InputActionValue)
{
	if (AMordecaiCharacter* MC = Cast<AMordecaiCharacter>(GetOwner()))
	{
		MC->CycleNextWeapon();
	}
}

void UMordecaiHeroComponent::HandleWeaponCyclePrevInput(const FInputActionValue& InputActionValue)
{
	if (AMordecaiCharacter* MC = Cast<AMordecaiCharacter>(GetOwner()))
	{
		MC->CyclePrevWeapon();
	}
}

// ---------------------------------------------------------------------------
// US-079: Pickup + Menu Input Handlers
// ---------------------------------------------------------------------------

void UMordecaiHeroComponent::HandleInteractInput(const FInputActionValue& InputActionValue)
{
	if (const AActor* Owner = GetOwner())
	{
		if (UMordecaiPickupInteractionComponent* Interaction = Owner->FindComponentByClass<UMordecaiPickupInteractionComponent>())
		{
			Interaction->TryPickupFocused();
		}
	}
}

void UMordecaiHeroComponent::HandlePauseInput(const FInputActionValue& InputActionValue)
{
	if (UMordecaiPauseMenuSubsystem* Subsystem = GetPauseMenuSubsystem())
	{
		Subsystem->TogglePauseMenu();
	}
}

void UMordecaiHeroComponent::HandleInventoryInput(const FInputActionValue& InputActionValue)
{
	if (UMordecaiPauseMenuSubsystem* Subsystem = GetPauseMenuSubsystem())
	{
		Subsystem->TogglePauseMenuToTab(UMordecaiInventoryWidget::GetInventoryTabId());
	}
}

UMordecaiPauseMenuSubsystem* UMordecaiHeroComponent::GetPauseMenuSubsystem() const
{
	const APawn* OwnerPawn = Cast<APawn>(GetOwner());
	UGameInstance* GameInstance = OwnerPawn ? OwnerPawn->GetGameInstance() : nullptr;
	return GameInstance ? GameInstance->GetSubsystem<UMordecaiPauseMenuSubsystem>() : nullptr;
}
