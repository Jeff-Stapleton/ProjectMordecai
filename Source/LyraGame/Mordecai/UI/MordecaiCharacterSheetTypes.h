// Project Mordecai — Character Sheet Types (US-066)
// Pure C++ data types for the character sheet widget.
// Testable without a running world or widget rendering.

#pragma once

#include "CoreMinimal.h"

/**
 * Primary attribute index — matches the order of the character_attributes_v1 primary_stats table.
 * Keep in sync with FMordecaiCharacterSheetData::PrimaryAttributes.
 */
enum class EMordecaiPrimaryAttributeIndex : uint8
{
	Strength     = 0,
	Dexterity    = 1,
	Endurance    = 2,
	Constitution = 3,
	Resistance   = 4,
	Discipline   = 5,
	Intelligence = 6,
	Wisdom       = 7,
	Charisma     = 8,
	Count        = 9
};

/**
 * Visual grouping for primary attributes (per game_design_v2 Section 5 flavor).
 */
enum class EMordecaiAttributeGroup : uint8
{
	Physical,   // STR, DEX, END
	Resilience, // CON, RES, DIS
	Magical     // INT, WIS, CHA
};

/**
 * One row in the primary attribute display. Shows name, raw base value, and
 * the effective modifier contribution (as a percentage bonus).
 */
struct FMordecaiPrimaryAttributeDisplay
{
	/** Attribute short code ("STR", "DEX", ...). */
	FName Code;

	/** Localized display name shown in UI. */
	FText DisplayName;

	/** Raw base value (e.g. 10 at character creation). */
	float BaseValue = 0.f;

	/** Effective Mod (diminishing-returns) derived from BaseValue. */
	float EffectiveMod = 0.f;

	/** True once a live value has been pulled from an ASC; false before binding. */
	bool bHasValue = false;
};

/**
 * One row in the derived stats display. MultiplierValue is a GAS attribute that
 * defaults to 1.0 meaning "no bonus"; the display formula is (V-1)*100%.
 */
struct FMordecaiDerivedStatDisplay
{
	FText DisplayName;
	float MultiplierValue = 1.f;
	bool bHasValue = false;
};

/**
 * One row in the core resources display (Health, Stamina, Spell Points, Posture).
 */
struct FMordecaiResourceDisplay
{
	FName Id;
	FText DisplayName;
	float Current = 0.f;
	float Max = 0.f;
	bool bHasValue = false;
};

/**
 * Pure data model for the Character Sheet widget. InitDefaults() populates
 * names/labels and leaves bHasValue=false on every row so the widget displays
 * placeholder ("--") until BindToASC() provides real values.
 *
 * No UObject ownership, no delegates — testable in isolation.
 */
struct FMordecaiCharacterSheetData
{
	TArray<FMordecaiPrimaryAttributeDisplay> PrimaryAttributes;    // 9 rows
	TArray<FMordecaiDerivedStatDisplay>      PrimaryDerivedStats;  // 5 rows
	TArray<FMordecaiDerivedStatDisplay>      SecondaryDerivedStats;// 9 rows
	TArray<FMordecaiResourceDisplay>         CoreResources;        // 4 rows

	FText PhysicalGroupLabel;
	FText ResilienceGroupLabel;
	FText MagicalGroupLabel;

	/** True if this data is currently backed by a live ASC. */
	bool bIsBound = false;

	/** Populate default names, group labels, and zero values. */
	void InitDefaults();

	/** Return the visual group for a primary attribute index (0..8). */
	static EMordecaiAttributeGroup GetGroupForAttribute(int32 PrimaryIndex);

	/** Placeholder used when no live value is available. */
	static const TCHAR* PlaceholderText() { return TEXT("--"); }

	/** Format a derived multiplier as a signed percent bonus string, e.g. "+15.0%". */
	static FString FormatPercentBonus(float MultiplierValue);

	/** Format the effective mod contribution of a primary attribute, e.g. "+9.4%". */
	static FString FormatEffectiveModBonus(float EffectiveMod);

	/** Format a resource pool as "Current / Max", using zero-decimal rounded values. */
	static FString FormatResource(float Current, float Max);
};
