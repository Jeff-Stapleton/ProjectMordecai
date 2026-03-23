// Project Mordecai — Feat Types (US-012)

#pragma once

#include "GameplayTagContainer.h"

#include "MordecaiFeatTypes.generated.h"

/** Feat rarity tier — determines whether drawback effects are applied. */
UENUM(BlueprintType)
enum class EMordecaiFeatTier : uint8
{
	Common = 0,
	Rare,
	Legendary
};

/** Condition types for feat unlock requirements. */
UENUM(BlueprintType)
enum class EMordecaiFeatConditionType : uint8
{
	/** Tracked gameplay stat meets or exceeds a threshold (e.g., "EnemiesBurnedSimultaneously" >= 6). */
	StatThreshold = 0,

	/** Cumulative event count meets or exceeds a threshold (e.g., "TotalPerfectParries" >= 50). */
	EventCount,

	/** Character currently has a specified gameplay tag. */
	TagPresent,

	/** A specified skill rank meets or exceeds a threshold. */
	SkillRank
};

/**
 * FMordecaiFeatCondition
 *
 *	A single unlock condition for a feat. All conditions on a feat must be met (AND logic).
 *	The ConditionType determines which fields are relevant:
 *	  - StatThreshold/EventCount: StatName + ThresholdValue
 *	  - TagPresent: RequiredTag
 *	  - SkillRank: StatName (used as skill name) + ThresholdValue (minimum rank)
 */
USTRUCT(BlueprintType)
struct LYRAGAME_API FMordecaiFeatCondition
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Mordecai|Feats")
	EMordecaiFeatConditionType ConditionType = EMordecaiFeatConditionType::StatThreshold;

	/** Gameplay tag to check for TagPresent conditions. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Mordecai|Feats",
		meta = (EditCondition = "ConditionType == EMordecaiFeatConditionType::TagPresent"))
	FGameplayTag RequiredTag;

	/** Stat/event/skill name for StatThreshold, EventCount, and SkillRank conditions. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Mordecai|Feats",
		meta = (EditCondition = "ConditionType != EMordecaiFeatConditionType::TagPresent"))
	FName StatName;

	/** Threshold value — the stat/count/rank must meet or exceed this. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Mordecai|Feats")
	int32 ThresholdValue = 0;
};

/**
 * FMordecaiFeatAppliedRecord
 *
 *	Tracks what was applied when a feat was unlocked.
 *	Used for queries and testing.
 */
USTRUCT(BlueprintType)
struct LYRAGAME_API FMordecaiFeatAppliedRecord
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Mordecai|Feats")
	int32 GrantedEffectsCount = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Mordecai|Feats")
	int32 GrantedAbilitiesCount = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Mordecai|Feats")
	int32 DrawbackEffectsCount = 0;
};
