// Project Mordecai

#pragma once

#include "GameModes/LyraGameState.h"

#include "MordecaiGameState.generated.h"

/**
 * AMordecaiGameState
 *
 *	Game state for Project Mordecai. Extends Lyra's base game state.
 */
UCLASS()
class LYRAGAME_API AMordecaiGameState : public ALyraGameState
{
	GENERATED_BODY()

public:
	AMordecaiGameState(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());
};
