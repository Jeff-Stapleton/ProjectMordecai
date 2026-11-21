// Project Mordecai — Recovery: block, parry, dodge, Blink and jump defend against enemy melee (US-101, issue #53 / N32)
// Every test lands a real enemy swing: the enemy's UMordecaiGA_MeleeAttack activates and its windup timer runs the hit
// query and ApplyDamageToTarget. The player's defenses are its real combat abilities, granted by the controller and
// raised through their input tags, and Blink is cast. The tests read Health, Stamina and Posture, not just tags.

#include "Misc/AutomationTest.h"
#include "Misc/ScopeExit.h"
#include "AbilitySystem/LyraAbilitySystemComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameplayAbilitySpec.h"
#include "TimerManager.h"
#include "UObject/SoftObjectPath.h"
#include "Mordecai/Combat/MordecaiGA_Block.h"
#include "Mordecai/Combat/MordecaiGA_Dodge.h"
#include "Mordecai/Combat/MordecaiGA_Parry.h"
#include "Mordecai/Combat/MordecaiHitDefense.h"
#include "Mordecai/Combat/MordecaiHitDetectionSubsystem.h"
#include "Mordecai/Combat/MordecaiPostureSystem.h"
#include "Mordecai/Magic/MordecaiGA_Blink.h"
#include "Mordecai/Magic/MordecaiSpellDataAsset.h"
#include "Mordecai/MordecaiGameplayTags.h"
#include "Mordecai/MordecaiPlayerController.h"
#include "Mordecai/StatusEffects/Effects/MordecaiGE_Weakened.h"
#include "Mordecai/UI/MordecaiDamagePopComponent.h"
#include "Mordecai/Weapons/MordecaiEquipmentComponent.h"
#include "Mordecai/Weapons/MordecaiShieldDataAsset.h"
#include "Tests/Recovery/MordecaiDefenseTestEnv.h"
#include "Tests/Recovery/MordecaiRecoveryTestEnv.h"

namespace MordecaiRecoveryDefenseTest
{
	/** DA_EnemyAttack_BasicSlash's swing (150 cm / 90 degrees, 15 Slash, posture scalar 0.3, jump-avoidable), 0.1 s phases. */
	inline UMordecaiAttackProfileDataAsset* MakeSlashProfile()
	{
		UMordecaiAttackProfileDataAsset* Profile = MordecaiRecoveryTest::MakeStubProfile();
		Profile->HitShapeParams.Radius = 150.f;
		Profile->HitShapeParams.Angle = 90.f;
		Profile->DamageProfile.BasePower = 15.f;
		Profile->PostureDamageScalar = 0.3f;
		Profile->JumpAvoidable = true;
		return Profile;
	}

	/** The same swing with an on-hit status effect that never touches Health: Weakened, always applied. */
	inline UMordecaiAttackProfileDataAsset* MakeWeakeningSlashProfile()
	{
		UMordecaiAttackProfileDataAsset* Profile = MakeSlashProfile();
		FMordecaiStatusOnHitEntry& Entry = Profile->StatusEffectsOnHit.AddDefaulted_GetRef();
		Entry.StatusEffectTag = MordecaiGameplayTags::Status_Weakened;
		Entry.ApplicationChance = 1.f;
		Entry.StatusEffectGEClass = UMordecaiGE_Weakened::StaticClass();
		return Profile;
	}

	/** Blink as DA_Spell_Blink times it (invulnerability 0.3 s outlasts the 0.2 s recovery), with no SP cost. */
	inline UMordecaiSpellDataAsset* MakeBlinkData()
	{
		UMordecaiSpellDataAsset* Data = NewObject<UMordecaiSpellDataAsset>();
		Data->SpellId = FName(TEXT("Blink"));
		Data->Range = 500.f;
		Data->BuffDuration = 0.3f;
		Data->DeliveryType = EMordecaiSpellDeliveryType::DashMovement;
		Data->TargetingType = EMordecaiSpellTargetingType::Dash;
		Data->WindupTime = 0.f;
		Data->CastTime = 0.f;
		Data->RecoveryTime = 0.2f;
		Data->SpellPointCost = 0;
		return Data;
	}

}

namespace MordecaiDefenseArcTest
{
	using namespace MordecaiRecoveryDefenseTest;

	struct FHitCosts
	{
		float Health = 0.f;
		float Stamina = 0.f;
		float Posture = 0.f;
	};

	inline FHitCosts LandBlockedSwingAtAngle(FDefenseTestEnv& Env, float DefenseAngleDegrees)
	{
		AMordecaiEnemyCharacter* Enemy = Env.SpawnAttacker(MakeSlashProfile());
		if (!Enemy)
		{
			return { -1.f, -1.f, -1.f };
		}
		Env.PlaceAttackerAtDefenseAngle(Enemy, DefenseAngleDegrees);
		Env.Press(MordecaiGameplayTags::Input_Block);
		Env.TickFrames(8);

		const float HealthBefore = Env.GetPlayer(UMordecaiAttributeSet::GetHealthAttribute());
		const float StaminaBefore = Env.GetPlayer(UMordecaiAttributeSet::GetStaminaAttribute());
		const float PostureBefore = Env.GetPlayer(UMordecaiAttributeSet::GetPostureAttribute());
		if (!Env.LandSwing(Enemy))
		{
			return { -1.f, -1.f, -1.f };
		}
		return {
			HealthBefore - Env.GetPlayer(UMordecaiAttributeSet::GetHealthAttribute()),
			StaminaBefore - Env.GetPlayer(UMordecaiAttributeSet::GetStaminaAttribute()),
			PostureBefore - Env.GetPlayer(UMordecaiAttributeSet::GetPostureAttribute())
		};
	}
}

// ---------------------------------------------------------------------------
// 1. Mordecai.Recovery.Defense.UndefendedHitLandsInFull (AC-101.1)
// The control: a player who does nothing takes the whole swing — health, posture, the on-hit status and one damage
// number — exactly once.
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_Recovery_Defense_UndefendedHitLandsInFull,
	"Mordecai.Recovery.Defense.UndefendedHitLandsInFull",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_Recovery_Defense_UndefendedHitLandsInFull::RunTest(const FString& Parameters)
{
	using namespace MordecaiRecoveryDefenseTest;

	FDefenseTestEnv Env;
	ON_SCOPE_EXIT { Env.TeardownDefender(); };
	if (!TestTrue("defending player set up", Env.SetupDefender(FVector(100.0, 0.0, 0.0))))
	{
		return true;
	}
	AMordecaiEnemyCharacter* Enemy = Env.SpawnAttacker(MakeWeakeningSlashProfile());
	if (!TestNotNull("attacker spawned", Enemy))
	{
		return true;
	}
	TestFalse("the grounded player is not airborne", Env.PlayerHas(MordecaiGameplayTags::State_Airborne));

	if (!TestTrue("the swing lands", Env.LandSwing(Enemy)))
	{
		return true;
	}
	TestEqual("health -15", Env.GetPlayer(UMordecaiAttributeSet::GetHealthAttribute()), 85.f);
	TestEqual("posture -4.5 (15 x 0.3)", Env.GetPlayer(UMordecaiAttributeSet::GetPostureAttribute()), 95.5f);
	TestEqual("stamina untouched", Env.GetPlayer(UMordecaiAttributeSet::GetStaminaAttribute()), 100.f);
	TestTrue("the on-hit status applies", Env.PlayerHas(MordecaiGameplayTags::Status_Weakened));
	TestEqual("one damage number", Env.PopCount(), 1);
	TestEqual("it shows 15", Env.LastPopAmount(), 15);
	return true;
}

// ---------------------------------------------------------------------------
// 1b. Mordecai.Recovery.Defense.PlayerHitsOnEnemiesUnchanged (AC-101.1)
// The defenses sit in the shared melee path: the player's own light attack still lands in full on an enemy.
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_Recovery_Defense_PlayerHitsOnEnemiesUnchanged,
	"Mordecai.Recovery.Defense.PlayerHitsOnEnemiesUnchanged",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_Recovery_Defense_PlayerHitsOnEnemiesUnchanged::RunTest(const FString& Parameters)
{
	using namespace MordecaiRecoveryDefenseTest;

	FDefenseTestEnv Env;
	ON_SCOPE_EXIT { Env.TeardownDefender(); };
	if (!TestTrue("player set up with a slash", Env.SetupDefender(FVector::ZeroVector, { MakeSlashProfile() })))
	{
		return true;
	}
	AMordecaiEnemyCharacter* Enemy = Env.SpawnEnemy(FVector(100.0, 0.0, 0.0), { MakeSlashProfile() });
	if (!TestNotNull("enemy spawned", Enemy))
	{
		return true;
	}
	SetFalling(Enemy, false);
	const float EnemyHealth = GetAttribute(Enemy, UMordecaiAttributeSet::GetHealthAttribute());
	const float EnemyPosture = GetAttribute(Enemy, UMordecaiAttributeSet::GetPostureAttribute());

	Env.Press(MordecaiGameplayTags::Input_LightAttack);
	const UMordecaiGA_MeleeAttack* Melee = Env.FindInstance<UMordecaiGA_MeleeAttack>();
	if (!TestNotNull("the light attack is active", Melee) || !TestTrue("in its windup", Melee->GetCurrentPhase() == EMordecaiAttackPhase::Windup))
	{
		return true;
	}
	for (int32 Frame = 0; Frame < 20 && Melee->GetCurrentPhase() == EMordecaiAttackPhase::Windup; ++Frame)
	{
		Env.TickFrames(1);
	}
	TestEqual("the enemy loses 15 health", GetAttribute(Enemy, UMordecaiAttributeSet::GetHealthAttribute()), EnemyHealth - 15.f);
	TestEqual("and 4.5 posture", GetAttribute(Enemy, UMordecaiAttributeSet::GetPostureAttribute()), EnemyPosture - 4.5f);
	TestEqual("the player shows one damage number", Env.PopCount(), 1);
	return true;
}

// ---------------------------------------------------------------------------
// 2. Mordecai.Recovery.Defense.BlockMitigatesEnemyHit (AC-101.2: AC-006.3, AC-006.4, AC-007.5)
// A held block after its perfect window: the swing costs health x (1 - Mitigation) once, stamina x (1 - Stability),
// posture x BlockPostureDamageMultiplier, and the damage number shows the mitigated amount.
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_Recovery_Defense_BlockMitigatesEnemyHit,
	"Mordecai.Recovery.Defense.BlockMitigatesEnemyHit",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_Recovery_Defense_BlockMitigatesEnemyHit::RunTest(const FString& Parameters)
{
	using namespace MordecaiRecoveryDefenseTest;

	FDefenseTestEnv Env;
	ON_SCOPE_EXIT { Env.TeardownDefender(); };
	if (!TestTrue("defending player set up", Env.SetupDefender(FVector(100.0, 0.0, 0.0))))
	{
		return true;
	}
	AMordecaiEnemyCharacter* Enemy = Env.SpawnAttacker(MakeSlashProfile());
	if (!TestNotNull("attacker spawned", Enemy))
	{
		return true;
	}

	Env.Press(MordecaiGameplayTags::Input_Block);
	const UMordecaiGA_Block* Block = Env.FindInstance<UMordecaiGA_Block>();
	if (!TestNotNull("block instance", Block) || !TestTrue("blocking", Env.PlayerHas(MordecaiGameplayTags::State_Blocking)))
	{
		return true;
	}
	// Hold the guard past its 250 ms perfect window: an ordinary blocked hit
	Env.TickFrames(8);
	TestFalse("the perfect-block window closed", Block->IsPerfectBlockWindowActive());
	TestFalse("no PerfectBlock tag after the window", Env.PlayerHas(MordecaiGameplayTags::State_PerfectBlock));

	if (!TestTrue("the swing lands", Env.LandSwing(Enemy)))
	{
		return true;
	}
	const float PostureMultiplier = GetDefault<UMordecaiPostureSystem>()->BlockPostureDamageMultiplier;
	TestEqual("health x (1 - Mitigation), once", Env.GetPlayer(UMordecaiAttributeSet::GetHealthAttribute()), 100.f - 15.f * (1.f - Block->Mitigation));
	TestEqual("stamina x (1 - Stability)", Env.GetPlayer(UMordecaiAttributeSet::GetStaminaAttribute()), 100.f - 15.f * (1.f - Block->Stability));
	TestEqual("posture x BlockPostureDamageMultiplier", Env.GetPlayer(UMordecaiAttributeSet::GetPostureAttribute()), 100.f - 4.5f * PostureMultiplier);
	TestEqual("with the defaults: 7.5 health", Env.GetPlayer(UMordecaiAttributeSet::GetHealthAttribute()), 92.5f);
	TestEqual("10.5 stamina", Env.GetPlayer(UMordecaiAttributeSet::GetStaminaAttribute()), 89.5f);
	TestEqual("2.25 posture", Env.GetPlayer(UMordecaiAttributeSet::GetPostureAttribute()), 97.75f);
	TestEqual("one damage number", Env.PopCount(), 1);
	TestEqual("it shows the mitigated 7.5, rounded", Env.LastPopAmount(), FMath::RoundToInt32(7.5f));
	TestTrue("the guard holds", Block->GetCurrentBlockPhase() == EMordecaiBlockPhase::Active && Env.PlayerHas(MordecaiGameplayTags::State_Blocking));
	return true;
}

// ---------------------------------------------------------------------------
// 2b. Mordecai.Recovery.Defense.TestAttackerSwingIsBlocked (AC-101.2)
// The CombatTestMap TestAttacker's own DA_EnemyAttack_BasicSlash on a held block: 7.5 health, 10.5 stamina, 2.25 posture.
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_Recovery_Defense_TestAttackerSwingIsBlocked,
	"Mordecai.Recovery.Defense.TestAttackerSwingIsBlocked",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_Recovery_Defense_TestAttackerSwingIsBlocked::RunTest(const FString& Parameters)
{
	using namespace MordecaiRecoveryDefenseTest;

	FDefenseTestEnv Env;
	ON_SCOPE_EXIT { Env.TeardownDefender(); };
	if (!TestTrue("defending player set up", Env.SetupDefender(FVector(100.0, 0.0, 0.0))))
	{
		return true;
	}
	AMordecaiEnemyCharacter* Enemy = Env.SpawnTestAttacker();
	if (!TestNotNull("TestAttacker spawned", Enemy) || !TestTrue("with its slash", Enemy->EnemyAttackProfiles.Num() > 0 && Enemy->EnemyAttackProfiles[0]))
	{
		return true;
	}
	const UMordecaiAttackProfileDataAsset* Slash = Enemy->EnemyAttackProfiles[0];
	TestEqual("BasicSlash deals 15", Slash->DamageProfile.BasePower, 15.f);

	Env.Press(MordecaiGameplayTags::Input_Block);
	Env.TickFrames(8);
	if (!TestTrue("the TestAttacker's swing lands", Env.LandSwing(Enemy)))
	{
		return true;
	}
	TestEqual("7.5 health", Env.GetPlayer(UMordecaiAttributeSet::GetHealthAttribute()), 92.5f);
	TestEqual("10.5 stamina", Env.GetPlayer(UMordecaiAttributeSet::GetStaminaAttribute()), 89.5f);
	TestEqual("2.25 posture", Env.GetPlayer(UMordecaiAttributeSet::GetPostureAttribute()), 100.f - Slash->DamageProfile.BasePower * Slash->PostureDamageScalar * 0.5f);
	return true;
}

// ---------------------------------------------------------------------------
// 3. Mordecai.Recovery.Defense.PerfectBlockSparesStaminaAndHurtsAttacker (AC-101.3: AC-006.9, AC-006.10)
// A swing landing 0.1 s after the guard goes up: health still mitigated, no stamina drain, PerfectBlockPostureDamage on
// the attacker. The PerfectBlock tag then ends with its window (a perfect block used to leave it on for good).
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_Recovery_Defense_PerfectBlock,
	"Mordecai.Recovery.Defense.PerfectBlockSparesStaminaAndHurtsAttacker",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_Recovery_Defense_PerfectBlock::RunTest(const FString& Parameters)
{
	using namespace MordecaiRecoveryDefenseTest;

	FDefenseTestEnv Env;
	ON_SCOPE_EXIT { Env.TeardownDefender(); };
	if (!TestTrue("defending player set up", Env.SetupDefender(FVector(100.0, 0.0, 0.0))))
	{
		return true;
	}
	AMordecaiEnemyCharacter* Enemy = Env.SpawnAttacker(MakeSlashProfile());
	if (!TestNotNull("attacker spawned", Enemy))
	{
		return true;
	}
	const float AttackerPosture = GetAttribute(Enemy, UMordecaiAttributeSet::GetPostureAttribute());

	// The guard goes up as the swing starts; the 0.1 s windup ends well inside the 250 ms window
	Env.Press(MordecaiGameplayTags::Input_Block);
	const UMordecaiGA_Block* Block = Env.FindInstance<UMordecaiGA_Block>();
	if (!TestNotNull("block instance", Block) || !TestTrue("the swing lands", Env.LandSwing(Enemy)))
	{
		return true;
	}
	TestTrue("inside the perfect window", Block->IsPerfectBlockWindowActive());
	TestEqual("health still mitigated", Env.GetPlayer(UMordecaiAttributeSet::GetHealthAttribute()), 92.5f);
	TestEqual("no stamina drain", Env.GetPlayer(UMordecaiAttributeSet::GetStaminaAttribute()), 100.f);
	TestEqual("the attacker takes PerfectBlockPostureDamage", GetAttribute(Enemy, UMordecaiAttributeSet::GetPostureAttribute()), AttackerPosture - Block->PerfectBlockPostureDamage);
	TestTrue("PerfectBlock is on", Env.PlayerHas(MordecaiGameplayTags::State_PerfectBlock));

	Env.TickFrames(8);
	TestFalse("the perfect window closed", Block->IsPerfectBlockWindowActive());
	TestFalse("PerfectBlock ends with its window", Env.PlayerHas(MordecaiGameplayTags::State_PerfectBlock));
	TestTrue("the guard is still up", Env.PlayerHas(MordecaiGameplayTags::State_Blocking));
	return true;
}

// ---------------------------------------------------------------------------
// 4. Mordecai.Recovery.Defense.GuardBrokenTakesFullHits (AC-101.4: AC-006.5)
// A blocked hit that empties stamina breaks the guard; the next swing, during the guard break, lands in full.
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_Recovery_Defense_GuardBrokenTakesFullHits,
	"Mordecai.Recovery.Defense.GuardBrokenTakesFullHits",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_Recovery_Defense_GuardBrokenTakesFullHits::RunTest(const FString& Parameters)
{
	using namespace MordecaiRecoveryDefenseTest;

	FDefenseTestEnv Env;
	ON_SCOPE_EXIT { Env.TeardownDefender(); };
	if (!TestTrue("defending player set up", Env.SetupDefender(FVector(100.0, 0.0, 0.0))))
	{
		return true;
	}
	AMordecaiEnemyCharacter* Enemy = Env.SpawnAttacker(MakeSlashProfile());
	if (!TestNotNull("attacker spawned", Enemy))
	{
		return true;
	}
	Env.GetPlayerASC()->SetNumericAttributeBase(UMordecaiAttributeSet::GetStaminaAttribute(), 5.f);

	Env.Press(MordecaiGameplayTags::Input_Block);
	const UMordecaiGA_Block* Block = Env.FindInstance<UMordecaiGA_Block>();
	Env.TickFrames(8);
	if (!TestNotNull("block instance", Block) || !TestTrue("the first swing lands", Env.LandSwing(Enemy)))
	{
		return true;
	}
	TestEqual("the breaking hit is still mitigated", Env.GetPlayer(UMordecaiAttributeSet::GetHealthAttribute()), 92.5f);
	TestEqual("stamina 5 - 10.5", Env.GetPlayer(UMordecaiAttributeSet::GetStaminaAttribute()), -5.5f);
	TestTrue("the guard breaks", Block->GetCurrentBlockPhase() == EMordecaiBlockPhase::GuardBroken);
	TestTrue("GuardBroken", Env.PlayerHas(MordecaiGameplayTags::State_GuardBroken));

	Env.FinishSwing(Enemy);
	if (!TestTrue("the second swing lands", Env.LandSwing(Enemy)))
	{
		return true;
	}
	TestTrue("still guard-broken at the second hit", Env.PlayerHas(MordecaiGameplayTags::State_GuardBroken));
	TestEqual("the second hit lands in full", Env.GetPlayer(UMordecaiAttributeSet::GetHealthAttribute()), 77.5f);
	TestEqual("a broken guard drains no stamina", Env.GetPlayer(UMordecaiAttributeSet::GetStaminaAttribute()), -5.5f);
	return true;
}

// ---------------------------------------------------------------------------
// 5. Mordecai.Recovery.Defense.ParryNegatesEnemyHit (AC-101.5: AC-006.13)
// A swing landing in the parry window does nothing to the player; the attacker is parried and loses posture, the
// player gets Riposte, and the parry ends without a whiff.
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_Recovery_Defense_ParryNegatesEnemyHit,
	"Mordecai.Recovery.Defense.ParryNegatesEnemyHit",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_Recovery_Defense_ParryNegatesEnemyHit::RunTest(const FString& Parameters)
{
	using namespace MordecaiRecoveryDefenseTest;

	FDefenseTestEnv Env;
	ON_SCOPE_EXIT { Env.TeardownDefender(); };
	if (!TestTrue("defending player set up", Env.SetupDefender(FVector(100.0, 0.0, 0.0))))
	{
		return true;
	}
	AMordecaiEnemyCharacter* Enemy = Env.SpawnAttacker(MakeWeakeningSlashProfile());
	if (!TestNotNull("attacker spawned", Enemy))
	{
		return true;
	}
	const float AttackerPosture = GetAttribute(Enemy, UMordecaiAttributeSet::GetPostureAttribute());

	Env.Press(MordecaiGameplayTags::Input_Parry);
	const UMordecaiGA_Parry* Parry = Env.FindInstance<UMordecaiGA_Parry>();
	if (!TestNotNull("parry instance", Parry) || !TestTrue("parrying", Env.PlayerHas(MordecaiGameplayTags::State_Parrying)))
	{
		return true;
	}
	const float StaminaAfterCost = Env.GetPlayer(UMordecaiAttributeSet::GetStaminaAttribute());
	TestEqual("the parry costs its stamina up front", StaminaAfterCost, 100.f - Parry->ParryStaminaCost);

	TestFalse("the parried swing is cancelled before it remains Active", Env.LandSwing(Enemy));
	TestEqual("no health lost", Env.GetPlayer(UMordecaiAttributeSet::GetHealthAttribute()), 100.f);
	TestEqual("no posture lost", Env.GetPlayer(UMordecaiAttributeSet::GetPostureAttribute()), 100.f);
	TestEqual("no further stamina", Env.GetPlayer(UMordecaiAttributeSet::GetStaminaAttribute()), StaminaAfterCost);
	TestFalse("no on-hit status", Env.PlayerHas(MordecaiGameplayTags::Status_Weakened));
	TestEqual("no damage number", Env.PopCount(), 0);
	TestTrue("the player gets Riposte", Env.PlayerHas(MordecaiGameplayTags::State_Riposte));
	TestFalse("no whiff", Env.PlayerHas(MordecaiGameplayTags::State_ParryWhiff));
	TestFalse("the parry resolved and ended", Env.IsActive(UMordecaiGA_Parry::StaticClass()));
	TestTrue("the attacker is parried", Enemy->GetAbilitySystemComponent()->HasMatchingGameplayTag(MordecaiGameplayTags::State_Parried));
	TestEqual("the attacker takes ParryPostureDamage", GetAttribute(Enemy, UMordecaiAttributeSet::GetPostureAttribute()), AttackerPosture - Parry->ParryPostureDamage);
	return true;
}

// ---------------------------------------------------------------------------
// 6. Mordecai.Recovery.Defense.DodgeIFramesNegateEnemyHit (AC-101.6: AC-005.3)
// A swing landing while Dodging is on does nothing: no health, posture, on-hit status or damage number. The first
// swing after the dodge lands in full.
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_Recovery_Defense_DodgeIFramesNegateEnemyHit,
	"Mordecai.Recovery.Defense.DodgeIFramesNegateEnemyHit",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_Recovery_Defense_DodgeIFramesNegateEnemyHit::RunTest(const FString& Parameters)
{
	using namespace MordecaiRecoveryDefenseTest;

	FDefenseTestEnv Env;
	ON_SCOPE_EXIT { Env.TeardownDefender(); };
	if (!TestTrue("defending player set up", Env.SetupDefender(FVector(100.0, 0.0, 0.0))))
	{
		return true;
	}
	AMordecaiEnemyCharacter* Enemy = Env.SpawnAttacker(MakeWeakeningSlashProfile());
	if (!TestNotNull("attacker spawned", Enemy))
	{
		return true;
	}

	Env.Press(MordecaiGameplayTags::Input_Dodge);
	if (!TestTrue("dodging", Env.PlayerHas(MordecaiGameplayTags::State_Dodging)))
	{
		return true;
	}
	const float StaminaAfterCost = Env.GetPlayer(UMordecaiAttributeSet::GetStaminaAttribute());

	if (!TestTrue("the swing lands during the dodge", Env.LandSwing(Enemy)))
	{
		return true;
	}
	TestTrue("still inside the i-frames at the hit", Env.PlayerHas(MordecaiGameplayTags::State_Dodging));
	TestEqual("no health lost", Env.GetPlayer(UMordecaiAttributeSet::GetHealthAttribute()), 100.f);
	TestEqual("no posture lost", Env.GetPlayer(UMordecaiAttributeSet::GetPostureAttribute()), 100.f);
	TestEqual("a real perfect evasion refunds 10 stamina once",
		Env.GetPlayer(UMordecaiAttributeSet::GetStaminaAttribute()),
		StaminaAfterCost + 10.f);
	TestTrue("perfect evasion exposes the success feedback tag",
		Env.PlayerHas(MordecaiGameplayTags::State_PerfectDodge));
	TestFalse("no on-hit status", Env.PlayerHas(MordecaiGameplayTags::Status_Weakened));
	TestEqual("no damage number", Env.PopCount(), 0);

	// After the dodge's 400 ms active phase the i-frames are over
	Env.FinishSwing(Enemy);
	Env.TickFrames(10);
	if (!TestFalse("the dodge is over", Env.PlayerHas(MordecaiGameplayTags::State_Dodging)) || !TestTrue("the next swing lands", Env.LandSwing(Enemy)))
	{
		return true;
	}
	TestEqual("the next hit lands in full", Env.GetPlayer(UMordecaiAttributeSet::GetHealthAttribute()), 85.f);
	TestTrue("with its on-hit status", Env.PlayerHas(MordecaiGameplayTags::Status_Weakened));
	TestEqual("and its damage number", Env.PopCount(), 1);
	return true;
}

// ---------------------------------------------------------------------------
// 7. Mordecai.Recovery.Defense.BlinkInvulnerabilityNegatesEnemyHit (AC-101.7: AC-021.3)
// A swing landing during Blink's invulnerability does nothing; once its 0.3 s are over (after the spell itself ended at
// 0.2 s) the next swing lands.
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_Recovery_Defense_BlinkInvulnerabilityNegatesEnemyHit,
	"Mordecai.Recovery.Defense.BlinkInvulnerabilityNegatesEnemyHit",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_Recovery_Defense_BlinkInvulnerabilityNegatesEnemyHit::RunTest(const FString& Parameters)
{
	using namespace MordecaiRecoveryDefenseTest;

	FDefenseTestEnv Env;
	ON_SCOPE_EXIT { Env.TeardownDefender(); };
	if (!TestTrue("defending player set up", Env.SetupDefender(FVector(100.0, 0.0, 0.0))))
	{
		return true;
	}
	AMordecaiEnemyCharacter* Enemy = Env.SpawnAttacker(MakeWeakeningSlashProfile());
	UAbilitySystemComponent* ASC = Env.GetPlayerASC();
	if (!TestNotNull("attacker spawned", Enemy))
	{
		return true;
	}

	const FGameplayAbilitySpecHandle Blink = ASC->GiveAbility(FGameplayAbilitySpec(UMordecaiGA_Blink::StaticClass(), 1, INDEX_NONE, MakeBlinkData()));
	if (!TestTrue("Blink casts", ASC->TryActivateAbility(Blink)) || !TestTrue("invulnerable", Env.PlayerHas(MordecaiGameplayTags::State_Invulnerable)))
	{
		return true;
	}
	Env.PlaceAttackerAtDefenseAngle(Enemy, 180.f);
	if (!TestTrue("the swing lands during the invulnerability", Env.LandSwing(Enemy)))
	{
		return true;
	}
	TestTrue("still invulnerable at the hit", Env.PlayerHas(MordecaiGameplayTags::State_Invulnerable));
	TestEqual("no health lost", Env.GetPlayer(UMordecaiAttributeSet::GetHealthAttribute()), 100.f);
	TestEqual("no posture lost", Env.GetPlayer(UMordecaiAttributeSet::GetPostureAttribute()), 100.f);
	TestFalse("no on-hit status", Env.PlayerHas(MordecaiGameplayTags::Status_Weakened));
	TestEqual("no damage number", Env.PopCount(), 0);

	Env.FinishSwing(Enemy);
	Env.TickFrames(10);
	if (!TestFalse("the invulnerability is over", Env.PlayerHas(MordecaiGameplayTags::State_Invulnerable)) || !TestTrue("the next swing lands", Env.LandSwing(Enemy)))
	{
		return true;
	}
	TestEqual("the next hit lands in full", Env.GetPlayer(UMordecaiAttributeSet::GetHealthAttribute()), 85.f);
	return true;
}

// ---------------------------------------------------------------------------
// 7b. Mordecai.Recovery.Blink.InvulnerabilityExpires (AC-101.7; US-087 AC-087.7, N19)
// Blink's invulnerability (0.3 s) outlasts the spell (0.2 s recovery). Ending the ability cleared its timers, so the tag
// was never removed. It must be removed when its own duration ends, even though the ability ended first.
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_Recovery_Blink_InvulnerabilityExpires,
	"Mordecai.Recovery.Blink.InvulnerabilityExpires",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_Recovery_Blink_InvulnerabilityExpires::RunTest(const FString& Parameters)
{
	using namespace MordecaiRecoveryDefenseTest;

	FDefenseTestEnv Env;
	ON_SCOPE_EXIT { Env.TeardownDefender(); };
	if (!TestTrue("player set up", Env.SetupDefender(FVector::ZeroVector)))
	{
		return true;
	}
	UAbilitySystemComponent* ASC = Env.GetPlayerASC();
	const FGameplayAbilitySpecHandle Blink = ASC->GiveAbility(FGameplayAbilitySpec(UMordecaiGA_Blink::StaticClass(), 1, INDEX_NONE, MakeBlinkData()));
	const FGameplayAbilitySpec* Spec = ASC->FindAbilitySpecFromHandle(Blink);

	for (int32 Cast = 1; Cast <= 2; ++Cast)
	{
		if (!TestTrue(*FString::Printf(TEXT("cast %d: Blink casts"), Cast), ASC->TryActivateAbility(Blink)))
		{
			return true;
		}
		TestTrue(*FString::Printf(TEXT("cast %d: invulnerable"), Cast), Env.PlayerHas(MordecaiGameplayTags::State_Invulnerable));

		// The spell ends after its 0.2 s recovery (one frame activates the timers, four more run 0.2 s)
		for (int32 Frame = 0; Frame < 20 && Spec && Spec->IsActive(); ++Frame)
		{
			Env.TickFrames(1);
		}
		TestFalse(*FString::Printf(TEXT("cast %d: the spell ended"), Cast), Spec && Spec->IsActive());
		TestTrue(*FString::Printf(TEXT("cast %d: the invulnerability outlives the spell"), Cast), Env.PlayerHas(MordecaiGameplayTags::State_Invulnerable));

		Env.TickFrames(4);
		TestFalse(*FString::Printf(TEXT("cast %d: the invulnerability ends with its 0.3 s"), Cast), Env.PlayerHas(MordecaiGameplayTags::State_Invulnerable));
		Env.TickFrames(20);
		TestEqual(*FString::Printf(TEXT("cast %d: and stays gone"), Cast), ASC->GetTagCount(MordecaiGameplayTags::State_Invulnerable), 0);
	}
	return true;
}

// ---------------------------------------------------------------------------
// 8. Mordecai.Recovery.Defense.FallingGrantsAirborne (AC-101.8: AC-003.7)
// The player carries Mordecai.State.Airborne exactly while falling, on the PlayerState ASC: set on entering Falling,
// cleared on landing, cleared when the pawn's ASC is detached and seeded when it is attached mid-fall.
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_Recovery_Defense_FallingGrantsAirborne,
	"Mordecai.Recovery.Defense.FallingGrantsAirborne",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_Recovery_Defense_FallingGrantsAirborne::RunTest(const FString& Parameters)
{
	using namespace MordecaiRecoveryDefenseTest;

	MordecaiRecoveryTest::FRecoveryTestEnv Env;
	ON_SCOPE_EXIT { Env.Teardown(); };
	if (!TestTrue("player set up", Env.Setup(FVector::ZeroVector)))
	{
		return true;
	}
	UAbilitySystemComponent* ASC = Env.GetPlayerASC();
	ULyraPawnExtensionComponent* PawnExt = ULyraPawnExtensionComponent::FindPawnExtensionComponent(Env.Player);
	if (!TestNotNull("player ASC", ASC) || !TestNotNull("pawn extension", PawnExt))
	{
		return true;
	}

	SetFalling(Env.Player, false);
	TestFalse("walking: not airborne", ASC->HasMatchingGameplayTag(MordecaiGameplayTags::State_Airborne));
	SetFalling(Env.Player, true);
	TestEqual("falling: airborne, once", ASC->GetTagCount(MordecaiGameplayTags::State_Airborne), 1);
	SetFalling(Env.Player, false);
	TestFalse("landed: not airborne", ASC->HasMatchingGameplayTag(MordecaiGameplayTags::State_Airborne));

	// Detached mid-fall: the tag does not linger on the PlayerState's ASC
	SetFalling(Env.Player, true);
	PawnExt->UninitializeAbilitySystem();
	TestFalse("detached: the PlayerState ASC is not airborne", ASC->HasMatchingGameplayTag(MordecaiGameplayTags::State_Airborne));

	// Attached mid-fall: seeded from the current movement mode
	PawnExt->InitializeAbilitySystem(Env.PlayerState->GetLyraAbilitySystemComponent(), Env.PlayerState);
	TestEqual("attached while falling: airborne", ASC->GetTagCount(MordecaiGameplayTags::State_Airborne), 1);
	SetFalling(Env.Player, false);
	TestFalse("landed again: not airborne", ASC->HasMatchingGameplayTag(MordecaiGameplayTags::State_Airborne));
	return true;
}

// ---------------------------------------------------------------------------
// 8b. Mordecai.Recovery.Defense.EnemyFallingGrantsAirborne (AC-101.8: AC-003.7)
// An enemy carries Airborne while falling too, so the player's jump-avoidable sweeps pass under a falling enemy.
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_Recovery_Defense_EnemyFallingGrantsAirborne,
	"Mordecai.Recovery.Defense.EnemyFallingGrantsAirborne",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_Recovery_Defense_EnemyFallingGrantsAirborne::RunTest(const FString& Parameters)
{
	using namespace MordecaiRecoveryDefenseTest;

	MordecaiRecoveryTest::FRecoveryTestEnv Env;
	ON_SCOPE_EXIT { Env.Teardown(); };
	if (!TestTrue("player set up", Env.Setup(FVector::ZeroVector)))
	{
		return true;
	}
	UMordecaiAttackProfileDataAsset* Slash = MakeSlashProfile();
	AMordecaiEnemyCharacter* Enemy = Env.SpawnEnemy(FVector(100.0, 0.0, 0.0), { Slash });
	UMordecaiHitDetectionSubsystem* HitDetection = Env.World->GetSubsystem<UMordecaiHitDetectionSubsystem>();
	if (!TestNotNull("enemy spawned", Enemy) || !TestNotNull("hit detection", HitDetection))
	{
		return true;
	}
	UAbilitySystemComponent* EnemyASC = Enemy->GetAbilitySystemComponent();
	auto PlayerSweepHitsEnemy = [&]()
	{
		for (const FMordecaiHitResult& Hit : HitDetection->PerformMeleeHitDetection(
			Slash, Env.Player->GetActorLocation(), Env.Player->GetActorForwardVector(), Env.Player))
		{
			if (Hit.HitActor.Get() == Enemy)
			{
				return true;
			}
		}
		return false;
	};

	SetFalling(Enemy, false);
	TestFalse("walking: not airborne", EnemyASC->HasMatchingGameplayTag(MordecaiGameplayTags::State_Airborne));
	TestTrue("the player's sweep hits the grounded enemy", PlayerSweepHitsEnemy());
	SetFalling(Enemy, true);
	TestEqual("falling: airborne, once", EnemyASC->GetTagCount(MordecaiGameplayTags::State_Airborne), 1);
	TestFalse("the player's jump-avoidable sweep passes under the falling enemy", PlayerSweepHitsEnemy());
	SetFalling(Enemy, false);
	TestFalse("landed: not airborne", EnemyASC->HasMatchingGameplayTag(MordecaiGameplayTags::State_Airborne));
	return true;
}

// ---------------------------------------------------------------------------
// 9. Mordecai.Recovery.Defense.JumpAvoidsBasicSlash (AC-101.9: AC-003.7, AC-003.8)
// The TestAttacker's DA_EnemyAttack_BasicSlash is jump-avoidable: it misses a falling player and hits it on the ground.
// A swing that hits airborne targets still hits it in the air.
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_Recovery_Defense_JumpAvoidsBasicSlash,
	"Mordecai.Recovery.Defense.JumpAvoidsBasicSlash",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_Recovery_Defense_JumpAvoidsBasicSlash::RunTest(const FString& Parameters)
{
	using namespace MordecaiRecoveryDefenseTest;

	FDefenseTestEnv Env;
	ON_SCOPE_EXIT { Env.TeardownDefender(); };
	if (!TestTrue("player set up", Env.SetupDefender(FVector(100.0, 0.0, 0.0))))
	{
		return true;
	}
	AMordecaiEnemyCharacter* Enemy = Env.SpawnTestAttacker();
	UMordecaiGA_MeleeAttack* Melee = GetMelee(Enemy);
	if (!TestNotNull("TestAttacker spawned", Enemy) || !TestNotNull("its melee attack", Melee)
		|| !TestTrue("with its slash", Enemy->EnemyAttackProfiles.Num() > 0 && Enemy->EnemyAttackProfiles[0]))
	{
		return true;
	}
	const UMordecaiAttackProfileDataAsset* Slash = Enemy->EnemyAttackProfiles[0];
	TestTrue("BasicSlash is jump-avoidable", Slash->JumpAvoidable);
	TestFalse("BasicSlash does not hit airborne targets", Slash->HitsAirborne);
	const float Damage = Slash->DamageProfile.BasePower;

	SetFalling(Env.Player, true);
	TestTrue("the falling player is airborne", Env.PlayerHas(MordecaiGameplayTags::State_Airborne));
	if (!TestTrue("the swing comes", Env.LandSwing(Enemy)))
	{
		return true;
	}
	TestEqual("a falling player is not hit", Env.GetPlayer(UMordecaiAttributeSet::GetHealthAttribute()), 100.f);
	Env.FinishSwing(Enemy);

	SetFalling(Env.Player, false);
	if (!TestTrue("the next swing comes", Env.LandSwing(Enemy)))
	{
		return true;
	}
	TestEqual("the same player on the ground is hit", Env.GetPlayer(UMordecaiAttributeSet::GetHealthAttribute()), 100.f - Damage);
	Env.FinishSwing(Enemy);

	// A slam that hits airborne targets (AC-003.8), on a copy: never edit the loaded content asset
	UMordecaiAttackProfileDataAsset* Slam = DuplicateObject<UMordecaiAttackProfileDataAsset>(Slash, GetTransientPackage());
	Slam->HitsAirborne = true;
	Melee->AttackProfiles = { Slam };
	SetFalling(Env.Player, true);
	if (!TestTrue("the slam comes", Env.LandSwing(Enemy)))
	{
		return true;
	}
	TestEqual("a HitsAirborne swing hits the falling player", Env.GetPlayer(UMordecaiAttributeSet::GetHealthAttribute()), 100.f - 2.f * Damage);
	return true;
}

// ---------------------------------------------------------------------------
// Issue #71 / US-121 — a guard and parry cover a configurable frontal arc centred on actor forward.
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_DefenseArc_FrontIsBlocked,
	"Mordecai.DefenseArc.FrontIsBlocked",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_DefenseArc_FrontIsBlocked::RunTest(const FString& Parameters)
{
	using namespace MordecaiDefenseArcTest;
	FDefenseTestEnv Env;
	ON_SCOPE_EXIT { Env.TeardownDefender(); };
	if (!TestTrue("defender set up", Env.SetupDefender(FVector::ZeroVector)))
	{
		return true;
	}
	const FHitCosts Costs = LandBlockedSwingAtAngle(Env, 0.f);
	TestEqual("front hit is mitigated", Costs.Health, 7.5f);
	TestEqual("front hit drains block stamina", Costs.Stamina, 10.5f);
	TestEqual("front hit uses blocked posture", Costs.Posture, 2.25f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_DefenseArc_FiftyNineDegreesIsBlocked,
	"Mordecai.DefenseArc.FiftyNineDegreesIsBlocked",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_DefenseArc_FiftyNineDegreesIsBlocked::RunTest(const FString& Parameters)
{
	using namespace MordecaiDefenseArcTest;
	FDefenseTestEnv Env;
	ON_SCOPE_EXIT { Env.TeardownDefender(); };
	if (!TestTrue("defender set up", Env.SetupDefender(FVector::ZeroVector)))
	{
		return true;
	}
	const FHitCosts Costs = LandBlockedSwingAtAngle(Env, 59.f);
	TestEqual("59 degree hit is mitigated", Costs.Health, 7.5f);
	TestEqual("59 degree hit drains block stamina", Costs.Stamina, 10.5f);
	TestEqual("59 degree hit uses blocked posture", Costs.Posture, 2.25f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_DefenseArc_SixtyOneDegreesIsNotBlocked,
	"Mordecai.DefenseArc.SixtyOneDegreesIsNotBlocked",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_DefenseArc_SixtyOneDegreesIsNotBlocked::RunTest(const FString& Parameters)
{
	using namespace MordecaiDefenseArcTest;
	FDefenseTestEnv Env;
	ON_SCOPE_EXIT { Env.TeardownDefender(); };
	if (!TestTrue("defender set up", Env.SetupDefender(FVector::ZeroVector)))
	{
		return true;
	}
	const FHitCosts Costs = LandBlockedSwingAtAngle(Env, 61.f);
	TestEqual("61 degree hit lands in full", Costs.Health, 15.f);
	TestEqual("61 degree hit has no block stamina cost", Costs.Stamina, 0.f);
	TestEqual("61 degree hit uses unblocked posture", Costs.Posture, 4.5f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_DefenseArc_BehindIsNotBlocked,
	"Mordecai.DefenseArc.BehindIsNotBlocked",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_DefenseArc_BehindIsNotBlocked::RunTest(const FString& Parameters)
{
	using namespace MordecaiDefenseArcTest;
	FDefenseTestEnv Env;
	ON_SCOPE_EXIT { Env.TeardownDefender(); };
	if (!TestTrue("defender set up", Env.SetupDefender(FVector::ZeroVector)))
	{
		return true;
	}
	const FHitCosts Costs = LandBlockedSwingAtAngle(Env, 180.f);
	TestEqual("rear hit lands in full", Costs.Health, 15.f);
	TestEqual("rear hit has no block stamina cost", Costs.Stamina, 0.f);
	TestEqual("rear hit uses unblocked posture", Costs.Posture, 4.5f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_DefenseArc_EquippedShieldWidensArc,
	"Mordecai.DefenseArc.EquippedShieldWidensArc",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_DefenseArc_EquippedShieldWidensArc::RunTest(const FString& Parameters)
{
	using namespace MordecaiDefenseArcTest;

	FDefenseTestEnv DefaultEnv;
	ON_SCOPE_EXIT { DefaultEnv.TeardownDefender(); };
	if (!TestTrue("default defender set up", DefaultEnv.SetupDefender(FVector::ZeroVector)))
	{
		return true;
	}
	const FHitCosts DefaultCosts = LandBlockedSwingAtAngle(DefaultEnv, 70.f);
	TestEqual("70 degrees is outside the default 120 degree arc", DefaultCosts.Health, 15.f);
	TestEqual("default guard spends no stamina out of arc", DefaultCosts.Stamina, 0.f);

	FDefenseTestEnv ShieldEnv;
	ON_SCOPE_EXIT { ShieldEnv.TeardownDefender(); };
	if (!TestTrue("shield defender set up", ShieldEnv.SetupDefender(FVector::ZeroVector)))
	{
		return true;
	}
	UMordecaiShieldDataAsset* Buckler = NewObject<UMordecaiShieldDataAsset>();
	Buckler->BlockArcDegrees = 150.f;
	FMordecaiWeaponInstance ShieldInstance;
	ShieldInstance.InstanceId = FGuid::NewGuid();
	ShieldInstance.WeaponDataAsset = Buckler;
	UMordecaiEquipmentComponent* Equipment = ShieldEnv.Player->GetEquipmentComponent();
	if (!TestNotNull("equipment component", Equipment)
		|| !TestTrue("150 degree shield equips off hand", Equipment->EquipWeapon(ShieldInstance, EMordecaiEquipSlot::OffHand)))
	{
		return true;
	}
	const FHitCosts ShieldCosts = LandBlockedSwingAtAngle(ShieldEnv, 70.f);
	TestEqual("70 degrees is inside the configured 150 degree shield arc", ShieldCosts.Health, 7.5f);
	TestEqual("shield block drains stamina", ShieldCosts.Stamina, 10.5f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_DefenseArc_ProjectileFromBehindIsNotBlocked,
	"Mordecai.DefenseArc.ProjectileFromBehindIsNotBlocked",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_DefenseArc_ProjectileFromBehindIsNotBlocked::RunTest(const FString& Parameters)
{
	using namespace MordecaiRecoveryDefenseTest;
	FDefenseTestEnv Env;
	ON_SCOPE_EXIT { Env.TeardownDefender(); };
	if (!TestTrue("defender set up", Env.SetupDefender(FVector::ZeroVector)))
	{
		return true;
	}
	Env.Press(MordecaiGameplayTags::Input_Block);
	Env.TickFrames(8);
	const FVector DefenderLocation = Env.Player->GetActorLocation();
	const FMordecaiIncomingHitSource ProjectileSource = FMordecaiIncomingHitSource::FromProjectile(
		DefenderLocation - FVector(1000.f, 0.f, 0.f),
		FVector::ForwardVector);
	const FMordecaiHitDefenseResult Result = FMordecaiHitDefense::ResolveIncomingHit(
		Env.GetPlayerASC(), nullptr, 15.f, Env.World->GetTimeSeconds(), ProjectileSource);
	TestTrue("projectile source is behind the +X-facing defender", Result.Defense == EMordecaiHitDefense::None);
	TestEqual("rear projectile consumes no block stamina",
		Env.GetPlayer(UMordecaiAttributeSet::GetStaminaAttribute()), 100.f);
	TestEqual("rear projectile is not pre-applied as blocked health damage",
		Env.GetPlayer(UMordecaiAttributeSet::GetHealthAttribute()), 100.f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_DefenseArc_ParryBehindDoesNotSucceed,
	"Mordecai.DefenseArc.ParryBehindDoesNotSucceed",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_DefenseArc_ParryBehindDoesNotSucceed::RunTest(const FString& Parameters)
{
	using namespace MordecaiRecoveryDefenseTest;
	FDefenseTestEnv Env;
	ON_SCOPE_EXIT { Env.TeardownDefender(); };
	if (!TestTrue("defender set up", Env.SetupDefender(FVector::ZeroVector)))
	{
		return true;
	}
	AMordecaiEnemyCharacter* Enemy = Env.SpawnAttacker(MakeSlashProfile());
	if (!TestNotNull("attacker spawned", Enemy))
	{
		return true;
	}
	Env.PlaceAttackerAtDefenseAngle(Enemy, 180.f);
	Env.Press(MordecaiGameplayTags::Input_Parry);
	UMordecaiGA_Parry* Parry = Env.FindInstance<UMordecaiGA_Parry>();
	if (!TestNotNull("parry instance", Parry) || !TestTrue("rear swing lands", Env.LandSwing(Enemy)))
	{
		return true;
	}
	TestEqual("rear hit lands in full through parry", Env.GetPlayer(UMordecaiAttributeSet::GetHealthAttribute()), 85.f);
	TestEqual("rear hit uses normal posture", Env.GetPlayer(UMordecaiAttributeSet::GetPostureAttribute()), 95.5f);
	TestFalse("rear attacker is not parried",
		Enemy->GetAbilitySystemComponent()->HasMatchingGameplayTag(MordecaiGameplayTags::State_Parried));
	TestFalse("rear hit grants no riposte", Env.PlayerHas(MordecaiGameplayTags::State_Riposte));
	TestTrue("out-of-arc hit does not consume the parry window",
		Parry->GetCurrentParryPhase() == EMordecaiParryPhase::Active);
	return true;
}
