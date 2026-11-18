// Project Mordecai — Stamina Runtime (US-122 / issue #143)
// These tests drive the real PlayerState ASC, granted combat abilities, character sprint toggle,
// and world/component ticks. They intentionally do not retest UMordecaiStaminaSystem's pure maths.

#include "Misc/AutomationTest.h"
#include "Misc/ScopeExit.h"

#include "AbilitySystem/LyraAbilitySystemComponent.h"
#include "Character/LyraPawnExtensionComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameplayAbilitySpec.h"
#include "Mordecai/AbilitySystem/MordecaiAttributeSet.h"
#include "Mordecai/Combat/MordecaiGA_Block.h"
#include "Mordecai/Combat/MordecaiGA_Dodge.h"
#include "Mordecai/Combat/MordecaiGA_MeleeAttack.h"
#include "Mordecai/Combat/MordecaiStaminaSystem.h"
#include "Mordecai/MordecaiCharacter.h"
#include "Mordecai/MordecaiGameplayTags.h"
#include "Mordecai/MordecaiPlayerController.h"
#include "Mordecai/UI/MordecaiStaminaBarWidget.h"
#include "Tests/Recovery/MordecaiRecoveryTestEnv.h"

namespace MordecaiStaminaRuntimeTest
{
	static constexpr float FrameSeconds = 0.05f;

	struct FRuntimeEnv : MordecaiRecoveryTest::FRecoveryTestEnv
	{
		AMordecaiPlayerController* GrantPC = nullptr;
		UMordecaiAttackProfileDataAsset* AttackProfile = nullptr;

		bool SetupRuntime()
		{
			if (!Setup(FVector::ZeroVector))
			{
				return false;
			}

			// FRecoveryTestEnv intentionally never begins its synthetic world. Begin
			// the PlayerState so its real stamina component follows the production
			// BeginPlay path; the pawn extension remains manually initialized.
			if (!PlayerState->HasActorBegunPlay())
			{
				PlayerState->DispatchBeginPlay();
			}
			World->TimeSeconds = 10.f;
			if (UCharacterMovementComponent* Movement = Player->GetCharacterMovement())
			{
				Movement->SetMovementMode(MOVE_Walking);
			}

			AttackProfile = MordecaiRecoveryTest::MakeStubProfile();
			AttackProfile->DamageProfile.BasePower = 20.f;
			AttackProfile->StaminaCost = 15.f;
			AttackProfile->WindupTimeMs = 100.f;
			AttackProfile->ActiveTimeMs = 100.f;
			AttackProfile->RecoveryTimeMs = 100.f;

			GrantPC = NewObject<AMordecaiPlayerController>();
			GrantPC->PlayerState = PlayerState;
			GrantPC->DefaultSpellAbilities.Reset();
			GrantPC->MeleeAttackProfiles = { AttackProfile };
			GrantPC->GrantCombatAbilities();

			TickFrames(1);
			return GetASC() != nullptr;
		}

		ULyraAbilitySystemComponent* GetASC() const
		{
			return Cast<ULyraAbilitySystemComponent>(GetPlayerASC());
		}

		void Press(const FGameplayTag& InputTag) const
		{
			ULyraAbilitySystemComponent* ASC = GetASC();
			ASC->AbilityInputTagPressed(InputTag);
			ASC->ProcessAbilityInput(0.f, false);
		}

		void Release(const FGameplayTag& InputTag) const
		{
			ULyraAbilitySystemComponent* ASC = GetASC();
			ASC->AbilityInputTagReleased(InputTag);
			ASC->ProcessAbilityInput(0.f, false);
		}

		void TickFrames(int32 Frames) const
		{
			for (int32 Frame = 0; Frame < Frames; ++Frame)
			{
				++GFrameCounter;
				World->Tick(LEVELTICK_All, FrameSeconds);
			}
		}

		void TickSeconds(float Seconds) const
		{
			TickFrames(FMath::CeilToInt(Seconds / FrameSeconds));
		}

		void TickMovingSeconds(float Seconds) const
		{
			const int32 Frames = FMath::CeilToInt(Seconds / FrameSeconds);
			for (int32 Frame = 0; Frame < Frames; ++Frame)
			{
				Player->GetCharacterMovement()->Velocity =
					FVector(Player->WalkSpeed, 0.f, 0.f);
				TickFrames(1);
			}
		}

		void SetStamina(float Value) const
		{
			GetASC()->SetNumericAttributeBase(UMordecaiAttributeSet::GetStaminaAttribute(), Value);
		}

		float Stamina() const
		{
			return GetASC()->GetNumericAttribute(UMordecaiAttributeSet::GetStaminaAttribute());
		}

		template <typename AbilityT>
		AbilityT* Instance() const
		{
			FGameplayAbilitySpec* Spec = GetASC()->FindAbilitySpecFromClass(AbilityT::StaticClass());
			return Spec ? Cast<AbilityT>(Spec->GetPrimaryInstance()) : nullptr;
		}

		bool Active(TSubclassOf<UGameplayAbility> AbilityClass) const
		{
			const FGameplayAbilitySpec* Spec = GetASC()->FindAbilitySpecFromClass(AbilityClass);
			return Spec && Spec->IsActive();
		}

		void TeardownRuntime()
		{
			if (GrantPC)
			{
				GrantPC->PlayerState = nullptr;
			}
			GrantPC = nullptr;
			AttackProfile = nullptr;
			Teardown();
		}
	};

	int32 CountTierTags(const UAbilitySystemComponent* ASC)
	{
		return (ASC->HasMatchingGameplayTag(MordecaiGameplayTags::Stamina_Tier_Green) ? 1 : 0)
			+ (ASC->HasMatchingGameplayTag(MordecaiGameplayTags::Stamina_Tier_Yellow) ? 1 : 0)
			+ (ASC->HasMatchingGameplayTag(MordecaiGameplayTags::Stamina_Tier_Red) ? 1 : 0)
			+ (ASC->HasMatchingGameplayTag(MordecaiGameplayTags::Stamina_Tier_Exhausted) ? 1 : 0);
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_StaminaRuntime_RegenAfterSpendDelay,
	"Mordecai.Stamina.Runtime.RegenAfterSpendDelay",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_StaminaRuntime_RegenAfterSpendDelay::RunTest(const FString& Parameters)
{
	using namespace MordecaiStaminaRuntimeTest;
	FRuntimeEnv Env;
	ON_SCOPE_EXIT { Env.TeardownRuntime(); };
	if (!TestTrue("runtime world", Env.SetupRuntime()))
	{
		return true;
	}

	Env.Press(MordecaiGameplayTags::Input_LightAttack);
	const float Spent = Env.Stamina();
	TestEqual("real attack spends its profile cost", Spent, 85.f);
	Env.TickSeconds(1.25f);
	TestTrue("stamina regenerates after the spend delay", Env.Stamina() > Spent);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_StaminaRuntime_NoRegenDuringSpendDelay,
	"Mordecai.Stamina.Runtime.NoRegenDuringSpendDelay",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_StaminaRuntime_NoRegenDuringSpendDelay::RunTest(const FString& Parameters)
{
	using namespace MordecaiStaminaRuntimeTest;
	FRuntimeEnv Env;
	ON_SCOPE_EXIT { Env.TeardownRuntime(); };
	if (!TestTrue("runtime world", Env.SetupRuntime()))
	{
		return true;
	}

	Env.Press(MordecaiGameplayTags::Input_Dodge);
	const float Spent = Env.Stamina();
	TestEqual("real dodge spends stamina", Spent, 85.f);
	Env.TickSeconds(0.75f);
	TestEqual("no regeneration during the delay", Env.Stamina(), Spent);
	Env.TickSeconds(0.5f);
	TestTrue("the same runtime regenerates after the delay", Env.Stamina() > Spent);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_StaminaRuntime_SecondSpendRestartsDelay,
	"Mordecai.Stamina.Runtime.SecondSpendRestartsDelay",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_StaminaRuntime_SecondSpendRestartsDelay::RunTest(const FString& Parameters)
{
	using namespace MordecaiStaminaRuntimeTest;
	FRuntimeEnv Env;
	ON_SCOPE_EXIT { Env.TeardownRuntime(); };
	if (!TestTrue("runtime world", Env.SetupRuntime()))
	{
		return true;
	}

	Env.Press(MordecaiGameplayTags::Input_LightAttack);
	Env.TickSeconds(0.55f);
	Env.Press(MordecaiGameplayTags::Input_LightAttack);
	const float AfterSecondSpend = Env.Stamina();
	TestEqual("two real attacks spend twice", AfterSecondSpend, 70.f);
	Env.TickSeconds(0.75f);
	TestEqual("the second spend restarts the full delay", Env.Stamina(), AfterSecondSpend);
	Env.TickSeconds(0.5f);
	TestTrue("regeneration starts after the restarted delay", Env.Stamina() > AfterSecondSpend);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_StaminaRuntime_RegenUsesAttributeMultiplier,
	"Mordecai.Stamina.Runtime.RegenUsesAttributeMultiplier",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_StaminaRuntime_RegenUsesAttributeMultiplier::RunTest(const FString& Parameters)
{
	using namespace MordecaiStaminaRuntimeTest;
	FRuntimeEnv Env;
	ON_SCOPE_EXIT { Env.TeardownRuntime(); };
	if (!TestTrue("runtime world", Env.SetupRuntime()))
	{
		return true;
	}

	Env.GetASC()->SetNumericAttributeBase(
		UMordecaiAttributeSet::GetStaminaRegenMultiplierAttribute(), 2.f);
	Env.Press(MordecaiGameplayTags::Input_LightAttack);
	Env.TickSeconds(1.05f);
	const float BeforeTick = Env.Stamina();
	Env.TickFrames(1);
	const float Actual = Env.Stamina() - BeforeTick;
	const float Expected = GetDefault<UMordecaiStaminaSystem>()->ComputeRegenAmount(FrameSeconds) * 2.f;
	TestTrue("one authority runtime tick uses StaminaRegenMultiplier",
		FMath::IsNearlyEqual(Actual, Expected, 0.01f));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_StaminaRuntime_TierTagsTrackAttribute,
	"Mordecai.Stamina.Runtime.TierTagsTrackAttribute",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_StaminaRuntime_TierTagsTrackAttribute::RunTest(const FString& Parameters)
{
	using namespace MordecaiStaminaRuntimeTest;
	FRuntimeEnv Env;
	ON_SCOPE_EXIT { Env.TeardownRuntime(); };
	if (!TestTrue("runtime world", Env.SetupRuntime()))
	{
		return true;
	}
	UAbilitySystemComponent* ASC = Env.GetASC();

	struct FCase
	{
		float Stamina;
		FGameplayTag Expected;
	};
	const FCase Cases[] = {
		{ 100.f, MordecaiGameplayTags::Stamina_Tier_Green },
		{ 50.f, MordecaiGameplayTags::Stamina_Tier_Yellow },
		{ 20.f, MordecaiGameplayTags::Stamina_Tier_Red },
		{ 0.f, MordecaiGameplayTags::Stamina_Tier_Exhausted },
		{ 90.f, MordecaiGameplayTags::Stamina_Tier_Green }
	};
	for (const FCase& Case : Cases)
	{
		Env.SetStamina(Case.Stamina);
		TestTrue(*FString::Printf(TEXT("%.0f has expected tier"), Case.Stamina),
			ASC->HasMatchingGameplayTag(Case.Expected));
		TestEqual(*FString::Printf(TEXT("%.0f has one tier tag"), Case.Stamina), CountTierTags(ASC), 1);
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_StaminaRuntime_LowTierScalesMeleeAndDodge,
	"Mordecai.Stamina.Runtime.LowTierScalesMeleeAndDodge",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_StaminaRuntime_LowTierScalesMeleeAndDodge::RunTest(const FString& Parameters)
{
	using namespace MordecaiStaminaRuntimeTest;
	{
		FRuntimeEnv Env;
		ON_SCOPE_EXIT { Env.TeardownRuntime(); };
		if (!TestTrue("melee runtime world", Env.SetupRuntime()))
		{
			return true;
		}
		Env.SetStamina(20.f);
		Env.Press(MordecaiGameplayTags::Input_LightAttack);
		Env.TickFrames(1);
		const UMordecaiGA_MeleeAttack* Melee = Env.Instance<UMordecaiGA_MeleeAttack>();
		if (TestNotNull("real melee instance", Melee))
		{
			TestEqual("Red tier scales real melee damage by 0.75", Melee->ComputeHealthDamage(), -15.f);
		}
	}
	{
		FRuntimeEnv Env;
		ON_SCOPE_EXIT { Env.TeardownRuntime(); };
		if (!TestTrue("dodge runtime world", Env.SetupRuntime()))
		{
			return true;
		}
		Env.SetStamina(20.f);
		Env.Press(MordecaiGameplayTags::Input_Dodge);
		Env.TickFrames(1);
		const UMordecaiGA_Dodge* Dodge = Env.Instance<UMordecaiGA_Dodge>();
		if (TestNotNull("real dodge instance", Dodge))
		{
			const float BaseSpeed = Dodge->DodgeDistanceCm / Dodge->GetDodgeDurationSeconds();
			TestTrue("Red tier scales real dodge distance by 0.75",
				FMath::IsNearlyEqual(
					Dodge->ComputeImpulseVelocity(FVector::ForwardVector).Size(), BaseSpeed * 0.75f, 0.01f));
		}
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_StaminaRuntime_ActivationTierSnapshottedBeforeCost,
	"Mordecai.Stamina.Runtime.ActivationTierSnapshottedBeforeCost",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_StaminaRuntime_ActivationTierSnapshottedBeforeCost::RunTest(
	const FString& Parameters)
{
	using namespace MordecaiStaminaRuntimeTest;
	{
		FRuntimeEnv Env;
		ON_SCOPE_EXIT { Env.TeardownRuntime(); };
		if (!TestTrue("melee runtime world", Env.SetupRuntime()))
		{
			return true;
		}
		AMordecaiEnemyCharacter* Target =
			Env.SpawnEnemy(FVector(100.f, 0.f, 0.f), {});
		if (!TestNotNull("real melee target", Target))
		{
			return true;
		}
		UAbilitySystemComponent* TargetASC = Target->GetAbilitySystemComponent();
		const float HealthBefore = TargetASC->GetNumericAttribute(
			UMordecaiAttributeSet::GetHealthAttribute());

		// 40/100 is Yellow (1.0); this attack's 15 cost lands at 25/100 Red
		// before the real windup reaches its hit.
		Env.SetStamina(40.f);
		Env.Press(MordecaiGameplayTags::Input_LightAttack);
		TestEqual("the activation cost crosses Yellow to Red", Env.Stamina(), 25.f);
		TestTrue("post-cost authoritative tier is Red",
			Env.GetASC()->HasMatchingGameplayTag(MordecaiGameplayTags::Stamina_Tier_Red));
		for (int32 Frame = 0; Frame < 10
			&& FMath::IsNearlyEqual(
				TargetASC->GetNumericAttribute(UMordecaiAttributeSet::GetHealthAttribute()),
				HealthBefore);
			++Frame)
		{
			Env.TickFrames(1);
		}
		TestEqual(
			"the real hit keeps the Yellow activation snapshot",
			TargetASC->GetNumericAttribute(UMordecaiAttributeSet::GetHealthAttribute()),
			HealthBefore - 20.f);
	}
	{
		FRuntimeEnv Env;
		ON_SCOPE_EXIT { Env.TeardownRuntime(); };
		if (!TestTrue("dodge runtime world", Env.SetupRuntime()))
		{
			return true;
		}
		Env.SetStamina(40.f);
		Env.Press(MordecaiGameplayTags::Input_Dodge);
		const UMordecaiGA_Dodge* Dodge = Env.Instance<UMordecaiGA_Dodge>();
		if (TestNotNull("real dodge instance", Dodge))
		{
			const float BaseSpeed = Dodge->DodgeDistanceCm / Dodge->GetDodgeDurationSeconds();
			TestEqual("the activation cost crosses Yellow to Red", Env.Stamina(), 25.f);
			TestTrue("dodge movement keeps the Yellow activation snapshot",
				FMath::IsNearlyEqual(
					Dodge->ComputeImpulseVelocity(FVector::ForwardVector).Size(),
					BaseSpeed,
					0.01f));
		}
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_StaminaRuntime_LowTierScalesBlockStability,
	"Mordecai.Stamina.Runtime.LowTierScalesBlockStability",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_StaminaRuntime_LowTierScalesBlockStability::RunTest(const FString& Parameters)
{
	using namespace MordecaiStaminaRuntimeTest;
	FRuntimeEnv Env;
	ON_SCOPE_EXIT { Env.TeardownRuntime(); };
	if (!TestTrue("runtime world", Env.SetupRuntime()))
	{
		return true;
	}
	Env.SetStamina(20.f);
	Env.Press(MordecaiGameplayTags::Input_Block);
	UMordecaiGA_Block* Block = Env.Instance<UMordecaiGA_Block>();
	if (!TestNotNull("real block instance", Block))
	{
		return true;
	}
	Env.TickSeconds(Block->GetPerfectBlockWindowSeconds() + 0.05f);
	const float Before = Env.Stamina();
	Block->ReceiveBlockedHit(10.f, Env.World->GetTimeSeconds());
	const float ExpectedDrain = 10.f * (1.f - Block->Stability * 0.75f);
	TestTrue("Red tier reduces effective block stability to Stability x 0.75",
		FMath::IsNearlyEqual(Before - Env.Stamina(), ExpectedDrain, 0.01f));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_StaminaRuntime_SprintDrainsAndTags,
	"Mordecai.Stamina.Runtime.SprintDrainsAndTags",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_StaminaRuntime_SprintDrainsAndTags::RunTest(const FString& Parameters)
{
	using namespace MordecaiStaminaRuntimeTest;
	FRuntimeEnv Env;
	ON_SCOPE_EXIT { Env.TeardownRuntime(); };
	if (!TestTrue("runtime world", Env.SetupRuntime()))
	{
		return true;
	}
	const float Before = Env.Stamina();
	Env.Player->SetSprinting(true);
	TestTrue("sprint toggle applies authoritative sprint tag",
		Env.GetASC()->HasMatchingGameplayTag(MordecaiGameplayTags::State_Sprinting));
	Env.TickMovingSeconds(0.5f);
	TestTrue("moving authority runtime continuously drains sprint stamina", Env.Stamina() < Before);
	Env.Player->SetSprinting(false);
	TestFalse("stopping sprint removes authoritative tag",
		Env.GetASC()->HasMatchingGameplayTag(MordecaiGameplayTags::State_Sprinting));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_StaminaRuntime_StandingSprintDoesNotDrain,
	"Mordecai.Stamina.Runtime.StandingSprintDoesNotDrain",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_StaminaRuntime_StandingSprintDoesNotDrain::RunTest(
	const FString& Parameters)
{
	using namespace MordecaiStaminaRuntimeTest;
	FRuntimeEnv Env;
	ON_SCOPE_EXIT { Env.TeardownRuntime(); };
	if (!TestTrue("runtime world", Env.SetupRuntime()))
	{
		return true;
	}

	Env.SetStamina(50.f);
	Env.TickSeconds(1.1f);
	const float BeforeToggle = Env.Stamina();
	Env.Player->GetCharacterMovement()->Velocity = FVector::ZeroVector;
	Env.Player->SetSprinting(true);
	Env.TickSeconds(0.35f);
	TestTrue("standing still with sprint toggled continues regenerating",
		Env.Stamina() > BeforeToggle);
	TestFalse("stationary sprint auto-clears after 0.25 seconds", Env.Player->bIsSprinting);
	TestFalse("stationary sprint removes the authoritative tag",
		Env.GetASC()->HasMatchingGameplayTag(MordecaiGameplayTags::State_Sprinting));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_StaminaRuntime_RegenDelayAfterSprintStops,
	"Mordecai.Stamina.Runtime.RegenDelayAfterSprintStops",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_StaminaRuntime_RegenDelayAfterSprintStops::RunTest(
	const FString& Parameters)
{
	using namespace MordecaiStaminaRuntimeTest;
	FRuntimeEnv Env;
	ON_SCOPE_EXIT { Env.TeardownRuntime(); };
	if (!TestTrue("runtime world", Env.SetupRuntime()))
	{
		return true;
	}

	Env.SetStamina(50.f);
	Env.Player->SetSprinting(true);
	Env.TickMovingSeconds(0.25f);
	Env.Player->SetSprinting(false);
	const float AtStop = Env.Stamina();
	Env.TickSeconds(0.75f);
	TestEqual("no regeneration before one delay after the last moving drain", Env.Stamina(), AtStop);
	Env.TickSeconds(0.5f);
	TestTrue("regeneration resumes one delay after sprint stops", Env.Stamina() > AtStop);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_StaminaRuntime_SprintInterruptsClearToggle,
	"Mordecai.Stamina.Runtime.SprintInterruptsClearToggle",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_StaminaRuntime_SprintInterruptsClearToggle::RunTest(
	const FString& Parameters)
{
	using namespace MordecaiStaminaRuntimeTest;
	{
		FRuntimeEnv Env;
		ON_SCOPE_EXIT { Env.TeardownRuntime(); };
		if (!TestTrue("stop runtime world", Env.SetupRuntime()))
		{
			return true;
		}
		Env.Player->SetSprinting(true);
		Env.TickSeconds(0.35f);
		TestFalse("stopping for 0.25 seconds clears sprint", Env.Player->bIsSprinting);
	}
	{
		FRuntimeEnv Env;
		ON_SCOPE_EXIT { Env.TeardownRuntime(); };
		if (!TestTrue("dodge runtime world", Env.SetupRuntime()))
		{
			return true;
		}
		Env.Player->SetSprinting(true);
		Env.Press(MordecaiGameplayTags::Input_Dodge);
		TestFalse("real dodge activation clears sprint", Env.Player->bIsSprinting);
		TestFalse("real dodge activation removes sprint tag",
			Env.GetASC()->HasMatchingGameplayTag(MordecaiGameplayTags::State_Sprinting));
	}
	{
		FRuntimeEnv Env;
		ON_SCOPE_EXIT { Env.TeardownRuntime(); };
		if (!TestTrue("death runtime world", Env.SetupRuntime()))
		{
			return true;
		}
		Env.Player->SetSprinting(true);
		Env.Player->HandlePlayerDeath();
		TestFalse("real death clears sprint", Env.Player->bIsSprinting);
		TestFalse("real death removes sprint tag",
			Env.GetASC()->HasMatchingGameplayTag(MordecaiGameplayTags::State_Sprinting));
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_StaminaRuntime_AllInterruptTagsClearSprint,
	"Mordecai.Stamina.Runtime.AllInterruptTagsClearSprint",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_StaminaRuntime_AllInterruptTagsClearSprint::RunTest(
	const FString& Parameters)
{
	using namespace MordecaiStaminaRuntimeTest;
	const struct
	{
		FGameplayTag Tag;
		const TCHAR* Name;
	} Cases[] = {
		{ MordecaiGameplayTags::State_AttackCommitted, TEXT("attack windup") },
		{ MordecaiGameplayTags::State_Blocking, TEXT("block") },
		{ MordecaiGameplayTags::State_Parrying, TEXT("parry") },
		{ MordecaiGameplayTags::Status_MicroStunned, TEXT("micro-stun") },
		{ MordecaiGameplayTags::State_PostureBroken, TEXT("posture break") },
		{ MordecaiGameplayTags::State_KnockedDown, TEXT("knockdown") },
	};

	for (const auto& Case : Cases)
	{
		FRuntimeEnv Env;
		if (!TestTrue(*FString::Printf(TEXT("%s runtime world"), Case.Name), Env.SetupRuntime()))
		{
			Env.TeardownRuntime();
			continue;
		}
		Env.Player->SetSprinting(true);
		Env.GetASC()->AddLooseGameplayTag(Case.Tag);
		TestFalse(*FString::Printf(TEXT("%s clears sprint"), Case.Name), Env.Player->bIsSprinting);
		TestFalse(*FString::Printf(TEXT("%s removes sprint tag"), Case.Name),
			Env.GetASC()->HasMatchingGameplayTag(MordecaiGameplayTags::State_Sprinting));
		Env.TeardownRuntime();
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_StaminaRuntime_ZeroStaminaActionsActivate,
	"Mordecai.Stamina.Runtime.ZeroStaminaActionsActivate",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_StaminaRuntime_ZeroStaminaActionsActivate::RunTest(const FString& Parameters)
{
	using namespace MordecaiStaminaRuntimeTest;
	const struct
	{
		FGameplayTag Input;
		TSubclassOf<UGameplayAbility> Ability;
		const TCHAR* Name;
	} Cases[] = {
		{ MordecaiGameplayTags::Input_LightAttack, UMordecaiGA_MeleeAttack::StaticClass(), TEXT("attack") },
		{ MordecaiGameplayTags::Input_Dodge, UMordecaiGA_Dodge::StaticClass(), TEXT("dodge") },
		{ MordecaiGameplayTags::Input_Block, UMordecaiGA_Block::StaticClass(), TEXT("block") }
	};

	for (const auto& Case : Cases)
	{
		FRuntimeEnv Env;
		if (!TestTrue(*FString::Printf(TEXT("%s runtime world"), Case.Name), Env.SetupRuntime()))
		{
			Env.TeardownRuntime();
			continue;
		}
		Env.SetStamina(0.f);
		Env.Press(Case.Input);
		TestTrue(*FString::Printf(TEXT("%s activates at zero stamina"), Case.Name), Env.Active(Case.Ability));
		Env.TeardownRuntime();
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_StaminaRuntime_ZeroStaminaSprintContinues,
	"Mordecai.Stamina.Runtime.ZeroStaminaSprintContinues",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_StaminaRuntime_ZeroStaminaSprintContinues::RunTest(const FString& Parameters)
{
	using namespace MordecaiStaminaRuntimeTest;
	FRuntimeEnv Env;
	ON_SCOPE_EXIT { Env.TeardownRuntime(); };
	if (!TestTrue("runtime world", Env.SetupRuntime()))
	{
		return true;
	}
	Env.SetStamina(1.f);
	Env.Player->SetSprinting(true);
	Env.TickMovingSeconds(0.2f);
	TestTrue("sprint remains toggled below zero", Env.Player->bIsSprinting);
	TestTrue("authoritative sprint tag remains below zero",
		Env.GetASC()->HasMatchingGameplayTag(MordecaiGameplayTags::State_Sprinting));
	TestTrue("stamina crossed zero", Env.Stamina() <= 0.f);
	const float ExpectedSpeed =
		Env.Player->WalkSpeed * Env.Player->SprintSpeedMultiplier;
	TestTrue("zero-stamina sprint keeps the tier-independent 1.5x speed",
		FMath::IsNearlyEqual(Env.Player->GetCharacterMovement()->MaxWalkSpeed, ExpectedSpeed, 0.01f));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_StaminaRuntime_DeadDoesNotRegenerate,
	"Mordecai.Stamina.Runtime.DeadDoesNotRegenerate",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_StaminaRuntime_DeadDoesNotRegenerate::RunTest(const FString& Parameters)
{
	using namespace MordecaiStaminaRuntimeTest;
	FRuntimeEnv Env;
	ON_SCOPE_EXIT { Env.TeardownRuntime(); };
	if (!TestTrue("runtime world", Env.SetupRuntime()))
	{
		return true;
	}
	Env.SetStamina(50.f);
	Env.GetASC()->AddLooseGameplayTag(MordecaiGameplayTags::State_Dead);
	Env.TickSeconds(2.f);
	TestEqual("dead player does not regenerate", Env.Stamina(), 50.f);
	Env.GetASC()->RemoveLooseGameplayTag(MordecaiGameplayTags::State_Dead);
	Env.TickFrames(1);
	TestTrue("same runtime resumes regeneration when alive", Env.Stamina() > 50.f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_StaminaRuntime_MaxStaminaRefreshesTier,
	"Mordecai.Stamina.Runtime.MaxStaminaRefreshesTier",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_StaminaRuntime_MaxStaminaRefreshesTier::RunTest(
	const FString& Parameters)
{
	using namespace MordecaiStaminaRuntimeTest;
	FRuntimeEnv Env;
	ON_SCOPE_EXIT { Env.TeardownRuntime(); };
	if (!TestTrue("runtime world", Env.SetupRuntime()))
	{
		return true;
	}

	Env.SetStamina(70.f);
	TestTrue("70/100 starts Green",
		Env.GetASC()->HasMatchingGameplayTag(MordecaiGameplayTags::Stamina_Tier_Green));
	Env.GetASC()->SetNumericAttributeBase(UMordecaiAttributeSet::GetEnduranceAttribute(), 12.f);
	UMordecaiAttributeSet* Attributes = const_cast<UMordecaiAttributeSet*>(
		Env.GetASC()->GetSet<UMordecaiAttributeSet>());
	if (!TestNotNull("runtime attribute set", Attributes))
	{
		return true;
	}
	Attributes->RecalculateDerivedStats();
	TestTrue("Endurance recalculation increases MaxStamina", Attributes->GetMaxStamina() > 100.f);
	TestTrue("the direct MaxStamina write refreshes 70 to Yellow",
		Env.GetASC()->HasMatchingGameplayTag(MordecaiGameplayTags::Stamina_Tier_Yellow));
	TestEqual("MaxStamina refresh leaves exactly one tier", CountTierTags(Env.GetASC()), 1);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_StaminaRuntime_RespawnRestoresFullGreen,
	"Mordecai.Stamina.Runtime.RespawnRestoresFullGreen",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_StaminaRuntime_RespawnRestoresFullGreen::RunTest(
	const FString& Parameters)
{
	using namespace MordecaiStaminaRuntimeTest;
	FRuntimeEnv Env;
	ON_SCOPE_EXIT { Env.TeardownRuntime(); };
	if (!TestTrue("runtime world", Env.SetupRuntime()))
	{
		return true;
	}

	Env.SetStamina(10.f);
	Env.Player->SetSprinting(true);
	Env.Player->HandlePlayerDeath();
	Env.Player->HandlePlayerRespawn();
	const float MaxStamina = Env.GetASC()->GetNumericAttribute(
		UMordecaiAttributeSet::GetMaxStaminaAttribute());
	TestEqual("respawn restores full stamina", Env.Stamina(), MaxStamina);
	TestTrue("respawn refreshes Green",
		Env.GetASC()->HasMatchingGameplayTag(MordecaiGameplayTags::Stamina_Tier_Green));
	TestEqual("respawn leaves exactly one tier", CountTierTags(Env.GetASC()), 1);
	TestFalse("respawn starts unsprinted", Env.Player->bIsSprinting);
	TestFalse("respawn clears authoritative sprint tag",
		Env.GetASC()->HasMatchingGameplayTag(MordecaiGameplayTags::State_Sprinting));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_StaminaRuntime_ASCUninitializeClearsSprint,
	"Mordecai.Stamina.Runtime.ASCUninitializeClearsSprint",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_StaminaRuntime_ASCUninitializeClearsSprint::RunTest(
	const FString& Parameters)
{
	using namespace MordecaiStaminaRuntimeTest;
	FRuntimeEnv Env;
	ON_SCOPE_EXIT { Env.TeardownRuntime(); };
	if (!TestTrue("runtime world", Env.SetupRuntime()))
	{
		return true;
	}
	ULyraPawnExtensionComponent* PawnExtension =
		ULyraPawnExtensionComponent::FindPawnExtensionComponent(Env.Player);
	if (!TestNotNull("pawn extension", PawnExtension))
	{
		return true;
	}

	UAbilitySystemComponent* PersistentASC = Env.GetASC();
	Env.Player->SetSprinting(true);
	PawnExtension->UninitializeAbilitySystem();
	TestFalse("ASC uninitialization resets the old pawn toggle", Env.Player->bIsSprinting);
	TestFalse("ASC uninitialization clears the persistent PlayerState tag",
		PersistentASC->HasMatchingGameplayTag(MordecaiGameplayTags::State_Sprinting));
	PawnExtension->InitializeAbilitySystem(
		Env.PlayerState->GetLyraAbilitySystemComponent(), Env.PlayerState);
	TestFalse("a newly attached pawn starts unsprinted", Env.Player->bIsSprinting);
	TestFalse("a newly attached pawn does not inherit the sprint tag",
		PersistentASC->HasMatchingGameplayTag(MordecaiGameplayTags::State_Sprinting));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_StaminaRuntime_HUDReadsAuthoritativeTier,
	"Mordecai.Stamina.Runtime.HUDReadsAuthoritativeTier",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_StaminaRuntime_HUDReadsAuthoritativeTier::RunTest(const FString& Parameters)
{
	using namespace MordecaiStaminaRuntimeTest;
	FRuntimeEnv Env;
	ON_SCOPE_EXIT { Env.TeardownRuntime(); };
	if (!TestTrue("runtime world", Env.SetupRuntime()))
	{
		return true;
	}
	UAbilitySystemComponent* ASC = Env.GetASC();
	Env.SetStamina(100.f);
	ASC->SetLooseGameplayTagCount(MordecaiGameplayTags::Stamina_Tier_Green, 0);
	ASC->SetLooseGameplayTagCount(MordecaiGameplayTags::Stamina_Tier_Red, 1);

	UMordecaiStaminaBarWidget* Widget = NewObject<UMordecaiStaminaBarWidget>();
	Widget->BindToASC(ASC);
	TestEqual("HUD follows the ASC Red tag even when the ratio is full",
		Widget->GetStaminaTier(), EMordecaiStaminaTier::Red);
	return true;
}
