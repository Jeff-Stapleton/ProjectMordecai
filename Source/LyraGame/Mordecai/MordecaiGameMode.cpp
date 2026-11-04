// Project Mordecai

#include "MordecaiGameMode.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "Development/LyraDeveloperSettings.h"
#include "GameModes/LyraWorldSettings.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/PlayerState.h"
#include "Mordecai/MordecaiCharacter.h"
#include "Mordecai/MordecaiGameplayTags.h"
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
	if (!DeadPlayer)
	{
		return;
	}

	// Issue #6: the death ends the life's kill count (AC-063.2). The death screen shows its total; the HUD starts over.
	CompletedLifeKillCount = EnemyKillCount;
	SetEnemyKillCount(0);

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

	// Reset arena if configured (AC-053.8). The enemies only: the death already reset the kill count.
	if (ArenaResetOnPlayerRespawn)
	{
		ResetEnemies();
	}

	PendingRespawnPlayer.Reset();
}

// ---------------------------------------------------------------------------
// Arena: Enemy Kill Tracking (US-053, issue #6)
// ---------------------------------------------------------------------------

void AMordecaiGameMode::NotifyEnemyKilled(AMordecaiEnemyCharacter* KilledEnemy, APlayerState* Killer)
{
	if (!Killer)
	{
		return;
	}

	// The player's Dead tag is on its PlayerState's ASC (HandlePlayerDeath): a projectile or DoT from a life that has
	// ended scores for no one (Jeff, 2026-11-04)
	const UAbilitySystemComponent* KillerASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(Killer);
	if (KillerASC && KillerASC->HasMatchingGameplayTag(MordecaiGameplayTags::State_Dead))
	{
		return;
	}

	SetEnemyKillCount(EnemyKillCount + 1);
}

void AMordecaiGameMode::SetEnemyKillCount(int32 NewCount)
{
	if (EnemyKillCount == NewCount)
	{
		return;
	}
	EnemyKillCount = NewCount;
	OnEnemyKillCountChanged.Broadcast(EnemyKillCount);
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
	ResetEnemies();
	SetEnemyKillCount(0);
}

void AMordecaiGameMode::ResetEnemies()
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
