# Voice and copy

Two registers share the screen. The plain voice is the game. The Tally is a counting machine that may add one dry line. The world's own weight (Impermanence vs Control) is never undercut.

## Plain voice: the game

- Sentence case. A verb on every button: "Identify", "Allocate", "Stay", "Quit", "Equip". Never "OK", "Yes" or "No".
- Digits for every number. Units the way the game counts them: "12 / 20" (a rank), "7 / 13" (Spell Points), "4.0" (seconds), "x3", "+9.4%".
- The exact system words: Health, Stamina, Posture, Spell Points (SP), Posture Broken, Guard Broken, Knocked Down, Exhausted, Skill Points, Allocate, Feats, Common / Rare / Legendary, Drawback, Unidentified, Mage Tower, Stored Resources, Upgrade Key.
- No exclamation marks, except the shipping combat callouts.
- **A keycap leads its label**, everywhere: "[F] Pick up Rusty Sword", "[I] Inventory", "[Esc] Stay", "[Enter] Identify". On a hint bar the pairs sit in that order, split by "|": "[Esc] Back | [I] Close".
- **Errors** say what happened and what happens next. Never apologise, and never promise an item or a system the game does not have.
  - Example (Dialog's error, example copy): title "Couldn't load the map", body "The Expedition failed to start. You will return to the title screen.", button "Return to title".
  - Write each real error for a failure the code can report (`TODO(DECISION)` in Dialog).
  - The HUD writes no errors. A spell the player cannot afford shows hollow cost rivets and the "SP" lock chip (SpellSlot), never "Not enough SP".

## The Tally: remarks

- An Automation-Age counting engine. It is deadpan and literal, quietly proud of numbers, and puzzled by what it cannot count.
- It is indifferent, not cruel. It is funny because it cares precisely about the wrong thing.
- At most 9 words, always on the second line. Every ticket and plate is complete without its remark.
- Never during combat (a receipt forced by the 20 s cap prints without remarks). Never on death, boss, biome, loading, ending or status lines. Never about story, NPC loss or the Lich King. Never about a failure.
- Where remarks may go: a SKILL UP ticket's footer when it has no plain detail, a plate's remark tag, the quit dialog's body, and the title tagline. FEAT PROGRESS, STORED, ITEM and IDENTIFIED tickets carry facts in their footers, so they carry no remark (TallyRail).
- Write to fit: a ticket footer remark has 222 px at 20 px (about 22 characters); a plate's remark tag has 560 px.
- Players control it: Settings > Accessibility > "Tally remarks" On / Off. "Tally notifications" Calm and Minimal also drop remarks.
- Never "Filed under..." jokes. Never addresses the player as a contestant: there is no audience in this world.

The ticket and plate copy below follows TallyRail's kinds table and TallyPlate's kinds; those cards are the canon. Kickers are written in sentence case in data and cased with ToUpper on screen. Every remark here is example copy waiting for a tone read (Open decisions).

| Moment | Piece | Headline (plain) | Remark (optional) |
|---|---|---|---|
| Skill rank | SKILL UP ticket | "Skill up", rank "6 / 20" on row 1; "Dodging" | "Six. Up from five." |
| Rank-ups merged | SKILL UP ticket | The latest rank, "6 / 20"; "Dodging"; footer "+2 ranks" | None: the plain footer wins |
| Feat progress | FEAT PROGRESS ticket | "Feat progress"; "Pyromaniac" "4 / 6"; footer "Burn 6 enemies at once." | None |
| Stored loot | STORED ticket | "Stored"; "Iron Ore" x2, "Town Stone" x1 | None |
| Item picked up | ITEM ticket | "Item · Purple" and the badge; "Unidentified Purple Amulet" | None |
| Identify | IDENTIFIED ticket | "Identified"; "Mysterious Amulet"; footer badge + "Purple · Magical Item" | "It had a name all along." (see below) |
| Feat unlocked | TallyPlate | "Feat unlocked · Common"; "Pyromaniac"; "Burned 6 enemies at once." | "Six at once. The Tally checked twice." |
| Skill milestone | TallyPlate | "Technique · Rank 10"; "Dodging"; "Dodging now cancels light-attack recovery." (example line) | "Ten. A tidy number." |
| Level up (`TODO(DECISION)`: no Level or XP exists) | TallyPlate | "Level up"; "Level 7"; "+1 Skill Point" | "Seven. Up from six, as forecast." |
| Quit | Dialog | "Quit to desktop?"; buttons "Quit" (left) and "[Esc] Stay" (right, default) | "The Tally will keep count." |
| Title screen | TitleScreen | "PROJECT MORDECAI" | "Everything counts. Nothing lasts." (the tagline) |
| Acceptance ending | The last ticket | "Count closed." | None: it is the last ticket |

"It had a name all along." is the one identify remark; no other version exists. The IDENTIFIED ticket's footer carries the rarity and type, so the rail has no slot for it today. Where it prints is `TODO(DECISION)` (Open decisions).

## World voice: weight

The World voice is set in Atkinson Hyperlegible Next 800 in tracked caps. It carries no Tally remark, no Rowdies lettering, no tally strokes and no rivets. It may use the family's construction (plates, inlay mats, strings, a stamp ring), because that is not voice. The loading screen keeps exactly two Tally pieces: the wordmark and the throbber.
- **Death:** "You Died", the countdown, and "Kills: 4". An optional plain line, "Nothing stays. Not even this.", is copy for a tone read (`TODO(DECISION)`).
- **Biome cards:** the biome name alone ("MIRE OF ECHOES"). The world bible's theme lines and its emotional-arc words (Denial, Control, Avoidance...) are design notes, not UI copy.
- **Boss intro:** the name ("IRON WARDEN") and one plain line from the boss data, `IntroLine` (proposed field). Iron Warden: "Enforces strict patterns. Punishes deviation." (example). It is one string, stored in data and used as is in every card.
- **Boss phase:** the Banner's Phase state, in the World voice.
- **Loading:** "PROJECT MORDECAI" and "Loading...". No remark; a gameplay tip, if one shows, is in the plain voice.
- **Status lines** use the launch-set identities verbatim: Burning "You're on a clock", Bleeding "Recovery is denied", Poisoned "You can't catch your breath", Frostbitten "Sluggish body, slower reactions", Shocked "Timing disruption", Weakened "You can't break them", Brittle "You break easily", Silenced "Magic is shut off", Rooted "Position solved or punished", Blinded "Reads become unreliable", Fear "Composure collapse", Cursed "Resource rules broken", Exposed "Next mistake hurts more", Corroded "Gear advantage denied", Drenched "Elemental rules shift", Focused "Mastery streak reward".

## Shipping strings (restyle, never reword)

The Tier 4 playtests compare these text values. Change one only together with its playtest.

- **Combat callouts:** "Perfect Dodge!", "Perfect Block!", "Parried!", "Parry Failed!", "Posture Broken!".
- **Combo:** "Hit N".
- **HUD:** "Kills: N", SP as "x / y" (e.g. "7 / 13"), "[F] Pick up {item}", "[I] Inventory".
  - The F and the I are drawn as keycaps in front of their labels, but the TextBlocks keep "[F] …" and "[I] Inventory" until device-aware glyphs ship together with the playtests.
- **Death:** "You Died", the countdown digit, "Kills: N".
- **Loading:** "Project Mordecai" (shown as PROJECT MORDECAI via ToUpper), "Loading...".
- **Pause menu:** "Paused", the tab labels "Character", "Skills", "Feats", "Inventory", "Settings", and the placeholder "Coming Soon".
- **Inventory:** "Inventory is empty.", "No items match this filter.", "No resources stored.", "Identify", "Stored Resources", and ledger rows like "Iron Ore  x1" (two spaces).
- **Inventory filters:** "All", "Weapons", "Armor", "Trinkets", "Consumables", "Materials", "Quest", "Magical".
- **Item names:**
  - "Unidentified {Rarity} {Type}" (e.g. "Unidentified Purple Amulet") and "Unknown Item".
  - Row affixes: the "? " prefix, "  x3", "  [equipped]".
- **Feats:** "Feats: N unlocked — a Common, b Rare, c Legendary".
- **Skills:** "rank / 20".
- **Weapon:** "Unarmed".
- **Status names:** today `StatusNameText` shows the tag segment ("StoneSkin"). The mockups show the display name "Stone Skin". The switch lands with US-091 and its playtests.

## Item and system names

- Rarity words are colour words, in six tiers: Common, Green, Blue, Purple, Red, Gold. Red and Gold are separate tiers. Don't use Uncommon, Epic or Mythic in player-facing text.
- Item type strings: Weapon, Armor, Trinket, Consumable, Material, Resource, Upgrade Key, Quest Item, Magical Item, Currency.
- Feat tiers: Common, Rare, Legendary. Rare and Legendary feats carry a Drawback, shown in the one Drawback box.
- Biomes: Verdant Wilds, Searing Expanse, Mire of Echoes, Crystal Caverns, Storm Peaks, Withered Kingdom.
- Bosses: Thorn Guardian, Iron Warden, Drowned King, Crystal Seer, Tempest Ascendant, The Lich King.

## Example data

Anything a card invents is labelled "example" in its README. The recurring examples: the feats "Steel Rhythm", "Living Furnace" and "Unflinching"; the area "Sunken Causeway"; every boss `IntroLine`; every Tally remark; the milestone lines paraphrased from the skill sheet. Real data stays real: Pyromaniac (Common, "Burn 6 enemies at once"), the four spells and their costs, the weapons, the items and the enemies listed in the build contract.
