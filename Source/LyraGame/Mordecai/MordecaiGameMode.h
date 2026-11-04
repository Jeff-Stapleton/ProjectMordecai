// Project Mordecai

#pragma once

#include "GameModes/LyraGameMode.h"

#include "MordecaiGameMode.generated.h"

class AMordecaiCharacter;
class AMordecaiEnemyCharacter;
class APlayerState;

/** The player's kill count changed (a kill, the player's death, a manual arena reset). */
DECLARE_MULTICAST_DELEGATE_OneParam(FOnMordecaiEnemyKillCountChanged, int32 /*KillCount*/);

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

	virtual void InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage) override;

	// --- Experience Selection ---

	/**
	 * Experience used when nothing else names one. Without this, Lyra falls back to
	 * B_LyraDefaultExperience (the shooter) in packaged builds, because Mordecai maps
	 * don't set DefaultGameplayExperience in World Settings.
	 */
	UPROPERTY(EditDefaultsOnly, Category = "Mordecai|Experience", meta = (AllowedTypes = "LyraExperienceDefinition"))
	FPrimaryAssetId DefaultExperience = FPrimaryAssetId(FPrimaryAssetType(TEXT("LyraExperienceDefinition")), FName(TEXT("B_MordecaiArenaExperience")));

	/**
	 * True when no higher-priority source picks the experience, so DefaultExperience should be used.
	 * Mirrors the sources ALyraGameMode::HandleMatchAssignmentIfNotExpectingOne checks:
	 * URL options, PIE developer settings, command line, World Settings.
	 */
	static bool ShouldUseDefaultExperience(const FString& Options, const TCHAR* CommandLine, bool bDeveloperOverrideApplies, bool bWorldSettingsHasExperience);

	// --- Arena Configuration (US-053) ---

	/** Delay in seconds before the player is respawned after death (AC-053.4). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Mordecai|Arena")
	float RespawnDelaySeconds = 3.0f;

	/** If true, all enemies are reset when the player respawns (AC-053.8). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Mordecai|Arena")
	bool ArenaResetOnPlayerRespawn = true;

	// --- Arena API ---

	/** Reset all registered enemies to full health/posture (AC-053.9), and the kill count. The respawn resets the
	 *  enemies only: the player's death has already reset the count. */
	UFUNCTION(BlueprintCallable, Category = "Mordecai|Arena")
	void ResetArena();

	/**
	 * The enemies the player has killed this life (AC-053.7, issue #6): the one count the HUD kill counter and the
	 * agent bridge's player.kills show. It goes back to 0 when the player dies.
	 * Single-player: one count for the local player (per-player counts are issue #77).
	 */
	UFUNCTION(BlueprintCallable, Category = "Mordecai|Arena")
	int32 GetEnemyKillCount() const { return EnemyKillCount; }

	/** The kills of the life that ended at the player's last death: what the death screen shows (AC-062.2). */
	UFUNCTION(BlueprintCallable, Category = "Mordecai|Arena")
	int32 GetCompletedLifeKillCount() const { return CompletedLifeKillCount; }

	/** Broadcast with the new GetEnemyKillCount() whenever it changes. */
	FOnMordecaiEnemyKillCountChanged OnEnemyKillCountChanged;

	/** Called by the player character when it dies, before it shows as dead: ends the life's kill count (its total
	 *  for the death screen, the live count back to 0) and starts the respawn timer. */
	void NotifyPlayerDeath(AMordecaiCharacter* DeadPlayer);

	/**
	 * Called when an enemy dies. Killer is the player whose damage killed it (the killing blow's effect context), or
	 * null when no player did. Only a living player's kill counts: an enemy that dies to anything else, or to a
	 * projectile or DoT that lands while its player is dead, scores nothing.
	 */
	void NotifyEnemyKilled(AMordecaiEnemyCharacter* KilledEnemy, APlayerState* Killer);

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
	int32 CompletedLifeKillCount = 0;
	FTimerHandle RespawnTimerHandle;
	TWeakObjectPtr<AMordecaiCharacter> PendingRespawnPlayer;

	void ExecuteRespawn();
	void ResetEnemies();
	void SetEnemyKillCount(int32 NewCount);
};
