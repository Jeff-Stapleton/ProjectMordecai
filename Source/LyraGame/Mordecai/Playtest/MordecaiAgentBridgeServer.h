// Project Mordecai — Agent bridge TCP server (Specs/agent_bridge_v1.md §2-§4)
//
// Localhost-only NDJSON server. Everything (accept, socket IO, parsing, command execution, UObject
// access) runs on the game thread from an FTSTicker callback that ticks every frame, also while the
// game is paused and during the loading screen. The listen socket is non-blocking and polled there:
// no listener thread, no accept queue. Compiled out of Shipping.

#pragma once

#include "CoreMinimal.h"
#include "Containers/Ticker.h"
#include "Dom/JsonObject.h"
#include "Dom/JsonValue.h"
#include "InputCoreTypes.h"
#include "Mordecai/Playtest/MordecaiAgentBridgeInput.h"
#include "Mordecai/Playtest/MordecaiAgentBridgeProtocol.h"
#include "Mordecai/Playtest/MordecaiAgentBridgeState.h"
#include "UObject/WeakObjectPtrTemplates.h"

class FSocket;
class UGameInstance;
class UGameViewportClient;

#if !UE_BUILD_SHIPPING

class FMordecaiAgentBridgeServer
{
public:
	FMordecaiAgentBridgeServer(UGameInstance* InGameInstance, const FMordecaiAgentBridgeConfig& InConfig);
	~FMordecaiAgentBridgeServer();

	/** Bind and listen on 127.0.0.1:<Port>, then start ticking. False (and logged) when the port cannot be bound. */
	bool Start();

private:
	struct FHeldButton
	{
		FKey Key;
		FVector2D Absolute = FVector2D::ZeroVector;
	};

	/** A command that takes time; Advance returns true once it has replied. */
	struct FPendingCommand
	{
		TSharedPtr<FJsonValue> Id;
		FString Cmd;
		TFunction<bool(double Now)> Advance;
	};

	struct FConnection
	{
		int32 Serial = 0;
		FSocket* Socket = nullptr;
		FMordecaiAgentBridgeLineFramer Framer;
		TArray<uint8> SendBuffer;
		TArray<FMordecaiAgentBridgeFramedLine> Queue;
		TUniquePtr<FPendingCommand> Active;
		/** Mouse buttons this connection's click holds down (keys and axes live in the server-wide union). */
		TArray<FHeldButton> HeldButtons;
		/** The peer sent EOF: no more requests, but the ones already received still run and get replies. */
		bool bPeerClosed = false;
		/** Close at the end of this tick (send error, queue finished after EOF, quit, ...). */
		bool bClosed = false;
	};

	/** A refused (5th) connection: its error line is sent and its write side shut down; drained until the peer closes. */
	struct FRefusedConnection
	{
		FSocket* Socket = nullptr;
		double Deadline = 0.0;
	};

	bool Tick(float DeltaTime);

	void AcceptConnections(double Now);
	void Refuse(FSocket* Socket, double Now);
	void DrainRefusedConnections(double Now);
	void Receive(FConnection& Connection);
	void Flush(FConnection& Connection);
	void CloseSocket(FSocket*& Socket);
	void CloseFinishedConnections();
	void RemoveClosedConnections();
	int32 CountOpenConnections() const;

	void ProcessConnection(FConnection& Connection, double Now);
	void Execute(FConnection& Connection, const FMordecaiAgentBridgeFramedLine& Line, double Now);
	void Send(FConnection& Connection, const TSharedRef<FJsonObject>& Response);

	// Commands
	void HandlePing(FConnection& Connection, const FMordecaiAgentBridgeRequest& Request);
	void HandleState(FConnection& Connection, const FMordecaiAgentBridgeRequest& Request);
	void HandleKey(FConnection& Connection, const FMordecaiAgentBridgeRequest& Request, double Now);
	void HandleAxis(FConnection& Connection, const FMordecaiAgentBridgeRequest& Request, double Now);
	void HandleMouse(FConnection& Connection, const FMordecaiAgentBridgeRequest& Request);
	void HandleClick(FConnection& Connection, const FMordecaiAgentBridgeRequest& Request, double Now);
	void HandleScreenshot(FConnection& Connection, const FMordecaiAgentBridgeRequest& Request, double Now);
	void HandleWait(FConnection& Connection, const FMordecaiAgentBridgeRequest& Request, double Now);
	void HandleWaitUntil(FConnection& Connection, const FMordecaiAgentBridgeRequest& Request, double Now);
	void HandleQuit(FConnection& Connection, const FMordecaiAgentBridgeRequest& Request);

	// Held input: one server-wide union (Slate sees one keyboard, pad and mouse); each connection
	// adds and removes only its own holds, and Slate gets one event per union transition.
	bool PressKey(FConnection& Connection, const FKey& Key, MordecaiAgentBridge::ERoute Route, double Now);
	/** Returns whether this connection held the key (was_held). */
	bool ReleaseKey(FConnection& Connection, const FKey& Key);
	void SetAxis(FConnection& Connection, const FKey& Key, float Value, MordecaiAgentBridge::ERoute Route);
	void ReleaseAllHeldInput(FConnection& Connection);
	void UpdateHeldInput(double Now);
	/** Synthesize the threshold buttons a real pad derives from Axis at Value (down / up on change). */
	void UpdateThresholdButtons(const FKey& Axis, float Value, MordecaiAgentBridge::ERoute Route, double Now);
	bool IsMouseButtonHeld(const FKey& Button) const;
	FModifierKeysState CurrentModifiers() const;
	TArray<FMordecaiAgentBridgeHeldInput> CollectHeldInputs() const;
	TArray<FString> CollectSynthesizedKeys() const;
	void FillBridgeContext(FMordecaiAgentBridgeStateContext& Context) const;

	/** Reply fields every input command carries: ready and env. */
	void AddInputFields(const TSharedRef<FJsonObject>& Response, bool bReady, const FMordecaiAgentBridgeEnv& Env) const;
	bool IsGameReady() const;
	UGameViewportClient* GetViewportClient() const;

	void HandleScreenshotProcessed();

	TWeakObjectPtr<UGameInstance> GameInstance;
	FMordecaiAgentBridgeConfig Config;
	TSharedPtr<FJsonObject> BuildInfo;

	FSocket* ListenSocket = nullptr;
	TArray<TUniquePtr<FConnection>> Connections;
	TArray<FRefusedConnection> RefusedConnections;
	int32 NextConnectionSerial = 1;

	FMordecaiAgentBridgeInputUnion HeldUnion;
	FMordecaiAgentBridgeAutoRepeat AutoRepeat;
	/** The last analog sample sent per axis (0 = released), so a release is sent exactly once. */
	TMap<FKey, float> LastSentAxisValues;
	/** Per axis: the threshold buttons currently synthesized down. */
	TMap<FKey, TArray<FKey>> ThresholdButtonsDown;

	FTSTicker::FDelegateHandle TickHandle;
	FDelegateHandle ScreenshotProcessedHandle;
	uint64 ScreenshotsProcessed = 0;
	int32 NextScreenshotNumber = 1;

	FMordecaiAgentBridgeInput Input;
	bool bQuitRequested = false;
	bool bExitRequested = false;
};

#endif // !UE_BUILD_SHIPPING
