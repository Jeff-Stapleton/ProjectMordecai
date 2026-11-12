// Project Mordecai — shared owning-player theme subscription helpers.

#pragma once

#include "Mordecai/UI/TinTally/MordecaiUIThemeSubsystem.h"

namespace MordecaiUIThemeBinding
{
	template <typename WidgetType>
	void Bind(
		UUserWidget* Widget,
		TWeakObjectPtr<UMordecaiUIThemeSubsystem>& BoundSubsystem,
		FDelegateHandle& Handle,
		WidgetType* Receiver,
		void (WidgetType::*Callback)(EMordecaiUITheme, const FMordecaiUIThemeColors&))
	{
		UMordecaiUIThemeSubsystem* Subsystem =
			UMordecaiUIThemeSubsystem::ResolveForWidget(Widget);
		if (Subsystem && (!Handle.IsValid() || BoundSubsystem.Get() != Subsystem))
		{
			if (UMordecaiUIThemeSubsystem* Previous = BoundSubsystem.Get())
			{
				Previous->OnThemeChangedNative().Remove(Handle);
			}
			BoundSubsystem = Subsystem;
			Handle = Subsystem->OnThemeChangedNative().AddUObject(Receiver, Callback);
		}
		if (Subsystem)
		{
			(Receiver->*Callback)(Subsystem->GetActiveTheme(), Subsystem->GetActiveColors());
		}
	}

	inline void Unbind(
		TWeakObjectPtr<UMordecaiUIThemeSubsystem>& BoundSubsystem,
		FDelegateHandle& Handle)
	{
		if (UMordecaiUIThemeSubsystem* Subsystem = BoundSubsystem.Get())
		{
			Subsystem->OnThemeChangedNative().Remove(Handle);
		}
		Handle.Reset();
		BoundSubsystem.Reset();
	}
}
