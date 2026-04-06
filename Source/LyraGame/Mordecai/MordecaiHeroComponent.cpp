// Project Mordecai

#include "MordecaiHeroComponent.h"
#include "GameFeatures/GameFeatureAction_AddInputContextMapping.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputAction.h"
#include "InputMappingContext.h"
#include "InputModifiers.h"
#include "GameFramework/PlayerController.h"

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

	UE_LOG(LogTemp, Warning, TEXT("MORDECAI: Programmatic IMC created and input bound successfully."));
}

// Unused stubs (movement is handled by base class Input_Move via the programmatic IMC with correct modifiers)
void UMordecaiHeroComponent::Input_MoveForward(const FInputActionValue& InputActionValue) {}
void UMordecaiHeroComponent::Input_MoveBackward(const FInputActionValue& InputActionValue) {}
void UMordecaiHeroComponent::Input_MoveRight(const FInputActionValue& InputActionValue) {}
void UMordecaiHeroComponent::Input_MoveLeft(const FInputActionValue& InputActionValue) {}
