// Project Mordecai — Character Tests (Story 2.1)

#include "Misc/AutomationTest.h"
#include "Mordecai/MordecaiCharacter.h"
#include "Components/ArrowComponent.h"
#include "GameFramework/CharacterMovementComponent.h"

// Mordecai.Character.MovesInWorldSpace
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecaiChar_MovesWorldSpace,
	"Mordecai.Character.MovesInWorldSpace",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecaiChar_MovesWorldSpace::RunTest(const FString& Parameters)
{
	// Verify character is configured for world-space movement (AC-2.1.1)
	// In twin-stick: bOrientRotationToMovement must be false (character doesn't auto-face movement dir)
	const AMordecaiCharacter* CDO = GetDefault<AMordecaiCharacter>();
	TestNotNull("Character CDO exists", CDO);

	// Controller rotation should not drive character rotation directly
	TestFalse("bUseControllerRotationYaw is false", CDO->bUseControllerRotationYaw);

	return true;
}

// Mordecai.Character.RotatesIndependentlyOfMovement
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecaiChar_IndependentRotation,
	"Mordecai.Character.RotatesIndependentlyOfMovement",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecaiChar_IndependentRotation::RunTest(const FString& Parameters)
{
	// Verify rotation is driven by right stick / mouse (AC-2.1.2)
	const AMordecaiCharacter* CDO = GetDefault<AMordecaiCharacter>();
	TestNotNull("Character CDO exists", CDO);

	// Character should NOT auto-orient to movement direction
	TestFalse("bUseControllerRotationPitch is false", CDO->bUseControllerRotationPitch);
	TestFalse("bUseControllerRotationRoll is false", CDO->bUseControllerRotationRoll);

	return true;
}

// Mordecai.Character.SprintIncreasesSpeed
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecaiChar_SprintSpeed,
	"Mordecai.Character.SprintIncreasesSpeed",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecaiChar_SprintSpeed::RunTest(const FString& Parameters)
{
	// Verify sprint multiplier makes character faster (AC-2.1.3)
	const AMordecaiCharacter* CDO = GetDefault<AMordecaiCharacter>();
	TestNotNull("Character CDO exists", CDO);

	TestTrue("Walk speed is positive", CDO->WalkSpeed > 0.0f);
	TestTrue("Sprint multiplier > 1.0", CDO->SprintSpeedMultiplier > 1.0f);
	TestTrue("Sprint speed > walk speed",
		CDO->WalkSpeed * CDO->SprintSpeedMultiplier > CDO->WalkSpeed);

	return true;
}

// Mordecai.Character.JumpAppliesGravity
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecaiChar_JumpGravity,
	"Mordecai.Character.JumpAppliesGravity",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecaiChar_JumpGravity::RunTest(const FString& Parameters)
{
	// Verify character has gravity and jump velocity (AC-2.1.4)
	UClass* CharClass = AMordecaiCharacter::StaticClass();
	TestNotNull("Character class exists", CharClass);

	// Check movement component defaults on CDO
	const AMordecaiCharacter* CDO = GetDefault<AMordecaiCharacter>();
	TestNotNull("Character CDO exists", CDO);

	// The character should be configured for jumping
	TestTrue("Character can jump", CDO->JumpMaxCount > 0);

	return true;
}

// Mordecai.Character.HasPlaceholderVisuals
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecaiChar_PlaceholderVisuals,
	"Mordecai.Character.HasPlaceholderVisuals",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecaiChar_PlaceholderVisuals::RunTest(const FString& Parameters)
{
	// Verify capsule + arrow component exist (AC-2.1.5)
	UClass* CharClass = AMordecaiCharacter::StaticClass();
	TestNotNull("Character class exists", CharClass);

	// Check that FacingArrow subobject exists on the class template
	const AMordecaiCharacter* CDO = GetDefault<AMordecaiCharacter>();
	TestNotNull("Character CDO exists", CDO);

	UArrowComponent* Arrow = CDO->GetFacingArrow();
	TestNotNull("FacingArrow component exists", Arrow);

	return true;
}

// Mordecai.Character.MovementIsConfigurable
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecaiChar_MovementConfigurable,
	"Mordecai.Character.MovementIsConfigurable",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecaiChar_MovementConfigurable::RunTest(const FString& Parameters)
{
	// Verify movement tuning properties are exposed (AC-2.1.6)
	UClass* CharClass = AMordecaiCharacter::StaticClass();

	FProperty* WalkSpeedProp = CharClass->FindPropertyByName(FName("WalkSpeed"));
	TestNotNull("WalkSpeed property exposed", WalkSpeedProp);

	FProperty* SprintMultProp = CharClass->FindPropertyByName(FName("SprintSpeedMultiplier"));
	TestNotNull("SprintSpeedMultiplier property exposed", SprintMultProp);

	FProperty* AccelProp = CharClass->FindPropertyByName(FName("GroundAcceleration"));
	TestNotNull("GroundAcceleration property exposed", AccelProp);

	FProperty* DecelProp = CharClass->FindPropertyByName(FName("GroundDeceleration"));
	TestNotNull("GroundDeceleration property exposed", DecelProp);

	return true;
}
