// Project Mordecai — Stamina Runtime (US-122 / issue #143)
// These tests drive the real PlayerState ASC, granted combat abilities, character sprint toggle,
// and world/component ticks. They intentionally do not retest UMordecaiStaminaSystem's pure maths.

#include "Misc/AutomationTest.h"
#include "Misc/ScopeExit.h"

#include "AbilitySystem/LyraAbilitySystemComponent.h"
#include "Character/LyraPawnData.h"
#include "Character/LyraPawnExtensionComponent.h"
#include "EnhancedInputComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameplayAbilitySpec.h"
#include "GameplayEffect.h"
#include "InputAction.h"
#include "InputMappingContext.h"
#include "Mordecai/AbilitySystem/MordecaiAttributeSet.h"
#include "Mordecai/Combat/MordecaiGA_Block.h"
#include "Mordecai/Combat/MordecaiGA_Dodge.h"
#include "Mordecai/Combat/MordecaiGA_MeleeAttack.h"
#include "Mordecai/Combat/MordecaiGA_Parry.h"
#include "Mordecai/Combat/MordecaiStaminaSystem.h"
#include "Mordecai/MordecaiCharacter.h"
#include "Mordecai/MordecaiGameplayTags.h"
#include "Mordecai/MordecaiHeroComponent.h"
#include "Mordecai/MordecaiPlayerController.h"
#include "Mordecai/Input/MordecaiInputTriggers.h"
#include "Mordecai/StatusEffects/MordecaiStatusEffectComponent.h"
#include "Mordecai/StatusEffects/Effects/MordecaiGE_Fear.h"
#include "Mordecai/StatusEffects/Effects/MordecaiGE_Focused.h"
#include "Mordecai/StatusEffects/Effects/MordecaiGE_Rooted.h"
#include "Mordecai/UI/MordecaiStaminaBarWidget.h"
#include "Tests/Recovery/MordecaiRecoveryTestEnv.h"

namespace MordecaiStaminaRuntimeTest
{
	static constexpr float FrameSeconds = 0.05f;

	static bool ExecuteBinding(
		const UEnhancedInputComponent* EIC,
		const UInputAction* Action,
		const ETriggerEvent Event,
		const UObject* BoundTo)
	{
		for (const TUniquePtr<FEnhancedInputActionEventBinding>& Binding :
			EIC->GetActionEventBindings())
		{
			if (Binding && Binding->GetAction() == Action
				&& Binding->GetTriggerEvent() == Event
				&& Binding->IsBoundToObject(BoundTo))
			{
				Binding->Execute(FInputActionInstance(Action));
				return true;
			}
		}
		return false;
	}

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

		FActiveGameplayEffectHandle ApplyStatus(
			TSubclassOf<UGameplayEffect> EffectClass) const
		{
			ULyraAbilitySystemComponent* ASC = GetASC();
			const FGameplayEffectSpecHandle Spec =
				ASC->MakeOutgoingSpec(EffectClass, 1.f, ASC->MakeEffectContext());
			return Spec.IsValid()
				? ASC->ApplyGameplayEffectSpecToSelf(*Spec.Data.Get())
				: FActiveGameplayEffectHandle();
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

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_StaminaRuntime_BreakFreeAtFloorRestartsDelay,
	"Mordecai.Stamina.Runtime.BreakFreeAtFloorRestartsDelay",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_StaminaRuntime_BreakFreeAtFloorRestartsDelay::RunTest(
	const FString& Parameters)
{
	using namespace MordecaiStaminaRuntimeTest;
	FRuntimeEnv Env;
	ON_SCOPE_EXIT { Env.TeardownRuntime(); };
	if (!TestTrue("runtime world", Env.SetupRuntime()))
	{
		return true;
	}

	UMordecaiStatusEffectComponent* Status =
		NewObject<UMordecaiStatusEffectComponent>(Env.Player);
	Status->SetAbilitySystemComponentOverride(Env.GetASC());
	Status->RegisterComponent();
	Env.SetStamina(-50.f);
	Env.TickSeconds(0.75f);
	TestTrue("real Rooted effect applied",
		Status->ApplyStatusEffect(UMordecaiGE_Rooted::StaticClass(), Env.Player).IsValid());

	FGameplayEventData EventData;
	EventData.Instigator = Env.Player;
	Env.GetASC()->HandleGameplayEvent(MordecaiGameplayTags::Event_BreakFree, &EventData);
	TestFalse("break-free removes Rooted",
		Status->HasStatusEffect(MordecaiGameplayTags::Status_Rooted));
	TestEqual("break-free cannot spend below the floor", Env.Stamina(), -50.f);

	// This passes the old delay's remaining 0.25 s, but not a full new delay.
	Env.TickSeconds(0.5f);
	TestEqual("accepted break-free at the floor restarts the full regen delay",
		Env.Stamina(), -50.f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_StaminaRuntime_SpendClampsAtFloor,
	"Mordecai.Stamina.Runtime.SpendClampsAtFloor",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_StaminaRuntime_SpendClampsAtFloor::RunTest(const FString& Parameters)
{
	using namespace MordecaiStaminaRuntimeTest;
	FRuntimeEnv Env;
	ON_SCOPE_EXIT { Env.TeardownRuntime(); };
	if (!TestTrue("runtime world", Env.SetupRuntime()))
	{
		return true;
	}

	Env.SetStamina(-45.f);
	Env.GetASC()->ApplyModToAttribute(
		UMordecaiAttributeSet::GetStaminaAttribute(), EGameplayModOp::Additive, -25.f);
	TestEqual("spending near the floor clamps at -50", Env.Stamina(), -50.f);
	Env.GetASC()->ApplyModToAttribute(
		UMordecaiAttributeSet::GetStaminaAttribute(), EGameplayModOp::Additive, -25.f);
	TestEqual("spending at the floor never goes below -50", Env.Stamina(), -50.f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_StaminaRuntime_BlockedHitRestartsDelay,
	"Mordecai.Stamina.Runtime.BlockedHitRestartsDelay",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_StaminaRuntime_BlockedHitRestartsDelay::RunTest(const FString& Parameters)
{
	using namespace MordecaiStaminaRuntimeTest;
	FRuntimeEnv Env;
	ON_SCOPE_EXIT { Env.TeardownRuntime(); };
	if (!TestTrue("runtime world", Env.SetupRuntime()))
	{
		return true;
	}

	UMordecaiAttackProfileDataAsset* EnemyProfile =
		MordecaiRecoveryTest::MakeStubProfile();
	EnemyProfile->DamageProfile.BasePower = 15.f;
	AMordecaiEnemyCharacter* Enemy =
		Env.SpawnEnemy(FVector(100.f, 0.f, 0.f), { EnemyProfile });
	if (!TestNotNull("real enemy attacker", Enemy))
	{
		return true;
	}
	Enemy->SetActorRotation(FRotator(0.f, 180.f, 0.f));

	Env.SetStamina(50.f);
	Env.TickSeconds(0.4f);
	Env.Press(MordecaiGameplayTags::Input_Block);
	UMordecaiGA_Block* Block = Env.Instance<UMordecaiGA_Block>();
	if (!TestNotNull("real block instance", Block))
	{
		return true;
	}
	Env.TickSeconds(Block->GetPerfectBlockWindowSeconds() + 0.05f);
	const float BeforeHit = Env.Stamina();

	UAbilitySystemComponent* EnemyASC = Enemy->GetAbilitySystemComponent();
	FGameplayAbilitySpec* EnemySpec =
		EnemyASC ? EnemyASC->FindAbilitySpecFromClass(UMordecaiGA_MeleeAttack::StaticClass()) : nullptr;
	UMordecaiGA_MeleeAttack* EnemyMelee =
		EnemySpec ? Cast<UMordecaiGA_MeleeAttack>(EnemySpec->GetPrimaryInstance()) : nullptr;
	if (!TestNotNull("real enemy melee instance", EnemyMelee)
		|| !TestTrue("real enemy swing activates",
			EnemyASC->TryActivateAbilityByClass(UMordecaiGA_MeleeAttack::StaticClass())))
	{
		return true;
	}
	for (int32 Frame = 0;
		Frame < 10 && EnemyMelee->GetCurrentPhase() == EMordecaiAttackPhase::Windup;
		++Frame)
	{
		Env.TickFrames(1);
	}
	TestEqual("enemy swing reaches the hit-defense path",
		EnemyMelee->GetCurrentPhase(), EMordecaiAttackPhase::Active);
	const float AfterHit = Env.Stamina();
	const float ExpectedDrain =
		EnemyProfile->DamageProfile.BasePower * (1.f - Block->Stability);
	TestTrue("the blocked enemy hit spends stamina",
		FMath::IsNearlyEqual(BeforeHit - AfterHit, ExpectedDrain, 0.01f));

	// This passes the pre-hit delay's remaining time, but not a full post-hit delay.
	Env.TickSeconds(0.5f);
	TestEqual("the real blocked hit restarts the full regen delay",
		Env.Stamina(), AfterHit);
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

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_StaminaRuntime_FocusedForgivesRedPenaltyOnMeleeHit,
	"Mordecai.Stamina.Runtime.FocusedForgivesRedPenaltyOnMeleeHit",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_StaminaRuntime_FocusedForgivesRedPenaltyOnMeleeHit::RunTest(
	const FString& Parameters)
{
	using namespace MordecaiStaminaRuntimeTest;
	FRuntimeEnv Env;
	ON_SCOPE_EXIT { Env.TeardownRuntime(); };
	if (!TestTrue("runtime world", Env.SetupRuntime()))
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
	TestTrue("real Focused effect applied",
		Env.ApplyStatus(UMordecaiGE_Focused::StaticClass()).IsValid());
	Env.SetStamina(20.f);
	Env.Press(MordecaiGameplayTags::Input_LightAttack);
	for (int32 Frame = 0; Frame < 10
		&& FMath::IsNearlyEqual(
			TargetASC->GetNumericAttribute(UMordecaiAttributeSet::GetHealthAttribute()),
			HealthBefore);
		++Frame)
	{
		Env.TickFrames(1);
	}
	TestEqual("Focused treats the Red penalty as Normal on a real hit",
		TargetASC->GetNumericAttribute(UMordecaiAttributeSet::GetHealthAttribute()),
		HealthBefore - 20.f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_StaminaRuntime_FearAmplifiesRedPenaltyOnMeleeHit,
	"Mordecai.Stamina.Runtime.FearAmplifiesRedPenaltyOnMeleeHit",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_StaminaRuntime_FearAmplifiesRedPenaltyOnMeleeHit::RunTest(
	const FString& Parameters)
{
	using namespace MordecaiStaminaRuntimeTest;
	FRuntimeEnv Env;
	ON_SCOPE_EXIT { Env.TeardownRuntime(); };
	if (!TestTrue("runtime world", Env.SetupRuntime()))
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
	TestTrue("real Fear effect applied",
		Env.ApplyStatus(UMordecaiGE_Fear::StaticClass()).IsValid());
	Env.SetStamina(20.f);
	Env.Press(MordecaiGameplayTags::Input_LightAttack);
	for (int32 Frame = 0; Frame < 10
		&& FMath::IsNearlyEqual(
			TargetASC->GetNumericAttribute(UMordecaiAttributeSet::GetHealthAttribute()),
			HealthBefore);
		++Frame)
	{
		Env.TickFrames(1);
	}
	TestTrue("Fear scales the Red penalty by 1.5 on a real hit",
		FMath::IsNearlyEqual(
			TargetASC->GetNumericAttribute(UMordecaiAttributeSet::GetHealthAttribute()),
			HealthBefore - 12.5f,
			0.01f));
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

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_StaminaRuntime_BlockAttributesScaleRealDrain,
	"Mordecai.Stamina.Runtime.BlockAttributesScaleRealDrain",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_StaminaRuntime_BlockAttributesScaleRealDrain::RunTest(
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
	Env.GetASC()->SetNumericAttributeBase(
		UMordecaiAttributeSet::GetBlockStabilityMultiplierAttribute(), 0.5f);
	Env.GetASC()->SetNumericAttributeBase(
		UMordecaiAttributeSet::GetBlockStaminaCostMultiplierAttribute(), 2.f);
	Env.Press(MordecaiGameplayTags::Input_Block);
	UMordecaiGA_Block* Block = Env.Instance<UMordecaiGA_Block>();
	if (!TestNotNull("real block instance", Block))
	{
		return true;
	}
	Env.TickSeconds(Block->GetPerfectBlockWindowSeconds() + 0.05f);
	const float Before = Env.Stamina();
	Block->ReceiveBlockedHit(10.f, Env.World->GetTimeSeconds());
	const float EffectiveStability = Block->Stability * 0.5f;
	const float ExpectedDrain = 10.f * (1.f - EffectiveStability) * 2.f;
	TestTrue("block stability and stamina-cost attributes both scale the real drain",
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
	Env.TickSeconds(0.75f);
	TestTrue("standing still with sprint toggled continues regenerating",
		Env.Stamina() > BeforeToggle);
	TestTrue("the one-second toggle grace keeps an idle sprint available",
		Env.Player->bIsSprinting);
	const float DuringGrace = Env.Stamina();
	Env.TickSeconds(0.55f);
	TestTrue("standing still never drains stamina", Env.Stamina() >= DuringGrace);
	TestFalse("stationary sprint clears after grace plus stopped threshold", Env.Player->bIsSprinting);
	TestFalse("stationary sprint removes the authoritative tag",
		Env.GetASC()->HasMatchingGameplayTag(MordecaiGameplayTags::State_Sprinting));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_StaminaRuntime_SprintGraceAllowsDelayedMovement,
	"Mordecai.Stamina.Runtime.SprintGraceAllowsDelayedMovement",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_StaminaRuntime_SprintGraceAllowsDelayedMovement::RunTest(
	const FString& Parameters)
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
	Env.TickSeconds(0.5f);
	TestTrue("Shift then waiting 0.5 seconds keeps sprint toggled", Env.Player->bIsSprinting);
	TestEqual("the stationary grace spends no stamina", Env.Stamina(), Before);
	Env.TickMovingSeconds(0.15f);
	TestTrue("starting movement after 0.5 seconds keeps sprint toggled", Env.Player->bIsSprinting);
	TestTrue("the delayed moving sprint drains stamina", Env.Stamina() < Before);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_StaminaRuntime_MovingSprintSurvivesOneSecond,
	"Mordecai.Stamina.Runtime.MovingSprintSurvivesOneSecond",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_StaminaRuntime_MovingSprintSurvivesOneSecond::RunTest(
	const FString& Parameters)
{
	using namespace MordecaiStaminaRuntimeTest;
	FRuntimeEnv Env;
	ON_SCOPE_EXIT { Env.TeardownRuntime(); };
	if (!TestTrue("runtime world", Env.SetupRuntime()))
	{
		return true;
	}
	Env.Player->SetSprinting(true);
	Env.TickMovingSeconds(1.1f);
	TestTrue("a continuously moving sprint remains toggled after one second",
		Env.Player->bIsSprinting);
	TestTrue("a continuously moving sprint retains its authoritative tag",
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
		if (!TestTrue("attack runtime world", Env.SetupRuntime()))
		{
			return true;
		}
		Env.Player->SetSprinting(true);
		Env.Press(MordecaiGameplayTags::Input_LightAttack);
		TestFalse("real attack windup clears sprint", Env.Player->bIsSprinting);
		TestFalse("real attack windup removes sprint tag",
			Env.GetASC()->HasMatchingGameplayTag(MordecaiGameplayTags::State_Sprinting));
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
		if (!TestTrue("block runtime world", Env.SetupRuntime()))
		{
			return true;
		}
		Env.Player->SetSprinting(true);
		Env.Press(MordecaiGameplayTags::Input_Block);
		TestTrue("real block activates",
			Env.GetASC()->HasMatchingGameplayTag(MordecaiGameplayTags::State_Blocking));
		TestFalse("real block activation clears sprint", Env.Player->bIsSprinting);
		TestFalse("real block activation removes sprint tag",
			Env.GetASC()->HasMatchingGameplayTag(MordecaiGameplayTags::State_Sprinting));
	}
	{
		FRuntimeEnv Env;
		ON_SCOPE_EXIT { Env.TeardownRuntime(); };
		if (!TestTrue("parry runtime world", Env.SetupRuntime()))
		{
			return true;
		}
		Env.Player->SetSprinting(true);
		Env.Press(MordecaiGameplayTags::Input_Parry);
		TestTrue("real parry activates", Env.Active(UMordecaiGA_Parry::StaticClass()));
		TestFalse("real parry activation clears sprint", Env.Player->bIsSprinting);
		TestFalse("real parry activation removes sprint tag",
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

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_StaminaRuntime_SprintToggleRefusedWhileBlocking,
	"Mordecai.Stamina.Runtime.SprintToggleRefusedWhileBlocking",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_StaminaRuntime_SprintToggleRefusedWhileBlocking::RunTest(
	const FString& Parameters)
{
	using namespace MordecaiStaminaRuntimeTest;
	FRuntimeEnv Env;
	ON_SCOPE_EXIT { Env.TeardownRuntime(); };
	if (!TestTrue("runtime world", Env.SetupRuntime()))
	{
		return true;
	}
	Env.Press(MordecaiGameplayTags::Input_Block);
	TestTrue("real block activation owns the blocking tag",
		Env.GetASC()->HasMatchingGameplayTag(MordecaiGameplayTags::State_Blocking));
	Env.Player->SetSprinting(true);
	TestFalse("sprint toggle is refused while the real block is active",
		Env.Player->bIsSprinting);
	TestFalse("refused sprint adds no authoritative tag",
		Env.GetASC()->HasMatchingGameplayTag(MordecaiGameplayTags::State_Sprinting));
	Env.Release(MordecaiGameplayTags::Input_Block);
	TestFalse("releasing block does not resurrect the refused sprint",
		Env.Player->bIsSprinting);
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

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FMordecai_StaminaRuntime_ShiftTabPreservesSprintDuringInterrupts,
	"Mordecai.Stamina.Runtime.ShiftTabPreservesSprintDuringInterrupts",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_StaminaRuntime_ShiftTabPreservesSprintDuringInterrupts::RunTest(
	const FString& Parameters)
{
	using namespace MordecaiStaminaRuntimeTest;
	const struct
	{
		FGameplayTag Tag;
		const TCHAR* Name;
	} Cases[] = {
		{ MordecaiGameplayTags::State_AttackCommitted, TEXT("attack committed") },
		{ MordecaiGameplayTags::State_Dodging, TEXT("dodge") },
		{ MordecaiGameplayTags::State_Blocking, TEXT("block") },
	};

	for (const auto& Case : Cases)
	{
		FRuntimeEnv Env;
		if (!TestTrue(
			*FString::Printf(TEXT("%s runtime world"), Case.Name),
			Env.SetupRuntime()))
		{
			Env.TeardownRuntime();
			continue;
		}

		UMordecaiHeroComponent* Hero =
			Env.Player->FindComponentByClass<UMordecaiHeroComponent>();
		UEnhancedInputComponent* EIC =
			NewObject<UEnhancedInputComponent>(Env.Player);
		const UInputMappingContext* Context =
			Hero ? Hero->BuildAndBindRuntimeInput(EIC) : nullptr;
		const UInputAction* KeyboardSprint = nullptr;
		const UInputAction* ShiftTab = nullptr;
		for (const FEnhancedActionKeyMapping& Mapping :
			Context ? Context->GetMappings()
					: TArray<FEnhancedActionKeyMapping>())
		{
			for (const TObjectPtr<UInputTrigger>& Trigger : Mapping.Triggers)
			{
				if (Mapping.Key == EKeys::LeftShift
					&& ExactCast<UMordecaiInputTriggerFreshKeyPress>(Trigger.Get()))
				{
					KeyboardSprint = Mapping.Action;
				}
				if (Mapping.Key == EKeys::Tab
					&& ExactCast<UMordecaiInputTriggerShiftPressed>(Trigger.Get()))
				{
					ShiftTab = Mapping.Action;
				}
			}
		}
		if (!TestNotNull(TEXT("keyboard sprint action"), KeyboardSprint)
			|| !TestNotNull(TEXT("Shift+Tab action"), ShiftTab))
		{
			Env.TeardownRuntime();
			continue;
		}

		Env.GetASC()->AddLooseGameplayTag(Case.Tag);
		TestFalse(*FString::Printf(TEXT("%s begins unsprinted"), Case.Name),
			Env.Player->bIsSprinting);
		TestTrue(*FString::Printf(TEXT("%s Shift binding executes"), Case.Name),
			ExecuteBinding(EIC, KeyboardSprint, ETriggerEvent::Triggered, Hero));
		TestFalse(*FString::Printf(TEXT("%s refuses the sprint toggle"), Case.Name),
			Env.Player->bIsSprinting);
		TestTrue(*FString::Printf(TEXT("%s chord binding executes"), Case.Name),
			ExecuteBinding(EIC, ShiftTab, ETriggerEvent::Triggered, Hero));
		TestFalse(*FString::Printf(
			TEXT("%s Shift+Tab preserves the pre-chord sprint intent"),
			Case.Name), Env.Player->bIsSprinting);
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

	// Simulate stale persistent PlayerState state before a replacement pawn attaches.
	// The new pawn's OnAbilitySystemInitialized reset must clear this independently
	// of the old pawn's uninitialization cleanup above.
	PersistentASC->SetLooseGameplayTagCount(
		MordecaiGameplayTags::State_Sprinting,
		1,
		EGameplayTagReplicationState::TagOnly);
	AMordecaiCharacter* NewPawn = Env.World->SpawnActor<AMordecaiCharacter>(
		AMordecaiCharacter::StaticClass(),
		FVector(500.f, 0.f, 0.f),
		FRotator::ZeroRotator,
		MordecaiRecoveryTest::FRecoveryTestEnv::AlwaysSpawn());
	if (!TestNotNull("replacement pawn", NewPawn))
	{
		return true;
	}
	ULyraPawnExtensionComponent* NewPawnExtension =
		ULyraPawnExtensionComponent::FindPawnExtensionComponent(NewPawn);
	if (!TestNotNull("replacement pawn extension", NewPawnExtension))
	{
		return true;
	}
	NewPawnExtension->SetPawnData(NewObject<ULyraPawnData>(GetTransientPackage()));
	NewPawnExtension->InitializeAbilitySystem(
		Env.PlayerState->GetLyraAbilitySystemComponent(), Env.PlayerState);
	TestFalse("a newly attached pawn starts unsprinted", NewPawn->bIsSprinting);
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
