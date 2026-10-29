// Project Mordecai — Agent bridge TCP server (Specs/agent_bridge_v1.md §2-§4)

#include "Mordecai/Playtest/MordecaiAgentBridgeServer.h"

#if !UE_BUILD_SHIPPING

#include "Common/TcpSocketBuilder.h"
#include "CoreGlobals.h"
#include "Engine/GameInstance.h"
#include "Engine/GameViewportClient.h"
#include "Engine/World.h"
#include "GameFramework/WorldSettings.h"
#include "Framework/Application/SlateApplication.h"
#include "HAL/FileManager.h"
#include "HAL/PlatformProcess.h"
#include "Interfaces/IPv4/IPv4Address.h"
#include "Interfaces/IPv4/IPv4Endpoint.h"
#include "Misc/App.h"
#include "Misc/ConfigCacheIni.h"
#include "Misc/Paths.h"
#include "SocketSubsystem.h"
#include "Sockets.h"
#include "UnrealClient.h"

#include "Mordecai/Playtest/MordecaiAgentBridgePredicate.h"
#include "Mordecai/Playtest/MordecaiAgentBridgeState.h"

namespace MordecaiAgentBridgeServerPrivate
{
	static constexpr int32 MaxSendBufferBytes = 64 * 1024 * 1024;
	static constexpr int32 MaxReceivePerTickBytes = 8 * 1024 * 1024;
	static constexpr int32 MaxAcceptsPerTick = 16;

	static TSharedRef<FJsonValue> ToJsonArray(const TArray<FString>& Strings)
	{
		TArray<TSharedPtr<FJsonValue>> Values;
		for (const FString& String : Strings)
		{
			Values.Add(MakeShared<FJsonValueString>(String));
		}
		return MakeShared<FJsonValueArray>(Values);
	}

	static ISocketSubsystem* Sockets()
	{
		return ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM);
	}

	/** The most of one frame the world simulates (AWorldSettings::FixupDeltaSeconds; 0.4 s by default). */
	static double MaxFrameSeconds(const UGameViewportClient* ViewportClient)
	{
		const UWorld* World = ViewportClient ? ViewportClient->GetWorld() : nullptr;
		const AWorldSettings* Settings = World ? World->GetWorldSettings() : nullptr;
		return Settings && Settings->MaxUndilatedFrameTime > 0.f ? Settings->MaxUndilatedFrameTime : 0.4;
	}

	/** Feeds a hold clock this frame: its delta, and how long the frame has taken so far. */
	static bool AdvanceHold(FMordecaiAgentBridgeHoldClock& Clock, double TickNow)
	{
		return Clock.Advance(TickNow, GFrameCounter, FApp::GetDeltaTime(), TickNow - FApp::GetCurrentTime());
	}

	/** A finished hold: held_s (wall time), simulated_s (the game time it was down for) and frames. */
	static void AddHoldFields(const TSharedRef<FJsonObject>& Response, const FMordecaiAgentBridgeHoldClock& Clock, double Now)
	{
		Response->SetField(TEXT("held_s"), MordecaiAgentBridge::MakeNumber(Clock.GetRealSeconds(Now)));
		Response->SetField(TEXT("simulated_s"), MordecaiAgentBridge::MakeNumber(Clock.GetSimulatedSeconds()));
		Response->SetNumberField(TEXT("frames"), static_cast<double>(Clock.GetFrames()));
	}
}

// ---------------------------------------------------------------------------
// Lifetime
// ---------------------------------------------------------------------------

FMordecaiAgentBridgeServer::FMordecaiAgentBridgeServer(UGameInstance* InGameInstance, const FMordecaiAgentBridgeConfig& InConfig)
	: GameInstance(InGameInstance)
	, Config(InConfig)
{
	const FString BuildInfoPath = MordecaiAgentBridge::FindBuildInfoFile(FPlatformProcess::BaseDir(), 6);
	BuildInfo = MordecaiAgentBridge::ReadBuildInfo(BuildInfoPath);
	UE_LOG(LogMordecaiAgentBridge, Log, TEXT("AgentBridge: build_info.json %s"),
		BuildInfo.IsValid() ? *BuildInfoPath : TEXT("not found (build: null)"));

	// Gamepad repeat cadence as XInputInterface.cpp reads it ([/Script/Engine.InputSettings] in Input.ini)
	float InitialButtonRepeatDelay = static_cast<float>(MordecaiAgentBridge::GamepadInitialRepeatDelaySeconds);
	float ButtonRepeatDelay = static_cast<float>(MordecaiAgentBridge::GamepadRepeatDelaySeconds);
	if (GConfig)
	{
		GConfig->GetFloat(TEXT("/Script/Engine.InputSettings"), TEXT("InitialButtonRepeatDelay"), InitialButtonRepeatDelay, GInputIni);
		GConfig->GetFloat(TEXT("/Script/Engine.InputSettings"), TEXT("ButtonRepeatDelay"), ButtonRepeatDelay, GInputIni);
	}
	AutoRepeat.SetGamepadTiming(InitialButtonRepeatDelay, ButtonRepeatDelay);
}

FMordecaiAgentBridgeServer::~FMordecaiAgentBridgeServer()
{
	if (TickHandle.IsValid())
	{
		FTSTicker::RemoveTicker(TickHandle);
		TickHandle.Reset();
	}
	if (ScreenshotProcessedHandle.IsValid())
	{
		FScreenshotRequest::OnScreenshotRequestProcessed().Remove(ScreenshotProcessedHandle);
	}

	// 1. Stop accepting
	CloseSocket(ListenSocket);

	// 2. Refused sockets still draining
	for (FRefusedConnection& Refused : RefusedConnections)
	{
		CloseSocket(Refused.Socket);
	}
	RefusedConnections.Reset();

	// 3. Live connections: release what they hold (unless the engine is already exiting), flush, close
	const bool bSendReleases = !IsEngineExitRequested();
	for (TUniquePtr<FConnection>& Connection : Connections)
	{
		if (bSendReleases)
		{
			ReleaseAllHeldInput(*Connection);
		}
		Flush(*Connection);
		CloseSocket(Connection->Socket);
	}
	Connections.Reset();
	HeldUnion.Reset();
	AutoRepeat.Reset();
}

bool FMordecaiAgentBridgeServer::Start()
{
	// 127.0.0.1 only (no firewall prompt, no remote access); not reusable, so a second game cannot share
	// the port. Non-blocking and polled from the game-thread ticker: no listener thread, no accept race.
	const FIPv4Endpoint Endpoint(FIPv4Address::InternalLoopback, static_cast<uint16>(Config.Port));
	ListenSocket = FTcpSocketBuilder(TEXT("MordecaiAgentBridgeListen"))
		.AsNonBlocking()
		.AsReusable(false)
		.BoundToEndpoint(Endpoint)
		.Listening(8)
		.Build();
	if (!ListenSocket)
	{
		UE_LOG(LogMordecaiAgentBridge, Error, TEXT("AgentBridge: failed to listen on 127.0.0.1:%d"), Config.Port);
		return false;
	}

	ScreenshotProcessedHandle = FScreenshotRequest::OnScreenshotRequestProcessed().AddRaw(this, &FMordecaiAgentBridgeServer::HandleScreenshotProcessed);
	TickHandle = FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateRaw(this, &FMordecaiAgentBridgeServer::Tick), 0.0f);

	UE_LOG(LogMordecaiAgentBridge, Display, TEXT("AgentBridge: listening on 127.0.0.1:%d"), Config.Port);
	return true;
}

void FMordecaiAgentBridgeServer::HandleScreenshotProcessed()
{
	++ScreenshotsProcessed;
}

// ---------------------------------------------------------------------------
// Tick
// ---------------------------------------------------------------------------

bool FMordecaiAgentBridgeServer::Tick(float DeltaTime)
{
	const double Now = FPlatformTime::Seconds();
	if (bQuitRequested)
	{
		// The quit reply and every cancellation went out last tick; now exit
		for (TUniquePtr<FConnection>& Connection : Connections)
		{
			Flush(*Connection);
		}
		DrainRefusedConnections(Now);
		if (!bExitRequested)
		{
			bExitRequested = true;
			UE_LOG(LogMordecaiAgentBridge, Display, TEXT("AgentBridge: quit requested by a client"));
			RequestEngineExit(TEXT("AgentBridge quit"));
		}
		return true;
	}

	for (TUniquePtr<FConnection>& Connection : Connections)
	{
		Receive(*Connection);
	}
	for (int32 Index = 0; Index < Connections.Num() && !bQuitRequested; ++Index)
	{
		ProcessConnection(*Connections[Index], Now);
	}
	if (!bQuitRequested)
	{
		UpdateHeldInput(Now);
	}
	for (TUniquePtr<FConnection>& Connection : Connections)
	{
		Flush(*Connection);
	}
	CloseFinishedConnections();
	RemoveClosedConnections();

	// After the removal, so a client that just went away does not count against the 4-connection cap
	if (!bQuitRequested)
	{
		AcceptConnections(Now);
	}
	DrainRefusedConnections(Now);
	return true;
}

// ---------------------------------------------------------------------------
// Sockets
// ---------------------------------------------------------------------------

int32 FMordecaiAgentBridgeServer::CountOpenConnections() const
{
	// A peer that sent EOF can never send another request, so it does not hold a slot while its last
	// commands finish (a killed CLI wait-until would otherwise lock the bridge for up to 600 s)
	int32 Count = 0;
	for (const TUniquePtr<FConnection>& Connection : Connections)
	{
		Count += (Connection->bClosed || Connection->bPeerClosed) ? 0 : 1;
	}
	return Count;
}

void FMordecaiAgentBridgeServer::AcceptConnections(double Now)
{
	if (!ListenSocket)
	{
		return;
	}
	for (int32 Accepted = 0; Accepted < MordecaiAgentBridgeServerPrivate::MaxAcceptsPerTick; ++Accepted)
	{
		bool bPending = false;
		if (!ListenSocket->HasPendingConnection(bPending) || !bPending)
		{
			return;
		}
		FSocket* Socket = ListenSocket->Accept(TEXT("MordecaiAgentBridgeClient"));
		if (!Socket)
		{
			return;
		}
		Socket->SetNonBlocking(true);
		Socket->SetNoDelay(true);

		if (CountOpenConnections() >= MordecaiAgentBridge::MaxConnections)
		{
			Refuse(Socket, Now);
			continue;
		}

		TUniquePtr<FConnection> Connection = MakeUnique<FConnection>();
		Connection->Serial = NextConnectionSerial++;
		Connection->Socket = Socket;
		UE_LOG(LogMordecaiAgentBridge, Display, TEXT("AgentBridge: client %d connected (%d open)"), Connection->Serial, CountOpenConnections() + 1);
		Connections.Add(MoveTemp(Connection));
	}
}

void FMordecaiAgentBridgeServer::Refuse(FSocket* Socket, double Now)
{
	const FString Line = MordecaiAgentBridge::ToJsonLine(MordecaiAgentBridge::MakeErrorResponse(nullptr, FString(),
		MordecaiAgentBridge::ErrTooManyConnections,
		FString::Printf(TEXT("at most %d concurrent connections"), MordecaiAgentBridge::MaxConnections))) + TEXT("\n");
	const FTCHARToUTF8 Utf8(*Line);
	int32 Sent = 0;
	Socket->Send(reinterpret_cast<const uint8*>(Utf8.Get()), Utf8.Length(), Sent);
	// FIN after the line, then keep reading: closing with unread input (the client's first request) would
	// send an RST, and the client would get a reset instead of the line
	Socket->Shutdown(ESocketShutdownMode::Write);
	RefusedConnections.Add({ Socket, Now + MordecaiAgentBridge::RefusedConnectionLingerSeconds });
	UE_LOG(LogMordecaiAgentBridge, Warning, TEXT("AgentBridge: refused a connection (too many connections)"));
}

void FMordecaiAgentBridgeServer::DrainRefusedConnections(double Now)
{
	for (int32 Index = RefusedConnections.Num() - 1; Index >= 0; --Index)
	{
		FRefusedConnection& Refused = RefusedConnections[Index];
		bool bDone = Now >= Refused.Deadline;
		uint8 Discard[4096];
		for (int32 Reads = 0; Reads < 64 && !bDone; ++Reads)
		{
			int32 Read = 0;
			if (!Refused.Socket->Recv(Discard, UE_ARRAY_COUNT(Discard), Read))
			{
				bDone = true; // the peer closed (or the socket failed)
			}
			else if (Read <= 0)
			{
				break; // nothing waiting
			}
		}
		if (bDone)
		{
			CloseSocket(Refused.Socket);
			RefusedConnections.RemoveAtSwap(Index);
		}
	}
}

void FMordecaiAgentBridgeServer::Receive(FConnection& Connection)
{
	if (Connection.bClosed || Connection.bPeerClosed || !Connection.Socket)
	{
		return;
	}
	uint8 Buffer[16 * 1024];
	int32 Total = 0;
	TArray<FMordecaiAgentBridgeFramedLine> Lines;
	while (Total < MordecaiAgentBridgeServerPrivate::MaxReceivePerTickBytes)
	{
		int32 Read = 0;
		// Recv returns false when the peer closed (or on an error); true with 0 bytes when nothing is waiting
		if (!Connection.Socket->Recv(Buffer, UE_ARRAY_COUNT(Buffer), Read))
		{
			// EOF: no more requests, but the ones already received still run and reply (a client that
			// half-closes after sending, such as ncat, waits for them)
			Connection.bPeerClosed = true;
			break;
		}
		if (Read <= 0)
		{
			break;
		}
		Total += Read;
		Connection.Framer.Append(Buffer, Read, Lines);
	}
	Connection.Queue.Append(MoveTemp(Lines));
}

void FMordecaiAgentBridgeServer::Flush(FConnection& Connection)
{
	while (Connection.Socket && Connection.SendBuffer.Num() > 0)
	{
		int32 Sent = 0;
		if (!Connection.Socket->Send(Connection.SendBuffer.GetData(), Connection.SendBuffer.Num(), Sent))
		{
			const ESocketErrors Error = MordecaiAgentBridgeServerPrivate::Sockets()->GetLastErrorCode();
			if (Error != SE_EWOULDBLOCK)
			{
				Connection.bClosed = true;
				Connection.SendBuffer.Reset();
			}
			return; // retry the rest next tick
		}
		if (Sent <= 0)
		{
			return;
		}
		Connection.SendBuffer.RemoveAt(0, Sent, EAllowShrinking::No);
	}
}

void FMordecaiAgentBridgeServer::CloseSocket(FSocket*& Socket)
{
	if (Socket)
	{
		Socket->Close();
		MordecaiAgentBridgeServerPrivate::Sockets()->DestroySocket(Socket);
		Socket = nullptr;
	}
}

void FMordecaiAgentBridgeServer::CloseFinishedConnections()
{
	for (TUniquePtr<FConnection>& Connection : Connections)
	{
		// After EOF: close once every received request has replied and the replies are out
		if (Connection->bPeerClosed && !Connection->bClosed && !Connection->Active.IsValid()
			&& Connection->Queue.Num() == 0 && Connection->SendBuffer.Num() == 0)
		{
			Connection->bClosed = true;
		}
	}
}

void FMordecaiAgentBridgeServer::RemoveClosedConnections()
{
	for (int32 Index = Connections.Num() - 1; Index >= 0; --Index)
	{
		FConnection& Connection = *Connections[Index];
		if (!Connection.bClosed)
		{
			continue;
		}
		// Disconnect releases everything this connection holds and drops its pending command
		Connection.Active.Reset();
		ReleaseAllHeldInput(Connection);
		CloseSocket(Connection.Socket);
		UE_LOG(LogMordecaiAgentBridge, Display, TEXT("AgentBridge: client %d disconnected"), Connection.Serial);
		Connections.RemoveAt(Index);
	}
}

void FMordecaiAgentBridgeServer::Send(FConnection& Connection, const TSharedRef<FJsonObject>& Response)
{
	if (Connection.bClosed)
	{
		return;
	}
	const FString Line = MordecaiAgentBridge::ToJsonLine(Response) + TEXT("\n");
	const FTCHARToUTF8 Utf8(*Line);
	if (Connection.SendBuffer.Num() + Utf8.Length() > MordecaiAgentBridgeServerPrivate::MaxSendBufferBytes)
	{
		UE_LOG(LogMordecaiAgentBridge, Warning, TEXT("AgentBridge: client %d is not reading its replies; closing it"), Connection.Serial);
		Connection.bClosed = true;
		return;
	}
	Connection.SendBuffer.Append(reinterpret_cast<const uint8*>(Utf8.Get()), Utf8.Length());
}

// ---------------------------------------------------------------------------
// Dispatch
// ---------------------------------------------------------------------------

void FMordecaiAgentBridgeServer::ProcessConnection(FConnection& Connection, double Now)
{
	// Strict arrival order: a command that takes time blocks the requests behind it. A peer that sent
	// EOF still gets every request it sent executed and answered.
	while (!Connection.bClosed && !bQuitRequested)
	{
		if (Connection.Active.IsValid())
		{
			if (!Connection.Active->Advance(Now))
			{
				return;
			}
			Connection.Active.Reset();
			continue;
		}
		if (Connection.Queue.Num() == 0)
		{
			return;
		}
		const FMordecaiAgentBridgeFramedLine Line = Connection.Queue[0];
		Connection.Queue.RemoveAt(0);
		Execute(Connection, Line, Now);
	}
}

void FMordecaiAgentBridgeServer::Execute(FConnection& Connection, const FMordecaiAgentBridgeFramedLine& Line, double Now)
{
	if (Line.bOversize)
	{
		Send(Connection, MordecaiAgentBridge::MakeErrorResponse(nullptr, FString(), MordecaiAgentBridge::ErrBadRequest,
			FString::Printf(TEXT("request line longer than %d bytes"), MordecaiAgentBridge::MaxLineBytes)));
		return;
	}

	FMordecaiAgentBridgeRequest Request;
	FString Message;
	if (!MordecaiAgentBridge::ParseRequest(Line.Text, Request, Message))
	{
		Send(Connection, MordecaiAgentBridge::MakeErrorResponse(Request.Id, Request.Cmd, MordecaiAgentBridge::ErrBadRequest, Message));
		return;
	}

	const FString& Cmd = Request.Cmd;
	if (Cmd == TEXT("ping")) { HandlePing(Connection, Request); }
	else if (Cmd == TEXT("state")) { HandleState(Connection, Request); }
	else if (Cmd == TEXT("key")) { HandleKey(Connection, Request, Now); }
	else if (Cmd == TEXT("axis")) { HandleAxis(Connection, Request, Now); }
	else if (Cmd == TEXT("mouse")) { HandleMouse(Connection, Request); }
	else if (Cmd == TEXT("click")) { HandleClick(Connection, Request, Now); }
	else if (Cmd == TEXT("screenshot")) { HandleScreenshot(Connection, Request, Now); }
	else if (Cmd == TEXT("wait")) { HandleWait(Connection, Request, Now); }
	else if (Cmd == TEXT("wait_until")) { HandleWaitUntil(Connection, Request, Now); }
	else if (Cmd == TEXT("quit")) { HandleQuit(Connection, Request); }
	else
	{
		Send(Connection, MordecaiAgentBridge::MakeErrorResponse(Request.Id, Cmd, MordecaiAgentBridge::ErrUnknownCommand,
			FString::Printf(TEXT("unknown command '%s' (ping, state, key, axis, mouse, click, screenshot, wait, wait_until, quit)"), *Cmd)));
	}
}

UGameViewportClient* FMordecaiAgentBridgeServer::GetViewportClient() const
{
	const UGameInstance* Instance = GameInstance.Get();
	return Instance ? Instance->GetGameViewportClient() : nullptr;
}

bool FMordecaiAgentBridgeServer::IsGameReady() const
{
	return MordecaiAgentBridge::State::GetReadiness(FMordecaiAgentBridgeStateContext::FromGameInstance(GameInstance.Get())).IsReady();
}

void FMordecaiAgentBridgeServer::AddInputFields(const TSharedRef<FJsonObject>& Response, bool bReady, const FMordecaiAgentBridgeEnv& Env) const
{
	Response->SetBoolField(TEXT("ready"), bReady);
	Response->SetObjectField(TEXT("env"), Env.ToJson());
}

void FMordecaiAgentBridgeServer::FillBridgeContext(FMordecaiAgentBridgeStateContext& Context) const
{
	Context.HeldInputs = CollectHeldInputs();
	Context.SynthesizedKeys = CollectSynthesizedKeys();
	Context.bVirtualCursor = FMordecaiAgentBridgeInput::IsVirtualCursorActive();
}

// ---------------------------------------------------------------------------
// ping / state
// ---------------------------------------------------------------------------

void FMordecaiAgentBridgeServer::HandlePing(FConnection& Connection, const FMordecaiAgentBridgeRequest& Request)
{
	const FMordecaiAgentBridgeStateContext Context = FMordecaiAgentBridgeStateContext::FromGameInstance(GameInstance.Get());
	const FMordecaiAgentBridgeReadiness Ready = MordecaiAgentBridge::State::GetReadiness(Context);

	const TSharedRef<FJsonObject> Response = MordecaiAgentBridge::MakeOkResponse(Request.Id, Request.Cmd);
	Response->SetNumberField(TEXT("bridge_version"), MordecaiAgentBridge::ProtocolVersion);
	Response->SetNumberField(TEXT("pid"), FPlatformProcess::GetCurrentProcessId());
	Response->SetStringField(TEXT("token"), Config.Token);
	if (BuildInfo.IsValid())
	{
		Response->SetObjectField(TEXT("build"), BuildInfo);
	}
	else
	{
		Response->SetField(TEXT("build"), MakeShared<FJsonValueNull>());
	}
	Response->SetField(TEXT("map"), MordecaiAgentBridge::MakeStringOrNull(Ready.Map, !Ready.bHasWorld));
	Response->SetField(TEXT("experience"), MordecaiAgentBridge::MakeStringOrNull(Ready.Experience, Ready.Experience.IsEmpty()));
	Response->SetBoolField(TEXT("experience_loaded"), Ready.bExperienceLoaded);
	if (Ready.bLoadingScreenKnown)
	{
		Response->SetBoolField(TEXT("loading_screen"), Ready.bLoadingScreen);
	}
	else
	{
		Response->SetField(TEXT("loading_screen"), MakeShared<FJsonValueNull>());
	}
	Response->SetBoolField(TEXT("has_pawn"), Ready.bHasPawn);
	Response->SetBoolField(TEXT("ready"), Ready.IsReady());
	Response->SetBoolField(TEXT("paused"), Ready.bPaused);
	Response->SetField(TEXT("real_time_s"), MordecaiAgentBridge::MakeNumber(FPlatformTime::Seconds() - GStartTime));
	Response->SetField(TEXT("world_time_s"), Context.World ? MordecaiAgentBridge::MakeNumber(Context.World->GetTimeSeconds()) : MordecaiAgentBridge::MakeStringOrNull(FString(), true));
	Response->SetNumberField(TEXT("frame"), static_cast<double>(GFrameCounter));
	Send(Connection, Response);
}

void FMordecaiAgentBridgeServer::HandleState(FConnection& Connection, const FMordecaiAgentBridgeRequest& Request)
{
	MordecaiAgentBridge::FStateCommandArgs Args;
	FMordecaiAgentBridgeError Error;
	if (!MordecaiAgentBridge::ParseStateArgs(*Request.Args, Args, Error))
	{
		Send(Connection, MordecaiAgentBridge::MakeErrorResponse(Request.Id, Request.Cmd, Error.Code, Error.Message));
		return;
	}
	FMordecaiAgentBridgeStateContext Context = FMordecaiAgentBridgeStateContext::FromGameInstance(GameInstance.Get());
	FillBridgeContext(Context);

	const TSharedRef<FJsonObject> Response = MordecaiAgentBridge::MakeOkResponse(Request.Id, Request.Cmd);
	Response->SetObjectField(TEXT("state"), MordecaiAgentBridge::State::GatherState(Context, Args.Sections, Args.Radius, Args.bIncludeAllActors));
	Send(Connection, Response);
}

// ---------------------------------------------------------------------------
// key / axis
// ---------------------------------------------------------------------------

void FMordecaiAgentBridgeServer::HandleKey(FConnection& Connection, const FMordecaiAgentBridgeRequest& Request, double Now)
{
	using namespace MordecaiAgentBridge;
	FKeyCommandArgs Args;
	FMordecaiAgentBridgeError Error;
	if (!ParseKeyArgs(*Request.Args, Args, Error))
	{
		Send(Connection, MakeErrorResponse(Request.Id, Request.Cmd, Error.Code, Error.Message));
		return;
	}
	FString Reason;
	UGameViewportClient* ViewportClient = GetViewportClient();
	if (!FMordecaiAgentBridgeInput::IsReady(ViewportClient, Reason))
	{
		Send(Connection, MakeErrorResponse(Request.Id, Request.Cmd, ErrNotReady, Reason));
		return;
	}
	if (Args.Action != EKeyAction::Release)
	{
		for (int32 Index = 0; Index < Args.Keys.Num(); ++Index)
		{
			// Only this connection's own holds: another connection holding the key just shares it
			if (HeldUnion.HoldsKey(Connection.Serial, Args.Keys[Index]))
			{
				Send(Connection, MakeErrorResponse(Request.Id, Request.Cmd, ErrBadRequest,
					FString::Printf(TEXT("key '%s' is already held by this connection; release it first"), *Args.KeyNames[Index])));
				return;
			}
		}
	}

	const bool bReady = IsGameReady();
	const FMordecaiAgentBridgeEnv Env = Input.PrepareEnvironment(Args.Route, ViewportClient);
	const TSharedRef<FJsonObject> Response = MakeOkResponse(Request.Id, Request.Cmd);
	Response->SetField(TEXT("keys"), MordecaiAgentBridgeServerPrivate::ToJsonArray(Args.KeyNames));
	Response->SetStringField(TEXT("action"), KeyActionToString(Args.Action));
	Response->SetStringField(TEXT("route"), RouteToString(Args.Route));
	if (Args.Route == ERoute::Viewport)
	{
		Response->SetBoolField(TEXT("diagnostic"), true);
	}
	AddInputFields(Response, bReady, Env);

	if (Args.Action == EKeyAction::Press)
	{
		for (const FKey& Key : Args.Keys)
		{
			PressKey(Connection, Key, Args.Route, Now);
		}
		Send(Connection, Response);
		return;
	}
	if (Args.Action == EKeyAction::Release)
	{
		TArray<TSharedPtr<FJsonValue>> WasHeld;
		for (const FKey& Key : Args.Keys)
		{
			WasHeld.Add(MakeShared<FJsonValueBoolean>(ReleaseKey(Connection, Key)));
		}
		if (Args.bListSyntax)
		{
			Response->SetArrayField(TEXT("was_held"), WasHeld);
		}
		else
		{
			Response->SetField(TEXT("was_held"), WasHeld[0]);
		}
		Send(Connection, Response);
		return;
	}

	// Tap: all keys down this frame, all up together once the game has simulated hold_s with them
	// down, after 2 frames at least (FMordecaiAgentBridgeHoldClock)
	for (const FKey& Key : Args.Keys)
	{
		PressKey(Connection, Key, Args.Route, Now);
	}
	FMordecaiAgentBridgeHoldClock Clock(Args.HoldSeconds, 2, Now, GFrameCounter, 10.0,
		MordecaiAgentBridgeServerPrivate::MaxFrameSeconds(ViewportClient));
	const TArray<FKey> Keys = Args.Keys;
	FConnection* ConnectionPtr = &Connection;

	TUniquePtr<FPendingCommand> Pending = MakeUnique<FPendingCommand>();
	Pending->Id = Request.Id;
	Pending->Cmd = Request.Cmd;
	Pending->Advance = [this, ConnectionPtr, Response, Keys, Clock](double TickNow) mutable
	{
		if (!MordecaiAgentBridgeServerPrivate::AdvanceHold(Clock, TickNow))
		{
			return false;
		}
		for (const FKey& Key : Keys)
		{
			ReleaseKey(*ConnectionPtr, Key);
		}
		MordecaiAgentBridgeServerPrivate::AddHoldFields(Response, Clock, TickNow);
		Send(*ConnectionPtr, Response);
		return true;
	};
	Connection.Active = MoveTemp(Pending);
}

void FMordecaiAgentBridgeServer::HandleAxis(FConnection& Connection, const FMordecaiAgentBridgeRequest& Request, double Now)
{
	using namespace MordecaiAgentBridge;
	FAxisCommandArgs Args;
	FMordecaiAgentBridgeError Error;
	if (!ParseAxisArgs(*Request.Args, Args, Error))
	{
		Send(Connection, MakeErrorResponse(Request.Id, Request.Cmd, Error.Code, Error.Message));
		return;
	}
	FString Reason;
	UGameViewportClient* ViewportClient = GetViewportClient();
	if (!FMordecaiAgentBridgeInput::IsReady(ViewportClient, Reason))
	{
		Send(Connection, MakeErrorResponse(Request.Id, Request.Cmd, ErrNotReady, Reason));
		return;
	}

	const bool bReady = IsGameReady();
	const FMordecaiAgentBridgeEnv Env = Input.PrepareEnvironment(Args.Route, ViewportClient);
	const TSharedRef<FJsonObject> Response = MakeOkResponse(Request.Id, Request.Cmd);
	const TSharedRef<FJsonObject> Values = MakeShared<FJsonObject>();
	for (int32 Index = 0; Index < Args.Keys.Num(); ++Index)
	{
		Values->SetField(Args.KeyNames[Index], MakeNumber(Args.Values[Index]));
		// A held value is sent by UpdateHeldInput later this tick (one sample per frame); a release now
		SetAxis(Connection, Args.Keys[Index], Args.Values[Index], Args.Route);
	}
	Response->SetObjectField(TEXT("values"), Values);
	Response->SetStringField(TEXT("route"), RouteToString(Args.Route));
	if (Args.Route == ERoute::Viewport)
	{
		Response->SetBoolField(TEXT("diagnostic"), true);
	}
	AddInputFields(Response, bReady, Env);

	if (!Args.HoldSeconds.IsSet())
	{
		// Persistent: re-sent every frame until the next axis command for that key (0 releases)
		Response->SetField(TEXT("hold_s"), MakeShared<FJsonValueNull>());
		Send(Connection, Response);
		return;
	}

	const double HoldSeconds = Args.HoldSeconds.GetValue();
	Response->SetField(TEXT("hold_s"), MakeNumber(HoldSeconds));
	FMordecaiAgentBridgeHoldClock Clock(HoldSeconds, 1, Now, GFrameCounter, 10.0,
		MordecaiAgentBridgeServerPrivate::MaxFrameSeconds(ViewportClient));
	const TArray<FKey> Keys = Args.Keys;
	const ERoute Route = Args.Route;
	FConnection* ConnectionPtr = &Connection;

	TUniquePtr<FPendingCommand> Pending = MakeUnique<FPendingCommand>();
	Pending->Id = Request.Id;
	Pending->Cmd = Request.Cmd;
	Pending->Advance = [this, ConnectionPtr, Response, Keys, Route, Clock](double TickNow) mutable
	{
		if (!MordecaiAgentBridgeServerPrivate::AdvanceHold(Clock, TickNow))
		{
			return false;
		}
		for (const FKey& Key : Keys)
		{
			SetAxis(*ConnectionPtr, Key, 0.f, Route);
		}
		MordecaiAgentBridgeServerPrivate::AddHoldFields(Response, Clock, TickNow);
		Send(*ConnectionPtr, Response);
		return true;
	};
	Connection.Active = MoveTemp(Pending);
}

// ---------------------------------------------------------------------------
// mouse / click
// ---------------------------------------------------------------------------

void FMordecaiAgentBridgeServer::HandleMouse(FConnection& Connection, const FMordecaiAgentBridgeRequest& Request)
{
	using namespace MordecaiAgentBridge;
	FMouseCommandArgs Args;
	FMordecaiAgentBridgeError Error;
	if (!ParseMouseArgs(*Request.Args, Args, Error))
	{
		Send(Connection, MakeErrorResponse(Request.Id, Request.Cmd, Error.Code, Error.Message));
		return;
	}
	FString Reason;
	UGameViewportClient* ViewportClient = GetViewportClient();
	const FMordecaiAgentBridgeViewportTransform Transform = FMordecaiAgentBridgeViewportTransform::FromViewportClient(ViewportClient);
	if (!FMordecaiAgentBridgeInput::IsReady(ViewportClient, Reason) || !Transform.bValid)
	{
		Send(Connection, MakeErrorResponse(Request.Id, Request.Cmd, ErrNotReady, Reason.IsEmpty() ? FString(TEXT("the game viewport has no geometry yet")) : Reason));
		return;
	}

	const bool bReady = IsGameReady();
	FMordecaiAgentBridgeEnv Env = Input.PrepareEnvironment(ERoute::Slate, ViewportClient);
	const FVector2D Absolute = Transform.ToAbsolute(Args.Pixel);
	Input.MoveCursor(Absolute, CurrentModifiers());
	Env.bVirtualCursor = FMordecaiAgentBridgeInput::IsVirtualCursorActive();

	const TSharedRef<FJsonObject> Response = MakeOkResponse(Request.Id, Request.Cmd);
	Response->SetField(TEXT("x"), MakeNumber(Args.Pixel.X, 1));
	Response->SetField(TEXT("y"), MakeNumber(Args.Pixel.Y, 1));
	Response->SetField(TEXT("abs"), MakeVector2D(Absolute));
	AddInputFields(Response, bReady, Env);
	Send(Connection, Response);
}

void FMordecaiAgentBridgeServer::HandleClick(FConnection& Connection, const FMordecaiAgentBridgeRequest& Request, double Now)
{
	using namespace MordecaiAgentBridge;
	FClickCommandArgs Args;
	FMordecaiAgentBridgeError Error;
	if (!ParseClickArgs(*Request.Args, Args, Error))
	{
		Send(Connection, MakeErrorResponse(Request.Id, Request.Cmd, Error.Code, Error.Message));
		return;
	}
	FString Reason;
	UGameViewportClient* ViewportClient = GetViewportClient();
	const FMordecaiAgentBridgeViewportTransform Transform = FMordecaiAgentBridgeViewportTransform::FromViewportClient(ViewportClient);
	if (!FMordecaiAgentBridgeInput::IsReady(ViewportClient, Reason) || !Transform.bValid)
	{
		Send(Connection, MakeErrorResponse(Request.Id, Request.Cmd, ErrNotReady, Reason.IsEmpty() ? FString(TEXT("the game viewport has no geometry yet")) : Reason));
		return;
	}
	// One mouse: a button another click still holds cannot go down again
	if (IsMouseButtonHeld(Args.Button))
	{
		Send(Connection, MakeErrorResponse(Request.Id, Request.Cmd, ErrBadRequest, FString::Printf(TEXT("button %s is already held"), *Args.ButtonName)));
		return;
	}

	const bool bReady = IsGameReady();
	FMordecaiAgentBridgeEnv Env = Input.PrepareEnvironment(ERoute::Slate, ViewportClient);
	Input.EnsureVirtualCursor();
	const FVector2D Absolute = Args.Pixel.IsSet() ? Transform.ToAbsolute(Args.Pixel.GetValue()) : FMordecaiAgentBridgeInput::GetCursorAbsolute();
	const FVector2D Pixel = Args.Pixel.IsSet() ? Args.Pixel.GetValue() : Transform.ToViewport(Absolute);

	// Frame N: move there and press (the in-memory cursor is re-asserted after the down)
	const FString Hit = Input.PressMouseButton(Args.Button, Absolute, CurrentModifiers());
	Connection.HeldButtons.Add({ Args.Button, Absolute });
	Env.bVirtualCursor = FMordecaiAgentBridgeInput::IsVirtualCursorActive();

	const TSharedRef<FJsonObject> Response = MakeOkResponse(Request.Id, Request.Cmd);
	Response->SetField(TEXT("x"), MakeNumber(Pixel.X, 1));
	Response->SetField(TEXT("y"), MakeNumber(Pixel.Y, 1));
	Response->SetField(TEXT("abs"), MakeVector2D(Absolute));
	Response->SetStringField(TEXT("button"), Args.ButtonName);
	Response->SetField(TEXT("hit"), MakeStringOrNull(Hit, Hit.IsEmpty()));
	AddInputFields(Response, bReady, Env);

	FMordecaiAgentBridgeHoldClock Clock(Args.HoldSeconds, 1, Now, GFrameCounter, 10.0,
		MordecaiAgentBridgeServerPrivate::MaxFrameSeconds(ViewportClient));
	const FKey Button = Args.Button;
	FConnection* ConnectionPtr = &Connection;

	TUniquePtr<FPendingCommand> Pending = MakeUnique<FPendingCommand>();
	Pending->Id = Request.Id;
	Pending->Cmd = Request.Cmd;
	Pending->Advance = [this, ConnectionPtr, Response, Env, Button, Absolute, Clock](double TickNow) mutable
	{
		if (!MordecaiAgentBridgeServerPrivate::AdvanceHold(Clock, TickNow))
		{
			// Keep the in-memory cursor on the button while it is down (UpdateHeldInput does it too)
			Input.ReassertVirtualCursor(Absolute);
			return false;
		}
		// Once hold_s is simulated (1 frame at least): a real move to the same position, then the up there
		Input.ReleaseMouseButton(Button, Absolute, CurrentModifiers());
		ConnectionPtr->HeldButtons.RemoveAll([&](const FHeldButton& Held) { return Held.Key == Button; });
		Env.bVirtualCursor = Env.bVirtualCursor && FMordecaiAgentBridgeInput::IsVirtualCursorActive();
		Response->SetObjectField(TEXT("env"), Env.ToJson());
		MordecaiAgentBridgeServerPrivate::AddHoldFields(Response, Clock, TickNow);
		Send(*ConnectionPtr, Response);
		return true;
	};
	Connection.Active = MoveTemp(Pending);
}

// ---------------------------------------------------------------------------
// screenshot / wait / wait_until / quit
// ---------------------------------------------------------------------------

void FMordecaiAgentBridgeServer::HandleScreenshot(FConnection& Connection, const FMordecaiAgentBridgeRequest& Request, double Now)
{
	using namespace MordecaiAgentBridge;
	FScreenshotCommandArgs Args;
	FMordecaiAgentBridgeError Error;
	if (!ParseScreenshotArgs(*Request.Args, Args, Error))
	{
		Send(Connection, MakeErrorResponse(Request.Id, Request.Cmd, Error.Code, Error.Message));
		return;
	}

	FString Path = Args.Path.IsEmpty()
		? Config.ShotDir / FString::Printf(TEXT("bridge_%02d.png"), NextScreenshotNumber++)
		: Args.Path;
	Path = FPaths::ConvertRelativePathToFull(Path);
	if (FPaths::GetExtension(Path).IsEmpty())
	{
		Path += TEXT(".png");
	}
	IFileManager::Get().MakeDirectory(*FPaths::GetPath(Path), /*Tree*/ true);
	// A stale file must not count as this screenshot
	IFileManager::Get().Delete(*Path, /*RequireExists*/ false, /*EvenReadOnly*/ true, /*Quiet*/ true);

	FConnection* ConnectionPtr = &Connection;
	const TSharedPtr<FJsonValue> Id = Request.Id;
	const FString Cmd = Request.Cmd;
	bool bRequested = false;
	uint64 ProcessedAtRequest = 0;

	TUniquePtr<FPendingCommand> Pending = MakeUnique<FPendingCommand>();
	Pending->Id = Id;
	Pending->Cmd = Cmd;
	Pending->Advance = [this, ConnectionPtr, Id, Cmd, Path, Now, bRequested, ProcessedAtRequest](double TickNow) mutable
	{
		if (!bRequested)
		{
			// One global request slot, shared with -PlaytestCapture: wait until it is free
			if (!FScreenshotRequest::IsScreenshotRequested())
			{
				FScreenshotRequest::RequestScreenshot(Path, /*bInShowUI*/ true, /*bAddFilenameSuffix*/ false);
				bRequested = true;
				ProcessedAtRequest = ScreenshotsProcessed;
			}
		}
		else
		{
			const bool bProcessed = ScreenshotsProcessed > ProcessedAtRequest || !FScreenshotRequest::IsScreenshotRequested();
			if (bProcessed)
			{
				const int64 Bytes = IFileManager::Get().FileSize(*Path);
				if (Bytes > 0)
				{
					const TSharedRef<FJsonObject> Response = MakeOkResponse(Id, Cmd);
					Response->SetStringField(TEXT("path"), Path);
					Response->SetNumberField(TEXT("bytes"), static_cast<double>(Bytes));
					Send(*ConnectionPtr, Response);
				}
				else
				{
					Send(*ConnectionPtr, MakeErrorResponse(Id, Cmd, ErrFailed, FString::Printf(TEXT("the screenshot was processed but %s is missing or empty"), *Path)));
				}
				return true;
			}
		}
		if (TickNow - Now >= ScreenshotTimeoutSeconds)
		{
			Send(*ConnectionPtr, MakeErrorResponse(Id, Cmd, ErrTimeout, FString::Printf(
				TEXT("%s was not written within %.0fs (a minimized window renders nothing)"), *Path, ScreenshotTimeoutSeconds)));
			return true;
		}
		return false;
	};
	Connection.Active = MoveTemp(Pending);
}

void FMordecaiAgentBridgeServer::HandleWait(FConnection& Connection, const FMordecaiAgentBridgeRequest& Request, double Now)
{
	using namespace MordecaiAgentBridge;
	FWaitCommandArgs Args;
	FMordecaiAgentBridgeError Error;
	if (!ParseWaitArgs(*Request.Args, Args, Error))
	{
		Send(Connection, MakeErrorResponse(Request.Id, Request.Cmd, Error.Code, Error.Message));
		return;
	}
	FConnection* ConnectionPtr = &Connection;
	const TSharedRef<FJsonObject> Response = MakeOkResponse(Request.Id, Request.Cmd);
	Response->SetField(TEXT("seconds"), MakeNumber(Args.Seconds));
	const double Seconds = Args.Seconds;

	TUniquePtr<FPendingCommand> Pending = MakeUnique<FPendingCommand>();
	Pending->Id = Request.Id;
	Pending->Cmd = Request.Cmd;
	Pending->Advance = [this, ConnectionPtr, Response, Now, Seconds](double TickNow)
	{
		if (TickNow - Now < Seconds)
		{
			return false;
		}
		Response->SetField(TEXT("elapsed_s"), MordecaiAgentBridge::MakeNumber(TickNow - Now));
		Send(*ConnectionPtr, Response);
		return true;
	};
	Connection.Active = MoveTemp(Pending);
}

void FMordecaiAgentBridgeServer::HandleWaitUntil(FConnection& Connection, const FMordecaiAgentBridgeRequest& Request, double Now)
{
	using namespace MordecaiAgentBridge;
	FWaitUntilCommandArgs Args;
	FMordecaiAgentBridgeError Error;
	if (!ParseWaitUntilArgs(*Request.Args, Args, Error))
	{
		Send(Connection, MakeErrorResponse(Request.Id, Request.Cmd, Error.Code, Error.Message));
		return;
	}
	FString ParseError;
	const TSharedPtr<FMordecaiAgentBridgePredicate> Predicate = FMordecaiAgentBridgePredicate::Parse(Args.Condition, ParseError);
	if (!Predicate.IsValid())
	{
		Send(Connection, MakeErrorResponse(Request.Id, Request.Cmd, ErrBadRequest, FString::Printf(TEXT("bad condition: %s"), *ParseError)));
		return;
	}

	// Gather only the sections the condition's paths start with
	TArray<FString> Sections;
	for (const FString& Root : Predicate->GetRootIdentifiers())
	{
		if (GetStateSectionNames().Contains(Root))
		{
			Sections.Add(Root);
		}
	}

	FConnection* ConnectionPtr = &Connection;
	const TSharedPtr<FJsonValue> Id = Request.Id;
	const FString Cmd = Request.Cmd;
	const double Timeout = Args.TimeoutSeconds;
	const double Poll = Args.PollSeconds;
	double NextPoll = Now;
	TSharedPtr<FJsonObject> LastValues = MakeShared<FJsonObject>();

	TUniquePtr<FPendingCommand> Pending = MakeUnique<FPendingCommand>();
	Pending->Id = Id;
	Pending->Cmd = Cmd;
	Pending->Advance = [this, ConnectionPtr, Id, Cmd, Predicate, Sections, Timeout, Poll, Now, NextPoll, LastValues](double TickNow) mutable
	{
		if (TickNow >= NextPoll)
		{
			NextPoll = TickNow + Poll; // poll_s, but never more than once per frame (one evaluation per tick)
			FMordecaiAgentBridgeStateContext Context = FMordecaiAgentBridgeStateContext::FromGameInstance(GameInstance.Get());
			FillBridgeContext(Context);
			// Sections empty means the condition names no state section: evaluate against an empty state
			const TSharedRef<FJsonObject> State = Sections.Num() > 0
				? MordecaiAgentBridge::State::GatherState(Context, Sections, 1500.0, false)
				: MakeShared<FJsonObject>();
			const bool bMet = Predicate->Evaluate(State, &LastValues);
			if (bMet)
			{
				const TSharedRef<FJsonObject> Response = MakeOkResponse(Id, Cmd);
				Response->SetField(TEXT("elapsed_s"), MordecaiAgentBridge::MakeNumber(TickNow - Now));
				Response->SetObjectField(TEXT("values"), LastValues);
				Send(*ConnectionPtr, Response);
				return true;
			}
		}
		if (TickNow - Now >= Timeout)
		{
			const TSharedRef<FJsonObject> Response = MakeErrorResponse(Id, Cmd, ErrTimeout, FString::Printf(
				TEXT("condition not met within %gs: %s"), Timeout, *Predicate->GetExpression()));
			Response->SetField(TEXT("elapsed_s"), MordecaiAgentBridge::MakeNumber(TickNow - Now));
			Response->SetObjectField(TEXT("values"), LastValues);
			Send(*ConnectionPtr, Response);
			return true;
		}
		return false;
	};
	Connection.Active = MoveTemp(Pending);
}

void FMordecaiAgentBridgeServer::HandleQuit(FConnection& Connection, const FMordecaiAgentBridgeRequest& Request)
{
	using namespace MordecaiAgentBridge;
	Send(Connection, MakeOkResponse(Request.Id, Request.Cmd));

	for (TUniquePtr<FConnection>& Other : Connections)
	{
		// Pending and queued commands everywhere are cancelled, and all held input is released
		if (Other->Active.IsValid())
		{
			Send(*Other, MakeErrorResponse(Other->Active->Id, Other->Active->Cmd, ErrCancelled, TEXT("cancelled by quit")));
			Other->Active.Reset();
		}
		for (const FMordecaiAgentBridgeFramedLine& Line : Other->Queue)
		{
			FMordecaiAgentBridgeRequest Queued;
			FString Ignored;
			ParseRequest(Line.Text, Queued, Ignored);
			Send(*Other, MakeErrorResponse(Queued.Id, Queued.Cmd, ErrCancelled, TEXT("cancelled by quit")));
		}
		Other->Queue.Reset();
		ReleaseAllHeldInput(*Other);
	}
	for (TUniquePtr<FConnection>& Other : Connections)
	{
		Flush(*Other);
	}
	bQuitRequested = true; // RequestEngineExit on the next tick
}

// ---------------------------------------------------------------------------
// Held input (the server-wide union)
// ---------------------------------------------------------------------------

FModifierKeysState FMordecaiAgentBridgeServer::CurrentModifiers() const
{
	return MordecaiAgentBridge::MakeModifierKeysState(HeldUnion.GetDownKeys());
}

bool FMordecaiAgentBridgeServer::PressKey(FConnection& Connection, const FKey& Key, MordecaiAgentBridge::ERoute Route, double Now)
{
	if (HeldUnion.HoldsKey(Connection.Serial, Key))
	{
		return false;
	}
	if (HeldUnion.AddKey(Connection.Serial, Key, Route))
	{
		// 0 -> 1: the key goes down for Slate (the modifiers include it, as the OS reports them)
		Input.SendKey(GetViewportClient(), Key, /*bDown*/ true, /*bRepeat*/ false, CurrentModifiers(), Route);
		AutoRepeat.KeyDown(Key, Now);
	}
	return true;
}

bool FMordecaiAgentBridgeServer::ReleaseKey(FConnection& Connection, const FKey& Key)
{
	const MordecaiAgentBridge::ERoute Route = HeldUnion.GetKeyRoute(Key);
	bool bWasHeld = false;
	if (HeldUnion.RemoveKey(Connection.Serial, Key, bWasHeld))
	{
		// 1 -> 0: the last holder let go
		AutoRepeat.KeyUp(Key);
		Input.SendKey(GetViewportClient(), Key, /*bDown*/ false, /*bRepeat*/ false, CurrentModifiers(), Route);
	}
	return bWasHeld;
}

void FMordecaiAgentBridgeServer::SetAxis(FConnection& Connection, const FKey& Key, float Value, MordecaiAgentBridge::ERoute Route)
{
	const MordecaiAgentBridge::ERoute AxisRoute = HeldUnion.CountAxisHolders(Key) > 0 ? HeldUnion.GetAxisRoute(Key) : Route;
	HeldUnion.SetAxis(Connection.Serial, Key, Value, Route);
	const float* LastSent = LastSentAxisValues.Find(Key);
	if (HeldUnion.GetAxisValue(Key) == 0.f && LastSent && *LastSent != 0.f)
	{
		// Released: gamepad axes latch their last sample (PlayerInput.cpp), so send one 0 now, then let the
		// threshold buttons go up after it (XInput sends the analog values before the buttons)
		Input.SendAnalog(GetViewportClient(), Key, 0.f, CurrentModifiers(), AxisRoute);
		LastSentAxisValues.Add(Key, 0.f);
		UpdateThresholdButtons(Key, 0.f, AxisRoute, FPlatformTime::Seconds());
	}
	// A held value is not sent here: UpdateHeldInput sends exactly one sample per frame
}

bool FMordecaiAgentBridgeServer::IsMouseButtonHeld(const FKey& Button) const
{
	for (const TUniquePtr<FConnection>& Connection : Connections)
	{
		if (Connection->HeldButtons.ContainsByPredicate([&](const FHeldButton& Held) { return Held.Key == Button; }))
		{
			return true;
		}
	}
	return false;
}

void FMordecaiAgentBridgeServer::ReleaseAllHeldInput(FConnection& Connection)
{
	if (!FSlateApplication::IsInitialized())
	{
		// No Slate to tell: just forget this connection's holds
		bool bIgnored = false;
		for (const FKey& Key : HeldUnion.GetKeysHeldBy(Connection.Serial))
		{
			HeldUnion.RemoveKey(Connection.Serial, Key, bIgnored);
		}
		for (const FKey& Axis : HeldUnion.GetAxesHeldBy(Connection.Serial))
		{
			HeldUnion.SetAxis(Connection.Serial, Axis, 0.f, MordecaiAgentBridge::ERoute::Slate);
		}
		Connection.HeldButtons.Reset();
		return;
	}
	TArray<FKey> Keys = HeldUnion.GetKeysHeldBy(Connection.Serial);
	for (int32 Index = Keys.Num() - 1; Index >= 0; --Index)
	{
		ReleaseKey(Connection, Keys[Index]);
	}
	for (const FKey& Axis : HeldUnion.GetAxesHeldBy(Connection.Serial))
	{
		SetAxis(Connection, Axis, 0.f, HeldUnion.GetAxisRoute(Axis));
	}
	for (const FHeldButton& Button : Connection.HeldButtons)
	{
		Input.ReleaseMouseButton(Button.Key, Button.Absolute, CurrentModifiers());
	}
	Connection.HeldButtons.Reset();
}

void FMordecaiAgentBridgeServer::UpdateThresholdButtons(const FKey& Axis, float Value, MordecaiAgentBridge::ERoute Route, double Now)
{
	const TArray<FKey> Desired = MordecaiAgentBridge::GetAxisThresholdKeysDown(Axis, Value);
	TArray<FKey>& Down = ThresholdButtonsDown.FindOrAdd(Axis);
	if (Desired == Down)
	{
		return;
	}
	UGameViewportClient* ViewportClient = GetViewportClient();
	bool bWasHeld = false;
	for (const FKey& Key : Down)
	{
		if (!Desired.Contains(Key))
		{
			const MordecaiAgentBridge::ERoute KeyRoute = HeldUnion.GetKeyRoute(Key);
			if (HeldUnion.RemoveKey(FMordecaiAgentBridgeInputUnion::SynthesizedHolder, Key, bWasHeld))
			{
				AutoRepeat.KeyUp(Key);
				Input.SendKey(ViewportClient, Key, /*bDown*/ false, /*bRepeat*/ false, CurrentModifiers(), KeyRoute);
			}
		}
	}
	for (const FKey& Key : Desired)
	{
		if (!Down.Contains(Key) && HeldUnion.AddKey(FMordecaiAgentBridgeInputUnion::SynthesizedHolder, Key, Route))
		{
			// As XInput: the button's own down event, with the pad's device and repeat schedule
			Input.SendKey(ViewportClient, Key, /*bDown*/ true, /*bRepeat*/ false, CurrentModifiers(), Route);
			AutoRepeat.KeyDown(Key, Now);
		}
	}
	Down = Desired;
}

void FMordecaiAgentBridgeServer::UpdateHeldInput(double Now)
{
	if (!FSlateApplication::IsInitialized())
	{
		return;
	}
	UGameViewportClient* ViewportClient = GetViewportClient();

	// Keep the in-memory cursor on a held mouse button (CommonInput or real OS moves may swap it back)
	for (const TUniquePtr<FConnection>& Connection : Connections)
	{
		if (Connection->HeldButtons.Num() > 0)
		{
			Input.ReassertVirtualCursor(Connection->HeldButtons.Last().Absolute);
		}
	}

	// Held axes: exactly one sample per frame (the union value), then the threshold buttons it implies
	for (const FKey& Axis : HeldUnion.GetHeldAxes())
	{
		const float Value = HeldUnion.GetAxisValue(Axis);
		const MordecaiAgentBridge::ERoute Route = HeldUnion.GetAxisRoute(Axis);
		Input.SendAnalog(ViewportClient, Axis, Value, CurrentModifiers(), Route);
		LastSentAxisValues.Add(Axis, Value);
		UpdateThresholdButtons(Axis, Value, Route, Now);
	}

	// Auto-repeat like the devices: keyboard typematic on the newest key, gamepad buttons each on their own
	const FModifierKeysState Modifiers = CurrentModifiers();
	for (const FKey& Key : AutoRepeat.CollectRepeats(Now))
	{
		if (HeldUnion.IsKeyDown(Key))
		{
			Input.SendKey(ViewportClient, Key, /*bDown*/ true, /*bRepeat*/ true, Modifiers, HeldUnion.GetKeyRoute(Key));
		}
	}
}

TArray<FMordecaiAgentBridgeHeldInput> FMordecaiAgentBridgeServer::CollectHeldInputs() const
{
	TArray<FMordecaiAgentBridgeHeldInput> Held;
	for (const FKey& Key : HeldUnion.GetDownKeys())
	{
		const int32 Holders = HeldUnion.CountKeyHolders(Key, /*bIncludeSynthesized*/ false);
		if (Holders > 0)
		{
			Held.Add({ Key.GetFName().ToString(), TEXT("key"), 1.0, Holders });
		}
	}
	for (const FKey& Axis : HeldUnion.GetHeldAxes())
	{
		Held.Add({ Axis.GetFName().ToString(), TEXT("axis"), HeldUnion.GetAxisValue(Axis), HeldUnion.CountAxisHolders(Axis) });
	}
	TArray<TPair<FKey, int32>> Buttons;
	for (const TUniquePtr<FConnection>& Connection : Connections)
	{
		for (const FHeldButton& Button : Connection->HeldButtons)
		{
			if (TPair<FKey, int32>* Existing = Buttons.FindByPredicate([&](const TPair<FKey, int32>& Entry) { return Entry.Key == Button.Key; }))
			{
				++Existing->Value;
			}
			else
			{
				Buttons.Add({ Button.Key, 1 });
			}
		}
	}
	for (const TPair<FKey, int32>& Button : Buttons)
	{
		Held.Add({ Button.Key.GetFName().ToString(), TEXT("mouse"), 1.0, Button.Value });
	}
	return Held;
}

TArray<FString> FMordecaiAgentBridgeServer::CollectSynthesizedKeys() const
{
	TArray<FString> Keys;
	for (const FKey& Key : HeldUnion.GetDownKeys())
	{
		if (HeldUnion.HoldsKey(FMordecaiAgentBridgeInputUnion::SynthesizedHolder, Key))
		{
			Keys.Add(Key.GetFName().ToString());
		}
	}
	return Keys;
}

#endif // !UE_BUILD_SHIPPING
