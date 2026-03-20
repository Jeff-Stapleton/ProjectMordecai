// Project Mordecai — Native Gameplay Tag Definitions (US-002)

#include "Mordecai/MordecaiGameplayTags.h"

namespace MordecaiGameplayTags
{
	// State tags (US-003)
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(State_Airborne, "Mordecai.State.Airborne", "Target is currently airborne (jumping/launched)");

	// Damage type tags (AC-002.11)
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Damage_Physical,        "Mordecai.Damage.Physical",        "Physical damage — direct bodily harm");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Damage_Physical_Slash,  "Mordecai.Damage.Physical.Slash",  "Slash subtype of Physical");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Damage_Physical_Pierce, "Mordecai.Damage.Physical.Pierce", "Pierce subtype of Physical");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Damage_Physical_Blunt,  "Mordecai.Damage.Physical.Blunt",  "Blunt subtype of Physical");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Damage_Fire,            "Mordecai.Damage.Fire",            "Fire damage — heat and combustion");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Damage_Frost,           "Mordecai.Damage.Frost",           "Frost damage — cold and freezing");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Damage_Lightning,       "Mordecai.Damage.Lightning",       "Lightning damage — electricity and shock");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Damage_Poison,          "Mordecai.Damage.Poison",          "Poison damage — toxins and venom");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Damage_Corrosive,       "Mordecai.Damage.Corrosive",       "Corrosive damage — material erosion");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Damage_Arcane,          "Mordecai.Damage.Arcane",          "Arcane damage — raw magical force");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Damage_Shadow,          "Mordecai.Damage.Shadow",          "Shadow damage — corruption and curses");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Damage_Radiant,         "Mordecai.Damage.Radiant",         "Radiant damage — sanctified and purifying");
}
