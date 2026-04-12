// Project Mordecai

#pragma once

#include "Character/LyraHeroComponent.h"

#include "MordecaiHeroComponent.generated.h"

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
};
