// Project Mordecai

#pragma once

#include "Input/CommonAnalogCursor.h"

enum class ECommonInputMode : uint8;

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
