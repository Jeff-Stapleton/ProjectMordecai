// Project Mordecai — Enemy AI Controller (US-051)

#include "Mordecai/Enemy/MordecaiEnemyAIController.h"

#include "Mordecai/Enemy/MordecaiEnemyCharacter.h"
#include "Mordecai/MordecaiGameplayTags.h"
#include "AbilitySystemComponent.h"
#include "GameplayAbilitySpec.h"
#include "Kismet/GameplayStatics.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(MordecaiEnemyAIController)

// ---------------------------------------------------------------------------
// Constructor
// ---------------------------------------------------------------------------

AMordecaiEnemyAIController::AMordecaiEnemyAIController()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = true;
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

	AActor* Target = FindPlayerTarget();
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
		// AC-051.4: Attack when in range
		else if (IsInAttackRange(DistToTarget))
		{
			TransitionToState(EMordecaiEnemyAIState::Attack);
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
			else if (Target && IsInAttackRange(DistToTarget))
			{
				TransitionToState(EMordecaiEnemyAIState::Attack);
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
// Helpers
// ---------------------------------------------------------------------------

AActor* AMordecaiEnemyAIController::FindPlayerTarget() const
{
	if (const UWorld* World = GetWorld())
	{
		return UGameplayStatics::GetPlayerPawn(World, 0);
	}
	return nullptr;
}
