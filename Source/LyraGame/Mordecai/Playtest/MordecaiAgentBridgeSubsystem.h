// Project Mordecai — Agent bridge subsystem (Specs/agent_bridge_v1.md)

#pragma once

#include "Subsystems/GameInstanceSubsystem.h"

#include "MordecaiAgentBridgeSubsystem.generated.h"

class FMordecaiAgentBridgeServer;

/**
 * UMordecaiAgentBridgeSubsystem
 *
 *	Lets an agent play the packaged game: a localhost NDJSON server (127.0.0.1, -AgentBridgePort=)
 *	that injects real input through Slate (keys, gamepad axes, a virtual mouse cursor), reports game
 *	state, waits on state predicates and takes screenshots. It never calls gameplay or UI functions as
 *	a stand-in for input. Only created in non-Shipping builds launched with the exact -AgentBridge
 *	token; the server and the input injection are compiled out of Shipping.
 */
UCLASS()
class UMordecaiAgentBridgeSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	virtual bool ShouldCreateSubsystem(UObject* Outer) const override;
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	/** True while the server is listening (false in Shipping or when the port could not be bound). */
	bool IsListening() const { return Server != nullptr; }

private:
	/** Owned; deleted in Deinitialize. Never set in Shipping. */
	FMordecaiAgentBridgeServer* Server = nullptr;
};
