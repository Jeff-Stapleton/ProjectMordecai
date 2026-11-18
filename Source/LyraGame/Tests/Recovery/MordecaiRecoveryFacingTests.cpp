// Project Mordecai — aim-driven player facing (US-098 / issue #34)

#include "Misc/AutomationTest.h"
#include "Misc/ConfigCacheIni.h"
#include "Misc/ScopeExit.h"
#include "AbilitySystem/LyraAbilitySystemComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/Widget.h"
#include "EnhancedInputComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/InputSettings.h"
#include "HAL/IConsoleManager.h"
#include "InputActionValue.h"
#include "InputMappingContext.h"
#include "InputModifiers.h"
#include "Mordecai/Camera/MordecaiCameraMode_Diorama.h"
#include "Mordecai/Combat/MordecaiGA_MeleeAttack.h"
#include "Mordecai/Combat/MordecaiGA_Dodge.h"
#include "Mordecai/Combat/MordecaiHitDetectionSubsystem.h"
#include "Mordecai/MordecaiCharacter.h"
#include "Mordecai/MordecaiGameplayTags.h"
#include "Mordecai/MordecaiHeroComponent.h"
#include "Mordecai/UI/MordecaiCombatHUDWidget.h"
#include "Tests/Recovery/MordecaiRecoveryTestEnv.h"

namespace MordecaiRecoveryFacingTest
{
	static bool InvokeMouseAim(
		FAutomationTestBase& Test,
		UMordecaiHeroComponent* Hero,
		const FVector& RayOrigin,
		const FVector& RayDirection)
	{
		UFunction* Function = Hero
			? Hero->FindFunction(TEXT("ApplyMouseAimFromRay"))
			: nullptr;
		if (!Test.TestNotNull(
			TEXT("Hero exposes the cursor-ray projection used by mouse aim"),
			Function))
		{
			return false;
		}

		struct FParams
		{
			FVector WorldOrigin;
			FVector WorldDirection;
			bool ReturnValue = false;
		};
		FParams Params{RayOrigin, RayDirection};
		Hero->ProcessEvent(Function, &Params);
		return Params.ReturnValue;
	}

	static bool InvokeStickAim(
		FAutomationTestBase& Test,
		UMordecaiHeroComponent* Hero,
		const FVector2D& StickInput)
	{
		UFunction* Function = Hero
			? Hero->FindFunction(TEXT("ApplyGamepadAim"))
			: nullptr;
		if (!Test.TestNotNull(
			TEXT("Hero exposes the deadzone-aware right-stick aim path"),
			Function))
		{
			return false;
		}

		struct FParams
		{
			FVector2D StickInput;
			bool ReturnValue = false;
		};
		FParams Params{StickInput};
		Hero->ProcessEvent(Function, &Params);
		return Params.ReturnValue;
	}

	static bool InvokeInputAction(
		FAutomationTestBase& Test,
		UMordecaiHeroComponent* Hero,
		const TCHAR* FunctionName,
		const FInputActionValue& Value)
	{
		UFunction* Function = Hero ? Hero->FindFunction(FunctionName) : nullptr;
		if (!Test.TestNotNull(
			*FString::Printf(TEXT("Hero exposes the real %s input handler"), FunctionName),
			Function))
		{
			return false;
		}

		struct FParams
		{
			FInputActionValue InputActionValue;
		};
		FParams Params{Value};
		Hero->ProcessEvent(Function, &Params);
		return true;
	}

	static bool SetupControlledAimEnv(
		FAutomationTestBase& Test,
		MordecaiRecoveryTest::FRecoveryControlledTestEnv& Env)
	{
		if (!Test.TestTrue(TEXT("controlled player world"), Env.SetupWithController()))
		{
			return false;
		}
		Env.PC->Possess(Env.Player);
		return Test.TestTrue(
			TEXT("the real look handlers resolve the possessing controller"),
			Env.Player->GetController() == Env.PC);
	}

	static UInputModifierDeadZone* FindDeadZone(
		const UInputMappingContext* Context,
		const FKey& Key)
	{
		if (!Context)
		{
			return nullptr;
		}
		for (const FEnhancedActionKeyMapping& Mapping : Context->GetMappings())
		{
			if (Mapping.Key == Key)
			{
				for (UInputModifier* Modifier : Mapping.Modifiers)
				{
					if (UInputModifierDeadZone* DeadZone =
						Cast<UInputModifierDeadZone>(Modifier))
					{
						return DeadZone;
					}
				}
			}
		}
		return nullptr;
	}

	static int32 CountDeadZones(
		const UInputMappingContext* Context,
		const FKey& Key)
	{
		int32 Count = 0;
		for (const FEnhancedActionKeyMapping& Mapping :
			Context ? Context->GetMappings() : TArray<FEnhancedActionKeyMapping>())
		{
			if (Mapping.Key == Key)
			{
				for (const UInputModifier* Modifier : Mapping.Modifiers)
				{
					Count += IsValid(Modifier)
						&& Modifier->IsA<UInputModifierDeadZone>();
				}
			}
		}
		return Count;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FMordecai_Recovery_Facing_FollowsMouseAim,
	"Mordecai.Recovery.Facing.FollowsMouseAim",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_Recovery_Facing_FollowsMouseAim::RunTest(const FString& Parameters)
{
	using namespace MordecaiRecoveryFacingTest;

	AMordecaiCharacter* Character = NewObject<AMordecaiCharacter>();
	UMordecaiHeroComponent* Hero =
		Character->FindComponentByClass<UMordecaiHeroComponent>();
	if (!TestNotNull(TEXT("Character owns its Mordecai hero component"), Hero))
	{
		return true;
	}

	// A vertical ray through world +Y intersects the character-height plane at +Y.
	const bool bApplied = InvokeMouseAim(
		*this,
		Hero,
		FVector(0.f, 500.f, 1000.f),
		FVector(0.f, 0.f, -1.f));
	TestTrue(TEXT("The valid cursor ray applies an aim direction"), bApplied);
	TestTrue(
		TEXT("A cursor projected at +Y turns the character to yaw 90"),
		FMath::IsNearlyEqual(Character->GetActorRotation().Yaw, 90.f));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FMordecai_Recovery_Facing_MouseDeadRadiusPreventsJitter,
	"Mordecai.Recovery.Facing.MouseDeadRadiusPreventsJitter",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_Recovery_Facing_MouseDeadRadiusPreventsJitter::RunTest(
	const FString& Parameters)
{
	using namespace MordecaiRecoveryFacingTest;

	AMordecaiCharacter* Character = NewObject<AMordecaiCharacter>();
	UMordecaiHeroComponent* Hero =
		Character->FindComponentByClass<UMordecaiHeroComponent>();
	Character->SetActorRotation(FRotator(0.f, 45.f, 0.f));

	const bool bApplied = InvokeMouseAim(
		*this,
		Hero,
		FVector(0.f, 20.f, 1000.f),
		FVector(0.f, 0.f, -1.f));
	TestFalse(
		TEXT("A cursor inside the 40 cm capsule-radius dead zone does not jitter facing"),
		bApplied);
	TestTrue(
		TEXT("Rejected near-character mouse aim preserves the last facing"),
		FMath::IsNearlyEqual(Character->GetActorRotation().Yaw, 45.f));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FMordecai_Recovery_Facing_FollowsStickAim,
	"Mordecai.Recovery.Facing.FollowsStickAim",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_Recovery_Facing_FollowsStickAim::RunTest(const FString& Parameters)
{
	using namespace MordecaiRecoveryFacingTest;

	AMordecaiCharacter* Character = NewObject<AMordecaiCharacter>();
	UMordecaiHeroComponent* Hero =
		Character->FindComponentByClass<UMordecaiHeroComponent>();
	if (!TestNotNull(TEXT("Character owns its Mordecai hero component"), Hero))
	{
		return true;
	}

	TestTrue(
		TEXT("A full right-stick-left input applies facing"),
		InvokeStickAim(*this, Hero, FVector2D(-1.f, 0.f)));
	TestTrue(
		TEXT("Right-stick left maps to world -Y / yaw -90"),
		FMath::IsNearlyEqual(Character->GetActorRotation().Yaw, -90.f));

	// ApplyGamepadAim receives the already-deadzoned mapping value. Releasing the stick emits no
	// new direction, while AimDeadzoneAppliedOnce exercises the real mapping's 0.2 threshold.
	TestFalse(
		TEXT("A released stick does not replace the last facing"),
		InvokeStickAim(*this, Hero, FVector2D::ZeroVector));
	TestTrue(
		TEXT("Stick release preserves yaw -90"),
		FMath::IsNearlyEqual(Character->GetActorRotation().Yaw, -90.f));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FMordecai_Recovery_Facing_AimDeadzoneAppliedOnce,
	"Mordecai.Recovery.Facing.AimDeadzoneAppliedOnce",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_Recovery_Facing_AimDeadzoneAppliedOnce::RunTest(
	const FString& Parameters)
{
	using namespace MordecaiRecoveryFacingTest;
	using namespace MordecaiRecoveryTest;

	FRecoveryControlledTestEnv Env;
	ON_SCOPE_EXIT { Env.TeardownWithController(); };
	if (!SetupControlledAimEnv(*this, Env))
	{
		return true;
	}

	UMordecaiHeroComponent* Hero =
		Env.Player->FindComponentByClass<UMordecaiHeroComponent>();
	UEnhancedInputComponent* EIC = NewObject<UEnhancedInputComponent>(Env.PC);
	const UInputMappingContext* Context = Hero
		? Hero->BuildAndBindRuntimeInput(EIC)
		: nullptr;
	if (!TestNotNull(TEXT("real runtime mapping context"), Context))
	{
		return true;
	}

	UInputModifierDeadZone* AimDeadZone =
		FindDeadZone(Context, EKeys::Gamepad_Right2D);
	UInputModifierDeadZone* MoveDeadZone =
		FindDeadZone(Context, EKeys::Gamepad_Left2D);
	if (!TestNotNull(TEXT("right-stick mapping owns its one deadzone"), AimDeadZone)
		|| !TestNotNull(TEXT("left-stick mapping owns an independent deadzone"), MoveDeadZone))
	{
		return true;
	}
	TestEqual(
		TEXT("Aim mapping applies the 0.2 deadzone once"),
		AimDeadZone->LowerThreshold,
		0.2f);
	TestEqual(
		TEXT("Movement keeps its own 0.25 deadzone"),
		MoveDeadZone->LowerThreshold,
		0.25f);
	TestEqual(
		TEXT("The real right-stick mapping has exactly one deadzone modifier"),
		CountDeadZones(Context, EKeys::Gamepad_Right2D),
		1);
	const IConsoleVariable* GlobalAxisConfigMode =
		IConsoleManager::Get().FindConsoleVariable(
			TEXT("input.GlobalAxisConfigMode"));
	if (TestNotNull(
		TEXT("UE 5.7 exposes the legacy-axis compatibility mode"),
		GlobalAxisConfigMode))
	{
		TestEqual(
			TEXT("UE 5.7 defaults legacy AxisConfig injection to mouse keys only"),
			GlobalAxisConfigMode->GetInt(),
			0);
	}

	const FInputActionValue ModifiedValue = AimDeadZone->ModifyRaw(
		nullptr,
		FInputActionValue(FVector2D(0.25f, 0.f)),
		0.f);
	const FVector2D MappedStick = ModifiedValue.Get<FVector2D>();
	TestTrue(
		TEXT("A raw 0.25 aim survives the real 0.2 mapping deadzone"),
		MappedStick.SizeSquared() > UE_KINDA_SMALL_NUMBER);
	Env.Player->SetActorRotation(FRotator::ZeroRotator);
	InvokeInputAction(
		*this,
		Hero,
		TEXT("Input_LookStick"),
		FInputActionValue(MappedStick));
	TestTrue(
		TEXT("The mapped 0.25 value is not rejected by a second gameplay deadzone"),
		FMath::IsNearlyEqual(Env.Player->GetActorRotation().Yaw, 90.f, 0.1f));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FMordecai_Recovery_Facing_AimRPCThreshold,
	"Mordecai.Recovery.Facing.AimRPCThreshold",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_Recovery_Facing_AimRPCThreshold::RunTest(
	const FString& Parameters)
{
	TestFalse(
		TEXT("A sub-degree yaw change does not send another unreliable RPC"),
		AMordecaiCharacter::ShouldSendAimFacingRPC(10.f, 10.9f));
	TestFalse(
		TEXT("Exactly one degree remains below the send threshold"),
		AMordecaiCharacter::ShouldSendAimFacingRPC(10.f, 11.f));
	TestTrue(
		TEXT("A yaw change over one degree sends an update"),
		AMordecaiCharacter::ShouldSendAimFacingRPC(10.f, 11.01f));
	TestTrue(
		TEXT("The threshold uses wrapped yaw delta"),
		AMordecaiCharacter::ShouldSendAimFacingRPC(179.f, -179.f));
	return true;
}

IMPLEMENT_COMPLEX_AUTOMATION_TEST(
	FMordecai_Recovery_Facing_BlockedWhileIncapacitated,
	"Mordecai.Recovery.Facing.BlockedWhileIncapacitated",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

void FMordecai_Recovery_Facing_BlockedWhileIncapacitated::GetTests(
	TArray<FString>& OutBeautifiedNames,
	TArray<FString>& OutTestCommands) const
{
	const TCHAR* Cases[] = {
		TEXT("ControllerLookIgnored"),
		TEXT("Dead"),
		TEXT("MicroStunned"),
		TEXT("PostureBroken"),
		TEXT("KnockedDown"),
		TEXT("Dodging"),
		TEXT("Parrying"),
		TEXT("AttackCommitted"),
	};
	for (const TCHAR* Case : Cases)
	{
		OutBeautifiedNames.Add(Case);
		OutTestCommands.Add(Case);
	}
}

bool FMordecai_Recovery_Facing_BlockedWhileIncapacitated::RunTest(
	const FString& Parameters)
{
	using namespace MordecaiRecoveryFacingTest;
	using namespace MordecaiRecoveryTest;

	FRecoveryControlledTestEnv Env;
	ON_SCOPE_EXIT { Env.TeardownWithController(); };
	if (!SetupControlledAimEnv(*this, Env))
	{
		return true;
	}

	UAbilitySystemComponent* ASC = Env.GetPlayerASC();
	UMordecaiHeroComponent* Hero =
		Env.Player->FindComponentByClass<UMordecaiHeroComponent>();
	if (!TestNotNull(TEXT("player ASC"), ASC)
		|| !TestNotNull(TEXT("Mordecai hero component"), Hero))
	{
		return true;
	}

	FGameplayTag GateTag;
	if (Parameters == TEXT("ControllerLookIgnored"))
	{
		Env.PC->SetIgnoreLookInput(true);
	}
	else if (Parameters == TEXT("Dead"))
	{
		GateTag = MordecaiGameplayTags::State_Dead;
	}
	else if (Parameters == TEXT("MicroStunned"))
	{
		GateTag = MordecaiGameplayTags::Status_MicroStunned;
	}
	else if (Parameters == TEXT("PostureBroken"))
	{
		GateTag = MordecaiGameplayTags::State_PostureBroken;
	}
	else if (Parameters == TEXT("KnockedDown"))
	{
		GateTag = MordecaiGameplayTags::State_KnockedDown;
	}
	else if (Parameters == TEXT("Dodging"))
	{
		GateTag = MordecaiGameplayTags::State_Dodging;
	}
	else if (Parameters == TEXT("Parrying"))
	{
		GateTag = MordecaiGameplayTags::State_Parrying;
	}
	else if (Parameters == TEXT("AttackCommitted"))
	{
		GateTag = MordecaiGameplayTags::State_AttackCommitted;
		TestTrue(
			TEXT("The committed attack phase has its own native facing-lock tag"),
			GateTag.IsValid());
	}
	else
	{
		AddError(FString::Printf(TEXT("Unknown facing gate case: %s"), *Parameters));
		return false;
	}
	if (GateTag.IsValid())
	{
		ASC->AddLooseGameplayTag(GateTag);
	}

	Env.Player->SetActorRotation(FRotator::ZeroRotator);
	InvokeInputAction(
		*this,
		Hero,
		TEXT("Input_LookStick"),
		FInputActionValue(FVector2D(1.f, 0.f)));
	TestTrue(
		*FString::Printf(
			TEXT("%s blocks the real right-stick look path"),
			*Parameters),
		FMath::IsNearlyZero(Env.Player->GetActorRotation().Yaw));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FMordecai_Recovery_Facing_AttackLockEndsInRecovery,
	"Mordecai.Recovery.Facing.AttackLockEndsInRecovery",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_Recovery_Facing_AttackLockEndsInRecovery::RunTest(
	const FString& Parameters)
{
	using namespace MordecaiRecoveryFacingTest;
	using namespace MordecaiRecoveryTest;

	FRecoveryControlledTestEnv Env;
	ON_SCOPE_EXIT { Env.TeardownWithController(); };
	if (!SetupControlledAimEnv(*this, Env))
	{
		return true;
	}

	ULyraAbilitySystemComponent* ASC =
		Cast<ULyraAbilitySystemComponent>(Env.GetPlayerASC());
	UMordecaiHeroComponent* Hero =
		Env.Player->FindComponentByClass<UMordecaiHeroComponent>();
	if (!TestNotNull(TEXT("Lyra ASC"), ASC)
		|| !TestNotNull(TEXT("Mordecai hero component"), Hero))
	{
		return true;
	}

	const FGameplayAbilitySpecHandle Handle =
		ASC->GiveAbility(FGameplayAbilitySpec(UMordecaiGA_MeleeAttack::StaticClass(), 1));
	FGameplayAbilitySpec* Granted = ASC->FindAbilitySpecFromHandle(Handle);
	UMordecaiGA_MeleeAttack* Melee = Granted
		? Cast<UMordecaiGA_MeleeAttack>(Granted->GetPrimaryInstance())
		: nullptr;
	if (!TestNotNull(TEXT("real instanced melee ability"), Melee))
	{
		return true;
	}
	Melee->AttackProfiles = {MakeStubProfile()};

	TestTrue(TEXT("real melee ability activates"), ASC->TryActivateAbility(Handle));
	TestEqual(
		TEXT("swing begins in windup"),
		Melee->GetCurrentPhase(),
		EMordecaiAttackPhase::Windup);
	TestTrue(
		TEXT("State.Attacking remains the whole-ability exclusivity tag"),
		ASC->HasMatchingGameplayTag(MordecaiGameplayTags::State_Attacking));
	const FGameplayTag AttackCommittedTag =
		MordecaiGameplayTags::State_AttackCommitted;
	TestTrue(
		TEXT("AttackCommitted is a native tag"),
		AttackCommittedTag.IsValid());
	TestTrue(
		TEXT("AttackCommitted locks facing during windup"),
		ASC->HasMatchingGameplayTag(AttackCommittedTag));

	Env.Player->SetActorRotation(FRotator::ZeroRotator);
	InvokeInputAction(
		*this,
		Hero,
		TEXT("Input_LookStick"),
		FInputActionValue(FVector2D(1.f, 0.f)));
	TestTrue(
		TEXT("right-stick aim cannot redirect a committed windup"),
		FMath::IsNearlyZero(Env.Player->GetActorRotation().Yaw));

	// Exercise the ability's real phase transition method directly. Timer cadence is already
	// covered by MeleeAttack phase tests; this test owns the phase/tag/input interaction.
	Melee->TransitionToPhase(EMordecaiAttackPhase::Active);
	Melee->TransitionToPhase(EMordecaiAttackPhase::Recovery);
	TestEqual(
		TEXT("real swing reaches recovery"),
		Melee->GetCurrentPhase(),
		EMordecaiAttackPhase::Recovery);
	TestTrue(
		TEXT("State.Attacking remains until the ability ends"),
		ASC->HasMatchingGameplayTag(MordecaiGameplayTags::State_Attacking));
	TestFalse(
		TEXT("AttackCommitted ends when recovery begins"),
		ASC->HasMatchingGameplayTag(AttackCommittedTag));

	InvokeInputAction(
		*this,
		Hero,
		TEXT("Input_LookStick"),
		FInputActionValue(FVector2D(1.f, 0.f)));
	TestTrue(
		TEXT("aim-facing resumes during recovery"),
		FMath::IsNearlyEqual(Env.Player->GetActorRotation().Yaw, 90.f, 0.1f));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FMordecai_Recovery_Facing_MovementDoesNotOverrideAim,
	"Mordecai.Recovery.Facing.MovementDoesNotOverrideAim",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_Recovery_Facing_MovementDoesNotOverrideAim::RunTest(
	const FString& Parameters)
{
	using namespace MordecaiRecoveryFacingTest;
	using namespace MordecaiRecoveryTest;

	FRecoveryControlledTestEnv Env;
	ON_SCOPE_EXIT { Env.TeardownWithController(); };
	if (!SetupControlledAimEnv(*this, Env))
	{
		return true;
	}

	UMordecaiHeroComponent* Hero =
		Env.Player->FindComponentByClass<UMordecaiHeroComponent>();
	InvokeInputAction(
		*this,
		Hero,
		TEXT("Input_LookStick"),
		FInputActionValue(FVector2D(1.f, 0.f)));
	const float AimYaw = Env.Player->GetActorRotation().Yaw;
	UCharacterMovementComponent* Movement =
		Env.Player->GetCharacterMovement();
	TestNotNull(TEXT("Character movement component"), Movement);
	if (Movement)
	{
		TestFalse(
			TEXT("Movement never orients the character to velocity"),
			Movement->bOrientRotationToMovement);
		TestFalse(
			TEXT("Fixed camera control rotation never replaces aim-facing"),
			Movement->bUseControllerDesiredRotation);
	}

	const FVector Start = Env.Player->GetActorLocation();
	InvokeInputAction(
		*this,
		Hero,
		TEXT("Input_Move"),
		FInputActionValue(FVector2D(0.f, 1.f)));
	if (Movement)
	{
		Movement->TickComponent(0.1f, LEVELTICK_All, nullptr);
	}
	TestTrue(
		TEXT("The real movement handler and movement tick consume forward input"),
		Env.Player->GetActorLocation().X > Start.X
			|| Env.Player->GetVelocity().X > UE_KINDA_SMALL_NUMBER);
	TestTrue(
		TEXT("Ticking real forward movement preserves the established +Y aim"),
		FMath::IsNearlyEqual(Env.Player->GetActorRotation().Yaw, AimYaw));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FMordecai_Recovery_Camera_StaysFixedWhileAiming,
	"Mordecai.Recovery.Camera.StaysFixedWhileAiming",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_Recovery_Camera_StaysFixedWhileAiming::RunTest(
	const FString& Parameters)
{
	using namespace MordecaiRecoveryFacingTest;
	using namespace MordecaiRecoveryTest;

	FRecoveryControlledTestEnv Env;
	ON_SCOPE_EXIT { Env.TeardownWithController(); };
	if (!SetupControlledAimEnv(*this, Env))
	{
		return true;
	}
	UMordecaiHeroComponent* Hero =
		Env.Player->FindComponentByClass<UMordecaiHeroComponent>();
	InvokeInputAction(
		*this,
		Hero,
		TEXT("Input_LookStick"),
		FInputActionValue(FVector2D(1.f, 0.f)));
	TestTrue(
		TEXT("The test actually aims the pawn to world +Y"),
		FMath::IsNearlyEqual(Env.Player->GetActorRotation().Yaw, 90.f, 0.1f));

	const UMordecaiCameraMode_Diorama* Camera =
		GetDefault<UMordecaiCameraMode_Diorama>();
	TestNotNull(TEXT("Diorama camera CDO"), Camera);
	if (Camera)
	{
		TestEqual(TEXT("Camera pitch remains fixed"), Camera->PitchAngle, -50.f);
	}

	TestFalse(
		TEXT("Aim-facing does not make the camera follow pawn yaw"),
		Env.Player->bUseControllerRotationYaw);
	TestTrue(
		TEXT("Aiming does not replace the fixed controller yaw"),
		FMath::IsNearlyZero(Env.PC->GetControlRotation().Yaw));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FMordecai_Recovery_Facing_CombatConsumersUseActorForward,
	"Mordecai.Recovery.Facing.CombatConsumersUseActorForward",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_Recovery_Facing_CombatConsumersUseActorForward::RunTest(
	const FString& Parameters)
{
	using namespace MordecaiRecoveryFacingTest;

	AMordecaiCharacter* Character = NewObject<AMordecaiCharacter>();
	UMordecaiHeroComponent* Hero =
		Character->FindComponentByClass<UMordecaiHeroComponent>();
	InvokeStickAim(*this, Hero, FVector2D(1.f, 0.f));

	const FVector Forward = Character->GetActorForwardVector();
	TestTrue(
		TEXT("Aim-facing updates the actor-forward direction consumed by melee and spells"),
		Forward.Equals(FVector::RightVector, KINDA_SMALL_NUMBER));
	TestTrue(
		TEXT("The real melee arc query hits a target in the aimed +Y direction"),
		UMordecaiHitDetectionSubsystem::IsPointInArcSector(
			FVector::ZeroVector,
			Forward,
			FVector(0.f, 150.f, 0.f),
			250.f,
			60.f,
			0.f));
	TestFalse(
		TEXT("The real melee arc query rejects the old +X direction after aiming +Y"),
		UMordecaiHitDetectionSubsystem::IsPointInArcSector(
			FVector::ZeroVector,
			Forward,
			FVector(150.f, 0.f, 0.f),
			250.f,
			60.f,
			0.f));
	TestTrue(
		TEXT("A no-input dodge falls back opposite the same actor-forward direction"),
		UMordecaiGA_Dodge::ComputeDodgeDirection(
			FVector::ZeroVector,
			Forward).Equals(-FVector::RightVector, KINDA_SMALL_NUMBER));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FMordecai_Recovery_Facing_PhysicalMouseGameplayContract,
	"Mordecai.Recovery.Facing.PhysicalMouseGameplayContract",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_Recovery_Facing_PhysicalMouseGameplayContract::RunTest(
	const FString& Parameters)
{
	using namespace MordecaiRecoveryTest;

	const UMordecaiCombatHUDWidget* HUDDefaults =
		GetDefault<UMordecaiCombatHUDWidget>();
	const TOptional<FUIInputConfig> Config =
		HUDDefaults->GetDesiredInputConfig();
	if (TestTrue(TEXT("combat HUD declares gameplay mouse input"), Config.IsSet()))
	{
		TestEqual(
			TEXT("Gameplay remains the active CommonUI input mode"),
			Config->GetInputMode(),
			ECommonInputMode::Game);
		TestEqual(
			TEXT("Gameplay keeps physical mouse deltas live except while a click is held"),
			Config->GetMouseCaptureMode(),
			EMouseCaptureMode::CaptureDuringMouseDown);
		TestEqual(
			TEXT("Gameplay confines the cursor to the viewport"),
			Config->GetMouseLockMode(),
			EMouseLockMode::LockAlways);
		TestTrue(
			TEXT("Gameplay hides the OS cursor and leaves the HUD reticle as the one cursor"),
			Config->HideCursorDuringViewportCapture());
	}
	TestTrue(
		TEXT("Active mouse gameplay draws the HUD aim reticle"),
		UMordecaiCombatHUDWidget::ShouldShowAimReticle(true, true, false));
	TestFalse(
		TEXT("The HUD aim reticle is hidden while paused in a menu"),
		UMordecaiCombatHUDWidget::ShouldShowAimReticle(true, true, true));
	TestFalse(
		TEXT("The HUD aim reticle is hidden for gamepad input"),
		UMordecaiCombatHUDWidget::ShouldShowAimReticle(true, false, false));

	const UInputSettings* InputSettings = UInputSettings::GetInputSettings();
	if (TestNotNull(TEXT("project input settings"), InputSettings))
	{
		TestEqual(
			TEXT("The project default no longer permanently captures the physical cursor"),
			InputSettings->DefaultViewportMouseCaptureMode,
			EMouseCaptureMode::CaptureDuringMouseDown);
	}

	const AMordecaiCharacter* CharacterDefaults =
		GetDefault<AMordecaiCharacter>();
	const UMordecaiHeroComponent* HeroDefaults =
		CharacterDefaults->FindComponentByClass<UMordecaiHeroComponent>();
	if (TestNotNull(TEXT("default hero component"), HeroDefaults))
	{
		TestTrue(
			TEXT("Hero ticks so mouse aim is recomputed as camera and height change"),
			HeroDefaults->PrimaryComponentTick.bCanEverTick);
	}

	FRecoveryControlledTestEnv Env;
	ON_SCOPE_EXIT { Env.TeardownWithController(); };
	if (!TestTrue(TEXT("controlled HUD world"), Env.SetupWithController()))
	{
		return true;
	}
	UMordecaiCombatHUDWidget* HUD =
		Env.MakeWidget<UMordecaiCombatHUDWidget>();
	UWidget* Cursor = HUD
		? HUD->GetNamedWidget(HUD, TEXT("AimCursorRoot"))
		: nullptr;
	if (TestNotNull(
		TEXT("HUD owns a screenshot-visible, non-interactive aim cursor"),
		Cursor))
	{
		TestTrue(
			TEXT("Aim cursor cannot consume attack clicks"),
			Cursor->GetVisibility() == ESlateVisibility::HitTestInvisible
				|| Cursor->GetVisibility() == ESlateVisibility::Collapsed);
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FMordecai_Recovery_Facing_PreservesCapsuleContract,
	"Mordecai.Recovery.Facing.PreservesCapsuleContract",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_Recovery_Facing_PreservesCapsuleContract::RunTest(
	const FString& Parameters)
{
	const AMordecaiCharacter* Character = GetDefault<AMordecaiCharacter>();
	const UCapsuleComponent* Capsule = Character->GetCapsuleComponent();
	const UCharacterMovementComponent* Movement =
		Character->GetCharacterMovement();

	TestNotNull(TEXT("Player capsule"), Capsule);
	TestNotNull(TEXT("Player movement"), Movement);
	if (Capsule)
	{
		TestEqual(TEXT("Capsule radius remains 40"), Capsule->GetUnscaledCapsuleRadius(), 40.f);
		TestEqual(TEXT("Capsule half-height remains 90"), Capsule->GetUnscaledCapsuleHalfHeight(), 90.f);
	}
	if (Movement)
	{
		TestEqual(TEXT("Maximum step height remains 45"), Movement->MaxStepHeight, 45.f);
	}
	return true;
}
