// Project Mordecai

#pragma once

#include "Input/CommonAnalogCursor.h"
#include "Input/CommonUIActionRouterBase.h"

#include "MordecaiCommonAnalogCursor.generated.h"

/**
 * CommonUI cursor policy for Mordecai gameplay.
 *
 * The project keeps this type separate from the action-router subsystem so the
 * input-mode policy has direct headless coverage.
 */
class FMordecaiCommonAnalogCursor : public FCommonAnalogCursor
{
public:
	explicit FMordecaiCommonAnalogCursor(const UCommonUIActionRouterBase& InActionRouter);

	static bool ShouldSimulateVirtualAccept(ECommonInputMode InputMode);

	virtual bool ShouldVirtualAcceptSimulateMouseButton(
		const FKeyEvent& InKeyEvent,
		EInputEvent InputEvent) const override;
};

/** Installs Mordecai's analog-cursor policy for each local player. */
UCLASS()
class LYRAGAME_API UMordecaiCommonUIActionRouter : public UCommonUIActionRouterBase
{
	GENERATED_BODY()

public:
	/** Read-only Tier 2 support: whether the live input mode would turn Gamepad A into a UI click. */
	UFUNCTION(BlueprintPure, Category = "Mordecai|Input")
	bool ShouldSimulateVirtualAcceptForActiveInputMode() const;

protected:
	virtual TSharedRef<FCommonAnalogCursor> MakeAnalogCursor() const override;
};
