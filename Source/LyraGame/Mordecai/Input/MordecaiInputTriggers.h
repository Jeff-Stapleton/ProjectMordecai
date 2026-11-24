// Project Mordecai

#pragma once

#include "InputTriggers.h"

#include "MordecaiInputTriggers.generated.h"

/** A fresh main-key press triggers only while one configured modifier key is held. */
UCLASS(NotBlueprintable)
class LYRAGAME_API UMordecaiInputTriggerShiftPressed final : public UInputTrigger
{
	GENERATED_BODY()

public:
	UPROPERTY()
	TArray<FKey> ModifierKeys;

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

/** Blocks an otherwise valid mapping while one configured modifier key is held. */
UCLASS(NotBlueprintable)
class LYRAGAME_API UMordecaiInputTriggerShiftBlocker final : public UInputTrigger
{
	GENERATED_BODY()

public:
	UPROPERTY()
	TArray<FKey> ModifierKeys;

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

/** Fires only for the physical press event of one configured key. */
UCLASS(NotBlueprintable)
class LYRAGAME_API UMordecaiInputTriggerFreshKeyPress final : public UInputTrigger
{
	GENERATED_BODY()

public:
	UPROPERTY()
	FKey Key;

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
