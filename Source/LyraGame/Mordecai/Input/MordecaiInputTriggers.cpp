// Project Mordecai

#include "MordecaiInputTriggers.h"

#include "EnhancedPlayerInput.h"
#include "InputCoreTypes.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(MordecaiInputTriggers)

namespace
{
	bool IsAnyModifierPressed(
		const UEnhancedPlayerInput* PlayerInput,
		const TArray<FKey>& ModifierKeys)
	{
		if (!PlayerInput)
		{
			return false;
		}
		for (const FKey& Key : ModifierKeys)
		{
			if (PlayerInput->IsPressed(Key))
			{
				return true;
			}
		}
		return false;
	}
}

ETriggerState UMordecaiInputTriggerShiftPressed::UpdateState_Implementation(
	const UEnhancedPlayerInput* PlayerInput,
	FInputActionValue ModifiedValue,
	float DeltaTime)
{
	if (!IsAnyModifierPressed(PlayerInput, ModifierKeys))
	{
		return ETriggerState::None;
	}
	return IsActuated(ModifiedValue) && !IsActuated(LastValue)
		? ETriggerState::Triggered
		: ETriggerState::None;
}

ETriggerState UMordecaiInputTriggerShiftBlocker::UpdateState_Implementation(
	const UEnhancedPlayerInput* PlayerInput,
	FInputActionValue ModifiedValue,
	float DeltaTime)
{
	return IsAnyModifierPressed(PlayerInput, ModifierKeys)
		? ETriggerState::Triggered
		: ETriggerState::None;
}

ETriggerState UMordecaiInputTriggerFreshKeyPress::UpdateState_Implementation(
	const UEnhancedPlayerInput* PlayerInput,
	FInputActionValue ModifiedValue,
	float DeltaTime)
{
	return PlayerInput
		&& Key.IsValid()
		&& PlayerInput->WasJustPressed(Key)
		&& IsActuated(ModifiedValue)
		? ETriggerState::Triggered
		: ETriggerState::None;
}
