// Project Mordecai — US-125 attack cancellation, dodge travel/reward, and combo-swap safety.

#include "Misc/AutomationTest.h"
#include "Misc/ScopeExit.h"

#include "AbilitySystem/LyraAbilitySystemComponent.h"
#include "Abilities/Tasks/AbilityTask_ApplyRootMotionConstantForce.h"
#include "Components/BoxComponent.h"
#include "EnhancedInputComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/RootMotionSource.h"
#include "GameplayAbilitySpec.h"
#include "InputAction.h"
#include "Mordecai/AbilitySystem/MordecaiAttributeSet.h"
#include "Mordecai/Combat/MordecaiGA_Block.h"
#include "Mordecai/Combat/MordecaiGA_Dodge.h"
#include "Mordecai/Combat/MordecaiGA_MeleeAttack.h"
#include "Mordecai/Combat/MordecaiGA_Parry.h"
#include "Mordecai/Combat/MordecaiHitDefense.h"
#include "Mordecai/MordecaiGameplayTags.h"
#include "Mordecai/MordecaiHeroComponent.h"
#include "Mordecai/MordecaiPlayerController.h"
#include "Mordecai/Weapons/MordecaiEquipmentComponent.h"
#include "Mordecai/Weapons/MordecaiWeaponProfileFactory.h"
#include "Tests/Recovery/MordecaiRecoveryTestEnv.h"
#include "UObject/SoftObjectPath.h"

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
			Player->GetCharacterMovement()->bRunPhysicsWithNoController = true;
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

		bool WaitForPhase(EMordecaiAttackPhase Phase, int32 MaxFrames = 100) const
		{
			for (int32 Frame = 0; Frame < MaxFrames; ++Frame)
			{
				const UMordecaiGA_MeleeAttack* Melee = Instance<UMordecaiGA_MeleeAttack>();
				if (Melee && Melee->GetCurrentPhase() == Phase)
				{
					return true;
				}
				TickFrames(1);
			}
			return false;
		}

		bool AdvanceToComboStep(int32 ZeroBasedStep) const
		{
			UMordecaiGA_MeleeAttack* Melee = Instance<UMordecaiGA_MeleeAttack>();
			if (!Melee)
			{
				return false;
			}
			if (!Melee->IsActive())
			{
				Press(MordecaiGameplayTags::Input_LightAttack);
			}
			while (Melee->GetCurrentComboIndex() < ZeroBasedStep)
			{
				if (!WaitForPhase(EMordecaiAttackPhase::Recovery))
				{
					return false;
				}
				Melee->NotifyComboInput();
				const int32 Previous = Melee->GetCurrentComboIndex();
				for (int32 Frame = 0; Frame < 100
					&& Melee->GetCurrentComboIndex() == Previous; ++Frame)
				{
					TickFrames(1);
				}
				if (Melee->GetCurrentComboIndex() == Previous)
				{
					return false;
				}
			}
			return true;
		}

		UMordecaiEquipmentComponent* EquipDaggerAndGreatsword()
		{
			Equipment = Player->FindComponentByClass<UMordecaiEquipmentComponent>();
			if (!Equipment)
			{
				return nullptr;
			}
			Equipment->AddAvailableWeapon(
				UMordecaiWeaponProfileFactory::CreateDagger(GetTransientPackage()));
			Equipment->AddAvailableWeapon(
				UMordecaiWeaponProfileFactory::CreateGreatsword(GetTransientPackage()));
			Equipment->CycleNextWeapon();
			if (UMordecaiGA_MeleeAttack* Melee = Instance<UMordecaiGA_MeleeAttack>())
			{
				Melee->SetEquipmentComponentOverride(Equipment);
			}
			return Equipment;
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

	static UInputAction* LoadInputAction(const TCHAR* Name)
	{
		return Cast<UInputAction>(FSoftObjectPath(FString::Printf(
			TEXT("/MordecaiCore/Input/Actions/%s.%s"), Name, Name)).TryLoad());
	}

	static bool ExecuteBinding(
		const UEnhancedInputComponent* EIC,
		const UInputAction* Action,
		ETriggerEvent Event,
		const UObject* BoundTo)
	{
		if (!EIC || !Action)
		{
			return false;
		}
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
	Env.Press(MordecaiGameplayTags::Input_LightAttack);
	TestTrue("follow-up was queued before cancel", Env.GetASC()->HasMatchingGameplayTag(
		MordecaiGameplayTags::State_ComboWindow));
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
	Env.TickSeconds(1.f);
	TestFalse("queued follow-up never hits or starts after cancel",
		Env.Active<UMordecaiGA_MeleeAttack>());
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
	for (const EMordecaiAttackPhase Phase :
		{ EMordecaiAttackPhase::Windup, EMordecaiAttackPhase::Recovery })
	{
		FRuntimeEnv Env;
		if (!TestTrue("runtime world", Env.SetupRuntime()))
		{
			Env.TeardownRuntime();
			continue;
		}
		Env.Press(MordecaiGameplayTags::Input_LightAttack);
		if (Phase == EMordecaiAttackPhase::Recovery)
		{
			TestTrue("attack reaches Recovery", Env.WaitForPhase(Phase));
		}
		Env.Press(MordecaiGameplayTags::Input_Parry);
		TestTrue("melee remains active", Env.Active<UMordecaiGA_MeleeAttack>());
		TestFalse("parry cannot overlap or cancel an attack", Env.Active<UMordecaiGA_Parry>());
		Env.TeardownRuntime();
	}
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
		const FRotator InitialFacing(0.f, 37.f, 0.f);
		Env.Player->SetActorRotation(InitialFacing);
		const FVector Start = Env.Player->GetActorLocation();
		Env.Press(MordecaiGameplayTags::Input_Dodge);
		const UMordecaiGA_Dodge* Dodge = Env.Instance<UMordecaiGA_Dodge>();
		if (!TestNotNull(*FString::Printf(TEXT("%s active Dodge instance"), Case.Tier), Dodge))
		{
			Env.TeardownRuntime();
			continue;
		}
		TestTrue(*FString::Printf(TEXT("%s runtime constant-force source is active"), Case.Tier),
			Env.Player->GetCharacterMovement()->CurrentRootMotion.HasActiveRootMotionSources());
		const TSharedPtr<FRootMotionSource> RootMotion =
			Env.Player->GetCharacterMovement()->CurrentRootMotion.GetRootMotionSource(
				TEXT("MordecaiDodge"));
		TestTrue(*FString::Printf(TEXT("%s dodge leaves gravity enabled"), Case.Tier),
			RootMotion
				&& RootMotion->Settings.HasFlag(
					ERootMotionSourceSettingsFlags::IgnoreZAccumulate));
		Env.TickSeconds(0.45f);
		if (UAbilityTask_ApplyRootMotionConstantForce* Task =
			Dodge->GetRootMotionTask())
		{
			// This headless world never begins play, so tick the duration-owning
			// task explicitly after advancing its world clock.
			Task->TickTask(0.f);
		}
		const float RuntimeDistance =
			FVector::Dist2D(Start, Env.Player->GetActorLocation());
		TestTrue(*FString::Printf(TEXT("%s measured runtime travel %.1f cm is %.0f +/-10%% cm"),
			Case.Tier, RuntimeDistance, Case.ExpectedDistance),
			FMath::IsNearlyEqual(
				RuntimeDistance,
				Case.ExpectedDistance,
				Case.ExpectedDistance * 0.1f));
		TestEqual(*FString::Printf(TEXT("%s dodge finishes at zero horizontal speed"), Case.Tier),
			static_cast<float>(Env.Player->GetCharacterMovement()->Velocity.Size2D()), 0.f, 1.f);
		TestFalse(*FString::Printf(TEXT("%s duration task closes i-frames"), Case.Tier),
			Env.GetASC()->HasMatchingGameplayTag(MordecaiGameplayTags::State_Dodging));
		TestEqual(*FString::Printf(TEXT("%s dodge preserves twin-stick facing"), Case.Tier),
			static_cast<float>(Env.Player->GetActorRotation().Yaw),
			static_cast<float>(InitialFacing.Yaw),
			0.1f);
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
	TestEqual("one owning-client feedback RPC",
		Env.Player->GetPerfectDodgeFeedbackCount(), 1);
	TestFalse("perfect feedback does not rely on a loose gameplay tag",
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
	TestEqual("ordinary evade sends no owning-client feedback RPC",
		Env.Player->GetPerfectDodgeFeedbackCount(), 0);
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

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_AttackCancel_DefenseActivationArbitratesBeforeCancel,
	"Mordecai.AttackCancel.DefenseActivationArbitratesBeforeCancel",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_AttackCancel_DefenseActivationArbitratesBeforeCancel::RunTest(
	const FString& Parameters)
{
	using namespace MordecaiAttackCancelTest;

	// A Dodge that fails its own cooldown check must not consume a valid Recovery attack.
	{
		FRuntimeEnv Env;
		if (TestTrue("dodge runtime world", Env.SetupRuntime()))
		{
			UMordecaiHeroComponent* Hero =
				Env.Player->FindComponentByClass<UMordecaiHeroComponent>();
			UEnhancedInputComponent* EIC =
				NewObject<UEnhancedInputComponent>(Env.Player);
			Hero->SetAbilitySystemOverride(Env.GetASC());
			Hero->BuildAndBindRuntimeInput(EIC);
			UInputAction* Dodge = LoadInputAction(TEXT("IA_Mordecai_Dodge"));
			Env.EnterRecovery();
			Env.GetASC()->AddLooseGameplayTag(
				MordecaiGameplayTags::State_DodgeCooldown);
			TestTrue("Dodge binding executes",
				ExecuteBinding(EIC, Dodge, ETriggerEvent::Triggered, Hero));
			Env.GetASC()->ProcessAbilityInput(0.f, false);
			TestTrue("cooldown-rejected Dodge leaves Recovery attack running",
				Env.Active<UMordecaiGA_MeleeAttack>());
			TestFalse("Dodge remains rejected", Env.Active<UMordecaiGA_Dodge>());
			Env.GetASC()->RemoveLooseGameplayTag(
				MordecaiGameplayTags::State_DodgeCooldown);
		}
		Env.TeardownRuntime();
	}

	// A held Block rejected during a committed phase stays suppressed until release,
	// then a fresh press in Recovery is allowed to cancel.
	{
		FRuntimeEnv Env;
		if (TestTrue("block runtime world", Env.SetupRuntime()))
		{
			UMordecaiHeroComponent* Hero =
				Env.Player->FindComponentByClass<UMordecaiHeroComponent>();
			UEnhancedInputComponent* EIC =
				NewObject<UEnhancedInputComponent>(Env.Player);
			Hero->SetAbilitySystemOverride(Env.GetASC());
			Hero->BuildAndBindRuntimeInput(EIC);
			UInputAction* Block = LoadInputAction(TEXT("IA_Mordecai_Block"));
			Env.Press(MordecaiGameplayTags::Input_LightAttack);
			TestTrue("first held Block trigger executes",
				ExecuteBinding(EIC, Block, ETriggerEvent::Triggered, Hero));
			TestTrue("repeat held Block trigger executes",
				ExecuteBinding(EIC, Block, ETriggerEvent::Triggered, Hero));
			Env.GetASC()->ProcessAbilityInput(0.f, false);
			TestFalse("committed held Block is rejected", Env.Active<UMordecaiGA_Block>());
			TestTrue("attack survives held Block", Env.Active<UMordecaiGA_MeleeAttack>());
			TestTrue("attack reaches Recovery while LT remains held",
				Env.WaitForPhase(EMordecaiAttackPhase::Recovery));
			TestTrue("suppressed held LT still executes its binding",
				ExecuteBinding(EIC, Block, ETriggerEvent::Triggered, Hero));
			Env.GetASC()->ProcessAbilityInput(0.f, false);
			TestFalse("suppressed held LT does not retry in Recovery",
				Env.Active<UMordecaiGA_Block>());
			TestTrue("release binding executes",
				ExecuteBinding(EIC, Block, ETriggerEvent::Completed, Hero));
			Env.GetASC()->ProcessAbilityInput(0.f, false);
			TestTrue("fresh Block binding executes",
				ExecuteBinding(EIC, Block, ETriggerEvent::Triggered, Hero));
			Env.GetASC()->ProcessAbilityInput(0.f, false);
			TestTrue("fresh Block activates", Env.Active<UMordecaiGA_Block>());
			TestFalse("fresh Block cancels Recovery",
				Env.Active<UMordecaiGA_MeleeAttack>());
		}
		Env.TeardownRuntime();
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_AttackCancel_MeleeCannotStartDuringDefense,
	"Mordecai.AttackCancel.MeleeCannotStartDuringDefense",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_AttackCancel_MeleeCannotStartDuringDefense::RunTest(
	const FString& Parameters)
{
	using namespace MordecaiAttackCancelTest;
	FRuntimeEnv Env;
	ON_SCOPE_EXIT { Env.TeardownRuntime(); };
	if (!TestTrue("runtime world", Env.SetupRuntime()))
	{
		return true;
	}
	UMordecaiGA_MeleeAttack* Melee = Env.Instance<UMordecaiGA_MeleeAttack>();
	TestTrue("Melee blocks while Blocking",
		Melee && Melee->HasActivationBlockedTag(MordecaiGameplayTags::State_Blocking));
	TestTrue("Melee blocks while Dodging",
		Melee && Melee->HasActivationBlockedTag(MordecaiGameplayTags::State_Dodging));
	for (const FGameplayTag State : {
		MordecaiGameplayTags::State_Blocking.GetTag(),
		MordecaiGameplayTags::State_Dodging.GetTag() })
	{
		Env.GetASC()->AddLooseGameplayTag(State);
		Env.Press(MordecaiGameplayTags::Input_LightAttack);
		TestFalse(*FString::Printf(TEXT("melee cannot start during %s"), *State.ToString()),
			Env.Active<UMordecaiGA_MeleeAttack>());
		Env.Release(MordecaiGameplayTags::Input_LightAttack);
		Env.GetASC()->RemoveLooseGameplayTag(State);
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_Combo_CancelledFinisherResetsToStepOne,
	"Mordecai.Combo.CancelledFinisherResetsToStepOne",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_Combo_CancelledFinisherResetsToStepOne::RunTest(
	const FString& Parameters)
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
	Equipment->AddAvailableWeapon(
		UMordecaiWeaponProfileFactory::CreateLongsword(GetTransientPackage()));
	Equipment->CycleNextWeapon();
	UMordecaiGA_MeleeAttack* Melee = Env.Instance<UMordecaiGA_MeleeAttack>();
	Melee->SetEquipmentComponentOverride(Equipment);
	TestTrue("Longsword reaches L3", Env.AdvanceToComboStep(2));
	TestTrue("L3 reaches Recovery", Env.WaitForPhase(EMordecaiAttackPhase::Recovery));
	Env.Press(MordecaiGameplayTags::Input_Dodge);
	TestFalse("Dodge cancels L3 Recovery", Env.Active<UMordecaiGA_MeleeAttack>());
	Env.TickSeconds(1.f);
	Env.Release(MordecaiGameplayTags::Input_LightAttack);
	Env.Press(MordecaiGameplayTags::Input_LightAttack);
	TestTrue("next RT starts an attack", Env.Active<UMordecaiGA_MeleeAttack>());
	TestEqual("next RT starts L1, not stale L3", Melee->GetCurrentComboIndex(), 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_Combo_DaggerCancelSwapStartsGreatswordStepOne,
	"Mordecai.Combo.DaggerCancelSwapStartsGreatswordStepOne",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_Combo_DaggerCancelSwapStartsGreatswordStepOne::RunTest(
	const FString& Parameters)
{
	using namespace MordecaiAttackCancelTest;
	FRuntimeEnv Env;
	ON_SCOPE_EXIT { Env.TeardownRuntime(); };
	if (!TestTrue("runtime world", Env.SetupRuntime())
		|| !TestNotNull("Dagger/Greatsword equipment", Env.EquipDaggerAndGreatsword()))
	{
		return true;
	}
	UMordecaiGA_MeleeAttack* Melee = Env.Instance<UMordecaiGA_MeleeAttack>();
	TestTrue("Dagger reaches L4", Env.AdvanceToComboStep(3));
	TestTrue("Dagger L4 reaches Recovery",
		Env.WaitForPhase(EMordecaiAttackPhase::Recovery));
	Env.Press(MordecaiGameplayTags::Input_Dodge);
	TestFalse("Dodge cancels Dagger L4 Recovery",
		Env.Active<UMordecaiGA_MeleeAttack>());
	Env.Equipment->CycleNextWeapon();
	Env.TickSeconds(1.f);
	Env.Release(MordecaiGameplayTags::Input_LightAttack);
	Env.Press(MordecaiGameplayTags::Input_LightAttack);
	TestTrue("Greatsword RT works after Dagger cancel",
		Env.Active<UMordecaiGA_MeleeAttack>());
	TestEqual("Greatsword starts at L1", Melee->GetCurrentComboIndex(), 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_Combo_HighDaggerSwapToShorterChainEveryPhase,
	"Mordecai.Combo.HighDaggerSwapToShorterChainEveryPhase",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_Combo_HighDaggerSwapToShorterChainEveryPhase::RunTest(
	const FString& Parameters)
{
	using namespace MordecaiAttackCancelTest;
	for (const EMordecaiAttackPhase Phase : {
		EMordecaiAttackPhase::Windup,
		EMordecaiAttackPhase::Active,
		EMordecaiAttackPhase::Recovery })
	{
		FRuntimeEnv Env;
		if (!TestTrue("runtime world", Env.SetupRuntime())
			|| !TestNotNull("Dagger/Greatsword equipment",
				Env.EquipDaggerAndGreatsword()))
		{
			Env.TeardownRuntime();
			continue;
		}
		UMordecaiGA_MeleeAttack* Melee =
			Env.Instance<UMordecaiGA_MeleeAttack>();
		TestTrue("Dagger reaches step 4", Env.AdvanceToComboStep(3));
		TestTrue(*FString::Printf(TEXT("Dagger L4 reaches phase %d"),
			static_cast<int32>(Phase)), Env.WaitForPhase(Phase));
		Env.Equipment->CycleNextWeapon();
		Env.TickSeconds(2.f);
		TestFalse("swapped chain ends within a bound",
			Env.Active<UMordecaiGA_MeleeAttack>());
		Env.Release(MordecaiGameplayTags::Input_LightAttack);
		Env.Press(MordecaiGameplayTags::Input_LightAttack);
		TestTrue("shorter weapon remains usable",
			Env.Active<UMordecaiGA_MeleeAttack>());
		TestEqual("shorter weapon starts step 1",
			Melee->GetCurrentComboIndex(), 0);
		Env.TeardownRuntime();
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_AttackCancel_PostureBreakClearsDefenseTransitions,
	"Mordecai.AttackCancel.PostureBreakClearsDefenseTransitions",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_AttackCancel_PostureBreakClearsDefenseTransitions::RunTest(
	const FString& Parameters)
{
	using namespace MordecaiAttackCancelTest;
	for (const bool bBreakDuringDodge : { true, false })
	{
		FRuntimeEnv Env;
		if (!TestTrue("runtime world", Env.SetupRuntime()))
		{
			Env.TeardownRuntime();
			continue;
		}
		if (bBreakDuringDodge)
		{
			Env.Press(MordecaiGameplayTags::Input_Dodge);
			TestTrue("root-motion Dodge is active",
				Env.Active<UMordecaiGA_Dodge>());
		}
		else
		{
			Env.EnterRecovery();
			Env.Press(MordecaiGameplayTags::Input_Dodge);
			TestFalse("Recovery attack was cancelled",
				Env.Active<UMordecaiGA_MeleeAttack>());
		}
		Env.GetASC()->SetNumericAttributeBase(
			UMordecaiAttributeSet::GetPostureAttribute(), 0.f);
		Env.TickFrames(1);
		TestTrue("posture break state starts", Env.GetASC()->HasMatchingGameplayTag(
			MordecaiGameplayTags::State_PostureBroken));
		TestFalse("posture break cancels Dodge", Env.Active<UMordecaiGA_Dodge>());
		TestFalse("posture break leaves no melee",
			Env.Active<UMordecaiGA_MeleeAttack>());
		TestFalse("posture break removes Dodging", Env.GetASC()->HasMatchingGameplayTag(
			MordecaiGameplayTags::State_Dodging));
		Env.TeardownRuntime();
	}
	return true;
}
