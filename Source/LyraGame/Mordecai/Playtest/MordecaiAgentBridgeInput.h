// Project Mordecai — Agent bridge input injection (Specs/agent_bridge_v1.md §1, §4 key/axis/mouse/click)
//
// Injects OS-level events into Slate (FSlateApplication::Process*Event), so every event takes the path a
// real keyboard, mouse or gamepad event takes: input preprocessors, user-0 focus, SViewport ->
// FSceneViewport -> game viewport client -> CommonUI router -> PlayerController -> Enhanced Input.
// The bridge never calls gameplay or UI functions as a stand-in for input. The only other path,
// ERoute::Viewport (FSceneViewport::On*), is diagnostic and never acceptance evidence.
// Compiled out of Shipping.

#pragma once

#include "CoreMinimal.h"
#include "Dom/JsonObject.h"
#include "GenericPlatform/GenericApplication.h"
#include "InputCoreTypes.h"
#include "Mordecai/Playtest/MordecaiAgentBridgeProtocol.h"

class UGameViewportClient;

#if !UE_BUILD_SHIPPING

/** What the bridge had to emulate so a backgrounded or locked game behaves like a focused window. */
struct FMordecaiAgentBridgeEnv
{
	bool bFocusRestored = false;
	bool bVirtualCursor = false;
	bool bAppActive = false;

	TSharedRef<FJsonObject> ToJson() const;
};

class FMordecaiAgentBridgeInput
{
public:
	/** Slate is up and the game viewport (Slate widget and scene viewport) exists. */
	static bool IsReady(const UGameViewportClient* ViewportClient, FString& OutReason);

	/**
	 * Keep Slate accepting device input while the app is inactive and, for the Slate route, give user 0
	 * keyboard focus back to the game viewport only when its focus path holds no live widget at all and
	 * the CommonUI router is not in Menu mode (MordecaiAgentBridge::ShouldRestoreUserFocus). A path whose
	 * leaf died still routes keys along its live part, as for a player, so it is left alone.
	 */
	FMordecaiAgentBridgeEnv PrepareEnvironment(MordecaiAgentBridge::ERoute Route, const UGameViewportClient* ViewportClient);

	/** Swap Slate's cursor user to the in-memory cursor (idempotent). True when it had to swap. */
	bool EnsureVirtualCursor();

	/**
	 * EnsureVirtualCursor, then put the in-memory cursor at Absolute. Called on every mouse/click, after
	 * each injected button event, and every frame while a bridge mouse button is down: CommonInput's
	 * switch to MouseAndKeyboard and real OS mouse moves (more than 3) both swap Slate back to the OS
	 * cursor, whose position Slate's synthesized moves would then use to un-hover the pressed button.
	 */
	void ReassertVirtualCursor(const FVector2D& Absolute);

	/** Live Slate state: the cursor user's cursor is not the platform (OS) cursor right now. */
	static bool IsVirtualCursorActive();

	/** Live Slate state: user 0's focus path still holds at least one live widget. */
	static bool HasLiveFocusPath();

	/** Key down / repeat / up for a keyboard key or gamepad button. */
	bool SendKey(UGameViewportClient* ViewportClient, const FKey& Key, bool bDown, bool bRepeat, const FModifierKeysState& Modifiers, MordecaiAgentBridge::ERoute Route);

	/** One analog sample for a gamepad axis. */
	bool SendAnalog(UGameViewportClient* ViewportClient, const FKey& Key, float Value, const FModifierKeysState& Modifiers, MordecaiAgentBridge::ERoute Route);

	/** Move the virtual cursor to Absolute (Slate desktop space) and inject a mouse move there. */
	void MoveCursor(const FVector2D& Absolute, const FModifierKeysState& Modifiers);

	/** Move to Absolute, then button down. Returns the type of the widget Slate hit-tests there ("" for none). */
	FString PressMouseButton(const FKey& Button, const FVector2D& Absolute, const FModifierKeysState& Modifiers);

	/**
	 * A non-synthetic move at Absolute (so the pressed button is hovered again, as under a player's
	 * cursor), then button up there.
	 */
	void ReleaseMouseButton(const FKey& Button, const FVector2D& Absolute, const FModifierKeysState& Modifiers);

	static FVector2D GetCursorAbsolute();

private:
	/** Pressed buttons as Slate sees them from the bridge. FPointerEvent keeps a pointer to this set. */
	TSet<FKey> PressedMouseButtons;
};

#endif // !UE_BUILD_SHIPPING
