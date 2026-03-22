// Project Mordecai

#pragma once

#include "CoreMinimal.h"

/**
 * FMordecaiAttributeScaling
 *
 *	Static utility for attribute scaling math.
 *	Contains the Effective Mod diminishing returns formula from character_attributes_v1.
 */
struct LYRAGAME_API FMordecaiAttributeScaling
{
	/**
	 * Calculates the Effective Mod value using a 4-band diminishing returns formula.
	 *
	 * Band 1 (Mod 1-5):   100% efficiency
	 * Band 2 (Mod 6-10):   70% efficiency
	 * Band 3 (Mod 11-15):  45% efficiency
	 * Band 4 (Mod 16+):    25% efficiency
	 *
	 * Formula: min(Mod,5) + max(min(Mod-5,5),0)*0.7 + max(min(Mod-10,5),0)*0.45 + max(Mod-15,0)*0.25
	 */
	static float CalculateEffectiveMod(float RawMod);
};
