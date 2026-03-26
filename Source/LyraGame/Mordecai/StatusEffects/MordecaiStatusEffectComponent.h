// Project Mordecai — Status Effect Component (US-013)

#pragma once

#include "Components/ActorComponent.h"
#include "GameplayTagContainer.h"
#include "ActiveGameplayEffectHandle.h"

#include "MordecaiStatusEffectComponent.generated.h"

class UAbilitySystemComponent;
class UGameplayEffect;

/**
 * UMordecaiStatusEffectComponent
 *
 *	Convenience component on actors that can receive status effects (AC-013.4).
 *	Wraps ASC operations for status effect application, removal, and queries.
 *	Actual status state lives in the ASC's active GE list — this component
 *	is a thin delegation layer.
 */
UCLASS(BlueprintType, meta = (BlueprintSpawnableComponent))
class LYRAGAME_API UMordecaiStatusEffectComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UMordecaiStatusEffectComponent(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	// --- Application ---

	/** Apply a status effect GE to this actor (AC-013.4, AC-013.5). */
	UFUNCTION(BlueprintCallable, Category = "Mordecai|StatusEffects")
	FActiveGameplayEffectHandle ApplyStatusEffect(TSubclassOf<UGameplayEffect> GEClass, AActor* Instigator, float Level = 1.0f);

	// --- Removal ---

	/** Remove a status effect by its status tag (AC-013.4). */
	UFUNCTION(BlueprintCallable, Category = "Mordecai|StatusEffects")
	void RemoveStatusEffect(FGameplayTag StatusTag);

	/** Remove all active status effects (AC-013.4). */
	UFUNCTION(BlueprintCallable, Category = "Mordecai|StatusEffects")
	void RemoveAllStatusEffects();

	/** Remove all status effects belonging to a category (AC-013.9). */
	UFUNCTION(BlueprintCallable, Category = "Mordecai|StatusEffects")
	void RemoveStatusEffectsByCategory(FGameplayTag CategoryTag);

	// --- Queries ---

	/** Returns true if the given status effect is currently active (AC-013.4). */
	UFUNCTION(BlueprintCallable, Category = "Mordecai|StatusEffects")
	bool HasStatusEffect(FGameplayTag StatusTag) const;

	/** Returns all currently active status effect tags (AC-013.4). */
	UFUNCTION(BlueprintCallable, Category = "Mordecai|StatusEffects")
	FGameplayTagContainer GetActiveStatusTags() const;

	// --- Bleeding Management (US-014) ---

	/**
	 * Notify the component that the owner took damage.
	 * If Bleeding is active, this triggers hit-refresh (remove + re-apply) and resets clot timer.
	 */
	UFUNCTION(BlueprintCallable, Category = "Mordecai|StatusEffects")
	void NotifyDamageTaken();

	/** Start tracking Bleeding clot timer and hit-refresh. Called automatically on Bleeding application. */
	void StartBleedingTracking(TSubclassOf<UGameplayEffect> BleedingGEClass, float ClotTimeSec);

	/** Stop tracking Bleeding mechanics. Called when Bleeding is removed. */
	void StopBleedingTracking();

	/** Force clot expiry for testing. */
	void ForceBleedingClotExpiry();

	// --- Frostbitten Management (US-015) ---

	/** Start tracking Frostbitten stacks. Called automatically on Frostbitten application. */
	void StartFrostbittenTracking(int32 MaxStacks);

	/** Stop tracking Frostbitten mechanics. */
	void StopFrostbittenTracking();

	/** Get current Frostbitten stack count (0 if not active). */
	int32 GetFrostbittenStackCount() const;

	/**
	 * Reduce Frostbitten stacks by CountToRemove.
	 * Interface for AC-015.7: Perfect dodge breaks stacks.
	 * TODO(DECISION): Remove 1 stack or all stacks on perfect dodge.
	 */
	void ReduceFrostbittenStacks(int32 CountToRemove = 1);

	// --- Shocked Management (US-015) ---

	/** Start tracking Shocked mechanics. Called automatically on Shocked application. */
	void StartShockedTracking(float MicroStunChance, float CastInterruptChance);

	/** Stop tracking Shocked mechanics. */
	void StopShockedTracking();

	/** Get current Shocked stack count (0 if not active). */
	int32 GetShockedStackCount() const;

	/**
	 * Reduce Shocked stacks by CountToRemove.
	 * Interface for AC-015.14: Perfect block clears 1 stack.
	 */
	void ReduceShockedStacks(int32 CountToRemove = 1);

	// --- Dependency Injection (for testing) ---

	/** Override the ASC reference for unit tests without a full actor setup. */
	void SetAbilitySystemComponentOverride(UAbilitySystemComponent* InASC);

private:
	/** Find the ASC on the owning actor (or use the override). */
	UAbilitySystemComponent* GetAbilitySystemComponent() const;

	/** Override for ASC (testing). */
	UPROPERTY()
	TObjectPtr<UAbilitySystemComponent> ASCOverride;

	// --- Bleeding tracking state (US-014) ---
	void OnBleedingClotExpired();

	UPROPERTY()
	TSubclassOf<UGameplayEffect> CachedBleedingGEClass;

	FTimerHandle BleedingClotTimerHandle;
	float CachedBleedingClotDuration = 4.0f;
	bool bTrackingBleeding = false;

	// --- Frostbitten tracking state (US-015) ---
	FActiveGameplayEffectHandle FrostbittenActiveHandle;
	int32 CachedFrostbittenMaxStacks = 5;
	bool bTrackingFrostbitten = false;

	// --- Shocked tracking state (US-015) ---
	FActiveGameplayEffectHandle ShockedActiveHandle;
	float CachedShockedMicroStunChance = 0.15f;
	float CachedShockedCastInterruptChance = 0.25f;
	bool bTrackingShocked = false;
};
