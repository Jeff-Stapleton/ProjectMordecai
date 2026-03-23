// Project Mordecai — Feat Component (US-012)

#pragma once

#include "Components/ActorComponent.h"
#include "GameplayTagContainer.h"
#include "GameplayAbilitySpec.h"
#include "Mordecai/Feats/MordecaiFeatTypes.h"

#include "MordecaiFeatComponent.generated.h"

class UMordecaiFeatDataAsset;
class UAbilitySystemComponent;
class UMordecaiSkillComponent;

/** Native C++ delegate fired when a feat is unlocked. */
DECLARE_MULTICAST_DELEGATE_OneParam(FOnMordecaiFeatUnlocked, FName /*FeatName*/);

/** Blueprint-exposed delegate for feat unlock events. */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnMordecaiFeatUnlockedBP, FName, FeatName);

/**
 * UMordecaiFeatComponent
 *
 *	ActorComponent on PlayerState that manages the feat system.
 *	Tracks unlock conditions, unlocked feats, and gameplay stat counters.
 *	When all conditions for a feat are met, auto-unlocks it and applies
 *	the appropriate effects/abilities to the character's ASC.
 */
UCLASS(BlueprintType, meta = (BlueprintSpawnableComponent))
class LYRAGAME_API UMordecaiFeatComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UMordecaiFeatComponent(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	// --- Feat Registration ---

	/** Register a feat data asset so the component can track and evaluate it. */
	void RegisterFeatDataAsset(UMordecaiFeatDataAsset* FeatData);

	// --- Stat Tracking ---

	/** Increment a tracked gameplay stat and trigger condition evaluation for relevant feats. */
	UFUNCTION(BlueprintCallable, Category = "Mordecai|Feats")
	void IncrementFeatStat(FName StatName, int32 Amount = 1);

	/** Get the current value of a tracked stat. Returns 0 if not tracked. */
	UFUNCTION(BlueprintCallable, Category = "Mordecai|Feats")
	int32 GetFeatStatValue(FName StatName) const;

	// --- Feat Queries ---

	/** Returns true if the named feat has been unlocked. */
	UFUNCTION(BlueprintCallable, Category = "Mordecai|Feats")
	bool HasFeat(FName FeatName) const;

	/** Returns the set of all unlocked feat names. */
	const TSet<FName>& GetUnlockedFeats() const { return UnlockedFeats; }

	/** Returns the applied record for a feat (what effects/abilities were applied on unlock). */
	UFUNCTION(BlueprintCallable, Category = "Mordecai|Feats")
	FMordecaiFeatAppliedRecord GetFeatAppliedRecord(FName FeatName) const;

	/** Returns true if the specified feat tag has been granted. */
	bool HasGrantedFeatTag(FGameplayTag Tag) const;

	// --- External Notifications (trigger re-evaluation) ---

	/** Notify the component that a gameplay tag changed on the character. */
	UFUNCTION(BlueprintCallable, Category = "Mordecai|Feats")
	void NotifyTagChanged(FGameplayTag Tag, bool bAdded);

	/** Notify the component that a skill rank changed. */
	UFUNCTION(BlueprintCallable, Category = "Mordecai|Feats")
	void NotifySkillRankChanged(FName SkillName, int32 NewRank);

	// --- Dependency Injection (for testing) ---

	/** Override the skill component reference (for unit tests without a world). */
	void SetSkillComponentOverride(UMordecaiSkillComponent* InSkillComp);

	// --- Delegates ---

	/** Native C++ delegate — fires when any feat is unlocked. */
	FOnMordecaiFeatUnlocked OnFeatUnlocked;

	/** Blueprint delegate — fires when any feat is unlocked. */
	UPROPERTY(BlueprintAssignable, Category = "Mordecai|Feats")
	FOnMordecaiFeatUnlockedBP OnFeatUnlockedBP;

	// --- Replication ---

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

private:
	/** Re-evaluate all pending (not yet unlocked) feats. */
	void ReevaluateAllFeats();

	/** Check all conditions for a feat and unlock if all pass. */
	void EvaluateFeatConditions(UMordecaiFeatDataAsset* FeatData);

	/** Evaluate a single condition against current state. */
	bool EvaluateCondition(const FMordecaiFeatCondition& Condition) const;

	/** Unlock a feat: apply effects, abilities, drawbacks, tags, fire events. */
	void UnlockFeat(UMordecaiFeatDataAsset* FeatData);

	/** Find the ASC on the owning actor. */
	UAbilitySystemComponent* FindAbilitySystemComponent() const;

	/** Find the skill component on the owning actor (or use override). */
	UMordecaiSkillComponent* FindSkillComponent() const;

	// --- Replicated State ---

	UPROPERTY(Replicated)
	TArray<FName> ReplicatedUnlockedFeats;

	// --- Non-Replicated State ---

	/** Set of unlocked feat names (fast lookup). */
	TSet<FName> UnlockedFeats;

	/** Tracked gameplay stats for condition evaluation. */
	TMap<FName, int32> TrackedStats;

	/** Registered feat data assets. */
	UPROPERTY()
	TMap<FName, TObjectPtr<UMordecaiFeatDataAsset>> RegisteredFeats;

	/** Records of what was applied per feat (for queries and testing). */
	TMap<FName, FMordecaiFeatAppliedRecord> FeatAppliedRecords;

	/** Gameplay tags active on the character (for TagPresent condition evaluation). */
	FGameplayTagContainer ActiveTags;

	/** Granted feat tags (pattern: Mordecai.Feat.<FeatName>). */
	TSet<FGameplayTag> GrantedFeatTags;

	/** Override for skill component (testing). */
	UPROPERTY()
	TObjectPtr<UMordecaiSkillComponent> SkillComponentOverride;

	/** Ability spec handles for granted feat abilities (for potential removal). */
	TMap<FName, TArray<FGameplayAbilitySpecHandle>> FeatAbilityHandles;
};
