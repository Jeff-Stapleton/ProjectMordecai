// Project Mordecai — Native Gameplay Tag Declarations (US-002)

#pragma once

#include "NativeGameplayTags.h"

namespace MordecaiGameplayTags
{
	// Damage type tags (AC-002.11)
	// Physical subtypes are nested under Physical per damage_types_v1.md
	LYRAGAME_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Damage_Physical);
	LYRAGAME_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Damage_Physical_Slash);
	LYRAGAME_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Damage_Physical_Pierce);
	LYRAGAME_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Damage_Physical_Blunt);
	LYRAGAME_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Damage_Fire);
	LYRAGAME_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Damage_Frost);
	LYRAGAME_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Damage_Lightning);
	LYRAGAME_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Damage_Poison);
	LYRAGAME_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Damage_Corrosive);
	LYRAGAME_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Damage_Arcane);
	LYRAGAME_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Damage_Shadow);
	LYRAGAME_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Damage_Radiant);
}
