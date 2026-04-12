// Project Mordecai — Perfect Action Tracker Component (US-018)

#pragma once

#include "Components/ActorComponent.h"
#include "GameplayTagContainer.h"

#include "MordecaiPerfectActionTracker.generated.h"

class UAbilitySystemComponent;
struct FGameplayEventData;

/**
 * UMordecaiPerfectActionTracker
 *
 *	Tracks consecutive perfect actions (perfect dodge, block, parry) and grants
 *	Focused buff when the streak reaches the configured threshold (AC-018.9).
 *
 *	- Listens for Mordecai.Event.PerfectAction → increments counter
 *	- Listens for Mordecai.Event.DamageTaken → resets counter + removes Focused
 *	- Listens for Mordecai.Event.NonPerfectAction → resets counter
 *	- At threshold → applies UMordecaiGE_Focused
 */
UCLASS(BlueprintType, meta = (BlueprintSpawnableComponent))
class LYRAGAME_API UMordecaiPerfectActionTracker : public UActorComponent
{
	GENERATED_BODY()

public:
	UMordecaiPerfectActionTracker(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	/** Number of consecutive perfect actions needed to grant Focused (AC-018.9). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Mordecai|Focused")
	int32 FocusedStreakThreshold = 3;

	/** Current consecutive perfect action count. */
	UFUNCTION(BlueprintCallable, Category = "Mordecai|Focused")
	int32 GetCurrentStreak() const { return ConsecutivePerfectActions; }

	/** Returns true if Focused is currently active on the owner. */
	UFUNCTION(BlueprintCallable, Category = "Mordecai|Focused")
	bool IsFocused() const;

	/** Override ASC reference for testing. */
	void SetAbilitySystemComponentOverride(UAbilitySystemComponent* InASC);

	/** Begin listening for gameplay events. Call after ASC is available. */
	void StartTracking();

	/** Stop listening and clean up. */
	void StopTracking();

	/** Simulate a perfect action event (for testing). */
	void SimulatePerfectAction();

	/** Simulate a damage taken event (for testing). */
	void SimulateDamageTaken();

	/** Simulate a non-perfect action event (for testing). */
	void SimulateNonPerfectAction();

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	UAbilitySystemComponent* GetAbilitySystemComponent() const;

	void OnPerfectAction(const FGameplayEventData* Payload);
	void OnDamageTaken(const FGameplayEventData* Payload);
	void OnNonPerfectAction(const FGameplayEventData* Payload);

	void ApplyFocused();
	void RemoveFocused();
	void ResetStreak();

	UPROPERTY()
	TObjectPtr<UAbilitySystemComponent> ASCOverride;

	int32 ConsecutivePerfectActions = 0;
	bool bTracking = false;

	FDelegateHandle PerfectActionDelegateHandle;
	FDelegateHandle DamageTakenDelegateHandle;
	FDelegateHandle NonPerfectActionDelegateHandle;
};
