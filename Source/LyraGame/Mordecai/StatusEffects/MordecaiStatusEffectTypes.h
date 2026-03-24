// Project Mordecai — Status Effect Types (US-013)

#pragma once

#include "GameplayTagContainer.h"

#include "MordecaiStatusEffectTypes.generated.h"

/** Categories for grouping status effects (US-013, AC-013.12). */
UENUM(BlueprintType)
enum class EMordecaiStatusEffectCategory : uint8
{
	DoT           UMETA(DisplayName = "DoT"),
	Debuff        UMETA(DisplayName = "Debuff"),
	Control       UMETA(DisplayName = "Control"),
	Buff          UMETA(DisplayName = "Buff"),
	Environmental UMETA(DisplayName = "Environmental"),
};

/**
 * TODO(DECISION): Stacking policy per agent_rules_v2.md Open Item #2.
 * Default: Refresh. Final stacking behavior TBD.
 */
UENUM(BlueprintType)
enum class EMordecaiStackingPolicy : uint8
{
	Refresh       UMETA(DisplayName = "Refresh Duration"),
	StackCount    UMETA(DisplayName = "Stack Count"),
	StackDuration UMETA(DisplayName = "Stack Duration"),
	Independent   UMETA(DisplayName = "Independent"),
};

/**
 * Returns the category for a given status effect gameplay tag (AC-013.13).
 * E.g., Mordecai.Status.Burning → DoT, Mordecai.Status.Focused → Buff.
 */
LYRAGAME_API EMordecaiStatusEffectCategory GetStatusEffectCategory(FGameplayTag StatusTag);

/**
 * Returns all status effect tags belonging to the given category.
 * Used by cleanse-by-category to know which statuses to remove.
 */
LYRAGAME_API TArray<FGameplayTag> GetStatusTagsForCategory(EMordecaiStatusEffectCategory Category);

/**
 * Returns the immunity tag corresponding to a status tag.
 * E.g., Mordecai.Status.Burning → Mordecai.Immunity.Burning.
 */
LYRAGAME_API FGameplayTag GetImmunityTagForStatus(FGameplayTag StatusTag);
