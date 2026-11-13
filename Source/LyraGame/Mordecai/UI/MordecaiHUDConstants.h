// Project Mordecai — shared combat HUD constants.

#pragma once

#include "CoreMinimal.h"

namespace MordecaiHUDConstants
{
	struct FCombatCalloutLayout
	{
		float FontSizePt;
		float SlotHeightPx;
		float BottomFromCenterPx;
		float ComboTopFromCenterPx;
	};

	/**
	 * Approved AC-106.4 policy: the callout grows upward from a fixed +180
	 * bottom while the combo remains fixed below it at +184.
	 */
	inline constexpr FCombatCalloutLayout GetCombatCalloutLayout(int32 TextSizeIndex)
	{
		return TextSizeIndex >= 2
			? FCombatCalloutLayout{42.f, 68.f, 180.f, 184.f}
			: TextSizeIndex == 1
				? FCombatCalloutLayout{36.f, 60.f, 180.f, 184.f}
				: FCombatCalloutLayout{30.f, 52.f, 180.f, 184.f};
	}

	inline constexpr float LowHealthThreshold = 0.25f;
	inline constexpr float StaminaGreenThreshold = 0.66f;
	inline constexpr float StaminaYellowThreshold = 0.33f;
}
