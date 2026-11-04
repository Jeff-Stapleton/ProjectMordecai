# Project Mordecai: UI that exists in code today

Scope: every file in `Source/LyraGame/Mordecai/UI/`, plus the Mordecai UI code elsewhere: `MordecaiPlayerController.cpp`, `MordecaiCharacter.cpp`, `Enemy/MordecaiEnemyCharacter.cpp`, `Items/MordecaiItemPickup.cpp`, `Items/MordecaiItemLibrary.cpp`, `MordecaiHeroComponent.cpp` and `Playtest/MordecaiAgentBridgeState.cpp`. It also covers the `Config/*.ini` files, the MordecaiCore content listing, and screenshots from recent packaged playtests (Saved/Playtests) that show what players see now.

**Main finding.** All Mordecai UI is **built in C++ at runtime**. Every `WBP_*` asset is an empty shell: it was made by Python `WidgetBlueprintFactory`, and `Config/DefaultEditor.ini` sets `DefaultRootWidget=None`. A byte scan of `WBP_CombatHUD.uasset` finds no child widgets. So each widget's `BuildDefaultContent()` / `BuildDefaultLayout()` fallback is what ships. The visual style is mostly engine defaults:
- Roboto Bold 24 text.
- The default `UProgressBar` style (light grey track, tinted fill).
- The default `UButton` style (dark box with a thin light outline, tinted).

There are **no UMG animations, no textures, no custom fonts and no style assets** in MordecaiCore.

---

## Widget inventory

### 1. UMordecaiCombatHUDWidget: combat HUD root (`UI/MordecaiCombatHUDWidget.*`; asset `/MordecaiCore/UI/WBP_CombatHUD`)
- **Base and creation.** It is a `UUserWidget`. `AMordecaiPlayerController::CreateCombatHUD()` creates it in `OnPossess`, for a local controller only.
  - It loads `/MordecaiCore/UI/WBP_CombatHUD.WBP_CombatHUD_C` by path string, then calls `AddToViewport()` (ZOrder 0).
  - It binds to the PlayerState ASC, and binds the pickup prompt to the pawn's `UMordecaiPickupInteractionComponent`.
- **Built by** `BuildDefaultLayout()` when `WidgetTree->RootWidget` is null. Root is a `UCanvasPanel` named "Root". All children are `BindWidgetOptional` UPROPERTYs, created with `CreateWidget`.
- **Children, in design px.** `UIScaleRule=ScaleToFit`, `DesignScreenSize` 1920×1080. The UIScaleCurve gives 0.666 at 720p and 1.0 at 1080p. Slot sizes are fixed (`SetAutoSize(false)`).

| Slot | Anchor | Position | Size | Alignment |
|---|---|---|---|---|
| HealthBar | TopLeft (0,0) | (20, 20) | 250×20 | (0,0) |
| StaminaBar | TopLeft | (20, 48) | 250×20 | (0,0) |
| PostureBar | TopLeft | (20, 76) | 250×16 | (0,0) |
| SpellPointsBar | TopLeft | (20, 100) | 250×16 | (0,0) |
| SpellCooldownA | TopLeft | (20, 124) | 120×16 | (0,0) |
| SpellCooldownB | TopLeft | (150, 124) | 120×16 | (0,0) |
| KillCounter | TopRight (1,0) | (-170, 20) | 150×24 | (0,0) |
| ComboCounter | BottomCenter (0.5,1) | (0, -120) | 200×40 | (0.5, 1) |
| CombatFeedback | BottomCenter | (0, -80) | 300×30 | (0.5, 1) |
| StatusEffectBar | BottomLeft (0,1) | (20, -60) | 400×40 | (0,0) |
| PickupPrompt | BottomCenter | (0, -30) | 420×56 | (0.5, 1) |

- **`GetBoundChildCount()`** counts 10 slots. It omits PickupPrompt.
- **Cooldown binding.** `BindToASC` binds every child except the cooldowns. Its comment says they are bound individually through `BindToCooldownTag()`, but nothing calls that.
- **Draw order.** `AddToViewport` puts the HUD on the viewport overlay, above Lyra's root layout (`GameUIPolicy.cpp:125 AddToPlayerScreen(1000)`). So the HUD draws over the pause-menu and death-screen dims, and the screenshots show bright bars while paused or dead.

### 2. UMordecaiHealthBarWidget (`UI/MordecaiHealthBarWidget.*`; asset `WBP_HealthBar`, unused by the C++ layout)
- **Tree:** a CanvasPanel holding `HealthProgressBar` (a UProgressBar anchored (0,0,1,1), offsets 0).
- **Fill:** `FLinearColor::Green`. Initial percent is 1.0.
- **Shows:** Health/MaxHealth, clamped to 0–1 (0 if MaxHealth ≤ 0). It is bound through GAS attribute-change delegates. There is no text and no damage-trail or lag animation.

### 3. UMordecaiStaminaBarWidget (`UI/MordecaiStaminaBarWidget.*`)
- **Tree:** a CanvasPanel holding `StaminaProgressBar`, the same fill layout as the health bar.
- **Tier colors** are EditAnywhere UPROPERTYs:
  - `GreenTierColor` (0,1,0)
  - `YellowTierColor` (1,1,0)
  - `RedTierColor` (1,0,0)
  - `ExhaustedTierColor` (0.5,0,0)
- **Thresholds** (`MordecaiHUDConstants`, AC-052.8):
  - Green: ratio ≥ 0.66.
  - Yellow: ratio ≥ 0.33.
  - Red: ratio > 0.
  - Exhausted: ratio ≤ 0, or MaxStamina ≤ 0.
- **Enum:** `EMordecaiStaminaTier {Green, Yellow, Red, Exhausted}`.
- **States:** the four tiers, color-switched at once with no animation. Stamina can go negative to the -50 floor (N30), and the bar then reads empty.

### 4. UMordecaiPostureBarWidget (`UI/MordecaiPostureBarWidget.*`)
- **Tree:** a CanvasPanel holding `PostureProgressBar`.
- **Colors** (EditAnywhere): `NormalPostureColor` (0.2,0.6,1.0) and `BrokenPostureColor` (1.0,0.2,0.2).
- **Broken state:** listens for `Mordecai.State.PostureBroken` (NewOrRemoved) and swaps the color while the tag is present.
- **Shows:** Posture/MaxPosture.

### 5. UMordecaiSpellPointsBarWidget (`UI/MordecaiSpellPointsBarWidget.*`)
- **Tree:** a CanvasPanel holding:
  - `SpellPointsProgressBar`, fill (0.4,0.2,0.8) ("Purple for spell points").
  - `SpellPointsText`, a UTextBlock anchored (0,0,1,1) with alignment (0.5,0.5). With stretched anchors this does not center it, so the text renders left-aligned (seen in the screenshots). It is White, font size 10.
- **Text format:** `"%d / %d"`, floored and ≥ 0, e.g. "10 / 10" or "7 / 10".
- **Defaults:** cached values 10/10 (MaxSpellPoints is 10; design question N6).

### 6. UMordecaiSpellCooldownWidget ×2 (`UI/MordecaiSpellCooldownWidget.*`; assets `WBP_SpellCooldownA` and `WBP_SpellCooldownB`)
- **Tree:** a CanvasPanel holding:
  - `CooldownProgressBar` (anchors (0,0,1,0.7)), fill (0.3,0.6,1.0) "Light blue".
  - `CooldownText` (anchors (0,0.7,1,1)), White, size 9, centered.
- **Behavior:**
  - Percent = elapsed/total (1 = ready). Text is `"%.1f"` seconds remaining, empty when ready.
  - NativeTick updates it while on cooldown. It reads the real duration from the active GE that carries the tag.
- **Ready state:** a full bar and empty text.
- **There is no slot label (no key glyph, no spell name or icon).** Never bound in game, so it always shows "ready".

### 7. UMordecaiStatusEffectBarWidget and UMordecaiStatusEffectIndicatorWidget (`UI/MordecaiStatusEffectBarWidget.*`, `UI/MordecaiStatusEffectIndicatorWidget.*`; assets `WBP_StatusEffectBar` and `WBP_StatusEffectIndicator`)
- **Bar.** Root is a `UHorizontalBox` named `IndicatorContainer`. It holds at most `MaxVisibleIndicators = 8`, and a 9th is dropped silently.
- **Buff list:** `Mordecai.Status.Focused`, `Blessed`, `StoneSkin`, `FireWard` and `Restoration`. Every other `Mordecai.Status.*` tag is a debuff, and `Mordecai.Status.Category.*` tags are skipped.
- **Tints:** buff (0,1,0,1), debuff (1,0,0,1).
- **Indicator tree:** a HorizontalBox holding:
  - `StatusNameText`: White, size 10, padding (2,0).
  - `DurationText`: (0.8,0.8,0.8), size 9, padding (2,0), `"%.1f"`.
- **The tint is never drawn.** `TintImage` (UImage) is BindWidgetOptional and the fallback does not build it.
- **The duration is static.** It is set once from the GE's total duration, and nothing ticks it.
- **Name bug (N16).** The bar registers on the parent tag `Mordecai.Status`, so the indicator shows the last segment of that parent: "Status". The screenshot shows "Status 10.0" bottom-left for Stone Skin, and only one indicator can ever exist. Fix planned in US-091.

### 8. UMordecaiKillCounterWidget (`UI/MordecaiKillCounterWidget.*`; asset `WBP_KillCounter`)
- **Tree:** a CanvasPanel holding `KillCountText`, White, size 14.
- **Format:** `"Kills: %d"`.
- **Behavior:** it increments on `Mordecai.Event.EnemyKill` tag-count changes and resets on `Mordecai.Event.PlayerDeath`.
- **N1:** nothing grants that tag, so it stays at "Kills: 0". The fix is the second half of US-086.

### 9. UMordecaiComboCounterWidget (`UI/MordecaiComboCounterWidget.*`; asset `WBP_ComboCounter`)
- **Tree:** a CanvasPanel holding `ComboText`, Gold (1,0.84,0), size 24, centered.
- **Format:** `"Hit %d"` (index+1), empty for index < 0.
- **Visibility:** HitTestInvisible while `Mordecai.State.ComboWindow` is present, Collapsed otherwise. It starts hidden.
- **Never shows text.** `SetComboIndex` has no callers, so the text is always empty, and combos are not wired anyway (N24).

### 10. UMordecaiCombatFeedbackWidget (`UI/MordecaiCombatFeedbackWidget.*`; asset `WBP_CombatFeedback`)
- **Tree:** a CanvasPanel holding `FeedbackText`, size 20, centered.
- **Exact strings by tag:**

| Tag | Text | Color |
|---|---|---|
| `Mordecai.State.PerfectDodge` | "Perfect Dodge!" | Gold |
| `Mordecai.State.PerfectBlock` | "Perfect Block!" | Gold |
| `Mordecai.State.Parried` | "Parried!" | Gold |
| `Mordecai.State.ParryWhiff` | "Parry Failed!" | Red |
| `Mordecai.State.PostureBroken` | "Posture Broken!" | Gold |

- **Color constants** (`MordecaiFeedbackColors`): Gold (1,0.84,0,1), Red (1,0.2,0.2,1), White (1,1,1,1).
- **Timing:** shown for `FeedbackDisplayDuration = 1.0` s, then Collapsed. There is no fade, despite the comment.

### 11. UMordecaiPickupPromptWidget (`UI/MordecaiPickupPromptWidget.*`)
- **Tree:** a VerticalBox holding:
  - `PromptText`: White, centered, default 24 Bold. It reads "[F] Pick up {0}" and is collapsed when nothing is focused.
  - `InventoryHintText`: (0.8,0.8,0.8,0.9), centered, always "[I] Inventory".
- **Item names are identification-aware**, for example "Unidentified Purple Amulet".
- **Static keyboard glyphs.** Device-aware gamepad glyphs are "out of scope for the slice" (US-079 note).
- **"[I] Inventory" stays on screen** during the pause menu and the death screen.

### 12. UMordecaiEquippedWeaponWidget (`UI/MordecaiEquippedWeaponWidget.*`)
- **Creation:** `AMordecaiCharacter::InitializeWeaponLoadout` creates it and calls `AddToViewport(50)`.
- **Data:** it binds `OnWeaponChanged` (MainHand/TwoHand) and caches `DisplayedWeaponName` (the weapon's DisplayName, or "Unarmed") and `DisplayedWeaponType`.
- **No widget tree. It draws nothing (N3).** The comment expects bottom-center per AC-078.11.
- **Weapons in the loadout:** Longsword, Dagger, Greatsword, Spear, Mace. The `EMordecaiWeaponType` enum also has Shortsword, Axe, Quarterstaff, Unarmed, Longbow, Shortbow, Crossbow, Throwable and Wand.
- **Fix:** US-091.

### 13. UMordecaiEnemyHealthBarWidget (`UI/MordecaiEnemyHealthBarWidget.*`; asset `WBP_EnemyHealthBar`)
- **Tree:** a CanvasPanel holding `HealthProgressBar`, fill `FLinearColor::Red`.
- **Placement:** hosted by a `UWidgetComponent` created in `AMordecaiEnemyCharacter::CreateHealthBarWidget()`:
  - `EWidgetSpace::Screen`, DrawSize 150×20, relative location Z +120.
  - Widget class `WBP_EnemyHealthBar`, set on `BP_MordecaiEnemy_Frontliner` by script.
- **This is the red bar visible over enemies today.**
- **Observed:** it may not redraw after `ResetForArena`; the TestDummy bar kept showing about 92% after a reset.

### 14. UMordecaiEnemyIndicatorWidget and UMordecaiEnemyIndicatorComponent (`UI/MordecaiEnemyIndicator*`)
- **Widget.** It implements Lyra's `IIndicatorWidgetInterface`. Its tree is a VerticalBox holding:
  - `NameText` (centered). It uses `EnemyDisplayName`, or else the actor's internal name `Enemy->GetName()`.
  - `HealthProgressBar` (fill).
  - `StatusIconContainer` (an HBox of 16×16 `UImage`s).
- **Icons:** they come from `StatusIconMap`, which is empty. A missing texture leaves a plain UImage.
- **Health colors:**
  - > 0.5: HealthColorGreen (0,0.8,0,1).
  - 0.25–0.5: HealthColorYellow (1,0.85,0,1).
  - < 0.25: HealthColorRed (0.9,0.1,0.1,1).
- **Statuses watched:** Burning, Bleeding, Poisoned, Frostbitten, Shocked, Weakened, Brittle, Silenced, Rooted, Blinded, Fear, Cursed, Exposed, Corroded, Drenched and Focused.
- **Component settings:**
  - `IndicatorHeightOffset` 120, `IndicatorVisibilityRange` 2000.
  - The visibility check runs every 0.25 s.
  - Descriptor alignment is HAlign_Center / VAlign_Bottom.
- **Registration:** it registers on damage or aggro and unregisters on death.
- **Never on screen (N17).** Nothing calls `NotifyAggroActivated`, there is no Lyra indicator manager or `UIndicatorLayer` in the arena, and `IndicatorWidgetClass` is unset. Fix planned in US-092.

### 15. UMordecaiDamagePopComponent (`UI/MordecaiDamagePopComponent.*`) — floating damage numbers, logic only
- **Base:** it extends `ULyraNumberPopComponent` and lives on the player controller.
- **Config:**
  - `PopLifespanSec` 1.5, `PopRiseSpeed` 100 cm/s.
  - `PopScale` 1.0, `CriticalHitScale` 1.5.
  - `PopScatterRadius` 30 cm, `MaxConcurrentPops` 20 (the oldest is recycled).
  - `HealingColor` (0.2,1.0,0.2), shown with a "+" prefix per the header comment.
- **`DamageTypeColors` map:**

| Damage type | Color | Code comment |
|---|---|---|
| Physical, Slash, Pierce, Blunt | (1,1,1) | White |
| Fire | (1,0.5,0) | "Orange" |
| Frost | (0.3,0.8,1.0) | "Cyan" |
| Lightning | (1,1,0.3) | "Yellow" |
| Poison | (0.3,0.9,0.3) | "Green" |
| Arcane | (0.7,0.3,1.0) | "Purple" |
| Radiant | (1,0.85,0) | "Gold (Holy)" |
| Shadow | (0.6,0.1,0.1) | "Dark Red" |
| Corrosive, and the default for untagged | (0.7,0.7,0.7) | Gray |

- **It renders nothing.** It only appends `FMordecaiDamagePopEntry` records. The Lyra base `AddNumberPop` is an empty virtual, and the Mordecai override does not spawn visuals.

### 16. UMordecaiPauseMenuWidget, Subsystem, Types and TabButton (`UI/MordecaiPauseMenu*`, `UI/MordecaiTabButton.*`)
- **Base and input.** It is a `ULyraActivatableWidget` (CommonUI). `InputConfig = ELyraWidgetInputMode::Menu`: game input is blocked and the cursor is shown. `bIsBackHandler = true`.
- **Pushing.** `UMordecaiPauseMenuSubsystem` (a GameInstance subsystem) pushes it onto **`UI.Layer.Menu`** with `UPrimaryGameLayout::PushWidgetToLayerStack`. It pauses the game (`SetGamePaused`) when `bCanPauseGame` is true (default true). Multiplayer pause is `TODO(DECISION)` (AC-069.13).
- **Opening and closing:**
  - Escape or gamepad Start (`Gamepad_Special_Right`) toggles it.
  - I opens it straight to Inventory (`TogglePauseMenuToTab("inventory")`).
  - The CommonUI back action closes it: Escape or `Gamepad_FaceButton_Right` (DT_UniversalActions DefaultBack).
  - UI actions:
    - `UI.Action.Mordecai.PauseMenu.PreviousTab` (LB) and `.NextTab` (RB) cycle tabs with wrap.
    - `.Inventory` (I) closes it on the key-up of a press made inside the menu.
  - The last active tab is remembered across closes. CommonUI pools the widget, so the tab content is rebound on every `NativeOnActivated`.
- **Tabs** (`FMordecaiPauseMenuTabModel::InitDefaults`), in order, as tab id / label: `character` "Character", `skills` "Skills", `feats` "Feats", `inventory` "Inventory", `settings` "Settings". The default active tab is character.
- **Real content is registered for Character and Inventory only.** Skills, Feats and Settings use `UMordecaiPauseMenuPlaceholderWidget`.
- **Fallback layout** (`EnsureDefaultLayout`):
  - Root CanvasPanel.
  - `BackgroundDim`: a UBorder, brush (0,0,0,0.65), full screen.
  - `MenuColumn`: a VerticalBox, centered (anchors 0.5, alignment 0.5), fixed **1000×640** design px. It holds:
    - `TitleText` "Paused": centered, bottom padding 12, default 24 Bold, white.
    - `TabBar`: an HBox, centered, bottom padding 12.
    - `TabContentBorder`: a UBorder, brush (0.05,0.05,0.08,0.9), padding 16, Fill.
- **Tab buttons** (`UMordecaiTabButton`, a `UButton` with an FName id and `OnClickedWithId`):
  - Background tint: active (0.25,0.35,0.55,1), inactive (0.12,0.12,0.15,1).
  - Label: a UTextBlock, White when active, (0.7,0.7,0.7,1) when inactive.
  - Slot padding (4,0).
  - The tab bar is cleared and rebuilt on every change. There are no hover, pressed or focus visuals beyond the engine default.
- **Not shown:** an action bar or input hints. `bDisplayInActionBar` is false.

### 17. UMordecaiPauseMenuPlaceholderWidget
- A CanvasPanel holding `PlaceholderText` "Coming Soon" (auto-size, default 24 Bold, top-left of the content area).
- Shown on the Skills, Feats and Settings tabs.

### 18. UMordecaiCharacterSheetWidget and FMordecaiCharacterSheetData (`UI/MordecaiCharacterSheet*`) — the Character tab
- **No widget tree, so the tab is blank (N4).** Only a data model exists, bound live to the ASC.
- **Model contents:**
  - **Group labels:** "Physical", "Resilience", "Magical". Indices 0–2 are Physical, 3–5 Resilience, 6–8 Magical.
  - **9 primary attributes** (code / name): STR "Strength", DEX "Dexterity", END "Endurance", CON "Constitution", RES "Resistance", DIS "Discipline", INT "Intelligence", WIS "Wisdom", CHA "Charisma". Each has a BaseValue and an EffectiveMod (diminishing returns).
  - **5 primary-derived stats:** "Physical Damage", "Attack Speed", "Affliction Resist", "Magic Damage", "Cast Speed".
  - **9 secondary-derived stats:** "Armor Penetration", "Physical Crit Chance", "Stamina Regen", "Health Regen", "Affliction Recovery", "Posture Recovery", "Spell Points Regen", "Resistance Penetration", "Magic Crit Chance".
  - **4 resources:** "Health", "Stamina", "Spell Points", "Posture".
- **Formats:**
  - Placeholder "--".
  - Percent bonus: `(V-1)*100` as `"%s%.1f%%"` (e.g. "+15.0%").
  - Effective-mod bonus: `"+9.4%"` style.
  - Resource: `"%d / %d"`.
- **Planned:** US-093 (AC-093.1) builds the tree.

### 19. UMordecaiSkillTreeWidget (`UI/MordecaiSkillTreeWidget.*`) — the Skills tab, API only, not registered (N5)
- **Rows** (`FMordecaiSkillTreeRow`): SkillName, DisplayName (the FName as text), CurrentRank, `MaxRank = 20`, Category, bCanAllocate.
- **Categories** (`EMordecaiSkillCategory`): Weapon, DefenseArmor, MovementExploration, UtilityInteraction, Magic.
- **Milestones:** ranks {1, 5, 10, 15, 20}. Each `FMordecaiSkillTreeMilestoneRow` has Rank, Description, bReached, bAbilityGranted and bHasDescription. The fallback description is "No data available.".
- **Allocation API:** available skill points, `CanAllocateSkillPoint` and `RequestAllocateSkillPoint`.
- **No visuals.** US-093 expects "Weapon" and "rank / 20" rows.

### 20. UMordecaiFeatDisplayWidget (`UI/MordecaiFeatDisplayWidget.*`) — the Feats tab, API only, not registered (N5)
- **Unlocked rows:** name, tier, description and applied record.
- **Locked rows:** the description is replaced by "???" and per-condition progress is shown.
  - Condition types: StatThreshold, EventCount, TagPresent, SkillRank.
  - Each condition has current, threshold and met fields.
- **Summary:** TotalUnlocked and the Common/Rare/Legendary counts.
- **Tier colors** (`TierColors`, EditDefaultsOnly): Common (0.8,0.8,0.8) "Gray", Rare (0.3,0.5,1.0) "Blue", Legendary (1.0,0.75,0.0) "Gold".
- **No visuals.** US-093 expects a "Feats: N unlocked" header.

### 21. UMordecaiInventoryWidget (`UI/MordecaiInventoryWidget.*`) — the Inventory tab
- **Layout** (`BuildDefaultLayout`): root HorizontalBox holding two columns.
  - **ListColumn** (VBox, Fill, right padding 12):
    - `FilterBar`: an HBox, bottom padding 8, with 8 `UMordecaiTabButton`s (padding (2,0)): "All", "Weapons", "Armor", "Trinkets", "Consumables", "Materials", "Quest", "Magical". Active and inactive tints match the tabs.
    - `ItemListBox`: a ScrollBox, Fill.
    - `ListPlaceholder`: (0.6,0.6,0.6).
  - **LedgerColumn** (VBox, Automatic):
    - `LedgerHeader` "Stored Resources", bottom padding 8.
    - `LedgerBox`.
    - `LedgerPlaceholder` (0.6,0.6,0.6).
- **Filter mapping:** Materials covers Material + TownResource; Magical covers MagicalItem + UpgradeKey; Quest covers QuestItem.
- **Row:** an HBox holding a name TextBlock (Fill, VAlign center, colored by rarity) and an "Identify" button (left padding 8), shown for unidentified items only.
- **Row string composition:**
  - Prefix `"? "` when unidentified.
  - Suffix `"  x%d"` when stackable and quantity > 1.
  - Suffix `"  [equipped]"` when equipped.
- **Ledger rows:** `"{0}  x{1}"` (e.g. "Iron Ore  x1"), sorted alphabetically.
- **Placeholders:**
  - List: "--" (unbound), "Inventory is empty.", "No items match this filter."
  - Ledger: "--", "No resources stored."
- **Rarity colors** (static constexpr):

| Rarity | Color |
|---|---|
| Common | (0.55,0.55,0.55) |
| Green | (0.10,0.80,0.10) |
| Blue | (0.15,0.40,1.00) |
| Purple | (0.60,0.20,0.90) |
| Red | (0.90,0.10,0.10) |
| Gold | (1.00,0.84,0.15) |

  The enum comment maps them to Common(Gray) < Green(Uncommon) < Blue(Rare) < Purple(Epic) < Red(Legendary) < Gold(Mythic).
- **Cached but not displayed:** Description, Icon (`TSoftObjectPtr<UTexture2D>`) and Type.
- **Out of scope:** drop and equip actions ("read + identify only").
- **Known defect:** the filter bar is wider than the content area and draws over "Stored Resources" (screenshot `20261030-155532-us079/bridge_03.png`).

### 22. UMordecaiDeathScreenWidget and UMordecaiDeathScreenComponent (`UI/MordecaiDeathScreen*`)
- **Base and input.** It is a `ULyraActivatableWidget` with `InputConfig = GameAndMenu` and `GameMouseCaptureMode = CapturePermanently_IncludingInitialMouseDown`.
- **Pushing.** The component, on the player controller, listens for `Mordecai.State.Dead` and pushes the widget onto **`UI.Layer.Game`**. It removes it when the tag goes.
- **Tree:**
  - Root CanvasPanel, HitTestInvisible.
  - `BackgroundDim`: (0,0,0,0.5), full screen.
  - `MessageColumn`: a VBox, centered, auto-size, line padding (0,4). It holds:
    - `DeathText` "You Died": size 48, (0.8,0.1,0.1).
    - `CountdownText`: size 32, White. The ceiling integer of the remaining time, empty when ≤ 0.
    - `KillCountText`: size 20, White, "Kills: %d".
- **Timing:** the countdown is `RespawnDelaySeconds` (3 s observed) and holds while the world is paused. No animation, and no button: respawn is automatic.
- **Remote clients see no death screen** (non-replicated tag).

### 23. UMordecaiLoadingScreenWidget (`UI/MordecaiLoadingScreenWidget.*`)
- **Configured** as `CommonLoadingScreenSettings.LoadingScreenWidget=/Script/LyraGame.MordecaiLoadingScreenWidget` (`Config/DefaultGame.ini`). It builds its tree in `Initialize()`.
- **Tree:**
  - An opaque full-screen `Background` UBorder, (0.010,0.010,0.012,1).
  - A centered VBox (line padding (0,6)) holding:
    - `TitleText` "Project Mordecai": size 56, typeface "Bold", LetterSpacing 150, `ToUpper`, so it reads "PROJECT MORDECAI". Color (0.86,0.80,0.68).
    - `StatusText` "Loading...": size 20, "Regular", (0.55,0.55,0.55).
    - A `UThrobber`, tinted with the status color, top padding 12.
- **Art direction:** a `TODO(DECISION)` in code: "the loading screen's art (logo, key art, palette) has no direction yet".
- **Playtest hook:** it logs "MordecaiLoadingScreen: shown", and `playtest_boot.py` checks for that line.

### 24. World-space and other UI outside `UI/`
- **Item pickup name label** (`AMordecaiItemPickup::NameLabel`):
  - A `UTextRenderComponent` at Z +70, center aligned, WorldSize 22, `FColor::White`.
  - A billboard turned to the local camera every frame. The default rotation is (50,180,0) for the diorama camera (pitch -50).
  - It renders light grey and is faint on the light floor, from the engine's lit text material.
  - Text is identification-aware; seen: "Health Potion", "Iron Ore", "Town Stone", "Rusty Sword", "Upgrade Key", "Unidentified Purple Amulet".
- **Facing arrow** (`AMordecaiCharacter::FacingArrowComponent`):
  - `UArrowComponent`, `ArrowColor = FColor::Green`, ArrowSize 1.5, Z +50.
  - Arrow components are hidden in game by default, so it is probably not visible (N12, suspected).
- **On-screen engine messages:** shown to players at about (40,107) of the 720p frame, in yellow, plus a grey "'DisableAllScreenMessages' to suppress" line. Seen:
  - "P2PConnectionTimeout has moved from OnlineSubsystemSteam to SocketSubsystemSteamIP, previous location is depreceated"
  - "PassLoadingScreenWindowBackToGame failed.  No Window"
  - Earlier: "MORDECAI: Programmatic IMC created and input bound successfully (including 4 spell actions).", "MORDECAI: Weapon loadout initialized — 5 weapons available", "MORDECAI: SetupPlayerInputComponent called…"
  - Tracked as issue #29 and US-091 AC-091.5.

### 25. Front end (Lyra, not Mordecai)
- `GameDefaultMap=/Game/System/FrontEnd/Maps/L_LyraFrontEnd`, experience `B_LyraFrontEnd_Experience`.
- **Screenshot** `Saved/Playtests/20261030-155056-boot/engine_01.png`:
  - The Lyra lobby background.
  - A blue-glass center panel with the "LYRA" logo.
  - Buttons "PLAY LYRA", "OPTIONS", "CREDITS", "SHOW REPLAY", "QUIT LYRA".
  - A UE logo at the bottom.
  - Bottom-left: "Player 1: <machine/online id>", "NULL:…", "UE5-CL-0 (Development)".
  - Bottom-right: "Player 2: Not logged in" and "ADD PLAYER".
- **Mordecai's only front-end content** is the playlist `/MordecaiCore/System/Playlists/DA_Mordecai_Arena` (`LyraUserFacingExperienceDefinition`):
  - TileTitle "Mordecai Arena", TileSubTitle "Dev test arena".
  - TileDescription "Project Mordecai diorama arena (DevTestMap)".
  - bShowInFrontEnd True. No TileIcon and no LoadingScreenWidget.

---

## Current palette
Every color literal found. Values are `FLinearColor` (linear) unless noted. **UMG and Slate treat FLinearColor as linear and display it sRGB-encoded,** so the on-screen hex (column 3) is much lighter than a designer reading the numbers as sRGB would expect (column 4). The screenshots confirm the sRGB column: the loading background shows about #1B1A1F, and "You Died" shows about #E75959.

| Literal | Where | Displayed sRGB | If read as sRGB ×255 | Meaning |
|---|---|---|---|---|
| (0,1,0) `FLinearColor::Green` | Health fill; stamina Green tier | #00FF00 | #00FF00 | player health; stamina ≥66% |
| (1,1,0) | stamina Yellow tier | #FFFF00 | #FFFF00 | stamina 33–66% |
| (1,0,0) | stamina Red tier; `FLinearColor::Red` enemy health bar | #FF0000 | #FF0000 | stamina >0–33%; enemy HP |
| (0.5,0,0) | stamina Exhausted | #BC0000 | #800000 | stamina ≤0 |
| (0.2,0.6,1.0) | posture normal | #7CCBFF | #3399FF | posture |
| (1.0,0.2,0.2) | posture broken; feedback Red ("Parry Failed!") | #FF7C7C | #FF3333 | broken / negative |
| (0.4,0.2,0.8) | SP fill ("Purple") | #AA7CE7 | #6633CC | spell points |
| (0.3,0.6,1.0) | cooldown fill ("Light blue") | #95CBFF | #4C99FF | spell cooldown |
| (0,1,0,1) / (1,0,0,1) | status buff / debuff tint (never drawn) | #00FF00 / #FF0000 | same | buff / debuff |
| (0.8,0.8,0.8) | status duration text; pickup hint (α 0.9); feat Common tier | #E7E7E7 | #CCCCCC | secondary text |
| (1,0.84,0) | combo text and feedback Gold | #FFEC00 | #FFD600 | positive combat feedback |
| (0,0.8,0) / (1,0.85,0) / (0.9,0.1,0.1) | enemy indicator health >50% / 25–50% / <25% | #00E700 / #FFED00 / #F35959 | #00CC00 / #FFD900 / #E61A1A | enemy health state |
| (1,1,1) White | Physical damage pops; most text | #FFFFFF | #FFFFFF | default |
| (1,0.5,0) | Fire pop ("Orange") | #FFBC00 | #FF8000 | damage type |
| (0.3,0.8,1.0) | Frost pop ("Cyan") | #95E7FF | #4CCCFF | damage type |
| (1,1,0.3) | Lightning pop | #FFFF95 | #FFFF4C | damage type |
| (0.3,0.9,0.3) | Poison pop | #95F395 | #4CE64C | damage type |
| (0.7,0.3,1.0) | Arcane pop | #DA95FF | #B24CFF | damage type |
| (1,0.85,0) | Radiant pop ("Gold (Holy)") | #FFED00 | #FFD900 | damage type |
| (0.6,0.1,0.1) | Shadow pop ("Dark Red") | #CB5959 | #991A1A | damage type |
| (0.7,0.7,0.7) | Corrosive and default pop; inactive tab/filter label | #DADADA | #B2B2B2 | grey |
| (0.2,1.0,0.2) | healing pop | #7CFF7C | #33FF33 | healing |
| (0,0,0,0.65) | pause menu BackgroundDim | black @65% | — | modal dim |
| (0,0,0,0.5) | death screen BackgroundDim | black @50% | — | overlay dim |
| (0.05,0.05,0.08,0.9) | pause TabContentBorder | #3F3F50 @90% | #0D0D14 | panel |
| (0.25,0.35,0.55,1) | active tab and filter button tint | #89A0C4 | #40598C | selected |
| (0.12,0.12,0.15,1) | inactive tab and filter button tint | #61616C | #1F1F26 | unselected |
| (0.6,0.6,0.6) | inventory list/ledger placeholder | #CBCBCB | #999999 | empty state |
| (0.55,0.55,0.55) | rarity Common; loading status text and throbber | #C4C4C4 | #8C8C8C | common / muted |
| (0.10,0.80,0.10) | rarity Green | #59E759 | #1ACC1A | uncommon |
| (0.15,0.40,1.00) | rarity Blue | #6CAAFF | #2666FF | rare |
| (0.60,0.20,0.90) | rarity Purple | #CB7CF3 | #9933E6 | epic |
| (0.90,0.10,0.10) | rarity Red | #F35959 | #E61A1A | legendary |
| (1.00,0.84,0.15) | rarity Gold | #FFEC6C | #FFD626 | mythic |
| (0.3,0.5,1.0) / (1.0,0.75,0.0) | feat Rare / Legendary tier | #95BCFF / #FFE100 | #4C80FF / #FFBF00 | feat tiers |
| (0.8,0.1,0.1) | "You Died" | #E75959 | #CC1A1A | death |
| (0.010,0.010,0.012) | loading background | #19191D | #030303 | near-black |
| (0.86,0.80,0.68) | loading title | #EFE7D7 | #DBCCAD | warm bone/parchment |
| `FColor::White` (sRGB) | pickup TextRender labels | #FFFFFF, rendered lit, so light grey | — | world label |
| `FColor::Green` (sRGB) | facing arrow | #00FF00 | — | facing |

- **Default engine widget styles** (no literal in code): `UProgressBar` track light grey, `UButton` dark box with a light outline.
- **Lyra content colors** on the front end: "blue glass" and Orbitron (issue #62). They come from Lyra assets, not Mordecai code.
- **World object colors** (materials, not UI), as seen in screenshots: Frontliner enemies dark red cylinders (`MI_Enemy_Frontliner`), TestAttacker yellow, TestDummy pale blue-grey. The arena is light grey.

## Current typography
- **Face:** only the engine default, Roboto (`UWidget::GetDefaultFontName()`, `/Engine/EngineFonts/Roboto`).
  - `UTextBlock` defaults to **Roboto "Bold", size 24** (engine `TextBlock.cpp:35`).
  - Code changes only `Size`, except the loading screen, which also sets the typeface and letter spacing.
  - No Mordecai font asset or `CommonTextStyle` exists.
- **Sizes** (Slate font size at design scale 1.0; 720p renders ×0.666):

| Size | Element |
|---|---|
| 56 | Loading title, Bold, LetterSpacing 150, ToUpper |
| 48 | "You Died" |
| 32 | Death countdown digit |
| 24 (default) | Combo "Hit N"; "Paused"; tab labels; filter labels; inventory rows; "Stored Resources"; ledger rows; "Identify"; "Coming Soon"; pickup prompt; "[I] Inventory"; enemy indicator name |
| 20 | Combat feedback; death "Kills: N"; loading "Loading..." (Regular) |
| 14 | HUD "Kills: N" |
| 10 | SP "x / y"; status name |
| 9 | Cooldown seconds; status duration |

- **World text:** pickup labels use TextRender `WorldSize 22` (world units) in the engine's default TextRender font.
- **Lyra typography still on the front end and dialogs:**
  - Orbitron (`Config/DefaultGame.ini` stages `UI/Foundation/Fonts/Orbitron/Raw`).
  - CommonUI editor templates `TextStyle-Regular` and `ButtonStyle-Primary-M`.
  - `CommonUIRichTextData`.
  - Dialogs `W_ConfirmationDefault` and `W_ConfirmationError`.

## Current layout
**HUD at 1920×1080 design px (ScaleToFit).**
- **Top-left stack, x = 20:**
  - Health 250×20 at y 20.
  - Stamina 250×20 at y 48.
  - Posture 250×16 at y 76.
  - SP 250×16 at y 100, with "x / y" left-aligned inside.
  - Two cooldown bars, 120×16 each, at (20,124) and (150,124). Each has a 70% bar over a 30% text strip.
- **Top-right:** "Kills: N", 150×24 at (W-170, 20).
- **Bottom-center, measured bottom-up:**
  - PickupPrompt 420×56, bottom edge 30 px up ("[F] Pick up …" over "[I] Inventory").
  - CombatFeedback 300×30, bottom edge 80 px up. It overlaps the prompt's top by about 6 px.
  - ComboCounter 200×40, bottom edge 120 px up.
- **Bottom-left:** StatusEffectBar 400×40, from 60 px up to 20 px up. Indicators flow left to right; at most 8.
- **Unplaced:** the weapon readout (`AddToViewport(50)`, empty).
- **US-088** plans a row of 4 cooldowns under the SP bar.
- **US-091** plans the weapon name bottom-center or bottom-right, not overlapping the pickup prompt.

**Layering.**
- The combat HUD (Z 0) and the weapon widget (Z 50) are viewport-overlay widgets, above Lyra's `W_OverallUILayout` (Z 1000 on the player screen).
- The layout's CommonUI layers are `UI.Layer.Game` (death screen), `UI.Layer.GameMenu` (unused), `UI.Layer.Menu` (pause menu) and `UI.Layer.Modal` (Lyra dialogs).
- So the menus and the death dim render under the HUD.

**World-space.**
- Old enemy health bar: screen-space WidgetComponent, 150×20, at Z +120 above the enemy.
- Enemy indicator (inactive): anchored bottom-center at Z +120, visible within 2000 units.
- Pickup labels: Z +70.
- Facing arrow: Z +50.

**Pause menu structure.**
- A full-screen 65% black dim, then a centered 1000×640 column:
  - Title "Paused".
  - Tab bar: [Character][Skills][Feats][Inventory][Settings]. The active tab has a blue-grey tint.
  - Content panel, padding 16.
- **Navigation:**
  - Mouse click on the tabs; LB/RB cycle with wrap.
  - Escape, gamepad B, or Start toggles. I closes, and I from gameplay opens Inventory.
  - The game is paused while open.
- **Inventory:** two columns, list (left) and ledger (right).
  - Left: the 8-filter row, then a scroll list of rarity-colored names, each with an "Identify" button if unidentified.
  - Right: "Stored Resources" with "Name  xN" lines.
- **Character, Skills, Feats:** no visual layout (see the Gaps section).

**Death screen.** A 50% black dim (HitTestInvisible), then a centered "You Died" / countdown / "Kills: N". The HUD stays visible over it.

**Loading screen.** Near-black, centered "PROJECT MORDECAI" / "Loading..." / a throbber. It is held for about 2 s after the experience loads (3.02–3.53 s visible in the boots).

## Gaps
**Lyra-branded or placeholder UI still shown to players.**
- **Front end is entirely Lyra.** It is issues #57–#69, all `needs-decision`:
  - #57 LYRA logo, "PLAY LYRA" / "QUIT LYRA".
  - #58 lobby background.
  - #59 Lyra shooter and party modes playable.
  - #60 PLAY leads to Lyra's multiplayer and replay screens.
  - #61 machine name, online ID, engine build stamp and ADD PLAYER.
  - #62 Orbitron and blue-glass style on the front end and dialogs.
  - #63 credits.
  - #64 translated menu text.
  - #65 front-end music and sounds.
  - #66 exe version resource and icon.
  - #67 `LyraGame.exe`.
  - #68 Lyra fallback experience, HUD and escape menu.
  - #69 module, config and cvar names.
  - Also: `[/Script/EngineSettings.GeneralProjectSettings] ProjectName=Lyra`, Description "Sample starter game for Unreal Engine 5".
- **Lyra-owned input and glyph data:**
  - `InputData=/Game/UI/B_CommonInputData`.
  - Controller data `CommonInput_KeyboardMouse`, `CommonInput_Gamepad_XSX` and `CommonInput_Gamepad_PS5`.
  - `DefaultGamepadName=XSX`, and Lyra's `DT_UniversalActions`.
  - Lyra options/settings and confirmation dialogs.
- **On-screen engine and debug messages** in yellow (issue #29, US-091 AC-091.5).
- **Loading screen** is a placeholder: the name in Roboto on near-black, with the art `TODO(DECISION)`.

**Mordecai UI that exists in code but shows nothing or the wrong thing.**
- N3: the weapon readout draws nothing.
- N4: the Character tab is blank.
- N5: Skills and Feats show "Coming Soon". Settings also shows "Coming Soon", which AC-069.3 accepts.
- N8: 2 cooldown widgets, not 4; never bound, so always "ready"; no key or spell labels.
- N16: the status indicator reads "Status", its duration never counts down, and its buff/debuff tint image is never built.
- N1: the kill counter never increments.
- The combo counter never sets text (`SetComboIndex` has no callers).
- N17: enemy indicators are never on screen. Enemies show only the plain red screen-space bar, with no name.
- Damage and healing numbers are never drawn (logic only).
- N12: the facing arrow is probably hidden in game.
- The inventory filter bar overflows into the ledger header.
- The pickup labels are faint on a light floor.
- The HUD and "[I] Inventory" draw over the pause menu and death screen, because they are added to the viewport rather than the CommonUI layer stack.
- No gamepad glyphs: prompts hard-code "[F]" and "[I]".
- There is no action bar or input hints in menus, no item detail or description panel, no item icons, and no equip or use actions (N22).
- No hover, focus or pressed styling beyond engine defaults; no animations or transitions anywhere.

**Input actions with no UI yet.** These assets exist in `Plugins/GameFeatures/MordecaiCore/Content/Input/Actions/`, which implies future UI needs:
- `IA_Mordecai_Map` (map screen).
- `IA_Mordecai_ItemA`–`ItemD` (quick item slots).
- `IA_Mordecai_SkillA` / `SkillB` (skill slots).
- `IA_Mordecai_Throw`, `IA_Mordecai_Sneak`.
- `IA_Mordecai_Spell1`–`Spell4` (spell slots Q / E / R / One; gamepad LB / RB / none / DPad Up).

## Implementation notes for designers
1. **Everything visible is UMG built in C++** (`WidgetTree->ConstructWidget` in `NativeOnInitialized`; `Initialize()` for the loading screen). It is guarded by `if (WidgetTree && !WidgetTree->RootWidget)`.
   - A designer-built WBP that has a root widget replaces the fallback.
   - The WBP must then provide the **`BindWidgetOptional` names** exactly, or that piece silently disappears:
     - HUD: `HealthBar`, `StaminaBar`, `PostureBar`, `SpellPointsBar`, `StatusEffectBar`, `SpellCooldownA`, `SpellCooldownB`, `ComboCounter`, `CombatFeedback`, `KillCounter`, `PickupPrompt`.
     - Bars: `HealthProgressBar`, `StaminaProgressBar`, `PostureProgressBar`, `SpellPointsProgressBar` and `SpellPointsText`, `CooldownProgressBar` and `CooldownText`.
     - Status: `IndicatorContainer`, `StatusNameText`, `DurationText`, `TintImage`.
     - Text widgets: `KillCountText`, `ComboText`, `FeedbackText`, `PromptText` and `InventoryHintText`.
     - Pause menu: `TitleText`, `TabBar`, `TabContentBorder`.
     - Inventory: `FilterBar`, `ItemListBox`, `ListPlaceholder`, `LedgerHeader`, `LedgerBox`, `LedgerPlaceholder`.
     - Death screen: `DeathText`, `CountdownText`, `KillCountText`.
     - Loading screen: `TitleText`, `StatusText`.
     - Enemy indicator: `NameText`, `HealthProgressBar`, `StatusIconContainer`.
   - A WBP must be **saved and committed**, or the packaged game shows nothing. The HUD itself is loaded by path string `/MordecaiCore/UI/WBP_CombatHUD`.
2. **CommonUI widgets.** The pause menu and the death screen are `ULyraActivatableWidget`s pushed onto Lyra's `UPrimaryGameLayout`: `UI.Layer.Menu` and `UI.Layer.Game` respectively.
   - The HUD, the weapon widget and the enemy bars are plain `UUserWidget`s or WidgetComponents.
   - CommonUI **pools** activatable widgets, so data must bind in `NativeOnActivated` and unbind in `NativeOnDeactivated`.
   - Menu input mode blocks every key that has no UI action binding. A new menu key needs:
     - a native `UI.Action.*` tag;
     - a `+InputActions=` entry in `Config/DefaultInput.ini` `[/Script/CommonUI.CommonUIInputSettings]`;
     - `RegisterUIActionBinding` once, in `NativeOnInitialized`.
   - Today's UI action display names: "Back" (Escape, `Gamepad_Special_Right`), "Previous Tab" (LB), "Next Tab" (RB), "Close" (I). All use `bDisplayInActionBar` false.
3. **Colors must be authored as sRGB and converted** (`FLinearColor::FromSRGBColor` / FColor). The current literals are linear values whose comments assume sRGB ("Orange" Fire shows #FFBC00).
   - Several colors are already designer-overridable UPROPERTYs: stamina tiers, posture colors, `TierColors`, `DamageTypeColors`, `HealingColor` and `StatusIconMap`.
4. **Resolution:** design for 1920×1080 with `ScaleToFit` (0.666 at 720p). Playtests capture 1280×720 frames.
5. **Must keep for automated playtests** (`Playtest/MordecaiAgentBridgeState.cpp`):
   - **Widget naming.** The bridge reads only widgets whose native class name starts with "Mordecai", and their visible `UTextBlock` texts and `UButton`s. A button's label is its first TextBlock child, which is how `click_button("Identify")` works.
   - **Names looked up directly:** `PromptText`, `InventoryHintText`, `ItemListBox`, `LedgerBox`, `ListPlaceholder`, `LedgerPlaceholder`.
   - **Exact strings the playtests compare:** "Kills: N", "You Died", "Paused", the tab labels, "Coming Soon", "Inventory is empty.", "No resources stored.", "Identify", "[I] Inventory", "x / y" for SP, and status names such as "StoneSkin" / "Burning".
   - **Planned checks** expect a "Feats: …" header and "/ 20" rows.
   - Text must stay real `UTextBlock`s, not baked into textures, or the playtests go blind.
6. **No PIE-only paths and no editor-only state.** No content loaded only by path string without cooking (all of `/MordecaiCore` is cooked).
   - The loading screen must stay opaque, because the world renders behind it during the 2 s hold.
   - It must build its tree in `Initialize()`.
7. **Open recovery stories that touch this UI:**
   - US-086 (kill counter half).
   - US-088 (4 cooldown widgets labelled Q / E / R / 1, bound to `Mordecai.Cooldown.Spell.*`).
   - US-091 (status names, weapon name HUD, no debug text).
   - US-092 (enemy indicators: the PC gets a `ULyraIndicatorManagerComponent` and the HUD gets a `UIndicatorLayer`).
   - US-093 (Character, Skills and Feats tab trees fitting 1000×640, a scroll box for long skill lists).


## UI INVENTORY
- [HUD|implemented] **Combat HUD root (UMordecaiCombatHUDWidget / WBP_CombatHUD)**: Container for all player combat HUD elements; built in C++ (BuildDefaultLayout) because WBP_CombatHUD is an empty shell | data: Hosts 11 slots: HealthBar, StaminaBar, PostureBar, SpellPointsBar, SpellCooldownA, SpellCooldownB, KillCounter, ComboCounter, CombatFeedback, StatusEffectBar, PickupPrompt; 1920x1080 design px, ScaleToFit | states: bound to PlayerState ASC on possess (local player only); AddToViewport Z0 so it draws ABOVE pause menu / death screen dims; remote clients get none (Source/LyraGame/Mordecai/UI/MordecaiCombatHUDWidget.cpp:34-127; MordecaiPlayerController.cpp:83-133)
- [HUD|implemented] **Player health bar**: Show player Health as fraction of MaxHealth | data: Health/MaxHealth clamped 0-1; 250x20 at top-left (20,20); fill FLinearColor::Green (#00FF00); no numeric text | states: full, partial, empty (0 when MaxHealth<=0); no damage trail/animation (Source/LyraGame/Mordecai/UI/MordecaiHealthBarWidget.cpp)
- [HUD|implemented] **Player stamina bar with tier color**: Show stamina amount and tier | data: Stamina/MaxStamina; 250x20 at (20,48); tiers Green >=0.66 (0,1,0), Yellow >=0.33 (1,1,0), Red >0 (1,0,0), Exhausted <=0 (0.5,0,0 -> #BC0000) | states: Green, Yellow, Red, Exhausted (stamina can sit at -50 floor; never regenerates today, N30) (Source/LyraGame/Mordecai/UI/MordecaiStaminaBarWidget.h:50-61, .cpp:44-85)
- [HUD|implemented] **Player posture bar**: Show posture meter and posture-broken state | data: Posture/MaxPosture; 250x16 at (20,76); normal (0.2,0.6,1.0)->#7CCBFF, broken (1.0,0.2,0.2)->#FF7C7C | states: normal, broken (Mordecai.State.PostureBroken present) (Source/LyraGame/Mordecai/UI/MordecaiPostureBarWidget.*)
- [HUD|implemented] **Spell Points bar + 'x / y' text**: Show discrete spell point resource | data: SpellPoints/MaxSpellPoints fill (0.4,0.2,0.8)->#AA7CE7; text '%d / %d' e.g. '10 / 10', white size 10, renders left-aligned; 250x16 at (20,100); MaxSP 10 | states: full, partial, empty; no regen (N6) (Source/LyraGame/Mordecai/UI/MordecaiSpellPointsBarWidget.cpp)
- [HUD|implemented] **Spell cooldown indicators (2 slots A/B)**: Show remaining cooldown per spell slot | data: bar (top 70%) fill (0.3,0.6,1.0)->#95CBFF; text '%.1f' seconds (size 9, white, centered, bottom 30%); empty text when ready; 120x16 at (20,124) and (150,124); no key/spell label or icon | states: ready (full bar, no text), on cooldown (ticking fill + seconds); never bound in game so always ready (Source/LyraGame/Mordecai/UI/MordecaiSpellCooldownWidget.cpp; MordecaiCombatHUDWidget.cpp:81-92)
- [HUD|planned-story] **Four spell cooldown widgets labelled Q / E / R / 1**: One cooldown per loadout spell (Fireball, Blink, Stone Skin, Restoration) | data: cooldown per Mordecai.Cooldown.Spell.Fireball/.Blink/.StoneSkin/.Restoration; durations Fireball 2.0, Blink 4.0, Stone Skin 15.0, Restoration 10.0 s; row under SP bar | states: ready, cooling down, (missing tag -> warning) (stories/backlog/US-088-recovery-spell-cooldowns-hud.md AC-088.3/088.4)
- [HUD|implemented] **Kill counter**: Session kill count | data: 'Kills: %d', white size 14, 150x24 at top-right (W-170,20) | states: 0..N; resets on Mordecai.Event.PlayerDeath; currently never increments (N1) (Source/LyraGame/Mordecai/UI/MordecaiKillCounterWidget.cpp; stories/in-progress/US-086 AC-086.3/086.5)
- [HUD|implemented] **Combo counter ('Hit N')**: Show current melee combo hit | data: 'Hit %d' (index+1), Gold (1,0.84,0)->#FFEC00, size 24, centered, 200x40 bottom-center 120 px up | states: hidden (Collapsed) / visible while Mordecai.State.ComboWindow; text always empty today (SetComboIndex has no caller) (Source/LyraGame/Mordecai/UI/MordecaiComboCounterWidget.cpp)
- [HUD|implemented] **Combat feedback flash**: Brief text for perfect/negative defensive outcomes | data: 'Perfect Dodge!', 'Perfect Block!', 'Parried!', 'Posture Broken!' in Gold (1,0.84,0); 'Parry Failed!' in Red (1,0.2,0.2)->#FF7C7C; size 20 centered; 300x30 bottom-center 80 px up | states: hidden; shown 1.0 s (FeedbackDisplayDuration) then collapsed, no fade (Source/LyraGame/Mordecai/UI/MordecaiCombatFeedbackWidget.cpp:49-105)
- [HUD|implemented] **Status effect bar + indicators**: Show active buffs/debuffs on the player | data: row of up to 8 indicators, 400x40 bottom-left (20,-60); each: name (last tag segment, white size 10) + duration '%.1f' (0.8 grey size 9); buffs Focused/Blessed/StoneSkin/FireWard/Restoration green tint, others red tint | states: empty, 1-8 indicators; BUG N16: shows 'Status' and only one indicator; duration static; tint image never built (Source/LyraGame/Mordecai/UI/MordecaiStatusEffectBarWidget.cpp, MordecaiStatusEffectIndicatorWidget.cpp; US-091 AC-091.1/091.2)
- [HUD|implemented] **Pickup prompt '[F] Pick up <item>'**: Tell player a pickup is focused and how to collect it | data: '[F] Pick up {0}' with identification-aware item name (e.g. 'Unidentified Purple Amulet'), white, default Roboto Bold 24, centered, 420x56 bottom-center 30 px up | states: collapsed when nothing focused; visible when a pickup is in range/focused; static keyboard glyph only (Source/LyraGame/Mordecai/UI/MordecaiPickupPromptWidget.cpp)
- [HUD|implemented] **Persistent '[I] Inventory' hint**: Assist hint for opening inventory | data: '[I] Inventory', (0.8,0.8,0.8,0.9), centered under the pickup prompt | states: always visible (also over pause menu and death screen) (Source/LyraGame/Mordecai/UI/MordecaiPickupPromptWidget.cpp:25-28,105-112)
- [HUD|implemented] **Equipped weapon readout**: Show the currently equipped weapon name/type | data: DisplayedWeaponName (weapon DisplayName or 'Unarmed'), DisplayedWeaponType; loadout Longsword, Dagger, Greatsword, Spear, Mace | states: data updates on OnWeaponChanged; draws NOTHING (no widget tree, N3); AddToViewport(50) (Source/LyraGame/Mordecai/UI/MordecaiEquippedWeaponWidget.cpp; MordecaiCharacter.cpp:193-206; US-091 AC-091.3/091.4 (bottom-center or bottom-right))
- [World-space|implemented] **Floating damage / healing numbers**: Show damage dealt by type, crits and heals | data: amount; color by Mordecai.Damage.* (Physical white, Fire (1,0.5,0)->#FFBC00, Frost #95E7FF, Lightning #FFFF95, Poison #95F395, Arcane #DA95FF, Radiant #FFED00, Shadow #CB5959, Corrosive/default #DADADA); heal (0.2,1,0.2) with '+'; lifespan 1.5 s, rise 100 cm/s, crit scale 1.5, scatter 30 cm, max 20 | states: normal, critical, healing; logic only - nothing is rendered (Source/LyraGame/Mordecai/UI/MordecaiDamagePopComponent.h:105-140, .cpp:24-39)
- [World-space|implemented] **Enemy health bar (widget component)**: Enemy health above each enemy | data: Health/MaxHealth, fill FLinearColor::Red #FF0000; screen-space WidgetComponent DrawSize 150x20 at Z +120; no name | states: full..empty; may not redraw after arena reset (Source/LyraGame/Mordecai/UI/MordecaiEnemyHealthBarWidget.cpp; Enemy/MordecaiEnemyCharacter.cpp:521-544)
- [World-space|implemented] **Enemy indicator (name + color-coded health + status icons)**: Lyra IndicatorSystem marker over aggroed/damaged enemies | data: NameText (EnemyDisplayName or actor name), health bar green (0,0.8,0) >50% / yellow (1,0.85,0) 25-50% / red (0.9,0.1,0.1) <25%, 16x16 status icons for 16 statuses; Z +120, visible within 2000 | states: unregistered, registered (on damage/aggro), hidden beyond range, removed on death; never appears today (N17) (Source/LyraGame/Mordecai/UI/MordecaiEnemyIndicatorWidget.cpp, MordecaiEnemyIndicatorComponent.cpp; US-092)
- [World-space|implemented] **Item pickup name label**: Name floating over ground pickups | data: identification-aware item name ('Health Potion', 'Iron Ore', 'Town Stone', 'Rusty Sword', 'Upgrade Key', 'Unidentified Purple Amulet'); TextRender WorldSize 22, FColor::White (renders light grey), Z +70, billboard | states: visible until picked up; faint on light floors (Source/LyraGame/Mordecai/Items/MordecaiItemPickup.cpp:43-52,162-186)
- [World-space|implemented] **Player facing arrow**: Show twin-stick facing direction | data: UArrowComponent, ArrowColor FColor::Green, ArrowSize 1.5, Z +50 | states: probably hidden in game (N12); facing pinned to +X by camera (known issue) (Source/LyraGame/Mordecai/MordecaiCharacter.cpp:66-71)
- [In-game menu|implemented] **Pause menu shell**: Tabbed in-game menu; pauses the game | data: title 'Paused'; tab bar Character/Skills/Feats/Inventory/Settings; content panel 1000x640 design px, padding 16, brush (0.05,0.05,0.08,0.9)->#3F3F50@90%; full-screen dim black 65% | states: closed, open (game paused, Menu input mode, cursor shown); remembers last tab; opens via Escape/Start, I opens to Inventory; closes via Escape, Gamepad B, I (key-up) (Source/LyraGame/Mordecai/UI/MordecaiPauseMenuWidget.cpp:237-292; MordecaiPauseMenuSubsystem.cpp; MordecaiPauseMenuTypes.cpp:11-22)
- [In-game menu|implemented] **Pause menu tab buttons**: Switch tabs by click or LB/RB | data: labels 'Character','Skills','Feats','Inventory','Settings'; active tint (0.25,0.35,0.55)->#89A0C4 + white label; inactive (0.12,0.12,0.15)->#61616C + (0.7,0.7,0.7) label; padding (4,0) | states: active, inactive; LB previous / RB next with wrap; no hover/focus styling beyond engine default; no action-bar hints (Source/LyraGame/Mordecai/UI/MordecaiPauseMenuWidget.cpp:294-323; MordecaiTabButton.*; Config/DefaultInput.ini:39-43)
- [In-game menu|implemented] **Character tab (character sheet)**: Show attributes, derived stats and resources | data: 9 primaries STR/DEX/END/CON/RES/DIS/INT/WIS/CHA in groups Physical/Resilience/Magical (base value + effective mod '+9.4%'); 5 primary-derived (Physical Damage, Attack Speed, Affliction Resist, Magic Damage, Cast Speed); 9 secondary-derived (Armor Penetration, Physical Crit Chance, Stamina Regen, Health Regen, Affliction Recovery, Posture Recovery, Spell Points Regen, Resistance Penetration, Magic Crit Chance) as '+15.0%'; Health/Stamina/Spell Points/Posture '%d / %d' | states: unbound ('--' placeholders), bound live; currently BLANK (no widget tree, N4) (Source/LyraGame/Mordecai/UI/MordecaiCharacterSheetTypes.cpp; MordecaiCharacterSheetWidget.cpp; US-093 AC-093.1/093.3)
- [In-game menu|implemented] **Skills tab (skill tree)**: Browse skills by category, ranks, milestones, allocate skill points | data: categories Weapon/DefenseArmor/MovementExploration/UtilityInteraction/Magic; rows SkillName, CurrentRank/20, bCanAllocate; milestone ranks 1/5/10/15/20 with Description, reached, ability granted ('No data available.' fallback); available skill points | states: tab shows 'Coming Soon' today (not registered, N5); data API only (Source/LyraGame/Mordecai/UI/MordecaiSkillTreeWidget.*; US-093 AC-093.2/093.4 ('Weapon', 'rank / 20'))
- [In-game menu|implied] **Skill point allocation control**: Spend an available skill point on a skill | data: available points, per-skill CanAllocateSkillPoint (disabled state), RequestAllocateSkillPoint | states: enabled, disabled (no points / rank 20 / unregistered skill) (Source/LyraGame/Mordecai/UI/MordecaiSkillTreeWidget.cpp:129-169)
- [In-game menu|implemented] **Feats tab (feat display)**: Show unlocked and locked feats with progress | data: unlocked: name, tier badge, description, applied record; locked: name, tier, '???' teaser, per-condition progress current/threshold (StatThreshold, EventCount, TagPresent, SkillRank); summary TotalUnlocked + Common/Rare/Legendary counts; tier colors Common (0.8)->#E7E7E7, Rare (0.3,0.5,1)->#95BCFF, Legendary (1,0.75,0)->#FFE100 | states: tab shows 'Coming Soon' today (N5); planned header 'Feats: N unlocked' (Source/LyraGame/Mordecai/UI/MordecaiFeatDisplayWidget.*; US-093 AC-093.5)
- [In-game menu|implemented] **Inventory tab - category filter bar**: Filter the flat item list | data: 8 buttons 'All','Weapons','Armor','Trinkets','Consumables','Materials','Quest','Magical'; same active/inactive tints as tabs | states: one active filter; overflows and overlaps 'Stored Resources' header (known defect) (Source/LyraGame/Mordecai/UI/MordecaiInventoryWidget.cpp:461-487)
- [In-game menu|implemented] **Inventory tab - item list rows**: Unlimited-carry flat list of held items | data: name colored by rarity (Common #C4C4C4, Green #59E759, Blue #6CAAFF, Purple #CB7CF3, Red #F35959, Gold #FFEC6C); '? ' prefix when unidentified; '  x%d' when stackable qty>1; '  [equipped]' | states: identified, unidentified, equipped, stacked; scroll list (Source/LyraGame/Mordecai/UI/MordecaiInventoryWidget.h:159-164, .cpp:489-535)
- [In-game menu|implemented] **Identify button**: Identify an unidentified item | data: 'Identify' button per unidentified row (left padding 8) | states: present only for unidentified rows; no-op if service unavailable (Source/LyraGame/Mordecai/UI/MordecaiInventoryWidget.cpp:518-532)
- [In-game menu|implemented] **Stored Resources ledger panel**: Auto-stored materials/town resources | data: header 'Stored Resources'; rows '{Name}  x{Count}' sorted alphabetically (e.g. 'Iron Ore  x1', 'Town Stone  x1') | states: unbound '--', empty 'No resources stored.', populated (Source/LyraGame/Mordecai/UI/MordecaiInventoryWidget.cpp:432-452,544-562)
- [In-game menu|implemented] **Inventory empty / filtered placeholders**: Empty-state messaging | data: '--' (unbound), 'Inventory is empty.', 'No items match this filter.' in (0.6,0.6,0.6)->#CBCBCB | states: unbound, empty, filtered-empty, hidden (Source/LyraGame/Mordecai/UI/MordecaiInventoryWidget.cpp:217-245)
- [In-game menu|implied] **Item detail / description panel with icon**: Show selected item description, icon, type | data: Description (identification-aware: full, ShortDescription, or empty), Icon TSoftObjectPtr<UTexture2D>, Type - cached in row model but not displayed | states: identified vs unidentified description (Source/LyraGame/Mordecai/UI/MordecaiInventoryWidget.h:49-88; Items/MordecaiItemLibrary.cpp:156-175)
- [In-game menu|implied] **Item use / equip actions**: Use consumables, equip found weapons | data: none yet ('Drop and equip actions are OUT OF SCOPE') | states: N22 design question (Source/LyraGame/Mordecai/UI/MordecaiInventoryWidget.h:134; Specs/agent_playtest_plan_v1.md N22)
- [In-game menu|implemented] **Settings tab**: Game settings | data: 'Coming Soon' placeholder (Roboto Bold 24) | states: placeholder only; TODO(DECISION) whether a real Settings tab is wanted (Source/LyraGame/Mordecai/UI/MordecaiPauseMenuPlaceholderWidget.cpp:31-32; US-093 Technical Notes)
- [In-game menu|implied] **Menu input hints / action bar**: Show Back / Previous Tab / Next Tab / Close bindings | data: CommonUI display names 'Back' (Escape, Gamepad_Special_Right), 'Previous Tab' (Gamepad_LeftShoulder), 'Next Tab' (Gamepad_RightShoulder), 'Close' (I) | states: not displayed (bDisplayInActionBar=false) (Config/DefaultInput.ini:36-43; MordecaiPauseMenuWidget.cpp:170-190)
- [Notification / overlay|implemented] **Death screen**: Overlay while the player is dead, with respawn countdown | data: 'You Died' size 48 (0.8,0.1,0.1)->#E75959; countdown ceil-int size 32 white (e.g. '3'); 'Kills: %d' size 20 white; full-screen dim black 50% | states: shown on Mordecai.State.Dead, removed on respawn; countdown holds while paused; HitTestInvisible; HUD still drawn over it (Source/LyraGame/Mordecai/UI/MordecaiDeathScreenWidget.cpp:49-95; MordecaiDeathScreenComponent.cpp)
- [Loading / transition|implemented] **Loading screen**: Shown during map/experience loads | data: 'PROJECT MORDECAI' (size 56 Bold, letter spacing 150, uppercase, (0.86,0.80,0.68)->#EFE7D7); 'Loading...' (size 20 Regular, (0.55)->#C4C4C4); throbber; opaque bg (0.010,0.010,0.012)->#19191D | states: visible during load + ~2 s hold (3.02-3.53 s observed); art TODO(DECISION) (Source/LyraGame/Mordecai/UI/MordecaiLoadingScreenWidget.cpp; Config/DefaultGame.ini:83-86)
- [Notification / overlay|implemented] **On-screen engine/debug messages**: (unwanted) engine warnings shown to players | data: yellow text at ~(40,107) of 720p: 'P2PConnectionTimeout has moved from OnlineSubsystemSteam to SocketSubsystemSteamIP, previous location is depreceated', 'PassLoadingScreenWindowBackToGame failed.  No Window', earlier 'MORDECAI: ...' lines; grey ''DisableAllScreenMessages' to suppress' | states: present in packaged Development build; to be removed (issue #29, US-091 AC-091.5) (Specs/agent_playtest_plan_v1.md (on-screen debug text entries); Saved/Playtests/20261030-154954-boot/engine_01.png)
- [Main menu / front end|implemented] **Lyra main menu (L_LyraFrontEnd)**: Game entry point (GameDefaultMap) | data: LYRA logo; 'PLAY LYRA', 'OPTIONS', 'CREDITS', 'SHOW REPLAY', 'QUIT LYRA'; Player 1 machine/online id, 'UE5-CL-0 (Development)', 'Player 2: Not logged in', 'ADD PLAYER'; lobby background, blue glass panel, Orbitron | states: Lyra-branded; issues #57-#65 needs-decision (Config/DefaultEngine.ini:68; Saved/Playtests/20261030-155056-boot/engine_01.png; Specs/agent_playtest_plan_v1.md:204-219)
- [Main menu / front end|implemented] **Front-end experience tile 'Mordecai Arena'**: Player path from front end into the arena | data: TileTitle 'Mordecai Arena', TileSubTitle 'Dev test arena', TileDescription 'Project Mordecai diorama arena (DevTestMap)'; no TileIcon; no LoadingScreenWidget | states: listed among Lyra playlists (bShowInFrontEnd True); PLAY routes through Lyra multiplayer/replay screens (#60) (Scripts/create_arena_playlist.py:8-16,78-82; Plugins/GameFeatures/MordecaiCore/Content/System/Playlists/DA_Mordecai_Arena.uasset)
- [Main menu / front end|implemented] **Lyra options, credits, replay, confirmation/error dialogs**: Settings and system dialogs | data: /Game/UI/Foundation/Dialogs/W_ConfirmationDefault, W_ConfirmationError; Lyra settings screens; credits | states: Lyra-styled (#62, #63, #64) (Config/DefaultGame.ini:79-81; Specs/agent_playtest_plan_v1.md:206-214)
- [Other|implemented] **Input glyph / controller data sets**: Device-aware key icons for prompts and action bars | data: CommonInput_KeyboardMouse, CommonInput_Gamepad_XSX, CommonInput_Gamepad_PS5 (Lyra content); DefaultInputType MouseAndKeyboard; DefaultGamepadName XSX; InputData /Game/UI/B_CommonInputData | states: Lyra assets; Mordecai prompts don't use them (hard-coded '[F]', '[I]') (Config/Windows/WindowsGame.ini:1-10; Config/DefaultGame.ini:88-94; MordecaiPickupPromptWidget.h:26-27)
- [HUD|implied] **Quick item slots A-D**: Consumable/quick item usage | data: input actions IA_Mordecai_ItemA, ItemB, ItemC, ItemD exist; no runtime mapping or UI | states: none (Plugins/GameFeatures/MordecaiCore/Content/Input/Actions/IA_Mordecai_Item{A,B,C,D}.uasset)
- [HUD|implied] **Skill slots A/B**: Active skill usage | data: input actions IA_Mordecai_SkillA, IA_Mordecai_SkillB exist; no UI | states: none (Plugins/GameFeatures/MordecaiCore/Content/Input/Actions/IA_Mordecai_Skill{A,B}.uasset)
- [In-game menu|implied] **Map screen**: World/dungeon map | data: input action IA_Mordecai_Map exists; no binding, no UI | states: none (Plugins/GameFeatures/MordecaiCore/Content/Input/Actions/IA_Mordecai_Map.uasset)
- [HUD|planned-story] **Spell slot key/spell labels**: Show which key casts which spell | data: Spell1 Q / Gamepad LB (Fireball, cost 3), Spell2 E / RB (Blink, 2), Spell3 R / no gamepad (Stone Skin, 4), Spell4 One / DPad Up (Restoration, 3) | states: not displayed today; US-088 adds slot-key labels to cooldown widgets (Source/LyraGame/Mordecai/MordecaiHeroComponent.cpp:200-218; US-088 AC-088.3)

## KEY TERMS
- Combat HUD (UMordecaiCombatHUDWidget / WBP_CombatHUD) - the player HUD root, built in C++ BuildDefaultLayout because the WBP is an empty shell
- Spell Points (SP) - discrete spell resource shown as 'x / y' (MaxSpellPoints 10)
- Posture / Posture Broken - stagger meter; Mordecai.State.PostureBroken turns the bar red
- Stamina tiers - Green (>=66%), Yellow (>=33%), Red (>0%), Exhausted (<=0) (EMordecaiStaminaTier)
- Perfect Dodge! / Perfect Block! / Parried! / Parry Failed! / Posture Broken! - exact combat feedback strings
- Hit N - combo counter text format
- Kills: N - kill counter and death-screen format
- You Died - death screen headline
- [F] Pick up {item} / [I] Inventory - pickup and inventory assist prompts
- Paused - pause menu title
- Pause menu tabs - Character, Skills, Feats, Inventory, Settings (ids character/skills/feats/inventory/settings)
- Coming Soon - placeholder tab content
- Stored Resources - ledger panel header for auto-stored materials/town resources (Resource Ledger)
- Identify - button that identifies an unidentified item
- Unidentified {Rarity} {Type} / Unknown Item - pre-identification item names
- Rarity tiers - Common, Green, Blue, Purple, Red, Gold (Gray < Uncommon < Rare < Epic < Legendary < Mythic)
- Inventory filters - All, Weapons, Armor, Trinkets, Consumables, Materials, Quest, Magical
- Feat tiers - Common, Rare, Legendary; locked feats show '???'
- Skill categories - Weapon, DefenseArmor, MovementExploration, UtilityInteraction, Magic; ranks 0-20, milestones 1/5/10/15/20
- Attribute groups - Physical (STR, DEX, END), Resilience (CON, RES, DIS), Magical (INT, WIS, CHA)
- Spell loadout - Fireball (Q), Blink (E), Stone Skin (R), Restoration (1)
- Status effects - Mordecai.Status.* tags (Burning, Bleeding, Poisoned, Frostbitten, Shocked, Weakened, Brittle, Silenced, Rooted, Blinded, Fear, Cursed, Exposed, Corroded, Drenched; buffs Focused, Blessed, StoneSkin, FireWard, Restoration)
- Damage types - Physical (Slash/Pierce/Blunt), Fire, Frost, Lightning, Poison, Arcane, Radiant, Shadow, Corrosive (each has a damage-pop color)
- Enemy indicator - Lyra IndicatorSystem world marker (name, color-coded health, status icons), distinct from the old screen-space enemy health bar
- UI.Layer.Game / UI.Layer.Menu / UI.Layer.Modal - Lyra CommonUI layer stack (death screen on Game, pause menu on Menu)
- UI.Action.Mordecai.PauseMenu.PreviousTab / NextTab / Inventory - CommonUI UI action tags for LB / RB / I in the pause menu
- BindWidgetOptional - the named widget slots a designer WBP must provide to replace the C++ fallback
- Diorama camera - fixed overhead camera (pitch -50, never rotates); world labels billboard to it
- PROJECT MORDECAI / Loading... - loading screen strings
- Mordecai Arena - front-end playlist tile title (subtitle 'Dev test arena')

## OPEN QUESTIONS
- Should the combat HUD (and the weapon widget) move from AddToViewport into the CommonUI layer stack, e.g. UI.Layer.Game or a Lyra HUD layout? Today they draw above the pause-menu and death-screen dims, and '[I] Inventory' stays on screen in menus.
- Loading screen art direction (logo, key art, palette) is an explicit TODO(DECISION) in MordecaiLoadingScreenWidget.cpp:23-24.
- The front-end rebrand (#57-#69) is all needs-decision. Is a Mordecai front end wanted, or a direct boot into the arena? What happens to PLAY's Lyra multiplayer and replay screens, ADD PLAYER, credits and Lyra's options screens?
- With enemy indicators working (US-092), each enemy shows two health readouts: the old screen-space WBP_EnemyHealthBar and the indicator. Should the widget-component bar be removed? (TODO(DECISION) in US-092.)
- Is a real Settings tab wanted, or does the 'Coming Soon' placeholder stay? (TODO(DECISION) in US-093.)
- Should the palette be authored as sRGB hex? Current code comments name colors as if their values were sRGB (Fire 'Orange' (1,0.5,0) displays as #FFBC00 amber, Shadow 'Dark Red' as #CB5959), so the design system should state hex values and require FromSRGBColor conversion.
- Should item rarity colors (Common/Green/Blue/Purple/Red/Gold), feat tier colors (Common gray, Rare blue, Legendary gold), stamina tiers and enemy-health thresholds be unified into one semantic color scale? They currently use different values for 'gold', 'red' and 'blue'.
- Device-aware input glyphs: prompts hard-code '[F]' and '[I]'. Should the design system use CommonInput controller data (KBM/XSX/PS5 sets, which are Lyra content today) or a custom glyph set?
- Spell slot labelling: US-088 plans Q/E/R/1 labels. Spell3 (R, Stone Skin) has no gamepad binding. What should its gamepad glyph be?
- Where should the weapon name readout go? AC-078.11 says bottom-center, US-091 says bottom-center or bottom-right, and it must not overlap the pickup prompt. Should a weapon icon or type be shown too?
- Should damage numbers be built (DamagePopComponent has the colors and timings but renders nothing), and in what form? Lyra offers NiagaraText or MeshText number pops.
- Combo counter: is 'Hit N' still wanted once combos are wired (N24)? It currently never shows text.
- Facing and aim are pinned to +X by the camera, and the facing arrow is likely hidden (N12). Is an aim reticle or facing indicator part of the HUD design?
- Can the game show the player's own name or character on the HUD? No player portrait or name widget exists, and the enemy indicator falls back to the internal actor name (e.g. BP_..._C_1) when EnemyDisplayName is empty.