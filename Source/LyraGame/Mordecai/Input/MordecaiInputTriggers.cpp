// Project Mordecai

#include "MordecaiInputTriggers.h"

#include "EnhancedPlayerInput.h"
#include "InputCoreTypes.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(MordecaiInputTriggers)

ETriggerState UMordecaiInputTriggerShiftPressed::UpdateState_Implementation(
	const UEnhancedPlayerInput* PlayerInput,
	FInputActionValue ModifiedValue,
	float DeltaTime)
{
	if (!PlayerInput || !PlayerInput->IsPressed(EKeys::LeftShift))
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
	return PlayerInput && PlayerInput->IsPressed(EKeys::LeftShift)
		? ETriggerState::Triggered
		: ETriggerState::None;
}
