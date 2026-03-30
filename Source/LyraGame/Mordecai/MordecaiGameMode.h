// Project Mordecai

#pragma once

#include "GameModes/LyraGameMode.h"

#include "MordecaiGameMode.generated.h"

class AMordecaiCharacter;
class AMordecaiEnemyCharacter;

/**
 * AMordecaiGameMode
 *
 *	Game mode for Project Mordecai. Extends Lyra's base game mode.
 *	Handles player death/respawn cycle and arena game flow (US-053).
 */
UCLASS()
class LYRAGAME_API AMordecaiGameMode : public ALyraGameMode
{
	GENERATED_BODY()

public:
	AMordecaiGameMode(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	// --- Arena Configuration (US-053) ---

	/** Delay in seconds before the player is respawned after death (AC-053.4). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Mordecai|Arena")
	float RespawnDelaySeconds = 3.0f;

	/** If true, all enemies are reset when the player respawns (AC-053.8). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Mordecai|Arena")
	bool ArenaResetOnPlayerRespawn = true;

	// --- Arena API ---

	/** Reset all registered enemies to full health/posture (AC-053.9). */
	UFUNCTION(BlueprintCallable, Category = "Mordecai|Arena")
	void ResetArena();

	/** Current enemy kill count this session (AC-053.7). */
	UFUNCTION(BlueprintCallable, Category = "Mordecai|Arena")
	int32 GetEnemyKillCount() const { return EnemyKillCount; }

	/** Called by player character on death. Starts the respawn timer. */
	void NotifyPlayerDeath(AMordecaiCharacter* DeadPlayer);

	/** Called when an enemy is killed. Increments kill count. */
	void NotifyEnemyKilled(AMordecaiEnemyCharacter* KilledEnemy);

	/** Register an enemy for arena tracking (called by enemy on spawn). */
	void RegisterEnemy(AMordecaiEnemyCharacter* Enemy, const FTransform& SpawnTransform);

private:
	struct FEnemySpawnEntry
	{
		TWeakObjectPtr<AMordecaiEnemyCharacter> Enemy;
		FTransform SpawnTransform;
	};

	TArray<FEnemySpawnEntry> EnemyRegistry;
	int32 EnemyKillCount = 0;
	FTimerHandle RespawnTimerHandle;
	TWeakObjectPtr<AMordecaiCharacter> PendingRespawnPlayer;

	void ExecuteRespawn();
};
