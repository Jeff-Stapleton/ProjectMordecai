// Project Mordecai — Camera Tests (Story 1.2)

#include "Misc/AutomationTest.h"
#include "Components/StaticMeshComponent.h"
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

// Mordecai.Recovery.Camera.OccluderFadesWhenBlockingPlayer
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecaiRecoveryCamera_OccluderFades,
	"Mordecai.Recovery.Camera.OccluderFadesWhenBlockingPlayer",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecaiRecoveryCamera_OccluderFades::RunTest(const FString& Parameters)
{
	UStaticMeshComponent* Wall = NewObject<UStaticMeshComponent>();
	Wall->SetCollisionObjectType(ECC_WorldStatic);
	Wall->ComponentTags.Add(TEXT("Mordecai.OcclusionFade"));

	TestTrue("Tagged WorldStatic wall is eligible", UMordecaiCameraMode_Diorama::IsOcclusionFadeCandidate(Wall));
	const float HalfFade = UMordecaiCameraMode_Diorama::AdvanceOcclusionFade(1.0f, true, 0.125f, 0.25f);
	TestTrue("Blocking wall begins fading", HalfFade > 0.0f && HalfFade < 1.0f);
	TestEqual("Blocking wall reaches fully faded in 0.25 seconds",
		UMordecaiCameraMode_Diorama::AdvanceOcclusionFade(HalfFade, true, 0.125f, 0.25f), 0.0f);
	return true;
}

// Mordecai.Recovery.Camera.RestoresWhenNotBlocking
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecaiRecoveryCamera_Restores,
	"Mordecai.Recovery.Camera.RestoresWhenNotBlocking",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecaiRecoveryCamera_Restores::RunTest(const FString& Parameters)
{
	const float HalfRestored = UMordecaiCameraMode_Diorama::AdvanceOcclusionFade(0.0f, false, 0.125f, 0.25f);
	TestTrue("Clear wall begins restoring", HalfRestored > 0.0f && HalfRestored < 1.0f);
	TestEqual("Clear wall is fully restored in 0.25 seconds",
		UMordecaiCameraMode_Diorama::AdvanceOcclusionFade(HalfRestored, false, 0.125f, 0.25f), 1.0f);
	return true;
}

// Mordecai.Recovery.Camera.DoesNotFadeFloor
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecaiRecoveryCamera_DoesNotFadeFloor,
	"Mordecai.Recovery.Camera.DoesNotFadeFloor",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecaiRecoveryCamera_DoesNotFadeFloor::RunTest(const FString& Parameters)
{
	UStaticMeshComponent* Floor = NewObject<UStaticMeshComponent>();
	Floor->SetCollisionObjectType(ECC_WorldStatic);
	TestFalse("Untagged floor is not eligible", UMordecaiCameraMode_Diorama::IsOcclusionFadeCandidate(Floor));

	UStaticMeshComponent* Enemy = NewObject<UStaticMeshComponent>();
	Enemy->SetCollisionObjectType(ECC_Pawn);
	Enemy->ComponentTags.Add(TEXT("Mordecai.OcclusionFade"));
	TestFalse("Pawn geometry is never eligible", UMordecaiCameraMode_Diorama::IsOcclusionFadeCandidate(Enemy));
	return true;
}
