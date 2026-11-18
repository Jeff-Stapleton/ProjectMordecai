// Project Mordecai — Stamina Runtime (US-122 / issue #143)
// These tests drive the real PlayerState ASC, granted combat abilities, character sprint toggle,
// and world/component ticks. They intentionally do not retest UMordecaiStaminaSystem's pure maths.

#include "Misc/AutomationTest.h"
#include "Misc/ScopeExit.h"

#include "AbilitySystem/LyraAbilitySystemComponent.h"
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
	Env.TickSeconds(0.5f);
	TestTrue("authority runtime continuously drains sprint stamina", Env.Stamina() < Before);
	Env.Player->SetSprinting(false);
	TestFalse("stopping sprint removes authoritative tag",
		Env.GetASC()->HasMatchingGameplayTag(MordecaiGameplayTags::State_Sprinting));
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
	Env.TickSeconds(0.2f);
	TestTrue("sprint remains toggled below zero", Env.Player->bIsSprinting);
	TestTrue("authoritative sprint tag remains below zero",
		Env.GetASC()->HasMatchingGameplayTag(MordecaiGameplayTags::State_Sprinting));
	TestTrue("stamina crossed zero", Env.Stamina() <= 0.f);
	const float ExpectedSpeed =
		Env.Player->WalkSpeed * Env.Player->SprintSpeedMultiplier
		* GetDefault<UMordecaiStaminaSystem>()->ExhaustedMultiplier;
	TestTrue("zero-stamina sprint uses exhausted effectiveness",
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
