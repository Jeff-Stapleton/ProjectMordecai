// Project Mordecai — Enemy AI Types (US-051)

#pragma once

#include "MordecaiEnemyAITypes.generated.h"

/**
 * EMordecaiEnemyAIState
 *
 * States for the enemy AI state machine (AC-051.2).
 * Driven by tick-based evaluation in AMordecaiEnemyAIController.
 */
UENUM(BlueprintType)
enum class EMordecaiEnemyAIState : uint8
{
	Idle        UMETA(DisplayName = "Idle"),
	Approach    UMETA(DisplayName = "Approach"),
	Attack      UMETA(DisplayName = "Attack"),
	Recover     UMETA(DisplayName = "Recover"),
	Staggered   UMETA(DisplayName = "Staggered"),
	Leash       UMETA(DisplayName = "Leash"),
	Dead        UMETA(DisplayName = "Dead"),
};
