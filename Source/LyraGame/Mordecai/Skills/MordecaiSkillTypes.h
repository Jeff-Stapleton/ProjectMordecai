// Project Mordecai — Skill Types (US-011)

#pragma once

#include "MordecaiSkillTypes.generated.h"

/**
 * EMordecaiSkillCategory
 *
 *	5 skill categories matching skill_sheet_v1.1.md.
 */
UENUM(BlueprintType)
enum class EMordecaiSkillCategory : uint8
{
	Weapon,
	DefenseArmor,
	MovementExploration,
	UtilityInteraction,
	Magic
};

/**
 * FMordecaiSkillRankEntry
 *
 *	Replicated entry for skill rank storage (TMap<FName,int32> not supported for replication).
 */
USTRUCT()
struct FMordecaiSkillRankEntry
{
	GENERATED_BODY()

	UPROPERTY()
	FName SkillName;

	UPROPERTY()
	int32 Rank = 0;
};

/**
 * FMordecaiSkillRankInfo
 *
 *	Lightweight struct for returning skill name + current rank in queries.
 */
USTRUCT(BlueprintType)
struct FMordecaiSkillRankInfo
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Mordecai|Skills")
	FName SkillName;

	UPROPERTY(BlueprintReadOnly, Category = "Mordecai|Skills")
	int32 CurrentRank = 0;
};
