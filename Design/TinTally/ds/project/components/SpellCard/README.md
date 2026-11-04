# SpellCard

The spell detail card: an enamel tag with a Spell Point band, the spell glyph in SpellSlot's well, cooldown, cast time, targeting, movement and damage type, the effect lines and a description; ItemCard's sibling for spells. Use it wherever the game explains a spell in a menu; where that is (a focused spell slot in a pause-menu loadout, or a Spells tab) is TODO(DECISION), and it never shows on the HUD during play.

## Anatomy
- **Card**: ItemCard's construction, unchanged: 384 wide (compact 320), `enamel`, `radius-tag`, 3 px `ink` outline, 6 px print. Markup: `.tt-itemcard.tt-spellcard`; this card only adds the parts below.
- **Band**, 48 tall: the `spell-points` fill (ink text 7:1 and up in both themes) with a 3 px `ink` rule under it, the core tag grommet at the left, then the **SP cost as pips** (core `.tt-pips--enamel`: 14 px `ink` discs, one per Spell Point; shape carries the count) and "SPELL · 3 SP" (Next 800, 20 px, `ink`; "Spell" cased by ToUpper). The band plays the part of ItemCard's rarity band: spells have no rarity.
- **Head**: SpellSlot's 64 x 64 glyph well (`well`, 2 px `lip`, radius 8, D13) holding the 48 px spell glyph in its HUD colour (SpellSlot's icon classes: Fireball `dmg-fire`, Restoration `heal`, others `text`), so card and slot show the same picture; then the name in `ui-title`, `ink`, over the role line in `ui-label`, `ink-muted` ("Spell · Fire damage").
- **Stat strip** (ItemCard's strip: a full-bleed `enamel-shade` band between dashed rules), two columns, label in `ui-label` `ink-muted`, value right-aligned:
  - Cooldown, `num-s` ("2.0 s");
  - Cast time, `num-s` ("0.5 s"), or "Instant";
  - Targeting and range, `ui-label` 700 ("Projectile, 15 m", "Dash, 5 m", "Self");
  - While casting, the movement policy ("Slowed", "Free", "Rooted");
  - Base power, `num-s`, when the spell has one;
  - Damage: a core pill in the damage type's colour with `ink` text and the type glyph leading ("Fire" on `dmg-fire`); a heal spell shows a "Heal" pill on `heal` instead. Physical would carry no glyph (D15); no shipping spell deals it;
  - Slot: the keycap that casts it (`.tt-key--sm`).
- **Effect lines** (ItemCard's): a 20 px `ink` glyph and one line each ("Fire damage on hit. It can crit.", "Untouchable for 0.3 s.").
- **Description** in `ui-body`, `ink`.
- No action footer: a spell card explains, it does not act.

## States
- **Full** (384) for the four shipping spells, and **compact** (320) for a narrow menu column (the strip keeps every row).
- **Hung** (proposal): ItemCard's hung-tooltip build, the light string from the focused slot's foot into the card's eyelet, the eyelet under the slot's centre. The spec card shows it under a loadout row of the four SpellSlots with the Fireball slot focused.
- Cards are never focusable; focus stays on the slot or row that opened them.
- Rows a spell does not have are left out (Blink has no Base power and no Damage), never shown empty.
- TODO(DECISION): whether the card shows live state (cooling, unaffordable, Silenced). Today it shows the spell, not the moment; SpellSlot shows the moment.

## Data and implementation
- **Widget**: new `UMordecaiSpellCardWidget` (UserWidget, MVVM like ItemCard), bound to a `UMordecaiSpellDataAsset`. Proposed named parts: `SpellBand`, `CostPips`, `SpellBandText`, `SpellIcon`, `SpellNameText`, `SpellRoleText`, `StatList`, `EffectList`, `DescriptionText`. Every string is a real TextBlock.
- **Fields** (`UMordecaiSpellDataAsset`, values from `Scripts/us057_01_create_spell_data_assets.py`):
  - `DisplayName` → name; `SpellPointCost` → pips and "N SP"; `CooldownDuration` → "%.1f s".
  - Cast time = `WindupTime` + `CastTime`: when the spell fires (`UMordecaiGA_SpellBase` fires `OnSpellCast` after both phases); 0 reads "Instant". Recovery is not shown.
  - `TargetingType` + `Range` (uu / 100, in m) → "Projectile, 15 m"; Self has no range.
  - `MovementPolicy` → FreeMove "Free", SlowWhileCasting "Slowed", RootWhileCasting "Rooted".
  - `BasePower` → "Base power"; the scaled SpellPower (attributes, rank, upcast) is not shown. TODO(DECISION): show the scaled value instead.
  - Damage type: from the ability (`UMordecaiGA_Fireball::GetSpellDamageType()` = Fire); heal spells show Heal.
  - Effect lines: from `BuffDuration` and the ability (Blink's 0.3 s `State.Invulnerable`, Stone Skin's 10 s flat `DamageReduction`, Restoration's heal over 5 s).
  - Slot keycap: from the loadout binding (`MordecaiHeroComponent`): Q, E, R, 1; gamepad LB, RB, none, D-pad Up.
- **Missing fields** (TODO(DECISION)): `UMordecaiSpellDataAsset` has no `Description`, `Icon` or role line. ability_schema section 3 lists `Description` as required and `Icon` and `ShortDescription` as recommended; propose `Description` (FText), `Icon` (soft texture) and a role `AbilityType` (Spell, Mobility, Defensive...). Until they exist the card has no description and the role line falls back to "Spell".
- **Real data** (shipping DA_Spell_* values): Fireball 3 SP, 2.0 s, 0.2 + 0.3 s, Projectile 1500 uu, SlowWhileCasting, Base power 25, Fire; Blink 2 SP, 4.0 s, instant, Dash 500 uu, FreeMove, 0.3 s invulnerable; Stone Skin 4 SP, 15.0 s, 0.3 + 0.5 s, Self, RootWhileCasting, Base power 50, 10 s; Restoration 3 SP, 10.0 s, 0.2 + 0.4 s, Self, SlowWhileCasting, Base power 40, 5 s.
- **Example copy**: every description line, the role lines and the effect-line wording.
- **Assets** (03): the Tag stack at card size, the Pill stack for the damage pill, the Well for the glyph socket; glyphs `fireball`, `blink`, `stoneskin`, `restoration`, `fire`, `heal`, `immune`.
- **Tokens**: colour `enamel`, `enamel-shade`, `ink`, `ink-muted`, `spell-points`, `well`, `lip`, `text`, `dmg-fire`, `heal`; type `ui-title`, `ui-label`, `ui-body`, `num-s`; radius `radius-tag`, `radius-key`, `radius-pill`.

## Copy
- Shipping: the spell names "Fireball", "Blink", "Stone Skin", "Restoration".
- Proposed: "SPELL · 3 SP", "Cooldown", "Cast time", "Instant", "Targeting", "While casting", "Slowed", "Free", "Rooted", "Base power", "Damage", "Heals", "Slot", the role lines and the effect lines.
- Example: the descriptions ("Hurls a ball of fire where you aim.").
- Units as the game counts them: "2.0 s", "15 m", "3 SP". No Tally remarks on cards.

## Do / Don't
- Do build it from ItemCard's classes; a spell card is an item card with a Spell Point band.
- Do show the cost as pips and as "N SP"; never colour alone.
- Do lead the damage pill with the type glyph and print the word; the colour only confirms.
- Do keep every glyph and word on the enamel `ink`; colour lives in the band, the well and the pill.
- Don't use tally strokes for the cost or anything else on the card (D1).
- Don't draw gamepad keycaps beside keyboard ones; one device per screen (D5).
- Don't show the card on the HUD during play.
