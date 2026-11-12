// Project Mordecai — cooked Tin Tally foundation manifest (US-103)

#include "Mordecai/UI/TinTally/MordecaiUIFoundation.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(MordecaiUIFoundation)

bool UMordecaiUIPrimitiveStyleDataAsset::PopulateFromBuilder(FName PrimitiveName)
{
	const FMordecaiUIThemeColors DioramaColors = FMordecaiUIThemeColors::MakeDiorama();
	const FMordecaiUIThemeColors HighContrastColors =
		FMordecaiUIThemeColors::MakeHighContrast();
	if (PrimitiveName == TEXT("Plate"))
	{
		Diorama = FMordecaiUIPrimitiveBuilder::Plate(
			DioramaColors, EMordecaiUIPrintDepth::HUD, EMordecaiUITheme::Diorama);
		HighContrast = FMordecaiUIPrimitiveBuilder::Plate(
			HighContrastColors, EMordecaiUIPrintDepth::HUD, EMordecaiUITheme::HighContrast);
	}
	else if (PrimitiveName == TEXT("Pill"))
	{
		Diorama = FMordecaiUIPrimitiveBuilder::Pill(
			DioramaColors, EMordecaiUIPrintDepth::HUD, EMordecaiUITheme::Diorama);
		HighContrast = FMordecaiUIPrimitiveBuilder::Pill(
			HighContrastColors, EMordecaiUIPrintDepth::HUD, EMordecaiUITheme::HighContrast);
	}
	else if (PrimitiveName == TEXT("Tag"))
	{
		Diorama = FMordecaiUIPrimitiveBuilder::Tag(
			DioramaColors, EMordecaiUIPrintDepth::HUD, EMordecaiUITheme::Diorama);
		HighContrast = FMordecaiUIPrimitiveBuilder::Tag(
			HighContrastColors, EMordecaiUIPrintDepth::HUD, EMordecaiUITheme::HighContrast);
	}
	else if (PrimitiveName == TEXT("Well"))
	{
		Diorama = FMordecaiUIPrimitiveBuilder::Well(DioramaColors, false);
		HighContrast = FMordecaiUIPrimitiveBuilder::Well(HighContrastColors, false);
	}
	else if (PrimitiveName == TEXT("Keycap"))
	{
		Diorama = FMordecaiUIPrimitiveBuilder::Keycap(
			DioramaColors, EMordecaiKeycapShape::Square);
		HighContrast = FMordecaiUIPrimitiveBuilder::Keycap(
			HighContrastColors, EMordecaiKeycapShape::Square);
	}
	else if (PrimitiveName == TEXT("Focus"))
	{
		Diorama = FMordecaiUIPrimitiveBuilder::Focus(
			DioramaColors, 12.f, EMordecaiUITheme::Diorama);
		HighContrast = FMordecaiUIPrimitiveBuilder::Focus(
			HighContrastColors, 12.f, EMordecaiUITheme::HighContrast);
	}
	else
	{
		return false;
	}
	return true;
}
