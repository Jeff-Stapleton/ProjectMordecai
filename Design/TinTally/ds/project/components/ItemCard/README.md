# ItemCard

The enamel item tag: a rarity band carrying the colour word, the item name and type, a printed stat strip with compare deltas, the description and action keycaps. Use it for item tooltips, the Inventory tab's detail column, compare pairs and the service-screen trays (Mage Tower, Alchemist); SpellCard is its sibling for spells and reuses this construction.

## Anatomy
- **Card**: 384 wide (compact 320), `enamel`, `radius-tag`, 3 px `ink` outline, 6 px print shadow.
- **Rarity band**, 48 tall:
  - Fill is the rarity token, with a 3 px ink rule under it.
  - The eyelet at the left is the core tag grommet: a 10 px `well` hole, a 3 px `enamel-shade` ring and a 2 px `ink` ring. In High Contrast the ring turns white and the hole and the outer ring stay black, so it still reads as a hole.
  - The rarity badge in `ink`: Common is an empty ring, Green 1 stroke, Blue 2, Purple 3, Red 4, Gold the full bundle.
  - The text is "PURPLE · Amulet": the rarity word cased upper (TextTransformPolicy ToUpper on "Purple"), then the subtype or the type string. Next 800, 20 px, `ink`.
- **Body** (padding 14 / 18 / 18, 10 px gap; compact 12 / 16 / 14, 8 px gap):
  - The name in `ui-title`, `ink`.
  - The type line in `ui-label`, `ink-muted` ("Magical Item · Sword · Main hand").
  - Unidentified items add a 44 px "?" stamp (ink ring on `enamel-shade`, rotated -8°) left of the name.
- **Sealed note** (unidentified, full card only): a dashed `ink-muted` box on `enamel-shade` with the `mage-tower` glyph and "Unidentified — the Mage Tower can read this." The compact card drops it (see States).
- **Stat strip**: a full-bleed `enamel-shade` band between two dashed rules.
  - A three-column grid: label (`ui-label`, `ink-muted`), value (`num-s` 700, `ink`, right-aligned), delta.
  - Sealed variant: one row "Properties ???" (`num-m`) over a 12% ink hatch.
- **Delta pill**: 28 tall, 2 px ink border, `num-s` 700 in ink.
  - Up: `delta-up` fill (an alias of `heal`), the `arrow-up` glyph and the sign ("+9").
  - Down: `delta-down` fill (an alias of `danger`, ink holds 6.0:1), `arrow-down`, "−4".
  - Same: `enamel` fill and "=".
  - Compact card: a 28 px dot with the glyph only; the number moves to the accessible name.
- **Effects** (`ui-label`): a glyph plus one line each, e.g. the burning glyph with "On hit: 25% chance to build Burning (2.0 s cooldown)" and the fire glyph with "Grants Flame Arc".
- **Description** in `ui-body`, `ink`.
- **Action footer**: `enamel-shade` with a 2 px rule; the keycap leads its verb ("[Enter] Identify", D4).
- **Pinned (equipped)**: a 40 px enamel pin token driven through the eyelet (the grommet hides under it; the band text starts at 58 px to clear the token), and the **Equipped pill** across the band's top edge, 16 px from the right.
- **Equipped pill**: one piece wherever an item is equipped (this card and the Inventory row): the core chip (`.tt-chip`: `plate`, 36 tall, `ink` outline) with a 2 px `rim` edge, the `pin` glyph (20) and "Equipped" in `ui-label`, `text`. Never bracket text.
- **Hung (tooltip)**: the core light string (read through `--tt-string-light-*`: a 4 px `rim` cord with 1 px `ink` sides) from the selected row's foot into the eyelet, across a 34 px gap (58 px from the eyelet's centre). The eyelet sits under the row's badge.

## States
- **Identified**: Emberfang (Purple magical sword), with stats, effects and deltas.
- **Unidentified, partial info on**: "Unidentified Purple Amulet". Stamp, sealed note, "Properties ???", the ShortDescription.
- **Unidentified, partial info off**: "Unknown Item". Only the rarity and type tags show, and there is no description.
- **Stacked consumable**: Health Potion, "Carried x3", stack limit in the type line.
- **Equipped (pinned)** and **candidate**: the compare pair.
- **Compact**: the Inventory tab's right column, 320 wide, no action footer, dot deltas. An unidentified item keeps the stamp, "Properties ???" and the ShortDescription and drops the sealed note, because the row's Identify keycap carries the action (shown in the InventoryTab card).
- **Hung tooltip**: hangs from the selected, focused Inventory row (InventoryTab's row: the cream Selected fill, D6, with the focus ring).
- Cards are never focusable; focus stays on the row or slot that opened them.

## Data and implementation
- **Widget**: new `UMordecaiItemCardWidget` (UserWidget) with an MVVM view model; `ue5_tech_stack_context.md` names MVVM for "item tooltips & compare panels".
  - Named parts: `RarityBand`, `RarityBandText`, `RarityBadge`, `ItemNameText`, `ItemTypeText`, `UnidentifiedNote`, `StatList`, `EffectList`, `DescriptionText`, `ActionBar`.
  - Keep every string a real TextBlock; the bridge reads text, not pixels.
  - The card is 03's **Tag stack** at card size (RoundedBox print, outline and face; the grommet as three stacked RoundedBox Images). The band is a Border with per-corner radii (9 / 9 / 0 / 0) over a 3 px `ink` Image. The pin token, the Equipped pill (03's Pill stack) and the string (03's Strings, the light cord) are separate widgets; the string is never a child of the card, so it stays plumb.
- **Display rules** (`UMordecaiItemLibrary`):
  - Identified: DisplayName, Description, all tags.
  - Unidentified with ShowPartialInfoBeforeIdentify: "Unidentified <Rarity> <Subtype or type string>", the ShortDescription, rarity and type tags only.
  - Without it: "Unknown Item", an empty description.
  - Type strings come from `ItemTypeDisplayString` ("Magical Item", "Upgrade Key", "Resource" …).
- **Stats**
  - From `UMordecaiWeaponDataAsset`: BaseDamage, AttackSpeedMultiplier, PostureDamageBonus.
  - From StatModifiers (item schema, not in code yet): e.g. CritChance, FireDamageScalar.
  - Deltas compare against the main hand in `UMordecaiEquipmentComponent`.
  - The labels "Damage", "Attack speed", "Posture damage", "Crit chance" and "Fire damage" are proposed copy.
- **Example values** (labelled example, not data):
  - Rusty Sword: Damage 9, Attack speed ×1.0, Posture damage +10.
  - Emberfang: Damage 18, ×1.0, +6, Crit chance 5.0%, Fire damage 12.0% (the schema's WeaponDamage Add 18, CritChance Add 0.05, FireDamageScalar Add 0.12).
  - Emberfang's proc `emberfang_burn_proc` (OnHit, chance 0.25, internal cooldown 2.0) is shown as the Burning line; `emberfang_flame_arc` is shown as "Flame Arc".
  - The Upgrade Key description is example copy drawn from the village spec ("Rare progression unlockers used to break upgrade caps or unlock special structures"); "not wearable" follows game_design_v2.
- **Actions**
  - "Identify" exists (the Inventory row and the Mage Tower).
  - "Equip" and "Use" are TODO(DECISION) N22: no equip or use action exists. The preview draws those footers so the layout is ready; ship them only when the actions land.
  - Keycaps are device-aware through CommonInput: keyboard Enter (CommonUI accept), gamepad A. The keycap leads its verb. A preview draws one device (keyboard, D5).
- **Placement**
  - The tooltip hangs below the focused row, its eyelet under the row's badge, across a 34 px gap on the light string. It flips above the row when it would cross y 1048.
  - Compare pair: the equipped card on the left (pinned), the candidate on the right, 32 px apart.
- **Width**: 384 is the narrowest card where "Posture damage", a 5-character value and the widest delta ("+12.0%") share one line. In the 320 px column use the compact dot deltas.
- **Tokens**:
  - Colour: `enamel`, `enamel-shade`, `ink`, `ink-muted`, `well`, `rim`, `plate`, `text`, `rarity-*`, `delta-up`, `delta-down` (never `stamina-green`: compare never borrows a stamina tier's meaning).
  - Type: `ui-title`, `ui-label`, `ui-body`, `num-s`, `num-m`.
  - Radius: `radius-tag`, `radius-pill`.
- **Contrast**: ink on `enamel` 16.8:1 and on `enamel-shade` 14:1; `ink-muted` on `enamel-shade` 5.7:1; ink on `delta-down` 6.0:1; every rarity band carries ink text.

## Copy
- Band and names: "PURPLE · Amulet", "Unidentified Purple Amulet", "Unknown Item", type line "Magical Item · Amulet".
- Proposed (not shipping yet): "Unidentified — the Mage Tower can read this.", "Properties ???", "Carried x3", "Equipped", the stat labels.
- Shipping: "Identify".
- Descriptions, verbatim from data:
  - "An amulet of unknown power." (the amulet's ShortDescription)
  - "Restores a chunk of health. The Alchemist limits how many you can field."
  - "A worn blade. Barely holds an edge, but it is honest work."
  - "A named sword that kindles fire with every strike." (Emberfang, item schema example)
- No Tally remarks on cards.

## Do / Don't
- Do keep ink text on enamel and on the band. Rarity colour appears only as the band fill and the badge.
- Do print the sign and the arrow glyph on every delta; the colour only confirms.
- Do mark an equipped item with the Equipped pill, never "[equipped]" bracket text.
- Do set every stat value in Mono so compare columns line up.
- Don't use font arrows or triangles; use the `arrow-up` and `arrow-down` glyphs.
- Don't show stats for an unidentified item; show "???".
- Don't ship Equip or Use footers until N22 is decided.
