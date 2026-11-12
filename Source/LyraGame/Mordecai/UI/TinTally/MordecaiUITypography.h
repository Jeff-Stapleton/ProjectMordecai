// Project Mordecai — Tin Tally runtime typography helpers.

#pragma once

#include "CommonTextBlock.h"
#include "Components/TextBlock.h"

namespace MordecaiUITypography
{
	inline bool Apply(UTextBlock* Text, const TCHAR* StyleName)
	{
		if (!Text)
		{
			return false;
		}
		const FString ClassPath = FString::Printf(
			TEXT("/MordecaiCore/UI/TinTally/Fonts/Styles/TS_Tally_%s.TS_Tally_%s_C"),
			StyleName,
			StyleName);
		UClass* StyleClass = LoadClass<UCommonTextStyle>(nullptr, *ClassPath);
		if (!StyleClass)
		{
			return false;
		}
		const UCommonTextStyle* Style = GetDefault<UCommonTextStyle>(StyleClass);
		FSlateFontInfo Font;
		Style->GetFont(Font);
		Text->SetFont(Font);
		Text->SetLineHeightPercentage(Style->GetLineHeightPercentage());
		return true;
	}
}
