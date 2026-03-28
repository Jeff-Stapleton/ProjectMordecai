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

	// Status effect tags (US-013)
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Status_Burning,     "Mordecai.Status.Burning",     "Burning — damage over time");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Status_Bleeding,    "Mordecai.Status.Bleeding",    "Bleeding — healing received reduced/blocked");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Status_Poisoned,    "Mordecai.Status.Poisoned",    "Poisoned — stamina regen suppressed");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Status_Frostbitten, "Mordecai.Status.Frostbitten", "Frostbitten — move/attack speed reduced");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Status_Shocked,     "Mordecai.Status.Shocked",     "Shocked — micro-stun risk, timing disruption");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Status_Weakened,    "Mordecai.Status.Weakened",    "Weakened — reduces outgoing posture damage");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Status_Brittle,     "Mordecai.Status.Brittle",     "Brittle — increases incoming posture damage");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Status_Silenced,    "Mordecai.Status.Silenced",    "Silenced — cannot cast spells");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Status_Rooted,      "Mordecai.Status.Rooted",      "Rooted — cannot move, can still attack/block/cast");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Status_Blinded,     "Mordecai.Status.Blinded",     "Blinded — reduced accuracy and detection");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Status_Fear,        "Mordecai.Status.Fear",        "Fear — composure collapse, guard instability");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Status_Cursed,      "Mordecai.Status.Cursed",      "Cursed — spell regen and healing reduced");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Status_Exposed,     "Mordecai.Status.Exposed",     "Exposed — next hit deals bonus damage");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Status_Corroded,    "Mordecai.Status.Corroded",    "Corroded — armor efficiency and block stability reduced");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Status_Drenched,    "Mordecai.Status.Drenched",    "Drenched — elemental interaction modifier");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Status_Focused,     "Mordecai.Status.Focused",     "Focused — perfect-action streak buff");

	// Status effect category tags (US-013)
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Status_Category_DoT,           "Mordecai.Status.Category.DoT",           "Damage-over-time status effects");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Status_Category_Debuff,        "Mordecai.Status.Category.Debuff",        "Stat/capability debuff status effects");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Status_Category_Control,       "Mordecai.Status.Category.Control",       "Movement/action control status effects");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Status_Category_Buff,          "Mordecai.Status.Category.Buff",          "Positive buff status effects");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Status_Category_Environmental, "Mordecai.Status.Category.Environmental", "Environmental interaction status effects");

	// Immunity tags (US-013)
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Immunity_Burning,     "Mordecai.Immunity.Burning",     "Immune to Burning status effect");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Immunity_Bleeding,    "Mordecai.Immunity.Bleeding",    "Immune to Bleeding status effect");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Immunity_Poisoned,    "Mordecai.Immunity.Poisoned",    "Immune to Poisoned status effect");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Immunity_Frostbitten, "Mordecai.Immunity.Frostbitten", "Immune to Frostbitten status effect");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Immunity_Shocked,     "Mordecai.Immunity.Shocked",     "Immune to Shocked status effect");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Immunity_Weakened,    "Mordecai.Immunity.Weakened",    "Immune to Weakened status effect");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Immunity_Brittle,     "Mordecai.Immunity.Brittle",     "Immune to Brittle status effect");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Immunity_Silenced,    "Mordecai.Immunity.Silenced",    "Immune to Silenced status effect");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Immunity_Rooted,      "Mordecai.Immunity.Rooted",      "Immune to Rooted status effect");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Immunity_Blinded,     "Mordecai.Immunity.Blinded",     "Immune to Blinded status effect");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Immunity_Fear,        "Mordecai.Immunity.Fear",        "Immune to Fear status effect");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Immunity_Cursed,      "Mordecai.Immunity.Cursed",      "Immune to Cursed status effect");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Immunity_Exposed,     "Mordecai.Immunity.Exposed",     "Immune to Exposed status effect");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Immunity_Corroded,    "Mordecai.Immunity.Corroded",    "Immune to Corroded status effect");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Immunity_Drenched,    "Mordecai.Immunity.Drenched",    "Immune to Drenched status effect");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Immunity_Focused,     "Mordecai.Immunity.Focused",     "Immune to Focused status effect");

	// Status tags (US-015)
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Status_Frozen,       "Mordecai.Status.Frozen",       "Frozen — full incapacitation from max frost stacks");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Status_MicroStunned, "Mordecai.Status.MicroStunned", "MicroStunned — brief interrupt from Shocked on-hit");

	// Immunity tags (US-015)
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Immunity_Frozen, "Mordecai.Immunity.Frozen", "Immune to Frozen status effect");

	// State tags (US-014)
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(State_Casting, "Mordecai.State.Casting", "Character is currently casting a spell");

	// State tags (US-050)
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(State_Dead, "Mordecai.State.Dead", "Character is dead — no further damage, movement, or actions");

	// Event tags (US-006)
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Event_ParryInput,    "Mordecai.Event.ParryInput",    "Simultaneous block+attack input detected for parry");

	// Event tags (US-011)
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Event_SkillMilestone, "Mordecai.Event.SkillMilestone", "Fired when a skill reaches a milestone rank (1/5/10/15/20)");

	// Event tags (US-012)
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Event_FeatUnlocked, "Mordecai.Event.FeatUnlocked", "Fired when a feat is earned via gameplay achievement");

	// Event tags (US-014)
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Event_CastInterrupted, "Mordecai.Event.CastInterrupted", "Cast was interrupted by a status effect (e.g. Burning)");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Event_DamageTaken,     "Mordecai.Event.DamageTaken",     "Character took damage from any source");

	// Event tags (US-050)
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Event_Death, "Mordecai.Event.Death", "Character has died — broadcast for loot, scoring, AI reaction");

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
