// Project Mordecai

#pragma once

#include "Player/LyraPlayerController.h"

#include "MordecaiPlayerController.generated.h"

/**
 * AMordecaiPlayerController
 *
 *	Player controller for Project Mordecai. Extends Lyra's base controller.
 */
UCLASS()
class LYRAGAME_API AMordecaiPlayerController : public ALyraPlayerController
{
	GENERATED_BODY()

public:
	AMordecaiPlayerController(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());
};
