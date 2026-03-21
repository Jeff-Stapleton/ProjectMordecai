// Project Mordecai — Native Gameplay Tag Definitions (US-002)

#include "Mordecai/MordecaiGameplayTags.h"

namespace MordecaiGameplayTags
{
	// State tags (US-003)
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(State_Airborne, "Mordecai.State.Airborne", "Target is currently airborne (jumping/launched)");

	// State tags (US-004)
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(State_Attacking,   "Mordecai.State.Attacking",   "Character is currently executing an attack");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(State_ComboWindow, "Mordecai.State.ComboWindow", "Combo input window is open during Recovery phase");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(State_Rooted,      "Mordecai.State.Rooted",      "Character movement is disabled");

	// State tags (US-005)
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(State_Dodging,       "Mordecai.State.Dodging",       "Character is mid-dodge with i-frames");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(State_PerfectDodge,  "Mordecai.State.PerfectDodge",  "Perfect dodge timing window is active");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(State_DodgeCooldown, "Mordecai.State.DodgeCooldown", "Dodge is on cooldown — cannot re-dodge");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(State_PostureBroken, "Mordecai.State.PostureBroken", "Character posture is broken — staggered");

	// SetByCaller data tags (US-004)
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(SetByCaller_HealthDamage,  "Mordecai.SetByCaller.HealthDamage",  "SetByCaller tag for health damage magnitude");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(SetByCaller_PostureDamage, "Mordecai.SetByCaller.PostureDamage", "SetByCaller tag for posture damage magnitude");

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
