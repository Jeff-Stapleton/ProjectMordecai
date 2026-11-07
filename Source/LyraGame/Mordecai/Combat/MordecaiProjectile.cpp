// Project Mordecai — Projectile Actor (US-009)

#include "Mordecai/Combat/MordecaiProjectile.h"
#include "Mordecai/Combat/MordecaiHitDetectionSubsystem.h"
#include "Mordecai/Combat/MordecaiHitDetectionTypes.h"
#include "Mordecai/Combat/MordecaiGA_MeleeAttack.h"
#include "Mordecai/Magic/MordecaiGA_Blur.h"
#include "Mordecai/MordecaiGameplayTags.h"
#include "Mordecai/AbilitySystem/MordecaiAttributeSet.h"
#include "Mordecai/UI/MordecaiDamagePopComponent.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "UObject/ConstructorHelpers.h"
#include "GameplayEffect.h"
#include "Engine/OverlapResult.h"
#include "TimerManager.h"

// ---------------------------------------------------------------------------
// Constructor
// ---------------------------------------------------------------------------

AMordecaiProjectile::AMordecaiProjectile()
{
	PrimaryActorTick.bCanEverTick = true;

	// AC-009.7: Collision sphere with dedicated projectile channel
	CollisionComp = CreateDefaultSubobject<USphereComponent>(TEXT("CollisionComp"));
	CollisionComp->InitSphereRadius(10.f);
	CollisionComp->SetCollisionObjectType(Mordecai_TraceChannel_ProjectileDetection);
	CollisionComp->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	CollisionComp->SetGenerateOverlapEvents(true);
	CollisionComp->SetCollisionResponseToAllChannels(ECR_Ignore);
	CollisionComp->SetCollisionResponseToChannel(ECC_Pawn, ECR_Block);
	CollisionComp->SetCollisionResponseToChannel(ECC_WorldDynamic, ECR_Overlap);
	CollisionComp->SetCollisionResponseToChannel(ECC_WorldStatic, ECR_Block);
	SetRootComponent(CollisionComp);

	// AC-009.1: ProjectileMovementComponent for flight
	ProjectileMovement = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("ProjectileMovement"));
	ProjectileMovement->UpdatedComponent = CollisionComp;
	ProjectileMovement->bRotationFollowsVelocity = true;
	ProjectileMovement->bShouldBounce = false;
	ProjectileMovement->ProjectileGravityScale = 0.f;

	// AC-087.3: visible native placeholder until each spell gets authored VFX. The engine sphere
	// is cooked with the engine, needs no project content, and never participates in collision.
	ProjectileMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ProjectileMesh"));
	ProjectileMesh->SetupAttachment(CollisionComp);
	ProjectileMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	ProjectileMesh->SetRelativeScale3D(FVector(0.3f));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> SphereMesh(
		TEXT("/Engine/BasicShapes/Sphere.Sphere"));
	if (SphereMesh.Succeeded())
	{
		ProjectileMesh->SetStaticMesh(SphereMesh.Object);
	}

	// Replication for Iris
	bReplicates = true;
}

// ---------------------------------------------------------------------------
// Initialization
// ---------------------------------------------------------------------------

void AMordecaiProjectile::InitFromSpec(const FMordecaiProjectileSpec& InSpec, const FMordecaiDamageProfile& InDamageProfile)
{
	Spec = InSpec;
	DamageProfile = InDamageProfile;

	// AC-009.2: Configure speed
	ProjectileMovement->InitialSpeed = Spec.Speed;
	ProjectileMovement->MaxSpeed = Spec.Speed;
	ProjectileMovement->Velocity = GetActorForwardVector() * Spec.Speed;
	ProjectileMovement->Activate(true);

	// AC-009.3: Configure gravity
	ProjectileMovement->ProjectileGravityScale = Spec.GravityDrop;

	// Pierce and ricochet counters
	RemainingPierceCount = Spec.PierceCount;
	RemainingRicochetCount = Spec.RicochetCount;

	// Deferred spawns consume this in BeginPlay. Keep the documented post-spawn API compatible:
	// if BeginPlay already happened, apply the supplied lifetime now rather than retaining the
	// constructor's zero/default lifetime.
	if (HasActorBegunPlay())
	{
		SetLifeSpan(Spec.Lifetime > 0.f ? Spec.Lifetime : 0.f);
	}
}

void AMordecaiProjectile::SetProjectileInstigator(AActor* InInstigator)
{
	ProjectileInstigator = InInstigator;
	SetInstigator(Cast<APawn>(InInstigator));
	if (CollisionComp && InInstigator)
	{
		CollisionComp->IgnoreActorWhenMoving(InInstigator, true);
	}
}

// ---------------------------------------------------------------------------
// BeginPlay
// ---------------------------------------------------------------------------

void AMordecaiProjectile::BeginPlay()
{
	Super::BeginPlay();

	SpawnLocation = GetActorLocation();

	// AC-009.4: Set lifetime timer
	if (Spec.Lifetime > 0.f)
	{
		SetLifeSpan(Spec.Lifetime);
	}

	// Bind overlap event for hit detection
	CollisionComp->OnComponentBeginOverlap.AddDynamic(this, &AMordecaiProjectile::OnProjectileOverlap);
	ProjectileMovement->OnProjectileStop.AddDynamic(this, &AMordecaiProjectile::OnProjectileStopped);
}

// ---------------------------------------------------------------------------
// Tick — Range Check (AC-009.4)
// ---------------------------------------------------------------------------

void AMordecaiProjectile::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	CheckRangeAndLifetime();
}

void AMordecaiProjectile::CheckRangeAndLifetime()
{
	if (Spec.MaxRange > 0.f)
	{
		DistanceTraveled = FVector::Dist(SpawnLocation, GetActorLocation());
		if (DistanceTraveled >= Spec.MaxRange)
		{
			Destroy();
		}
	}
}

// ---------------------------------------------------------------------------
// Hit Detection
// ---------------------------------------------------------------------------

bool AMordecaiProjectile::ShouldIgnoreActor(const AActor* Actor) const
{
	if (!Actor)
	{
		return true;
	}

	// AC-009.6: Ignore instigator
	if (ProjectileInstigator.IsValid() && Actor == ProjectileInstigator.Get())
	{
		return true;
	}

	// AC-009.14: Skip already-hit targets
	if (HasAlreadyHit(Actor))
	{
		return true;
	}

	return false;
}

void AMordecaiProjectile::OnProjectileOverlap(
	UPrimitiveComponent* /*OverlappedComponent*/,
	AActor* OtherActor,
	UPrimitiveComponent* /*OtherComp*/,
	int32 /*OtherBodyIndex*/,
	bool /*bFromSweep*/,
	const FHitResult& SweepResult)
{
	if (!OtherActor || ShouldIgnoreActor(OtherActor))
	{
		return;
	}

	// WorldDynamic utility actors, pickup triggers and other non-combat overlaps are transparent:
	// only an actor with an ASC is a damageable projectile target. In particular they must not
	// enter HandleHit, consume pierce or destroy a zero-pierce projectile.
	if (!UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(OtherActor))
	{
		return;
	}

	FVector HitLoc = SweepResult.ImpactPoint.IsNearlyZero() ? FVector(OtherActor->GetActorLocation()) : FVector(SweepResult.ImpactPoint);
	HandleHit(OtherActor, HitLoc);
}

void AMordecaiProjectile::OnProjectileStopped(const FHitResult& ImpactResult)
{
	AActor* HitActor = ImpactResult.GetActor();
	if (!HitActor || ShouldIgnoreActor(HitActor)
		|| !UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(HitActor))
	{
		// WorldStatic and non-combat blocking actors stop the projectile without spending pierce.
		return;
	}

	const bool bWillPierce = RemainingPierceCount > 0;
	if (bWillPierce)
	{
		// A blocking target would immediately stop the resumed sweep again. Ignore only that
		// already-hit actor; ProcessPierceHit still owns the gameplay hit list and counter.
		CollisionComp->IgnoreActorWhenMoving(HitActor, true);
	}

	HandleHit(HitActor, ImpactResult.ImpactPoint);

	if (bWillPierce && IsValid(this) && !IsActorBeingDestroyed())
	{
		// StopSimulating's caller is still unwinding its blocking-hit handling. Resume on the
		// next frame so that outer call cannot deflect or stop the restored velocity again.
		GetWorldTimerManager().SetTimerForNextTick(FTimerDelegate::CreateWeakLambda(this, [this]()
		{
			if (IsValid(this) && !IsActorBeingDestroyed())
			{
				ProjectileMovement->SetUpdatedComponent(CollisionComp);
				ProjectileMovement->Velocity = GetActorForwardVector() * Spec.Speed;
				ProjectileMovement->Activate(true);
			}
		}));
	}
}

void AMordecaiProjectile::HandleHit(AActor* HitActor, const FVector& HitLocation)
{
	// AC-009.5: Apply damage to the direct hit target
	ApplyDamageToTarget(HitActor, DamageProfile.BasePower);

	// Check pierce (also adds to hit list)
	bool bContinueFlying = ProcessPierceHit(HitActor);

	if (!bContinueFlying)
	{
		// AC-009.15: Apply AoE on final hit (if configured)
		if (HasOnHitAoE())
		{
			ApplyAoEDamage(HitLocation, HitActor);
		}

		// AC-009.11: Attempt ricochet before destroying
		if (CanRicochet())
		{
			AttemptRicochet(HitLocation);
			return;
		}

		Destroy();
	}
}

// ---------------------------------------------------------------------------
// Pierce (AC-009.8, AC-009.9, AC-009.10)
// ---------------------------------------------------------------------------

bool AMordecaiProjectile::ProcessPierceHit(AActor* HitTarget)
{
	if (!HitTarget)
	{
		return false;
	}

	// Add to hit list to prevent re-hits (AC-009.14)
	AddToHitList(HitTarget);

	if (RemainingPierceCount > 0)
	{
		RemainingPierceCount--;
		return true; // Continue flying
	}

	return false; // Stop — no pierce remaining
}

// ---------------------------------------------------------------------------
// Ricochet (AC-009.11-14)
// ---------------------------------------------------------------------------

void AMordecaiProjectile::ConsumeRicochet()
{
	if (RemainingRicochetCount > 0)
	{
		RemainingRicochetCount--;
	}
}

AActor* AMordecaiProjectile::FindBestRicochetTarget(
	const FVector& HitLocation,
	const FVector& ProjectileForward,
	const TArray<AActor*>& Candidates) const
{
	AActor* BestTarget = nullptr;
	float BestDistance = TNumericLimits<float>::Max();

	for (AActor* Candidate : Candidates)
	{
		if (!Candidate || HasAlreadyHit(Candidate) || ShouldIgnoreActor(Candidate))
		{
			continue;
		}

		FVector ToCandidate = Candidate->GetActorLocation() - HitLocation;
		float DistToCandidate = ToCandidate.Size();

		// AC-009.11: Must be within ricochet range
		if (DistToCandidate > Spec.RicochetRange || DistToCandidate < KINDA_SMALL_NUMBER)
		{
			continue;
		}

		// AC-009.11: Must be within ricochet angle limit
		FVector DirToCandidate = ToCandidate.GetSafeNormal();
		float Dot = FVector::DotProduct(ProjectileForward.GetSafeNormal(), DirToCandidate);
		float AngleDeg = FMath::RadiansToDegrees(FMath::Acos(FMath::Clamp(Dot, -1.f, 1.f)));

		if (AngleDeg > Spec.RicochetAngleLimit)
		{
			continue;
		}

		// Pick nearest valid target
		if (DistToCandidate < BestDistance)
		{
			BestDistance = DistToCandidate;
			BestTarget = Candidate;
		}
	}

	return BestTarget;
}

void AMordecaiProjectile::AttemptRicochet(const FVector& HitLocation)
{
	if (!CanRicochet())
	{
		Destroy();
		return;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		Destroy();
		return;
	}

	// Sphere overlap to find ricochet candidates
	TArray<FOverlapResult> OverlapResults;
	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(this);
	if (ProjectileInstigator.IsValid())
	{
		QueryParams.AddIgnoredActor(ProjectileInstigator.Get());
	}

	World->OverlapMultiByChannel(
		OverlapResults,
		HitLocation,
		FQuat::Identity,
		ECC_Pawn,
		FCollisionShape::MakeSphere(Spec.RicochetRange),
		QueryParams);

	TArray<AActor*> Candidates;
	for (const FOverlapResult& Result : OverlapResults)
	{
		if (Result.GetActor())
		{
			Candidates.AddUnique(Result.GetActor());
		}
	}

	FVector ProjectileForward = GetActorForwardVector();
	AActor* RicochetTarget = FindBestRicochetTarget(HitLocation, ProjectileForward, Candidates);

	if (!RicochetTarget)
	{
		// AC-009.13: No valid target — destroy
		Destroy();
		return;
	}

	// AC-009.12: Redirect toward target at same speed
	ConsumeRicochet();
	FVector NewDirection = (RicochetTarget->GetActorLocation() - HitLocation).GetSafeNormal();
	ProjectileMovement->Velocity = NewDirection * Spec.Speed;
	SetActorLocation(HitLocation);
}

// ---------------------------------------------------------------------------
// Damage Application (AC-009.5, AC-009.28)
// ---------------------------------------------------------------------------

void AMordecaiProjectile::ApplyDamageToTarget(AActor* TargetActor, float DamageMagnitude)
{
	if (!TargetActor)
	{
		return;
	}

	UAbilitySystemComponent* TargetASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(TargetActor);
	if (!TargetASC)
	{
		return;
	}

	// AC-060.7: Blur miss chance — ranged projectile attacks can miss blurred targets
	if (TargetASC->HasMatchingGameplayTag(MordecaiGameplayTags::Status_Blurred))
	{
		float EvasionChance = TargetASC->GetNumericAttribute(UMordecaiAttributeSet::GetRangedEvasionChanceAttribute());
		if (EvasionChance > 0.f)
		{
			float Roll = FMath::FRand();
			if (UMordecaiGA_Blur::RollMiss(EvasionChance, Roll))
			{
				// Miss — skip damage application
				return;
			}
		}
	}

	// Use instigator's ASC as source for effect context
	UAbilitySystemComponent* SourceASC = nullptr;
	if (ProjectileInstigator.IsValid())
	{
		SourceASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(ProjectileInstigator.Get());
	}

	FGameplayEffectContextHandle Context;
	if (SourceASC)
	{
		Context = SourceASC->MakeEffectContext();
	}
	else
	{
		Context = TargetASC->MakeEffectContext();
	}
	Context.AddSourceObject(this);

	// AC-009.28: Health damage via instant GE (same pipeline as melee)
	{
		UGameplayEffect* HealthGE = NewObject<UGameplayEffect>(GetTransientPackage(), TEXT("GE_MordecaiProjectileHealthDamage"));
		HealthGE->DurationPolicy = EGameplayEffectDurationType::Instant;

		FGameplayModifierInfo& Mod = HealthGE->Modifiers.AddDefaulted_GetRef();
		Mod.Attribute = UMordecaiAttributeSet::GetHealthAttribute();
		Mod.ModifierOp = EGameplayModOp::Additive;
		Mod.ModifierMagnitude = FGameplayEffectModifierMagnitude(FScalableFloat(-DamageMagnitude));

		FGameplayEffectSpec HealthSpec(HealthGE, Context, 1.0f);

		// Tag with damage type
		FGameplayTag DamageTag = UMordecaiGA_MeleeAttack::GetDamageTagForType(DamageProfile.DamageType);
		if (DamageTag.IsValid())
		{
			HealthSpec.AddDynamicAssetTag(DamageTag);
		}

		TargetASC->ApplyGameplayEffectSpecToSelf(HealthSpec);
	}

	// Posture damage if applicable
	if (DamageProfile.AppliesPostureDamage)
	{
		UGameplayEffect* PostureGE = NewObject<UGameplayEffect>(GetTransientPackage(), TEXT("GE_MordecaiProjectilePostureDamage"));
		PostureGE->DurationPolicy = EGameplayEffectDurationType::Instant;

		FGameplayModifierInfo& Mod = PostureGE->Modifiers.AddDefaulted_GetRef();
		Mod.Attribute = UMordecaiAttributeSet::GetPostureAttribute();
		Mod.ModifierOp = EGameplayModOp::Additive;
		Mod.ModifierMagnitude = FGameplayEffectModifierMagnitude(FScalableFloat(-DamageMagnitude));

		FGameplayEffectSpec PostureSpec(PostureGE, Context, 1.0f);

		FGameplayTag DamageTag = UMordecaiGA_MeleeAttack::GetDamageTagForType(DamageProfile.DamageType);
		if (DamageTag.IsValid())
		{
			PostureSpec.AddDynamicAssetTag(DamageTag);
		}

		TargetASC->ApplyGameplayEffectSpecToSelf(PostureSpec);
	}

	// US-056: Apply status effects on hit
	UMordecaiGA_MeleeAttack::ApplyStatusEffectsFromEntries(
		StatusEffectsOnHit, SourceASC ? SourceASC : TargetASC, TargetASC, this);

	// US-065: Fire floating damage number
	{
		FGameplayTag DamageTag = UMordecaiGA_MeleeAttack::GetDamageTagForType(DamageProfile.DamageType);
		int32 DisplayDamage = FMath::Abs(FMath::RoundToInt32(DamageMagnitude));
		UMordecaiDamagePopComponent::BroadcastDamagePop(
			TargetActor->GetWorld(), DisplayDamage, DamageTag, TargetActor->GetActorLocation(), /*bIsCritical=*/false);
	}
}

// ---------------------------------------------------------------------------
// AoE Damage (AC-009.15-18)
// ---------------------------------------------------------------------------

void AMordecaiProjectile::ApplyAoEDamage(const FVector& HitLocation, AActor* DirectHitActor)
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	UMordecaiHitDetectionSubsystem* HitDetection = World->GetSubsystem<UMordecaiHitDetectionSubsystem>();
	if (!HitDetection)
	{
		return;
	}

	// AC-009.15: Circle overlap at hit location
	AActor* AttackerActor = ProjectileInstigator.IsValid() ? ProjectileInstigator.Get() : nullptr;
	TArray<FMordecaiHitResult> AoEHits = HitDetection->CircleOverlapQuery(
		HitLocation,
		Spec.OnHitAoERadius,
		AttackerActor,
		EMordecaiTargetFilter::All,
		/*bAllowFriendlyFire=*/false,
		/*bJumpAvoidable=*/false,
		/*bHitsAirborne=*/true);

	// AC-009.16: AoE damage = BasePower * OnHitAoEDamageScalar
	float AoEDamage = ComputeAoEDamage();

	for (const FMordecaiHitResult& Hit : AoEHits)
	{
		if (Hit.HitActor.IsValid())
		{
			// AC-009.18: Direct hit target also takes AoE damage (stacks with direct)
			ApplyDamageToTarget(Hit.HitActor.Get(), AoEDamage);
		}
	}
}
