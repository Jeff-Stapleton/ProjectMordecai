// Project Mordecai

#include "MordecaiGameMode.h"

#include "Mordecai/MordecaiCharacter.h"
#include "Mordecai/Enemy/MordecaiEnemyCharacter.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(MordecaiGameMode)

AMordecaiGameMode::AMordecaiGameMode(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

// ---------------------------------------------------------------------------
// Arena: Player Death / Respawn (US-053)
// ---------------------------------------------------------------------------

void AMordecaiGameMode::NotifyPlayerDeath(AMordecaiCharacter* DeadPlayer)
{
	if (!DeadPlayer)
	{
		return;
	}

	PendingRespawnPlayer = DeadPlayer;

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
	AMordecaiCharacter* Player = PendingRespawnPlayer.Get();
	if (!Player)
	{
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

	PendingRespawnPlayer.Reset();
}

// ---------------------------------------------------------------------------
// Arena: Enemy Kill Tracking (US-053)
// ---------------------------------------------------------------------------

void AMordecaiGameMode::NotifyEnemyKilled(AMordecaiEnemyCharacter* KilledEnemy)
{
	EnemyKillCount++;
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
	for (FEnemySpawnEntry& Entry : EnemyRegistry)
	{
		if (AMordecaiEnemyCharacter* Enemy = Entry.Enemy.Get())
		{
			Enemy->ResetForArena();
			Enemy->SetActorTransform(Entry.SpawnTransform);
		}
	}
}
