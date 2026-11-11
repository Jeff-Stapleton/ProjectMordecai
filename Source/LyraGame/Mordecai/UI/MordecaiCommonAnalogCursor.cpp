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
	// In Game mode, virtual Accept is also a gameplay button (Gamepad A -> Jump). CommonUI's
	// simulated click consumes it before Enhanced Input. Menus still need the ordinary UI click.
	return InputMode != ECommonInputMode::Game;
}

bool FMordecaiCommonAnalogCursor::ShouldVirtualAcceptSimulateMouseButton(
	const FKeyEvent& InKeyEvent,
	const EInputEvent InputEvent) const
{
	return ShouldSimulateVirtualAccept(ActionRouter.GetActiveInputMode());
}

bool UMordecaiCommonUIActionRouter::ShouldSimulateVirtualAcceptForActiveInputMode() const
{
	return FMordecaiCommonAnalogCursor::ShouldSimulateVirtualAccept(GetActiveInputMode());
}

TSharedRef<FCommonAnalogCursor> UMordecaiCommonUIActionRouter::MakeAnalogCursor() const
{
	return FCommonAnalogCursor::CreateAnalogCursor<FMordecaiCommonAnalogCursor>(*this);
}
