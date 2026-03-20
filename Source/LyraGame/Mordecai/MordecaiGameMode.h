// Project Mordecai

#pragma once

#include "GameModes/LyraGameMode.h"

#include "MordecaiGameMode.generated.h"

/**
 * AMordecaiGameMode
 *
 *	Game mode for Project Mordecai. Extends Lyra's base game mode.
 */
UCLASS()
class LYRAGAME_API AMordecaiGameMode : public ALyraGameMode
{
	GENERATED_BODY()

public:
	AMordecaiGameMode(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());
};
