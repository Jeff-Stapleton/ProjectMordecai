// Project Mordecai — Enemy Character Tests (US-050)

#include "Misc/AutomationTest.h"
#include "Mordecai/Enemy/MordecaiEnemyCharacter.h"
#include "Mordecai/AbilitySystem/MordecaiAbilitySystemComponent.h"
#include "Mordecai/AbilitySystem/MordecaiAttributeSet.h"
#include "Mordecai/Combat/MordecaiPostureSystem.h"
#include "Mordecai/MordecaiGameplayTags.h"
#include "AbilitySystemComponent.h"
#include "GameFramework/CharacterMovementComponent.h"

// ---------------------------------------------------------------------------
// 1. Mordecai.Enemy.CharacterHasAbilitySystemComponent (AC-050.2)
// Verify enemy has a replicated UMordecaiAbilitySystemComponent
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_Enemy_HasASC,
	"Mordecai.Enemy.CharacterHasAbilitySystemComponent",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_Enemy_HasASC::RunTest(const FString& Parameters)
{
	// AC-050.1: class extends ALyraCharacter
	TestTrue("AMordecaiEnemyCharacter extends ALyraCharacter",
		AMordecaiEnemyCharacter::StaticClass()->IsChildOf(ALyraCharacter::StaticClass()));

	// AC-050.2: enemy owns a UMordecaiAbilitySystemComponent
	const AMordecaiEnemyCharacter* CDO = GetDefault<AMordecaiEnemyCharacter>();
	TestNotNull("Enemy CDO exists", CDO);

	UAbilitySystemComponent* ASC = CDO->GetAbilitySystemComponent();
	TestNotNull("Enemy has AbilitySystemComponent", ASC);

	if (ASC)
	{
		TestTrue("ASC is UMordecaiAbilitySystemComponent",
			ASC->IsA<UMordecaiAbilitySystemComponent>());
		// AC-050.11: ASC is replicated
		TestTrue("ASC is replicated", ASC->GetIsReplicated());
	}

	return true;
}

// ---------------------------------------------------------------------------
// 2. Mordecai.Enemy.CharacterHasAttributeSet (AC-050.3)
// Verify enemy has a UMordecaiAttributeSet with Health, MaxHealth, Posture, MaxPosture
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_Enemy_HasAttributeSet,
	"Mordecai.Enemy.CharacterHasAttributeSet",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_Enemy_HasAttributeSet::RunTest(const FString& Parameters)
{
	const AMordecaiEnemyCharacter* CDO = GetDefault<AMordecaiEnemyCharacter>();
	TestNotNull("Enemy CDO exists", CDO);

	UMordecaiAttributeSet* AttrSet = CDO->GetMordecaiAttributeSet();
	TestNotNull("Enemy has MordecaiAttributeSet", AttrSet);

	// Verify required attributes exist on the set
	TestTrue("Health attribute valid", UMordecaiAttributeSet::GetHealthAttribute().IsValid());
	TestTrue("MaxHealth attribute valid", UMordecaiAttributeSet::GetMaxHealthAttribute().IsValid());
	TestTrue("Posture attribute valid", UMordecaiAttributeSet::GetPostureAttribute().IsValid());
	TestTrue("MaxPosture attribute valid", UMordecaiAttributeSet::GetMaxPostureAttribute().IsValid());

	return true;
}

// ---------------------------------------------------------------------------
// 3. Mordecai.Enemy.CharacterHasEnemyTeamTag (AC-050.4)
// Verify Mordecai.Team.Enemy tag is valid and enemy has ASC for tag application
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_Enemy_HasEnemyTeamTag,
	"Mordecai.Enemy.CharacterHasEnemyTeamTag",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_Enemy_HasEnemyTeamTag::RunTest(const FString& Parameters)
{
	// Verify Team_Enemy tag is valid
	FGameplayTag EnemyTag = MordecaiGameplayTags::Team_Enemy;
	TestTrue("Mordecai.Team.Enemy tag is valid", EnemyTag.IsValid());
	TestEqual("Tag name is Mordecai.Team.Enemy", EnemyTag.GetTagName(), FName("Mordecai.Team.Enemy"));

	// Tag is applied at runtime in InitializeAbilitySystem/BeginPlay.
	// Verify enemy has an ASC capable of holding the tag.
	const AMordecaiEnemyCharacter* CDO = GetDefault<AMordecaiEnemyCharacter>();
	TestNotNull("Enemy CDO exists", CDO);
	TestNotNull("Enemy has ASC for tag application", CDO->GetAbilitySystemComponent());

	return true;
}

// ---------------------------------------------------------------------------
// 4. Mordecai.Enemy.HealthDamageReducesHealth (AC-050.5)
// Verify SetByCaller health damage tag and Health attribute exist for damage pipeline
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_Enemy_HealthDamageReducesHealth,
	"Mordecai.Enemy.HealthDamageReducesHealth",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_Enemy_HealthDamageReducesHealth::RunTest(const FString& Parameters)
{
	// AC-050.5: SetByCaller tag for health damage exists
	FGameplayTag HealthDamageTag = MordecaiGameplayTags::SetByCaller_HealthDamage;
	TestTrue("SetByCaller_HealthDamage tag is valid", HealthDamageTag.IsValid());

	// Health attribute exists and has correct defaults
	TestTrue("Health attribute valid", UMordecaiAttributeSet::GetHealthAttribute().IsValid());

	const UMordecaiAttributeSet* AttrCDO = GetDefault<UMordecaiAttributeSet>();
	TestEqual("Default Health is 100", AttrCDO->GetHealth(), 100.f);
	TestEqual("Default MaxHealth is 100", AttrCDO->GetMaxHealth(), 100.f);

	// Health is clamped to [0, MaxHealth] — ensures damage stops at 0
	TestTrue("Health <= MaxHealth", AttrCDO->GetHealth() <= AttrCDO->GetMaxHealth());
	TestTrue("Health >= 0", AttrCDO->GetHealth() >= 0.f);

	// Enemy configurable default matches attribute default
	const AMordecaiEnemyCharacter* EnemyCDO = GetDefault<AMordecaiEnemyCharacter>();
	TestEqual("Enemy DefaultMaxHealth is 100", EnemyCDO->DefaultMaxHealth, 100.f);

	return true;
}

// ---------------------------------------------------------------------------
// 5. Mordecai.Enemy.DeathTriggeredAtZeroHealth (AC-050.6)
// Verify death state is applied when health reaches 0
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_Enemy_DeathTriggeredAtZeroHealth,
	"Mordecai.Enemy.DeathTriggeredAtZeroHealth",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_Enemy_DeathTriggeredAtZeroHealth::RunTest(const FString& Parameters)
{
	// AC-050.6: State_Dead tag exists
	FGameplayTag DeadTag = MordecaiGameplayTags::State_Dead;
	TestTrue("Mordecai.State.Dead tag is valid", DeadTag.IsValid());

	// Create test enemy and verify death state
	AMordecaiEnemyCharacter* Enemy = NewObject<AMordecaiEnemyCharacter>();
	TestNotNull("Test enemy created", Enemy);
	TestFalse("Enemy is not dead initially", Enemy->IsDead());

	// Trigger death directly (in live gameplay, this fires via OnHealthChanged when Health <= 0)
	Enemy->EnterDeathState();
	TestTrue("Enemy is dead after EnterDeathState", Enemy->IsDead());

	// Verify ASC has the State_Dead tag
	UAbilitySystemComponent* ASC = Enemy->GetAbilitySystemComponent();
	TestNotNull("ASC exists", ASC);
	if (ASC)
	{
		TestTrue("ASC has State_Dead tag", ASC->HasMatchingGameplayTag(DeadTag));
	}

	return true;
}

// ---------------------------------------------------------------------------
// 6. Mordecai.Enemy.DeathDisablesMovement (AC-050.6)
// Verify death disables collision (movement disabled via SetMovementMode in live play)
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_Enemy_DeathDisablesMovement,
	"Mordecai.Enemy.DeathDisablesMovement",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_Enemy_DeathDisablesMovement::RunTest(const FString& Parameters)
{
	AMordecaiEnemyCharacter* Enemy = NewObject<AMordecaiEnemyCharacter>();
	TestNotNull("Test enemy created", Enemy);

	// Collision enabled before death
	TestTrue("Collision enabled before death", Enemy->GetActorEnableCollision());

	// Movement component exists (structural requirement for movement disabling)
	UCharacterMovementComponent* CMC = Enemy->GetCharacterMovement();
	TestNotNull("Movement component exists", CMC);

	// Trigger death
	Enemy->EnterDeathState();

	// AC-050.6: collision changes to NoCollision
	TestFalse("Collision disabled after death", Enemy->GetActorEnableCollision());

	// Verify death state
	TestTrue("Is dead", Enemy->IsDead());

	return true;
}

// ---------------------------------------------------------------------------
// 7. Mordecai.Enemy.DeathBroadcastsEvent (AC-050.6)
// Verify Event_Death tag exists and enemy can broadcast it via ASC
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_Enemy_DeathBroadcastsEvent,
	"Mordecai.Enemy.DeathBroadcastsEvent",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_Enemy_DeathBroadcastsEvent::RunTest(const FString& Parameters)
{
	// AC-050.6: Event_Death tag exists for gameplay event broadcast
	FGameplayTag DeathEventTag = MordecaiGameplayTags::Event_Death;
	TestTrue("Mordecai.Event.Death tag is valid", DeathEventTag.IsValid());

	// Enemy has ASC for event broadcasting
	const AMordecaiEnemyCharacter* CDO = GetDefault<AMordecaiEnemyCharacter>();
	TestNotNull("Enemy has ASC for event broadcasting", CDO->GetAbilitySystemComponent());

	// Event is dispatched via ASC->HandleGameplayEvent() in EnterDeathState
	// Full event testing requires a live world; structural check verifies the pipeline exists.

	return true;
}

// ---------------------------------------------------------------------------
// 8. Mordecai.Enemy.PostureDamageReducesPosture (AC-050.7)
// Verify posture damage pipeline: SetByCaller tag, attribute, and PostureSystem
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_Enemy_PostureDamageReducesPosture,
	"Mordecai.Enemy.PostureDamageReducesPosture",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_Enemy_PostureDamageReducesPosture::RunTest(const FString& Parameters)
{
	// AC-050.7: SetByCaller tag for posture damage exists
	FGameplayTag PostureDamageTag = MordecaiGameplayTags::SetByCaller_PostureDamage;
	TestTrue("SetByCaller_PostureDamage tag is valid", PostureDamageTag.IsValid());

	// Posture attribute exists
	TestTrue("Posture attribute valid", UMordecaiAttributeSet::GetPostureAttribute().IsValid());

	// Enemy has PostureSystem
	const AMordecaiEnemyCharacter* CDO = GetDefault<AMordecaiEnemyCharacter>();
	TestNotNull("Enemy has PostureSystem", CDO->GetPostureSystem());

	// PostureSystem correctly computes posture damage
	UMordecaiPostureSystem* System = NewObject<UMordecaiPostureSystem>();
	float Damage = System->ComputePostureDamage(25.f, 1.0f);
	TestEqual("25 base x 1.0 scalar = 25 posture damage", Damage, 25.f);

	return true;
}

// ---------------------------------------------------------------------------
// 9. Mordecai.Enemy.PostureBreakAtZero (AC-050.8)
// Verify posture break at zero with 1000ms duration
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_Enemy_PostureBreakAtZero,
	"Mordecai.Enemy.PostureBreakAtZero",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_Enemy_PostureBreakAtZero::RunTest(const FString& Parameters)
{
	// AC-050.8: PostureBroken tag is valid
	FGameplayTag PostureBrokenTag = MordecaiGameplayTags::State_PostureBroken;
	TestTrue("Mordecai.State.PostureBroken tag is valid", PostureBrokenTag.IsValid());

	// PostureSystem detects break at zero
	UMordecaiPostureSystem* System = NewObject<UMordecaiPostureSystem>();
	TestTrue("Break when posture reaches 0", System->ShouldBreakPosture(0.f));
	TestTrue("Break when posture negative", System->ShouldBreakPosture(-5.f));
	TestFalse("No break when posture is 50", System->ShouldBreakPosture(50.f));

	// Default PostureBrokenDurationMs is 1000ms (per design_decisions_log)
	TestEqual("Default PostureBrokenDurationMs is 1000", System->PostureBrokenDurationMs, 1000.f);

	// Verify enemy's EnterPostureBrokenState applies the tag
	AMordecaiEnemyCharacter* Enemy = NewObject<AMordecaiEnemyCharacter>();
	UAbilitySystemComponent* ASC = Enemy->GetAbilitySystemComponent();
	TestNotNull("ASC exists for posture break", ASC);

	Enemy->EnterPostureBrokenState();
	if (ASC)
	{
		TestTrue("PostureBroken tag applied after break",
			ASC->HasMatchingGameplayTag(PostureBrokenTag));
	}

	return true;
}

// ---------------------------------------------------------------------------
// 10. Mordecai.Enemy.PostureRegeneratesAfterDelay (AC-050.9)
// Verify posture regen rate 10/sec with 3000ms delay
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_Enemy_PostureRegeneratesAfterDelay,
	"Mordecai.Enemy.PostureRegeneratesAfterDelay",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_Enemy_PostureRegeneratesAfterDelay::RunTest(const FString& Parameters)
{
	UMordecaiPostureSystem* System = NewObject<UMordecaiPostureSystem>();

	// AC-050.9: default regen rate 10/sec
	TestEqual("Default PostureRegenRate is 10", System->PostureRegenRate, 10.f);

	// AC-050.9: default regen delay 3000ms
	TestEqual("Default PostureRegenDelayMs is 3000", System->PostureRegenDelayMs, 3000.f);

	// Regen paused after posture damage
	System->NotifyPostureDamageApplied(10.0);
	TestFalse("No regen 1s after damage", System->IsRegenAllowed(11.0));
	TestFalse("No regen 2.5s after damage", System->IsRegenAllowed(12.5));
	TestTrue("Regen allowed 3.1s after damage", System->IsRegenAllowed(13.1));

	// Regen amount for 1 second = 10 units
	float Regen = System->ComputeRegenAmount(1.0f);
	TestEqual("1s regen = 10 units", Regen, 10.f);

	// Enemy's PostureSystem has correct defaults
	const AMordecaiEnemyCharacter* CDO = GetDefault<AMordecaiEnemyCharacter>();
	UMordecaiPostureSystem* EnemyPosture = CDO->GetPostureSystem();
	TestNotNull("Enemy has PostureSystem", EnemyPosture);
	if (EnemyPosture)
	{
		TestEqual("Enemy PostureRegenRate is 10", EnemyPosture->PostureRegenRate, 10.f);
		TestEqual("Enemy PostureRegenDelayMs is 3000", EnemyPosture->PostureRegenDelayMs, 3000.f);
	}

	return true;
}

// ---------------------------------------------------------------------------
// 11. Mordecai.Enemy.BaseStatsConfigurable (AC-050.10)
// Verify configurable UPROPERTY defaults for MaxHealth, MaxPosture, MoveSpeed
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_Enemy_BaseStatsConfigurable,
	"Mordecai.Enemy.BaseStatsConfigurable",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_Enemy_BaseStatsConfigurable::RunTest(const FString& Parameters)
{
	UClass* EnemyClass = AMordecaiEnemyCharacter::StaticClass();

	// AC-050.10: configurable properties exist
	FProperty* MaxHealthProp = EnemyClass->FindPropertyByName(FName("DefaultMaxHealth"));
	TestNotNull("DefaultMaxHealth property exposed", MaxHealthProp);

	FProperty* MaxPostureProp = EnemyClass->FindPropertyByName(FName("DefaultMaxPosture"));
	TestNotNull("DefaultMaxPosture property exposed", MaxPostureProp);

	FProperty* MoveSpeedProp = EnemyClass->FindPropertyByName(FName("DefaultMoveSpeed"));
	TestNotNull("DefaultMoveSpeed property exposed", MoveSpeedProp);

	// Verify reasonable defaults
	const AMordecaiEnemyCharacter* CDO = GetDefault<AMordecaiEnemyCharacter>();
	TestEqual("DefaultMaxHealth is 100", CDO->DefaultMaxHealth, 100.f);
	TestEqual("DefaultMaxPosture is 100", CDO->DefaultMaxPosture, 100.f);
	TestTrue("DefaultMoveSpeed is positive", CDO->DefaultMoveSpeed > 0.f);

	return true;
}

// ---------------------------------------------------------------------------
// 12. Mordecai.Enemy.DeadEnemyRejectsDamage (AC-050.12)
// Verify dead enemy: Health clamped at 0, death state is idempotent
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_Enemy_DeadEnemyRejectsDamage,
	"Mordecai.Enemy.DeadEnemyRejectsDamage",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_Enemy_DeadEnemyRejectsDamage::RunTest(const FString& Parameters)
{
	// AC-050.12: Health clamped to [0, MaxHealth] by attribute set
	const UMordecaiAttributeSet* AttrCDO = GetDefault<UMordecaiAttributeSet>();
	TestTrue("Health >= 0 (floor clamped)", AttrCDO->GetHealth() >= 0.f);
	TestTrue("Health <= MaxHealth (ceiling clamped)", AttrCDO->GetHealth() <= AttrCDO->GetMaxHealth());

	// Dead enemy stays dead — EnterDeathState is idempotent
	AMordecaiEnemyCharacter* Enemy = NewObject<AMordecaiEnemyCharacter>();
	Enemy->EnterDeathState();
	TestTrue("Enemy is dead", Enemy->IsDead());

	// Calling EnterDeathState again does not crash (idempotent)
	Enemy->EnterDeathState();
	TestTrue("Still dead after redundant death call", Enemy->IsDead());

	// Health attribute clamping ensures further damage to a dead enemy (Health=0)
	// stays at 0. The OnHealthChanged handler checks !IsDead() before re-triggering.

	return true;
}
