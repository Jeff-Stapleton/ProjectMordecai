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

	// --- Dependency Injection (for testing) ---

	/** Override the ASC reference for unit tests without a full actor setup. */
	void SetAbilitySystemComponentOverride(UAbilitySystemComponent* InASC);

private:
	/** Find the ASC on the owning actor (or use the override). */
	UAbilitySystemComponent* GetAbilitySystemComponent() const;

	/** Override for ASC (testing). */
	UPROPERTY()
	TObjectPtr<UAbilitySystemComponent> ASCOverride;
};
