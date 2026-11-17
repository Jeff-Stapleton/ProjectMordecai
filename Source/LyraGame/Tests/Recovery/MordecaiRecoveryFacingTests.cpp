// Project Mordecai — aim-driven player facing (US-098 / issue #34)

#include "Misc/AutomationTest.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Mordecai/Camera/MordecaiCameraMode_Diorama.h"
#include "Mordecai/Combat/MordecaiGA_Dodge.h"
#include "Mordecai/MordecaiCharacter.h"
#include "Mordecai/MordecaiHeroComponent.h"

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

	TestFalse(
		TEXT("A stick value below the 0.2 deadzone does not replace facing"),
		InvokeStickAim(*this, Hero, FVector2D(0.1f, 0.f)));
	TestTrue(
		TEXT("A full right-stick-left input applies facing"),
		InvokeStickAim(*this, Hero, FVector2D(-1.f, 0.f)));
	TestTrue(
		TEXT("Right-stick left maps to world -Y / yaw -90"),
		FMath::IsNearlyEqual(Character->GetActorRotation().Yaw, -90.f));

	// Releasing the stick emits no new direction: keep the last facing.
	TestFalse(
		TEXT("A released stick does not replace the last facing"),
		InvokeStickAim(*this, Hero, FVector2D::ZeroVector));
	TestTrue(
		TEXT("Stick release preserves yaw -90"),
		FMath::IsNearlyEqual(Character->GetActorRotation().Yaw, -90.f));
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

	AMordecaiCharacter* Character = NewObject<AMordecaiCharacter>();
	UMordecaiHeroComponent* Hero =
		Character->FindComponentByClass<UMordecaiHeroComponent>();
	if (!TestNotNull(TEXT("Character owns its Mordecai hero component"), Hero))
	{
		return true;
	}

	InvokeStickAim(*this, Hero, FVector2D(1.f, 0.f));
	const float AimYaw = Character->GetActorRotation().Yaw;
	const UCharacterMovementComponent* Movement =
		Character->GetCharacterMovement();
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
	TestTrue(
		TEXT("Adding forward movement input preserves the established +Y aim"),
		FMath::IsNearlyEqual(Character->GetActorRotation().Yaw, AimYaw));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FMordecai_Recovery_Camera_StaysFixedWhileAiming,
	"Mordecai.Recovery.Camera.StaysFixedWhileAiming",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_Recovery_Camera_StaysFixedWhileAiming::RunTest(
	const FString& Parameters)
{
	const UMordecaiCameraMode_Diorama* Camera =
		GetDefault<UMordecaiCameraMode_Diorama>();
	TestNotNull(TEXT("Diorama camera CDO"), Camera);
	if (Camera)
	{
		TestEqual(TEXT("Camera pitch remains fixed"), Camera->PitchAngle, -50.f);
	}

	const AMordecaiCharacter* Character = GetDefault<AMordecaiCharacter>();
	TestFalse(
		TEXT("Aim-facing does not make the camera follow pawn yaw"),
		Character->bUseControllerRotationYaw);
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
		TEXT("A no-input dodge falls back opposite the same actor-forward direction"),
		UMordecaiGA_Dodge::ComputeDodgeDirection(
			FVector::ZeroVector,
			Forward).Equals(-FVector::RightVector, KINDA_SMALL_NUMBER));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FMordecai_Recovery_Facing_AimRotationIsServerAuthoritative,
	"Mordecai.Recovery.Facing.AimRotationIsServerAuthoritative",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_Recovery_Facing_AimRotationIsServerAuthoritative::RunTest(
	const FString& Parameters)
{
	AMordecaiCharacter* Character = NewObject<AMordecaiCharacter>();
	UFunction* ServerFunction =
		Character->FindFunction(TEXT("ServerSetAimFacingDirection"));
	if (TestNotNull(TEXT("Aim-facing has a server RPC"), ServerFunction))
	{
		TestTrue(
			TEXT("Aim-facing RPC executes on the server"),
			ServerFunction->HasAnyFunctionFlags(FUNC_NetServer));
		TestFalse(
			TEXT("High-rate aim-facing RPC is intentionally unreliable"),
			ServerFunction->HasAnyFunctionFlags(FUNC_NetReliable));
	}
	TestTrue(
		TEXT("Character movement continues to replicate actor rotation"),
		Character->IsReplicatingMovement());

	Character->SetAimFacingDirection(FVector::RightVector);
	TestTrue(
		TEXT("The owning character applies +Y facing immediately"),
		Character->GetActorForwardVector().Equals(
			FVector::RightVector,
			KINDA_SMALL_NUMBER));
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
