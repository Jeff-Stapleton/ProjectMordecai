# SkillsTab

The Skills tab of the pause menu: the skill-point badge, a category rail, ranked skill rows with 20-stroke tally bars and Allocate buttons, and the selected skill's milestones. Use it as the content of the `skills` tab inside the PauseShell's 948 x 540 content box.

## Anatomy
Positions are design px at 1920 x 1080 (content box at x 486-1434, y 298-838). States not on screen are in the spec band under the screen.

| Part | Box | Build |
|---|---|---|
| Header | 948 x 52 at (486, 298) | Left: the ONE skill-point badge (`.tt-skills__points`, also on CharacterTab): a 48 px `good` coin (2 px `ink` edge, an `ink` ring 3 px inside) holding the `skill-point` glyph in `ink`, then "Skill Points" (`ui-label`, `text-muted`) over the count (`num-l`, `good`, a 2-digit box). Right: "Weapon · 13 skills" in `ui-label` (`text` for the category, `text-muted` for the count). |
| Category rail | 198 wide at x 486, tags 68 tall at y 364 / 442 / 520 / 598 / 676 | Tag shape (radius 4 / 10 / 10 / 4) with a grommet at x 10 (12 px `well` hole), glyph 24, label `ui-label` on up to two lines (label column 124 px). Idle: `plate-raised`, 2 px `lip`, `text-muted`; grommet rings `plate` 3 px + `lip` 2 px. Selected: the cream `enamel` fill (core `.is-selected`, D6), 2 px `ink`, `ink` label at 700, a 4 px `enamel-shade` foot, the core grommet (`enamel-shade` 3 px + `ink` 2 px). Hover: `rim` edge, `text` label. |
| Skill list (ScrollBox) | 454 x 474 at (698, 364) | Rows 100 tall, gap 8 (row tops y 364 / 472 / 580 / 688 / 796; the box cuts the fifth row, and the scroll track says more follow). The ScrollBox reaches 8 px into the gutters so a focus ring (6 px out, 7 in High Contrast) is never clipped. |
| Row line 1 (32) | | Name `ui-title` `text`; right: "Rank" `ui-label` `text-muted` + "12 / 20" `num-m` `text` in a 106 px box. Longest weapon name: "Two-Weapon Fighting" (264 px) still fits beside the rank. |
| Row line 2 (48) | | The 20-stroke tally bar at Tally's skill-row size (24 tall, stroke 3, pitch 7: 148 px; four bundles, slashes on 5 / 10 / 15 / 20, a cap dot on rank 1), the reason text right-aligned (`ui-label` in a 117 px box: "Not learned" is 108, "Max rank" 84), the Allocate button (149 x 48). |
| Allocate | 149 x 48 | Button's small row action (`.tt-btn.tt-btn--sm`): `plate-raised` face, 2 px `lip` edge, 4 px `ink` print, the `skill-point` glyph (20) + "Allocate" (Next 700, 22 px). Locked: the core disabled look (`plate`, dashed `lip`, `text-disabled`, no print) with the `lock` glyph. |
| Scroll track | 8 x 474 at x 1157 | `well` with a 1 px `lip` ring; thumb `rim` with a 2 px `ink` border, height = visible / total. |
| Detail panel | 268 x 474 at (1166, 364) | `plate-raised`, 2 px `lip`, radius 10, padding 12 / 14. Name `ui-title`, "Rank" + "12 / 20" `num-m`, a core `.tt-divider`, "Milestones" (`ui-label` caps `text-muted`), five milestone rows (stamp 40 px + description `ui-label`, gap 2) on a 2 px `lip` track; the list ends about 20 px above the panel's foot. |

Lit tally strokes are `text` on plate and `ink` on the cream selected row; Rank 20 is a full bundle in `text`, never `good` (D1: a gold full bundle means rarity Gold only). Unlit strokes are core's dotted strokes (`lip`; `ink-muted` on cream through `.tt-tally--enamel`), so lit versus unlit is a shape, not a colour (in High Contrast both are white).

## States
- **Row:**
  - idle: transparent, a 2 px `rim` 30% hairline at its foot;
  - selected (drives the detail panel): the cream `enamel` fill with a 2 px `ink` edge (core `.is-selected`, D6); name, rank and lit strokes turn `ink`, the "Rank" label and reason `ink-muted`;
  - focused: core focus with `.tt-focus--fill-gap` (the ring outside the row, an `ink` ring in the gap), on top of selected; gamepad A or keyboard Enter presses its Allocate;
  - unlearned (Rank 0): name in `text-muted`, reason "Not learned", Allocate enabled (learning costs 1 point);
  - max (Rank 20): a full bundle in `text`, reason "Max rank", Allocate locked.
- **Allocate (spec band):** idle; hover (lifts 2 px, print 6, the edge doubles to 4 px `rim`); pressed (sinks 3 px, print 1, `plate` face); focused (core button ring: a 2 px `ink` gap, the `focus` ring, the keyline; the print stays); locked (dashed `lip`, `lock` glyph, `text-disabled`, no print). Locked when there are 0 skill points, the skill is at Rank 20, or the skill is unregistered. The label reads "Allocate" in every state.
- **0 skill points:** the badge goes dark (coin `plate` with `lip` rings, the count `text-disabled`) and the header says "No skill points" once, beside it; every row's Allocate locks without repeating the reason.
- **Milestone stamp:** reached (`enamel-shade` face, a 3 px `enamel` inner ring, 2 px `ink` rim, `rim` halo, `ink` number); next (`plate` disc, solid `rim` ring, `text` number, plus "3 ranks to go" in `good`, singular "1 rank to go"); locked (dashed `lip` ring, `text-disabled` number and description, a `lock` glyph before the text). Light face versus dark face versus dashed ring: never colour alone.
- **Allocate feedback:** the button runs KeyPress; the new stroke lights at once; a milestone rank flips its stamp to reached with StampReveal; the rank-up is filed to the spike (the HUD is under the veil) and the Technique plate prints after the menu closes. Reduce Motion: the brand book's fades, values swap. TODO(DECISION): a stroke "scratch-in" animation would need its own row in the brand book Motion table; none is drawn until then.
- **Category:** selected / idle / hover; an empty category shows the one empty state (core `.tt-empty`, D8): "No skills in this category yet." (proposed copy; no skill data assets exist today).

## Data and implementation
- Widget: `UMordecaiSkillTreeWidget` (API only, not registered: N5, so the tab shows "Coming Soon" today). Register it for `skills` with `RegisterTab`; US-093 builds the tree (AC-093.2, AC-093.4).
- Rows from `FMordecaiSkillTreeRow`: `SkillName`, `DisplayName`, `CurrentRank`, `MaxRank` (20), `Category`, `bCanAllocate`. Milestones from `FMordecaiSkillTreeMilestoneRow`: `Rank` (1 / 5 / 10 / 15 / 20), `Description`, `bReached`, `bAbilityGranted`, `bHasDescription`; fallback description "No data available.". Allocation: `UMordecaiSkillComponent::AvailableSkillPoints`, `CanAllocateSkillPoint`, `RequestAllocateSkillPoint`. Rebind in `NativeOnActivated`.
- GAS: a milestone applies the tag `Mordecai.Skill.<SkillName>.Rank<N>`, fires `Mordecai.Event.SkillMilestone` and grants the milestone ability; listen for the event to flip the stamp live.
- Categories (`EMordecaiSkillCategory` to label): Weapon, DefenseArmor "Defense & Armor", MovementExploration "Movement & Exploration", UtilityInteraction "Utility & Interaction", Magic. Glyphs: `weapon`, `armor`, `map`, `chest`, `magical`. v1.1 counts: 13 / 8 / 6 / 4 / 11.
- Weapon skills in sheet order: Longswords, Greatswords, Shortswords, Daggers, Axes, Maces, Spears, Quarterstaff, Unarmed, Longbow, Shortbow, Crossbows, Two-Weapon Fighting.
- Playtests: the rank TextBlock reads "12 / 20" (checks look for "/ 20"); "Rank" is a separate label TextBlock. The Allocate button is Button's proposed `UMordecaiButton` in the small style (`BS_TT_Small`); its first TextBlock child reads "Allocate" in every state (the glyph and the lock are UImages), so `click_button("Allocate")` keeps working.
- Proposed `BindWidgetOptional` names: `SkillPointsText`, `CategoryRail`, `SkillListBox` (ScrollBox), `SkillDetailName`, `SkillDetailRank`, `MilestoneList`; per row `NameText`, `RankText`, `RankBar`, `ReasonText`, `AllocateButton`.
- The tally bar is `UMordecaiTallyWidget` (03's Tally atlas: `Count`, `Max`) at the skill-row size; tint the filled layer `text`, or `ink` while the row is selected.
- Ranks are bought with skill points (1 point per rank, 0 to 1 included). Skills levelling from use does not exist. TODO(DECISION): use-based skill growth.
- Milestone texts in the preview are condensed from `skill_sheet_v1.1` for Longswords (example copy); data supplies the real description, and `{x} * {rank}` placeholders must be resolved before display.
- UMG sizes: `ui-title` 19.5 pt, `num-l` 22.5 pt, `num-m` 18 pt, `ui-label` 15 pt, button label 16.5 pt.
- Tokens: `plate`, `plate-raised`, `well`, `lip`, `rim`, `ink`, `ink-muted`, `enamel`, `enamel-shade`, `text`, `text-muted`, `text-disabled`, `good`, `focus`.

## Copy
- "Skill Points" + count; "Rank" + "x / 20"; "Allocate" (verb, every state).
- Reasons: "No skill points" (header, once), "Max rank", "Not learned".
- "Milestones"; "3 ranks to go" / "1 rank to go"; "No data available." (fallback, exact).
- Header meta: "{Category} · {N} skills".
- Empty category (proposed): "No skills in this category yet."
- No Tally remarks on this screen; the Technique plate carries the celebration.

## Do / Don't
- Do keep "Allocate" as the label in every state and explain a locked state with plain text.
- Do show a row-specific reason once, beside its locked button; show the 0-points reason once, in the header.
- Do keep the ScrollBox cut through a row so more rows are obviously below.
- Do use the core dotted empty strokes and Tally's skill-row size; don't restyle strokes per component.
- Don't hide rows at Rank 0; they are skills the player can learn.
- Don't colour the tally bar by category or turn Rank 20 gold; `good` on strokes means rarity Gold only.
- Don't let the detail text push the panel past 474 px; clamp descriptions to three lines and scroll the panel if data runs longer.
