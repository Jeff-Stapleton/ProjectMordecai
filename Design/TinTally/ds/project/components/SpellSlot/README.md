# SpellSlot

A 78 x 78 plate for one equipped spell: its glyph, the input that casts it, its Spell Point cost and its cooldown. Use four in a row under the gauge plate, one per spell input (Q, E, R, 1), always visible; the SkillRadial below is the bindings spec's alternative model, a proposal only.

## Anatomy

- **Plate** 78 x 78, `.tt-plate` (radius 12, 2 px `rim`, 3 px `ink` outline, print shadow; no decorative rivets: it is a HUD plate). Row: anchor top-left, y 216, x 32 / 120 / 208 / 296 (88 px pitch, 10 px gutters). Box x 32-374, y 216-294; the cost rivets rise to y 207 and the keycaps hang to y 304.
- **Well** inset 5 px inside the rim: 64 x 64 (the system's spell slot well), radius 8, `well` + 2 px `lip`. Spell glyph 48 px, centred. Tint: Fireball `dmg-fire`, Restoration `heal`, Blink and Stone Skin `text`.
- **Cost rivets.** One 14 px disc per Spell Point (`spell-points`, 2 px `ink` ring), centred on the top edge, right-aligned 6 px from the right, 3 px apart: Fireball 3, Blink 2, Stone Skin 4, Restoration 3 (max 4 = 65 px). They are Spell Point data, not ornament, so they are allowed on a HUD plate.
- **Keycap** `.tt-key--sm` (32 px, letters Mono 800 20 px), hanging off the bottom-left corner: 6 px past the plate's left edge and 10 px below its bottom edge, 3 px clear of the previous slot's outline.
  - Keyboard (square): Q, E, R, 1.
  - Gamepad: LB and RB as small pill keycaps (`.tt-key--sm.tt-key--shoulder`); D-pad Up as Keycap's 32 px cross (`.tt-key--sm.tt-key--dpad.tt-key--dpad-up`: 10 px `enamel` arms in a 2 px `ink` edge, a 6 x 8 `ink` insert in the Up arm); R as the dashed unbound cap (`.tt-key--none` on `plate`), because Spell 3 has no gamepad button.
- **Cooldown: the SpellSlot shutter** (the asset manifest's name). A tin roller shutter (4 px `plate-raised` slats, 2 px `well` grooves) hangs from the top of the well. Its height = remaining / total, so it rolls up and uncovers the glyph from the bottom. A 3 px `rim` lip marks its edge. `CooldownText` sits centred in the well, `num-m`, `text`, with a 2 px `ink` outline.

## States

| State | Trigger | Look |
|---|---|---|
| Ready | no cooldown effect, SpellPoints >= cost, not blocked | rivets filled, glyph at 100% |
| Ready glint | the cooldown effect ends | ReadyGlint: two `rim` bands (9 px and 3 px, 35°) sweep once across the well, the lip turns `rim`, then they settle |
| Cooling | an active GE carrying the spell's cooldown tag | shutter + seconds ("1.4", "12.6"), glyph at `opacity-unaffordable` (0.4) |
| Not enough SP | SpellPoints < cost | rivets hollow (`well` fill, `lip` ring), glyph at 0.4, an "SP" lock chip centred over the well (30 px `plate-raised` pill, 2 px `lip`, `lock` glyph 20 + "SP" `ui-label`) |
| Cooling and not enough SP | both | the shutter and seconds win; rivets hollow; no lock chip |
| Casting | `Mordecai.State.Casting` from this slot's ability (Windup and Cast phases) | plate rim and well lip turn `spell-points` and the well lip doubles to 4 px (a 6 px violet frame), so it also reads by width in High Contrast, where the resting rim and lip are white; keycap sunk 3 px (`.is-pressed`, Motion: KeyPress) |
| Silenced | `Mordecai.Status.Silenced`, or casting blocked by PostureBroken / KnockedDown | glyph at 0.4, muzzle badge: 46 px `plate` disc, 3 px `debuff` ring, `silenced` glyph 32 in `debuff`; rivets stay filled (it is affordable, just blocked) |
| Refused press | the input arrives in any blocked state, or `Mordecai.Event.CastInterrupted` | Motion: Shake, with the negative thunk. Fixes "spent SP with no feedback" (N20) |

The violet casting frame is used for nothing else. The 40% glyph always comes with a second cue (shutter, hollow rivets + lock, or muzzle).

**Reduce Motion:** no glint sweep and no shake; the lip fades to `rim` instead (ReadyGlint's Reduce Motion form).

## SkillRadial (proposal, `TODO(DECISION)`)

The bindings spec (`control_bindings_v1.1`) models two equipped skills instead of four direct slots: Use Skill A = Q / RB (tap), Use Skill B = E / LB (tap), and a hold on the same input opens a radial ("Option B: radial does NOT slow/pause time; selection is committed"). The game ships four direct slots, so this is drawn as a proposal only.

- **Opens** while Skill A or Skill B is held; **commits** the aimed option on release; releasing with no aim keeps the equipped skill. A tap still casts. **No time slow**, so no veil and no input-mode change: movement and the world keep running.
- **Layout:** centred on the player (the screen centre, 960, 540). Up to 8 sockets clockwise from the top, 45° apart, 128 px from the player to each socket's centre; the centre stays clear so the player stays visible. Each socket is a 64 px round plate (`.tt-plate`) with a 40 px spell glyph.
- **Socket states:** equipped = Selected (`enamel` fill, `ink` glyph, `.tt-selectable.is-selected`); aimed = the focus ring (ink | focus | ink); empty = a `plate` disc with a dashed `lip` ring (nothing to pick). Unaffordable spells keep the slot's rules (hollow rivets on the HUD slot after equipping), not drawn here.
- **Info plate** above the ring (52 px tall, top 246 px above the player, so it ends about 30 px above the top socket's outline): the held keycap (sunk), the aimed spell's name (`ui-title`) and "2 SP · 4.0 s" (`num-s`, `text-muted`). It sits above, never below the player, because the callout lane is under the player's feet.
- **Data:** the known spells (the four today; Cone of Cold and Magic Missile in the preview are examples from the new spells proposal). Proposed widget `UMordecaiSkillRadialWidget`, a child of the HUD root (D20), never pushed on a layer, since it must not take input focus from the game.
- **Undecided:** whether the game moves to Skill A / B at all (US-097), the hold threshold, the aim dead zone, and keyboard aim (mouse direction from the player).

## Data and implementation

- **Widget:** UMordecaiSpellCooldownWidget. Today there are two (`SpellCooldownA`, `SpellCooldownB`, 120 x 16, never bound: N8). US-088 makes four: keep `SpellCooldownA` and `SpellCooldownB`, add `SpellCooldownC` and `SpellCooldownD`, each 78 x 78 at the positions above, `bAutoSize` false, children of the HUD root canvas (D20).
- **Names to keep:**
  - `CooldownProgressBar` becomes the SpellSlot shutter (asset manifest): a UProgressBar with `BarFillType` TopToBottom, an empty background and `FillImage` = `M_UI_PatternFill` at 0° (4 px `plate-raised` slats, 2 px `well` grooves), with a 3 px `rim` Image at its edge. Today's Percent is elapsed / total (1 = ready); feed it 1 − that.
  - `CooldownText`: "%.1f", empty when ready. Mono `num-m` 24 px (18 pt), FSlateFontInfo `OutlineSettings` 2 px `ink`.
- **New optional widgets** (C++ fallbacks needed): `SpellIcon` (UImage: the ability's `Icon`, e.g. `T_UI_Ability_Fireball`, or the stamp glyph), `SlotKey` (keycap: a text block "Q" until CommonInput device glyphs ship), `CostRivets` (UHorizontalBox of UImage), `LockChip`, `MuzzleBadge`.
- **Cooldown tags:** `Mordecai.Cooldown.Spell.Fireball`, `.Blink`, `.StoneSkin` and `.Restoration` (parent `Mordecai.Cooldown.Spell`) are **proposed in US-088** (AC-088.1); they do not exist in `MordecaiGameplayTags` yet, and no `DA_Spell_*` sets a `CooldownTag` (US-089). The duration comes from the active GE (the widget already reads it).
- **Data:** cost = the spell's SpellPoints cost (`DA_Spell_*`). SpellPoints from UMordecaiAttributeSet. Tags: `Mordecai.State.Casting`, `Mordecai.Status.Silenced`, `Mordecai.State.PostureBroken`, `Mordecai.State.KnockedDown`; event `Mordecai.Event.CastInterrupted` (SP lost, no cooldown applied).
- **Real values:** Q Fireball 3 SP, 2.0 s; E Blink 2 SP, 4.0 s; R Stone Skin 4 SP, 15.0 s; 1 Restoration 3 SP, 10.0 s.
- **Inputs** (MordecaiHeroComponent.cpp): Spell1 Q / LB, Spell2 E / RB, Spell3 R / none, Spell4 One / D-pad Up. The bindings spec gives 1 and D-pad Up to quick items A and C (see QuickItems), so those keys are part of the same US-097 decision.
- `TODO(DECISION)` US-097: which gamepad mapping is authoritative (the bindings spec puts Skill A on RB), and what Spell 3 gets on gamepad (AC-057.5 said LB + RB; the slot shows the unbound cap until then).
- `TODO(DECISION)`: four direct slots (the game) or Skill A / B with the SkillRadial (the bindings spec). This component draws the game.
- `TODO(DECISION)`: charges (Blink Rank 20, "2 charges") and upcasting (no input yet) have no slot treatment.
- **Tokens:** `plate`, `rim`, `ink`, `well`, `lip`, `plate-raised`, `spell-points`, `text`, `text-muted`, `debuff`, `dmg-fire`, `heal`, `enamel` (keycaps, the equipped socket), `focus`, `opacity-unaffordable`.

## Copy

- Keycaps: "Q", "E", "R", "1"; gamepad "LB", "RB", the unbound cap (no text), the D-pad cross.
- `CooldownText`: "%.1f" seconds ("1.4", "12.6"), empty when ready.
- Lock chip: "SP".
- The spell names (Fireball, Blink, Stone Skin, Restoration) appear in tooltips, menus and the SkillRadial's info plate, not on the slot.
- No Tally remarks.

## Do / Don't

- Do keep the slot 78 x 78 on the 88 px pitch, and keep the keycap visible in every state.
- Do pair the 40% glyph with its second cue every time.
- Don't draw the cooldown as a radial clock or a bare number.
- Don't write "Not enough SP" on the HUD: the hollow rivets and the lock chip say it.
- Don't reuse the violet casting frame or the debuff muzzle ring for anything else.
- Don't put a veil or a time slow behind the SkillRadial, and don't cover the player with it.
