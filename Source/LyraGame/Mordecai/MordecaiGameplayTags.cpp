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
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Immunity_Frozen,       "Mordecai.Immunity.Frozen",       "Immune to Frozen status effect");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Immunity_MicroStunned, "Mordecai.Immunity.MicroStunned", "Immune to MicroStunned status effect");

	// State tags (US-014)
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(State_Casting, "Mordecai.State.Casting", "Character is currently casting a spell");

	// State tags (US-019)
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(State_CastingSlow,   "Mordecai.State.CastingSlow",   "Character is casting a spell with reduced movement speed");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(State_CastingRooted, "Mordecai.State.CastingRooted", "Character is casting a spell and cannot move");

	// Ability tags (US-019)
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Ability_Spell, "Mordecai.Ability.Spell", "Identifies an ability as a spell");

	// Ability tags (US-020)
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Ability_Spell_Fireball,     "Mordecai.Ability.Spell.Fireball",     "Fireball spell ability");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Ability_Spell_ConeOfCold,   "Mordecai.Ability.Spell.ConeOfCold",   "Cone of Cold spell ability");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Ability_Spell_MagicMissile, "Mordecai.Ability.Spell.MagicMissile", "Magic Missile spell ability");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Ability_Spell_StoneSkin,    "Mordecai.Ability.Spell.StoneSkin",    "Stone Skin spell ability");

	// Status tags (US-020)
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Status_StoneSkin, "Mordecai.Status.StoneSkin", "Stone Skin buff is active — damage reduction applied");

	// Ability tags (US-021)
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Ability_Spell_Blink,    "Mordecai.Ability.Spell.Blink",    "Blink spell ability — short-range teleport");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Ability_Spell_Sleep,    "Mordecai.Ability.Spell.Sleep",    "Sleep spell ability — single-target crowd control");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Ability_Spell_FireWard, "Mordecai.Ability.Spell.FireWard", "Fire Ward spell ability — fire damage absorb shield");

	// Status tags (US-021)
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Status_Sleeping, "Mordecai.Status.Sleeping", "Sleeping — incapacitated, wakes on damage");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Status_FireWard, "Mordecai.Status.FireWard", "Fire Ward active — absorbing fire damage");

	// State tags (US-021)
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(State_Invulnerable, "Mordecai.State.Invulnerable", "Invulnerable — immune to all damage (Blink i-frames)");

	// Ability tags (US-022)
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Ability_Spell_Bless,       "Mordecai.Ability.Spell.Bless",       "Bless spell ability — self-targeted damage multiplier buff");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Ability_Spell_Restoration, "Mordecai.Ability.Spell.Restoration", "Restoration spell ability — self-targeted heal over time");

	// Status tags (US-022)
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Status_Blessed,     "Mordecai.Status.Blessed",     "Blessed — increased physical and magic damage multipliers");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Status_Restoration, "Mordecai.Status.Restoration", "Restoration — healing over time active");

	// Ability tags (US-023)
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Ability_Spell_Snare,         "Mordecai.Ability.Spell.Snare",         "Snare spell ability — single-target root");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Ability_Spell_Enfeeble,      "Mordecai.Ability.Spell.Enfeeble",      "Enfeeble spell ability — single-target Weakened debuff");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Ability_Spell_EnchantWeapon, "Mordecai.Ability.Spell.EnchantWeapon", "Enchant Weapon spell ability — elemental melee buff");

	// Status tags (US-023)
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Status_EnchantedWeapon, "Mordecai.Status.EnchantedWeapon", "Enchanted Weapon — melee attacks deal bonus elemental damage");

	// Event tags (US-019)
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Event_SpellCast, "Mordecai.Event.SpellCast", "Spell cast phase completed successfully");

	// SetByCaller data tags (US-019)
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(SetByCaller_SpellPointCost, "Mordecai.SetByCaller.SpellPointCost", "SetByCaller tag for spell point cost magnitude");

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

	// Event tags (US-053)
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Event_PlayerDeath, "Mordecai.Event.PlayerDeath", "Player character has died — broadcast for respawn, UI, game flow");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Event_EnemyKill,   "Mordecai.Event.EnemyKill",   "An enemy was killed — broadcast for scoring, arena tracking");

	// SetByCaller data tags (US-004)
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(SetByCaller_HealthDamage,  "Mordecai.SetByCaller.HealthDamage",  "SetByCaller tag for health damage magnitude");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(SetByCaller_PostureDamage, "Mordecai.SetByCaller.PostureDamage", "SetByCaller tag for posture damage magnitude");

	// SetByCaller data tags (US-008)
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(SetByCaller_StaminaCost, "Mordecai.Data.StaminaCost", "SetByCaller tag for stamina cost magnitude");

	// Team tags (US-009)
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Team_Enemy, "Mordecai.Team.Enemy", "Actor belongs to the enemy team — used by aim assist filtering");

	// Attack tags (US-016)
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Attack_Heavy, "Mordecai.Attack.Heavy", "Heavy/charged attack — partially bypasses Weakened penalty");

	// Ability category tags (US-017)
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Ability_Category_Spell,   "Mordecai.Ability.Category.Spell",   "Identifies an ability as a spell — blocked by Silenced");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Ability_Category_Magical, "Mordecai.Ability.Category.Magical", "Identifies an ability as magical — blocked by Silenced even if not a spell");

	// Status tags (US-059)
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Status_Resolved, "Mordecai.Status.Resolved", "Resolved — Fear cancelled by perfect block/parry during Fear");

	// Ability tags (US-060)
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Ability_Spell_Illusion, "Mordecai.Ability.Spell.Illusion", "Illusion spell ability — summon aggro-drawing decoy");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Ability_Spell_Blur,     "Mordecai.Ability.Spell.Blur",     "Blur spell ability — self-targeted evasion buff");

	// Status tags (US-060)
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Status_Blurred, "Mordecai.Status.Blurred", "Blurred — ranged projectile attacks have a chance to miss");

	// Team tags (US-060)
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Team_Ally, "Mordecai.Team.Ally", "Actor is an allied decoy — enemies prefer targeting it");

	// Event tags (US-018)
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Event_PerfectAction,    "Mordecai.Event.PerfectAction",    "Generic event for any perfect action (perfect dodge, perfect block, perfect parry)");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Event_NonPerfectAction, "Mordecai.Event.NonPerfectAction", "A non-perfect action occurred — resets Focused streak counter");

	// Event tags (US-017)
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Event_BreakFree, "Mordecai.Event.BreakFree", "Break-free action — heavy stamina cost to remove Rooted early");

	// Item type tags (US-032)
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Item_Type_Weapon,       "Mordecai.Item.Type.Weapon",       "Item is a weapon");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Item_Type_Armor,        "Mordecai.Item.Type.Armor",        "Item is armor");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Item_Type_Trinket,      "Mordecai.Item.Type.Trinket",      "Item is a trinket / accessory");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Item_Type_Consumable,   "Mordecai.Item.Type.Consumable",   "Item is a consumable (potion, scroll)");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Item_Type_Material,     "Mordecai.Item.Type.Material",     "Item is a crafting/upgrade material (auto-stored by default)");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Item_Type_TownResource, "Mordecai.Item.Type.TownResource", "Item is a town-level resource (auto-stored)");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Item_Type_UpgradeKey,   "Mordecai.Item.Type.UpgradeKey",   "Item is a persistent upgrade key");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Item_Type_QuestItem,    "Mordecai.Item.Type.QuestItem",    "Item is a quest-bound item");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Item_Type_MagicalItem,  "Mordecai.Item.Type.MagicalItem",  "Item is a magical item (may require identification)");

	// Item rarity tags (US-032)
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Item_Rarity_Common, "Mordecai.Item.Rarity.Common", "Common rarity");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Item_Rarity_Green,  "Mordecai.Item.Rarity.Green",  "Green (uncommon) rarity");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Item_Rarity_Blue,   "Mordecai.Item.Rarity.Blue",   "Blue (rare) rarity");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Item_Rarity_Purple, "Mordecai.Item.Rarity.Purple", "Purple (epic) rarity");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Item_Rarity_Red,    "Mordecai.Item.Rarity.Red",    "Red (legendary) rarity");

	// Weapon type tags (US-024)
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Weapon_Type_Sword,    "Mordecai.Weapon.Type.Sword",    "Character is wielding a sword-class weapon");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Weapon_Type_Axe,      "Mordecai.Weapon.Type.Axe",      "Character is wielding an axe-class weapon");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Weapon_Type_Mace,     "Mordecai.Weapon.Type.Mace",     "Character is wielding a mace-class weapon");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Weapon_Type_Spear,    "Mordecai.Weapon.Type.Spear",    "Character is wielding a spear-class weapon");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Weapon_Type_Staff,    "Mordecai.Weapon.Type.Staff",    "Character is wielding a staff-class weapon");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Weapon_Type_Dagger,   "Mordecai.Weapon.Type.Dagger",   "Character is wielding a dagger-class weapon");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Weapon_Type_Bow,      "Mordecai.Weapon.Type.Bow",      "Character is wielding a bow-class weapon");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Weapon_Type_Crossbow, "Mordecai.Weapon.Type.Crossbow", "Character is wielding a crossbow-class weapon");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Weapon_Type_Wand,     "Mordecai.Weapon.Type.Wand",     "Character is wielding a wand-class weapon");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Weapon_Type_Unarmed,  "Mordecai.Weapon.Type.Unarmed",  "Character is fighting unarmed");

	// Weapon slot tags (US-024)
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Weapon_Slot_MainHand, "Mordecai.Weapon.Slot.MainHand", "Weapon equipped in main hand slot");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Weapon_Slot_OffHand,  "Mordecai.Weapon.Slot.OffHand",  "Weapon equipped in off-hand slot");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Weapon_Slot_TwoHand,  "Mordecai.Weapon.Slot.TwoHand",  "Two-handed weapon equipped — both hands occupied");

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
