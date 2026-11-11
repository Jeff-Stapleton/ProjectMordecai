// Project Mordecai

#include "Mordecai/UI/MordecaiCommonAnalogCursor.h"

#include "CommonInputTypeEnum.h"
#include "Input/CommonUIActionRouterBase.h"

FMordecaiCommonAnalogCursor::FMordecaiCommonAnalogCursor(
	const UCommonUIActionRouterBase& InActionRouter)
	: FCommonAnalogCursor(InActionRouter)
{
}

bool FMordecaiCommonAnalogCursor::ShouldSimulateVirtualAccept(
	const ECommonInputMode InputMode)
{
	// RED scaffold: this is CommonUI's current behavior in every input mode.
	return true;
}

bool FMordecaiCommonAnalogCursor::ShouldVirtualAcceptSimulateMouseButton(
	const FKeyEvent& InKeyEvent,
	const EInputEvent InputEvent) const
{
	return ShouldSimulateVirtualAccept(ActionRouter.GetActiveInputMode());
}
