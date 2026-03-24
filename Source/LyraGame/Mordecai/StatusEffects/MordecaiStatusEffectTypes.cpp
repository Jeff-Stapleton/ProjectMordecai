// Project Mordecai — Status Effect Types (US-013)

#include "Mordecai/StatusEffects/MordecaiStatusEffectTypes.h"
#include "Mordecai/MordecaiGameplayTags.h"

// ---------------------------------------------------------------------------
// Category mapping — derived from status_effects_launch_set_v1.md
// ---------------------------------------------------------------------------
// DoT:           Burning, Bleeding, Poisoned
// Debuff:        Frostbitten, Weakened, Brittle, Cursed, Corroded, Exposed
// Control:       Shocked, Silenced, Rooted, Blinded, Fear
// Buff:          Focused
// Environmental: Drenched
// ---------------------------------------------------------------------------

namespace
{
	struct FStatusCategoryEntry
	{
		FGameplayTag Tag;
		EMordecaiStatusEffectCategory Category;
	};

	const TArray<FStatusCategoryEntry>& GetCategoryMap()
	{
		static TArray<FStatusCategoryEntry> Map;
		if (Map.Num() == 0)
		{
			using namespace MordecaiGameplayTags;
			Map = {
				// DoT
				{ Status_Burning,     EMordecaiStatusEffectCategory::DoT },
				{ Status_Bleeding,    EMordecaiStatusEffectCategory::DoT },
				{ Status_Poisoned,    EMordecaiStatusEffectCategory::DoT },
				// Debuff
				{ Status_Frostbitten, EMordecaiStatusEffectCategory::Debuff },
				{ Status_Weakened,    EMordecaiStatusEffectCategory::Debuff },
				{ Status_Brittle,     EMordecaiStatusEffectCategory::Debuff },
				{ Status_Cursed,      EMordecaiStatusEffectCategory::Debuff },
				{ Status_Corroded,    EMordecaiStatusEffectCategory::Debuff },
				{ Status_Exposed,     EMordecaiStatusEffectCategory::Debuff },
				// Control
				{ Status_Shocked,     EMordecaiStatusEffectCategory::Control },
				{ Status_Silenced,    EMordecaiStatusEffectCategory::Control },
				{ Status_Rooted,      EMordecaiStatusEffectCategory::Control },
				{ Status_Blinded,     EMordecaiStatusEffectCategory::Control },
				{ Status_Fear,        EMordecaiStatusEffectCategory::Control },
				// Buff
				{ Status_Focused,     EMordecaiStatusEffectCategory::Buff },
				// Environmental
				{ Status_Drenched,    EMordecaiStatusEffectCategory::Environmental },
			};
		}
		return Map;
	}
}

EMordecaiStatusEffectCategory GetStatusEffectCategory(FGameplayTag StatusTag)
{
	for (const FStatusCategoryEntry& Entry : GetCategoryMap())
	{
		if (Entry.Tag == StatusTag)
		{
			return Entry.Category;
		}
	}

	// Default to Debuff for unknown tags
	return EMordecaiStatusEffectCategory::Debuff;
}

TArray<FGameplayTag> GetStatusTagsForCategory(EMordecaiStatusEffectCategory Category)
{
	TArray<FGameplayTag> Result;
	for (const FStatusCategoryEntry& Entry : GetCategoryMap())
	{
		if (Entry.Category == Category)
		{
			Result.Add(Entry.Tag);
		}
	}
	return Result;
}

FGameplayTag GetImmunityTagForStatus(FGameplayTag StatusTag)
{
	// Map: Mordecai.Status.<Name> → Mordecai.Immunity.<Name>
	static const FString StatusPrefix = TEXT("Mordecai.Status.");
	static const FString ImmunityPrefix = TEXT("Mordecai.Immunity.");

	const FString StatusStr = StatusTag.ToString();
	if (StatusStr.StartsWith(StatusPrefix))
	{
		const FString StatusName = StatusStr.RightChop(StatusPrefix.Len());
		return FGameplayTag::RequestGameplayTag(FName(*(ImmunityPrefix + StatusName)), false);
	}

	return FGameplayTag();
}
