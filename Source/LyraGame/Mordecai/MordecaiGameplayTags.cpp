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

	// State tags (US-006)
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(State_Blocking,     "Mordecai.State.Blocking",     "Character is actively blocking");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(State_PerfectBlock,  "Mordecai.State.PerfectBlock",  "Perfect block timing window is active");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(State_GuardBroken,   "Mordecai.State.GuardBroken",   "Guard broken — stamina depleted while blocking");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(State_Parrying,      "Mordecai.State.Parrying",      "Character is in parry attempt window");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(State_Parried,       "Mordecai.State.Parried",       "Character was parried — staggered by defender");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(State_Riposte,       "Mordecai.State.Riposte",       "Character has riposte/punish window after parry");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(State_ParryWhiff,    "Mordecai.State.ParryWhiff",    "Parry failed — animation-locked punish window");

	// State tags (US-007)
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(State_KnockedDown,    "Mordecai.State.KnockedDown",    "Character is knocked down — extended vulnerability window");

	// Stamina tier tags (US-008)
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Stamina_Tier_Green,     "Mordecai.Stamina.Tier.Green",     "Stamina above 66% — full effectiveness + bonus");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Stamina_Tier_Yellow,    "Mordecai.Stamina.Tier.Yellow",    "Stamina 33-66% — normal effectiveness");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Stamina_Tier_Red,       "Mordecai.Stamina.Tier.Red",       "Stamina below 33% — reduced effectiveness");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Stamina_Tier_Exhausted, "Mordecai.Stamina.Tier.Exhausted", "Stamina at or below 0 — heavily reduced effectiveness");

	// State tags (US-008)
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(State_Sprinting,          "Mordecai.State.Sprinting",          "Character is currently sprinting");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(State_StaminaRegenPaused, "Mordecai.State.StaminaRegenPaused", "Stamina regeneration is paused after consumption");

	// Status effect tags (US-007 stubs — no effect until Epic 4)
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Status_Weakened,      "Mordecai.Status.Weakened",      "Weakened — reduces outgoing posture damage");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Status_Brittle,       "Mordecai.Status.Brittle",       "Brittle — increases incoming posture damage");

	// Event tags (US-006)
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Event_ParryInput,    "Mordecai.Event.ParryInput",    "Simultaneous block+attack input detected for parry");

	// Event tags (US-011)
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Event_SkillMilestone, "Mordecai.Event.SkillMilestone", "Fired when a skill reaches a milestone rank (1/5/10/15/20)");

	// Event tags (US-012)
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Event_FeatUnlocked, "Mordecai.Event.FeatUnlocked", "Fired when a feat is earned via gameplay achievement");

	// SetByCaller data tags (US-004)
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(SetByCaller_HealthDamage,  "Mordecai.SetByCaller.HealthDamage",  "SetByCaller tag for health damage magnitude");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(SetByCaller_PostureDamage, "Mordecai.SetByCaller.PostureDamage", "SetByCaller tag for posture damage magnitude");

	// SetByCaller data tags (US-008)
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(SetByCaller_StaminaCost, "Mordecai.Data.StaminaCost", "SetByCaller tag for stamina cost magnitude");

	// Team tags (US-009)
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Team_Enemy, "Mordecai.Team.Enemy", "Actor belongs to the enemy team — used by aim assist filtering");

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
