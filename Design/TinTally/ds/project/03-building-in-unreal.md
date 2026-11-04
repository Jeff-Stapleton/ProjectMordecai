# Building it in Unreal

Every Mordecai widget today is UMG built in C++. `BuildDefaultLayout()` / `BuildDefaultContent()` run when the WBP has no root, and the WBP assets are empty shells. Tin Tally keeps that structure. It restyles the C++ fallbacks and lets a designer-built WBP replace any of them, provided the WBP keeps the `BindWidgetOptional` names. This section is the single asset manifest: components cite its asset names and do not invent their own.

## Theme

- Put the tokens in a `UMordecaiUIThemeDataAsset` with one asset per theme: `DA_UITheme_Diorama` and `DA_UITheme_HighContrast`.
  - Each field is an `FLinearColor` converted from the sRGB hex with `FLinearColor::FromSRGBColor(FColor::FromHex(...))`.
  - Settings > Accessibility > High Contrast swaps the asset. Because every plate, tag and well is a tinted RoundedBox (below), a theme swap is only a tint change.
- Fill the existing designer-overridable properties from the asset at construct time:
  - `UMordecaiStaminaBarWidget`: `GreenTierColor`, `YellowTierColor`, `RedTierColor`, `ExhaustedTierColor`
  - `UMordecaiPostureBarWidget`: `NormalPostureColor`, `BrokenPostureColor`
  - `UMordecaiFeatDisplayWidget::TierColors`
  - `UMordecaiDamagePopComponent`: `DamageTypeColors`, `HealingColor`
  - `UMordecaiEnemyIndicatorWidget::StatusIconMap`
- Replace the static constexpr rarity colours in `UMordecaiInventoryWidget` with the asset's rarity fields.

| Today (linear literal, displayed sRGB) | Token |
|---|---|
| Health `FLinearColor::Green` (#00FF00) | `vital` |
| Stamina tiers (0,1,0) / (1,1,0) / (1,0,0) | `stamina-green` / `stamina-yellow` / `stamina-red` + stripes |
| Exhausted (0.5,0,0) | no fill; the `StaminaDebtBar` hatch, `danger` lines on `well` (`ExhaustedTierColor` = `danger`) |
| Posture (0.2,0.6,1.0) | `posture` |
| Posture broken (1.0,0.2,0.2) | `posture` under an `ink` hatch, its lip to `rim`; no red, no chip (`BrokenPostureColor` = `posture`) |
| SP fill (0.4,0.2,0.8) | `spell-points` tokens |
| Cooldown fill (0.3,0.6,1.0) | the SpellSlot shutter: `plate-raised` slats, `well` grooves, a `rim` edge |
| Combo / positive feedback gold (1,0.84,0) | `good` |
| "Parry Failed!" (1,0.2,0.2) | `danger-text` |
| Enemy health `FLinearColor::Red` and the green/yellow/red indicator | `foe` (one colour) + `vital-trail` |
| Tab active (0.25,0.35,0.55) / inactive (0.12,0.12,0.15) | Selected `enamel` + `ink`; inactive `plate-raised` + `text-muted` |
| Pause content (0.05,0.05,0.08,0.9) and dim (0,0,0,0.65) | `plate` (opaque) and `veil` |
| Death dim | `veil-death` |
| "You Died" (0.8,0.1,0.1) | `danger-text` |
| Loading background (0.010,0.010,0.012) / title (0.86,0.80,0.68) | `plate` / `text` |
| Rarity Common...Gold constexpr | `rarity-common` ... `rarity-gold` |
| Feat Common / Rare / Legendary (0.8) / (0.3,0.5,1) / (1,0.75,0) | `feat-common` / `feat-rare` / `feat-legendary` |

## Asset manifest

| Asset | Build | Used by |
|---|---|---|
| **Plate stack** | An Overlay of five UBorders with `DrawAs = RoundedBox` brushes and per-corner `CornerRadii`, tinted from the theme asset, bottom to top: print (radius 15, `ink` at 0.55, render translation (0, 4) on the HUD, (0, 6) in menus); outline (radius 15, `ink`); rim shade (padding 3, radius 12, `rim-shade`); rim (padding 3 / 3 / 5 / 5, radius 12, `rim`); fill (padding 5, radius 10, `plate`). No plate textures. | Plate and everything built on it |
| **Pill stack** | The plate stack with every radius at half the height. | Status chips, the Inventory hint, enemy name pills, the boss phase tab |
| **Tag stack** | Tag's RoundedBox Overlay: print and outline (radii 7 / 13 / 13 / 7), face (padding 3, radii 4 / 10 / 10 / 4, `enamel`), and the grommet as three stacked RoundedBox Images centred at x 22 (22 px `ink`, 18 px `enamel-shade`, 12 px `well`). Pivot at the eyelet for TicketDrop. | Tag, tickets, the interact prompt, front-end tags, nameplates |
| **Well** | One Border: RoundedBox radius 8 (bars 6), `well` fill, 2 px `lip` outline. Inset cards: radius 12, `plate-raised`, 2 px `lip`. | Bar, SpellSlot, sockets, inset cards |
| **Keycaps** | RoundedBox: square (radius 8), round, pill; `enamel` face, 2 px `ink` border, a 4 px `enamel-shade` lip. Letters Mono 800. | Keycap and every keycap on screen |
| **Focus ring** | One RoundedBox ring Image, a 2 px `ink` keyline outside a 3 px `focus` ring, drawn the outline width (3 px, 4 in High Contrast) outside the piece; at offset 0 on keycaps and folder tabs; with an `ink` gap fill and no keyline on unoutlined rows and cards inside a plate. Set it as the CommonUI focus brush; shown on keyboard and gamepad focus only. | Every focusable control |
| **Strings** | Images tinted from the theme: the 2 px `ink` string, or the light cord (a 4 px `rim` Image between two 1 px `ink` Images). Never a child of the tag, so it stays plumb while the tag swings. | Tickets, nameplates, front-end tags |
| **`M_UI_PatternFill`** | The one pattern material. Parameters: Base, Line, Angle, Period, LineWidth. It samples `T_UI_Hatch16` and tiles in screen pixels (`GetPixelPosition`), not UVs, so patterns never stretch. Set it as a UProgressBar `FillImage` or an Image brush. | Stamina Red stripes; the `StaminaDebtBar` hatch; player posture broken (`posture` under `ink`); the enemy and boss break flood (`good` and `ink`); the Drawback hatch (`danger` and `ink` at −45°); the SpellSlot shutter |
| **SpellSlot shutter** | `CooldownProgressBar`: `BarFillType` TopToBottom, empty background, `FillImage` = `M_UI_PatternFill` at 0° (4 px `plate-raised` slats, 2 px `well` grooves), a 3 px `rim` Image at its edge. Percent = 1 − elapsed / total. `CooldownText` centred, `num-m`, 2 px `ink` outline. | SpellSlot |
| **Glyphs** | Single-ink masks: import the SVGs, or rasterise them at 64 px as white masks (Texture Group UI, compression UserInterface2D), tinted by the brush. | Every icon |
| **Tally atlas** | Bundle frames 0 to 5 at each size, each a filled layer (tinted per use) over the dotted ghost layer (`lip`), plus a cap-dot image, drawn by `UMordecaiTallyWidget` (`Count`, `Max`). Only for ranks, rarity badges, the seal and the throbber. | Tally, SkillsTab, rarity badges, LoadingScreen |
| **Badges** | 96 px silhouettes, an `ink` edge layer and a white face layer tinted per tier: feat circle, flat-top hexagon, eight-point starburst; the technique rounded square; the key tag. The feat medallion carries the feat's own glyph. | TallyPlate, FeatsTab, the spike |
| **Damage pops** | A pool of 20 `UMordecaiDamagePopWidget` (UMG) on a screen-space canvas in the HUD root, projected each frame with `ProjectWorldLocationToScreen`. Never constructed per hit. No Niagara text, no MeshText: they cannot draw the type glyph, the crit burst or the inverted Shadow print. | DamageNumber |
| **Low-health frame** | `LowHealthFrame`: an 8 px `vital` band (a Box-drawn Image with an 8 px margin, opaque) and a 3 px `ink` line inside it, HitTestInvisible, on the HUD root canvas. Not a plate; it does not pulse. | GaugePlate, the HUD root |
| **`M_UI_GroundDecal`** (proposed) | One deferred decal material on `UDecalComponent`s, projected straight down and sorted under actors, for enemy telegraphs, hazard zones, the aim chevron and the ground-target reticle. Parameters: Shape (sector, lane, circle, or a mask texture for blobs) with its params in cm, Charge (0-1), Fill (hatch, dense hatch, none), EdgeInk and EdgeRim in screen px (held constant across the foreshortened floor), RimDash. Colours from the theme asset, so High Contrast swaps them with the HUD. Floor stamps are a second small decal with a glyph mask. `TODO(DECISION)`: whether the UI or the VFX side builds and owns it. | WorldMarkers |

- **Print shadow:** a tinted copy of the piece's outline shape, `ink` at `opacity-print`, offset (0, 4) on the HUD and (0, 6) in menus. No blur.
- **Many-instance world widgets** (enemy tags, pickup labels): one SDF rounded-rect material (radius, outline, edge, face) may replace the Overlay stack for performance. Its colours still come from the theme asset.
- **Retired names** (do not use): `T_UI_PlateMask`, `M_UI_Plate` and its Material Parameter Collection, `T_UI_PlateFrame`, `T_UI_Plate`, `T_UI_PillPlate`, `T_UI_TagMask`, `T_UI_KeySquare`, `T_UI_KeyRound`, `T_UI_KeyPill`, `M_UI_BarFill`, `T_UI_StripeMask`, and Lyra's Niagara number pop. Device glyph art for CommonInput (Keycap's controller data) is a separate set and stays.

## Fonts

- Download Rowdies, Atkinson Hyperlegible Next and Atkinson Hyperlegible Mono from Google Fonts. Import the static TTFs, because UE handles variable fonts poorly.
- Build the composite fonts `F_Tally_Display`, `F_Tally_UI` and `F_Tally_Mono`.
- Make CommonUI text styles `TS_Tally_<style>` for every type style in this system:
  - Size in points = px x 0.75.
  - LineHeightPercentage = line height / size.
  - Letter spacing in 1/1000 em.
  - Outlines through `FontOutlineSettings`.
- Credits carry the three OFL notices.

## The HUD root and its layer

Today `AMordecaiPlayerController` creates `UMordecaiCombatHUDWidget`, a `UUserWidget`, and calls `AddToViewport()` (ZOrder 0, above Lyra's root layout). `AMordecaiCharacter` adds `UMordecaiEquippedWeaponWidget` with `AddToViewport(50)`. The HUD therefore draws over the pause veil and the death screen. The death screen is already pushed on `UI.Layer.Game` by `UMordecaiDeathScreenComponent`; the pause menu on `UI.Layer.Menu` by `UMordecaiPauseMenuSubsystem`.

The target:
1. **Reparent** `UMordecaiCombatHUDWidget` to `ULyraActivatableWidget`. `UPrimaryGameLayout::PushWidgetToLayerStack` only takes a `UCommonActivatableWidget`. Use `ULyraHUDLayout` as the model, but do not derive from it: it binds `UI.Action.Escape` to push Lyra's escape menu, which would fight the Mordecai pause menu (#68). Leave `InputConfig` at Default so the HUD never changes the input mode.
2. **Push it once** on `UI.Layer.Game` when the local player's pawn is ready, instead of `AddToViewport`. It stays for the session.
3. **Make every HUD region a child** of its root CanvasPanel: `GaugePlate`, `SpellCooldownA`-`D`, `StatusEffectBar`, `TallyRailHead`, `TallyTicketStack`, `TallyPlate`, `Banner`, `BossBar`, `CombatFeedback`, `ComboCounter`, `PickupPrompt`, `QuickItems`, the loadout plate (`UMordecaiEquippedWeaponWidget` moves in from its own `AddToViewport(50)`), the damage-pop canvas and `LowHealthFrame`. Each slot takes the anchor, offset, alignment and size from the HUD map, `bAutoSize` false. Never push any of them separately: a layer is a stack that shows only its top widget, and a second push hides the HUD.
4. **The death screen** stays the only other push on `UI.Layer.Game`. As the top of the stack it hides the HUD while the player is dead; popping it shows the HUD again.
5. **Keep values live while hidden.** A deactivated HUD is only hidden. Bind its GAS delegates once, when the pawn is ready, and keep them while it is hidden; also re-read every attribute and count in `NativeOnActivated`, so changes made while it was hidden (Kills resets to 0 on `Mordecai.Event.PlayerDeath`) show when it returns.
6. **Check the experience.** The Mordecai experience must push no other HUD layout onto `UI.Layer.Game` (Lyra's widget actions do this for its own modes).
7. **Town screens.** Town dialogue (`UMordecaiDialogueWidget`) is pushed on `UI.Layer.GameMenu`, under `UI.Layer.Menu`, so the pause menu can open above it; the service screens (`UMordecaiServiceScreenWidget`, `UMordecaiMageTowerWidget`) are pushed on `UI.Layer.Menu`, as the pause menu is.

## Names to keep (BindWidgetOptional)

A WBP that replaces a fallback must provide these names exactly, or that piece silently disappears.

**HUD**
- `HealthBar`, `StaminaBar`, `PostureBar`, `SpellPointsBar`
- `SpellCooldownA` and `SpellCooldownB` (US-088 adds C and D)
- `ComboCounter`, `CombatFeedback`, `KillCounter`, `StatusEffectBar`, `PickupPrompt`

**Bars**
- `HealthProgressBar`, `StaminaProgressBar`, `PostureProgressBar`
- `SpellPointsProgressBar`, `SpellPointsText`
- `CooldownProgressBar`, `CooldownText`

**Text and status**
- `IndicatorContainer`, `StatusNameText`, `DurationText`, `TintImage`
- `KillCountText`, `ComboText`, `FeedbackText`, `PromptText`, `InventoryHintText`

**Menus and screens**
- Pause menu: `TitleText`, `TabBar`, `TabContentBorder`
- Inventory: `FilterBar`, `ItemListBox`, `ListPlaceholder`, `LedgerHeader`, `LedgerBox`, `LedgerPlaceholder`
- Death screen: `DeathText`, `CountdownText`, `KillCountText`
- Loading screen: `TitleText`, `StatusText`. It builds in `Initialize()` and stays opaque.
- Enemy indicator: `NameText`, `HealthProgressBar`, `StatusIconContainer`

**Panel types.** Three of these are declared as `TObjectPtr<UHorizontalBox>` in C++. A designer WBP that uses a WrapBox (or any other panel) under that name will not bind until the UPROPERTY is retyped to `UPanelWidget*`, and the C++ fallback must then build the new panel:
- `FilterBar` (`UI/MordecaiInventoryWidget.h`): becomes a `UWrapBox` so the eight filters wrap (fixes #37).
- `IndicatorContainer` (`UI/MordecaiStatusEffectBarWidget.h`): becomes a `UWrapBox`, two rows of chips.
- `TabBar` (`UI/MordecaiPauseMenuWidget.h`): retype it too if it is ever restyled into anything but an HBox.

**New optional widgets** get C++ fallbacks too:
- Gauge: `GaugePlate`, `HealthPercentText`, `HealthTrailBar`, `HealthLeadBar`, `StaminaTierText`, `StaminaDebtBar`, `SpellPointTokens`.
- Rail and plates: `TallyRailHead`, `PendingBadge` (with `PendingText` "+%d"), `TallyTicketStack`, `TallyPlate`, `BossBar`, `Banner`.
- HUD root: `QuickItems`, `LowHealthFrame`.

**Gauge specifics.**
- `StaminaDebtBar`: a UProgressBar, `BarFillType` LeftToRight, `FillImage` = `M_UI_PatternFill` (the debt hatch), `Percent` = clamp(|Stamina| / MaxStamina, 0, 1) while Stamina < 0. It shares the left edge with the fill, so debt and fill use one scale. The floor is an absolute −50 (`UMordecaiStaminaSystem::StaminaFloor`, and the attribute clamp in `UMordecaiAttributeSet`), so the deepest debt is 50 / MaxStamina: 37% at the default MaxStamina of 134 (MaxStamina = 100 x (1 + 0.04 x EffEnd)). Stamina does not regenerate today (N30), so after a fight the bar sits at that floor.
- Player posture broken: while `Mordecai.State.PostureBroken` is on the player, the posture hairline floods with `posture` under an `ink` hatch (`M_UI_PatternFill`) and its lip turns `rim`. No red, no chip: the old `PostureBrokenChip` is gone. `TODO(DECISION)`: the player has no posture-break logic yet; only enemies break.
- `SpellPointTokens`: an HBox of token Images designed for 10 to 16 tokens, all inside the 268 px track. MaxSpellPoints = 10 x (1 + 0.04 x EffInt), which is 13 at the default INT, so the default reads "13 / 13". Above 16, collapse the tokens and show `SpellPointsProgressBar` in the same track (`TODO(DECISION)`, US-090).

**Spell slots.** The cooldown tags `Mordecai.Cooldown.Spell.Fireball`, `.Blink`, `.StoneSkin` and `.Restoration` (parent `Mordecai.Cooldown.Spell`) are proposed in US-088 (AC-088.1). They do not exist in `MordecaiGameplayTags` yet, and no `DA_Spell_*` sets a `CooldownTag` (US-089).

## Playtest safety

- **Text stays text.** The agent bridge reads only `Mordecai*` widgets, their visible `UTextBlock`s and `UButton`s, and a button's label is its first TextBlock child. Never bake strings into textures.
- **Casing:** caps come from `TextTransformPolicy = ToUpper`, so `GetText()` still returns "Paused", "You Died" and "Project Mordecai".
- **Keycaps lead their labels**, but until device-aware glyphs ship the TextBlocks keep "[F] Pick up …" and "[I] Inventory", and the keycap widget is not shown. Swap them in the same change as the playtests.
- **Animated text:** reveals, typing, scrambles and ghost digits run on an Image or overlay layer. Set the real TextBlock once, to the final shipped string.
- **Remarks and flavour lines** are new TextBlocks beside the shipping strings, never replacements.
- **Menu keys:** a new menu key needs a native `UI.Action.*` tag, a `+InputActions=` entry in `DefaultInput.ini`, and `RegisterUIActionBinding` in `NativeOnInitialized`, because CommonUI's Menu input mode blocks every unbound key. Keyboard Previous / Next Tab is unbound today (`TODO(DECISION)`, proposal Q / E): never draw it as bound, because the Tier 4 scripts press real keys.
- **Activatable widgets** (pause menu, death screen, service screens) are pooled. Bind in `NativeOnActivated` and unbind in `NativeOnDeactivated`. The HUD root is the exception: it is pushed once and only hidden, so it keeps its bindings and refreshes on activation (above).
- **Content:** save and commit every texture, material, font, style and WBP. Content that exists only in editor memory does not ship.
