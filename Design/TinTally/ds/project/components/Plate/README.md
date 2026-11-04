# Plate

The pressed-tin panel that every HUD cluster, menu and dialog sits on: a 100% opaque `plate` fill with an `ink` outline outside and a cream `rim` inside. Use the HUD plate for anything that sits on the world, the menu plate for the pause shell and dialogs, the inset plate for cards and panes inside a menu plate, and the well for anything sunk into a plate.

## Anatomy
- **Outline**: 3 px `ink` outside the body (4 px in High Contrast, automatic through `--tt-outline`). Its corner radius is 15, so it stays concentric with the body.
- **Rim**: 2 px `rim` (`stroke-rim`) inside the body on the top and left edges.
- **Rim shade**: 2 px `rim-shade` inside the body on the bottom and right edges. It makes the tin read as pressed. It is decoration and never the only edge.
- **Fill**: `plate`, always 100% opaque, so floor contrast can be computed.
- **Print**: a hard offset shadow, no blur. HUD plates use (0, 4) at `ink` 55% (`print-hud`); menu plates (`.tt-plate--menu`) use (0, 6).
- **Radius**: 12 (`radius-plate`).
- **Padding**: 16 (`space-4`) on the HUD and 24 (`space-5`) in menus. The spell slot is a 78 x 78 plate around a 64 x 64 well, 5 px inside the rim (SpellSlot's).
- **Header** (`.tt-plate__head`, menu plates only): the title on the left in `ui-title` and an optional count or action on the right, 12 px above the core `.tt-divider` (a 2 px `rim` rule at 50%), 16 px of space below it.
- **Rivets** (`.tt-plate--riveted`, optional): four 12 px pressed rivets, 8 px in from the rim's corners: a `rim-shade` face with a `rim` highlight inside a 2 px `ink` ring. Only on Tally plates (TallyPlate) and front-end menu plates (the title wordmark, Credits). Never on HUD plates (gauge, spell slots, rail head, loadout) or World-voice plates (banners, the boss bar, death, loading). SpellSlot's cost dots are Spell Point data, not rivets.
- **Depth ladder**, one step per level and never skipped:
  1. World.
  2. Plate: outline and print.
  3. Inset plate (`.tt-plate--inset`): `plate-raised` with a 2 px `lip` border. No outline and no print.
  4. Well (`.tt-plate__well`): `well` with a 2 px `lip`, radius 8 (`radius-key`).

### The dual edge
The outline and the rim each clear 3:1 on one side of the floor range, so one of the two always holds. The table uses the preview floor stand-ins, diorama theme (High Contrast in brackets):

| Floor | Ink outline | Rim | Edge that holds |
|---|---|---|---|
| Blockout | 14.4:1 (16.4) | 1.0 (1.3) | ink |
| Snow | 16.8:1 (19.1) | 1.2 (1.1) | ink |
| Sand | 11.2:1 (12.7) | 1.3 (1.7) | ink |
| Grass | 7.6:1 (8.6) | 1.9 (2.4) | ink |
| Mire | 2.5:1 (2.9) | 5.7:1 (7.3) | rim |
| Night | 1.2:1 (1.4) | 12.1:1 (15.6) | rim |

## States
- **Lit**: the default.
- **Show and hide**: the whole widget fades (its render opacity) or moves; Reduce Motion turns movement into a fade. A plate's FILL alpha is never below 100%: never a translucent plate on the world. Each component cites its own motion row (TallyPlate PlateDrop, Banner BannerFade).
- **Unlit** (`.is-unlit`): the Acceptance ending. When the Tally fails, the plates go dark.
  - The rim turns `lip` and the rim shade turns `well`.
  - All lettering turns `text-disabled`. Bar fills, bar lips and well lips turn `plate-raised`. Trails, leads and notches hide.
  - High Contrast keeps the full white outline (core, D22): the rim stays white on all four sides and wells keep their white `lip`; the dark fills and the disabled lettering carry the change. Sinking or dimming never removes an edge there.
  - The plate keeps its outline, its print and its 100% fill: only colours change.
  - Timing: `TODO(DECISION)`, owned by the ending sequence. Suggested: each plate swaps over 400 ms, linear; plates go 250 ms apart: rail head first, then tickets, spell slots, gauge plate and loadout last.
- **Inset card** (attribute cards, detail panes, list rows inside a menu plate; add `.tt-selectable` and `.tt-focus--fill-gap`):
  - Rest: `lip` border.
  - Hover (`.is-hover`, mouse only): the edge doubles to 4 px `rim`. In High Contrast `lip` and `rim` are both white, so the width carries it.
  - Focused (`.is-focused` / `:focus-visible`): core `.tt-focus--fill-gap`. An inset card has no outline of its own, so an `ink` ring fills the gap and the 3 px `focus` ring sits outside it, with no keyline (the plate is the ground). Focus keeps the hover edge.
  - Selected (`.is-selected`): the core Selected look, one everywhere (D6): `enamel` fill, `ink` text, `ink` border.
  - Selected + focused: the enamel card with the focus ring on top.
  - In UMG: the core focus ring brush (`03` Focus ring) in its fill-gap form, shown on keyboard or gamepad focus only.

## Data and implementation
- **No plate widget exists today.** `UMordecaiCombatHUDWidget::BuildDefaultLayout` puts bare UProgressBars and TextBlocks on a CanvasPanel.
  - Build one reusable `WBP_TT_Plate` (new): a UserWidget with a NamedSlot `Content`, a `Variant` enum (Hud, Menu, Inset) and a `bRiveted` flag (Tally and front-end plates only).
  - Wrap the existing `BindWidgetOptional` children in it. Binding is by name, so wrapping is safe: `HealthBar`, `StaminaBar`, `PostureBar`, `SpellPointsBar`, `KillCounter`, `SpellCooldownA` / `SpellCooldownB`, and the pause menu's `TabContentBorder`.
- **Construction**: the Plate stack in `03-building-in-unreal.md` (the asset manifest): an Overlay of five UBorders whose brushes are `DrawAs = RoundedBox` with per-corner `OutlineSettings.CornerRadii`, bottom to top:
  1. Print: radius 15, tint `ink` at alpha 0.55, render translation (0, 4) on the HUD or (0, 6) in menus. Same size as layer 2.
  2. Outline: radius 15, `ink`.
  3. Rim shade: padding 3, radius 12, `rim-shade`.
  4. Rim: padding (3, 3, 5, 5), radius 12, `rim`. The 2 px of layer 3 left showing on the right and bottom is the rim shade.
  5. Fill: padding 5, radius 10, `plate`. Content goes inside with 16 or 24 padding.
  - No plate textures: a theme change is only a tint change.
  - Rivets: four 12 px RoundedBox Images (ink ring, `rim-shade` face, `rim` highlight) in the plate's corners.
- **Inset**: one Border, RoundedBox radius 12, fill `plate-raised`, outline 2 px `lip`. **Well**: one Border, RoundedBox radius 8, fill `well`, outline 2 px `lip`.
- **Colours** come from the theme DataAsset (`UMordecaiUIThemeDataAsset`, `DA_UITheme_Diorama` / `DA_UITheme_HighContrast`), which holds the tokens.json hex values. Convert with `FLinearColor::FromSRGBColor`; never type linear values.
- **Sizes** at 1920 x 1080. Each box is the plate body; the outline (3 px) and the print (+4 below) sit outside it, inside the 32 px edge margin. In UMG, size the slot as body + (6, 10) and offset it by (-3, -3).

| Plate | Class | Anchor | Offset | Body |
|---|---|---|---|---|
| Gauge plate | `.tt-plate--gauge` | top-left | (32, 32) | 456 x 152 |
| Spell slot, x4 | `.tt-plate--slot` (well `__well--slot` 64 x 64) | top-left | x 32 / 120 / 208 / 296, y 216 | 78 x 78 |
| Rail head | `.tt-plate--rail` | top-right | (-32, 32) | 320 x 56 |
| Big-moment plate | `.tt-plate--moment` | top-centre | (0, 40); (0, 168) under a boss bar | 640 x 128 |
| Loadout | `.tt-plate--loadout` | bottom-right | (-32, -32) | 340 x 84 |
| Pause shell content | `.tt-plate--menu` (`TabContentBorder`) | centre | (460, 276) | 1000 x 584, under the 56 px tab bar; content box 948 x 540 (padding 20 / 24) |

- **Hit testing**: HUD plates are `SelfHitTestInvisible`. Menu plates are `SelfHitTestInvisible` too; only their controls take clicks.
- **Layer**: HUD plates are children of the one HUD root, pushed once on `UI.Layer.Game` (D20), so the pause veil covers them and the death screen hides them. Today the HUD uses `AddToViewport` and draws over the menus.
- **Tokens**: `plate`, `plate-raised`, `well`, `lip`, `rim`, `rim-shade`, `ink`, `enamel`, `text`, `text-muted`, `text-disabled`, `focus`, `print-hud`, `print-menu`, `radius-plate`, `radius-key`, `stroke-rim`.

## Copy
A plate has no copy of its own; it carries other components' strings.
- Header titles use `ui-title`, sentence case, exact system vocabulary ("Stored Resources").
- The Tally voice appears only on big-moment plates. Plates for death, the boss bar, biome cards and endings use the World voice, with no remark.
- Example copy in the preview: the riveted "Feat unlocked / Pyromaniac" plate (TallyPlate draws the real one), "Searing Expanse" as a World-voice plate (Banner draws the real one) and "Kills: 4".

## Do / Don't
- Do keep every plate's fill 100% opaque, with the outline and the print. Fade the whole widget, never the fill.
- Do use only plate text tokens on a plate: `text`, `text-muted`, `text-disabled`, rarity and feat colours, `danger-text`, `good`.
- Do step down the depth ladder inside a plate: inset plate, then well.
- Do use the core Selected fill and the core focus variants on inset cards; no local rings or index bars.
- Don't nest an outlined plate inside another plate, and don't give an inset plate an outline or a print.
- Don't let `rim-shade` be the only edge, and don't swap the rim to a hue to signal state on the HUD.
- Don't put rivets on HUD plates or World-voice plates, and don't add screws, bevel gradients, glass, blur or translucent fills. The tin is flat lithograph colour.
