# Enemy AI Framework v1

## Core AI Components

EnemyController BehaviorTree Blackboard

------------------------------------------------------------------------

## Enemy Archetypes

Basic Elite Boss

------------------------------------------------------------------------

## Combat Behavior

Enemies evaluate:

-   distance to player
-   cooldown availability
-   posture state

Behavior loop:

Idle → Detect Player → Engage → Attack Patterns → Recover → Repeat

------------------------------------------------------------------------

## Boss Phases

Boss fights can include multiple phases.

Example:

Phase 1: Basic attacks Phase 2: AoE attacks unlocked Phase 3: Enrage
mechanics
