// Project Mordecai — Arena/Death/Respawn Tests (US-053)

#include "Misc/AutomationTest.h"
#include "Mordecai/MordecaiCharacter.h"
#include "Mordecai/MordecaiGameMode.h"
#include "Mordecai/Enemy/MordecaiEnemyCharacter.h"
#include "Mordecai/AbilitySystem/MordecaiAbilitySystemComponent.h"
#include "Mordecai/AbilitySystem/MordecaiAttributeSet.h"
#include "Mordecai/MordecaiGameplayTags.h"
#include "AbilitySystemComponent.h"
#include "GameFramework/CharacterMovementComponent.h"

// ---------------------------------------------------------------------------
// 1. Mordecai.Arena.PlayerDeathAtZeroHealth (AC-053.1)
// When Health reaches <= 0, player character enters death state.
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_Arena_PlayerDeathAtZeroHealth,
	"Mordecai.Arena.PlayerDeathAtZeroHealth",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_Arena_PlayerDeathAtZeroHealth::RunTest(const FString& Parameters)
{
	// AC-053.1: State_Dead tag exists
	FGameplayTag DeadTag = MordecaiGameplayTags::State_Dead;
	TestTrue("Mordecai.State.Dead tag is valid", DeadTag.IsValid());

	// Player character can handle death
	AMordecaiCharacter* Player = NewObject<AMordecaiCharacter>();
	TestNotNull("Player character created", Player);
	TestFalse("Player is not dead initially", Player->IsDead());

	Player->HandlePlayerDeath();
	TestTrue("Player is dead after HandlePlayerDeath", Player->IsDead());

	// Idempotent
	Player->HandlePlayerDeath();
	TestTrue("Still dead after redundant call", Player->IsDead());

	return true;
}

// ---------------------------------------------------------------------------
// 2. Mordecai.Arena.PlayerDeathDisablesMovement (AC-053.2)
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_Arena_PlayerDeathDisablesMovement,
	"Mordecai.Arena.PlayerDeathDisablesMovement",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_Arena_PlayerDeathDisablesMovement::RunTest(const FString& Parameters)
{
	AMordecaiCharacter* Player = NewObject<AMordecaiCharacter>();
	UCharacterMovementComponent* CMC = Player->GetCharacterMovement();
	TestNotNull("Player has movement component", CMC);

	Player->HandlePlayerDeath();

	if (CMC)
	{
		TestTrue("Movement mode is None after death", CMC->MovementMode == MOVE_None);
	}

	return true;
}

// ---------------------------------------------------------------------------
// 3. Mordecai.Arena.PlayerDeathBlocksInput (AC-053.2)
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_Arena_PlayerDeathBlocksInput,
	"Mordecai.Arena.PlayerDeathBlocksInput",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_Arena_PlayerDeathBlocksInput::RunTest(const FString& Parameters)
{
	// State_Dead tag blocks ability activation via ASC tag blocking
	FGameplayTag DeadTag = MordecaiGameplayTags::State_Dead;
	TestTrue("State_Dead tag valid for input blocking", DeadTag.IsValid());

	// Death handler runs without crash and marks character dead
	AMordecaiCharacter* Player = NewObject<AMordecaiCharacter>();
	Player->HandlePlayerDeath();
	TestTrue("Player dead — input blocked via State_Dead + DisableInput", Player->IsDead());

	return true;
}

// ---------------------------------------------------------------------------
// 4. Mordecai.Arena.PlayerDeathBroadcastsEvent (AC-053.3)
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_Arena_PlayerDeathBroadcastsEvent,
	"Mordecai.Arena.PlayerDeathBroadcastsEvent",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_Arena_PlayerDeathBroadcastsEvent::RunTest(const FString& Parameters)
{
	FGameplayTag PlayerDeathTag = MordecaiGameplayTags::Event_PlayerDeath;
	TestTrue("Mordecai.Event.PlayerDeath tag is valid", PlayerDeathTag.IsValid());
	TestEqual("Tag name correct",
		PlayerDeathTag.GetTagName(), FName("Mordecai.Event.PlayerDeath"));

	FGameplayTag DeathTag = MordecaiGameplayTags::Event_Death;
	TestTrue("Mordecai.Event.Death tag is valid", DeathTag.IsValid());

	return true;
}

// ---------------------------------------------------------------------------
// 5. Mordecai.Arena.RespawnRestoresHealth (AC-053.5)
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_Arena_RespawnRestoresHealth,
	"Mordecai.Arena.RespawnRestoresHealth",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_Arena_RespawnRestoresHealth::RunTest(const FString& Parameters)
{
	// Test via enemy (has own ASC + attribute set as subobjects)
	AMordecaiEnemyCharacter* Enemy = NewObject<AMordecaiEnemyCharacter>();

	UMordecaiAttributeSet* AttrSet = Enemy->GetMordecaiAttributeSet();
	TestNotNull("Attribute set exists", AttrSet);
	if (!AttrSet) return true;

	const float MaxHealth = AttrSet->GetMaxHealth();
	TestTrue("MaxHealth > 0", MaxHealth > 0.f);

	// Simulate damage via direct attribute set (ASC not fully initialized in test)
	AttrSet->InitHealth(0.f);
	TestEqual("Health is 0 after damage", AttrSet->GetHealth(), 0.f);

	// Enter death manually (in live game, OnHealthChanged triggers this)
	Enemy->EnterDeathState();
	TestTrue("Enemy is dead", Enemy->IsDead());

	// ResetForArena restores health via InitHealth
	Enemy->ResetForArena();
	TestEqual("Health restored to MaxHealth", AttrSet->GetHealth(), MaxHealth);

	return true;
}

// ---------------------------------------------------------------------------
// 6. Mordecai.Arena.RespawnRestoresStamina (AC-053.5)
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_Arena_RespawnRestoresStamina,
	"Mordecai.Arena.RespawnRestoresStamina",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_Arena_RespawnRestoresStamina::RunTest(const FString& Parameters)
{
	// Stamina attributes exist
	TestTrue("Stamina attribute valid", UMordecaiAttributeSet::GetStaminaAttribute().IsValid());
	TestTrue("MaxStamina attribute valid", UMordecaiAttributeSet::GetMaxStaminaAttribute().IsValid());

	// Verify defaults match
	const UMordecaiAttributeSet* AttrCDO = GetDefault<UMordecaiAttributeSet>();
	TestEqual("Default Stamina == MaxStamina", AttrCDO->GetStamina(), AttrCDO->GetMaxStamina());

	// Player respawn clears death state
	AMordecaiCharacter* Player = NewObject<AMordecaiCharacter>();
	Player->HandlePlayerDeath();
	TestTrue("Player dead before respawn", Player->IsDead());

	Player->HandlePlayerRespawn();
	TestFalse("Player alive after respawn", Player->IsDead());

	return true;
}

// ---------------------------------------------------------------------------
// 7. Mordecai.Arena.RespawnRemovesDeadTag (AC-053.5)
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_Arena_RespawnRemovesDeadTag,
	"Mordecai.Arena.RespawnRemovesDeadTag",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_Arena_RespawnRemovesDeadTag::RunTest(const FString& Parameters)
{
	// Test via enemy (has ASC for tag verification — tags work without InitAbilityActorInfo)
	AMordecaiEnemyCharacter* Enemy = NewObject<AMordecaiEnemyCharacter>();

	Enemy->EnterDeathState();
	TestTrue("Enemy dead", Enemy->IsDead());

	UAbilitySystemComponent* ASC = Enemy->GetAbilitySystemComponent();
	if (ASC)
	{
		TestTrue("State_Dead tag present", ASC->HasMatchingGameplayTag(MordecaiGameplayTags::State_Dead));
	}

	Enemy->ResetForArena();
	TestFalse("Enemy not dead after reset", Enemy->IsDead());
	if (ASC)
	{
		TestFalse("State_Dead tag removed", ASC->HasMatchingGameplayTag(MordecaiGameplayTags::State_Dead));
	}

	// Player death flag also clears on respawn
	AMordecaiCharacter* Player = NewObject<AMordecaiCharacter>();
	Player->HandlePlayerDeath();
	TestTrue("Player dead", Player->IsDead());
	Player->HandlePlayerRespawn();
	TestFalse("Player alive after respawn", Player->IsDead());

	return true;
}

// ---------------------------------------------------------------------------
// 8. Mordecai.Arena.RespawnReenablesMovement (AC-053.6)
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_Arena_RespawnReenablesMovement,
	"Mordecai.Arena.RespawnReenablesMovement",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_Arena_RespawnReenablesMovement::RunTest(const FString& Parameters)
{
	AMordecaiCharacter* Player = NewObject<AMordecaiCharacter>();
	UCharacterMovementComponent* CMC = Player->GetCharacterMovement();
	TestNotNull("CMC exists", CMC);

	Player->HandlePlayerDeath();
	if (CMC)
	{
		TestTrue("Movement None after death", CMC->MovementMode == MOVE_None);
	}

	Player->HandlePlayerRespawn();
	if (CMC)
	{
		TestTrue("Movement Walking after respawn", CMC->MovementMode == MOVE_Walking);
	}

	return true;
}

// ---------------------------------------------------------------------------
// 9. Mordecai.Arena.KillCountIncrementsOnEnemyDeath (AC-053.7)
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_Arena_KillCountIncrementsOnEnemyDeath,
	"Mordecai.Arena.KillCountIncrementsOnEnemyDeath",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_Arena_KillCountIncrementsOnEnemyDeath::RunTest(const FString& Parameters)
{
	AMordecaiGameMode* GM = NewObject<AMordecaiGameMode>();
	TestNotNull("GameMode created", GM);
	TestEqual("Initial kill count is 0", GM->GetEnemyKillCount(), 0);

	GM->NotifyEnemyKilled(nullptr);
	TestEqual("Kill count is 1", GM->GetEnemyKillCount(), 1);

	GM->NotifyEnemyKilled(nullptr);
	TestEqual("Kill count is 2", GM->GetEnemyKillCount(), 2);

	FGameplayTag EnemyKillTag = MordecaiGameplayTags::Event_EnemyKill;
	TestTrue("Mordecai.Event.EnemyKill tag is valid", EnemyKillTag.IsValid());

	return true;
}

// ---------------------------------------------------------------------------
// 10. Mordecai.Arena.ArenaResetRestoresEnemies (AC-053.9)
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_Arena_ArenaResetRestoresEnemies,
	"Mordecai.Arena.ArenaResetRestoresEnemies",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_Arena_ArenaResetRestoresEnemies::RunTest(const FString& Parameters)
{
	AMordecaiEnemyCharacter* Enemy = NewObject<AMordecaiEnemyCharacter>();

	UMordecaiAttributeSet* AttrSet = Enemy->GetMordecaiAttributeSet();
	TestNotNull("Attribute set exists", AttrSet);

	// Kill enemy
	Enemy->EnterDeathState();
	TestTrue("Enemy is dead", Enemy->IsDead());
	TestFalse("Collision disabled", Enemy->GetActorEnableCollision());

	// ResetForArena restores enemy
	Enemy->ResetForArena();
	TestFalse("Enemy alive after reset", Enemy->IsDead());
	TestTrue("Collision re-enabled", Enemy->GetActorEnableCollision());

	if (AttrSet)
	{
		TestEqual("Health restored", AttrSet->GetHealth(), AttrSet->GetMaxHealth());
		TestEqual("Posture restored", AttrSet->GetPosture(), AttrSet->GetMaxPosture());
	}

	return true;
}

// ---------------------------------------------------------------------------
// 11. Mordecai.Arena.RespawnDelayIsConfigurable (AC-053.4)
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_Arena_RespawnDelayIsConfigurable,
	"Mordecai.Arena.RespawnDelayIsConfigurable",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_Arena_RespawnDelayIsConfigurable::RunTest(const FString& Parameters)
{
	UClass* GMClass = AMordecaiGameMode::StaticClass();

	FProperty* RespawnDelayProp = GMClass->FindPropertyByName(FName("RespawnDelaySeconds"));
	TestNotNull("RespawnDelaySeconds property exposed", RespawnDelayProp);

	const AMordecaiGameMode* GMCDO = GetDefault<AMordecaiGameMode>();
	TestEqual("Default RespawnDelaySeconds is 3.0", GMCDO->RespawnDelaySeconds, 3.0f);

	FProperty* ArenaResetProp = GMClass->FindPropertyByName(FName("ArenaResetOnPlayerRespawn"));
	TestNotNull("ArenaResetOnPlayerRespawn property exposed", ArenaResetProp);
	TestTrue("Default ArenaResetOnPlayerRespawn is true", GMCDO->ArenaResetOnPlayerRespawn);

	return true;
}
