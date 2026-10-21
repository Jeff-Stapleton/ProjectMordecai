// Project Mordecai — Agent bridge input injection (Specs/agent_bridge_v1.md §1, §4)

#include "Mordecai/Playtest/MordecaiAgentBridgeInput.h"

#if !UE_BUILD_SHIPPING

#include "Engine/GameInstance.h"
#include "Engine/GameViewportClient.h"
#include "Engine/LocalPlayer.h"
#include "Framework/Application/SlateApplication.h"
#include "Framework/Application/SlateUser.h"
#include "GenericPlatform/GenericPlatformInputDeviceMapper.h"
#include "Input/CommonUIActionRouterBase.h"
#include "Input/Events.h"
#include "Layout/WidgetPath.h"
#include "Slate/SceneViewport.h"
#include "Widgets/SViewport.h"
#include "Widgets/SWindow.h"

namespace MordecaiAgentBridgeInputPrivate
{
	/**
	 * Read-only access to two FSlateUser accessors that are SLATE_SCOPE (protected outside the Slate
	 * module): the user's cursor and its weak focus path. A pointer to the inherited member, formed
	 * through a derived class name (C++ [class.protected]), calls them on any FSlateUser. Never
	 * instantiated; nothing in Slate is changed.
	 */
	struct FSlateUserReadAccess : public FSlateUser
	{
		FSlateUserReadAccess() = delete;
		FSlateUserReadAccess(const FSlateUserReadAccess&) = delete;
		FSlateUserReadAccess& operator=(const FSlateUserReadAccess&) = delete;

		static TSharedPtr<ICursor> CursorOf(const FSlateUser& User)
		{
			TSharedPtr<ICursor> (FSlateUser::*Getter)() const = &FSlateUserReadAccess::GetCursor;
			return (User.*Getter)();
		}

		static const FWeakWidgetPath& WeakFocusPathOf(const FSlateUser& User)
		{
			const FWeakWidgetPath& (FSlateUser::*Getter)() const = &FSlateUserReadAccess::GetWeakFocusPath;
			return (User.*Getter)();
		}
	};

	/** The faux cursor starts at INT_MIN; treat anything that far out as "no position yet". */
	static bool IsUsableCursorPosition(const FVector2D& Position)
	{
		return FMath::Abs(Position.X) < 1.0e8 && FMath::Abs(Position.Y) < 1.0e8;
	}

	/** The CommonUI router of the game's first local player is in Menu input mode. */
	static bool IsRouterInMenuMode(const UGameViewportClient* ViewportClient)
	{
		const UGameInstance* GameInstance = ViewportClient ? ViewportClient->GetGameInstance() : nullptr;
		const ULocalPlayer* LocalPlayer = GameInstance ? GameInstance->GetFirstGamePlayer() : nullptr;
		const UCommonUIActionRouterBase* Router = LocalPlayer ? LocalPlayer->GetSubsystem<UCommonUIActionRouterBase>() : nullptr;
		return Router && Router->GetActiveInputMode() == ECommonInputMode::Menu;
	}

	static FGeometry ViewportGeometry(UGameViewportClient* ViewportClient)
	{
		const TSharedPtr<SViewport> Widget = ViewportClient ? ViewportClient->GetGameViewportWidget() : nullptr;
		return Widget.IsValid() ? Widget->GetTickSpaceGeometry() : FGeometry();
	}
}

TSharedRef<FJsonObject> FMordecaiAgentBridgeEnv::ToJson() const
{
	const TSharedRef<FJsonObject> Json = MakeShared<FJsonObject>();
	Json->SetBoolField(TEXT("focus_restored"), bFocusRestored);
	Json->SetBoolField(TEXT("virtual_cursor"), bVirtualCursor);
	Json->SetBoolField(TEXT("app_active"), bAppActive);
	return Json;
}

bool FMordecaiAgentBridgeInput::IsReady(const UGameViewportClient* ViewportClient, FString& OutReason)
{
	if (!FSlateApplication::IsInitialized())
	{
		OutReason = TEXT("Slate is not initialized yet");
		return false;
	}
	if (!ViewportClient || !ViewportClient->Viewport || !ViewportClient->GetGameViewportWidget().IsValid())
	{
		OutReason = TEXT("no game viewport yet");
		return false;
	}
	return true;
}

FMordecaiAgentBridgeEnv FMordecaiAgentBridgeInput::PrepareEnvironment(MordecaiAgentBridge::ERoute Route, const UGameViewportClient* ViewportClient)
{
	FMordecaiAgentBridgeEnv Env;
	FSlateApplication& Slate = FSlateApplication::Get();

	// Inactive apps otherwise drop mouse capture and preprocessor-relevant input
	Slate.SetHandleDeviceInputWhenApplicationNotActive(true);

	// Only when the focus path is genuinely empty (a game launched on a locked desktop), and never while
	// a CommonUI menu is up: a dead leaf still routes keys along the live part of the path for a player
	if (MordecaiAgentBridge::ShouldRestoreUserFocus(Route, HasLiveFocusPath(), MordecaiAgentBridgeInputPrivate::IsRouterInMenuMode(ViewportClient)))
	{
		Slate.SetAllUserFocusToGameViewport();
		Env.bFocusRestored = true;
	}
	Env.bVirtualCursor = IsVirtualCursorActive();
	Env.bAppActive = Slate.IsActive();
	return Env;
}

bool FMordecaiAgentBridgeInput::HasLiveFocusPath()
{
	if (!FSlateApplication::IsInitialized())
	{
		return false;
	}
	FSlateApplication& Slate = FSlateApplication::Get();
	const TSharedPtr<FSlateUser> User = Slate.GetUser(Slate.GetUserIndexForKeyboard());
	if (!User.IsValid())
	{
		return false;
	}
	const FWeakWidgetPath& Path = MordecaiAgentBridgeInputPrivate::FSlateUserReadAccess::WeakFocusPathOf(*User);
	for (const TWeakPtr<SWidget>& Widget : Path.Widgets)
	{
		if (Widget.IsValid())
		{
			return true;
		}
	}
	return false;
}

bool FMordecaiAgentBridgeInput::EnsureVirtualCursor()
{
	if (!FSlateApplication::IsInitialized())
	{
		return false;
	}
	// Idempotent in Slate: it only swaps while the cursor user still has the platform cursor
	FSlateApplication& Slate = FSlateApplication::Get();
	if (IsVirtualCursorActive())
	{
		return false;
	}
	const FVector2D Before = Slate.GetCursorPos();
	Slate.UsePlatformCursorForCursorUser(false);
	const bool bSwapped = IsVirtualCursorActive();
	if (bSwapped && MordecaiAgentBridgeInputPrivate::IsUsableCursorPosition(Before))
	{
		// A fresh in-memory cursor starts at INT_MIN: start it where the real one was, for a sane first delta
		Slate.SetCursorPos(Before);
	}
	return bSwapped;
}

void FMordecaiAgentBridgeInput::ReassertVirtualCursor(const FVector2D& Absolute)
{
	EnsureVirtualCursor();
	// Never move the human's OS cursor: only the in-memory one
	if (IsVirtualCursorActive())
	{
		FSlateApplication::Get().SetCursorPos(Absolute);
	}
}

bool FMordecaiAgentBridgeInput::IsVirtualCursorActive()
{
	if (!FSlateApplication::IsInitialized())
	{
		return false;
	}
	FSlateApplication& Slate = FSlateApplication::Get();
	const TSharedPtr<FSlateUser> CursorUser = Slate.GetCursorUser();
	if (!CursorUser.IsValid())
	{
		return false;
	}
	const TSharedPtr<ICursor> Cursor = MordecaiAgentBridgeInputPrivate::FSlateUserReadAccess::CursorOf(*CursorUser);
	return Cursor.IsValid() && Cursor != Slate.GetPlatformCursor();
}

bool FMordecaiAgentBridgeInput::SendKey(UGameViewportClient* ViewportClient, const FKey& Key, bool bDown, bool bRepeat, const FModifierKeysState& Modifiers, MordecaiAgentBridge::ERoute Route)
{
	if (!FSlateApplication::IsInitialized())
	{
		return false;
	}
	FSlateApplication& Slate = FSlateApplication::Get();

	// Build the event exactly as FSlateApplication::OnKeyDown / OnControllerButtonPressed do
	TOptional<FKeyEvent> Event;
	if (Key.IsGamepadKey())
	{
		const FInputDeviceId Device = IPlatformInputDeviceMapper::Get().GetDefaultInputDevice();
		Event.Emplace(Key, Modifiers, Device, bRepeat, 0u, 0u, Slate.GetUserIndexForInputDevice(Device));
	}
	else
	{
		const uint32* KeyCode = nullptr;
		const uint32* CharCode = nullptr;
		FInputKeyManager::Get().GetCodesFromKey(Key, KeyCode, CharCode);
		Event.Emplace(Key, Modifiers, static_cast<uint32>(Slate.GetUserIndexForKeyboard()), bRepeat,
			CharCode ? *CharCode : 0u, KeyCode ? *KeyCode : 0u);
	}

	if (Route == MordecaiAgentBridge::ERoute::Viewport)
	{
		FSceneViewport* SceneViewport = ViewportClient ? ViewportClient->GetGameViewport() : nullptr;
		if (!SceneViewport)
		{
			return false;
		}
		const FGeometry Geometry = MordecaiAgentBridgeInputPrivate::ViewportGeometry(ViewportClient);
		const FReply Reply = bDown ? SceneViewport->OnKeyDown(Geometry, Event.GetValue()) : SceneViewport->OnKeyUp(Geometry, Event.GetValue());
		return Reply.IsEventHandled();
	}
	return bDown ? Slate.ProcessKeyDownEvent(Event.GetValue()) : Slate.ProcessKeyUpEvent(Event.GetValue());
}

bool FMordecaiAgentBridgeInput::SendAnalog(UGameViewportClient* ViewportClient, const FKey& Key, float Value, const FModifierKeysState& Modifiers, MordecaiAgentBridge::ERoute Route)
{
	if (!FSlateApplication::IsInitialized())
	{
		return false;
	}
	FSlateApplication& Slate = FSlateApplication::Get();
	const FInputDeviceId Device = IPlatformInputDeviceMapper::Get().GetDefaultInputDevice();
	// As FSlateApplication::OnControllerAnalog builds it; FSceneViewport negates Gamepad_RightY itself
	const FAnalogInputEvent Event(Key, Modifiers, Device, false, 0u, 0u, Value, Slate.GetUserIndexForInputDevice(Device));

	if (Route == MordecaiAgentBridge::ERoute::Viewport)
	{
		FSceneViewport* SceneViewport = ViewportClient ? ViewportClient->GetGameViewport() : nullptr;
		if (!SceneViewport)
		{
			return false;
		}
		return SceneViewport->OnAnalogValueChanged(MordecaiAgentBridgeInputPrivate::ViewportGeometry(ViewportClient), Event).IsEventHandled();
	}
	return Slate.ProcessAnalogInputEvent(Event);
}

void FMordecaiAgentBridgeInput::MoveCursor(const FVector2D& Absolute, const FModifierKeysState& Modifiers)
{
	if (!FSlateApplication::IsInitialized())
	{
		return;
	}
	FSlateApplication& Slate = FSlateApplication::Get();
	FVector2D Last = Slate.GetCursorPos();
	if (!MordecaiAgentBridgeInputPrivate::IsUsableCursorPosition(Last))
	{
		Last = Absolute;
	}
	ReassertVirtualCursor(Absolute);
	const FPointerEvent Event(Slate.GetUserIndexForMouse(), FSlateApplicationBase::CursorPointerIndex,
		Absolute, Last, PressedMouseButtons, EKeys::Invalid, 0.f, Modifiers);
	Slate.ProcessMouseMoveEvent(Event);
	// A real move switches CommonInput to MouseAndKeyboard (and Slate to the OS cursor) when the last
	// input was a gamepad: put the in-memory cursor back where the move went
	ReassertVirtualCursor(Absolute);
}

FString FMordecaiAgentBridgeInput::PressMouseButton(const FKey& Button, const FVector2D& Absolute, const FModifierKeysState& Modifiers)
{
	if (!FSlateApplication::IsInitialized())
	{
		return FString();
	}
	FSlateApplication& Slate = FSlateApplication::Get();
	MoveCursor(Absolute, Modifiers);

	// What Slate's hit test finds at the click position (a covering widget wins, as for a player)
	FString Hit;
	const FWidgetPath Path = Slate.LocateWindowUnderMouse(Absolute, Slate.GetInteractiveTopLevelWindows(), false, Slate.GetUserIndexForMouse());
	if (Path.IsValid() && Path.Widgets.Num() > 0)
	{
		Hit = Path.Widgets.Last().Widget->GetTypeAsString();
	}

	PressedMouseButtons.Add(Button);
	const FPointerEvent Event(Slate.GetUserIndexForMouse(), FSlateApplicationBase::CursorPointerIndex,
		Absolute, Absolute, PressedMouseButtons, Button, 0.f, Modifiers);
	Slate.ProcessMouseButtonDownEvent(nullptr, Event);
	// The down's CommonInput preprocessor may have switched the input method to MouseAndKeyboard, which
	// swaps Slate back to the OS cursor synchronously (UCommonInputSubsystem::RecalculateCurrentInputType)
	ReassertVirtualCursor(Absolute);
	return Hit;
}

void FMordecaiAgentBridgeInput::ReleaseMouseButton(const FKey& Button, const FVector2D& Absolute, const FModifierKeysState& Modifiers)
{
	if (!FSlateApplication::IsInitialized())
	{
		return;
	}
	FSlateApplication& Slate = FSlateApplication::Get();
	// A player's cursor is still on the button when they let go: a real (non-synthetic) move there
	// re-hovers the pressed SButton even if a synthesized move un-hovered it meanwhile (it is the
	// captor, so it gets OnMouseEnter again). SButton::OnMouseButtonUp only clicks when hovered.
	ReassertVirtualCursor(Absolute);
	const FPointerEvent Move(Slate.GetUserIndexForMouse(), FSlateApplicationBase::CursorPointerIndex,
		Absolute, Absolute, PressedMouseButtons, EKeys::Invalid, 0.f, Modifiers);
	Slate.ProcessMouseMoveEvent(Move, /*bIsSynthetic*/ false);

	PressedMouseButtons.Remove(Button);
	const FPointerEvent Event(Slate.GetUserIndexForMouse(), FSlateApplicationBase::CursorPointerIndex,
		Absolute, Absolute, PressedMouseButtons, Button, 0.f, Modifiers);
	Slate.ProcessMouseButtonUpEvent(Event);
	ReassertVirtualCursor(Absolute);
}

FVector2D FMordecaiAgentBridgeInput::GetCursorAbsolute()
{
	return FSlateApplication::IsInitialized() ? FVector2D(FSlateApplication::Get().GetCursorPos()) : FVector2D::ZeroVector;
}

#endif // !UE_BUILD_SHIPPING
