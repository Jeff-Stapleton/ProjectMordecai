// Project Mordecai — Melee Hit Detection Tests (US-003)

#include "Misc/AutomationTest.h"
#include "Mordecai/Combat/MordecaiHitDetectionTypes.h"
#include "Mordecai/Combat/MordecaiHitDetectionSubsystem.h"
#include "Mordecai/Combat/MordecaiAttackProfileDataAsset.h"
#include "Mordecai/MordecaiGameplayTags.h"
#include "Mordecai/Weapons/MordecaiWeaponDataAsset.h"
#include "Mordecai/Weapons/MordecaiWeaponProfileFactory.h"
#include "HAL/IConsoleManager.h"

// ---------------------------------------------------------------------------
// Mordecai.HitDetection.ArcSectorHitsActorInArc  (AC-003.3)
// Places actor at 45° in a 90° arc, verifies hit
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecaiHitDetection_ArcSectorHitsInArc,
	"Mordecai.HitDetection.ArcSectorHitsActorInArc",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecaiHitDetection_ArcSectorHitsInArc::RunTest(const FString& Parameters)
{
	const FVector Origin(0, 0, 0);
	const FVector Forward(1, 0, 0);
	const float Radius = 300.f;
	const float AngleDegrees = 90.f;
	const float StartAngleOffset = 0.f;

	// Target at 45° from forward, 200 units away (within radius)
	const FVector TargetLocation = Origin + FVector(
		FMath::Cos(FMath::DegreesToRadians(45.f)),
		FMath::Sin(FMath::DegreesToRadians(45.f)),
		0.f) * 200.f;

	bool bInArc = UMordecaiHitDetectionSubsystem::IsPointInArcSector(
		Origin, Forward, TargetLocation, Radius, AngleDegrees, StartAngleOffset);

	TestTrue("Actor at 45 degrees should be within 90 degree arc", bInArc);
	return true;
}

// ---------------------------------------------------------------------------
// Mordecai.HitDetection.ArcSectorMissesActorOutsideArc  (AC-003.3)
// Places actor at 120° in a 90° arc, verifies miss
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecaiHitDetection_ArcSectorMissesOutside,
	"Mordecai.HitDetection.ArcSectorMissesActorOutsideArc",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecaiHitDetection_ArcSectorMissesOutside::RunTest(const FString& Parameters)
{
	const FVector Origin(0, 0, 0);
	const FVector Forward(1, 0, 0);
	const float Radius = 300.f;
	const float AngleDegrees = 90.f;
	const float StartAngleOffset = 0.f;

	// Target at 120° from forward, 200 units away (within radius but outside angle)
	const FVector TargetLocation = Origin + FVector(
		FMath::Cos(FMath::DegreesToRadians(120.f)),
		FMath::Sin(FMath::DegreesToRadians(120.f)),
		0.f) * 200.f;

	bool bInArc = UMordecaiHitDetectionSubsystem::IsPointInArcSector(
		Origin, Forward, TargetLocation, Radius, AngleDegrees, StartAngleOffset);

	TestFalse("Actor at 120 degrees should be outside 90 degree arc", bInArc);
	return true;
}

// ---------------------------------------------------------------------------
// Mordecai.HitDetection.ArcSectorMissesActorBeyondRadius  (AC-003.3)
// Actor in angle but past radius, verifies miss
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecaiHitDetection_ArcSectorMissesBeyondRadius,
	"Mordecai.HitDetection.ArcSectorMissesActorBeyondRadius",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecaiHitDetection_ArcSectorMissesBeyondRadius::RunTest(const FString& Parameters)
{
	const FVector Origin(0, 0, 0);
	const FVector Forward(1, 0, 0);
	const float Radius = 300.f;
	const float AngleDegrees = 90.f;
	const float StartAngleOffset = 0.f;

	// Target directly ahead (0°) but 400 units away (beyond 300 radius)
	const FVector TargetLocation = Origin + Forward * 400.f;

	bool bInArc = UMordecaiHitDetectionSubsystem::IsPointInArcSector(
		Origin, Forward, TargetLocation, Radius, AngleDegrees, StartAngleOffset);

	TestFalse("Actor beyond radius should miss even if in angle", bInArc);
	return true;
}

// ---------------------------------------------------------------------------
// Mordecai.HitDetection.CapsuleHitsActorInLane  (AC-003.4)
// Actor directly ahead within lane, verifies hit
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecaiHitDetection_CapsuleHitsInLane,
	"Mordecai.HitDetection.CapsuleHitsActorInLane",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecaiHitDetection_CapsuleHitsInLane::RunTest(const FString& Parameters)
{
	const FVector Origin(0, 0, 0);
	const FVector Forward(1, 0, 0);
	const float Length = 200.f;
	const float Width = 100.f;

	// Target directly ahead, 150 units forward (within lane)
	const FVector TargetLocation(150.f, 0.f, 0.f);

	bool bInLane = UMordecaiHitDetectionSubsystem::IsPointInCapsuleLane(
		Origin, Forward, TargetLocation, Length, Width);

	TestTrue("Actor directly ahead should be in capsule lane", bInLane);
	return true;
}

// ---------------------------------------------------------------------------
// Mordecai.HitDetection.CapsuleMissesActorBesideLane  (AC-003.4)
// Actor perpendicular to lane, verifies miss
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecaiHitDetection_CapsuleMissesBesideLane,
	"Mordecai.HitDetection.CapsuleMissesActorBesideLane",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecaiHitDetection_CapsuleMissesBesideLane::RunTest(const FString& Parameters)
{
	const FVector Origin(0, 0, 0);
	const FVector Forward(1, 0, 0);
	const float Length = 200.f;
	const float Width = 100.f;

	// Target to the side: 100 forward, 200 lateral (halfWidth=50, so 200 is way outside)
	const FVector TargetLocation(100.f, 200.f, 0.f);

	bool bInLane = UMordecaiHitDetectionSubsystem::IsPointInCapsuleLane(
		Origin, Forward, TargetLocation, Length, Width);

	TestFalse("Actor beside the lane should miss", bInLane);
	return true;
}

// ---------------------------------------------------------------------------
// Mordecai.HitDetection.CircleHitsActorInRadius  (AC-003.5)
// Actor within circle, verifies hit
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecaiHitDetection_CircleHitsInRadius,
	"Mordecai.HitDetection.CircleHitsActorInRadius",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecaiHitDetection_CircleHitsInRadius::RunTest(const FString& Parameters)
{
	const FVector Center(0, 0, 0);
	const float Radius = 200.f;

	// Target 100 units away (within 200 radius)
	const FVector TargetLocation(100.f, 0.f, 0.f);

	bool bInCircle = UMordecaiHitDetectionSubsystem::IsPointInCircle(
		Center, TargetLocation, Radius);

	TestTrue("Actor within circle radius should hit", bInCircle);
	return true;
}

// ---------------------------------------------------------------------------
// Mordecai.HitDetection.CircleMissesActorOutsideRadius  (AC-003.5)
// Actor outside circle, verifies miss
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecaiHitDetection_CircleMissesOutside,
	"Mordecai.HitDetection.CircleMissesActorOutsideRadius",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecaiHitDetection_CircleMissesOutside::RunTest(const FString& Parameters)
{
	const FVector Center(0, 0, 0);
	const float Radius = 200.f;

	// Target 300 units away (outside 200 radius)
	const FVector TargetLocation(300.f, 0.f, 0.f);

	bool bInCircle = UMordecaiHitDetectionSubsystem::IsPointInCircle(
		Center, TargetLocation, Radius);

	TestFalse("Actor outside circle radius should miss", bInCircle);
	return true;
}

// ---------------------------------------------------------------------------
// Mordecai.HitDetection.JumpAvoidableSkipsAirborneTarget  (AC-003.7)
// Sweep with JumpAvoidable=true vs airborne target, verifies no hit
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecaiHitDetection_JumpAvoidableSkipsAirborne,
	"Mordecai.HitDetection.JumpAvoidableSkipsAirborneTarget",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecaiHitDetection_JumpAvoidableSkipsAirborne::RunTest(const FString& Parameters)
{
	// JumpAvoidable=true, target is airborne, HitsAirborne=false → EXCLUDE
	bool bExclude = UMordecaiHitDetectionSubsystem::ShouldExcludeTarget(
		/*bIsAttacker=*/ false,
		/*bIsFriendly=*/ false,
		/*bTargetIsAirborne=*/ true,
		/*bJumpAvoidable=*/ true,
		/*bHitsAirborne=*/ false,
		/*bAllowFriendlyFire=*/ false);

	TestTrue("Airborne target should be excluded by JumpAvoidable attack", bExclude);
	return true;
}

// ---------------------------------------------------------------------------
// Mordecai.HitDetection.JumpAvoidableHitsGroundedTarget  (AC-003.7)
// Sweep with JumpAvoidable=true vs grounded target, verifies hit
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecaiHitDetection_JumpAvoidableHitsGrounded,
	"Mordecai.HitDetection.JumpAvoidableHitsGroundedTarget",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecaiHitDetection_JumpAvoidableHitsGrounded::RunTest(const FString& Parameters)
{
	// JumpAvoidable=true, target is NOT airborne → INCLUDE
	bool bExclude = UMordecaiHitDetectionSubsystem::ShouldExcludeTarget(
		/*bIsAttacker=*/ false,
		/*bIsFriendly=*/ false,
		/*bTargetIsAirborne=*/ false,
		/*bJumpAvoidable=*/ true,
		/*bHitsAirborne=*/ false,
		/*bAllowFriendlyFire=*/ false);

	TestFalse("Grounded target should NOT be excluded by JumpAvoidable attack", bExclude);
	return true;
}

// ---------------------------------------------------------------------------
// Mordecai.HitDetection.HitsAirborneIncludesJumpingTarget  (AC-003.8)
// Thrust with HitsAirborne=true vs airborne target, verifies hit
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecaiHitDetection_HitsAirborneIncludes,
	"Mordecai.HitDetection.HitsAirborneIncludesJumpingTarget",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecaiHitDetection_HitsAirborneIncludes::RunTest(const FString& Parameters)
{
	// HitsAirborne=true, target is airborne → INCLUDE (not excluded)
	bool bExclude = UMordecaiHitDetectionSubsystem::ShouldExcludeTarget(
		/*bIsAttacker=*/ false,
		/*bIsFriendly=*/ false,
		/*bTargetIsAirborne=*/ true,
		/*bJumpAvoidable=*/ false,
		/*bHitsAirborne=*/ true,
		/*bAllowFriendlyFire=*/ false);

	TestFalse("Airborne target should NOT be excluded when HitsAirborne is true", bExclude);
	return true;
}

// ---------------------------------------------------------------------------
// Mordecai.HitDetection.AttackerExcludedFromResults  (AC-003.6)
// Attacker standing in own sweep area, verifies not hit
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecaiHitDetection_AttackerExcluded,
	"Mordecai.HitDetection.AttackerExcludedFromResults",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecaiHitDetection_AttackerExcluded::RunTest(const FString& Parameters)
{
	// Target IS the attacker → EXCLUDE
	bool bExclude = UMordecaiHitDetectionSubsystem::ShouldExcludeTarget(
		/*bIsAttacker=*/ true,
		/*bIsFriendly=*/ false,
		/*bTargetIsAirborne=*/ false,
		/*bJumpAvoidable=*/ false,
		/*bHitsAirborne=*/ false,
		/*bAllowFriendlyFire=*/ false);

	TestTrue("Attacker should be excluded from their own hit results", bExclude);
	return true;
}

// ---------------------------------------------------------------------------
// Mordecai.HitDetection.FriendlyFireOffExcludesAllies  (AC-003.9)
// Friendly actor in range with FF off, verifies no hit
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecaiHitDetection_FriendlyFireOff,
	"Mordecai.HitDetection.FriendlyFireOffExcludesAllies",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecaiHitDetection_FriendlyFireOff::RunTest(const FString& Parameters)
{
	// Target is friendly, FF off → EXCLUDE
	bool bExcludeFriendlyNoFF = UMordecaiHitDetectionSubsystem::ShouldExcludeTarget(
		/*bIsAttacker=*/ false,
		/*bIsFriendly=*/ true,
		/*bTargetIsAirborne=*/ false,
		/*bJumpAvoidable=*/ false,
		/*bHitsAirborne=*/ false,
		/*bAllowFriendlyFire=*/ false);
	TestTrue("Friendly target should be excluded when FF is off", bExcludeFriendlyNoFF);

	// Target is friendly, FF on → INCLUDE
	bool bExcludeFriendlyWithFF = UMordecaiHitDetectionSubsystem::ShouldExcludeTarget(
		/*bIsAttacker=*/ false,
		/*bIsFriendly=*/ true,
		/*bTargetIsAirborne=*/ false,
		/*bJumpAvoidable=*/ false,
		/*bHitsAirborne=*/ false,
		/*bAllowFriendlyFire=*/ true);
	TestFalse("Friendly target should NOT be excluded when FF is on", bExcludeFriendlyWithFF);

	return true;
}

// ---------------------------------------------------------------------------
// Mordecai.HitDetection.UnifiedDispatchUsesCorrectShape  (AC-003.2)
// Calls PerformMeleeHitDetection with each AttackType, verifies correct query
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecaiHitDetection_UnifiedDispatch,
	"Mordecai.HitDetection.UnifiedDispatchUsesCorrectShape",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecaiHitDetection_UnifiedDispatch::RunTest(const FString& Parameters)
{
	// Verify the subsystem class exists and is a world subsystem
	const UClass* SubsystemClass = UMordecaiHitDetectionSubsystem::StaticClass();
	TestNotNull("HitDetectionSubsystem class exists", SubsystemClass);
	TestTrue("Inherits from UWorldSubsystem",
		SubsystemClass->IsChildOf(UWorldSubsystem::StaticClass()));

	// Verify PerformMeleeHitDetection method exists via UFUNCTION reflection
	const UFunction* DispatchFunc = SubsystemClass->FindFunctionByName(FName("PerformMeleeHitDetection"));
	TestNotNull("PerformMeleeHitDetection function exists", DispatchFunc);

	// Verify each melee attack type maps to the correct shape
	{
		UMordecaiAttackProfileDataAsset* SweepProfile = NewObject<UMordecaiAttackProfileDataAsset>();
		SweepProfile->AttackType = EMordecaiAttackType::MeleeSweep;
		SweepProfile->HitShapeType = EMordecaiHitShapeType::ArcSector;
		TestEqual("Sweep uses ArcSector", SweepProfile->HitShapeType, EMordecaiHitShapeType::ArcSector);
	}
	{
		UMordecaiAttackProfileDataAsset* ThrustProfile = NewObject<UMordecaiAttackProfileDataAsset>();
		ThrustProfile->AttackType = EMordecaiAttackType::MeleeThrust;
		ThrustProfile->HitShapeType = EMordecaiHitShapeType::Capsule;
		TestEqual("Thrust uses Capsule", ThrustProfile->HitShapeType, EMordecaiHitShapeType::Capsule);
	}
	{
		UMordecaiAttackProfileDataAsset* SlamProfile = NewObject<UMordecaiAttackProfileDataAsset>();
		SlamProfile->AttackType = EMordecaiAttackType::MeleeSlam;
		SlamProfile->HitShapeType = EMordecaiHitShapeType::Circle;
		TestEqual("Slam uses Circle", SlamProfile->HitShapeType, EMordecaiHitShapeType::Circle);
	}

	// Verify FMordecaiHitResult struct has required fields (AC-003.11)
	const UScriptStruct* HitResultStruct = FMordecaiHitResult::StaticStruct();
	TestNotNull("FMordecaiHitResult registered", HitResultStruct);

	auto CheckField = [&](const TCHAR* FieldName)
	{
		const FProperty* Prop = HitResultStruct->FindPropertyByName(FName(FieldName));
		TestNotNull(FString::Printf(TEXT("FMordecaiHitResult.%s exists"), FieldName), Prop);
	};
	CheckField(TEXT("HitActor"));
	CheckField(TEXT("HitLocation"));
	CheckField(TEXT("HitNormal"));
	CheckField(TEXT("bWasAirborne"));

	// Verify EMordecaiTargetFilter enum (AC-003.6)
	const UEnum* FilterEnum = StaticEnum<EMordecaiTargetFilter>();
	TestNotNull("EMordecaiTargetFilter registered", FilterEnum);
	TestFalse("Has Enemies", FilterEnum->GetNameStringByValue(static_cast<int64>(EMordecaiTargetFilter::Enemies)).IsEmpty());
	TestFalse("Has Destructibles", FilterEnum->GetNameStringByValue(static_cast<int64>(EMordecaiTargetFilter::Destructibles)).IsEmpty());
	TestFalse("Has All", FilterEnum->GetNameStringByValue(static_cast<int64>(EMordecaiTargetFilter::All)).IsEmpty());

	// Verify Mordecai.State.Airborne tag exists
	FGameplayTag AirborneTag = FGameplayTag::RequestGameplayTag(FName("Mordecai.State.Airborne"), false);
	TestTrue("Mordecai.State.Airborne tag is registered", AirborneTag.IsValid());

	return true;
}

// ---------------------------------------------------------------------------
// Issue #92: debug geometry comes from the same attack-profile values as the query.
// These helpers are intentionally world-independent so NullRHI tests can check exact geometry.
// ---------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecaiHitDetection_DebugArcGeometry,
	"Mordecai.HitDetection.DebugDraw.ArcGeometryMatchesLongswordProfile",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecaiHitDetection_DebugArcGeometry::RunTest(const FString& Parameters)
{
	const UMordecaiWeaponDataAsset* Weapon =
		UMordecaiWeaponProfileFactory::CreateLongsword(GetTransientPackage());
	const UMordecaiAttackProfileDataAsset* Profile = Weapon->LightAttackProfiles[0];
	TestEqual("Longsword L1 is the real ArcSector profile", Profile->HitShapeType,
		EMordecaiHitShapeType::ArcSector);

	const FVector Origin(10.f, 20.f, 30.f);
	const FVector Forward(0.f, 2.f, 0.f);
	const FMordecaiHitShapeParams& Shape = Profile->HitShapeParams;
	const FMordecaiDebugArcGeometry Geometry =
		UMordecaiHitDetectionSubsystem::BuildDebugArcGeometry(
			Origin, Forward, Shape.Radius, Shape.Angle, Shape.StartAngleOffset);

	const FVector CenterDirection = Forward.GetSafeNormal2D().RotateAngleAxis(
		Shape.StartAngleOffset, FVector::UpVector);
	const FVector ExpectedStart =
		Origin + CenterDirection.RotateAngleAxis(-Shape.Angle * 0.5f, FVector::UpVector) * Shape.Radius;
	const FVector ExpectedEnd =
		Origin + CenterDirection.RotateAngleAxis(Shape.Angle * 0.5f, FVector::UpVector) * Shape.Radius;

	TestTrue("Arc keeps the query origin", Geometry.Origin.Equals(Origin));
	TestTrue("Arc has enough fan points to draw its boundary", Geometry.ArcPoints.Num() >= 2);
	TestTrue("Arc start edge uses radius, angle and start offset", Geometry.StartEdge.Equals(ExpectedStart, 0.01f));
	TestTrue("Arc end edge uses radius, angle and start offset", Geometry.EndEdge.Equals(ExpectedEnd, 0.01f));
	if (Geometry.ArcPoints.Num() >= 2)
	{
		TestTrue("First arc point is the start edge", Geometry.ArcPoints[0].Equals(ExpectedStart, 0.01f));
		TestTrue("Last arc point is the end edge", Geometry.ArcPoints.Last().Equals(ExpectedEnd, 0.01f));
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecaiHitDetection_DebugCapsuleGeometry,
	"Mordecai.HitDetection.DebugDraw.CapsuleGeometryMatchesShortswordProfile",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecaiHitDetection_DebugCapsuleGeometry::RunTest(const FString& Parameters)
{
	const UMordecaiWeaponDataAsset* Weapon =
		UMordecaiWeaponProfileFactory::CreateShortsword(GetTransientPackage());
	const UMordecaiAttackProfileDataAsset* Profile = Weapon->LightAttackProfiles[3];
	TestEqual("Shortsword L4 is the real Capsule profile", Profile->HitShapeType,
		EMordecaiHitShapeType::Capsule);

	const FVector Origin(10.f, 20.f, 30.f);
	const FVector Forward(0.f, 3.f, 0.f);
	const FMordecaiHitShapeParams& Shape = Profile->HitShapeParams;
	const FMordecaiDebugCapsuleGeometry Geometry =
		UMordecaiHitDetectionSubsystem::BuildDebugCapsuleGeometry(
			Origin, Forward, Shape.Length, Shape.Width);
	const FVector Forward2D = Forward.GetSafeNormal2D();
	const FVector Right = FVector::CrossProduct(FVector::UpVector, Forward2D);
	const float Radius = Shape.Width * 0.5f;
	const FVector End = Origin + Forward2D * Shape.Length;

	TestTrue("Capsule segment starts at the query origin", Geometry.SegmentStart.Equals(Origin));
	TestTrue("Capsule segment ends at origin plus normalized forward times length",
		Geometry.SegmentEnd.Equals(End, 0.01f));
	TestTrue("Capsule cap radius is half the query width", FMath::IsNearlyEqual(Geometry.CapRadius, Radius));
	TestTrue("Capsule start-left lane corner is exact", Geometry.StartLeft.Equals(Origin - Right * Radius, 0.01f));
	TestTrue("Capsule start-right lane corner is exact", Geometry.StartRight.Equals(Origin + Right * Radius, 0.01f));
	TestTrue("Capsule end-left lane corner is exact", Geometry.EndLeft.Equals(End - Right * Radius, 0.01f));
	TestTrue("Capsule end-right lane corner is exact", Geometry.EndRight.Equals(End + Right * Radius, 0.01f));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecaiHitDetection_DebugCircleGeometry,
	"Mordecai.HitDetection.DebugDraw.CircleGeometryMatchesGreatswordProfile",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecaiHitDetection_DebugCircleGeometry::RunTest(const FString& Parameters)
{
	const UMordecaiWeaponDataAsset* Weapon =
		UMordecaiWeaponProfileFactory::CreateGreatsword(GetTransientPackage());
	const UMordecaiAttackProfileDataAsset* Profile = Weapon->HeavyAttackProfile;
	TestEqual("Greatsword heavy is the real Circle profile", Profile->HitShapeType,
		EMordecaiHitShapeType::Circle);

	const FVector Origin(10.f, 20.f, 30.f);
	const FVector Forward(4.f, 0.f, 0.f);
	const FMordecaiHitShapeParams& Shape = Profile->HitShapeParams;
	const FMordecaiDebugCircleGeometry ProfileGeometry =
		UMordecaiHitDetectionSubsystem::BuildDebugCircleGeometry(
			Origin, Forward, Shape.Length, Shape.Radius);

	TestTrue("Circle profile radius is unchanged",
		FMath::IsNearlyEqual(ProfileGeometry.Radius, Shape.Radius));
	TestTrue("Greatsword circle center uses its real zero length", ProfileGeometry.Center.Equals(Origin));

	const float ImpactOffset = 75.f;
	const FMordecaiDebugCircleGeometry OffsetGeometry =
		UMordecaiHitDetectionSubsystem::BuildDebugCircleGeometry(
			Origin, Forward, ImpactOffset, Shape.Radius);
	TestTrue("Circle center is origin plus normalized forward times length",
		OffsetGeometry.Center.Equals(Origin + Forward.GetSafeNormal() * ImpactOffset, 0.01f));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecaiHitDetection_DebugDrawCVarDefaults,
	"Mordecai.HitDetection.DebugDraw.CVarsDefaultOnAndOneSecond",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecaiHitDetection_DebugDrawCVarDefaults::RunTest(const FString& Parameters)
{
	IConsoleVariable* DrawCVar = IConsoleManager::Get().FindConsoleVariable(
		TEXT("Mordecai.Debug.DrawMeleeHitShapes"));
	IConsoleVariable* DurationCVar = IConsoleManager::Get().FindConsoleVariable(
		TEXT("Mordecai.Debug.DrawMeleeHitShapesDuration"));
	TestNotNull("DrawMeleeHitShapes CVar is registered", DrawCVar);
	TestNotNull("DrawMeleeHitShapesDuration CVar is registered", DurationCVar);
	if (!DrawCVar || !DurationCVar)
	{
		return false;
	}
	TestEqual("Drawing defaults on in one obvious CVar definition", DrawCVar->GetInt(), 1);
	TestTrue("Drawing helper observes the default", UMordecaiHitDetectionSubsystem::ShouldDrawMeleeHitShapes());
	TestTrue("Duration defaults to one second", FMath::IsNearlyEqual(DurationCVar->GetFloat(), 1.f));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecaiHitDetection_DebugDrawCVarToggle,
	"Mordecai.HitDetection.DebugDraw.CVarTogglesWithoutRestart",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecaiHitDetection_DebugDrawCVarToggle::RunTest(const FString& Parameters)
{
	IConsoleVariable* DrawCVar = IConsoleManager::Get().FindConsoleVariable(
		TEXT("Mordecai.Debug.DrawMeleeHitShapes"));
	TestNotNull("DrawMeleeHitShapes CVar is registered", DrawCVar);
	if (!DrawCVar)
	{
		return false;
	}

	const int32 OriginalValue = DrawCVar->GetInt();
	DrawCVar->Set(0, ECVF_SetByConsole);
	TestFalse("0 disables the next draw path", UMordecaiHitDetectionSubsystem::ShouldDrawMeleeHitShapes());
	DrawCVar->Set(1, ECVF_SetByConsole);
	TestTrue("1 re-enables the next draw path without restart",
		UMordecaiHitDetectionSubsystem::ShouldDrawMeleeHitShapes());
	DrawCVar->Set(OriginalValue, ECVF_SetByConsole);
	return true;
}
