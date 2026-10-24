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

private:
	// Direct WASD handlers that bypass IMC modifiers (which can't be serialized via Python)
	void Input_MoveForward(const FInputActionValue& InputActionValue);
	void Input_MoveBackward(const FInputActionValue& InputActionValue);
	void Input_MoveRight(const FInputActionValue& InputActionValue);
	void Input_MoveLeft(const FInputActionValue& InputActionValue);

	// Spell input handlers (AC-057.5) — each activates the spell in the corresponding slot
	void HandleSpell1Input(const FInputActionValue& InputActionValue);
	void HandleSpell2Input(const FInputActionValue& InputActionValue);
	void HandleSpell3Input(const FInputActionValue& InputActionValue);
	void HandleSpell4Input(const FInputActionValue& InputActionValue);
	void ActivateSpellBySlot(int32 SlotIndex);

	// Weapon cycling handlers (US-078)
	void HandleWeaponCycleNextInput(const FInputActionValue& InputActionValue);
	void HandleWeaponCyclePrevInput(const FInputActionValue& InputActionValue);

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

	/** MordecaiIMC_Runtime, built once by BuildAndBindRuntimeInput. */
	UPROPERTY(Transient)
	TObjectPtr<UInputMappingContext> RuntimeMappingContext;

	/** Input actions with no asset, created with the runtime context. */
	UPROPERTY(Transient)
	TObjectPtr<UInputAction> InventoryAction;

	UPROPERTY(Transient)
	TObjectPtr<UInputAction> ParryAction;

	UPROPERTY(Transient)
	TObjectPtr<ULyraAbilitySystemComponent> AbilitySystemOverride;
};
