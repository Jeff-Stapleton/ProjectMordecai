// Project Mordecai

#include "MordecaiHeroComponent.h"
#include "Mordecai/MordecaiCharacter.h"
#include "Mordecai/MordecaiPlayerController.h"
#include "GameFeatures/GameFeatureAction_AddInputContextMapping.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemInterface.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputAction.h"
#include "InputMappingContext.h"
#include "InputModifiers.h"
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
		return;
	}

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

	// --- AC-057.5: Spell Input Actions (Q/LB, E/RB, R, 1/DPadUp) ---
	UInputAction* Spell1Action = LoadAction(TEXT("/MordecaiCore/Input/Actions/IA_Mordecai_Spell1.IA_Mordecai_Spell1"));
	UInputAction* Spell2Action = LoadAction(TEXT("/MordecaiCore/Input/Actions/IA_Mordecai_Spell2.IA_Mordecai_Spell2"));
	UInputAction* Spell3Action = LoadAction(TEXT("/MordecaiCore/Input/Actions/IA_Mordecai_Spell3.IA_Mordecai_Spell3"));
	UInputAction* Spell4Action = LoadAction(TEXT("/MordecaiCore/Input/Actions/IA_Mordecai_Spell4.IA_Mordecai_Spell4"));

	// --- US-078: Weapon Cycling Input Actions (Tab / Shift+Tab / Gamepad Y) ---
	UInputAction* WeaponCycleNextAction = LoadAction(TEXT("/MordecaiCore/Input/Actions/IA_Mordecai_WeaponSwap.IA_Mordecai_WeaponSwap"));
	UInputAction* WeaponCyclePrevAction = LoadAction(TEXT("/MordecaiCore/Input/Actions/IA_Mordecai_WeaponCyclePrev.IA_Mordecai_WeaponCyclePrev"));

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

	// Add the programmatic IMC to the subsystem
	Subsystem->AddMappingContext(MordecaiIMC, 2); // Priority 2 to override any broken .uasset IMC

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

	UE_LOG(LogTemp, Warning, TEXT("MORDECAI: Programmatic IMC created and input bound successfully (including %d spell actions)."),
		(Spell1Action ? 1 : 0) + (Spell2Action ? 1 : 0) + (Spell3Action ? 1 : 0) + (Spell4Action ? 1 : 0));
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
