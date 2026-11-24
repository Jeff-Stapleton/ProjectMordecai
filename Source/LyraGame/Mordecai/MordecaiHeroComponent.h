// Project Mordecai

#pragma once

#include "Character/LyraHeroComponent.h"

#include "MordecaiHeroComponent.generated.h"

class UEnhancedInputComponent;
class UInputAction;
class UInputMappingContext;
class ULyraAbilitySystemComponent;
class ULyraInputConfig;

/**
 * UMordecaiHeroComponent
 *
 *	Extends LyraHeroComponent to set Mordecai-specific DefaultInputMappings
 *	and handle movement input directly (bypassing broken IMC modifiers).
 */
UCLASS()
class LYRAGAME_API UMordecaiHeroComponent : public ULyraHeroComponent
{
	GENERATED_BODY()

public:
	UMordecaiHeroComponent(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	/** Ensure IMC_Mordecai is active and native input actions are bound.
	 *  Called from MordecaiCharacter::SetupPlayerInputComponent. */
	void EnsureMordecaiInputBound(APlayerController* PC);

	/**
	 * Builds MordecaiIMC_Runtime (once) and binds its actions on EIC; EnsureMordecaiInputBound then adds the context to
	 * the local player. Needs no LocalPlayer, so tests can call it. Calling it again rebinds instead of stacking a second
	 * set of bindings. Returns null if the input actions cannot load.
	 */
	UInputMappingContext* BuildAndBindRuntimeInput(UEnhancedInputComponent* EIC);

	/** The runtime mapping context (MordecaiIMC_Runtime), or null before it is built. */
	UFUNCTION(BlueprintPure, Category = "Mordecai|Input")
	UInputMappingContext* GetRuntimeMappingContext() const { return RuntimeMappingContext; }

	/**
	 * Hold time (s) that splits LMB / RT presses (US-083): released sooner is a light attack, held this long a heavy
	 * attack on release.
	 * TODO(DECISION): control_bindings_v1.1 says "charge time depends on weapon"; 0.4 s for every weapon until decided.
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Mordecai|Input")
	float HeavyAttackHoldSeconds = 0.4f;

	/** Radial right-stick magnitude below which the last aim-facing direction is retained. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Mordecai|Input", Meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float AimStickDeadzone = 0.2f;

	/**
	 * Removes EIC's action bindings to BoundObject for InputConfig's native action of InputTag. Returns how many were
	 * removed. Used to drop the crouch binding Lyra makes from InputData_Mordecai (InitializePlayerInput).
	 */
	static int32 RemoveNativeBindingsForTag(UEnhancedInputComponent* EIC, const ULyraInputConfig* InputConfig,
		const FGameplayTag& InputTag, const UObject* BoundObject);

	/** Use this ability system for combat input instead of the pawn's (testing). */
	void SetAbilitySystemOverride(ULyraAbilitySystemComponent* InASC);

protected:
	//~ ULyraHeroComponent interface
	virtual void InitializePlayerInput(UInputComponent* PlayerInputComponent) override;
	//~ End ULyraHeroComponent interface

	//~ UActorComponent interface
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	//~ End UActorComponent interface

private:
	// Fixed-camera input handlers. Movement stays in the yaw-zero camera basis while aim rotates only the pawn.
	UFUNCTION()
	void Input_Move(const FInputActionValue& InputActionValue);

	UFUNCTION()
	void Input_LookMouse(const FInputActionValue& InputActionValue);

	UFUNCTION()
	void Input_LookStick(const FInputActionValue& InputActionValue);

	/** Project a deprojected cursor ray onto the horizontal plane at the character's height. */
	UFUNCTION()
	bool ApplyMouseAimFromRay(FVector WorldOrigin, FVector WorldDirection);

	/** Apply right-stick aim in the fixed-camera basis; false inside the deadzone preserves last facing. */
	UFUNCTION()
	bool ApplyGamepadAim(FVector2D StickInput);

	/** Deproject the live OS cursor for local mouse aim and update cursor visibility for the last-used device. */
	void UpdateLocalMouseAim();

	// Spell input handlers (AC-057.5) — each activates the spell in the corresponding slot
	void HandleSpell1Input(const FInputActionValue& InputActionValue);
	void HandleSpell2Input(const FInputActionValue& InputActionValue);
	void HandleSpell3Input(const FInputActionValue& InputActionValue);
	void HandleSpell4Input(const FInputActionValue& InputActionValue);
	void ActivateSpellBySlot(int32 SlotIndex);

	// Weapon cycling handlers (US-078)
	void HandleWeaponCycleNextInput(const FInputActionValue& InputActionValue);
	void HandleWeaponCyclePrevInput(const FInputActionValue& InputActionValue);
	void HandleWeaponCyclePrevShiftTabInput(const FInputActionValue& InputActionValue);

	// Pickup + menu handlers (US-079)
	void HandleInteractInput(const FInputActionValue& InputActionValue);
	void HandlePauseInput(const FInputActionValue& InputActionValue);
	void HandleInventoryInput(const FInputActionValue& InputActionValue);
	class UMordecaiPauseMenuSubsystem* GetPauseMenuSubsystem() const;

	// Combat input (US-083): press / release a Mordecai.Input.* tag on the Lyra ASC, which activates the spec carrying it
	void HandleAbilityInputPressed(FGameplayTag InputTag);
	void HandleAbilityInputReleased(FGameplayTag InputTag);
	ULyraAbilitySystemComponent* FindLyraAbilitySystemComponent() const;

	// Sprint toggle (US-083)
	void HandleSprintToggleInput(const FInputActionValue& InputActionValue);
	void HandleSprintKeyboardPressed(const FInputActionValue& InputActionValue);
	void HandleSprintKeyboardReleased(const FInputActionValue& InputActionValue);
	void ResetSprintKeyboardPressState();

	/** MordecaiIMC_Runtime, built once by BuildAndBindRuntimeInput. */
	UPROPERTY(Transient)
	TObjectPtr<UInputMappingContext> RuntimeMappingContext;

	/** Input actions with no asset, created with the runtime context. */
	UPROPERTY(Transient)
	TObjectPtr<UInputAction> InventoryAction;

	UPROPERTY(Transient)
	TObjectPtr<UInputAction> ParryAction;

	UPROPERTY(Transient)
	TObjectPtr<UInputAction> WeaponCyclePrevShiftTabAction;

	UPROPERTY(Transient)
	TObjectPtr<UInputAction> SprintKeyboardPressAction;

	UPROPERTY(Transient)
	TObjectPtr<UInputAction> SprintKeyboardReleaseAction;

	UPROPERTY(Transient)
	TObjectPtr<ULyraAbilitySystemComponent> AbilitySystemOverride;

	/** A rejected held Block cannot retry every Triggered frame; release arms the next press. */
	bool bBlockSuppressedUntilRelease = false;

	/** State owned only by the current physical keyboard sprint-key press. */
	bool bSprintKeyboardPressActive = false;
	bool bSprintKeyboardChordConsumed = false;
	bool bSprintStateBeforeKeyboardPress = false;

	/** Lets a chord callback suppress a keyboard sprint callback later in the same engine frame. */
	uint64 LastShiftTabChordFrame = MAX_uint64;
};
