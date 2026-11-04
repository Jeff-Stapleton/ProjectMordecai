# LoadoutPlate

The equipped weapon at the bottom-right of the HUD: its glyph, its name, its damage subtype and the swap key. Always visible during play; it is the one place the weapon name shows (fixes N3).

## Anatomy

- **Plate** 340 x 84, `.tt-plate` (no rivets: HUD plate). Anchor bottom-right (1,1), offset (−32, −32), alignment (1,1): x 1548-1888, y 964-1048. The quick items row sits 16 px above it (its count pills stay 6 px clear of this plate's outline; see QuickItems).
- **Well** 64 x 64 (radius 8, `well`, 2 px `lip`), 8 px from the left rim. Weapon glyph 48 px in `text`.
- **Text column** (166 px with the "Tab" keycap, 188 px with the round "Y"; 12 px gaps either side):
  - `DisplayedWeaponName` in `ui-title` 26/32 700, `text`, single line, ellipsis on overflow.
  - Sub-line in `ui-label` 20/26, `text-muted`: the subtype glyph (20 px `slash`, `pierce` or `blunt`) and its word; an elemental bonus adds its type glyph after it.
  - Nothing else. **No strokes under the weapon name** (D1): the tally mark has three jobs (ranks, rarity, the seal / throbber), and the light-attack combo shows only as "Hit N" under the player (CombatCallout).
- **Keycap** "Tab" (`.tt-key`, 40 px tall, 62 px wide with Mono 800 22 px letters) at the right end; on gamepad a round "Y" (40 px).
- Name 32 + 2 + sub-line 26 = 60 px, centred in the 80 px inner height.
- **Widths measured** in the 166 px column: "Quarterstaff" 149 px (the longest weapon type name), "Greatsword" 138 px, the example "Emberfang" with its rarity badge 161 px.

## States

| State | Look |
|---|---|
| At rest | glyph, name, subtype |
| Swapping | on `OnWeaponChanged`: the keycap sinks (Motion: KeyPress), the plate dips 3 px and springs back (120 ms ease-out back), the glyph swaps on the press, the new name types in (Motion: TypeOn). The type-on runs on an overlay; `WeaponNameText` is set once, to the final name. The outline stays whole in High Contrast; only the print collapses |
| Unarmed | name "Unarmed", the `unarmed` glyph in `text-muted`, "Blunt". The well lip stays solid: dashed means unavailable (D24), and no weapon is a loadout, not a locked slot |
| Long name | ellipsis. "Quarterstaff" (149 px) is the longest weapon type name and fits the 166 px column; found weapons with longer names ellipsize |
| Found weapon (example) | name in its rarity colour with the rarity tally badge before it; an elemental bonus adds its type glyph after the subtype. Emberfang: Purple badge, "Slash" + `fire` glyph. Equipping found weapons is not built (N22): `TODO(DECISION)` |

**Reduce Motion:** no dip or type-on; the name swaps with a 150 ms fade.

## Data and implementation

- **Widget:** UMordecaiEquippedWeaponWidget. Today it has no tree, draws nothing (N3) and is added by `AMordecaiCharacter` with `AddToViewport(50)`.
  - Build the fallback tree in C++.
  - Move it into the root canvas of the HUD root (D20: `UMordecaiCombatHUDWidget` reparented to `ULyraActivatableWidget`, pushed once on `UI.Layer.Game`) at the box above. It is never pushed on its own and never added to the viewport.
- **Keep:** `DisplayedWeaponName` (the weapon's DisplayName, or "Unarmed") and `DisplayedWeaponType`.
- **New optional widgets:** `WeaponNameText` (UTextBlock; the bridge can read it), `WeaponIcon` (UImage), `DamageSubtypeText`, `ElementIcon` (UImage, Collapsed unless the weapon carries an element), `SwapKey`.
- **Data:**
  - `OnWeaponChanged` (MainHand / TwoHand) and `EMordecaiWeaponType`.
  - The subtype comes from the profile's damage tag (`Mordecai.Damage.Physical.Slash` / `.Pierce` / `.Blunt`).

    | Weapon | Subtype |
    |---|---|
    | Longsword | Slash |
    | Dagger | Pierce |
    | Greatsword | Slash |
    | Spear | Pierce |
    | Mace | Blunt |
    | Shortsword | Slash |
    | Axe | Slash |
    | Quarterstaff | Blunt |
    | Unarmed | Blunt |

- **Keys:** Tab (or gamepad Y tap) cycles to the next of the 5 weapons; BackSlash goes to the previous one and is not shown.
  - `TODO(DECISION)` (US-097): the spec has Tab / hold Y for a Set 1 ↔ Set 2 swap, and Y tap for Throw.
- **Brush:** the plate stack from the asset manifest (RoundedBox brushes, no plate textures).
- **Tokens:** `plate`, `rim`, `ink`, `well`, `lip`, `text`, `text-muted`, `enamel` (keycap), `rarity-*`, `dmg-fire`.

## Copy

- Names: "Longsword", "Dagger", "Greatsword", "Spear", "Mace" (the arena loadout). "Unarmed" is a shipping string; keep it exactly.
- Subtype words: "Slash", "Pierce", "Blunt".
- Keycaps: "Tab"; gamepad "Y".
- No Tally remarks on the loadout plate.

## Do / Don't

- Do keep the name a real UTextBlock.
- Don't draw tally strokes, pips or a combo count under the weapon name: the combo is CombatCallout's "Hit N".
- Don't put the weapon name bottom-centre: that lane belongs to the pickup prompt.
- Don't show rarity colour on the starting loadout (they have no rarity), and don't invent stats on this plate.
