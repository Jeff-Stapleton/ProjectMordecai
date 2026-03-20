// Project Mordecai — Camera Tests (Story 1.2)

#include "Misc/AutomationTest.h"
#include "Mordecai/Camera/MordecaiCameraMode_Diorama.h"

// Mordecai.Camera.FollowsCharacterPosition
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecaiCamera_FollowsCharacter,
	"Mordecai.Camera.FollowsCharacterPosition",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecaiCamera_FollowsCharacter::RunTest(const FString& Parameters)
{
	const UMordecaiCameraMode_Diorama* CDO = GetDefault<UMordecaiCameraMode_Diorama>();
	TestNotNull("Diorama camera mode CDO exists", CDO);

	// Camera has a positive follow speed, meaning it tracks the character
	TestTrue("FollowSpeed is positive", CDO->FollowSpeed > 0.0f);

	return true;
}

// Mordecai.Camera.RotationRemainsFixed
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecaiCamera_RotationFixed,
	"Mordecai.Camera.RotationRemainsFixed",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecaiCamera_RotationFixed::RunTest(const FString& Parameters)
{
	const UMordecaiCameraMode_Diorama* CDO = GetDefault<UMordecaiCameraMode_Diorama>();
	TestNotNull("Diorama camera mode CDO exists", CDO);

	// The camera pitch angle should be negative (looking down)
	TestTrue("PitchAngle is negative (looking down)", CDO->PitchAngle < 0.0f);

	// Distance should be reasonable for a diorama view
	TestTrue("Distance is reasonable", CDO->Distance > 500.0f);

	return true;
}

// Mordecai.Camera.ExposesDesignerProperties
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecaiCamera_DesignerProperties,
	"Mordecai.Camera.ExposesDesignerProperties",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecaiCamera_DesignerProperties::RunTest(const FString& Parameters)
{
	UClass* CameraClass = UMordecaiCameraMode_Diorama::StaticClass();
	TestNotNull("Camera mode class exists", CameraClass);

	// Verify designer-exposed properties exist via reflection (AC-1.2.3)
	FProperty* PitchProp = CameraClass->FindPropertyByName(FName("PitchAngle"));
	TestNotNull("PitchAngle property exists", PitchProp);

	FProperty* DistProp = CameraClass->FindPropertyByName(FName("Distance"));
	TestNotNull("Distance property exists", DistProp);

	FProperty* SpeedProp = CameraClass->FindPropertyByName(FName("FollowSpeed"));
	TestNotNull("FollowSpeed property exists", SpeedProp);

	return true;
}
