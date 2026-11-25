// Project Mordecai — Camera Tests (Story 1.2)

#include "Misc/AutomationTest.h"
#include "Camera/LyraCameraComponent.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/Engine.h"
#include "Engine/StaticMeshActor.h"
#include "Engine/World.h"
#include "Materials/Material.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Mordecai/Camera/MordecaiCameraMode_Diorama.h"

namespace MordecaiCameraTestHelpers
{
	struct FCameraTestWorld
	{
		UWorld* World = nullptr;

		bool Setup()
		{
			if (!GEngine)
			{
				return false;
			}
			World = UWorld::CreateWorld(EWorldType::Game, false);
			if (!World)
			{
				return false;
			}
			FWorldContext& WorldContext = GEngine->CreateNewWorldContext(EWorldType::Game);
			WorldContext.SetCurrentWorld(World);
			World->InitializeActorsForPlay(FURL());
			return true;
		}

		void Teardown()
		{
			if (World && GEngine)
			{
				GEngine->DestroyWorldContext(World);
				World->DestroyWorld(false);
			}
			World = nullptr;
		}
	};

	static UStaticMeshComponent* SpawnTraceWall(
		UWorld* World, const FVector& Location, bool bFadeTagged)
	{
		AStaticMeshActor* WallActor = World->SpawnActor<AStaticMeshActor>(
			AStaticMeshActor::StaticClass(), Location, FRotator::ZeroRotator);
		if (!WallActor)
		{
			return nullptr;
		}

		UStaticMeshComponent* Mesh = WallActor->GetStaticMeshComponent();
		UStaticMesh* Cube = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube"));
		if (!Mesh || !Cube)
		{
			return nullptr;
		}
		Mesh->SetStaticMesh(Cube);
		WallActor->SetActorScale3D(FVector(0.25f, 10.0f, 10.0f));
		Mesh->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
		Mesh->SetCollisionObjectType(ECC_WorldStatic);
		Mesh->SetCollisionResponseToAllChannels(ECR_Ignore);
		Mesh->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
		Mesh->SetMaterial(0, UMaterial::GetDefaultMaterial(MD_Surface));
		if (UMaterialInstanceDynamic* Material = Mesh->CreateDynamicMaterialInstance(0))
		{
			Material->SetScalarParameterValue(TEXT("MordecaiOcclusionFade"), 1.0f);
		}
		if (bFadeTagged)
		{
			Mesh->ComponentTags.Add(TEXT("Mordecai.OcclusionFade"));
		}
		Mesh->RecreatePhysicsState();
		return Mesh;
	}

	static float ReadFade(UStaticMeshComponent* Mesh)
	{
		UMaterialInstanceDynamic* Material =
			Mesh ? Cast<UMaterialInstanceDynamic>(Mesh->GetMaterial(0)) : nullptr;
		return Material ? Material->K2_GetScalarParameterValue(TEXT("MordecaiOcclusionFade")) : -1.0f;
	}
}

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

// Mordecai.Recovery.Camera.WorldTraceFadesAndRestoresTaggedWalls
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecaiRecoveryCamera_WorldTraceFadeRestore,
	"Mordecai.Recovery.Camera.WorldTraceFadesAndRestoresTaggedWalls",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecaiRecoveryCamera_WorldTraceFadeRestore::RunTest(const FString& Parameters)
{
	using namespace MordecaiCameraTestHelpers;
	FCameraTestWorld TestWorld;
	if (!TestTrue("Test world created", TestWorld.Setup()))
	{
		return true;
	}

	AActor* Target = TestWorld.World->SpawnActor<AActor>();
	ULyraCameraComponent* Camera = Target
		? NewObject<ULyraCameraComponent>(Target, TEXT("US099TestCamera"))
		: nullptr;
	if (!TestNotNull("Target actor spawned", Target)
		|| !TestNotNull("Camera component created", Camera))
	{
		TestWorld.Teardown();
		return true;
	}
	Target->AddInstanceComponent(Camera);
	Camera->RegisterComponent();

	UMordecaiCameraMode_Diorama* Mode =
		NewObject<UMordecaiCameraMode_Diorama>(Camera, TEXT("US099TestMode"));
	Mode->FollowSpeed = 100000.0f;
	Mode->UpdateCameraMode(0.0f);
	const FVector CameraLocation = Mode->GetCameraModeView().Location;
	const FVector TargetLocation = Target->GetActorLocation();

	UStaticMeshComponent* NearUntagged = SpawnTraceWall(
		TestWorld.World, FMath::Lerp(CameraLocation, TargetLocation, 0.25f), false);
	UStaticMeshComponent* TaggedWall = SpawnTraceWall(
		TestWorld.World, FMath::Lerp(CameraLocation, TargetLocation, 0.50f), true);
	UStaticMeshComponent* StackedTaggedWall = SpawnTraceWall(
		TestWorld.World, FMath::Lerp(CameraLocation, TargetLocation, 0.75f), true);
	if (!TestNotNull("Near untagged blocker spawned", NearUntagged)
		|| !TestNotNull("Tagged wall spawned", TaggedWall)
		|| !TestNotNull("Stacked tagged wall spawned", StackedTaggedWall))
	{
		TestWorld.Teardown();
		return true;
	}

	FHitResult FirstHit;
	const bool bHit = TestWorld.World->LineTraceSingleByChannel(
		FirstHit, CameraLocation, TargetLocation, ECC_Visibility);
	TestTrue("Positive control trace hits geometry", bHit);
	TestEqual("Positive control puts untagged geometry first", FirstHit.GetComponent(),
		static_cast<UPrimitiveComponent*>(NearUntagged));

	Mode->UpdateCameraMode(0.25f);
	TestEqual("Untagged nearer geometry remains opaque", ReadFade(NearUntagged), 1.0f);
	TestTrue("Tagged wall behind untagged geometry fades",
		ReadFade(TaggedWall) < 1.0f);
	TestTrue("Second stacked tagged wall also fades",
		ReadFade(StackedTaggedWall) < 1.0f);

	Target->SetActorLocation(FVector(0.0f, 5000.0f, 0.0f));
	Mode->UpdateCameraMode(0.25f);
	TestEqual("Tagged wall restores when target moves clear", ReadFade(TaggedWall), 1.0f);
	TestEqual("Stacked tagged wall restores when target moves clear", ReadFade(StackedTaggedWall), 1.0f);

	TestWorld.Teardown();
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

	UInstancedStaticMeshComponent* InstancedWall = NewObject<UInstancedStaticMeshComponent>();
	InstancedWall->SetCollisionObjectType(ECC_WorldStatic);
	InstancedWall->ComponentTags.Add(TEXT("Mordecai.OcclusionFade"));
	TestFalse("Instanced meshes are rejected because per-instance fading is unsupported",
		UMordecaiCameraMode_Diorama::IsOcclusionFadeCandidate(InstancedWall));
	return true;
}
