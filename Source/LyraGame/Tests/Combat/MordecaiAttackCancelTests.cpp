// Project Mordecai — US-125 attack cancellation, dodge travel/reward, and combo-swap safety.

#include "Misc/AutomationTest.h"
#include "Misc/ScopeExit.h"

#include "AbilitySystem/LyraAbilitySystemComponent.h"
#include "Components/BoxComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameplayAbilitySpec.h"
#include "Mordecai/AbilitySystem/MordecaiAttributeSet.h"
#include "Mordecai/Combat/MordecaiGA_Block.h"
#include "Mordecai/Combat/MordecaiGA_Dodge.h"
#include "Mordecai/Combat/MordecaiGA_MeleeAttack.h"
#include "Mordecai/Combat/MordecaiGA_Parry.h"
#include "Mordecai/Combat/MordecaiHitDefense.h"
#include "Mordecai/MordecaiGameplayTags.h"
#include "Mordecai/MordecaiPlayerController.h"
#include "Mordecai/Weapons/MordecaiEquipmentComponent.h"
#include "Mordecai/Weapons/MordecaiWeaponProfileFactory.h"
#include "Tests/Recovery/MordecaiRecoveryTestEnv.h"

namespace MordecaiAttackCancelTest
{
	static constexpr float FrameSeconds = 0.02f;

	struct FRuntimeEnv : MordecaiRecoveryTest::FRecoveryTestEnv
	{
		AMordecaiPlayerController* GrantPC = nullptr;
		UMordecaiAttackProfileDataAsset* AttackProfile = nullptr;
		UMordecaiEquipmentComponent* Equipment = nullptr;
		AActor* Floor = nullptr;

		bool SetupRuntime(bool bCancelableDodge = true, bool bCancelableBlock = true)
		{
			if (!Setup(FVector::ZeroVector))
			{
				return false;
			}
			if (!PlayerState->HasActorBegunPlay())
			{
				PlayerState->DispatchBeginPlay();
			}
			World->TimeSeconds = 10.f;
			Floor = World->SpawnActor<AActor>();
			if (!Floor)
			{
				return false;
			}
			UBoxComponent* FloorCollision =
				NewObject<UBoxComponent>(Floor, TEXT("US125TestFloor"));
			Floor->SetRootComponent(FloorCollision);
			FloorCollision->InitBoxExtent(FVector(5000.f, 5000.f, 10.f));
			FloorCollision->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
			FloorCollision->SetCollisionResponseToAllChannels(ECR_Block);
			FloorCollision->RegisterComponent();
			Floor->SetActorLocation(FVector(0.f, 0.f, -100.f));
			Player->GetCharacterMovement()->Activate(true);
			Player->GetCharacterMovement()->SetComponentTickEnabled(true);
			Player->GetCharacterMovement()->SetMovementMode(MOVE_Walking);

			AttackProfile = MordecaiRecoveryTest::MakeStubProfile();
			AttackProfile->WindupTimeMs = 100.f;
			AttackProfile->ActiveTimeMs = 100.f;
			AttackProfile->RecoveryTimeMs = 300.f;
			AttackProfile->CancelableIntoDodge = bCancelableDodge;
			AttackProfile->CancelableIntoBlock = bCancelableBlock;

			GrantPC = NewObject<AMordecaiPlayerController>();
			GrantPC->PlayerState = PlayerState;
			GrantPC->DefaultSpellAbilities.Reset();
			GrantPC->MeleeAttackProfiles = { AttackProfile };
			GrantPC->GrantCombatAbilities();
			TickFrames(1);
			Equipment = NewObject<UMordecaiEquipmentComponent>(Player);
			Equipment->UnarmedAttackProfiles = { AttackProfile };
			if (UMordecaiGA_MeleeAttack* Melee = Instance<UMordecaiGA_MeleeAttack>())
			{
				Melee->SetEquipmentComponentOverride(Equipment);
			}
			return GetASC() != nullptr;
		}

		ULyraAbilitySystemComponent* GetASC() const
		{
			return Cast<ULyraAbilitySystemComponent>(GetPlayerASC());
		}

		void Press(const FGameplayTag& InputTag) const
		{
			GetASC()->AbilityInputTagPressed(InputTag);
			GetASC()->ProcessAbilityInput(0.f, false);
		}

		void Release(const FGameplayTag& InputTag) const
		{
			GetASC()->AbilityInputTagReleased(InputTag);
			GetASC()->ProcessAbilityInput(0.f, false);
		}

		void TickFrames(int32 Frames) const
		{
			for (int32 Frame = 0; Frame < Frames; ++Frame)
			{
				++GFrameCounter;
				World->Tick(LEVELTICK_All, FrameSeconds);
				// The recovery test world never begins play, so its character movement component
				// is not in the world's normal tick list. Tick it explicitly for runtime dodge travel.
				UCharacterMovementComponent* Movement =
					Player->GetCharacterMovement();
				Movement->TickComponent(
					FrameSeconds, LEVELTICK_All, &Movement->PrimaryComponentTick);
			}
		}

		void TickSeconds(float Seconds) const
		{
			TickFrames(FMath::CeilToInt(Seconds / FrameSeconds));
		}

		void SetStamina(float Value) const
		{
			GetASC()->SetNumericAttributeBase(
				UMordecaiAttributeSet::GetStaminaAttribute(), Value);
			TickFrames(1);
		}

		float Stamina() const
		{
			return GetASC()->GetNumericAttribute(
				UMordecaiAttributeSet::GetStaminaAttribute());
		}

		template <typename AbilityT>
		AbilityT* Instance() const
		{
			FGameplayAbilitySpec* Spec =
				GetASC()->FindAbilitySpecFromClass(AbilityT::StaticClass());
			return Spec ? Cast<AbilityT>(Spec->GetPrimaryInstance()) : nullptr;
		}

		template <typename AbilityT>
		bool Active() const
		{
			const FGameplayAbilitySpec* Spec =
				GetASC()->FindAbilitySpecFromClass(AbilityT::StaticClass());
			return Spec && Spec->IsActive();
		}

		void EnterRecovery() const
		{
			Press(MordecaiGameplayTags::Input_LightAttack);
			for (int32 Frame = 0; Frame < 25; ++Frame)
			{
				const UMordecaiGA_MeleeAttack* Melee = Instance<UMordecaiGA_MeleeAttack>();
				if (Melee && Melee->GetCurrentPhase() == EMordecaiAttackPhase::Recovery)
				{
					return;
				}
				TickFrames(1);
			}
		}

		void TeardownRuntime()
		{
			if (GrantPC)
			{
				GrantPC->PlayerState = nullptr;
			}
			GrantPC = nullptr;
			AttackProfile = nullptr;
			Equipment = nullptr;
			Floor = nullptr;
			Teardown();
		}
	};
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_AttackCancel_RecoveryDodgeCancelsBeforeActivation,
	"Mordecai.AttackCancel.RecoveryDodgeCancelsBeforeActivation",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_AttackCancel_RecoveryDodgeCancelsBeforeActivation::RunTest(const FString& Parameters)
{
	using namespace MordecaiAttackCancelTest;
	FRuntimeEnv Env;
	ON_SCOPE_EXIT { Env.TeardownRuntime(); };
	if (!TestTrue("runtime world", Env.SetupRuntime()))
	{
		return true;
	}
	Env.EnterRecovery();
	TestTrue("attack reached Recovery", Env.Instance<UMordecaiGA_MeleeAttack>()
		&& Env.Instance<UMordecaiGA_MeleeAttack>()->GetCurrentPhase() == EMordecaiAttackPhase::Recovery);
	Env.Press(MordecaiGameplayTags::Input_Dodge);
	TestFalse("melee ends before dodge state begins", Env.Active<UMordecaiGA_MeleeAttack>());
	TestFalse("Attacking tag is removed", Env.GetASC()->HasMatchingGameplayTag(
		MordecaiGameplayTags::State_Attacking));
	TestFalse("ComboWindow tag is removed", Env.GetASC()->HasMatchingGameplayTag(
		MordecaiGameplayTags::State_ComboWindow));
	TestTrue("dodge activates", Env.GetASC()->HasMatchingGameplayTag(
		MordecaiGameplayTags::State_Dodging));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_AttackCancel_RecoveryBlockCancelsBeforeActivation,
	"Mordecai.AttackCancel.RecoveryBlockCancelsBeforeActivation",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_AttackCancel_RecoveryBlockCancelsBeforeActivation::RunTest(const FString& Parameters)
{
	using namespace MordecaiAttackCancelTest;
	FRuntimeEnv Env;
	ON_SCOPE_EXIT { Env.TeardownRuntime(); };
	if (!TestTrue("runtime world", Env.SetupRuntime()))
	{
		return true;
	}
	Env.EnterRecovery();
	Env.Press(MordecaiGameplayTags::Input_Block);
	TestFalse("melee ends before block", Env.Active<UMordecaiGA_MeleeAttack>());
	TestFalse("Attacking tag is removed", Env.GetASC()->HasMatchingGameplayTag(
		MordecaiGameplayTags::State_Attacking));
	TestFalse("ComboWindow tag is removed", Env.GetASC()->HasMatchingGameplayTag(
		MordecaiGameplayTags::State_ComboWindow));
	TestTrue("block activates", Env.Active<UMordecaiGA_Block>());
	Env.Release(MordecaiGameplayTags::Input_Block);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_AttackCancel_CommittedDefenseRejected,
	"Mordecai.AttackCancel.CommittedDefenseRejected",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_AttackCancel_CommittedDefenseRejected::RunTest(const FString& Parameters)
{
	using namespace MordecaiAttackCancelTest;
	for (const FGameplayTag& DefenseInput :
		{ MordecaiGameplayTags::Input_Dodge.GetTag(), MordecaiGameplayTags::Input_Block.GetTag() })
	{
		FRuntimeEnv Env;
		if (!TestTrue("runtime world", Env.SetupRuntime()))
		{
			Env.TeardownRuntime();
			continue;
		}
		Env.Press(MordecaiGameplayTags::Input_LightAttack);
		TestTrue("attack is committed", Env.GetASC()->HasMatchingGameplayTag(
			MordecaiGameplayTags::State_AttackCommitted));
		Env.Press(DefenseInput);
		TestTrue("committed attack remains active", Env.Active<UMordecaiGA_MeleeAttack>());
		TestFalse("dodge is rejected while committed", Env.Active<UMordecaiGA_Dodge>());
		TestFalse("block is rejected while committed", Env.Active<UMordecaiGA_Block>());
		Env.TeardownRuntime();
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_AttackCancel_HeavyRecoveryRemainsCommitted,
	"Mordecai.AttackCancel.HeavyRecoveryRemainsCommitted",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_AttackCancel_HeavyRecoveryRemainsCommitted::RunTest(const FString& Parameters)
{
	using namespace MordecaiAttackCancelTest;
	for (const FGameplayTag& DefenseInput :
		{ MordecaiGameplayTags::Input_Dodge.GetTag(), MordecaiGameplayTags::Input_Block.GetTag() })
	{
		FRuntimeEnv Env;
		if (!TestTrue("runtime world", Env.SetupRuntime()))
		{
			Env.TeardownRuntime();
			continue;
		}
		Env.AttackProfile->InputSlot = EMordecaiInputSlot::Heavy;
		Env.EnterRecovery();
		Env.Press(DefenseInput);
		TestTrue("heavy attack remains active in Recovery",
			Env.Active<UMordecaiGA_MeleeAttack>());
		TestFalse("Dodge cannot cancel heavy Recovery", Env.Active<UMordecaiGA_Dodge>());
		TestFalse("Block cannot cancel heavy Recovery", Env.Active<UMordecaiGA_Block>());
		Env.TeardownRuntime();
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_AttackCancel_ProfileFlagsGateRecoveryCancel,
	"Mordecai.AttackCancel.ProfileFlagsGateRecoveryCancel",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_AttackCancel_ProfileFlagsGateRecoveryCancel::RunTest(const FString& Parameters)
{
	using namespace MordecaiAttackCancelTest;
	for (const FGameplayTag& DefenseInput :
		{ MordecaiGameplayTags::Input_Dodge.GetTag(), MordecaiGameplayTags::Input_Block.GetTag() })
	{
		FRuntimeEnv Env;
		if (!TestTrue("runtime world", Env.SetupRuntime(false, false)))
		{
			Env.TeardownRuntime();
			continue;
		}
		Env.EnterRecovery();
		Env.Press(DefenseInput);
		TestTrue("uncancelable light Recovery remains active",
			Env.Active<UMordecaiGA_MeleeAttack>());
		TestFalse("Dodge respects its profile flag", Env.Active<UMordecaiGA_Dodge>());
		TestFalse("Block respects its profile flag", Env.Active<UMordecaiGA_Block>());
		Env.TeardownRuntime();
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_AttackCancel_ParryDoesNotCancelAttack,
	"Mordecai.AttackCancel.ParryDoesNotCancelAttack",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_AttackCancel_ParryDoesNotCancelAttack::RunTest(const FString& Parameters)
{
	using namespace MordecaiAttackCancelTest;
	FRuntimeEnv Env;
	ON_SCOPE_EXIT { Env.TeardownRuntime(); };
	if (!TestTrue("runtime world", Env.SetupRuntime()))
	{
		return true;
	}
	Env.Press(MordecaiGameplayTags::Input_LightAttack);
	Env.Press(MordecaiGameplayTags::Input_Parry);
	TestTrue("committed melee remains active", Env.Active<UMordecaiGA_MeleeAttack>());
	TestFalse("parry cannot overlap or cancel an attack", Env.Active<UMordecaiGA_Parry>());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_Dodge_RuntimeTravelsTierScaledDistance,
	"Mordecai.Dodge.RuntimeTravelsTierScaledDistance",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_Dodge_RuntimeTravelsTierScaledDistance::RunTest(const FString& Parameters)
{
	using namespace MordecaiAttackCancelTest;
	const struct
	{
		float Stamina;
		float ExpectedDistance;
		const TCHAR* Tier;
	} Cases[] = {
		{ 100.f, 500.f, TEXT("Green") },
		{ 40.f, 400.f, TEXT("Yellow") },
		{ 20.f, 300.f, TEXT("Red") },
		{ 0.f, 200.f, TEXT("Exhausted") },
	};

	for (const auto& Case : Cases)
	{
		FRuntimeEnv Env;
		if (!TestTrue(*FString::Printf(TEXT("%s runtime world"), Case.Tier), Env.SetupRuntime()))
		{
			Env.TeardownRuntime();
			continue;
		}
		Env.SetStamina(Case.Stamina);
		Env.Press(MordecaiGameplayTags::Input_Dodge);
		const UMordecaiGA_Dodge* Dodge = Env.Instance<UMordecaiGA_Dodge>();
		if (!TestNotNull(*FString::Printf(TEXT("%s active Dodge instance"), Case.Tier), Dodge))
		{
			Env.TeardownRuntime();
			continue;
		}
		TestTrue(*FString::Printf(TEXT("%s runtime constant-force source is active"), Case.Tier),
			Env.Player->GetCharacterMovement()->CurrentRootMotion.HasActiveRootMotionSources());
		const float RuntimeDistance =
			Dodge->ComputeImpulseVelocity(Dodge->GetDodgeDirection()).Size2D()
			* Dodge->GetDodgeDurationSeconds();
		TestTrue(*FString::Printf(TEXT("%s runtime source travels %.0f +/-10%% cm (configured %.1f)"),
			Case.Tier, Case.ExpectedDistance, RuntimeDistance),
			FMath::IsNearlyEqual(
				RuntimeDistance,
				Case.ExpectedDistance,
				Case.ExpectedDistance * 0.1f));
		Env.TickSeconds(0.45f);
		TestEqual(*FString::Printf(TEXT("%s dodge finishes at zero horizontal speed"), Case.Tier),
			static_cast<float>(Env.Player->GetCharacterMovement()->Velocity.Size2D()), 0.f, 1.f);
		Env.TeardownRuntime();
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_Dodge_PerfectEvadeRefundsOnce,
	"Mordecai.Dodge.PerfectEvadeRefundsOnce",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_Dodge_PerfectEvadeRefundsOnce::RunTest(const FString& Parameters)
{
	using namespace MordecaiAttackCancelTest;
	FRuntimeEnv Env;
	ON_SCOPE_EXIT { Env.TeardownRuntime(); };
	if (!TestTrue("runtime world", Env.SetupRuntime()))
	{
		return true;
	}
	int32 PerfectEvents = 0;
	FGameplayEventMulticastDelegate& Delegate =
		Env.GetASC()->GenericGameplayEventCallbacks.FindOrAdd(
			MordecaiGameplayTags::Event_PerfectAction);
	const FDelegateHandle EventHandle = Delegate.AddLambda(
		[&PerfectEvents](const FGameplayEventData*) { ++PerfectEvents; });
	ON_SCOPE_EXIT { Delegate.Remove(EventHandle); };

	Env.Press(MordecaiGameplayTags::Input_Dodge);
	TestEqual("dodge paid 15 stamina", Env.Stamina(), 85.f);
	const FMordecaiHitDefenseResult First = FMordecaiHitDefense::ResolveIncomingHit(
		Env.GetASC(), nullptr, 20.f, Env.World->GetTimeSeconds());
	const FMordecaiHitDefenseResult Second = FMordecaiHitDefense::ResolveIncomingHit(
		Env.GetASC(), nullptr, 20.f, Env.World->GetTimeSeconds());
	TestEqual("both hits are evaded", First.Defense, EMordecaiHitDefense::Evaded);
	TestEqual("second hit is also evaded", Second.Defense, EMordecaiHitDefense::Evaded);
	TestEqual("one 10-stamina refund", Env.Stamina(), 95.f);
	TestEqual("one PerfectAction event", PerfectEvents, 1);
	TestTrue("successful evasion exposes PerfectDodge feedback tag",
		Env.GetASC()->HasMatchingGameplayTag(MordecaiGameplayTags::State_PerfectDodge));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_Dodge_OrdinaryEvadeDoesNotRefund,
	"Mordecai.Dodge.OrdinaryEvadeDoesNotRefund",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_Dodge_OrdinaryEvadeDoesNotRefund::RunTest(const FString& Parameters)
{
	using namespace MordecaiAttackCancelTest;
	FRuntimeEnv Env;
	ON_SCOPE_EXIT { Env.TeardownRuntime(); };
	if (!TestTrue("runtime world", Env.SetupRuntime()))
	{
		return true;
	}
	int32 PerfectEvents = 0;
	FGameplayEventMulticastDelegate& Delegate =
		Env.GetASC()->GenericGameplayEventCallbacks.FindOrAdd(
			MordecaiGameplayTags::Event_PerfectAction);
	const FDelegateHandle EventHandle = Delegate.AddLambda(
		[&PerfectEvents](const FGameplayEventData*) { ++PerfectEvents; });
	ON_SCOPE_EXIT { Delegate.Remove(EventHandle); };

	Env.Press(MordecaiGameplayTags::Input_Dodge);
	Env.TickSeconds(0.20f);
	const FMordecaiHitDefenseResult Result = FMordecaiHitDefense::ResolveIncomingHit(
		Env.GetASC(), nullptr, 20.f, Env.World->GetTimeSeconds());
	TestEqual("later hit is still evaded", Result.Defense, EMordecaiHitDefense::Evaded);
	TestEqual("ordinary evade receives no refund", Env.Stamina(), 85.f);
	TestEqual("ordinary evade sends no PerfectAction", PerfectEvents, 0);
	TestFalse("ordinary evade produces no PerfectDodge feedback tag",
		Env.GetASC()->HasMatchingGameplayTag(MordecaiGameplayTags::State_PerfectDodge));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_Combo_WeaponSwapPinsStepAndResetsChain,
	"Mordecai.Combo.WeaponSwapPinsStepAndResetsChain",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_Combo_WeaponSwapPinsStepAndResetsChain::RunTest(const FString& Parameters)
{
	using namespace MordecaiAttackCancelTest;
	FRuntimeEnv Env;
	ON_SCOPE_EXIT { Env.TeardownRuntime(); };
	if (!TestTrue("runtime world", Env.SetupRuntime()))
	{
		return true;
	}
	UMordecaiEquipmentComponent* Equipment =
		Env.Player->FindComponentByClass<UMordecaiEquipmentComponent>();
	if (!TestNotNull("real player equipment component", Equipment))
	{
		return true;
	}
	Equipment->AddAvailableWeapon(
		UMordecaiWeaponProfileFactory::CreateDagger(GetTransientPackage()));
	Equipment->AddAvailableWeapon(
		UMordecaiWeaponProfileFactory::CreateGreatsword(GetTransientPackage()));
	Equipment->CycleNextWeapon();

	UMordecaiGA_MeleeAttack* Melee = Env.Instance<UMordecaiGA_MeleeAttack>();
	if (!TestNotNull("granted melee instance", Melee))
	{
		return true;
	}
	Melee->SetEquipmentComponentOverride(Equipment);
	Env.Press(MordecaiGameplayTags::Input_LightAttack);
	const UMordecaiAttackProfileDataAsset* PinnedDaggerStep = Melee->GetActiveProfile();
	TestNotNull("Dagger step is active", PinnedDaggerStep);
	Equipment->CycleNextWeapon();
	TestEqual("weapon swap does not replace the in-flight profile",
		Melee->GetActiveProfile(), PinnedDaggerStep);

	Env.TickSeconds(0.22f);
	Env.Press(MordecaiGameplayTags::Input_LightAttack);
	Env.TickSeconds(0.35f);
	TestFalse("queued continuation was cleared and attack ended",
		Env.Active<UMordecaiGA_MeleeAttack>());
	TestFalse("Attacking cannot stick after the swap",
		Env.GetASC()->HasMatchingGameplayTag(MordecaiGameplayTags::State_Attacking));

	Env.Release(MordecaiGameplayTags::Input_LightAttack);
	Env.Press(MordecaiGameplayTags::Input_LightAttack);
	TestTrue("new weapon starts a fresh attack", Env.Active<UMordecaiGA_MeleeAttack>());
	TestEqual("new weapon starts at combo step 1", Melee->GetCurrentComboIndex(), 0);
	TestTrue("new weapon's profile replaces the old profile only on the new attack",
		Melee->GetActiveProfile() != PinnedDaggerStep);
	return true;
}
