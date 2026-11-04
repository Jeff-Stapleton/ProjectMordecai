# Tally

The counting motif: four upright strokes closed by a slash, the Tally's own handwriting, with exactly three jobs: skill ranks, rarity badges, and the seal and loading throbber. Use it only for those three; everything else that counts uses pips, numbers, thin bars, notched tracks, dividers or tokens, so a row of strokes always means one thing.

## Anatomy
- **Strokes 1 to 4**: upright, round caps. Unlit strokes stay printed as dots (core `.tt-tally-empty`): an unlit upright is a thinner column of dots, an unlit slash three dots that never land on an upright. Dots are `lip` on plate and `ink-muted` on enamel (automatic inside `.tt-tag`; elsewhere `.tt-tally--enamel`). They are dotted in every theme: in High Contrast `lip` is white like a lit stroke, and the shape still tells them apart. Never restyle the dots locally.
- **Slash** (= 5): one px heavier than a stroke, corner to corner across its four strokes. Bundles stand 8 px apart.
- **Cap dot**: a dot on top of stroke 1, for skill rank 1 only (the first milestone).
- **Colour**: lit strokes are `text` on plate and `ink` on enamel, never `good`.
  - Rank 20 is a full bundle (all four bundles closed) in `text`, the same colour as rank 12.
  - Rarity badges take their rarity token. A gold full bundle means rarity Gold and nothing else.
  - The throbber is drawn in `text`; the seal in `rim` on plate and `ink` on enamel.
- **Sizes** (`data-height` / `data-stroke` / `data-pitch`). This table is the source; components cite it:

| Use | Height | Stroke | Pitch | Width | Used by |
|---|---|---|---|---|---|
| Skill row | 24 | 3 | 7 | 148 for 20 | SkillsTab rows, TallyPlate's technique kicker row |
| Skill detail | 32 | 4 | 9 | 180 for 20 | a skill's detail pane (with the milestone ruler) |
| Rarity badge | 20 (24 in large lists) | 3 | 6 | trimmed to the strokes: 8 (1 stroke) to 26 (4); Gold 28; the Common ring 20 | every rarity badge |
| Throbber | 32 | 4 | 7 | one bundle | LoadingScreen |
| Seal | 96 (48) ring, bundle 40 (20) | 5 (3) | 8 (5) | one bundle | title, receipts, the last ticket |

- **Milestone ruler** (`.tt-tally-ruler`, detail size): "1" sits under rank 1's stroke (x 6); 5, 10, 15 and 20 sit under each bundle's fourth stroke, where its slash ends (x 34, 81, 128, 175).
  - Reached numbers are `text`; the others `text-muted`.

## The three jobs
1. **Skill ranks** (`.tt-tally-rank`): 20 strokes in 4 bundles.
   - The slashes land exactly on the milestones 5, 10, 15 and 20, and rank 1 wears the cap dot. So all five milestones (1 / 5 / 10 / 15 / 20) are visible in the mark itself.
   - Row: name (`ui-body`), the 148 x 24 strokes, then "rank / 20" in a 112 px Mono 700 24 box.
   - Rank 0 (`.is-unlearned`) mutes the name and the value.
2. **Rarity** (`.tt-rarity` + `data-rarity`): Common = an empty ring, Green = 1 stroke, Blue = 2, Purple = 3, Red = 4, Gold = the full bundle.
   - Always with the rarity colour and the colour word in the name ("Unidentified Purple Amulet").
   - Blue and Purple converge for deuteranopes, and the count carries them.
   - On enamel the badge sits in an `ink` socket (`.tt-socket`).
3. **The seal and the throbber**:
   - The **seal** (`.tt-tally-seal`): a double ring around one full bundle, in `rim` on plate. As a stamp (`--stamp`) it is `ink` on enamel, turned −8°. It appears on the title screen, at the foot of receipts and on the last ticket, "Count closed.".
   - The **throbber** (`.tt-tally-throbber`): one bundle drawn stroke by stroke over its own dotted ghost, in `text`, on the loading screen ONLY. Timing: Motion **Throbber** (the brand book's row). It keeps looping under Reduce Motion, because it proves the game has not hung.

## Not a tally (D1)
Everything else that counts uses another mark. These replace every stroke that was drawn elsewhere:

| Count | Mark | Owner |
|---|---|---|
| Light-attack combo | "Hit N" alone; no strokes under the weapon name | CombatCallout; LoadoutPlate |
| Boss phases | Pips (`.tt-pips--lg`, 20 px) | BossBar |
| Status stacks | Pips (`.tt-pips`, 14 px) | StatusChip |
| Feat conditions | A thin bar (`.tt-bar--thin`) with a "6 / 10" count | FeatsTab, TallyRail, TallyPlate |
| Options ranges | A notched track (a `.tt-bar` with `__notch` marks) | OptionsScreen |
| Title and Credits dividers | `.tt-divider` | TitleScreen, Credits |
| Loading | The wordmark and the throbber only: no tally field, no tally rule, no seal medallion | LoadingScreen |
| Credits printing | "Printing..." with no throbber | Credits |
| The rail's pending badge | The number only ("+3") | TallyRail |
| World map charted cells | Numerals | WorldMap |
| Spell Points | Tokens (below) | GaugePlate, CharacterTab |
| Kills, enemy tier, feat tier | "Kills: 12"; gold diamonds; badge silhouettes (circle, flat-top hexagon, starburst) | TallyRail; EnemyTag; FeatsTab |
| Health, Stamina, Posture | Meters with notches | Bar |

## Spell Points: tokens, not strokes
Spell Points count in fives like the Tally, but as **tokens** (`.tt-tally-sp`), never strokes. The row is designed for 10 to 16 tokens, and it always fits GaugePlate's 268 px track:

| MaxSpellPoints | Token | Gaps (in a five / between fives) | Widest row | Class |
|---|---|---|---|---|
| 10 | 22 px | 4 / 12 | 10 = 264 px | `.tt-tally-sp` |
| 11 to 13 | 16 px | 3 / 8 | 13 = 254 px | `.tt-tally-sp--16` |
| 14 to 16 | 14 px | 2 / 6 | 16 = 266 px | `.tt-tally-sp--14` |
| above 16 | the `SpellPointsProgressBar` fallback in the same track | | | `TODO(DECISION)`, US-090 |

- MaxSpellPoints = 10 x (1 + 0.04 x EffInt) (`UMordecaiAttributeSet`), which is 13 at the default INT, so the default state reads "13 / 13" with 16 px tokens. 10 is the floor (EffInt 0); 14 and 16 are example values.
- A held token is `spell-points` with an `ink` ring and an `ink` slot notch on top, like a coin.
- A spent token is `well` with a `lip` ring.

## States
- **Skill rank**:
  - Unlearned (0).
  - Learning: the next stroke draws on when Allocate lands (in the pause menu, so never in combat).
  - Milestone: the slash or cap dot draws on, and its ruler number turns `text`. The milestone plate itself is another lane (TallyPlate).
  - Max (20 / 20): the full bundle in `text`.
- **Rarity**: static.
- **Spell Points**: held or spent per token. The text stays the authority ("13 / 13").
- **Throbber**: running, also under Reduce Motion.

## Data and implementation
- **Draw from images, not live geometry**: Slate lines have no round caps.
  - Make the `03` Tally atlas: bundle frames 0 to 5 at each size, each frame a filled layer (tinted per use) over the dotted ghost layer (`lip`), the same in both themes.
  - Add a separate cap-dot image.
  - New widget `UMordecaiTallyWidget`: `Count` and `Max`, one UImage per bundle; bundle b shows frame clamp(Count − 5b, 0, 5). Only for ranks, rarity badges, the seal and the throbber.
  - Rarity badges use frames 1 to 4 and 5, plus the ring.
- **Skills tab** (`UMordecaiSkillTreeWidget`): rows from `FMordecaiSkillTreeRow` (CurrentRank, MaxRank = 20), milestones from `FMordecaiSkillTreeMilestoneRow` {1, 5, 10, 15, 20}.
  - The "rank / 20" text stays a real TextBlock: playtests expect "/ 20".
  - The strokes update on allocation and on `Mordecai.Event.SkillMilestone`. Ranks are bought with skill points (Allocate) today; use-based levelling is `TODO(DECISION)`.
- **Rarity**: `UMordecaiInventoryWidget` rows. The static constexpr rarity colours are replaced by the `rarity-*` tokens. Pickup tags carry the badge in a socket.
- **Spell Points** (`UMordecaiSpellPointsBarWidget`):
  - Keep `SpellPointsProgressBar` and `SpellPointsText` ("x / y", read by the bridge).
  - Add an HBox `SpellPointTokens` of disc Images (on and off textures at 22, 16 and 14 px), grouped in fives; pick the size from MaxSpellPoints with the table above.
  - The gauge plate shows the progress bar only above 16.
- **Loading screen** (`UMordecaiLoadingScreenWidget`, built in `Initialize()`): the throbber replaces the `UThrobber` under `StatusText`, as a UImage flipbook or a UMG animation on the Throbber timing. Keep `TitleText` and `StatusText`.
  - The loading screen is World voice: the wordmark and the throbber are its only Tally pieces. No remark, no tally field or rule, no seal medallion.
- **Tokens**: `text`, `ink`, `ink-muted`, `lip`, `rim`, `spell-points`, `well`, `enamel`, `rarity-common`, `rarity-green`, `rarity-blue`, `rarity-purple`, `rarity-red`, `rarity-gold`.

## Copy
- Shipping strings, exact: "rank / 20" (e.g. "12 / 20"), SP "x / y" (e.g. "13 / 13"), the colour words Common, Green, Blue, Purple, Red and Gold, "Loading...", "Count closed.".
- Example values in the preview: the skill ranks shown, "14 / 16", "10 / 10", the pips' "3 of 5", "6 / 10", "7 / 24".
- The marks carry no words of their own.

## Do / Don't
- Do keep the strokes to the three jobs, and give anything else that counts its own mark from the table above.
- Do keep unlit strokes printed as dots, so rank 12 reads as 12 of 20.
- Do pair a rarity badge with its colour and the colour word.
- Do size the skill row at 148 x 24 (24 / 3 / 7) wherever a rank row appears.
- Don't draw strokes for combos, kills, phases, stacks, conditions, sliders, tiers, dividers, badges or meters.
- Don't colour rank strokes `good`, or any rank bundle gold.
- Don't run the throbber anywhere but the loading screen, and don't stop it under Reduce Motion.
- Don't animate strokes during combat.
