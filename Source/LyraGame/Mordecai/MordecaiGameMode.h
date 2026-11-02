// Project Mordecai

#pragma once

#include "GameModes/LyraGameMode.h"

#include "MordecaiGameMode.generated.h"

class AMordecaiCharacter;
class AMordecaiEnemyCharacter;
class APlayerState;

/** Authoritative arena kill count changed (a kill increments it; player death resets it). */
DECLARE_MULTICAST_DELEGATE_OneParam(FOnMordecaiEnemyKillCountChanged, int32 /*NewKillCount*/);

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

	/** Two input-observation frames must fit the Tier 4 light-click cadence's 0.4 s bound. */
	static constexpr float MaximumUndilatedFrameTimeSeconds = 0.2f;

	// --- Arena API ---

	/** Reset all registered enemies to full health/posture (AC-053.9). */
	UFUNCTION(BlueprintCallable, Category = "Mordecai|Arena")
	void ResetArena();

	/** Current enemy kill count for this life (AC-053.7, AC-063.2). */
	UFUNCTION(BlueprintCallable, Category = "Mordecai|Arena")
	int32 GetEnemyKillCount() const { return EnemyKillCount; }

	/** Count a death screen should show: current life during the death event, cached completed life after its reset. */
	int32 GetDeathScreenEnemyKillCount() const
	{
		return bRespawnPending ? CompletedLifeEnemyKillCount : EnemyKillCount;
	}

	/** The one notification used by HUD consumers of GetEnemyKillCount(). */
	FOnMordecaiEnemyKillCountChanged& OnEnemyKillCountChanged() { return EnemyKillCountChanged; }

	/** Called by player character on death. Starts the respawn timer. */
	void NotifyPlayerDeath(AMordecaiCharacter* DeadPlayer);

	/** Called when an enemy dies. Counts it only when KillerPlayerState is a Mordecai player. */
	void NotifyEnemyKilled(AMordecaiEnemyCharacter* KilledEnemy, APlayerState* KillerPlayerState = nullptr);

	/** Register an enemy for arena tracking (called by enemy on spawn). */
	void RegisterEnemy(AMordecaiEnemyCharacter* Enemy, const FTransform& SpawnTransform);

private:
	struct FEnemySpawnEntry
	{
		TWeakObjectPtr<AMordecaiEnemyCharacter> Enemy;
		FTransform SpawnTransform;
	};

	TArray<FEnemySpawnEntry> EnemyRegistry;
	TSet<TWeakObjectPtr<AMordecaiEnemyCharacter>> CountedEnemyDeaths;
	int32 EnemyKillCount = 0;
	int32 CompletedLifeEnemyKillCount = 0;
	bool bRespawnPending = false;
	FOnMordecaiEnemyKillCountChanged EnemyKillCountChanged;
	FTimerHandle RespawnTimerHandle;
	TWeakObjectPtr<AMordecaiCharacter> PendingRespawnPlayer;

	void ResetEnemyKillCount();
	void ExecuteRespawn();
};
