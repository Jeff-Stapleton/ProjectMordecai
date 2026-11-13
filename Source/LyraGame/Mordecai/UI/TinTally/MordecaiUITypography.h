// Project Mordecai — Tin Tally runtime typography helpers.

#pragma once

#include "CommonTextBlock.h"
#include "Components/TextBlock.h"

namespace MordecaiUITypography
{
	struct FCachedStyle
	{
		FSlateFontInfo Font;
		float LineHeightPercentage = 1.f;
		bool bFound = false;
	};

	inline bool Apply(UTextBlock* Text, const TCHAR* StyleName)
	{
		if (!Text)
		{
			return false;
		}

		static TMap<FName, FCachedStyle> StyleCache;
		const FName StyleKey(StyleName);
		FCachedStyle* Cached = StyleCache.Find(StyleKey);
		if (!Cached)
		{
			FCachedStyle Resolved;
			const FString ClassPath = FString::Printf(
				TEXT("/MordecaiCore/UI/TinTally/Fonts/Styles/TS_Tally_%s.TS_Tally_%s_C"),
				StyleName,
				StyleName);
			if (UClass* StyleClass =
				LoadClass<UCommonTextStyle>(nullptr, *ClassPath))
			{
				const UCommonTextStyle* Style =
					GetDefault<UCommonTextStyle>(StyleClass);
				Style->GetFont(Resolved.Font);
				Resolved.LineHeightPercentage =
					Style->GetLineHeightPercentage();
				Resolved.bFound = true;
			}
			Cached = &StyleCache.Add(StyleKey, MoveTemp(Resolved));
		}

		if (!Cached->bFound)
		{
			return false;
		}
		Text->SetFont(Cached->Font);
		Text->SetLineHeightPercentage(Cached->LineHeightPercentage);
		return true;
	}
}
