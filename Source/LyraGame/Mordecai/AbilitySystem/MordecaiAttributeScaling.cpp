// Project Mordecai

#include "MordecaiAttributeScaling.h"

float FMordecaiAttributeScaling::CalculateEffectiveMod(float RawMod)
{
	return FMath::Min(RawMod, 5.0f)
		+ FMath::Max(FMath::Min(RawMod - 5.0f, 5.0f), 0.0f) * 0.7f
		+ FMath::Max(FMath::Min(RawMod - 10.0f, 5.0f), 0.0f) * 0.45f
		+ FMath::Max(RawMod - 15.0f, 0.0f) * 0.25f;
}
