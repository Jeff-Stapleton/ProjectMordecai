// Project Mordecai — Status Effect Types (US-013)

#include "Mordecai/StatusEffects/MordecaiStatusEffectTypes.h"
#include "Mordecai/MordecaiGameplayTags.h"

#define LOCTEXT_NAMESPACE "MordecaiStatusEffects"

namespace
{
	FMordecaiStatusDisplayData StatusRow(
		const FGameplayTag Tag,
		const FText& DisplayName,
		const TCHAR* Glyph,
		const TCHAR* GlyphColorToken,
		const EMordecaiStatusEffectCategory DisplaySortCategory,
		const float DefaultDuration,
		const int32 MaxVisibleStacks = 0,
		const EMordecaiStatusDisplayPolarity DisplayPolarity =
			EMordecaiStatusDisplayPolarity::Debuff,
		const FText& FixedValueText = FText::GetEmpty())
	{
		FMordecaiStatusDisplayData Row;
		Row.Tag = Tag;
		Row.DisplayName = DisplayName;
		Row.GlyphName = FName(Glyph);
		Row.GlyphColorToken = FName(GlyphColorToken);
		Row.DisplaySortCategory = DisplaySortCategory;
		Row.DisplayPolarity = DisplayPolarity;
		Row.DefaultDuration = DefaultDuration;
		Row.MaxVisibleStacks = MaxVisibleStacks;
		Row.FixedValueText = FixedValueText;
		return Row;
	}

	struct FStatusCategoryEntry
	{
		FGameplayTag Tag;
		EMordecaiStatusEffectCategory Category;
	};

	/** Exact gameplay map from main 3d9f08b4. UI display rows must not change it. */
	const TArray<FStatusCategoryEntry>& GetGameplayCategoryMap()
	{
		using namespace MordecaiGameplayTags;
		static const TArray<FStatusCategoryEntry> Map = {
			{ Status_Burning, EMordecaiStatusEffectCategory::DoT },
			{ Status_Bleeding, EMordecaiStatusEffectCategory::DoT },
			{ Status_Poisoned, EMordecaiStatusEffectCategory::DoT },
			{ Status_Frostbitten, EMordecaiStatusEffectCategory::Debuff },
			{ Status_Weakened, EMordecaiStatusEffectCategory::Debuff },
			{ Status_Brittle, EMordecaiStatusEffectCategory::Debuff },
			{ Status_Cursed, EMordecaiStatusEffectCategory::Debuff },
			{ Status_Corroded, EMordecaiStatusEffectCategory::Debuff },
			{ Status_Exposed, EMordecaiStatusEffectCategory::Debuff },
			{ Status_Shocked, EMordecaiStatusEffectCategory::Control },
			{ Status_Silenced, EMordecaiStatusEffectCategory::Control },
			{ Status_Rooted, EMordecaiStatusEffectCategory::Control },
			{ Status_Blinded, EMordecaiStatusEffectCategory::Control },
			{ Status_Fear, EMordecaiStatusEffectCategory::Control },
			{ Status_Frozen, EMordecaiStatusEffectCategory::Control },
			{ Status_MicroStunned, EMordecaiStatusEffectCategory::Control },
			{ Status_Focused, EMordecaiStatusEffectCategory::Buff },
			{ Status_Drenched, EMordecaiStatusEffectCategory::Environmental },
		};
		return Map;
	}
}

const TArray<FMordecaiStatusDisplayData>& GetAllStatusEffectDisplayData()
{
	using namespace MordecaiGameplayTags;
	static const TArray<FMordecaiStatusDisplayData> Rows = {
		StatusRow(Status_Burning, LOCTEXT("Burning", "Burning"), TEXT("burning"),
			TEXT("dmg-fire"), EMordecaiStatusEffectCategory::DoT, 5.f),
		StatusRow(Status_Bleeding, LOCTEXT("Bleeding", "Bleeding"), TEXT("bleeding"),
			TEXT("vital"), EMordecaiStatusEffectCategory::DoT, 6.f),
		StatusRow(Status_Poisoned, LOCTEXT("Poisoned", "Poisoned"), TEXT("poisoned"),
			TEXT("dmg-poison"), EMordecaiStatusEffectCategory::DoT, 8.f),
		StatusRow(Status_Frostbitten, LOCTEXT("Frostbitten", "Frostbitten"), TEXT("frostbitten"),
			TEXT("dmg-frost"), EMordecaiStatusEffectCategory::Debuff, 6.f, 4),
		StatusRow(Status_Frozen, LOCTEXT("Frozen", "Frozen"), TEXT("frozen"),
			TEXT("dmg-frost"), EMordecaiStatusEffectCategory::Control, 2.f),
		StatusRow(Status_Shocked, LOCTEXT("Shocked", "Shocked"), TEXT("shocked"),
			TEXT("dmg-lightning"), EMordecaiStatusEffectCategory::Control, 5.f, 3),
		StatusRow(Status_Weakened, LOCTEXT("Weakened", "Weakened"), TEXT("weakened"),
			TEXT("text"), EMordecaiStatusEffectCategory::Debuff, 8.f),
		StatusRow(Status_Brittle, LOCTEXT("Brittle", "Brittle"), TEXT("brittle"),
			TEXT("dmg-frost"), EMordecaiStatusEffectCategory::Debuff, 7.f),
		StatusRow(Status_Silenced, LOCTEXT("Silenced", "Silenced"), TEXT("silenced"),
			TEXT("text"), EMordecaiStatusEffectCategory::Control, 5.f),
		StatusRow(Status_Rooted, LOCTEXT("Rooted", "Rooted"), TEXT("rooted"),
			TEXT("text"), EMordecaiStatusEffectCategory::Control, 3.f),
		StatusRow(Status_Blinded, LOCTEXT("Blinded", "Blinded"), TEXT("blinded"),
			TEXT("text"), EMordecaiStatusEffectCategory::Control, 4.f),
		StatusRow(Status_Fear, LOCTEXT("Fear", "Fear"), TEXT("fear"),
			TEXT("dmg-shadow-edge"), EMordecaiStatusEffectCategory::Control, 5.f),
		StatusRow(Status_Cursed, LOCTEXT("Cursed", "Cursed"), TEXT("cursed"),
			TEXT("dmg-shadow-edge"), EMordecaiStatusEffectCategory::Debuff, 10.f),
		StatusRow(Status_Exposed, LOCTEXT("Exposed", "Exposed"), TEXT("exposed"),
			TEXT("danger-text"), EMordecaiStatusEffectCategory::Debuff, 5.f, 0,
			EMordecaiStatusDisplayPolarity::Debuff, LOCTEXT("ExposedValue", "1 hit")),
		StatusRow(Status_Corroded, LOCTEXT("Corroded", "Corroded"), TEXT("corroded"),
			TEXT("dmg-corrosive"), EMordecaiStatusEffectCategory::Debuff, 10.f),
		StatusRow(Status_Drenched, LOCTEXT("Drenched", "Drenched"), TEXT("drenched"),
			TEXT("dmg-frost"), EMordecaiStatusEffectCategory::Environmental, 15.f),
		StatusRow(Status_Sleeping, LOCTEXT("Sleeping", "Sleeping"), TEXT("sleeping"),
			TEXT("text"), EMordecaiStatusEffectCategory::Control, 0.f),
		StatusRow(Status_Focused, LOCTEXT("Focused", "Focused"), TEXT("focused"),
			TEXT("ink"), EMordecaiStatusEffectCategory::Buff, 0.f, 0,
			EMordecaiStatusDisplayPolarity::Buff, LOCTEXT("FocusedValue", "3/3")),
		StatusRow(Status_StoneSkin, LOCTEXT("StoneSkin", "Stone Skin"), TEXT("stoneskin"),
			TEXT("ink"), EMordecaiStatusEffectCategory::Buff, 10.f, 0,
			EMordecaiStatusDisplayPolarity::Buff),
		StatusRow(Status_FireWard, LOCTEXT("FireWard", "Fire Ward"), TEXT("fireward"),
			TEXT("ink"), EMordecaiStatusEffectCategory::Buff, 0.f, 0,
			EMordecaiStatusDisplayPolarity::Buff),
		StatusRow(Status_Blessed, LOCTEXT("Blessed", "Blessed"), TEXT("blessed"),
			TEXT("ink"), EMordecaiStatusEffectCategory::Buff, 0.f, 0,
			EMordecaiStatusDisplayPolarity::Buff),
		StatusRow(Status_Restoration, LOCTEXT("Restoration", "Restoration"), TEXT("restoration"),
			TEXT("ink"), EMordecaiStatusEffectCategory::Buff, 5.f, 0,
			EMordecaiStatusDisplayPolarity::Buff),
		StatusRow(Status_EnchantedWeapon, LOCTEXT("EnchantedWeapon", "Enchanted Weapon"), TEXT("enchanted"),
			TEXT("ink"), EMordecaiStatusEffectCategory::Buff, 0.f, 0,
			EMordecaiStatusDisplayPolarity::Buff),
		StatusRow(Status_Blurred, LOCTEXT("Blurred", "Blurred"), TEXT("blurred"),
			TEXT("ink"), EMordecaiStatusEffectCategory::Buff, 0.f, 0,
			EMordecaiStatusDisplayPolarity::Buff),
		StatusRow(Status_Resolved, LOCTEXT("Resolved", "Resolved"), TEXT("resolved"),
			TEXT("ink"), EMordecaiStatusEffectCategory::Buff, 0.f, 0,
			EMordecaiStatusDisplayPolarity::Buff),
	};
	return Rows;
}

const FMordecaiStatusDisplayData* FindStatusEffectDisplayData(const FGameplayTag StatusTag)
{
	return GetAllStatusEffectDisplayData().FindByPredicate(
		[StatusTag](const FMordecaiStatusDisplayData& Row)
		{
			return Row.Tag == StatusTag;
		});
}

EMordecaiStatusEffectCategory GetStatusEffectCategory(FGameplayTag StatusTag)
{
	for (const FStatusCategoryEntry& Entry : GetGameplayCategoryMap())
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
	for (const FStatusCategoryEntry& Entry : GetGameplayCategoryMap())
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

#undef LOCTEXT_NAMESPACE
