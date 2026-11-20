// Project Mordecai — US-123 enemy posture runtime RED-first contracts.

#include "Misc/AutomationTest.h"
#include "Misc/ScopeExit.h"

#include "Dom/JsonObject.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "Components/SphereComponent.h"
#include "Mordecai/AbilitySystem/MordecaiAttributeSet.h"
#include "Mordecai/Combat/MordecaiGA_Block.h"
#include "Mordecai/Combat/MordecaiGA_MeleeAttack.h"
#include "Mordecai/Combat/MordecaiGA_Parry.h"
#include "Mordecai/Combat/MordecaiProjectile.h"
#include "Mordecai/Combat/MordecaiPostureRuntimeComponent.h"
#include "Mordecai/Combat/MordecaiPostureSystem.h"
#include "Mordecai/Enemy/MordecaiEnemyCharacter.h"
#include "Mordecai/Enemy/MordecaiEnemyAIController.h"
#include "Mordecai/MordecaiGameplayTags.h"
#include "Mordecai/MordecaiCharacter.h"
#include "Mordecai/MordecaiPlayerState.h"
#include "Mordecai/Playtest/MordecaiAgentBridgeState.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Tests/Recovery/MordecaiDefenseTestEnv.h"

namespace MordecaiPostureRuntimeTest
{
	using namespace MordecaiRecoveryDefenseTest;

	UMordecaiAttackProfileDataAsset* MakePlayerSlash(
		const float HealthDamage = 16.f,
		const float PostureScalar = 1.f)
	{
		UMordecaiAttackProfileDataAsset* Profile = MordecaiRecoveryTest::MakeStubProfile();
		Profile->InputSlot = EMordecaiInputSlot::Light;
		Profile->DamageProfile.BasePower = HealthDamage;
		Profile->PostureDamageScalar = PostureScalar;
		Profile->StaminaCost = 0.f;
		return Profile;
	}

	void PinPlayerYellow(FDefenseTestEnv& Env)
	{
		UAbilitySystemComponent* ASC = Env.GetPlayerASC();
		ASC->SetNumericAttributeBase(UMordecaiAttributeSet::GetStaminaAttribute(), 50.f);
		ASC->SetLooseGameplayTagCount(
			MordecaiGameplayTags::Stamina_Tier_Green,
			0,
			EGameplayTagReplicationState::TagOnly);
		ASC->SetLooseGameplayTagCount(
			MordecaiGameplayTags::Stamina_Tier_Yellow,
			1,
			EGameplayTagReplicationState::TagOnly);
	}

	bool EnemyHas(const AMordecaiEnemyCharacter* Enemy, const FGameplayTag& Tag)
	{
		const UAbilitySystemComponent* ASC = Enemy ? Enemy->GetAbilitySystemComponent() : nullptr;
		return ASC && ASC->HasMatchingGameplayTag(Tag);
	}

	void SetEnemyAttribute(
		AMordecaiEnemyCharacter* Enemy,
		const FGameplayAttribute& Attribute,
		const float Value)
	{
		if (UAbilitySystemComponent* ASC = Enemy ? Enemy->GetAbilitySystemComponent() : nullptr)
		{
			ASC->SetNumericAttributeBase(Attribute, Value);
		}
	}

	bool LandPlayerMelee(FDefenseTestEnv& Env)
	{
		Env.Press(MordecaiGameplayTags::Input_LightAttack);
		const UMordecaiGA_MeleeAttack* Melee =
			Env.FindInstance<UMordecaiGA_MeleeAttack>();
		if (!Melee)
		{
			return false;
		}
		for (int32 Frame = 0;
			Frame < 40 && Melee->GetCurrentPhase() == EMordecaiAttackPhase::Windup;
			++Frame)
		{
			Env.TickFrames(1);
		}
		return Melee->GetCurrentPhase() == EMordecaiAttackPhase::Active;
	}

	AMordecaiProjectile* SpawnProjectile(
		FDefenseTestEnv& Env,
		AActor* InstigatorActor,
		const FVector& Location,
		const FVector& Direction,
		const float Damage,
		const bool bPostureDamage)
	{
		AMordecaiProjectile* Projectile = Env.World->SpawnActor<AMordecaiProjectile>(
			AMordecaiProjectile::StaticClass(),
			Location,
			Direction.Rotation(),
			MordecaiRecoveryTest::FRecoveryTestEnv::AlwaysSpawn());
		if (!Projectile)
		{
			return nullptr;
		}
		FMordecaiProjectileSpec Spec;
		Spec.Speed = 1000.f;
		Spec.Lifetime = 2.f;
		Spec.MaxRange = 1000.f;
		FMordecaiDamageProfile DamageProfile;
		DamageProfile.DamageType = EMordecaiDamageType::Fire;
		DamageProfile.BasePower = Damage;
		DamageProfile.AppliesPostureDamage = bPostureDamage;
		Projectile->InitFromSpec(Spec, DamageProfile);
		Projectile->SetProjectileInstigator(InstigatorActor);
		if (!Projectile->HasActorBegunPlay())
		{
			Projectile->DispatchBeginPlay();
		}
		return Projectile;
	}

	void TickProjectile(FDefenseTestEnv& Env, AMordecaiProjectile* Projectile, const int32 Frames = 8)
	{
		for (int32 Frame = 0; Frame < Frames && IsValid(Projectile)
			&& !Projectile->IsActorBeingDestroyed(); ++Frame)
		{
			++GFrameCounter;
			Env.World->TimeSeconds += FrameSeconds;
			Env.World->GetTimerManager().Tick(FrameSeconds);
			Projectile->GetProjectileMovement()->TickComponent(
				FrameSeconds,
				LEVELTICK_All,
				nullptr);
			if (IsValid(Projectile) && !Projectile->IsActorBeingDestroyed())
			{
				Projectile->GetCollisionComponent()->UpdateOverlaps();
			}
		}
	}

	UMordecaiPostureRuntimeComponent* StartPlayerPostureRuntime(FDefenseTestEnv& Env)
	{
		if (Env.PlayerState && !Env.PlayerState->HasActorBegunPlay())
		{
			Env.PlayerState->DispatchBeginPlay();
		}
		return Env.PlayerState
			? Env.PlayerState->FindComponentByClass<UMordecaiPostureRuntimeComponent>()
			: nullptr;
	}

	void SetPlayerAttribute(
		FDefenseTestEnv& Env,
		const FGameplayAttribute& Attribute,
		const float Value)
	{
		if (UAbilitySystemComponent* ASC = Env.GetPlayerASC())
		{
			ASC->SetNumericAttributeBase(Attribute, Value);
		}
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_Posture_Runtime_BreakIsServerOnlyAndReplicated,
	"Mordecai.Posture.Runtime.BreakIsServerOnlyAndReplicated",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_Posture_Runtime_BreakIsServerOnlyAndReplicated::RunTest(const FString& Parameters)
{
	using namespace MordecaiPostureRuntimeTest;

	FDefenseTestEnv Env;
	ON_SCOPE_EXIT { Env.TeardownDefender(); };
	UMordecaiAttackProfileDataAsset* Slash = MakePlayerSlash();
	if (!TestTrue("player setup", Env.SetupDefender(FVector::ZeroVector, { Slash })))
	{
		return true;
	}
	PinPlayerYellow(Env);
	AMordecaiEnemyCharacter* AuthorityEnemy =
		Env.SpawnEnemy(FVector(100.f, 0.f, 0.f), { Slash });
	AMordecaiEnemyCharacter* ClientCopy =
		Env.SpawnEnemy(FVector(100.f, 300.f, 0.f), { Slash });
	if (!TestNotNull("authority enemy", AuthorityEnemy)
		|| !TestNotNull("client copy", ClientCopy))
	{
		return true;
	}

	SetEnemyAttribute(
		AuthorityEnemy,
		UMordecaiAttributeSet::GetPostureAttribute(),
		1.f);
	TestTrue("player melee reaches active", LandPlayerMelee(Env));
	TestTrue(
		"authority melee break grants PostureBroken",
		EnemyHas(AuthorityEnemy, MordecaiGameplayTags::State_PostureBroken));

	ClientCopy->SetRole(ROLE_SimulatedProxy);
	SetEnemyAttribute(ClientCopy, UMordecaiAttributeSet::GetPostureAttribute(), 0.f);
	TestFalse(
		"non-authority copy never starts a break",
		EnemyHas(ClientCopy, MordecaiGameplayTags::State_PostureBroken));
	TestFalse(
		"non-authority posture system never enters broken state",
		ClientCopy->GetPostureSystem()->IsPostureBroken());
	TestEqual(
		"non-authority copy does not restore posture",
		GetAttribute(ClientCopy, UMordecaiAttributeSet::GetPostureAttribute()),
		0.f);
	Env.TickFrames(31);
	TestFalse(
		"non-authority copy has no break timer to expire",
		ClientCopy->GetPostureSystem()->IsPostureBroken());
	TestEqual(
		"non-authority copy still has no timer-driven posture restore",
		GetAttribute(ClientCopy, UMordecaiAttributeSet::GetPostureAttribute()),
		0.f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_Posture_Runtime_ExpiryRestoresMax,
	"Mordecai.Posture.Runtime.ExpiryRestoresMax",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_Posture_Runtime_ExpiryRestoresMax::RunTest(const FString& Parameters)
{
	using namespace MordecaiPostureRuntimeTest;
	using namespace MordecaiRecoveryDefenseTest;

	FDefenseTestEnv Env;
	ON_SCOPE_EXIT { Env.TeardownDefender(); };
	if (!TestTrue("player setup", Env.SetupDefender(FVector::ZeroVector)))
	{
		return true;
	}
	AMordecaiEnemyCharacter* Enemy =
		Env.SpawnEnemy(FVector(100.f, 0.f, 0.f), { MakePlayerSlash() });
	if (!TestNotNull("enemy", Enemy))
	{
		return true;
	}
	SetEnemyAttribute(Enemy, UMordecaiAttributeSet::GetPostureAttribute(), 0.f);
	TestTrue("break tag present", EnemyHas(Enemy, MordecaiGameplayTags::State_PostureBroken));
	AMordecaiEnemyAIController* AI = Cast<AMordecaiEnemyAIController>(Enemy->GetController());
	TestTrue("AI is staggered", AI && AI->GetCurrentState() == EMordecaiEnemyAIState::Staggered);

	Env.TickFrames(29);
	TestTrue("break remains before one and a half seconds", EnemyHas(
		Enemy, MordecaiGameplayTags::State_PostureBroken));
	TestTrue("AI remains staggered before one and a half seconds",
		AI && AI->GetCurrentState() == EMordecaiEnemyAIState::Staggered);
	Env.TickFrames(4);
	TestFalse("break tag expires", EnemyHas(Enemy, MordecaiGameplayTags::State_PostureBroken));
	TestEqual(
		"expiry restores max posture",
		GetAttribute(Enemy, UMordecaiAttributeSet::GetPostureAttribute()),
		GetAttribute(Enemy, UMordecaiAttributeSet::GetMaxPostureAttribute()));
	TestTrue("AI returns to Idle", AI && AI->GetCurrentState() == EMordecaiEnemyAIState::Idle);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_Posture_Runtime_BreakCancelsSwing,
	"Mordecai.Posture.Runtime.BreakCancelsSwing",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_Posture_Runtime_BreakCancelsSwing::RunTest(const FString& Parameters)
{
	using namespace MordecaiPostureRuntimeTest;
	using namespace MordecaiRecoveryDefenseTest;

	FDefenseTestEnv Env;
	ON_SCOPE_EXIT { Env.TeardownDefender(); };
	if (!TestTrue("player setup", Env.SetupDefender(FVector(100.f, 0.f, 0.f))))
	{
		return true;
	}
	UMordecaiAttackProfileDataAsset* SlowSlash = MakePlayerSlash();
	SlowSlash->WindupTimeMs = 500.f;
	AMordecaiEnemyCharacter* Enemy = Env.SpawnAttacker(SlowSlash);
	UAbilitySystemComponent* EnemyASC = Enemy ? Enemy->GetAbilitySystemComponent() : nullptr;
	UMordecaiGA_MeleeAttack* Melee = GetMelee(Enemy);
	if (!TestNotNull("enemy ASC", EnemyASC) || !TestNotNull("melee", Melee)
		|| !TestTrue("swing starts", EnemyASC->TryActivateAbilityByClass(
			UMordecaiGA_MeleeAttack::StaticClass())))
	{
		return true;
	}
	TestEqual("swing is in windup", Melee->GetCurrentPhase(), EMordecaiAttackPhase::Windup);
	const float HealthBefore =
		Env.GetPlayer(UMordecaiAttributeSet::GetHealthAttribute());
	SetEnemyAttribute(Enemy, UMordecaiAttributeSet::GetPostureAttribute(), 0.f);
	TestFalse("break cancels the active ability", EnemyASC->FindAbilitySpecFromClass(
		UMordecaiGA_MeleeAttack::StaticClass())->IsActive());
	Env.TickFrames(20);
	TestEqual(
		"cancelled windup lands no hit",
		Env.GetPlayer(UMordecaiAttributeSet::GetHealthAttribute()),
		HealthBefore);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_Posture_Runtime_StaggeredHitTimesOnePointFive,
	"Mordecai.Posture.Runtime.StaggeredHitTimesOnePointFive",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_Posture_Runtime_StaggeredHitTimesOnePointFive::RunTest(const FString& Parameters)
{
	using namespace MordecaiPostureRuntimeTest;
	using namespace MordecaiRecoveryDefenseTest;

	FDefenseTestEnv Env;
	ON_SCOPE_EXIT { Env.TeardownDefender(); };
	UMordecaiAttackProfileDataAsset* Slash = MakePlayerSlash(16.f, 0.f);
	if (!TestTrue("player setup", Env.SetupDefender(FVector::ZeroVector, { Slash })))
	{
		return true;
	}
	PinPlayerYellow(Env);
	AMordecaiEnemyCharacter* Enemy =
		Env.SpawnEnemy(FVector(100.f, 0.f, 0.f), { Slash });
	if (!TestNotNull("enemy", Enemy))
	{
		return true;
	}
	Enemy->EnterPostureBrokenState();
	const float HealthBefore = GetAttribute(Enemy, UMordecaiAttributeSet::GetHealthAttribute());
	TestTrue("staggered hit lands", LandPlayerMelee(Env));
	TestEqual(
		"staggered enemy takes 16 to 24",
		GetAttribute(Enemy, UMordecaiAttributeSet::GetHealthAttribute()),
		HealthBefore - 24.f);
	TestEqual("one damage pop", Env.PopCount(), 1);
	TestEqual("critical pop shows 24", Env.LastPopAmount(), 24);
	TestTrue("staggered pop is critical", Env.LastPopIsCritical());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_Posture_Runtime_KnockedDownHitTimesTwo,
	"Mordecai.Posture.Runtime.KnockedDownHitTimesTwo",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_Posture_Runtime_KnockedDownHitTimesTwo::RunTest(const FString& Parameters)
{
	using namespace MordecaiPostureRuntimeTest;
	using namespace MordecaiRecoveryDefenseTest;

	FDefenseTestEnv Env;
	ON_SCOPE_EXIT { Env.TeardownDefender(); };
	UMordecaiAttackProfileDataAsset* Slash = MakePlayerSlash(16.f, 0.f);
	if (!TestTrue("player setup", Env.SetupDefender(FVector::ZeroVector, { Slash })))
	{
		return true;
	}
	PinPlayerYellow(Env);
	AMordecaiEnemyCharacter* Enemy =
		Env.SpawnEnemy(FVector(100.f, 0.f, 0.f), { Slash });
	if (!TestNotNull("enemy", Enemy))
	{
		return true;
	}
	Enemy->EnterKnockDownState();
	const float HealthBefore = GetAttribute(Enemy, UMordecaiAttributeSet::GetHealthAttribute());
	TestTrue("knocked-down hit lands", LandPlayerMelee(Env));
	TestEqual(
		"knocked-down enemy takes 16 to 32",
		GetAttribute(Enemy, UMordecaiAttributeSet::GetHealthAttribute()),
		HealthBefore - 32.f);
	TestEqual("one damage pop", Env.PopCount(), 1);
	TestEqual("critical pop shows 32", Env.LastPopAmount(), 32);
	TestTrue("knockdown pop is critical", Env.LastPopIsCritical());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_Posture_Runtime_UnbrokenHitUnchanged,
	"Mordecai.Posture.Runtime.UnbrokenHitUnchanged",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_Posture_Runtime_UnbrokenHitUnchanged::RunTest(const FString& Parameters)
{
	using namespace MordecaiPostureRuntimeTest;
	using namespace MordecaiRecoveryDefenseTest;

	FDefenseTestEnv Env;
	ON_SCOPE_EXIT { Env.TeardownDefender(); };
	UMordecaiAttackProfileDataAsset* Slash = MakePlayerSlash(16.f, 0.f);
	if (!TestTrue("player setup", Env.SetupDefender(FVector::ZeroVector, { Slash })))
	{
		return true;
	}
	PinPlayerYellow(Env);
	AMordecaiEnemyCharacter* Enemy =
		Env.SpawnEnemy(FVector(100.f, 0.f, 0.f), { Slash });
	const float HealthBefore = GetAttribute(Enemy, UMordecaiAttributeSet::GetHealthAttribute());
	TestTrue("unbroken hit lands", LandPlayerMelee(Env));
	TestEqual(
		"unbroken enemy takes 16",
		GetAttribute(Enemy, UMordecaiAttributeSet::GetHealthAttribute()),
		HealthBefore - 16.f);
	TestFalse("ordinary pop is not critical", Env.LastPopIsCritical());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_Posture_Runtime_KnockdownReplacesStagger,
	"Mordecai.Posture.Runtime.KnockdownReplacesStagger",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_Posture_Runtime_KnockdownReplacesStagger::RunTest(const FString& Parameters)
{
	using namespace MordecaiPostureRuntimeTest;
	using namespace MordecaiRecoveryDefenseTest;

	FDefenseTestEnv Env;
	ON_SCOPE_EXIT { Env.TeardownDefender(); };
	if (!TestTrue("player setup", Env.SetupDefender(FVector::ZeroVector)))
	{
		return true;
	}
	auto SpawnBroken = [&Env]()
	{
		AMordecaiEnemyCharacter* Enemy =
			Env.SpawnEnemy(FVector(100.f, 0.f, 0.f), { MakePlayerSlash() });
		if (Enemy)
		{
			Enemy->EnterPostureBrokenState();
		}
		return Enemy;
	};

	AMordecaiEnemyCharacter* Below = SpawnBroken();
	MordecaiRecoveryTest::FRecoveryTestEnv::ApplyHealthDelta(
		Env.GetPlayerASC(), Below, -39.f);
	TestTrue("39 actual damage keeps stagger", EnemyHas(
		Below, MordecaiGameplayTags::State_PostureBroken));
	TestFalse("39 actual damage does not knock down", EnemyHas(
		Below, MordecaiGameplayTags::State_KnockedDown));

	AMordecaiEnemyCharacter* Exact = SpawnBroken();
	MordecaiRecoveryTest::FRecoveryTestEnv::ApplyHealthDelta(
		Env.GetPlayerASC(), Exact, -40.f);
	TestFalse("40 replaces stagger", EnemyHas(
		Exact, MordecaiGameplayTags::State_PostureBroken));
	TestTrue("40 actual damage knocks down", EnemyHas(
		Exact, MordecaiGameplayTags::State_KnockedDown));

	AMordecaiEnemyCharacter* Above = SpawnBroken();
	MordecaiRecoveryTest::FRecoveryTestEnv::ApplyHealthDelta(
		Env.GetPlayerASC(), Above, -41.f);
	TestTrue("41 actual damage knocks down", EnemyHas(
		Above, MordecaiGameplayTags::State_KnockedDown));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_Posture_Runtime_TwoHitsKnockDownEndToEnd,
	"Mordecai.Posture.Runtime.TwoHitsKnockDownEndToEnd",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_Posture_Runtime_TwoHitsKnockDownEndToEnd::RunTest(const FString& Parameters)
{
	using namespace MordecaiPostureRuntimeTest;
	using namespace MordecaiRecoveryDefenseTest;

	FDefenseTestEnv Env;
	ON_SCOPE_EXIT { Env.TeardownDefender(); };
	UMordecaiAttackProfileDataAsset* Slash = MakePlayerSlash(16.f, 0.f);
	if (!TestTrue("player setup", Env.SetupDefender(FVector::ZeroVector, { Slash })))
	{
		return true;
	}
	PinPlayerYellow(Env);
	AMordecaiEnemyCharacter* Enemy =
		Env.SpawnEnemy(FVector(100.f, 0.f, 0.f), { Slash });
	if (!TestNotNull("enemy", Enemy))
	{
		return true;
	}
	Enemy->EnterPostureBrokenState();
	const float HealthBefore = GetAttribute(Enemy, UMordecaiAttributeSet::GetHealthAttribute());

	TestTrue("first staggered melee lands", LandPlayerMelee(Env));
	TestEqual(
		"first hit takes 24 actual health",
		GetAttribute(Enemy, UMordecaiAttributeSet::GetHealthAttribute()),
		HealthBefore - 24.f);
	TestTrue("first hit keeps posture break", EnemyHas(
		Enemy, MordecaiGameplayTags::State_PostureBroken));
	TestFalse("first hit is below knockdown threshold", EnemyHas(
		Enemy, MordecaiGameplayTags::State_KnockedDown));

	Env.TickFrames(21);
	TestTrue("second staggered melee lands in the extended window", LandPlayerMelee(Env));
	TestEqual(
		"two staggered hits take 24 plus 24 actual health",
		GetAttribute(Enemy, UMordecaiAttributeSet::GetHealthAttribute()),
		HealthBefore - 48.f);
	TestFalse("knockdown replaces posture break", EnemyHas(
		Enemy, MordecaiGameplayTags::State_PostureBroken));
	TestTrue("24 plus 24 knocks down end to end", EnemyHas(
		Enemy, MordecaiGameplayTags::State_KnockedDown));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_Posture_Runtime_KnockdownExpiry,
	"Mordecai.Posture.Runtime.KnockdownExpiry",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_Posture_Runtime_KnockdownExpiry::RunTest(const FString& Parameters)
{
	using namespace MordecaiPostureRuntimeTest;
	using namespace MordecaiRecoveryDefenseTest;

	FDefenseTestEnv Env;
	ON_SCOPE_EXIT { Env.TeardownDefender(); };
	if (!TestTrue("player setup", Env.SetupDefender(FVector::ZeroVector)))
	{
		return true;
	}
	AMordecaiEnemyCharacter* Enemy =
		Env.SpawnEnemy(FVector(100.f, 0.f, 0.f), { MakePlayerSlash() });
	Enemy->EnterKnockDownState();
	TestTrue("knockdown starts", EnemyHas(Enemy, MordecaiGameplayTags::State_KnockedDown));
	Env.TickFrames(39);
	TestTrue("knockdown remains before two seconds", EnemyHas(
		Enemy, MordecaiGameplayTags::State_KnockedDown));
	Env.TickFrames(2);
	TestFalse("knockdown expires at two seconds", EnemyHas(
		Enemy, MordecaiGameplayTags::State_KnockedDown));
	TestEqual(
		"knockdown expiry restores max posture",
		GetAttribute(Enemy, UMordecaiAttributeSet::GetPostureAttribute()),
		GetAttribute(Enemy, UMordecaiAttributeSet::GetMaxPostureAttribute()));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_Posture_Runtime_NoRegenWhileDown,
	"Mordecai.Posture.Runtime.NoRegenWhileDown",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_Posture_Runtime_NoRegenWhileDown::RunTest(const FString& Parameters)
{
	using namespace MordecaiPostureRuntimeTest;
	using namespace MordecaiRecoveryDefenseTest;

	FDefenseTestEnv Env;
	ON_SCOPE_EXIT { Env.TeardownDefender(); };
	if (!TestTrue("player setup", Env.SetupDefender(FVector::ZeroVector)))
	{
		return true;
	}
	AMordecaiEnemyCharacter* Enemy =
		Env.SpawnEnemy(FVector(100.f, 0.f, 0.f), { MakePlayerSlash() });
	SetEnemyAttribute(Enemy, UMordecaiAttributeSet::GetPostureAttribute(), 30.f);
	Env.TickFrames(61);
	Enemy->RegeneratePosture(1.f);
	TestEqual(
		"control proves posture regenerates while not down",
		GetAttribute(Enemy, UMordecaiAttributeSet::GetPostureAttribute()),
		40.f);
	SetEnemyAttribute(Enemy, UMordecaiAttributeSet::GetPostureAttribute(), 30.f);
	Enemy->EnterKnockDownState();
	Env.TickFrames(20);
	Enemy->RegeneratePosture(1.f);
	TestEqual(
		"posture does not regenerate while down",
		GetAttribute(Enemy, UMordecaiAttributeSet::GetPostureAttribute()),
		30.f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_Posture_Runtime_WeakenedReducesPostureDealt,
	"Mordecai.Posture.Runtime.WeakenedReducesPostureDealt",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_Posture_Runtime_WeakenedReducesPostureDealt::RunTest(const FString& Parameters)
{
	using namespace MordecaiPostureRuntimeTest;
	using namespace MordecaiRecoveryDefenseTest;
	FDefenseTestEnv Env;
	ON_SCOPE_EXIT { Env.TeardownDefender(); };
	UMordecaiAttackProfileDataAsset* Slash = MakePlayerSlash();
	if (!TestTrue("player setup", Env.SetupDefender(FVector::ZeroVector, { Slash })))
	{
		return true;
	}
	PinPlayerYellow(Env);
	Env.GetPlayerASC()->SetNumericAttributeBase(
		UMordecaiAttributeSet::GetOutgoingPostureDamageMultiplierAttribute(), 0.5f);
	AMordecaiEnemyCharacter* Enemy =
		Env.SpawnEnemy(FVector(100.f, 0.f, 0.f), { Slash });
	const float Before = GetAttribute(Enemy, UMordecaiAttributeSet::GetPostureAttribute());
	TestTrue("weakened melee lands", LandPlayerMelee(Env));
	TestEqual(
		"outgoing 0.5 halves posture dealt",
		GetAttribute(Enemy, UMordecaiAttributeSet::GetPostureAttribute()),
		Before - 8.f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_Posture_Runtime_BrittleIncreasesPostureTaken,
	"Mordecai.Posture.Runtime.BrittleIncreasesPostureTaken",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_Posture_Runtime_BrittleIncreasesPostureTaken::RunTest(const FString& Parameters)
{
	using namespace MordecaiPostureRuntimeTest;
	using namespace MordecaiRecoveryDefenseTest;
	FDefenseTestEnv Env;
	ON_SCOPE_EXIT { Env.TeardownDefender(); };
	UMordecaiAttackProfileDataAsset* Slash = MakePlayerSlash();
	if (!TestTrue("player setup", Env.SetupDefender(FVector::ZeroVector, { Slash })))
	{
		return true;
	}
	PinPlayerYellow(Env);
	AMordecaiEnemyCharacter* Enemy =
		Env.SpawnEnemy(FVector(100.f, 0.f, 0.f), { Slash });
	Enemy->GetAbilitySystemComponent()->SetNumericAttributeBase(
		UMordecaiAttributeSet::GetIncomingPostureDamageMultiplierAttribute(), 2.f);
	const float Before = GetAttribute(Enemy, UMordecaiAttributeSet::GetPostureAttribute());
	TestTrue("brittle melee lands", LandPlayerMelee(Env));
	TestEqual(
		"incoming 2.0 doubles posture taken",
		GetAttribute(Enemy, UMordecaiAttributeSet::GetPostureAttribute()),
		Before - 32.f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_Posture_Runtime_ParryUsesPostureHelper,
	"Mordecai.Posture.Runtime.ParryUsesPostureHelper",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_Posture_Runtime_ParryUsesPostureHelper::RunTest(const FString& Parameters)
{
	using namespace MordecaiPostureRuntimeTest;
	using namespace MordecaiRecoveryDefenseTest;
	FDefenseTestEnv Env;
	ON_SCOPE_EXIT { Env.TeardownDefender(); };
	if (!TestTrue("player setup", Env.SetupDefender(FVector(100.f, 0.f, 0.f))))
	{
		return true;
	}
	AMordecaiEnemyCharacter* Enemy = Env.SpawnAttacker(MakePlayerSlash());
	UMordecaiGA_Parry* Parry = Env.FindInstance<UMordecaiGA_Parry>();
	if (!TestNotNull("parry", Parry) || !TestNotNull("enemy", Enemy))
	{
		return true;
	}
	SetEnemyAttribute(
		Enemy,
		UMordecaiAttributeSet::GetPostureAttribute(),
		Parry->ParryPostureDamage);
	Env.Press(MordecaiGameplayTags::Input_Parry);
	TestFalse("parried swing is cancelled", Env.LandSwing(Enemy));
	TestTrue("parry breaks posture through runtime", EnemyHas(
		Enemy, MordecaiGameplayTags::State_PostureBroken));
	TestTrue("parried state is present", EnemyHas(
		Enemy, MordecaiGameplayTags::State_Parried));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_Posture_Runtime_RegenUsesRecoveryMultiplier,
	"Mordecai.Posture.Runtime.RegenUsesRecoveryMultiplier",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_Posture_Runtime_RegenUsesRecoveryMultiplier::RunTest(const FString& Parameters)
{
	using namespace MordecaiPostureRuntimeTest;
	using namespace MordecaiRecoveryDefenseTest;
	FDefenseTestEnv Env;
	ON_SCOPE_EXIT { Env.TeardownDefender(); };
	if (!TestTrue("player setup", Env.SetupDefender(FVector::ZeroVector)))
	{
		return true;
	}
	AMordecaiEnemyCharacter* Enemy =
		Env.SpawnEnemy(FVector(100.f, 0.f, 0.f), { MakePlayerSlash() });
	Enemy->GetAbilitySystemComponent()->SetNumericAttributeBase(
		UMordecaiAttributeSet::GetPostureRecoveryMultiplierAttribute(), 2.f);
	SetEnemyAttribute(Enemy, UMordecaiAttributeSet::GetPostureAttribute(), 50.f);
	Env.TickFrames(61);
	const float Before = GetAttribute(Enemy, UMordecaiAttributeSet::GetPostureAttribute());
	Enemy->RegeneratePosture(0.5f);
	TestEqual(
		"enemy regen uses 2.0 recovery multiplier",
		GetAttribute(Enemy, UMordecaiAttributeSet::GetPostureAttribute()),
		Before + 10.f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_Posture_Runtime_ParriedExpires,
	"Mordecai.Posture.Runtime.ParriedExpires",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_Posture_Runtime_ParriedExpires::RunTest(const FString& Parameters)
{
	using namespace MordecaiPostureRuntimeTest;
	using namespace MordecaiRecoveryDefenseTest;
	FDefenseTestEnv Env;
	ON_SCOPE_EXIT { Env.TeardownDefender(); };
	if (!TestTrue("player setup", Env.SetupDefender(FVector::ZeroVector)))
	{
		return true;
	}
	AMordecaiEnemyCharacter* Enemy =
		Env.SpawnEnemy(FVector(100.f, 0.f, 0.f), { MakePlayerSlash() });
	Enemy->EnterParriedState();
	TestTrue("Parried starts", EnemyHas(Enemy, MordecaiGameplayTags::State_Parried));
	Env.TickFrames(21);
	TestFalse("Parried expires after one second", EnemyHas(
		Enemy, MordecaiGameplayTags::State_Parried));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_Posture_Runtime_RiposteExpiresOrConsumed,
	"Mordecai.Posture.Runtime.RiposteExpiresOrConsumed",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_Posture_Runtime_RiposteExpiresOrConsumed::RunTest(const FString& Parameters)
{
	using namespace MordecaiPostureRuntimeTest;
	using namespace MordecaiRecoveryDefenseTest;
	FDefenseTestEnv Env;
	ON_SCOPE_EXIT { Env.TeardownDefender(); };
	UMordecaiAttackProfileDataAsset* Slash = MakePlayerSlash(16.f, 0.f);
	if (!TestTrue("player setup", Env.SetupDefender(FVector(100.f, 0.f, 0.f), { Slash })))
	{
		return true;
	}
	PinPlayerYellow(Env);
	AMordecaiEnemyCharacter* ParriedEnemy = Env.SpawnAttacker(MakePlayerSlash());
	Env.Press(MordecaiGameplayTags::Input_Parry);
	TestFalse("parried swing is cancelled", Env.LandSwing(ParriedEnemy));
	TestTrue("Riposte starts", Env.PlayerHas(MordecaiGameplayTags::State_Riposte));

	ParriedEnemy->SetActorLocation(FVector(1000.f, 0.f, 0.f));
	AMordecaiEnemyCharacter* OtherEnemy =
		Env.SpawnEnemy(Env.Player->GetActorLocation() + FVector(100.f, 0.f, 0.f), { Slash });
	TestNotNull("different target", OtherEnemy);
	TestTrue("melee lands on another target", LandPlayerMelee(Env));
	TestFalse("any landed player melee consumes Riposte", Env.PlayerHas(
		MordecaiGameplayTags::State_Riposte));

	// Use a fresh runtime so the expiry assertion cannot inherit attack/combo
	// timers from the consumption route above.
	Env.TeardownDefender();
	if (!TestTrue("expiry player setup", Env.SetupDefender(FVector(100.f, 0.f, 0.f))))
	{
		return true;
	}
	AMordecaiEnemyCharacter* SecondParried = Env.SpawnAttacker(MakePlayerSlash());
	Env.Press(MordecaiGameplayTags::Input_Parry);
	TestFalse("second parried swing is cancelled", Env.LandSwing(SecondParried));
	TestTrue("second Riposte starts", Env.PlayerHas(MordecaiGameplayTags::State_Riposte));
	Env.TickFrames(19);
	TestTrue("Riposte remains before one second", Env.PlayerHas(
		MordecaiGameplayTags::State_Riposte));
	Env.TickFrames(3);
	TestFalse("Riposte expires after one second", Env.PlayerHas(
		MordecaiGameplayTags::State_Riposte));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_Posture_Runtime_ResetClearsAllPostureStates,
	"Mordecai.Posture.Runtime.ResetClearsAllPostureStates",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_Posture_Runtime_ResetClearsAllPostureStates::RunTest(const FString& Parameters)
{
	using namespace MordecaiPostureRuntimeTest;
	using namespace MordecaiRecoveryDefenseTest;
	FDefenseTestEnv Env;
	ON_SCOPE_EXIT { Env.TeardownDefender(); };
	if (!TestTrue("player setup", Env.SetupDefender(FVector::ZeroVector)))
	{
		return true;
	}
	auto AddAllStates = [](AMordecaiEnemyCharacter* Enemy)
	{
		for (const FGameplayTag Tag : {
			MordecaiGameplayTags::State_PostureBroken.GetTag(),
			MordecaiGameplayTags::State_KnockedDown.GetTag(),
			MordecaiGameplayTags::State_Parried.GetTag(),
			MordecaiGameplayTags::State_Riposte.GetTag() })
		{
			Enemy->GetAbilitySystemComponent()->SetLooseGameplayTagCount(
				Tag,
				1,
				EGameplayTagReplicationState::TagOnly);
		}
	};
	auto AllClear = [](const AMordecaiEnemyCharacter* Enemy)
	{
		for (const FGameplayTag Tag : {
			MordecaiGameplayTags::State_PostureBroken.GetTag(),
			MordecaiGameplayTags::State_KnockedDown.GetTag(),
			MordecaiGameplayTags::State_Parried.GetTag(),
			MordecaiGameplayTags::State_Riposte.GetTag() })
		{
			if (EnemyHas(Enemy, Tag))
			{
				return false;
			}
		}
		return true;
	};

	AMordecaiEnemyCharacter* ResetEnemy =
		Env.SpawnEnemy(FVector(100.f, 0.f, 0.f), { MakePlayerSlash() });
	AddAllStates(ResetEnemy);
	ResetEnemy->ResetForArena();
	TestTrue("arena reset clears every posture state", AllClear(ResetEnemy));

	AMordecaiEnemyCharacter* DeadEnemy =
		Env.SpawnEnemy(FVector(100.f, 300.f, 0.f), { MakePlayerSlash() });
	AddAllStates(DeadEnemy);
	MordecaiRecoveryTest::FRecoveryTestEnv::ApplyHealthDelta(
		Env.GetPlayerASC(), DeadEnemy, -1000.f);
	TestTrue("death clears every posture state", AllClear(DeadEnemy));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_Posture_Runtime_RiposteHitIsCritical,
	"Mordecai.Posture.Runtime.RiposteHitIsCritical",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_Posture_Runtime_RiposteHitIsCritical::RunTest(const FString& Parameters)
{
	using namespace MordecaiPostureRuntimeTest;
	using namespace MordecaiRecoveryDefenseTest;
	FDefenseTestEnv Env;
	ON_SCOPE_EXIT { Env.TeardownDefender(); };
	UMordecaiAttackProfileDataAsset* PlayerSlash = MakePlayerSlash(16.f, 0.f);
	if (!TestTrue("player setup", Env.SetupDefender(
		FVector(100.f, 0.f, 0.f), { PlayerSlash })))
	{
		return true;
	}
	PinPlayerYellow(Env);
	AMordecaiEnemyCharacter* Enemy = Env.SpawnAttacker(MakePlayerSlash());
	Env.Press(MordecaiGameplayTags::Input_Parry);
	TestFalse("parried swing cancelled", Env.LandSwing(Enemy));
	TestTrue("Riposte granted", Env.PlayerHas(MordecaiGameplayTags::State_Riposte));
	const float Before = GetAttribute(Enemy, UMordecaiAttributeSet::GetHealthAttribute());
	TestTrue("riposte hit lands", LandPlayerMelee(Env));
	TestEqual(
		"riposte takes 16 to 24",
		GetAttribute(Enemy, UMordecaiAttributeSet::GetHealthAttribute()),
		Before - 24.f);
	TestTrue("riposte pop is critical", Env.LastPopIsCritical());
	TestFalse("riposte is consumed", Env.PlayerHas(MordecaiGameplayTags::State_Riposte));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_Posture_Runtime_RiposteMultiTargetDecidedPerSwing,
	"Mordecai.Posture.Runtime.RiposteMultiTargetDecidedPerSwing",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_Posture_Runtime_RiposteMultiTargetDecidedPerSwing::RunTest(const FString& Parameters)
{
	using namespace MordecaiPostureRuntimeTest;
	using namespace MordecaiRecoveryDefenseTest;

	FDefenseTestEnv Env;
	ON_SCOPE_EXIT { Env.TeardownDefender(); };
	UMordecaiAttackProfileDataAsset* Slash = MakePlayerSlash(16.f, 0.f);
	if (!TestTrue("player setup", Env.SetupDefender(FVector::ZeroVector, { Slash })))
	{
		return true;
	}
	PinPlayerYellow(Env);
	AMordecaiEnemyCharacter* Left =
		Env.SpawnEnemy(FVector(100.f, -30.f, 0.f), { Slash });
	AMordecaiEnemyCharacter* Right =
		Env.SpawnEnemy(FVector(100.f, 30.f, 0.f), { Slash });
	if (!TestNotNull("left enemy", Left) || !TestNotNull("right enemy", Right))
	{
		return true;
	}
	Left->EnterParriedState();
	Right->EnterParriedState();
	Env.GetPlayerASC()->SetLooseGameplayTagCount(
		MordecaiGameplayTags::State_Riposte,
		1,
		EGameplayTagReplicationState::TagOnly);
	const float LeftBefore = GetAttribute(Left, UMordecaiAttributeSet::GetHealthAttribute());
	const float RightBefore = GetAttribute(Right, UMordecaiAttributeSet::GetHealthAttribute());

	TestTrue("multi-target riposte swing lands", LandPlayerMelee(Env));
	TestEqual(
		"left target receives the swing-level riposte decision",
		GetAttribute(Left, UMordecaiAttributeSet::GetHealthAttribute()),
		LeftBefore - 24.f);
	TestEqual(
		"right target receives the same swing-level riposte decision",
		GetAttribute(Right, UMordecaiAttributeSet::GetHealthAttribute()),
		RightBefore - 24.f);
	TestEqual("one damage pop is emitted per target", Env.PopCount(), 2);
	TestTrue("the final multi-target pop is also critical", Env.LastPopIsCritical());
	TestFalse("the swing consumes Riposte once after resolving its targets", Env.PlayerHas(
		MordecaiGameplayTags::State_Riposte));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_Posture_Runtime_RiposteDoesNotStackWithStagger,
	"Mordecai.Posture.Runtime.RiposteDoesNotStackWithStagger",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_Posture_Runtime_RiposteDoesNotStackWithStagger::RunTest(const FString& Parameters)
{
	using namespace MordecaiPostureRuntimeTest;
	using namespace MordecaiRecoveryDefenseTest;
	FDefenseTestEnv Env;
	ON_SCOPE_EXIT { Env.TeardownDefender(); };
	UMordecaiAttackProfileDataAsset* PlayerSlash = MakePlayerSlash(16.f, 0.f);
	if (!TestTrue("player setup", Env.SetupDefender(
		FVector(100.f, 0.f, 0.f), { PlayerSlash })))
	{
		return true;
	}
	PinPlayerYellow(Env);
	AMordecaiEnemyCharacter* Enemy = Env.SpawnAttacker(MakePlayerSlash());
	Env.Press(MordecaiGameplayTags::Input_Parry);
	TestFalse("parried swing cancelled", Env.LandSwing(Enemy));
	Enemy->EnterPostureBrokenState();
	const float Before = GetAttribute(Enemy, UMordecaiAttributeSet::GetHealthAttribute());
	TestTrue("combined riposte/stagger hit lands", LandPlayerMelee(Env));
	TestEqual(
		"riposte and stagger choose 1.5 instead of stacking to 2.25",
		GetAttribute(Enemy, UMordecaiAttributeSet::GetHealthAttribute()),
		Before - 24.f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_Posture_Runtime_PlayerPostureRegenerates,
	"Mordecai.Posture.Runtime.PlayerPostureRegenerates",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_Posture_Runtime_PlayerPostureRegenerates::RunTest(const FString& Parameters)
{
	using namespace MordecaiRecoveryDefenseTest;
	FDefenseTestEnv Env;
	ON_SCOPE_EXIT { Env.TeardownDefender(); };
	if (!TestTrue("player setup", Env.SetupDefender(FVector::ZeroVector)))
	{
		return true;
	}
	Env.PlayerState->DispatchBeginPlay();
	UMordecaiPostureRuntimeComponent* Runtime =
		Env.PlayerState->FindComponentByClass<UMordecaiPostureRuntimeComponent>();
	if (!TestNotNull("player posture runtime", Runtime))
	{
		return true;
	}
	UAbilitySystemComponent* ASC = Env.GetPlayerASC();
	ASC->SetNumericAttributeBase(
		UMordecaiAttributeSet::GetPostureRecoveryMultiplierAttribute(), 2.f);
	ASC->SetNumericAttributeBase(UMordecaiAttributeSet::GetPostureAttribute(), 50.f);
	for (int32 Frame = 0; Frame < 70; ++Frame)
	{
		++GFrameCounter;
		Env.World->TimeSeconds += FrameSeconds;
		Env.World->GetTimerManager().Tick(FrameSeconds);
		Runtime->PrimaryComponentTick.ExecuteTick(
			FrameSeconds,
			LEVELTICK_All,
			ENamedThreads::GameThread,
			nullptr);
	}
	const float Posture = ASC->GetNumericAttribute(
		UMordecaiAttributeSet::GetPostureAttribute());
	TestTrue(
		"player posture regenerates after its delay",
		Posture > 50.f);
	TestTrue(
		"2.0 recovery multiplier adds about ten posture in the first half second",
		Posture >= 59.f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_Posture_Runtime_ProjectileBreaksPosture,
	"Mordecai.Posture.Runtime.ProjectileBreaksPosture",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_Posture_Runtime_ProjectileBreaksPosture::RunTest(const FString& Parameters)
{
	using namespace MordecaiPostureRuntimeTest;
	using namespace MordecaiRecoveryDefenseTest;
	FDefenseTestEnv Env;
	ON_SCOPE_EXIT { Env.TeardownDefender(); };
	if (!TestTrue("player setup", Env.SetupDefender(FVector::ZeroVector)))
	{
		return true;
	}
	AMordecaiEnemyCharacter* Enemy =
		Env.SpawnEnemy(FVector(180.f, 0.f, 0.f), { MakePlayerSlash() });
	SetEnemyAttribute(Enemy, UMordecaiAttributeSet::GetPostureAttribute(), 1.f);
	AMordecaiProjectile* Projectile = SpawnProjectile(
		Env,
		Env.Player,
		Env.Player->GetActorLocation() + FVector(0.f, 0.f, 40.f),
		FVector::ForwardVector,
		10.f,
		true);
	TickProjectile(Env, Projectile);
	TestTrue("projectile posture damage breaks the enemy", EnemyHas(
		Enemy, MordecaiGameplayTags::State_PostureBroken));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_Posture_Runtime_PerfectBlockBreaksPosture,
	"Mordecai.Posture.Runtime.PerfectBlockBreaksPosture",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_Posture_Runtime_PerfectBlockBreaksPosture::RunTest(const FString& Parameters)
{
	using namespace MordecaiPostureRuntimeTest;
	using namespace MordecaiRecoveryDefenseTest;
	FDefenseTestEnv Env;
	ON_SCOPE_EXIT { Env.TeardownDefender(); };
	if (!TestTrue("player setup", Env.SetupDefender(FVector(100.f, 0.f, 0.f))))
	{
		return true;
	}
	AMordecaiEnemyCharacter* Enemy = Env.SpawnAttacker(MakePlayerSlash());
	UMordecaiGA_Block* Block = Env.FindInstance<UMordecaiGA_Block>();
	if (!TestNotNull("block", Block) || !TestNotNull("enemy", Enemy))
	{
		return true;
	}
	SetEnemyAttribute(
		Enemy,
		UMordecaiAttributeSet::GetPostureAttribute(),
		Block->PerfectBlockPostureDamage);
	Env.Press(MordecaiGameplayTags::Input_Block);
	TestFalse("posture break cancels the perfect-blocked swing", Env.LandSwing(Enemy));
	TestTrue("perfect block breaks attacker posture", EnemyHas(
		Enemy, MordecaiGameplayTags::State_PostureBroken));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_Posture_Runtime_ProjectileParryDoesNotStagger,
	"Mordecai.Posture.Runtime.ProjectileParryDoesNotStagger",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_Posture_Runtime_ProjectileParryDoesNotStagger::RunTest(const FString& Parameters)
{
	using namespace MordecaiPostureRuntimeTest;
	using namespace MordecaiRecoveryDefenseTest;
	FDefenseTestEnv Env;
	ON_SCOPE_EXIT { Env.TeardownDefender(); };
	if (!TestTrue("player setup", Env.SetupDefender(FVector::ZeroVector)))
	{
		return true;
	}
	AMordecaiEnemyCharacter* Enemy =
		Env.SpawnEnemy(FVector(180.f, 0.f, 0.f), { MakePlayerSlash() });
	Enemy->SetActorRotation(FRotator(0.f, 180.f, 0.f));
	Env.Press(MordecaiGameplayTags::Input_Parry);
	AMordecaiProjectile* Projectile = SpawnProjectile(
		Env,
		Enemy,
		Enemy->GetActorLocation() + FVector(0.f, 0.f, 40.f),
		-FVector::ForwardVector,
		10.f,
		false);
	TickProjectile(Env, Projectile);
	TestFalse("projectile parry does not apply Parried to its source", EnemyHas(
		Enemy, MordecaiGameplayTags::State_Parried));

	Env.TeardownDefender();
	if (!TestTrue("melee control player setup", Env.SetupDefender(FVector(100.f, 0.f, 0.f))))
	{
		return true;
	}
	AMordecaiEnemyCharacter* MeleeEnemy = Env.SpawnAttacker(MakePlayerSlash());
	Env.Press(MordecaiGameplayTags::Input_Parry);
	TestFalse("melee control swing is cancelled", Env.LandSwing(MeleeEnemy));
	TestTrue("melee parry applies Parried to its source", EnemyHas(
		MeleeEnemy, MordecaiGameplayTags::State_Parried));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_Posture_Runtime_BlockedHitNeverCritical,
	"Mordecai.Posture.Runtime.BlockedHitNeverCritical",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_Posture_Runtime_BlockedHitNeverCritical::RunTest(const FString& Parameters)
{
	using namespace MordecaiPostureRuntimeTest;
	using namespace MordecaiRecoveryDefenseTest;
	FDefenseTestEnv Env;
	ON_SCOPE_EXIT { Env.TeardownDefender(); };
	if (!TestTrue("player setup", Env.SetupDefender(FVector(100.f, 0.f, 0.f))))
	{
		return true;
	}
	AMordecaiEnemyCharacter* Enemy = Env.SpawnAttacker(MakePlayerSlash());
	Env.Press(MordecaiGameplayTags::Input_Block);
	Env.TickFrames(8);
	TestTrue("blocking is active", Env.PlayerHas(MordecaiGameplayTags::State_Blocking));
	Env.GetPlayerASC()->SetLooseGameplayTagCount(
		MordecaiGameplayTags::State_PostureBroken,
		1,
		EGameplayTagReplicationState::TagOnly);
	TestTrue("blocked swing resolves", Env.LandSwing(Enemy));
	TestEqual("blocked hit emits one pop", Env.PopCount(), 1);
	TestEqual("blocked vulnerable hit reports mitigated damage", Env.LastPopAmount(), 12);
	TestFalse("blocked hit is never critical", Env.LastPopIsCritical());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_Playtest_BridgeState_EnemyPosture,
	"Mordecai.Playtest.BridgeState.EnemyPosture",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_Playtest_BridgeState_EnemyPosture::RunTest(const FString& Parameters)
{
	AMordecaiEnemyCharacter* Enemy = NewObject<AMordecaiEnemyCharacter>();
	UMordecaiAttributeSet* Attributes = Enemy->GetMordecaiAttributeSet();
	if (!TestNotNull("enemy attribute set", Attributes))
	{
		return true;
	}
	Attributes->InitPosture(37.f);
	Attributes->InitMaxPosture(80.f);
	const TSharedRef<FJsonObject> Entry =
		MordecaiAgentBridge::State::MakeActorEntry(Enemy, nullptr);
	double Posture = -1.0;
	double MaxPosture = -1.0;
	TestTrue("actors[] enemy carries posture", Entry->TryGetNumberField(TEXT("posture"), Posture));
	TestTrue("actors[] enemy carries max_posture", Entry->TryGetNumberField(TEXT("max_posture"), MaxPosture));
	TestEqual("posture value", Posture, 37.0);
	TestEqual("max_posture value", MaxPosture, 80.0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_Posture_Runtime_PlayerBreakLocksAndRecovers,
	"Mordecai.Posture.Runtime.PlayerBreakLocksAndRecovers",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_Posture_Runtime_PlayerBreakLocksAndRecovers::RunTest(const FString& Parameters)
{
	using namespace MordecaiPostureRuntimeTest;
	using namespace MordecaiRecoveryDefenseTest;

	FDefenseTestEnv Env;
	ON_SCOPE_EXIT { Env.TeardownDefender(); };
	UMordecaiAttackProfileDataAsset* PlayerSlash = MakePlayerSlash(10.f, 0.f);
	PlayerSlash->WindupTimeMs = 5000.f;
	if (!TestTrue("player setup", Env.SetupDefender(FVector(100.f, 0.f, 0.f), { PlayerSlash }))
		|| !TestNotNull("player runtime", StartPlayerPostureRuntime(Env)))
	{
		return true;
	}
	AMordecaiEnemyCharacter* Enemy = Env.SpawnAttacker(MakePlayerSlash(10.f, 1.f));
	UAbilitySystemComponent* ASC = Env.GetPlayerASC();
	FGameplayAbilitySpec* PlayerMelee =
		Env.FindSpec(UMordecaiGA_MeleeAttack::StaticClass());
	UCharacterMovementComponent* Movement = Env.Player->GetCharacterMovement();
	if (!TestNotNull("enemy", Enemy) || !TestNotNull("player ASC", ASC)
		|| !TestNotNull("player melee", PlayerMelee) || !TestNotNull("movement", Movement))
	{
		return true;
	}

	SetPlayerAttribute(Env, UMordecaiAttributeSet::GetPostureAttribute(), 100.f);
	const float ControlPosture = Env.GetPlayer(UMordecaiAttributeSet::GetPostureAttribute());
	TestTrue("control enemy melee lands", Env.LandSwing(Enemy));
	TestTrue(
		"positive control proves real enemy melee damages player posture",
		Env.GetPlayer(UMordecaiAttributeSet::GetPostureAttribute()) < ControlPosture);
	TestFalse("positive control remains unbroken", Env.PlayerHas(
		MordecaiGameplayTags::State_PostureBroken));
	Env.FinishSwing(Enemy);

	Movement->SetMovementMode(MOVE_Walking);
	Env.Player->SetSprinting(true);
	TestTrue("player sprint is active before break", Env.Player->bIsSprinting);
	SetPlayerAttribute(Env, UMordecaiAttributeSet::GetPostureAttribute(), 0.f);
	TestTrue("posture zero starts the sprint-lock control break", Env.PlayerHas(
		MordecaiGameplayTags::State_PostureBroken));
	TestFalse("break clears sprint", Env.Player->bIsSprinting);
	TestEqual("break locks movement", Movement->MovementMode, MOVE_None);
	Env.TickFrames(31);
	TestFalse("sprint-lock control break recovers", Env.PlayerHas(
		MordecaiGameplayTags::State_PostureBroken));

	Env.Press(MordecaiGameplayTags::Input_LightAttack);
	TestTrue("player swing is active before breaking hit", PlayerMelee->IsActive());
	SetPlayerAttribute(Env, UMordecaiAttributeSet::GetPostureAttribute(), 1.f);
	TestTrue("breaking enemy melee lands", Env.LandSwing(Enemy));
	TestTrue("authority player receives PostureBroken", Env.PlayerHas(
		MordecaiGameplayTags::State_PostureBroken));
	TestFalse("break cancels the in-flight player ability", PlayerMelee->IsActive());
	TestEqual("break locks movement", Movement->MovementMode, MOVE_None);

	Env.Press(MordecaiGameplayTags::Input_LightAttack);
	TestFalse("break blocks a new player melee", PlayerMelee->IsActive());
	Env.TickFrames(29);
	TestTrue("break remains before 1.5 seconds", Env.PlayerHas(
		MordecaiGameplayTags::State_PostureBroken));
	TestEqual("movement remains locked before recovery", Movement->MovementMode, MOVE_None);
	Env.TickFrames(2);
	TestFalse("break expires after 1.5 seconds", Env.PlayerHas(
		MordecaiGameplayTags::State_PostureBroken));
	TestEqual(
		"recovery restores maximum posture",
		Env.GetPlayer(UMordecaiAttributeSet::GetPostureAttribute()),
		Env.GetPlayer(UMordecaiAttributeSet::GetMaxPostureAttribute()));
	TestEqual("recovery restores walking", Movement->MovementMode, MOVE_Walking);
	Env.Press(MordecaiGameplayTags::Input_LightAttack);
	TestTrue("player melee activates again after recovery", PlayerMelee->IsActive());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_Posture_Runtime_PlayerBreakIsServerOnly,
	"Mordecai.Posture.Runtime.PlayerBreakIsServerOnly",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_Posture_Runtime_PlayerBreakIsServerOnly::RunTest(const FString& Parameters)
{
	using namespace MordecaiPostureRuntimeTest;
	using namespace MordecaiRecoveryDefenseTest;

	FDefenseTestEnv Env;
	ON_SCOPE_EXIT { Env.TeardownDefender(); };
	if (!TestTrue("player setup", Env.SetupDefender(FVector::ZeroVector)))
	{
		return true;
	}
	Env.PlayerState->SetRole(ROLE_SimulatedProxy);
	UMordecaiPostureRuntimeComponent* Runtime = StartPlayerPostureRuntime(Env);
	if (!TestNotNull("client-copy runtime", Runtime))
	{
		return true;
	}
	SetPlayerAttribute(Env, UMordecaiAttributeSet::GetPostureAttribute(), 0.f);
	TestFalse("non-authority player never starts a break", Env.PlayerHas(
		MordecaiGameplayTags::State_PostureBroken));
	TestEqual(
		"non-authority player does not restore replicated posture",
		Env.GetPlayer(UMordecaiAttributeSet::GetPostureAttribute()),
		0.f);
	Env.TickFrames(31);
	TestFalse("non-authority player has no break timer to expire", Env.PlayerHas(
		MordecaiGameplayTags::State_PostureBroken));
	TestEqual(
		"non-authority posture remains server-owned",
		Env.GetPlayer(UMordecaiAttributeSet::GetPostureAttribute()),
		0.f);

	Env.TeardownDefender();
	if (!TestTrue("authority positive-control setup", Env.SetupDefender(FVector::ZeroVector))
		|| !TestNotNull("authority positive-control runtime", StartPlayerPostureRuntime(Env)))
	{
		return true;
	}
	SetPlayerAttribute(Env, UMordecaiAttributeSet::GetPostureAttribute(), 1.f);
	SetPlayerAttribute(Env, UMordecaiAttributeSet::GetPostureAttribute(), 0.f);
	TestTrue("authority positive control starts the same player break", Env.PlayerHas(
		MordecaiGameplayTags::State_PostureBroken));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_Posture_Runtime_PlayerVulnerabilityMeleeAndProjectile,
	"Mordecai.Posture.Runtime.PlayerVulnerabilityMeleeAndProjectile",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_Posture_Runtime_PlayerVulnerabilityMeleeAndProjectile::RunTest(const FString& Parameters)
{
	using namespace MordecaiPostureRuntimeTest;
	using namespace MordecaiRecoveryDefenseTest;

	FDefenseTestEnv Env;
	ON_SCOPE_EXIT { Env.TeardownDefender(); };
	if (!TestTrue("player setup", Env.SetupDefender(FVector::ZeroVector))
		|| !TestNotNull("player runtime", StartPlayerPostureRuntime(Env)))
	{
		return true;
	}
	AMordecaiEnemyCharacter* Enemy =
		Env.SpawnEnemy(FVector(180.f, 0.f, 0.f), { MakePlayerSlash(10.f, 0.f) });
	if (!TestNotNull("enemy", Enemy))
	{
		return true;
	}
	SetFalling(Enemy, false);
	Env.PlaceAttackerAtDefenseAngle(Enemy, 0.f);
	SetPlayerAttribute(Env, UMordecaiAttributeSet::GetMaxHealthAttribute(), 500.f);
	SetPlayerAttribute(Env, UMordecaiAttributeSet::GetHealthAttribute(), 500.f);

	const float MeleeControlBefore =
		Env.GetPlayer(UMordecaiAttributeSet::GetHealthAttribute());
	TestTrue("unbroken melee control lands", Env.LandSwing(Enemy));
	TestEqual(
		"unbroken melee control takes base damage",
		Env.GetPlayer(UMordecaiAttributeSet::GetHealthAttribute()),
		MeleeControlBefore - 10.f);
	Env.FinishSwing(Enemy);

	SetPlayerAttribute(Env, UMordecaiAttributeSet::GetPostureAttribute(), 0.f);
	TestTrue("real posture attribute path enters break", Env.PlayerHas(
		MordecaiGameplayTags::State_PostureBroken));
	const float MeleeBrokenBefore =
		Env.GetPlayer(UMordecaiAttributeSet::GetHealthAttribute());
	TestTrue("broken melee lands", Env.LandSwing(Enemy));
	TestEqual(
		"broken player takes 1.5x enemy melee damage",
		Env.GetPlayer(UMordecaiAttributeSet::GetHealthAttribute()),
		MeleeBrokenBefore - 15.f);
	Env.FinishSwing(Enemy);

	Env.TickFrames(31);
	const float ProjectileControlBefore =
		Env.GetPlayer(UMordecaiAttributeSet::GetHealthAttribute());
	AMordecaiProjectile* ControlProjectile = SpawnProjectile(
		Env,
		Enemy,
		Enemy->GetActorLocation() + FVector(0.f, 0.f, 40.f),
		-FVector::ForwardVector,
		10.f,
		false);
	TickProjectile(Env, ControlProjectile);
	TestEqual(
		"unbroken projectile control takes base damage",
		Env.GetPlayer(UMordecaiAttributeSet::GetHealthAttribute()),
		ProjectileControlBefore - 10.f);

	SetPlayerAttribute(Env, UMordecaiAttributeSet::GetPostureAttribute(), 0.f);
	const float ProjectileBrokenBefore =
		Env.GetPlayer(UMordecaiAttributeSet::GetHealthAttribute());
	AMordecaiProjectile* BrokenProjectile = SpawnProjectile(
		Env,
		Enemy,
		Enemy->GetActorLocation() + FVector(0.f, 0.f, 40.f),
		-FVector::ForwardVector,
		10.f,
		false);
	TickProjectile(Env, BrokenProjectile);
	TestEqual(
		"broken player takes 1.5x enemy projectile damage",
		Env.GetPlayer(UMordecaiAttributeSet::GetHealthAttribute()),
		ProjectileBrokenBefore - 15.f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_Posture_Runtime_PlayerKnockdownDeathAndRespawn,
	"Mordecai.Posture.Runtime.PlayerKnockdownDeathAndRespawn",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_Posture_Runtime_PlayerKnockdownDeathAndRespawn::RunTest(const FString& Parameters)
{
	using namespace MordecaiPostureRuntimeTest;
	using namespace MordecaiRecoveryDefenseTest;

	FDefenseTestEnv Env;
	ON_SCOPE_EXIT { Env.TeardownDefender(); };
	if (!TestTrue("player setup", Env.SetupDefender(FVector::ZeroVector))
		|| !TestNotNull("player runtime", StartPlayerPostureRuntime(Env)))
	{
		return true;
	}
	AMordecaiEnemyCharacter* Enemy = Env.SpawnAttacker(MakePlayerSlash(16.f, 0.f));
	UAbilitySystemComponent* EnemyASC = Enemy ? Enemy->GetAbilitySystemComponent() : nullptr;
	if (!TestNotNull("enemy", Enemy) || !TestNotNull("enemy ASC", EnemyASC))
	{
		return true;
	}
	SetPlayerAttribute(Env, UMordecaiAttributeSet::GetMaxHealthAttribute(), 500.f);
	SetPlayerAttribute(Env, UMordecaiAttributeSet::GetHealthAttribute(), 500.f);
	SetPlayerAttribute(Env, UMordecaiAttributeSet::GetPostureAttribute(), 0.f);
	TestTrue("player break starts", Env.PlayerHas(MordecaiGameplayTags::State_PostureBroken));

	MordecaiRecoveryTest::FRecoveryTestEnv::ApplyHealthDelta(EnemyASC, Env.Player, -39.f);
	TestTrue("39 enemy health damage keeps break", Env.PlayerHas(
		MordecaiGameplayTags::State_PostureBroken));
	TestFalse("39 enemy health damage does not knock down", Env.PlayerHas(
		MordecaiGameplayTags::State_KnockedDown));
	MordecaiRecoveryTest::FRecoveryTestEnv::ApplyHealthDelta(EnemyASC, Env.Player, -1.f);
	TestFalse("40 enemy health damage replaces break", Env.PlayerHas(
		MordecaiGameplayTags::State_PostureBroken));
	TestTrue("40 enemy health damage knocks player down", Env.PlayerHas(
		MordecaiGameplayTags::State_KnockedDown));

	const float DownBefore = Env.GetPlayer(UMordecaiAttributeSet::GetHealthAttribute());
	TestTrue("knocked-down melee lands", Env.LandSwing(Enemy));
	TestEqual(
		"knocked-down player takes 2x enemy melee damage",
		Env.GetPlayer(UMordecaiAttributeSet::GetHealthAttribute()),
		DownBefore - 32.f);
	Env.TickFrames(35);
	TestTrue("knockdown remains before two seconds", Env.PlayerHas(
		MordecaiGameplayTags::State_KnockedDown));
	Env.TickFrames(4);
	TestFalse("knockdown expires after two seconds", Env.PlayerHas(
		MordecaiGameplayTags::State_KnockedDown));

	UAbilitySystemComponent* ASC = Env.GetPlayerASC();
	for (const FGameplayTag Tag : {
		MordecaiGameplayTags::State_PostureBroken.GetTag(),
		MordecaiGameplayTags::State_KnockedDown.GetTag(),
		MordecaiGameplayTags::State_Parried.GetTag(),
		MordecaiGameplayTags::State_Riposte.GetTag() })
	{
		ASC->SetLooseGameplayTagCount(Tag, 1, EGameplayTagReplicationState::TagOnly);
	}
	MordecaiRecoveryTest::FRecoveryTestEnv::ApplyHealthDelta(EnemyASC, Env.Player, -1000.f);
	TestTrue("real enemy damage kills the player", Env.Player->IsDead());
	for (const FGameplayTag Tag : {
		MordecaiGameplayTags::State_PostureBroken.GetTag(),
		MordecaiGameplayTags::State_KnockedDown.GetTag(),
		MordecaiGameplayTags::State_Parried.GetTag(),
		MordecaiGameplayTags::State_Riposte.GetTag() })
	{
		TestFalse(
			FString::Printf(TEXT("death clears %s"), *Tag.ToString()),
			ASC->HasMatchingGameplayTag(Tag));
	}

	Env.Player->HandlePlayerRespawn();
	TestFalse("respawn removes death", Env.Player->IsDead());
	TestEqual(
		"respawn restores maximum posture",
		Env.GetPlayer(UMordecaiAttributeSet::GetPostureAttribute()),
		Env.GetPlayer(UMordecaiAttributeSet::GetMaxPostureAttribute()));
	return true;
}
