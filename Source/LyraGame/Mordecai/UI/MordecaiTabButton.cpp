// Project Mordecai — Id-Payload Button (US-079)

#include "Mordecai/UI/MordecaiTabButton.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(MordecaiTabButton)

void UMordecaiTabButton::InitButton(FName InButtonId)
{
	ButtonId = InButtonId;
	if (!bClickRelayBound)
	{
		OnClicked.AddDynamic(this, &UMordecaiTabButton::HandleClicked);
		bClickRelayBound = true;
	}
}

void UMordecaiTabButton::HandleClicked()
{
	OnClickedWithId.Broadcast(ButtonId);
}
