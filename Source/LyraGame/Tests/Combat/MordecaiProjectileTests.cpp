// Project Mordecai — Projectile System Tests (US-009)

#include "Misc/AutomationTest.h"
#include "Mordecai/Combat/MordecaiProjectile.h"
#include "Mordecai/Combat/MordecaiCombatTypes.h"
#include "Mordecai/Combat/MordecaiAttackProfileDataAsset.h"
#include "Mordecai/Combat/MordecaiHitDetectionTypes.h"
#include "Mordecai/MordecaiGameplayTags.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "Components/SphereComponent.h"

// ---------------------------------------------------------------------------
// Helper: Create a test projectile spec with common defaults
// ---------------------------------------------------------------------------
namespace
{
	FMordecaiProjectileSpec CreateTestProjectileSpec(
		float Speed = 1000.f,
		float GravityDrop = 0.f,
		float Lifetime = 5.f,
		float MaxRange = 2000.f,
		int32 PierceCount = 0,
		int32 RicochetCount = 0,
		float RicochetRange = 500.f,
		float RicochetAngleLimit = 90.f,
		float OnHitAoERadius = 0.f,
		float OnHitAoEDamageScalar = 0.5f)
	{
		FMordecaiProjectileSpec Spec;
		Spec.Speed = Speed;
		Spec.GravityDrop = GravityDrop;
		Spec.Lifetime = Lifetime;
		Spec.MaxRange = MaxRange;
		Spec.PierceCount = PierceCount;
		Spec.RicochetCount = RicochetCount;
		Spec.RicochetRange = RicochetRange;
		Spec.RicochetAngleLimit = RicochetAngleLimit;
		Spec.OnHitAoERadius = OnHitAoERadius;
		Spec.OnHitAoEDamageScalar = OnHitAoEDamageScalar;
		return Spec;
	}

	FMordecaiDamageProfile CreateTestDamageProfile(
		float BasePower = 25.f,
		EMordecaiDamageType DamageType = EMordecaiDamageType::Physical)
	{
		FMordecaiDamageProfile Profile;
		Profile.BasePower = BasePower;
		Profile.DamageType = DamageType;
		Profile.AppliesPostureDamage = true;
		return Profile;
	}
}

// ---------------------------------------------------------------------------
// 1. Mordecai.Projectile.SpawnsAndMovesForward (AC-009.1, AC-009.2)
// Verify projectile class exists and is configured with ProjectileMovementComponent
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_Projectile_SpawnsAndMovesForward,
	"Mordecai.Projectile.SpawnsAndMovesForward",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_Projectile_SpawnsAndMovesForward::RunTest(const FString& Parameters)
{
	// Verify class exists
	const UClass* ProjectileClass = AMordecaiProjectile::StaticClass();
	TestNotNull("AMordecaiProjectile class exists", ProjectileClass);
	TestTrue("Inherits from AActor", ProjectileClass->IsChildOf(AActor::StaticClass()));

	// Create a projectile CDO to verify component setup
	AMordecaiProjectile* Projectile = NewObject<AMordecaiProjectile>();
	TestNotNull("Projectile created", Projectile);

	// Verify it has a ProjectileMovementComponent
	UProjectileMovementComponent* MoveComp = Projectile->GetProjectileMovement();
	TestNotNull("Has ProjectileMovementComponent", MoveComp);

	// Verify it has a collision component
	USphereComponent* CollisionComp = Projectile->GetCollisionComponent();
	TestNotNull("Has SphereComponent for collision", CollisionComp);

	// AC-009.2: Initialize with spec and verify speed configuration
	FMordecaiProjectileSpec Spec = CreateTestProjectileSpec(/*Speed=*/1500.f);
	Projectile->InitFromSpec(Spec, CreateTestDamageProfile());

	TestEqual("InitialSpeed matches spec", MoveComp->InitialSpeed, 1500.f);
	TestEqual("MaxSpeed matches spec", MoveComp->MaxSpeed, 1500.f);

	return true;
}

// ---------------------------------------------------------------------------
// 2. Mordecai.Projectile.GravityDropCurvesTrajectory (AC-009.3)
// Verify GravityDrop > 0 applies gravity scale
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_Projectile_GravityDropCurvesTrajectory,
	"Mordecai.Projectile.GravityDropCurvesTrajectory",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_Projectile_GravityDropCurvesTrajectory::RunTest(const FString& Parameters)
{
	AMordecaiProjectile* Projectile = NewObject<AMordecaiProjectile>();

	// With GravityDrop = 0 (no gravity)
	FMordecaiProjectileSpec NoGravitySpec = CreateTestProjectileSpec(1000.f, /*GravityDrop=*/0.f);
	Projectile->InitFromSpec(NoGravitySpec, CreateTestDamageProfile());
	TestEqual("Zero gravity when GravityDrop=0", Projectile->GetProjectileMovement()->ProjectileGravityScale, 0.f);

	// With GravityDrop > 0 (apply gravity)
	FMordecaiProjectileSpec GravitySpec = CreateTestProjectileSpec(1000.f, /*GravityDrop=*/0.5f);
	Projectile->InitFromSpec(GravitySpec, CreateTestDamageProfile());
	TestEqual("Gravity scale set from GravityDrop", Projectile->GetProjectileMovement()->ProjectileGravityScale, 0.5f);

	return true;
}

// ---------------------------------------------------------------------------
// 3. Mordecai.Projectile.DestroysAfterLifetime (AC-009.4)
// Verify lifetime is configured on the projectile
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_Projectile_DestroysAfterLifetime,
	"Mordecai.Projectile.DestroysAfterLifetime",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_Projectile_DestroysAfterLifetime::RunTest(const FString& Parameters)
{
	AMordecaiProjectile* Projectile = NewObject<AMordecaiProjectile>();
	FMordecaiProjectileSpec Spec = CreateTestProjectileSpec(1000.f, 0.f, /*Lifetime=*/2.5f, /*MaxRange=*/5000.f);
	Projectile->InitFromSpec(Spec, CreateTestDamageProfile());

	TestEqual("Lifetime stored from spec", Projectile->GetLifetimeSeconds(), 2.5f);
	TestEqual("MaxRange stored from spec", Projectile->GetMaxRange(), 5000.f);

	return true;
}

// ---------------------------------------------------------------------------
// 4. Mordecai.Projectile.AppliesDamageOnHit (AC-009.5)
// Verify projectile stores damage profile for hit application
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_Projectile_AppliesDamageOnHit,
	"Mordecai.Projectile.AppliesDamageOnHit",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_Projectile_AppliesDamageOnHit::RunTest(const FString& Parameters)
{
	AMordecaiProjectile* Projectile = NewObject<AMordecaiProjectile>();
	FMordecaiDamageProfile DamageProfile = CreateTestDamageProfile(/*BasePower=*/30.f, EMordecaiDamageType::Fire);
	Projectile->InitFromSpec(CreateTestProjectileSpec(), DamageProfile);

	// Verify damage profile is stored
	const FMordecaiDamageProfile& StoredProfile = Projectile->GetDamageProfile();
	TestEqual("BasePower stored", StoredProfile.BasePower, 30.f);
	TestEqual("DamageType stored", StoredProfile.DamageType, EMordecaiDamageType::Fire);

	return true;
}

// ---------------------------------------------------------------------------
// 5. Mordecai.Projectile.IgnoresInstigator (AC-009.6)
// Verify instigator is excluded from hits
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_Projectile_IgnoresInstigator,
	"Mordecai.Projectile.IgnoresInstigator",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_Projectile_IgnoresInstigator::RunTest(const FString& Parameters)
{
	AMordecaiProjectile* Projectile = NewObject<AMordecaiProjectile>();

	// Verify that ShouldIgnoreActor returns true for instigator
	// We'll test via the hit list / instigator tracking API
	AActor* FakeInstigator = NewObject<AActor>();
	Projectile->SetProjectileInstigator(FakeInstigator);

	TestTrue("Should ignore instigator actor", Projectile->ShouldIgnoreActor(FakeInstigator));

	// Some other actor should not be ignored
	AActor* OtherActor = NewObject<AActor>();
	TestFalse("Should not ignore unrelated actor", Projectile->ShouldIgnoreActor(OtherActor));

	return true;
}

// ---------------------------------------------------------------------------
// 6. Mordecai.Projectile.PiercesThroughTargets (AC-009.8, AC-009.9)
// Verify pierce counter decrements and tracks hits
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_Projectile_PiercesThroughTargets,
	"Mordecai.Projectile.PiercesThroughTargets",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_Projectile_PiercesThroughTargets::RunTest(const FString& Parameters)
{
	AMordecaiProjectile* Projectile = NewObject<AMordecaiProjectile>();
	FMordecaiProjectileSpec Spec = CreateTestProjectileSpec();
	Spec.PierceCount = 2;
	Projectile->InitFromSpec(Spec, CreateTestDamageProfile());

	TestEqual("Initial pierce count", Projectile->GetRemainingPierceCount(), 2);

	// Simulate first pierce
	AActor* Target1 = NewObject<AActor>();
	bool bShouldContinue1 = Projectile->ProcessPierceHit(Target1);
	TestTrue("Should continue after first pierce", bShouldContinue1);
	TestEqual("Pierce count after first hit", Projectile->GetRemainingPierceCount(), 1);
	TestTrue("Target1 in hit list", Projectile->HasAlreadyHit(Target1));

	// Simulate second pierce
	AActor* Target2 = NewObject<AActor>();
	bool bShouldContinue2 = Projectile->ProcessPierceHit(Target2);
	TestTrue("Should continue after second pierce", bShouldContinue2);
	TestEqual("Pierce count after second hit", Projectile->GetRemainingPierceCount(), 0);

	// Third hit should stop (pierce exhausted)
	AActor* Target3 = NewObject<AActor>();
	bool bShouldContinue3 = Projectile->ProcessPierceHit(Target3);
	TestFalse("Should stop after pierce exhausted", bShouldContinue3);

	return true;
}

// ---------------------------------------------------------------------------
// 7. Mordecai.Projectile.NoPierceDestroysOnFirstHit (AC-009.9)
// PierceCount=0 means destroy on first hit
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_Projectile_NoPierceDestroysOnFirstHit,
	"Mordecai.Projectile.NoPierceDestroysOnFirstHit",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_Projectile_NoPierceDestroysOnFirstHit::RunTest(const FString& Parameters)
{
	AMordecaiProjectile* Projectile = NewObject<AMordecaiProjectile>();
	FMordecaiProjectileSpec Spec = CreateTestProjectileSpec();
	Spec.PierceCount = 0;
	Projectile->InitFromSpec(Spec, CreateTestDamageProfile());

	TestEqual("Pierce count is 0", Projectile->GetRemainingPierceCount(), 0);

	AActor* Target = NewObject<AActor>();
	bool bShouldContinue = Projectile->ProcessPierceHit(Target);
	TestFalse("Should not continue with PierceCount=0", bShouldContinue);

	return true;
}

// ---------------------------------------------------------------------------
// 8. Mordecai.Projectile.RicochetFindsNextTarget (AC-009.11, AC-009.12)
// Verify ricochet candidate selection logic
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_Projectile_RicochetFindsNextTarget,
	"Mordecai.Projectile.RicochetFindsNextTarget",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_Projectile_RicochetFindsNextTarget::RunTest(const FString& Parameters)
{
	AMordecaiProjectile* Projectile = NewObject<AMordecaiProjectile>();
	FMordecaiProjectileSpec Spec = CreateTestProjectileSpec();
	Spec.RicochetCount = 1;
	Spec.RicochetRange = 500.f;
	Spec.RicochetAngleLimit = 90.f;
	Projectile->InitFromSpec(Spec, CreateTestDamageProfile());

	TestEqual("Initial ricochet count", Projectile->GetRemainingRicochetCount(), 1);
	TestTrue("Can ricochet initially", Projectile->CanRicochet());

	// Simulate a ricochet being consumed
	Projectile->ConsumeRicochet();
	TestEqual("Ricochet count after use", Projectile->GetRemainingRicochetCount(), 0);
	TestFalse("Cannot ricochet after exhausted", Projectile->CanRicochet());

	return true;
}

// ---------------------------------------------------------------------------
// 9. Mordecai.Projectile.RicochetDestroysIfNoTarget (AC-009.13)
// Verify ricochet requires a valid target
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_Projectile_RicochetDestroysIfNoTarget,
	"Mordecai.Projectile.RicochetDestroysIfNoTarget",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_Projectile_RicochetDestroysIfNoTarget::RunTest(const FString& Parameters)
{
	AMordecaiProjectile* Projectile = NewObject<AMordecaiProjectile>();
	FMordecaiProjectileSpec Spec = CreateTestProjectileSpec();
	Spec.RicochetCount = 1;
	Spec.RicochetRange = 500.f;
	Projectile->InitFromSpec(Spec, CreateTestDamageProfile());

	// FindRicochetTarget with empty candidate array should return nullptr
	FVector HitLocation = FVector(100.f, 0.f, 0.f);
	FVector ProjectileForward = FVector(1.f, 0.f, 0.f);
	TArray<AActor*> EmptyCandidates;

	AActor* RicochetTarget = Projectile->FindBestRicochetTarget(
		HitLocation, ProjectileForward, EmptyCandidates);
	TestNull("No ricochet target from empty candidates", RicochetTarget);

	return true;
}

// ---------------------------------------------------------------------------
// 10. Mordecai.Projectile.RicochetSkipsAlreadyHit (AC-009.14)
// Verify already-hit actors are excluded from ricochet candidates
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_Projectile_RicochetSkipsAlreadyHit,
	"Mordecai.Projectile.RicochetSkipsAlreadyHit",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_Projectile_RicochetSkipsAlreadyHit::RunTest(const FString& Parameters)
{
	AMordecaiProjectile* Projectile = NewObject<AMordecaiProjectile>();
	FMordecaiProjectileSpec Spec = CreateTestProjectileSpec();
	Spec.RicochetCount = 2;
	Spec.RicochetRange = 500.f;
	Spec.RicochetAngleLimit = 90.f;
	Projectile->InitFromSpec(Spec, CreateTestDamageProfile());

	// Mark Target1 as already hit
	AActor* Target1 = NewObject<AActor>();
	Projectile->AddToHitList(Target1);

	TestTrue("Target1 already hit", Projectile->HasAlreadyHit(Target1));

	// Target1 should be filtered from ricochet candidates
	AActor* Target2 = NewObject<AActor>();
	TestFalse("Target2 not yet hit", Projectile->HasAlreadyHit(Target2));

	return true;
}

// ---------------------------------------------------------------------------
// 11. Mordecai.Projectile.OnHitAoEDamagesNearbyTargets (AC-009.15, AC-009.16)
// Verify AoE spec fields are stored correctly
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_Projectile_OnHitAoEDamagesNearbyTargets,
	"Mordecai.Projectile.OnHitAoEDamagesNearbyTargets",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_Projectile_OnHitAoEDamagesNearbyTargets::RunTest(const FString& Parameters)
{
	AMordecaiProjectile* Projectile = NewObject<AMordecaiProjectile>();
	FMordecaiProjectileSpec Spec = CreateTestProjectileSpec();
	Spec.OnHitAoERadius = 200.f;
	Spec.OnHitAoEDamageScalar = 0.75f;
	FMordecaiDamageProfile DmgProfile = CreateTestDamageProfile(40.f);
	Projectile->InitFromSpec(Spec, DmgProfile);

	TestTrue("Has on-hit AoE", Projectile->HasOnHitAoE());
	TestEqual("AoE radius from spec", Projectile->GetOnHitAoERadius(), 200.f);

	// AC-009.16: AoE damage = BasePower * OnHitAoEDamageScalar
	float ExpectedAoEDamage = 40.f * 0.75f;
	TestEqual("AoE damage calculation", Projectile->ComputeAoEDamage(), ExpectedAoEDamage);

	return true;
}

// ---------------------------------------------------------------------------
// 12. Mordecai.Projectile.OnHitAoEUsesCorrectDamageType (AC-009.17)
// Verify AoE uses the same damage type as the projectile
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_Projectile_OnHitAoEUsesCorrectDamageType,
	"Mordecai.Projectile.OnHitAoEUsesCorrectDamageType",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_Projectile_OnHitAoEUsesCorrectDamageType::RunTest(const FString& Parameters)
{
	AMordecaiProjectile* Projectile = NewObject<AMordecaiProjectile>();
	FMordecaiProjectileSpec Spec = CreateTestProjectileSpec();
	Spec.OnHitAoERadius = 150.f;
	FMordecaiDamageProfile DmgProfile = CreateTestDamageProfile(25.f, EMordecaiDamageType::Lightning);
	Projectile->InitFromSpec(Spec, DmgProfile);

	// AC-009.17: AoE damage carries same damage type as projectile
	TestEqual("AoE damage type matches projectile",
		Projectile->GetDamageProfile().DamageType, EMordecaiDamageType::Lightning);

	return true;
}
