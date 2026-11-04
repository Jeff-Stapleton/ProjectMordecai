# Keycap

A device-shaped input glyph, where the shape names the device:
- keyboard keys and mouse buttons get square caps;
- gamepad face buttons get round caps;
- shoulders get pills, and triggers get flat-footed pills;
- the D-pad gets a cross.

Use it wherever the UI names an input: interact prompts, button hints, the menu action bar, spell-slot corners and the controls list.

## Anatomy
- **Key** (`.tt-key`), the keyboard: 40 x 40 minimum, radius 8 (`radius-key`).
  - The face is `enamel` with a 2 px `ink` edge and a 4 px `enamel-shade` lip inside the bottom.
  - The letter is Mono 800 22 px `ink`; 20 px on the 32 px cap.
  - **Wide** (`--wide`, 72+) is for words: Tab, Esc, Shift, Alt, Enter. At 22 px, Tab, Esc and Alt fit 72; Shift and Enter grow to 90; Space is set to 104.
- **Mouse** (`--mouse`): a square cap, because the mouse is part of the KBM device, holding a drawn 20 x 27 mouse.
  - The mouse has a 2 px outline, with the buttons split 10 px from the top.
  - The lit button is solid ink: LMB left, RMB right, MMB a 6 x 9 wheel.
  - Mouse buttons are drawn, never spelled.
- **Face** (`--pad`): a 40 px circle with a plain letter A, B, X or Y. No brand colours.
- **Face position** (`.tt-key__diamond` in a `--pad` cap): four 10 px dots in a diamond, with the pressed one filled: bottom, right, left, top. It reads the same on any pad layout.
- **Shoulder** (`--shoulder`): a pill, 56 minimum. Used for LB, RB and the Menu / View buttons.
- **Trigger** (`--trigger`): a pill with a flat foot (radius 20 on top, 6 below), so LT and RT never read as LB and RB.
- **Stick click** (`--stick`): a 40 px circle with a 5 px `enamel` rim and a 2 px dish edge (`ink` 35% over `enamel-shade`) around an `enamel-shade` dish, holding L or R. The dish edge keeps it apart from a face button in High Contrast, where `enamel` and `enamel-shade` are both white. The 32 px cap uses a 3 px rim.
- **D-pad** (`--dpad` plus `--dpad-up | -right | -down | -left`): a 40 px cross.
  - Its arms are 12 px `enamel` inside a 2 px `ink` edge.
  - The bound direction carries an 8 x 9 `ink` insert, 4 px from the arm's tip. Enamel stays round the insert, so the cross never loses an arm on a dark plate.
  - **32 px D-pad** (`.tt-key--sm.tt-key--dpad`): drawn for 32 px, never scaled down (a scaled cross blurs its inked arm). 10 px `enamel` arms inside a 2 px `ink` edge (14 px across), and a 6 x 8 `ink` insert (8 x 6 on Left and Right), 4 px from the tip with 2 px of enamel round it. SpellSlot corners (Restoration on D-pad Up) and QuickItems use it; they draw no local cross.
- **Unbound** (`--none`): a dashed 2 px `lip` outline with nothing in it.
- **Chord and hold** (`.tt-keys`):
  - A chord joins caps with "+" (Mono 700 20, `text-muted` on plate, `ink-muted` on enamel).
  - A hold puts the word "Hold" before the cap.
- **Sizes**: 40 px on tags, prompts and menus; 32 px (`--sm`) in hint pills, hint bars, button leads and spell-slot corners. Every shape has a 32 px form (the preview's `--sm` row). Letters are 22 px on 40 px caps and 20 px on 32 px caps (the floor). On a 32 px wide cap "Esc" fits 56 and "Enter" grows to 80.
- **Placement**: the keycap LEADS its label everywhere, 8 px before it: "[F] Pick up Health Potion", "[I] Inventory", "[Esc] Stay", "[Enter] Identify" (D4).

## States
- **Normal**.
- **Pressed** (`.is-pressed`):
  - The cap sinks and the lip drops to 1 px (Motion: KeyPress).
  - High Contrast keeps the whole ink border and the dashed or solid edge: sinking shows by the offset, never by losing an edge (D22).
  - The face turns `enamel-shade`, with a 3 px pushed-in shade along its top edge.
  - The D-pad and the stick darken their face or dish to `ink` 22% over `enamel-shade` instead, which still shows in High Contrast.
  - Use it for live input feedback, such as the prompt's F while F is held, and on the rebinding screen.
- **Focused** (`.tt-focus--border` + `.is-focused` / `:focus-visible`): only where a keycap is itself a control, such as a row on the rebinding screen. The core ring sits at offset 0, right on the cap's ink border, with the 2 px ink keyline outside it. A keycap that only names an input never takes focus: its button or row does.
- **Disabled** (`aria-disabled`): the face turns `plate`, the edge `lip` and the letter `text-disabled`, with no lip. Use it when the action exists but is blocked, for example the spell keys while Silenced.
- **Unbound** (`--none`): the action has no key on this device. Never guess a glyph.

## Data and implementation
- **Widget** (new): `UMordecaiKeycapWidget`, a `UCommonUserWidget`.
  - Input: an `FKey`, or a `UInputAction` resolved through the player's Enhanced Input mappings for the current device.
  - Shape by key: `EKeys::IsGamepadKey` picks face, shoulder, trigger, stick or D-pad; mouse buttons pick the mouse cap; anything else is a square key.
  - The device follows `UCommonInputSubsystem::OnInputMethodChangedNative`: only the last device used is shown.
- **Short labels**: don't use `FKey::GetDisplayName`, which returns "Left Shift" and "Gamepad Face Button Bottom". Use this table:

| FKey | Cap |
|---|---|
| LeftShift / LeftAlt / SpaceBar / Escape / Tab / Enter | Shift / Alt / Space / Esc / Tab / Enter (wide) |
| One ... Four, letters | 1 ... 4, the letter |
| LeftMouseButton / RightMouseButton / MiddleMouseButton | mouse cap L / R / M |
| Gamepad_FaceButton_Bottom / _Right / _Left / _Top | A / B / X / Y |
| Gamepad_LeftShoulder / RightShoulder | LB / RB (pill) |
| Gamepad_LeftTrigger / RightTrigger | LT / RT (trigger) |
| Gamepad_LeftThumbstick / RightThumbstick | L / R (stick) |
| Gamepad_DPad_Up / Right / Down / Left | D-pad cross |
| Gamepad_Special_Right / Special_Left | Menu / View (pill) |

- **Construction**:
  - Square, round and pill caps: a Border with a RoundedBox brush (radius 8, 20 or 20; trigger corners 20, 20, 6, 6), `enamel` fill and a 2 px `ink` outline.
  - The lip is a 4 px `enamel-shade` Border along the bottom.
  - The letter is a `UCommonTextBlock` in Mono 800: 16.5 pt (22 px) on 40 px caps, 15 pt (20 px) on 32 px caps.
  - The mouse caps and the D-pad cross are two-layer images (a face mask and an edge mask, tinted from the theme) at 40 and 32 px, plus the insert Image per direction. The 32 px cross is its own art (10 px arms, 6 x 8 insert), not the 40 px one scaled. `TODO(DECISION)`: a drawn `T_UI_KeyDpad32` texture may read better at 720p, where the insert is about 4 x 5 px.
  - Focus: the core focus ring brush (`03` Focus ring) at offset 0.
- **CommonUI**: `TODO(DECISION)`: how CommonUI's own action widgets (`UCommonActionWidget`, the action bar, `UCommonButtonBase::InputActionWidget`) get these caps. Two options:
  - Mordecai controller data assets, replacing Lyra's `CommonInput_KeyboardMouse`, `CommonInput_Gamepad_XSX` and `CommonInput_Gamepad_PS5` in `B_CommonInputData`. Every CommonUI action widget then works unchanged, but each key needs a brush (about 60 small textures).
  - A text-driven `UMordecaiKeycapWidget` everywhere. No per-key art, but the action bar needs a custom entry widget.
- **Pad layout**: `TODO(DECISION)`: letters for every pad, the position diamond for every pad, or per-pad letters and symbols. The PS5 data set exists in Lyra content; with no brand colours, letters on a PlayStation pad are wrong.
- **Bindings today** (`MordecaiHeroComponent.cpp`, `Config/DefaultInput.ini`): see the device rows in the preview.
  - Light attack: LMB / RT. Heavy attack: Hold LMB / Hold RT.
  - Block: RMB / LT. Parry: RMB + LMB / LT + RT.
  - Dodge: Alt / B. Jump: Space / A. Sprint: Shift / L stick. Interact: F / X.
  - Fireball: Q / LB. Blink: E / RB. Stone Skin: R / unbound. Restoration: 1 / D-pad Up.
  - Next weapon: Tab / Y (a tap; BackSlash is previous weapon, with no pad button). The bindings doc's Weapon Set Swap (Tab / hold Y) and Throw (G / tap Y) are not mapped yet. Inventory: I / unbound. Pause: Esc / Menu. Back: Esc / B.
  - Open items:
    - US-097 `TODO(DECISION)`: Q and E are on LB and RB today, but the bindings doc says RB = Skill A.
    - Stone Skin has no gamepad button. AC-057.5's LB + RB is not mapped.
    - The gamepad stick key names are invalid today (US-097), so L-stick sprint does not work on a pad yet.
- **Names to keep**: the pickup prompt keeps `PromptText` and `InventoryHintText`. Until device glyphs ship, their strings stay "[F] Pick up {0}" and "[I] Inventory" (see Tag).
- **Tokens**: `enamel`, `enamel-shade`, `ink`, `ink-muted`, `plate`, `lip`, `text-disabled`, `text-muted`, `radius-key`, `radius-pill`.

## Copy
- Cap labels are exactly as in the table: F, I, Q, E, R, 1, Tab, Esc, Shift, Alt, Space, Enter, LB, RB, LT, RT, A, B, X, Y, L, R, Menu, View.
- "Hold" comes before a held input; "+" joins a chord.
- The CommonUI action names stay "Back", "Previous Tab", "Next Tab" and "Close".

## Do / Don't
- Do let the shape carry the device, and show only the last device used.
- Do put the keycap before its label, every time.
- Do draw mouse buttons and the D-pad. Never spell them ("LMB", "D-pad Up") on a cap.
- Do show `--none` for an action with no binding on this device, and hide hints that have no key (the gamepad has no Inventory key).
- Don't colour face buttons (no green A, red B), and don't draw PlayStation shapes or any platform logo.
- Don't shrink the letter below 20 px; the 32 px cap is the smallest. Don't scale a 40 px cap down to 32: use `--sm`.
- Don't put a keycap straight on the world: it sits on a tag, a plate or a chip.
