// Project Mordecai — Agent bridge subsystem (Specs/agent_bridge_v1.md)

#include "Mordecai/Playtest/MordecaiAgentBridgeSubsystem.h"

#include "Engine/GameInstance.h"
#include "Misc/CommandLine.h"
#include "Mordecai/Playtest/MordecaiAgentBridgeProtocol.h"
#include "Mordecai/Playtest/MordecaiAgentBridgeServer.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(MordecaiAgentBridgeSubsystem)

bool UMordecaiAgentBridgeSubsystem::ShouldCreateSubsystem(UObject* Outer) const
{
#if UE_BUILD_SHIPPING
	return false;
#else
	return Super::ShouldCreateSubsystem(Outer) && FMordecaiAgentBridgeConfig::FromCommandLine(FCommandLine::Get()).bEnabled;
#endif
}

void UMordecaiAgentBridgeSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

#if !UE_BUILD_SHIPPING
	const FMordecaiAgentBridgeConfig Config = FMordecaiAgentBridgeConfig::FromCommandLine(FCommandLine::Get());
	FMordecaiAgentBridgeServer* NewServer = new FMordecaiAgentBridgeServer(GetGameInstance(), Config);
	if (NewServer->Start())
	{
		Server = NewServer;
		UE_LOG(LogMordecaiAgentBridge, Display, TEXT("AgentBridge: token '%s', screenshots to %s"), *Config.Token, *Config.ShotDir);
	}
	else
	{
		// Bind failure leaves the bridge inert (already logged as an error)
		delete NewServer;
	}
#endif
}

void UMordecaiAgentBridgeSubsystem::Deinitialize()
{
#if !UE_BUILD_SHIPPING
	delete Server;
#endif
	Server = nullptr;
	Super::Deinitialize();
}
