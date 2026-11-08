// Project Mordecai — US-087 Fireball recovery regression tests (issue #20)

#include "Misc/AutomationTest.h"

#include "AbilitySystemComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "EngineUtils.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "GameFramework/WorldSettings.h"
#include "Mordecai/Combat/MordecaiHitDetectionTypes.h"
#include "Mordecai/Combat/MordecaiProjectile.h"
#include "Mordecai/Magic/MordecaiGA_Fireball.h"
#include "Mordecai/Magic/MordecaiSpellDataAsset.h"
#include "Mordecai/MordecaiGameplayTags.h"
#include "Mordecai/StatusEffects/Effects/MordecaiGE_Poisoned.h"
#include "MordecaiRecoveryProjectileTestHelpers.h"
#include "MordecaiRecoveryTestEnv.h"
#include "TimerManager.h"

namespace
{
	using namespace MordecaiRecoveryTest;

	constexpr float TickStep = 0.02f;

	/**
	 * A deliberately isolated world whose world-level BeginPlay state is real. The shared recovery fixture remains
	 * unbegun because many recovery tests manually control actor lifecycle and ticking.
	 */
	struct FBegunProjectileTestEnv
	{
		UWorld* World = nullptr;
		uint64 SavedFrameCounter = 0;

		bool Setup()
		{
			if (!GEngine)
			{
				return false;
			}
			SavedFrameCounter = GFrameCounter;
			World = UWorld::CreateWorld(EWorldType::Game, false);
			if (!World)
			{
				return false;
			}
			FWorldContext& WorldContext = GEngine->CreateNewWorldContext(EWorldType::Game);
			WorldContext.SetCurrentWorld(World);
			World->InitializeActorsForPlay(FURL());

			// AWorldSettings::NotifyBeginPlay is the engine path that dispatches actor BeginPlay and then marks
			// the world begun. UWorld::BeginPlay alone only reaches it through a GameMode/GameState, which this
			// minimal no-GameInstance fixture intentionally does not create.
			AWorldSettings* WorldSettings = World->GetWorldSettings();
			if (!WorldSettings)
			{
				Teardown();
				return false;
			}
			WorldSettings->NotifyBeginPlay();
			return World->HasBegunPlay();
		}

		void Tick(float Seconds) const
		{
			for (float Elapsed = 0.f; Elapsed < Seconds; Elapsed += TickStep)
			{
				World->Tick(LEVELTICK_All, TickStep);
				// This asset-free world has no level collision scene to refresh query-only overlap pairs for us.
				// Ask the real primitive collision system to refresh them after movement. In a begun world this reaches
				// BeginComponentOverlap's delegate broadcast; the old unbegun fixture stopped at its overlap list.
				for (TActorIterator<AMordecaiProjectile> It(World); It; ++It)
				{
					if (!It->IsActorBeingDestroyed())
					{
						It->GetCollisionComponent()->UpdateOverlaps();
					}
				}
				++GFrameCounter;
			}
		}

		void Teardown()
		{
			if (World && GEngine)
			{
				if (World->HasBegunPlay())
				{
					World->EndPlay(EEndPlayReason::Quit);
				}
				GEngine->DestroyWorldContext(World);
				World->DestroyWorld(false);
			}
			World = nullptr;
			GFrameCounter = SavedFrameCounter;
		}
	};

	FMordecaiProjectileSpec MakeSpec(
		float Speed = 1234.f, float Lifetime = 2.f, float MaxRange = 1500.f, int32 Pierce = 0)
	{
		FMordecaiProjectileSpec Spec;
		Spec.Speed = Speed;
		Spec.Lifetime = Lifetime;
		Spec.MaxRange = MaxRange;
		Spec.PierceCount = Pierce;
		return Spec;
	}

	FMordecaiDamageProfile MakeDamage(float Power = 20.f)
	{
		FMordecaiDamageProfile Damage;
		Damage.DamageType = EMordecaiDamageType::Fire;
		Damage.BasePower = Power;
		return Damage;
	}

	AMordecaiProjectile* SpawnConfiguredProjectile(
		FRecoveryTestEnv& Env,
		const FVector& Location,
		const FVector& Direction,
		const FMordecaiProjectileSpec& Spec,
		const FMordecaiDamageProfile& Damage)
	{
		AMordecaiProjectile* Projectile = Env.World->SpawnActor<AMordecaiProjectile>(
			AMordecaiProjectile::StaticClass(), Location, Direction.Rotation(), FRecoveryTestEnv::AlwaysSpawn());
		if (!Projectile)
		{
			return nullptr;
		}
		Projectile->InitFromSpec(Spec, Damage);
		Projectile->SetProjectileInstigator(Env.Player);
		if (!Projectile->HasActorBegunPlay())
		{
			Projectile->DispatchBeginPlay();
		}
		return Projectile;
	}

	AMordecaiProjectile* SpawnDeferredConfiguredProjectile(
		FRecoveryTestEnv& Env,
		const FVector& Location,
		const FVector& Direction,
		const FMordecaiProjectileSpec& Spec,
		const FMordecaiDamageProfile& Damage)
	{
		const FTransform SpawnTransform(Direction.Rotation(), Location);
		AMordecaiProjectile* Projectile = Env.World->SpawnActorDeferred<AMordecaiProjectile>(
			AMordecaiProjectile::StaticClass(), SpawnTransform, Env.Player, Env.Player,
			ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
		if (!Projectile)
		{
			return nullptr;
		}
		Projectile->InitFromSpec(Spec, Damage);
		Projectile->SetProjectileInstigator(Env.Player);
		Projectile->FinishSpawning(SpawnTransform);
		return Projectile;
	}

	AActor* SpawnCollisionSphere(
		UWorld* World,
		const FVector& Location,
		ECollisionChannel ObjectType,
		ECollisionResponse ProjectileResponse,
		float Radius = 35.f)
	{
		AActor* Actor = World->SpawnActor<AActor>(AActor::StaticClass(), Location, FRotator::ZeroRotator,
			FRecoveryTestEnv::AlwaysSpawn());
		if (!Actor)
		{
			return nullptr;
		}
		USphereComponent* Sphere = NewObject<USphereComponent>(Actor, TEXT("ProjectileTestSphere"));
		Actor->SetRootComponent(Sphere);
		Actor->AddInstanceComponent(Sphere);
		Sphere->InitSphereRadius(Radius);
		Sphere->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
		Sphere->SetGenerateOverlapEvents(true);
		Sphere->SetCollisionObjectType(ObjectType);
		Sphere->SetCollisionResponseToAllChannels(ECR_Ignore);
		Sphere->SetCollisionResponseToChannel(Mordecai_TraceChannel_ProjectileDetection, ProjectileResponse);
		Sphere->RegisterComponent();
		Actor->SetActorLocation(Location);
		return Actor;
	}

	AActor* SpawnBegunOverlapSphere(UWorld* World, const FVector& Location)
	{
		const FTransform Transform(FRotator::ZeroRotator, Location);
		AActor* Actor = World->SpawnActorDeferred<AActor>(
			AActor::StaticClass(), Transform, nullptr, nullptr, ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
		if (!Actor)
		{
			return nullptr;
		}
		USphereComponent* Sphere = NewObject<USphereComponent>(Actor, TEXT("NonASCOverlapSphere"));
		Actor->SetRootComponent(Sphere);
		Actor->AddInstanceComponent(Sphere);
		Sphere->InitSphereRadius(35.f);
		Sphere->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
		Sphere->SetGenerateOverlapEvents(true);
		Sphere->SetCollisionObjectType(ECC_WorldDynamic);
		Sphere->SetCollisionResponseToAllChannels(ECR_Ignore);
		Sphere->SetCollisionResponseToChannel(Mordecai_TraceChannel_ProjectileDetection, ECR_Overlap);
		Actor->FinishSpawning(Transform);
		if (!Sphere->IsRegistered())
		{
			Sphere->RegisterComponent();
		}
		Sphere->SetWorldLocation(Location);
		return Actor;
	}

	void AddProjectileOverlapSphere(AActor* Actor, float Radius = 45.f)
	{
		if (!Actor)
		{
			return;
		}
		Actor->SetActorEnableCollision(true);
		// The lightweight world does not build a map collision profile for character capsules.
		// Register an explicit Pawn blocking shape on the real ASC actor so swept movement reaches
		// OnProjectileStop, the collision path used by Pawn targets in the package.
		USphereComponent* Sphere = NewObject<USphereComponent>(Actor);
		Actor->AddInstanceComponent(Sphere);
		Sphere->SetupAttachment(Actor->GetRootComponent());
		Sphere->InitSphereRadius(Radius);
		Sphere->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
		Sphere->SetGenerateOverlapEvents(true);
		Sphere->SetCollisionObjectType(ECC_Pawn);
		Sphere->SetCollisionResponseToAllChannels(ECR_Ignore);
		Sphere->SetCollisionResponseToChannel(Mordecai_TraceChannel_ProjectileDetection, ECR_Block);
		Sphere->RegisterComponent();
		Sphere->SetWorldLocation(Actor->GetActorLocation());
		Sphere->UpdateOverlaps();
	}

	void TickWorld(UWorld* World, float Seconds)
	{
		for (float Elapsed = 0.f; Elapsed < Seconds; Elapsed += TickStep)
		{
			// This lightweight world never begins play, so it has no normal level tick. Advance
			// the same boundaries explicitly: spell/lifespan timers, then swept projectile
			// movement, then actor Tick (the MaxRange contract). This is component-level delivery
			// coverage; the packaged Tier 4 run supplies the integrated game-world proof.
			++GFrameCounter;
			World->TimeSeconds += TickStep;
			World->GetTimerManager().Tick(TickStep);
			TArray<TWeakObjectPtr<AMordecaiProjectile>> Projectiles;
			for (TActorIterator<AMordecaiProjectile> It(World); It; ++It)
			{
				Projectiles.Add(*It);
			}
			for (const TWeakObjectPtr<AMordecaiProjectile>& WeakProjectile : Projectiles)
			{
				if (AMordecaiProjectile* Projectile = WeakProjectile.Get())
				{
					Projectile->GetProjectileMovement()->TickComponent(
						TickStep, LEVELTICK_All, nullptr);
					if (IsValid(Projectile) && !Projectile->IsActorBeingDestroyed())
					{
						// A normal begun world does this as part of its component update. The
						// recovery world is deliberately unbegun, so refresh the collision
						// component's natural overlap set after its swept movement.
						Projectile->GetCollisionComponent()->UpdateOverlaps();
						Projectile->Tick(TickStep);
					}
				}
			}
		}
	}

	void TickTimersOnly(UWorld* World, float Seconds)
	{
		for (float Elapsed = 0.f; Elapsed < Seconds; Elapsed += TickStep)
		{
			++GFrameCounter;
			World->TimeSeconds += TickStep;
			World->GetTimerManager().Tick(TickStep);
		}
	}

	AMordecaiProjectile* FindLiveProjectile(UWorld* World)
	{
		for (TActorIterator<AMordecaiProjectile> It(World); It; ++It)
		{
			if (!It->IsActorBeingDestroyed())
			{
				return *It;
			}
		}
		return nullptr;
	}

	UMordecaiSpellDataAsset* MakeFireballData()
	{
		UMordecaiSpellDataAsset* Data = NewObject<UMordecaiSpellDataAsset>();
		Data->SpellId = TEXT("Fireball");
		Data->SpellPointCost = 0;
		Data->BasePower = 25.f;
		Data->Range = 1000.f;
		Data->ProjectileSpeed = 1200.f;
		Data->ProjectileLifetime = 2.f;
		Data->ProjectileCollisionRadius = 20.f;
		Data->WindupTime = 0.05f;
		Data->CastTime = 0.05f;
		Data->RecoveryTime = 0.05f;
		Data->DeliveryType = EMordecaiSpellDeliveryType::SpawnProjectile;
		Data->TargetingType = EMordecaiSpellTargetingType::Projectile;
		return Data;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecaiRecovery_ProjectileLifecycle,
	"Mordecai.Recovery.Fireball.ProjectileMovesAndExpiresFromConfiguredContract",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecaiRecovery_ProjectileLifecycle::RunTest(const FString& Parameters)
{
	FRecoveryTestEnv Env;
	if (!TestTrue("test world", Env.Setup(FVector::ZeroVector)))
	{
		return true;
	}

	const FVector Direction = FVector(0.6f, 0.8f, 0.f);
	AMordecaiProjectile* Moving = SpawnConfiguredProjectile(
		Env, FVector(0.f, 0.f, 100.f), Direction, MakeSpec(1234.f, 2.f, 1500.f), MakeDamage());
	if (TestNotNull("configured projectile", Moving))
	{
		TestTrue("active velocity uses configured direction and speed",
			Moving->GetProjectileMovement()->Velocity.Equals(Direction * 1234.f, 1.f));
		const FVector Start = Moving->GetActorLocation();
		TickWorld(Env.World, 0.1f);
		TestTrue("projectile displaces on its first world ticks",
			FVector::Dist(Start, Moving->GetActorLocation()) > 80.f);
	}

	const FVector DeferredDirection = FVector::RightVector;
	AMordecaiProjectile* Deferred = SpawnDeferredConfiguredProjectile(
		Env, FVector(0.f, -600.f, 100.f), DeferredDirection,
		MakeSpec(1234.f, 2.f, 1500.f), MakeDamage());
	if (TestNotNull("deferred non-axis projectile", Deferred))
	{
		TestTrue("deferred FinishSpawning preserves the world-space aim velocity",
			Deferred->GetProjectileMovement()->Velocity.Equals(DeferredDirection * 1234.f, 1.f));
		const FVector Start = Deferred->GetActorLocation();
		TickWorld(Env.World, 0.1f);
		const FVector Displacement = Deferred->GetActorLocation() - Start;
		TestTrue("deferred non-axis first movement follows the supplied world direction",
			FVector::DotProduct(Displacement.GetSafeNormal(), DeferredDirection) > 0.99f);
	}

	AMordecaiProjectile* Lifetime = SpawnConfiguredProjectile(
		Env, FVector(0.f, 300.f, 100.f), FVector::ForwardVector, MakeSpec(0.f, 0.08f, 0.f), MakeDamage());
	TWeakObjectPtr<AMordecaiProjectile> LifetimeWeak = Lifetime;
	TickWorld(Env.World, 0.12f);
	TestFalse("configured lifetime destroys projectile", LifetimeWeak.IsValid());

	AMordecaiProjectile* Range = SpawnConfiguredProjectile(
		Env, FVector(0.f, -300.f, 100.f), FVector::ForwardVector, MakeSpec(1000.f, 2.f, 80.f), MakeDamage());
	TWeakObjectPtr<AMordecaiProjectile> RangeWeak = Range;
	if (Range)
	{
		// Isolate range enforcement from the velocity regression: the movement component still
		// performs the natural swept move, while actor Tick must enforce MaxRange.
		Range->GetProjectileMovement()->Velocity = FVector(1000.f, 0.f, 0.f);
	}
	TickWorld(Env.World, 0.12f);
	TestFalse("configured maximum range destroys projectile", RangeWeak.IsValid());

	Env.Teardown();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecaiRecovery_ProjectileOverlapFiltering,
	"Mordecai.Recovery.Fireball.NonASCOverlapPassesThroughBeforeNaturalDamageAndStatus",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecaiRecovery_ProjectileOverlapFiltering::RunTest(const FString& Parameters)
{
	FBegunProjectileTestEnv Env;
	if (!TestTrue("isolated test world reached world-level BeginPlay", Env.Setup()))
	{
		return true;
	}

	AActor* NonASC = SpawnBegunOverlapSphere(Env.World, FVector(180.f, 0.f, 100.f));
	AMordecaiRecoveryProjectileASCTestPawn* Target =
		Env.World->SpawnActor<AMordecaiRecoveryProjectileASCTestPawn>(
			AMordecaiRecoveryProjectileASCTestPawn::StaticClass(), FVector(420.f, 0.f, 100.f),
			FRotator::ZeroRotator, FRecoveryTestEnv::AlwaysSpawn());
	UAbilitySystemComponent* TargetASC = Target ? Target->GetAbilitySystemComponent() : nullptr;
	if (!TestNotNull("registered non-ASC overlap fixture", NonASC)
		|| !TestNotNull("ASC Pawn target", TargetASC))
	{
		Env.Teardown();
		return true;
	}

	TestTrue("ASC target began play because it spawned into a begun world", Target->HasActorBegunPlay());
	TestEqual("ASC target is a real Pawn collision object",
		Target->GetCollision()->GetCollisionObjectType(), ECC_Pawn);
	TestEqual("ASC target blocks the projectile channel",
		Target->GetCollision()->GetCollisionResponseToChannel(Mordecai_TraceChannel_ProjectileDetection), ECR_Block);

	const float HealthBefore = TargetASC->GetNumericAttribute(UMordecaiAttributeSet::GetHealthAttribute());
	UMordecaiRecoveryProjectileOverlapSpy* OverlapSpy =
		NewObject<UMordecaiRecoveryProjectileOverlapSpy>(GetTransientPackage());
	OverlapSpy->ExpectedActor = NonASC;
	const FTransform ProjectileTransform(FVector::ForwardVector.Rotation(), FVector(0.f, 0.f, 100.f));
	AMordecaiProjectile* Projectile = Env.World->SpawnActorDeferred<AMordecaiProjectile>(
		AMordecaiProjectile::StaticClass(), ProjectileTransform, nullptr, nullptr,
		ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
	if (Projectile)
	{
		Projectile->InitFromSpec(MakeSpec(1000.f), MakeDamage(20.f));
		// Bind before FinishSpawning: the spy observes the same engine broadcast before the projectile's BeginPlay
		// adds its production handler. It never invokes that handler itself.
		Projectile->GetCollisionComponent()->OnComponentBeginOverlap.AddDynamic(
			OverlapSpy, &UMordecaiRecoveryProjectileOverlapSpy::HandleComponentBeginOverlap);
		FMordecaiStatusOnHitEntry Poison;
		Poison.StatusEffectTag = MordecaiGameplayTags::Status_Poisoned;
		Poison.ApplicationChance = 1.f;
		Poison.StatusEffectGEClass = UMordecaiGE_Poisoned::StaticClass();
		Projectile->SetStatusEffectsOnHit({ Poison });
		Projectile->FinishSpawning(ProjectileTransform);
	}
	if (!TestNotNull("configured observed projectile", Projectile))
	{
		Env.Teardown();
		return true;
	}
	TestTrue("projectile began play because it finished spawning into a begun world", Projectile->HasActorBegunPlay());
	TestEqual("projectile uses real Pawn blocking",
		Projectile->GetCollisionComponent()->GetCollisionResponseToChannel(ECC_Pawn), ECR_Block);

	// Move the real root primitive through the non-ASC query shape. MoveComponent owns overlap discovery and
	// UPrimitiveComponent owns delegate delivery; the test never invokes the projectile callback.
	FHitResult NonASCSweep;
	Projectile->GetCollisionComponent()->MoveComponent(
		FVector(180.f, 0.f, 0.f), Projectile->GetActorQuat(), true, &NonASCSweep);
	Projectile->GetCollisionComponent()->UpdateOverlaps();
	const bool bAliveAfterNonASC = IsValid(Projectile) && !Projectile->IsActorBeingDestroyed();
	const bool bNonASCEnteredHitList = Projectile->HasAlreadyHit(NonASC);
	AddInfo(FString::Printf(TEXT("begun-world callback diagnostic: world-begun=%d callback-count=%d "
		"non-asc-hit-list=%d projectile-alive=%d projectile-bound=%d pawn-response=%d "
		"dynamic-response=%d x=%.1f target-health=%.1f"),
		Env.World->HasBegunPlay(), OverlapSpy->ExpectedActorCallbacks, bNonASCEnteredHitList, bAliveAfterNonASC,
		Projectile->GetCollisionComponent()->OnComponentBeginOverlap.IsBound(),
		static_cast<int32>(Projectile->GetCollisionComponent()->GetCollisionResponseToChannel(ECC_Pawn)),
		static_cast<int32>(Projectile->GetCollisionComponent()->GetCollisionResponseToChannel(ECC_WorldDynamic)),
		Projectile ? Projectile->GetActorLocation().X : -1.f,
		TargetASC->GetNumericAttribute(UMordecaiAttributeSet::GetHealthAttribute())));

	TestEqual("the real non-ASC component overlap callback fired once",
		OverlapSpy->ExpectedActorCallbacks, 1);
	TestFalse("production filter keeps the non-ASC actor out of the gameplay hit list",
		bNonASCEnteredHitList);
	TestTrue("zero-pierce projectile survives the real non-ASC callback", bAliveAfterNonASC);
	TestEqual("ASC Pawn is still untouched immediately after the earlier non-ASC callback",
		TargetASC->GetNumericAttribute(UMordecaiAttributeSet::GetHealthAttribute()), HealthBefore);

	for (int32 Step = 0; Step < 30
		&& TargetASC->GetNumericAttribute(UMordecaiAttributeSet::GetHealthAttribute()) >= HealthBefore; ++Step)
	{
		Env.Tick(TickStep);
	}
	TestTrue("surviving projectile subsequently damages the blocking ASC Pawn",
		TargetASC->GetNumericAttribute(UMordecaiAttributeSet::GetHealthAttribute()) < HealthBefore);
	TestTrue("natural projectile delivery applies status-on-hit",
		TargetASC->HasMatchingGameplayTag(MordecaiGameplayTags::Status_Poisoned));
	TestFalse("default non-piercing projectile stops on the blocking ASC Pawn",
		IsValid(Projectile) && !Projectile->IsActorBeingDestroyed());

	Env.Teardown();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecaiRecovery_ProjectileBlockingAndPierce,
	"Mordecai.Recovery.Fireball.WorldStaticBlocksAndNonzeroPierceStillHitsTwoTargets",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecaiRecovery_ProjectileBlockingAndPierce::RunTest(const FString& Parameters)
{
	FRecoveryTestEnv Env;
	if (!TestTrue("test world", Env.Setup(FVector::ZeroVector)))
	{
		return true;
	}

	AActor* Wall = SpawnCollisionSphere(
		Env.World, FVector(180.f, -300.f, 100.f), ECC_WorldStatic, ECR_Block, 45.f);
	AMordecaiProjectile* Blocked = SpawnConfiguredProjectile(
		Env, FVector(0.f, -300.f, 100.f), FVector::ForwardVector, MakeSpec(1000.f), MakeDamage());
	TickWorld(Env.World, 0.35f);
	AddInfo(FString::Printf(TEXT("static diagnostic: wall x=%.1f, projectile x=%.1f, velocity=%.1f"),
		Wall ? Wall->GetActorLocation().X : -1.f, Blocked ? Blocked->GetActorLocation().X : -1.f,
		Blocked ? Blocked->GetProjectileMovement()->Velocity.Size() : -1.f));
	FHitResult PawnSweepHit;
	const bool bStoppedProjectileAlive = IsValid(Blocked) && !Blocked->IsActorBeingDestroyed();
	const ECollisionResponse PawnToProjectileResponse =
		Env.Player->GetCapsuleComponent()->GetCollisionResponseToChannel(
			Mordecai_TraceChannel_ProjectileDetection);
	if (bStoppedProjectileAlive)
	{
		Env.Player->SetActorLocation(FVector(0.f, -300.f, 100.f), false, nullptr,
			ETeleportType::TeleportPhysics);
		Env.Player->GetCapsuleComponent()->MoveComponent(
			FVector(200.f, 0.f, 0.f), Env.Player->GetActorQuat(), true, &PawnSweepHit);
	}
	AddInfo(FString::Printf(TEXT("stopped-projectile obstacle audit: alive=%d, pawn response to "
		"channel7=%d, pawn sweep blocking=%d, hit actor=%s"),
		bStoppedProjectileAlive, static_cast<int32>(PawnToProjectileResponse),
		PawnSweepHit.bBlockingHit, *GetNameSafe(PawnSweepHit.GetActor())));
	TestFalse("WorldStatic-stopped projectile is destroyed", bStoppedProjectileAlive);
	TestFalse("WorldStatic-stopped projectile does not remain as a pawn-blocking obstacle",
		bStoppedProjectileAlive && PawnSweepHit.GetActor() == Blocked);
	TestNotNull("WorldStatic fixture", Wall);
	TestTrue("WorldStatic stops projectile movement before crossing the fixture",
		Blocked && Blocked->GetActorLocation().X < 240.f
		&& Blocked->GetProjectileMovement()->Velocity.IsNearlyZero());

	AMordecaiEnemyCharacter* First = Env.SpawnEnemy(FVector(250.f, 300.f, 100.f), {});
	AMordecaiEnemyCharacter* Second = Env.SpawnEnemy(FVector(500.f, 300.f, 100.f), {});
	AddProjectileOverlapSphere(First);
	AddProjectileOverlapSphere(Second);
	UAbilitySystemComponent* FirstASC = First ? First->GetAbilitySystemComponent() : nullptr;
	UAbilitySystemComponent* SecondASC = Second ? Second->GetAbilitySystemComponent() : nullptr;
	const float FirstBefore = FirstASC
		? FirstASC->GetNumericAttribute(UMordecaiAttributeSet::GetHealthAttribute()) : -1.f;
	const float SecondBefore = SecondASC
		? SecondASC->GetNumericAttribute(UMordecaiAttributeSet::GetHealthAttribute()) : -1.f;
	AMordecaiProjectile* Piercing = SpawnConfiguredProjectile(
		Env, FVector(0.f, 300.f, 100.f), FVector::ForwardVector, MakeSpec(1000.f, 2.f, 1000.f, 1),
		MakeDamage(10.f));
	TickWorld(Env.World, 0.8f);

	TestTrue("nonzero pierce damages first target",
		FirstASC && FirstASC->GetNumericAttribute(UMordecaiAttributeSet::GetHealthAttribute()) < FirstBefore);
	TestTrue("nonzero pierce continues and damages second target",
		SecondASC && SecondASC->GetNumericAttribute(UMordecaiAttributeSet::GetHealthAttribute()) < SecondBefore);
	TestFalse("projectile stops after its one extra pierce is consumed",
		IsValid(Piercing) && !Piercing->IsActorBeingDestroyed());

	Env.Teardown();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecaiRecovery_FireballRealCast,
	"Mordecai.Recovery.Fireball.GASCastTimersSpawnClearVisibleProjectileAndNaturallyDamageTarget",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecaiRecovery_FireballRealCast::RunTest(const FString& Parameters)
{
	FRecoveryTestEnv Env;
	if (!TestTrue("test world", Env.Setup(FVector::ZeroVector)))
	{
		return true;
	}

	const FVector CastDirection = FVector::RightVector;
	Env.Player->SetActorRotation(CastDirection.Rotation());
	AMordecaiEnemyCharacter* Target = Env.SpawnEnemy(CastDirection * 450.f, {});
	AddProjectileOverlapSphere(Target);
	UAbilitySystemComponent* PlayerASC = Env.GetPlayerASC();
	UAbilitySystemComponent* TargetASC = Target ? Target->GetAbilitySystemComponent() : nullptr;
	UMordecaiSpellDataAsset* Data = MakeFireballData();
	if (!TestNotNull("player ASC", PlayerASC) || !TestNotNull("target ASC", TargetASC))
	{
		Env.Teardown();
		return true;
	}

	const float HealthBefore = TargetASC->GetNumericAttribute(UMordecaiAttributeSet::GetHealthAttribute());
	const FGameplayAbilitySpecHandle Handle = PlayerASC->GiveAbility(
		FGameplayAbilitySpec(UMordecaiGA_Fireball::StaticClass(), 1, INDEX_NONE, Data));
	TestTrue("GAS activates Fireball", PlayerASC->TryActivateAbility(Handle));

	// Advance only the ability's casting timers so the projectile can be inspected at its exact
	// spawn transform before any projectile movement tick.
	TickTimersOnly(Env.World, Data->WindupTime + Data->CastTime + TickStep);
	AMordecaiProjectile* CastProjectile = nullptr;
	float SpawnProjection = 0.f;
	bool bVisibleMesh = false;
	CastProjectile = FindLiveProjectile(Env.World);
	if (CastProjectile)
	{
		const FVector SpawnOffset = CastProjectile->GetActorLocation() - Env.Player->GetActorLocation();
		SpawnProjection = FVector::DotProduct(SpawnOffset, CastDirection);
		if (!CastProjectile->HasActorBegunPlay())
		{
			CastProjectile->DispatchBeginPlay();
		}
		if (UStaticMeshComponent* Mesh = CastProjectile->FindComponentByClass<UStaticMeshComponent>())
		{
			bVisibleMesh = Mesh->IsRegistered() && Mesh->GetStaticMesh() != nullptr
				&& Mesh->IsVisible() && Mesh->GetCollisionEnabled() == ECollisionEnabled::NoCollision;
		}
	}
	TestNotNull("casting timers spawned a projectile before movement", CastProjectile);
	if (CastProjectile)
	{
		TestTrue("non-axis GAS cast preserves its assisted world-space direction",
			FVector::DotProduct(
				CastProjectile->GetProjectileMovement()->Velocity.GetSafeNormal(), CastDirection) > 0.99f);
	}

	for (int32 Step = 0; Step < 100
		&& TargetASC->GetNumericAttribute(UMordecaiAttributeSet::GetHealthAttribute()) >= HealthBefore; ++Step)
	{
		TickWorld(Env.World, TickStep);
	}

	const float RequiredClearance = Env.Player->GetCapsuleComponent()->GetScaledCapsuleRadius()
		+ Data->ProjectileCollisionRadius + 5.f;
	TestTrue("pre-movement signed spawn projection includes combined radii and margin",
		SpawnProjection >= RequiredClearance - KINDA_SMALL_NUMBER);
	TestTrue("projectile owns a registered visible asset-backed NoCollision mesh", bVisibleMesh);
	TestTrue("naturally swept cast projectile damages the target",
		TargetASC->GetNumericAttribute(UMordecaiAttributeSet::GetHealthAttribute()) < HealthBefore);

	Env.Teardown();
	return true;
}
