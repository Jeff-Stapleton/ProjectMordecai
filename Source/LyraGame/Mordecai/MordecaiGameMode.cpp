// Project Mordecai

#include "MordecaiGameMode.h"

#include "Development/LyraDeveloperSettings.h"
#include "GameModes/LyraWorldSettings.h"
#include "Kismet/GameplayStatics.h"
#include "Mordecai/MordecaiCharacter.h"
#include "Mordecai/MordecaiPlayerController.h"
#include "Mordecai/MordecaiPlayerState.h"
#include "Mordecai/MordecaiGameState.h"
#include "Mordecai/Enemy/MordecaiEnemyCharacter.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(MordecaiGameMode)

AMordecaiGameMode::AMordecaiGameMode(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	GameStateClass = AMordecaiGameState::StaticClass();
	PlayerControllerClass = AMordecaiPlayerController::StaticClass();
	PlayerStateClass = AMordecaiPlayerState::StaticClass();
	DefaultPawnClass = AMordecaiCharacter::StaticClass();
}

void AMordecaiGameMode::InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage)
{
	Super::InitGame(MapName, Options, ErrorMessage);

	// Lyra picks the experience on the next tick from OptionsString, so naming ours here works in
	// PIE and packaged builds alike. The previous PIE-only ExperienceOverride left packaged builds
	// on B_LyraDefaultExperience (the shooter).
	const UWorld* World = GetWorld();
	const ALyraWorldSettings* WorldSettings = Cast<ALyraWorldSettings>(GetWorldSettings());
	const bool bDeveloperOverrideApplies = World && World->IsPlayInEditor()
		&& GetDefault<ULyraDeveloperSettings>()->ExperienceOverride.IsValid();
	const bool bWorldSettingsHasExperience = WorldSettings && WorldSettings->GetDefaultGameplayExperience().IsValid();

	if (DefaultExperience.IsValid()
		&& ShouldUseDefaultExperience(OptionsString, FCommandLine::Get(), bDeveloperOverrideApplies, bWorldSettingsHasExperience))
	{
		OptionsString += FString::Printf(TEXT("?Experience=%s"), *DefaultExperience.PrimaryAssetName.ToString());
	}
}

bool AMordecaiGameMode::ShouldUseDefaultExperience(const FString& Options, const TCHAR* CommandLine, bool bDeveloperOverrideApplies, bool bWorldSettingsHasExperience)
{
	FString ExperienceFromCommandLine;
	const bool bCommandLineHasExperience = CommandLine && FParse::Value(CommandLine, TEXT("Experience="), ExperienceFromCommandLine);

	return !UGameplayStatics::HasOption(Options, TEXT("Experience"))
		&& !bCommandLineHasExperience
		&& !bDeveloperOverrideApplies
		&& !bWorldSettingsHasExperience;
}

// ---------------------------------------------------------------------------
// Arena: Player Death / Respawn (US-053)
// ---------------------------------------------------------------------------

void AMordecaiGameMode::NotifyPlayerDeath(AMordecaiCharacter* DeadPlayer)
{
	if (!DeadPlayer || bRespawnPending)
	{
		return;
	}

	// HandlePlayerDeath broadcasts Mordecai.Event.PlayerDeath before it notifies the game mode. The death screen
	// therefore caches the completed life's count first; then this authoritative count and every bound HUD reset.
	CompletedLifeEnemyKillCount = EnemyKillCount;
	bRespawnPending = true;
	PendingRespawnPlayer = DeadPlayer;
	ResetEnemyKillCount();

	// Start respawn timer (AC-053.4, AC-053.11: server-authoritative)
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimer(
			RespawnTimerHandle, this,
			&AMordecaiGameMode::ExecuteRespawn,
			RespawnDelaySeconds, false);
	}
}

void AMordecaiGameMode::ExecuteRespawn()
{
	// A delayed projectile/DoT death cannot become the first kill of the next life, even when arena reset is disabled.
	ResetEnemyKillCount();

	AMordecaiCharacter* Player = PendingRespawnPlayer.Get();
	if (!Player)
	{
		bRespawnPending = false;
		PendingRespawnPlayer.Reset();
		return;
	}

	// Teleport to spawn point
	if (UWorld* World = GetWorld())
	{
		AActor* SpawnPoint = FindPlayerStart(Player->GetController());
		if (SpawnPoint)
		{
			Player->SetActorLocationAndRotation(
				SpawnPoint->GetActorLocation(),
				SpawnPoint->GetActorRotation());
		}
	}

	// Restore player (AC-053.5, AC-053.6)
	Player->HandlePlayerRespawn();

	// Reset arena if configured (AC-053.8)
	if (ArenaResetOnPlayerRespawn)
	{
		ResetArena();
	}

	bRespawnPending = false;
	PendingRespawnPlayer.Reset();
}

// ---------------------------------------------------------------------------
// Arena: Enemy Kill Tracking (US-053)
// ---------------------------------------------------------------------------

void AMordecaiGameMode::NotifyEnemyKilled(
	AMordecaiEnemyCharacter* KilledEnemy,
	APlayerState* KillerPlayerState)
{
	const TWeakObjectPtr<AMordecaiEnemyCharacter> WeakEnemy(KilledEnemy);
	// Single-player issue #6 contract: only a Mordecai player-owned GAS context earns a kill. The player state
	// parameter intentionally gives issue #77 a per-player key without adding replication or multiplayer state here.
	if (!KilledEnemy || !Cast<AMordecaiPlayerState>(KillerPlayerState)
		|| bRespawnPending || CountedEnemyDeaths.Contains(WeakEnemy))
	{
		return;
	}

	CountedEnemyDeaths.Add(WeakEnemy);
	EnemyKillCount++;
	EnemyKillCountChanged.Broadcast(EnemyKillCount);
}

void AMordecaiGameMode::ResetEnemyKillCount()
{
	if (EnemyKillCount != 0)
	{
		EnemyKillCount = 0;
		EnemyKillCountChanged.Broadcast(EnemyKillCount);
	}
}

void AMordecaiGameMode::RegisterEnemy(AMordecaiEnemyCharacter* Enemy, const FTransform& SpawnTransform)
{
	if (!Enemy)
	{
		return;
	}

	FEnemySpawnEntry Entry;
	Entry.Enemy = Enemy;
	Entry.SpawnTransform = SpawnTransform;
	EnemyRegistry.Add(Entry);
}

// ---------------------------------------------------------------------------
// Arena: Reset (US-053)
// ---------------------------------------------------------------------------

void AMordecaiGameMode::ResetArena()
{
	ResetEnemyKillCount();
	CountedEnemyDeaths.Reset();

	for (FEnemySpawnEntry& Entry : EnemyRegistry)
	{
		if (AMordecaiEnemyCharacter* Enemy = Entry.Enemy.Get())
		{
			Enemy->ResetForArena();
			Enemy->SetActorTransform(Entry.SpawnTransform);
		}
	}
}
