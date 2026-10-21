// Project Mordecai — Agent bridge protocol helpers (Specs/agent_bridge_v1.md §2-§5)
//
// Pure helpers shared by the in-game agent bridge (UMordecaiAgentBridgeSubsystem) and its headless
// tests: command-line config, NDJSON line framing, request parsing, response building, key-name
// resolution, argument validation, viewport <-> Slate absolute coordinates and build_info.json lookup.
// Everything here compiles in every configuration (including Shipping); only the socket server and
// the input injection are compiled out of Shipping.

#pragma once

#include "CoreMinimal.h"
#include "Dom/JsonObject.h"
#include "Dom/JsonValue.h"
#include "GenericPlatform/GenericApplication.h"
#include "InputCoreTypes.h"

DECLARE_LOG_CATEGORY_EXTERN(LogMordecaiAgentBridge, Log, All);

/**
 * Bridge settings from the command line:
 *   -AgentBridge [-AgentBridgePort=47777] [-AgentBridgeToken=<s>] [-AgentBridgeShotDir=<dir>]
 * Only the exact -AgentBridge token enables the bridge.
 */
struct FMordecaiAgentBridgeConfig
{
	static constexpr int32 DefaultPort = 47777;

	bool bEnabled = false;
	int32 Port = DefaultPort;
	FString Token;
	FString ShotDir;

	static FMordecaiAgentBridgeConfig FromCommandLine(const TCHAR* CommandLine);
};

/** One line produced by the framer: decoded text, or a marker that a line exceeded the limit. */
struct FMordecaiAgentBridgeFramedLine
{
	FString Text;
	bool bOversize = false;
};

/**
 * Splits a raw TCP byte stream into UTF-8 NDJSON lines.
 * - '\n' and "\r\n" both end a line; blank (whitespace-only) lines are dropped.
 * - A line longer than MaxLineBytes yields one bOversize entry; the rest of it is discarded up to
 *   the next '\n'.
 */
class FMordecaiAgentBridgeLineFramer
{
public:
	explicit FMordecaiAgentBridgeLineFramer(int32 InMaxLineBytes = 1024 * 1024);

	/** Feed received bytes; complete lines are appended to OutLines in order. */
	void Append(const uint8* Data, int32 Num, TArray<FMordecaiAgentBridgeFramedLine>& OutLines);

	/** Bytes of the current, not yet terminated line. */
	int32 GetPendingBytes() const { return Pending.Num(); }

private:
	void EmitPending(TArray<FMordecaiAgentBridgeFramedLine>& OutLines);

	TArray<uint8> Pending;
	int32 MaxLineBytes;
	bool bDiscarding = false;
};

/** A parsed request line. Id is a JSON null value when the request has no id. */
struct FMordecaiAgentBridgeRequest
{
	TSharedPtr<FJsonValue> Id;
	FString Cmd;
	TSharedPtr<FJsonObject> Args;
};

/** A validation failure: Code is a protocol error code (bad_request, bad_key, ...). */
struct FMordecaiAgentBridgeError
{
	FString Code;
	FString Message;
};

namespace MordecaiAgentBridge
{
	inline constexpr int32 ProtocolVersion = 1;
	inline constexpr int32 MaxLineBytes = 1024 * 1024;
	inline constexpr int32 MaxConnections = 4;
	/** A refused connection is drained for at most this long after its error line, so the client reads the line instead of an RST. */
	inline constexpr double RefusedConnectionLingerSeconds = 1.0;
	/** Keyboard typematic (Windows defaults: delay 1, speed 31): first repeat after 0.5 s, then about 30 per second. */
	inline constexpr double KeyRepeatDelaySeconds = 0.5;
	inline constexpr double KeyRepeatIntervalSeconds = 1.0 / 30.0;
	/** Gamepad button repeat: XInputInterface.cpp defaults, overridden by [/Script/Engine.InputSettings] in Input.ini (BaseInput.ini: 0.2 / 0.1). */
	inline constexpr double GamepadInitialRepeatDelaySeconds = 0.2;
	inline constexpr double GamepadRepeatDelaySeconds = 0.1;
	inline constexpr double ScreenshotTimeoutSeconds = 10.0;
	/** wait_until conditions: longest accepted text and deepest '!' / '(' nesting. */
	inline constexpr int32 MaxConditionLength = 4096;
	inline constexpr int32 MaxConditionDepth = 64;
	/** XInput's digital thresholds on its raw values (Xinput.h, used by XInputInterface.cpp). */
	inline constexpr int32 XInputLeftThumbDeadZone = 7849;   // XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE, of 32767
	inline constexpr int32 XInputRightThumbDeadZone = 8689;  // XINPUT_GAMEPAD_RIGHT_THUMB_DEADZONE
	inline constexpr int32 XInputTriggerThreshold = 30;      // XINPUT_GAMEPAD_TRIGGER_THRESHOLD, of 255

	// --- Error codes (§3) ---
	inline constexpr const TCHAR* ErrBadRequest = TEXT("bad_request");
	inline constexpr const TCHAR* ErrUnknownCommand = TEXT("unknown_command");
	inline constexpr const TCHAR* ErrBadKey = TEXT("bad_key");
	inline constexpr const TCHAR* ErrNotReady = TEXT("not_ready");
	inline constexpr const TCHAR* ErrTimeout = TEXT("timeout");
	inline constexpr const TCHAR* ErrFailed = TEXT("failed");
	inline constexpr const TCHAR* ErrTooManyConnections = TEXT("too_many_connections");
	inline constexpr const TCHAR* ErrCancelled = TEXT("cancelled");

	// --- Requests and responses ---

	/**
	 * Parse one request line. Returns false (a bad_request) for malformed JSON, a non-object, a
	 * missing/ill-typed cmd or an id that is not a number or string. Out.Id is filled whenever the
	 * id could be read, so the error reply can still echo it.
	 */
	bool ParseRequest(const FString& Line, FMordecaiAgentBridgeRequest& Out, FString& OutMessage);

	/** {"id":<Id|null>,"ok":true,"cmd":<Cmd>} — add result fields after. Empty Cmd writes null. */
	TSharedRef<FJsonObject> MakeOkResponse(const TSharedPtr<FJsonValue>& Id, const FString& Cmd);

	/** {"id":..,"ok":false,"cmd":..,"error":Code,"message":Message} */
	TSharedRef<FJsonObject> MakeErrorResponse(const TSharedPtr<FJsonValue>& Id, const FString& Cmd, const FString& Code, const FString& Message);

	/** Condensed JSON with no raw newline (no trailing newline either). */
	FString ToJsonLine(const TSharedRef<FJsonObject>& Object);

	/** A compact JSON number (rounded to Decimals, trailing zeros trimmed). Non-finite -> null. */
	TSharedRef<FJsonValue> MakeNumber(double Value, int32 Decimals = 3);
	TSharedRef<FJsonValue> MakeVector(const FVector& Value);
	TSharedRef<FJsonValue> MakeRotator(const FRotator& Value);
	TSharedRef<FJsonValue> MakeVector2D(const FVector2D& Value);
	TSharedRef<FJsonValue> MakeStringOrNull(const FString& Value, bool bNull);

	// --- Keys (§5) ---

	enum class EKeyUse : uint8
	{
		Key,         // key command: keyboard keys and gamepad buttons
		Axis,        // axis command: the six analog gamepad axes
		MouseButton  // click command: Left / Right / Middle
	};

	struct FKeyResolution
	{
		bool bOk = false;
		FKey Key;
		/** Canonical FKey name, e.g. "SpaceBar" for "space". */
		FString Name;
		FMordecaiAgentBridgeError Error;
	};

	/** Resolve a key name (case-insensitive, with aliases) and check it suits Use. */
	FKeyResolution ResolveKey(const FString& Name, EKeyUse Use, bool bAllowUnsafe = false);

	/** Development debug exec bindings with side effects (K, F1-F12, Insert, ...). */
	bool IsDeniedDebugKey(const FKey& Key);

	/** Shift / Ctrl / Alt / Cmd keys. */
	bool IsModifierKey(const FKey& Key);

	/** Modifier state from the keys the bridge holds (never the real keyboard). */
	FModifierKeysState MakeModifierKeysState(const TArray<FKey>& HeldKeys);

	/** Gamepad_LeftTriggerAxis / Gamepad_RightTriggerAxis (range 0..1; sticks are -1..1). */
	bool IsTriggerAxis(const FKey& Key);

	// --- Gamepad axis threshold buttons (§4 axis) ---

	/**
	 * The digital buttons a real XInput pad derives from one analog axis (XInputInterface.cpp
	 * SendControllerEvents): Gamepad_LeftX -> LeftStick_Left / LeftStick_Right, Gamepad_LeftY ->
	 * LeftStick_Down / LeftStick_Up, the right stick likewise, and each trigger axis -> its
	 * Gamepad_LeftTrigger / Gamepad_RightTrigger button (Negative is invalid for triggers).
	 */
	struct FAxisThresholdButtons
	{
		FKey Negative;
		FKey Positive;
		int32 Threshold = 0;   // on the raw XInput value
		bool bTrigger = false;
	};

	/** False for anything but the six analog gamepad axes. */
	bool GetAxisThresholdButtons(const FKey& Axis, FAxisThresholdButtons& OutButtons);

	/** The raw XInput value for a bridge axis value: sticks -32768..32767 (inverse of ShortToNormalizedFloat), triggers 0..255. */
	int32 AxisValueToXInputRaw(const FKey& Axis, float Value);

	/** The threshold buttons that are down for Axis at Value, by XInput's rule (raw value strictly past the threshold). */
	TArray<FKey> GetAxisThresholdKeysDown(const FKey& Axis, float Value);

	// --- Ids (§4 state: ids clients compare exactly) ---

	/**
	 * A canonical id from an FName: its text lowercased. A cooked (non-editor) build does not preserve
	 * FName case (WITH_CASE_PRESERVING_NAME is WITH_EDITORONLY_DATA), so FName::ToString returns the
	 * spelling first registered in the process, e.g. "Inventory" for the tab id FName("inventory").
	 */
	FString CanonicalNameId(const FName& Id);

	/** If Text parses as a GUID, its FGuid::ToString() form (32 uppercase hex digits, as inventory instance ids). */
	bool TryCanonicalGuid(const FString& Text, FString& OutGuid);

	/** A UMordecaiTabButton id: a GUID in instance-id form (identify buttons), anything else lowercased (tabs, filters). */
	FString CanonicalButtonId(const FString& IdText);

	// --- Coordinates (§4: screen coordinates are viewport pixels) ---

	/** Viewport pixel -> Slate absolute (desktop) position. */
	FVector2D ViewportToAbsolute(const FVector2D& Pixel, const FVector2D& ViewportAbsPos, const FVector2D& ViewportAbsSize, const FIntPoint& ViewportSize);

	/** Slate absolute position -> viewport pixel: px = (abs - vpAbsPos) * (viewportSize / vpAbsSize). */
	FVector2D AbsoluteToViewport(const FVector2D& Absolute, const FVector2D& ViewportAbsPos, const FVector2D& ViewportAbsSize, const FIntPoint& ViewportSize);

	// --- build_info.json (§4 ping) ---

	/** Look for build_info.json in StartDir and up to MaxParents parent directories. Empty if not found. */
	FString FindBuildInfoFile(const FString& StartDir, int32 MaxParents = 6);

	/** {commit, branch, dirty, built_at} from a build_info.json, or null if unreadable. */
	TSharedPtr<FJsonObject> ReadBuildInfo(const FString& FilePath);

	/** 'LyraExperienceDefinition:B_X', '/Path/B_X.B_X_C', 'Default__B_X_C', 'B_X_C' -> 'B_X'. */
	FString NormalizeExperienceName(const FString& Name);

	// --- Command arguments (§4) ---

	enum class ERoute : uint8
	{
		Slate,    // FSlateApplication::Process*Event — the player path
		Viewport  // FSceneViewport::On* — diagnostic only
	};

	enum class EKeyAction : uint8
	{
		Press,
		Release,
		Tap
	};

	struct FKeyCommandArgs
	{
		TArray<FKey> Keys;
		TArray<FString> KeyNames;
		bool bListSyntax = false;   // "keys":[...] rather than "key":"..."
		EKeyAction Action = EKeyAction::Tap;
		double HoldSeconds = 0.1;
		bool bUnsafe = false;
		ERoute Route = ERoute::Slate;
	};

	struct FAxisCommandArgs
	{
		TArray<FKey> Keys;
		TArray<FString> KeyNames;
		TArray<float> Values;
		TOptional<double> HoldSeconds;
		ERoute Route = ERoute::Slate;
	};

	struct FMouseCommandArgs
	{
		FVector2D Pixel = FVector2D::ZeroVector;
	};

	struct FClickCommandArgs
	{
		TOptional<FVector2D> Pixel;
		FKey Button;
		FString ButtonName;
		double HoldSeconds = 0.05;
	};

	struct FWaitCommandArgs
	{
		double Seconds = 0.0;
	};

	struct FWaitUntilCommandArgs
	{
		FString Condition;
		double TimeoutSeconds = 10.0;
		double PollSeconds = 0.1;
	};

	struct FStateCommandArgs
	{
		TArray<FString> Sections;
		double Radius = 1500.0;
		bool bIncludeAllActors = false;
	};

	struct FScreenshotCommandArgs
	{
		FString Path; // empty -> <ShotDir>/bridge_NN.png
	};

	ERoute ParseRouteName(const FString& Name, bool& bOutValid);
	const TCHAR* RouteToString(ERoute Route);
	const TCHAR* KeyActionToString(EKeyAction Action);

	/**
	 * §1 environment emulation: give user 0 keyboard focus back to the game viewport only for the
	 * Slate route, only when the user-0 focus path holds no live widget at all (a game launched on a
	 * locked desktop), and never while the CommonUI router is in Menu mode. A path whose leaf widget
	 * died still routes keys along its live part, as it does for a player, so it is left alone.
	 */
	bool ShouldRestoreUserFocus(ERoute Route, bool bFocusPathHasLiveWidget, bool bMenuInputMode);

	bool ParseKeyArgs(const FJsonObject& Request, FKeyCommandArgs& Out, FMordecaiAgentBridgeError& OutError);
	bool ParseAxisArgs(const FJsonObject& Request, FAxisCommandArgs& Out, FMordecaiAgentBridgeError& OutError);
	bool ParseMouseArgs(const FJsonObject& Request, FMouseCommandArgs& Out, FMordecaiAgentBridgeError& OutError);
	bool ParseClickArgs(const FJsonObject& Request, FClickCommandArgs& Out, FMordecaiAgentBridgeError& OutError);
	bool ParseWaitArgs(const FJsonObject& Request, FWaitCommandArgs& Out, FMordecaiAgentBridgeError& OutError);
	bool ParseWaitUntilArgs(const FJsonObject& Request, FWaitUntilCommandArgs& Out, FMordecaiAgentBridgeError& OutError);
	bool ParseStateArgs(const FJsonObject& Request, FStateCommandArgs& Out, FMordecaiAgentBridgeError& OutError);
	bool ParseScreenshotArgs(const FJsonObject& Request, FScreenshotCommandArgs& Out, FMordecaiAgentBridgeError& OutError);

	/** game, focus, player, input, pickups, inventory, pause_menu, widgets, actors (default order). */
	const TArray<FString>& GetStateSectionNames();
}

/**
 * Auto-repeat the way real devices produce it (§4 key):
 * - Keyboard (Windows typematic): only the most recently pressed key repeats, first after
 *   KeyboardDelay, then every KeyboardInterval. Pressing another key moves the repeat to it; the
 *   earlier key does not resume, even when the newer key is released.
 * - Gamepad buttons (XInputInterface.cpp): every held button repeats on its own schedule, first
 *   GamepadInitialDelay after its press, then every GamepadRepeatDelay.
 * Pure: the bridge feeds it union transitions and asks which keys are due.
 */
class FMordecaiAgentBridgeAutoRepeat
{
public:
	explicit FMordecaiAgentBridgeAutoRepeat(
		double InKeyboardDelay = MordecaiAgentBridge::KeyRepeatDelaySeconds,
		double InKeyboardInterval = MordecaiAgentBridge::KeyRepeatIntervalSeconds,
		double InGamepadInitialDelay = MordecaiAgentBridge::GamepadInitialRepeatDelaySeconds,
		double InGamepadRepeatDelay = MordecaiAgentBridge::GamepadRepeatDelaySeconds);

	void SetGamepadTiming(double InInitialDelay, double InRepeatDelay);

	/** Key went down (0 -> 1 holders). */
	void KeyDown(const FKey& Key, double Now);

	/** Key went up (1 -> 0 holders). */
	void KeyUp(const FKey& Key);

	/** Keys due for a bIsRepeat=true down event at Now (each at most once per call), rescheduled. */
	TArray<FKey> CollectRepeats(double Now);

	/** The keyboard key that currently repeats (invalid when none). */
	const FKey& GetTypematicKey() const { return TypematicKey; }

	void Reset();

private:
	double KeyboardDelay;
	double KeyboardInterval;
	double GamepadInitialDelay;
	double GamepadRepeatDelay;

	FKey TypematicKey;
	double TypematicNextTime = 0.0;
	TArray<TPair<FKey, double>> GamepadNextTimes;
};

/**
 * The server-wide union of the input every bridge connection holds (§2 Connections). Slate sees one
 * keyboard, one pad and one mouse: a key goes down on its first holder and up on its last; an axis
 * reads the held value with the largest magnitude. Each connection only adds and removes its own
 * holds. Holder ids are connection serials; SynthesizedHolder marks the threshold buttons the
 * bridge derives from axes, which are not reported as held by a connection. Pure (no Slate).
 */
class FMordecaiAgentBridgeInputUnion
{
public:
	static constexpr int32 SynthesizedHolder = -1;

	/** Holder starts holding Key. True on the 0 -> 1 transition (send the down); false if it already held it. */
	bool AddKey(int32 Holder, const FKey& Key, MordecaiAgentBridge::ERoute Route);

	/** Holder stops holding Key. True on the 1 -> 0 transition (send the up). bOutWasHeld: Holder held it. */
	bool RemoveKey(int32 Holder, const FKey& Key, bool& bOutWasHeld);

	bool HoldsKey(int32 Holder, const FKey& Key) const;
	bool IsKeyDown(const FKey& Key) const;
	int32 CountKeyHolders(const FKey& Key, bool bIncludeSynthesized) const;
	MordecaiAgentBridge::ERoute GetKeyRoute(const FKey& Key) const;

	/** Keys Holder holds, in press order. */
	TArray<FKey> GetKeysHeldBy(int32 Holder) const;

	/** Every key down in the union, in first-press order. */
	TArray<FKey> GetDownKeys() const;

	/** Holder's value for Axis; 0 removes Holder's hold. */
	void SetAxis(int32 Holder, const FKey& Axis, float Value, MordecaiAgentBridge::ERoute Route);

	/** The union value: the held value with the largest magnitude (the earliest holder wins a tie); 0 when nobody holds it. */
	float GetAxisValue(const FKey& Axis) const;
	int32 CountAxisHolders(const FKey& Axis) const;
	MordecaiAgentBridge::ERoute GetAxisRoute(const FKey& Axis) const;
	TArray<FKey> GetAxesHeldBy(int32 Holder) const;

	/** Axes with at least one non-zero holder, in first-hold order. */
	TArray<FKey> GetHeldAxes() const;

	void Reset();

private:
	struct FKeyHold
	{
		FKey Key;
		MordecaiAgentBridge::ERoute Route = MordecaiAgentBridge::ERoute::Slate;
		TArray<int32> Holders;
	};

	struct FAxisHold
	{
		FKey Key;
		MordecaiAgentBridge::ERoute Route = MordecaiAgentBridge::ERoute::Slate;
		TArray<TPair<int32, float>> Values;
	};

	TArray<FKeyHold> Keys;
	TArray<FAxisHold> Axes;
};
