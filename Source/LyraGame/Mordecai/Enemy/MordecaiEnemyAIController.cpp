// Project Mordecai — Enemy AI Controller (US-051)

#include "Mordecai/Enemy/MordecaiEnemyAIController.h"

#include "Mordecai/Enemy/MordecaiEnemyCharacter.h"
#include "Mordecai/Combat/MordecaiGA_MeleeAttack.h"
#include "Mordecai/Magic/MordecaiIllusionActor.h"
#include "Mordecai/MordecaiGameplayTags.h"
#include "AbilitySystemComponent.h"
#include "GameplayAbilitySpec.h"
#include "Kismet/GameplayStatics.h"
#include "EngineUtils.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(MordecaiEnemyAIController)

// ---------------------------------------------------------------------------
// Constructor
// ---------------------------------------------------------------------------

AMordecaiEnemyAIController::AMordecaiEnemyAIController()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = true;

	// Default melee attack ability for enemy AI (AC-054.13)
	MeleeAttackAbilityClass = UMordecaiGA_MeleeAttack::StaticClass();
}

// ---------------------------------------------------------------------------
// Lifecycle
// ---------------------------------------------------------------------------

void AMordecaiEnemyAIController::BeginPlay()
{
	Super::BeginPlay();

	if (APawn* MyPawn = GetPawn())
	{
		SpawnLocation = MyPawn->GetActorLocation();
	}
}

void AMordecaiEnemyAIController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);

	CachedEnemy = Cast<AMordecaiEnemyCharacter>(InPawn);
	if (CachedEnemy.IsValid())
	{
		CachedASC = Cast<UAbilitySystemComponent>(CachedEnemy->GetAbilitySystemComponent());
		BindTagDelegates();
	}

	SpawnLocation = InPawn->GetActorLocation();
}

void AMordecaiEnemyAIController::OnUnPossess()
{
	UnbindTagDelegates();
	CachedEnemy.Reset();
	CachedASC.Reset();

	Super::OnUnPossess();
}

// ---------------------------------------------------------------------------
// Tag Delegates (AC-051.7, AC-051.8)
// ---------------------------------------------------------------------------

void AMordecaiEnemyAIController::BindTagDelegates()
{
	if (!CachedASC.IsValid())
	{
		return;
	}

	CachedASC->RegisterGameplayTagEvent(
		MordecaiGameplayTags::State_PostureBroken, EGameplayTagEventType::NewOrRemoved)
		.AddUObject(this, &AMordecaiEnemyAIController::OnPostureBrokenTagChanged);

	CachedASC->RegisterGameplayTagEvent(
		MordecaiGameplayTags::State_Dead, EGameplayTagEventType::NewOrRemoved)
		.AddUObject(this, &AMordecaiEnemyAIController::OnDeadTagChanged);
}

void AMordecaiEnemyAIController::UnbindTagDelegates()
{
	if (!CachedASC.IsValid())
	{
		return;
	}

	CachedASC->RegisterGameplayTagEvent(
		MordecaiGameplayTags::State_PostureBroken, EGameplayTagEventType::NewOrRemoved)
		.RemoveAll(this);

	CachedASC->RegisterGameplayTagEvent(
		MordecaiGameplayTags::State_Dead, EGameplayTagEventType::NewOrRemoved)
		.RemoveAll(this);
}

// ---------------------------------------------------------------------------
// Tag Callbacks
// ---------------------------------------------------------------------------

void AMordecaiEnemyAIController::OnPostureBrokenTagChanged(const FGameplayTag Tag, int32 NewCount)
{
	if (CurrentState == EMordecaiEnemyAIState::Dead)
	{
		return;
	}

	if (NewCount > 0)
	{
		// AC-051.7: Enter Staggered state
		TransitionToState(EMordecaiEnemyAIState::Staggered);
	}
	else if (CurrentState == EMordecaiEnemyAIState::Staggered)
	{
		// AC-051.7: Tag removed — resume normal behavior
		TransitionToState(EMordecaiEnemyAIState::Idle);
	}
}

void AMordecaiEnemyAIController::OnDeadTagChanged(const FGameplayTag Tag, int32 NewCount)
{
	if (NewCount > 0)
	{
		// AC-051.8: Enter permanent Dead state
		TransitionToState(EMordecaiEnemyAIState::Dead);
	}
}

// ---------------------------------------------------------------------------
// State Transitions
// ---------------------------------------------------------------------------

void AMordecaiEnemyAIController::TransitionToState(EMordecaiEnemyAIState NewState)
{
	// AC-051.8: Dead is permanent — cannot leave
	if (CurrentState == EMordecaiEnemyAIState::Dead)
	{
		return;
	}

	// Exit current state
	switch (CurrentState)
	{
	case EMordecaiEnemyAIState::Approach:
		StopMovement();
		ClearFocus(EAIFocusPriority::Gameplay);
		break;
	case EMordecaiEnemyAIState::Attack:
		ClearFocus(EAIFocusPriority::Gameplay);
		bAttackInProgress = false;
		break;
	case EMordecaiEnemyAIState::Leash:
		StopMovement();
		break;
	default:
		break;
	}

	CurrentState = NewState;

	// Enter new state
	switch (NewState)
	{
	case EMordecaiEnemyAIState::Recover:
		RecoverElapsedTime = 0.f;
		break;
	case EMordecaiEnemyAIState::Staggered:
		StopMovement();
		ClearFocus(EAIFocusPriority::Gameplay);
		bAttackInProgress = false;
		break;
	case EMordecaiEnemyAIState::Dead:
		StopMovement();
		ClearFocus(EAIFocusPriority::Gameplay);
		bAttackInProgress = false;
		break;
	default:
		break;
	}
}

// ---------------------------------------------------------------------------
// Arena Reset (US-053, AC-053.9)
// ---------------------------------------------------------------------------

void AMordecaiEnemyAIController::ResetState()
{
	CurrentState = EMordecaiEnemyAIState::Idle;
	bAttackInProgress = false;
	RecoverElapsedTime = 0.f;
	StopMovement();
	ClearFocus(EAIFocusPriority::Gameplay);
}

void AMordecaiEnemyAIController::NotifyAttackCompleted()
{
	if (CurrentState == EMordecaiEnemyAIState::Attack)
	{
		bAttackInProgress = false;
		TransitionToState(EMordecaiEnemyAIState::Recover);
	}
}

// ---------------------------------------------------------------------------
// Tick — State Machine (AC-051.12: server authority only)
// ---------------------------------------------------------------------------

void AMordecaiEnemyAIController::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// AC-051.12: Only run state machine on server
	if (!HasAuthority())
	{
		return;
	}

	// AC-060.3: Prefer decoy targets over player
	UpdateStateMachine(DeltaTime, FindBestTarget());
}

void AMordecaiEnemyAIController::UpdateStateMachine(float DeltaTime, AActor* Target)
{
	// Dead state is permanent — no further actions (AC-051.8)
	if (CurrentState == EMordecaiEnemyAIState::Dead)
	{
		return;
	}

	// Staggered pauses all actions until tag removed (AC-051.7)
	if (CurrentState == EMordecaiEnemyAIState::Staggered)
	{
		return;
	}

	APawn* ControlledPawn = GetPawn();
	if (!ControlledPawn)
	{
		return;
	}

	const float DistToTarget = Target
		? FVector::Dist(ControlledPawn->GetActorLocation(), Target->GetActorLocation())
		: MAX_FLT;

	switch (CurrentState)
	{
	case EMordecaiEnemyAIState::Idle:
		// AC-051.3: Detect player within AggroRange
		if (Target && IsInAggroRange(DistToTarget))
		{
			TransitionToState(EMordecaiEnemyAIState::Approach);
		}
		break;

	case EMordecaiEnemyAIState::Approach:
		// AC-051.9: Leash if player too far or lost
		if (!Target || IsOutsideLeashRange(DistToTarget))
		{
			TransitionToState(EMordecaiEnemyAIState::Leash);
		}
		// AC-051.4: Attack when in range (AC-084.4: only with an attack that can hit; else hold here, facing it)
		else if (IsInAttackRange(DistToTarget))
		{
			if (!TryEnterAttackState())
			{
				SetFocus(Target, EAIFocusPriority::Gameplay);
				StopMovement();
			}
		}
		else
		{
			// AC-051.10: Face player during Approach
			SetFocus(Target, EAIFocusPriority::Gameplay);
			MoveToActor(Target);
		}
		break;

	case EMordecaiEnemyAIState::Attack:
		// AC-051.10: Face player during Attack
		if (Target)
		{
			SetFocus(Target, EAIFocusPriority::Gameplay);
		}

		// AC-051.5: Activate melee attack ability via ASC
		if (!bAttackInProgress)
		{
			if (CachedASC.IsValid() && MeleeAttackAbilityClass)
			{
				bAttackInProgress = CachedASC->TryActivateAbilityByClass(MeleeAttackAbilityClass);
			}
			if (!bAttackInProgress)
			{
				// Failed to activate — go to recover
				TransitionToState(EMordecaiEnemyAIState::Recover);
			}
		}
		else if (CachedASC.IsValid())
		{
			// Check if attack ability has finished
			FGameplayAbilitySpec* Spec = CachedASC->FindAbilitySpecFromClass(MeleeAttackAbilityClass);
			if (!Spec || !Spec->IsActive())
			{
				NotifyAttackCompleted();
			}
		}
		break;

	case EMordecaiEnemyAIState::Recover:
		// AC-051.6: Wait for AttackCooldownMs before next action
		RecoverElapsedTime += DeltaTime;
		if (RecoverElapsedTime >= AttackCooldownMs / 1000.f)
		{
			if (IsOutsideLeashRange(DistToTarget))
			{
				TransitionToState(EMordecaiEnemyAIState::Leash);
			}
			else if (Target && IsInAttackRange(DistToTarget) && TryEnterAttackState())
			{
				// Attacking again; without an attack that can hit, fall through to Approach (AC-084.4)
			}
			else if (Target)
			{
				TransitionToState(EMordecaiEnemyAIState::Approach);
			}
			else
			{
				TransitionToState(EMordecaiEnemyAIState::Idle);
			}
		}
		break;

	case EMordecaiEnemyAIState::Leash:
	{
		// AC-051.9: Walk back to spawn point
		const float DistToSpawn = FVector::Dist(ControlledPawn->GetActorLocation(), SpawnLocation);
		if (DistToSpawn < 100.f)
		{
			TransitionToState(EMordecaiEnemyAIState::Idle);
		}
		else
		{
			MoveToLocation(SpawnLocation);
		}
		break;
	}

	default:
		break;
	}
}

// ---------------------------------------------------------------------------
// Attack Availability (US-084, AC-084.4)
// ---------------------------------------------------------------------------

bool AMordecaiEnemyAIController::CanPerformMeleeAttack() const
{
	if (!CachedASC.IsValid() || !MeleeAttackAbilityClass)
	{
		return false;
	}

	FGameplayAbilitySpec* Spec = CachedASC->FindAbilitySpecFromClass(MeleeAttackAbilityClass);
	if (!Spec)
	{
		return false;
	}

	// A melee attack without a profile ends on activation and hits nothing; other abilities are trusted to act
	if (const UMordecaiGA_MeleeAttack* Melee = Cast<UMordecaiGA_MeleeAttack>(Spec->GetPrimaryInstance()))
	{
		return Melee->HasAnyAttackProfile();
	}
	return true;
}

float AMordecaiEnemyAIController::GetEffectiveAttackRange() const
{
	if (CachedASC.IsValid() && MeleeAttackAbilityClass)
	{
		if (const FGameplayAbilitySpec* Spec = CachedASC->FindAbilitySpecFromClass(MeleeAttackAbilityClass))
		{
			if (const UMordecaiGA_MeleeAttack* Melee = Cast<UMordecaiGA_MeleeAttack>(Spec->GetPrimaryInstance()))
			{
				// Issue #47: the attack profile decides the engage distance, so it can never outrange the swing
				const float Reach = Melee->GetAttackReach();
				if (Reach > 0.f)
				{
					return Reach;
				}
			}
		}
	}
	return AttackRange;
}

bool AMordecaiEnemyAIController::TryEnterAttackState()
{
	if (!CanPerformMeleeAttack())
	{
		if (!bLoggedNoMeleeAttack)
		{
			UE_LOG(LogMordecaiEnemy, Error, TEXT("enemy %s cannot attack: its melee ability %s is not granted or has no attack profiles"),
				*GetNameSafe(GetPawn()), *GetNameSafe(MeleeAttackAbilityClass.Get()));
			bLoggedNoMeleeAttack = true;
		}
		return false;
	}

	TransitionToState(EMordecaiEnemyAIState::Attack);
	return true;
}

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

AActor* AMordecaiEnemyAIController::FindBestTarget() const
{
	// AC-060.3: Prefer decoy (Team_Ally) targets within aggro range
	AActor* Decoy = FindNearestDecoy();
	if (Decoy)
	{
		return Decoy;
	}

	// Fallback to player
	return FindPlayerTarget();
}

AActor* AMordecaiEnemyAIController::FindPlayerTarget() const
{
	if (const UWorld* World = GetWorld())
	{
		return UGameplayStatics::GetPlayerPawn(World, 0);
	}
	return nullptr;
}

AActor* AMordecaiEnemyAIController::FindNearestDecoy() const
{
	const UWorld* World = GetWorld();
	APawn* ControlledPawn = GetPawn();
	if (!World || !ControlledPawn)
	{
		return nullptr;
	}

	AActor* BestDecoy = nullptr;
	float BestDist = MAX_FLT;

	for (TActorIterator<AMordecaiIllusionActor> It(World); It; ++It)
	{
		AMordecaiIllusionActor* Decoy = *It;
		if (!Decoy || !Decoy->IsAlive())
		{
			continue;
		}

		float Dist = FVector::Dist(ControlledPawn->GetActorLocation(), Decoy->GetActorLocation());
		if (Dist <= AggroRange && Dist < BestDist)
		{
			BestDist = Dist;
			BestDecoy = Decoy;
		}
	}

	return BestDecoy;
}
