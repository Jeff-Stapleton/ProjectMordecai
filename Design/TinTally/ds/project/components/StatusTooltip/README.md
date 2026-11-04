# StatusTooltip

The readout behind one status chip: its glyph, display name, polarity and category, tagline, time left, stack pips and, when the status spec gives one, its counterplay line. Use it in the pause menu when a StatusChip takes hover or focus; it is where the 16 mandated taglines finally appear, and it never shows on the HUD during play.

## Anatomy

A 380 px card, height from its content (about 250-330 px), padding 14 / 18 / 18 / 14, 10 px between parts. It carries StatusChip's polarity look at card size, so a player who knows the chip knows the card:

| Polarity | Ground and text | Edge | Mark |
|---|---|---|---|
| Debuff | `plate`, `text` / `text-muted` | 2 px `debuff` rim, radius 12 12 4 4, 3 px `ink` outline | a pinked hem along the bottom, 8 px in from each side: `debuff` teeth hanging 8 px, `ink` teeth 3 px proud of them (12 px pitch) |
| Buff | `enamel`, `ink` / `ink-muted` | 2 px `buff` rim, radius 12, 3 px `ink` outline | a raised 44 x 12 `buff` tab over the glyph well, with a 3 px `ink` border on its top and sides |

No print shadow (it floats like the chip it hangs from), no rivets, no tally strokes, no Tally remarks: status lines are World voice.

Top to bottom:
1. **Head.** A 48 px glyph well (`well` + 2 px `lip` on a debuff; `enamel-shade` + 2 px `ink` on a buff) holding the 32 px status glyph, tinted like the chip's glyph (Burning `dmg-fire`, Frostbitten `dmg-frost`, Shocked `dmg-lightning`; buffs `ink`). Beside it the display name (`ui-title` 26/32) over the kind line (`ui-label`): polarity, then the category when it differs ("Debuff · Damage over time", "Debuff · Control", "Debuff", "Buff").
2. **Tagline** (`ui-body` 22/30), verbatim, one line. Omitted when the status has none (Stone Skin, Fire Ward and the other spell buffs).
3. **Facts**, a label / value grid (`ui-label`; numbers `num-s`):
   - "Time left": "4.2 of 5.0 s" (time left of the full duration); "Until you are hit" for Focused.
   - "Stacks", only for stacking statuses: the stack pips (core `.tt-pips`, 14 px; reached = filled disc in the status colour, ahead = hollow ring; `.tt-pips--enamel` on a buff), then "2 of 4". Frostbitten adds "· Frozen at the 5th".
4. **Counterplay**, only when the status spec gives one: a `.tt-divider` (`--enamel` on a buff), the label "Counterplay" (UI 700 20, `text` / `ink`), then the spec's line (UI 600 20/26, `text-muted` / `ink-muted`), wrapping to at most 3 lines in 344 px.

**Hanging.** In the menu the card hangs on the top layer under its chip: a 20 px light string (core `.tt-string--light`) from just below the chip's hem (7 px, clear of a debuff chip's teeth), under the chip's glyph, down to the card's top edge. It overlays whatever is below and never moves the layout. If it would cross the bottom of the screen it hangs above the chip instead (string up), and it never covers its own chip.

## States

| State | Look |
|---|---|
| Shown | the chip takes keyboard / gamepad focus (the chip shows its focus ring) or mouse hover: the card appears at once (no drop in a menu; Reduce Motion is the same) |
| Live | the time left keeps counting down only if the game is not paused; in the pause menu it is frozen at the pause value |
| Stacks change | not while paused; out of a pause the card is gone |
| Hidden | focus or hover moves away, or the status ends |

## Data and implementation

- **Where it appears.** Behind a focusable StatusChip in the pause menu (`UI.Layer.Menu`). Its one host is CharacterTab's "Active statuses" row (proposed; no status row exists in the pause menu today): 600 x 46 at (486, 388), under the Character tab's header strip, the label "Statuses" then StatusChip's chips, each in the tab's focus order before the attribute cards (`ActiveStatusBox`, see CharacterTab's Data). CharacterTab draws it with the example moment's two statuses, Frostbitten at 2 of 4 and Focused; the first stage above reuses that row's markup. The HUD chips (`StatusEffectBar`, a child of the HUD root) never take focus during play.
- **Codex:** a screen that lists every status with its card (tags, taglines, counterplay) is `TODO(DECISION)`: where it lives (a Codex tab in the pause shell, or a section of the Feats or Character tab) and whether it shows statuses the player has not met.
- **Widget (proposed):** `UMordecaiStatusTooltipWidget`, used as the chip's CommonUI tooltip / a focus-driven popup owned by the menu tab, with `TooltipGlyph` (UImage), `TooltipNameText`, `TooltipKindText`, `TaglineText`, `TimeLeftText`, `StackPips` (UHorizontalBox of pip Images), `CounterplayText`. Every text is a real UTextBlock and the class starts with "Mordecai", so the bridge can read it.
- **Live data (GAS):**
  - the status tag (`Mordecai.Status.<Name>`) from the player's ASC;
  - the category from `GetStatusEffectCategory()` (`EMordecaiStatusEffectCategory`: DoT, Debuff, Control, Buff, Environmental; MordecaiStatusEffectTypes.cpp); polarity from the chip's `BuffTags`;
  - time left and full duration from the active GE (`GetActiveEffectRemainingAndDuration`); none for Focused (it lasts until the player is hit);
  - the stack count from the GE; the maximum from the status (Frostbitten 4 shown before Frozen, Shocked 3).
- **Static text (proposed data, `TODO(DECISION)`):** no status DataAsset exists. Proposal: `UMordecaiStatusDisplayDataAsset` (one per status, or one table keyed by the status tag) with `DisplayName` ("Stone Skin"), `Tagline`, `CounterplayText` (optional) and `Icon`. StatusChip's `StatusNameText` reads the same `DisplayName` once US-091 lands; until then it shows the tag segment ("StoneSkin").
- **Category words** ("Damage over time", "Control", "Environmental") are new player-facing copy for the enum, proposed here.
- **Counterplay copy** is the "Skillful Counterplay" column of `status_effects_launch_set_v1.md`, verbatim. It reads like design notes ("keep stamina high tiers"); a player-facing rewrite is `TODO(DECISION)`. Bleeding's line contains a placeholder ("Avoid being hit for X sec") and must not ship until the number exists.
- **Tokens:** `plate`, `enamel`, `enamel-shade`, `ink`, `ink-muted`, `text`, `text-muted`, `well`, `lip`, `rim` (light string, divider), `buff`, `debuff`, `dmg-fire`, `dmg-frost`, `dmg-lightning`.
- **Example data in the preview:** the times and stack counts (Frostbitten 2 of 4 and Focused match CharacterTab and HUDLayout's fight frame); the "Active statuses" row is CharacterTab's proposal.

## Copy

- Labels: "Time left", "Stacks", "Counterplay"; values "4.2 of 5.0 s", "2 of 4", "Until you are hit", "Frozen at the 5th".
- Kind line: "Debuff · Damage over time", "Debuff · Control", "Debuff · Environmental", "Debuff", "Buff".
- Taglines, verbatim (BUILD.md): Burning "You're on a clock", Bleeding "Recovery is denied", Poisoned "You can't catch your breath", Frostbitten "Sluggish body, slower reactions", Shocked "Timing disruption", Weakened "You can't break them", Brittle "You break easily", Silenced "Magic is shut off", Rooted "Position solved or punished", Blinded "Reads become unreliable", Fear "Composure collapse", Cursed "Resource rules broken", Exposed "Next mistake hurts more", Corroded "Gear advantage denied", Drenched "Elemental rules shift", Focused "Mastery streak reward".
- Counterplay lines shown: Burning "Break line with fire source; roll through water/dust to douse; perfect dodge reduces tick damage"; Frostbitten "Perfect dodge breaks frost stacks; heat sources thaw; keep stamina high tiers"; Shocked "Don’t eat chains; perfect block grounds and clears one stack; reposition"; Focused "Maintain spacing; don’t get greedy; reset fights if lost".
- No Tally remarks, ever.

## Do / Don't

- Do omit a line the data does not have (tagline, stacks, counterplay); never invent one.
- Do keep the chip's polarity look: tab on top for a buff, teeth below for a debuff.
- Do draw stacks as pips, never tally strokes.
- Don't show tooltips on the HUD in play, and don't let one cover its own chip.
- Don't reword a tagline; restyle only.
