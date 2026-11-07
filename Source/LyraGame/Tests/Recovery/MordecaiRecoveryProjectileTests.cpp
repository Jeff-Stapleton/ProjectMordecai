// Project Mordecai — US-087 Fireball recovery regression tests (issue #20)

#include "Misc/AutomationTest.h"

#include "AbilitySystemComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "EngineUtils.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "Mordecai/Combat/MordecaiHitDetectionTypes.h"
#include "Mordecai/Combat/MordecaiProjectile.h"
#include "Mordecai/Magic/MordecaiGA_Fireball.h"
#include "Mordecai/Magic/MordecaiSpellDataAsset.h"
#include "Mordecai/MordecaiGameplayTags.h"
#include "Mordecai/StatusEffects/Effects/MordecaiGE_Poisoned.h"
#include "MordecaiRecoveryTestEnv.h"
#include "TimerManager.h"

namespace
{
	using namespace MordecaiRecoveryTest;

	constexpr float TickStep = 0.02f;

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

	void AddProjectileOverlapSphere(AActor* Actor, float Radius = 45.f)
	{
		if (!Actor)
		{
			return;
		}
		Actor->SetActorEnableCollision(true);
		// The lightweight world does not build a map collision profile for character capsules.
		// Register an explicit WorldDynamic query shape on the real ASC actor so swept movement
		// still delivers the natural overlap to that actor (packaged Tier 4 covers its map capsule).
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
	FRecoveryTestEnv Env;
	if (!TestTrue("test world", Env.Setup(FVector::ZeroVector)))
	{
		return true;
	}

	AActor* NonASC = SpawnCollisionSphere(
		Env.World, FVector(180.f, 0.f, 100.f), ECC_WorldDynamic, ECR_Overlap);
	AMordecaiEnemyCharacter* Target = Env.SpawnEnemy(FVector(420.f, 0.f, 100.f), {});
	AddProjectileOverlapSphere(Target);
	UAbilitySystemComponent* TargetASC = Target ? Target->GetAbilitySystemComponent() : nullptr;
	if (!TestNotNull("registered non-ASC overlap fixture", NonASC)
		|| !TestNotNull("ASC target", TargetASC))
	{
		Env.Teardown();
		return true;
	}

	const float HealthBefore = TargetASC->GetNumericAttribute(UMordecaiAttributeSet::GetHealthAttribute());
	AMordecaiProjectile* Projectile = SpawnConfiguredProjectile(
		Env, FVector(0.f, 0.f, 100.f), FVector::ForwardVector, MakeSpec(1000.f), MakeDamage(20.f));
	if (Projectile)
	{
		FMordecaiStatusOnHitEntry Poison;
		Poison.StatusEffectTag = MordecaiGameplayTags::Status_Poisoned;
		Poison.ApplicationChance = 1.f;
		Poison.StatusEffectGEClass = UMordecaiGE_Poisoned::StaticClass();
		Projectile->SetStatusEffectsOnHit({ Poison });
	}
	TickWorld(Env.World, 0.6f);
	const USphereComponent* TargetSphere = Target->FindComponentByClass<USphereComponent>();
	AddInfo(FString::Printf(TEXT("delivery diagnostic: target object=%d response=%d overlaps=%d; "
		"test sphere registered=%d at x=%.1f; projectile bound=%d pawn-response=%d "
		"dynamic-response=%d overlaps=%d x=%.1f velocity=%.1f valid=%d"),
		static_cast<int32>(Target->GetCapsuleComponent()->GetCollisionObjectType()),
		static_cast<int32>(Target->GetCapsuleComponent()->GetCollisionResponseToChannel(
			Mordecai_TraceChannel_ProjectileDetection)),
		Target->GetCapsuleComponent()->GetGenerateOverlapEvents(),
		TargetSphere && TargetSphere->IsRegistered(), TargetSphere ? TargetSphere->GetComponentLocation().X : -1.f,
		Projectile->GetCollisionComponent()->OnComponentBeginOverlap.IsBound(),
		static_cast<int32>(Projectile->GetCollisionComponent()->GetCollisionResponseToChannel(ECC_Pawn)),
		static_cast<int32>(Projectile->GetCollisionComponent()->GetCollisionResponseToChannel(ECC_WorldDynamic)),
		Projectile->GetCollisionComponent()->GetGenerateOverlapEvents(),
		Projectile ? Projectile->GetActorLocation().X : -1.f,
		Projectile ? Projectile->GetProjectileMovement()->Velocity.Size() : -1.f,
		IsValid(Projectile) && !Projectile->IsActorBeingDestroyed()));

	TestTrue("non-ASC overlap did not consume the projectile before the target",
		TargetASC->GetNumericAttribute(UMordecaiAttributeSet::GetHealthAttribute()) < HealthBefore);
	TestTrue("natural projectile delivery applies status-on-hit",
		TargetASC->HasMatchingGameplayTag(MordecaiGameplayTags::Status_Poisoned));
	TestFalse("default non-piercing projectile stops on the damageable target",
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

	AMordecaiEnemyCharacter* Target = Env.SpawnEnemy(FVector(450.f, 0.f, 0.f), {});
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

	AMordecaiProjectile* CastProjectile = nullptr;
	float SpawnDistance = 0.f;
	bool bVisibleMesh = false;
	for (int32 Step = 0; Step < 100
		&& TargetASC->GetNumericAttribute(UMordecaiAttributeSet::GetHealthAttribute()) >= HealthBefore; ++Step)
	{
		TickWorld(Env.World, TickStep);
		if (!CastProjectile)
		{
			CastProjectile = FindLiveProjectile(Env.World);
			if (CastProjectile)
			{
				SpawnDistance = FVector::Dist2D(Env.Player->GetActorLocation(), CastProjectile->GetActorLocation());
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
		}
	}

	const float RequiredClearance = Env.Player->GetCapsuleComponent()->GetScaledCapsuleRadius()
		+ Data->ProjectileCollisionRadius;
	TestNotNull("casting timers spawned a projectile", CastProjectile);
	TestTrue("projectile spawns beyond combined caster/projectile radii with a margin",
		SpawnDistance > RequiredClearance);
	TestTrue("projectile owns a registered visible asset-backed NoCollision mesh", bVisibleMesh);
	TestTrue("naturally swept cast projectile damages the target",
		TargetASC->GetNumericAttribute(UMordecaiAttributeSet::GetHealthAttribute()) < HealthBefore);

	Env.Teardown();
	return true;
}
