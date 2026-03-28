// Project Mordecai — Enemy AI Controller (US-051)

#pragma once

#include "AIController.h"
#include "Mordecai/Enemy/MordecaiEnemyAITypes.h"
#include "GameplayTagContainer.h"

#include "MordecaiEnemyAIController.generated.h"

class AMordecaiEnemyCharacter;
class UAbilitySystemComponent;
class UGameplayAbility;

/**
 * AMordecaiEnemyAIController
 *
 * Simple tick-driven state-machine AI for enemies.
 * States: Idle -> Approach -> Attack -> Recover -> repeat.
 * Listens for PostureBroken/Dead tags to enter Staggered/Dead states.
 * Server-authoritative -- state machine only ticks on authority (AC-051.12).
 *
 * See: US-051, enemy_ai_framework_v1.md
 */
UCLASS()
class LYRAGAME_API AMordecaiEnemyAIController : public AAIController
{
	GENERATED_BODY()

public:
	AMordecaiEnemyAIController();

	// --- Configurable Parameters (AC-051.11) ---

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Mordecai|AI")
	float AggroRange = 900.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Mordecai|AI")
	float AttackRange = 200.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Mordecai|AI")
	float LeashRange = 1800.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Mordecai|AI")
	float AttackCooldownMs = 1500.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Mordecai|AI")
	TSubclassOf<UGameplayAbility> MeleeAttackAbilityClass;

	// --- State Access ---

	UFUNCTION(BlueprintCallable, Category = "Mordecai|AI")
	EMordecaiEnemyAIState GetCurrentState() const { return CurrentState; }

	// --- State Transitions (public for testing + internal use) ---

	void TransitionToState(EMordecaiEnemyAIState NewState);

	// --- Tag Callbacks (public for testing) ---

	void OnPostureBrokenTagChanged(const FGameplayTag Tag, int32 NewCount);
	void OnDeadTagChanged(const FGameplayTag Tag, int32 NewCount);

	// --- Distance Evaluation Helpers (public for testing) ---

	bool IsInAggroRange(float Distance) const { return Distance <= AggroRange; }
	bool IsInAttackRange(float Distance) const { return Distance <= AttackRange; }
	bool IsOutsideLeashRange(float Distance) const { return Distance > LeashRange; }

	// --- Attack Notification ---

	void NotifyAttackCompleted();

	// --- Spawn Location ---

	FVector GetSpawnLocation() const { return SpawnLocation; }

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;
	virtual void OnPossess(APawn* InPawn) override;
	virtual void OnUnPossess() override;

private:
	void BindTagDelegates();
	void UnbindTagDelegates();

	AActor* FindPlayerTarget() const;

	EMordecaiEnemyAIState CurrentState = EMordecaiEnemyAIState::Idle;
	FVector SpawnLocation = FVector::ZeroVector;

	TWeakObjectPtr<AMordecaiEnemyCharacter> CachedEnemy;
	TWeakObjectPtr<UAbilitySystemComponent> CachedASC;

	float RecoverElapsedTime = 0.f;
	bool bAttackInProgress = false;
};
