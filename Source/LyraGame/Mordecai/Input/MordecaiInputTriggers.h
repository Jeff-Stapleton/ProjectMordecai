// Project Mordecai

#pragma once

#include "InputTriggers.h"

#include "MordecaiInputTriggers.generated.h"

/** A fresh main-key press triggers only while keyboard Left Shift is held. */
UCLASS(NotBlueprintable)
class LYRAGAME_API UMordecaiInputTriggerShiftPressed final : public UInputTrigger
{
	GENERATED_BODY()

public:
	virtual ETriggerEventsSupported GetSupportedTriggerEvents() const override
	{
		return ETriggerEventsSupported::Instant;
	}

protected:
	virtual ETriggerState UpdateState_Implementation(
		const UEnhancedPlayerInput* PlayerInput,
		FInputActionValue ModifiedValue,
		float DeltaTime) override;
};

/** Blocks an otherwise valid mapping while keyboard Left Shift is held. */
UCLASS(NotBlueprintable)
class LYRAGAME_API UMordecaiInputTriggerShiftBlocker final : public UInputTrigger
{
	GENERATED_BODY()

protected:
	virtual ETriggerType GetTriggerType_Implementation() const override
	{
		return ETriggerType::Blocker;
	}

	virtual ETriggerState UpdateState_Implementation(
		const UEnhancedPlayerInput* PlayerInput,
		FInputActionValue ModifiedValue,
		float DeltaTime) override;

	virtual bool IsBlocking(ETriggerState State) const override
	{
		return State == ETriggerState::Triggered;
	}
};
