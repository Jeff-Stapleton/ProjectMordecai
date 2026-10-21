# agent_rules_v2.md — UE5 Tech Stack (Project Mordecai)

These rules are **authoritative**. If a tradeoff exists, follow this doc unless the user explicitly overrides it.

---

## 0) Engine Version

### DO
- Target **Unreal Engine 5.7** for all implementation decisions.
- Prefer UE 5.7-native APIs and recommended patterns.

### DO NOT
- Introduce version-specific workarounds for earlier UE versions unless explicitly requested.

### WHEN UNSURE
- Add a short comment: `// UE5.7: verify API name/signature if it differs from 5.5/5.6`

---

## 1) Camera & Controls (Locked)

### Camera Style
- **3rd-person diorama perspective**
- Camera **follows the character**
- Camera **does not rotate** (no free-look; no yaw/pitch driven by player input)

### DO
- Implement movement/aiming/targeting assuming a fixed-orientation camera.
- Ensure interaction selection works without camera rotation (e.g., auto-target, cone checks, closest-in-front logic, or explicit target cycling).

### DO NOT
- Add free-look, orbit camera, shoulder swap, right-stick rotation, or “hold-to-rotate camera.”
- Depend on camera yaw to define movement direction unless clearly documented and consistent.

### ALLOWED
- Screen shake / camera effects that do **not** change camera facing (orientation).
- Minor positional smoothing/lag to follow the character (no rotation).

---

## 2) Inventory & Items (Unlimited Carry / Town Gating) (Locked)

### Core Model
- Player inventory is **effectively unlimited** for normal adventuring.
- Inventory management is **not** intended to be a difficulty source.
- Players should be able to **pick up and keep everything** that is reasonably collectible during expeditions.

### Canonical Loot Friction
Loot pressure should come from **town-facing systems**, not bag space.

The primary return-to-town forcing functions are:
1. **Magical item identification** at the **Mage Tower**
2. **Healing potion stock limits** governed by the **Alchemist**
3. Delivering **materials / town resources / upgrade keys** into long-term progression loops

### Material Rule
- Most non-gear loot should be classified as **Material** or **TownResource**.
- Materials and town resources should be **auto-stored** into a shared progression inventory / resource ledger.
- Players should **not** be asked to drop useful monster parts, herbs, ore, hides, or upgrade materials because of inventory limits.

### Magical Item Rule
- Magical items may drop in an **Unidentified** state.
- Unidentified magical items cannot be fully used until processed by the **Mage Tower**.
- Identification is an intentional pacing mechanism and replaces “bag full” as a major looting constraint.

### Consumable Rule
- Consumable carry logic should support readable limits where needed, but **healing scarcity should primarily come from village production / stock**, not inventory capacity.
- The Alchemist determines how many health potions the player can reliably take into the field at a time.

### DO
- Implement inventory as a **flat, player-friendly carry model** with strong sorting/filtering.
- Support categories such as:
  - `Weapon`
  - `Armor`
  - `Trinket`
  - `Consumable`
  - `Material`
  - `TownResource`
  - `UpgradeKey`
  - `QuestItem`
  - `MagicalItem`
- Auto-route materials and town resources into persistent storage.
- Surface unidentified magical items clearly in UI and tooltips.
- Preserve strong organization, filtering, and search so unlimited carry remains readable.

### DO NOT
- Implement bag slots, container tetris, or specialized bags as a core system.
- Force players to discard strategically important materials due to lack of space.
- Use inventory capacity as the primary limiter on healing, crafting, or town progression.

### DEFAULTS (until user decides otherwise)
- Materials auto-store immediately on pickup.
- Town resources auto-store immediately on pickup.
- Magical items are individually tracked and may require identification.
- Sorting should prioritize: quest items, upgrade keys, magical items, then common gear.

---

## 3) Build, CI, and Testing (Local-Only for Now) (Locked)

### DO
- Keep build + test workflows **local**:
  - simple scripts (bat/sh/py) in `Scripts/`
  - basic commandlets
  - local automation tests that run without infrastructure
  - local packaging: `RunUAT BuildCookRun`, run only through `py Scripts/package_playtest.py` (a local script, not a pipeline)
  - local real-input playtests: the in-game agent bridge (non-Shipping builds, `-AgentBridge`, `127.0.0.1` only), its Python client `Scripts/agent_bridge.py`, the per-story scripts `Scripts/playtests/usXXX_<name>.py`, and the stdio MCP server `Scripts/mordecai_play_mcp.py` (Python packages pinned in `Scripts/requirements.txt`, installed once with `py -m pip install -r Scripts/requirements.txt`) — local scripts, not a pipeline

### DO NOT
- Introduce BuildGraph
- Introduce Horde
- Introduce Gauntlet or any other test-orchestration framework
- Build distributed CI/test pipelines
- Add heavyweight infra dependencies for building/testing

### GOOD PRACTICE
- Keep scripts in a predictable location (e.g., `/BuildScripts` or `/Scripts`) and document usage in `README.md`.

### DEFINITION OF DONE: PLAYABLE IN THE PACKAGED BUILD (Locked)
**Playable in the packaged build, or it isn't done.** A story with player-facing behavior (anything a player sees, presses, or navigates to) is done only when a playtest on a **fresh packaged build, built from the commit being delivered**, shows the feature working, reached the way a player reaches it — and you have opened the screenshots and stated what is visibly on screen. Every AC that needs player input (keys, movement, mouse, menus) is accepted **only** by a passing Tier 4 real-input playtest on that same package.

Verification ladder — run in order. A lower tier **never** substitutes for a higher tier.
1. **Tier 1 — Headless automation tests** (logic): `py Scripts/run_tests.py` (runs `Automation RunTests Mordecai` and judges pass/fail from the log — the raw `UnrealEditor-Cmd` exit code is 0 even when tests fail), per `CLAUDE.md` "Build & Test Commands".
2. **Tier 2 — PIE smoke tests** (editor integration): `py Scripts/run_pie_verify.py`.
3. **Tier 3 — Packaged playtest** (the game a player actually runs): `py Scripts/package_playtest.py`, then `py Scripts/playtest_boot.py`; review every screenshot.
4. **Tier 4 — Real-input playtest** (the player actually does it): the story's `py Scripts/playtests/usXXX_<name>.py` on the same fresh package as Tier 3; review every `bridge_NN.png`. The in-game agent bridge (`UMordecaiAgentBridgeSubsystem`, non-Shipping builds launched with `-AgentBridge`; contract in `Specs/agent_bridge_v1.md`) injects OS-level key/axis/mouse/click events into Slate, so they reach CommonUI and Enhanced Input along a player's path, and returns state JSON + screenshots. `Scripts/agent_bridge.py` is the Python client and CLI; the `mordecai-play` MCP server (`Scripts/mordecai_play_mcp.py`, `.mcp.json`) lets agents play interactively. See `CLAUDE.md` Step 4d.

### PIE SMOKE TESTS — Tier 2 (Locked)
Headless tests (NullRHI) cannot catch visual or runtime integration issues. PIE smoke tests **must** be run for **every story, regardless of Execution Mode** — no exceptions. They prove **editor integration only, never playability**: PIE runs against in-editor memory (including unsaved assets) and honors PIE-only settings.

- Run `py Scripts/run_pie_verify.py` after headless tests pass.
- If you create a new feature with runtime/visual behavior, **add a `Scripts/verify_pie_<feature>.py` smoke test** that verifies it works during PIE.
- Smoke tests query live game state via Python remote execution — they check widget visibility, spawn chain integrity, component existence, etc.
- **Do not fake the player.** A check that calls gameplay functions directly (teleport the pawn + `try_pickup_focused()`, `toggle_pause_menu_to_tab('inventory')`, etc.) is a **logic/diagnostic** check. Label it that way in script output and completion notes; never cite it as acceptance of a player-facing AC.
- A story is not done if PIE smoke tests fail. A PIE pass alone never makes a player-facing story done.

### PACKAGED PLAYTEST — Tier 3 (Locked)
Required for every story, regardless of Execution Mode (`CLAUDE.md` Step 4c). For a story with player-facing behavior it must show the feature; for any other story it proves the delivered commit still packages and boots to the expected experience. Both scripts are local and need no GUI editor (allowed in HEADLESS stories).

```powershell
# 1. Commit code AND content first, then package that commit (Development Win64 -> PackagedBuilds\Playtest\)
py Scripts/package_playtest.py            # iterative cook; --clean = full cook, --all-maps, --dry-run prints the UAT command
# 2. Boot the package, verify the experience, capture screenshots -> Saved\Playtests\<YYYYmmdd-HHMMSS>-boot\
py Scripts/playtest_boot.py               # DevTestMap, expects B_MordecaiArenaExperience
py Scripts/playtest_boot.py --frontend    # real player entry point (L_LyraFrontEnd), expects B_LyraFrontEnd_Experience
```

#### DO
- Package from a **clean tree** at the commit you deliver. A `build_info.json` with `"dirty": true` is a diagnostic build, not delivery evidence.
- Re-package after any change under `Source/`, `Plugins/`, `Content/`, `Config/` or the `.uproject`, and after any new commit made before the playtest — `playtest_boot.py` refuses stale builds (exit 2, `STALE BUILD`), including any build whose commit is not HEAD.
- Save all editor work before packaging — the cook reads assets from disk, not editor memory.
- Open **every** screenshot listed in `report.md` with the Read tool and state what is actually on screen (map, character, HUD, the feature's visible elements). A black/uniform-screenshot warning is a problem to investigate, not noise.
- Treat exit codes literally: `0` all checks pass; `1` gameplay failure (wrong experience, crash, timeout) — the story is not done; `2` infrastructure (exe missing, stale build, window not found, git error) — fix it and re-run.
- Cite the run dir (`Saved/Playtests/<run>/report.md` + absolute screenshot paths) in the story's completion notes and in the body of the story's final commit, made after the playtest. That commit must not touch `Source/`, `Plugins/`, `Content/`, `Config/` or the `.uproject` (otherwise re-package and re-run); `git commit --allow-empty` is fine for an evidence-only commit.
- Report FAILs and warnings honestly — never paper over them.
- Tier 3 sends no input. An AC that needs input (e.g. press F to pick up, I to open inventory) is checked at Tier 3 only as far as it can see (the content is in the packaged map and visible on screen); it is accepted only by the Tier 4 real-input playtest below — never on the strength of a Tier 3 screenshot or a direct function call.

#### DO NOT
- Pass `--allow-stale` to produce delivery evidence (diagnostic use only; it is recorded in the report).
- Hand-edit `build_info.json`, or playtest a build made by `Scripts/Package.bat` (it writes no `build_info.json`, so `playtest_boot.py` reports it as a STALE BUILD).
- Skip Tier 3 because packaging is slow. The first package can take an hour+ (iterative cooks are faster); that cost is accepted — the old "fast" loop shipped nothing playable.

#### STORIES
- Every player-facing story carries a **Playtest** section: launch path (front end or direct map + expected experience), the player's steps (keys/movement per `control_bindings_v1.1.md`), what must be visible on screen, and which playtest proves each step (the Tier 3 `playtest_boot.py` run; the story's Tier 4 script `Scripts/playtests/usXXX_<name>.py` for every step that needs input). If it is missing, add it to the story before implementing.
- A HEADLESS (C++) story whose feature needs content must name its paired EDITOR story. The feature is not playable — and the epic's playable milestone is not met — until both land and the packaged playtest passes. Record `NOT YET PLAYABLE — pending US-XXX` in the HEADLESS story's completion notes; the Tier 3 playtest (and the Tier 4 playtest, for input-driven ACs) is an AC of whichever paired story lands last.

### REAL-INPUT PLAYTEST — Tier 4 (Locked)
Required for every story with ACs that need player input (keys, movement, mouse, menus): such an AC is accepted **only** by a passing Tier 4 run (`CLAUDE.md` Step 4d). It runs on the same fresh package as Tier 3; Tier 3 stays mandatory for every story. Both are local scripts and need no GUI editor (allowed in HEADLESS stories).

```powershell
# Same fresh package as Tier 3 -- no re-package while HEAD is unchanged
py Scripts/playtests/usXXX_<name>.py      # e.g. us079_inventory.py -> Saved\Playtests\<YYYYmmdd-HHMMSS>-usXXX\
# Exploring and writing the script (not evidence by itself); always finish with quit
py Scripts/agent_bridge.py launch | state | press F | walk-to 400 0 | click-button "Identify" | screenshot | quit
```

#### DO
- Ship `Scripts/playtests/usXXX_<name>.py` with every player-facing story that has input-driven ACs, modelled on `Scripts/playtests/us079_inventory.py`: every player step is a check (PASS/FAIL/SKIP/WARN), every wait FAILs on timeout (never a false pass), and it screenshots what the ACs promise.
- Drive the game only through the bridge's real-input commands — `key`, `axis`, `mouse`, `click` (and the client helpers built on them: `tap`, `hold`, `walk_to`, `click_button`). They inject OS-level events into Slate, so they take a player's path (Slate preprocessors, focus, viewport, CommonUI, Enhanced Input). Read with `state`, `wait_until`, `screenshot`.
- Run it on the same fresh package Tier 3 used. It reuses `playtest_boot.py`'s staleness gate and must exit `0`.
- Treat exit codes literally: `0` all checks pass; `1` gameplay failure (the game did not do what a player expects) — the AC is not accepted, fix or file the bug; `2` infrastructure (STALE BUILD, launch failure, no bridge connection, crash) — fix it and re-run.
- Open **every** `bridge_NN.png` listed in `report.md` with the Read tool and state what is actually on screen; compare with what the report says it should show.
- Cite the run dir (`Saved/Playtests/<YYYYmmdd-HHMMSS>-usXXX/report.md` + absolute screenshot paths) next to the Tier 3 run in the story's completion notes and the closing commit body.
- Record an input the bridge cannot drive yet as `PENDING TIER 4 (bridge cannot drive: <reason>)`. That AC stays unchecked and is not claimed; every input AC the bridge can drive must still pass in the story's Tier 4 script. The story may then move to done, its Playable Status says the input is not yet accepted, and the bridge gap goes into the Known issues backlog of `Specs/agent_playtest_plan_v1.md`.
- Close the game afterwards: the playtest scripts and the MCP server kill their game on exit. After a CLI `launch` (detached) run `py Scripts/agent_bridge.py quit`; after a playtest script's `--keep-running` run the quit command it prints (`py Scripts/agent_bridge.py --port <port> --token <token> quit`) or `Stop-Process -Name LyraGame`.

#### DO NOT
- Call gameplay or UI functions as a stand-in for input: no remote exec, Lyra RPC registry, cheat or console commands, no `debug_*` bridge command. A `debug_*` result (`"debug": true`), a `route:"viewport"` key (`"diagnostic": true`) or any direct call is never Tier 4 evidence.
- Run Tier 4 acceptance on a STALE BUILD, a package built from a dirty tree, an attached game, or with `--allow-stale`. Those runs report `Evidence: DIAGNOSTIC ONLY -- not acceptance evidence`; acceptance needs `Evidence: ACCEPTANCE CANDIDATE` and exit 0.
- Work around a game bug inside the playtest (e.g. close a menu some other way because Escape does nothing). A FAIL is a bug to fix or file; US-079's first run found exactly that one (Escape could not close the pause menu; since fixed).
- Record `PENDING TIER 4 (real input)`. It is retired; only `PENDING TIER 4 (bridge cannot drive: <reason>)` remains.
- Treat a CLI or MCP session as acceptance. Acceptance is the story's playtest script, run on a fresh package.

### CONTENT MUST BE COOKED (Locked)
- Content that C++ loads by a path string (`LoadClass`/`LoadObject`/`FSoftObjectPath(TEXT("/MordecaiCore/..."))`) is invisible to the cooker. It works in PIE and is missing in the packaged game. This left the 2026-10-18 build with 0 weapons, 0 spells and no HUD.
- All of `/MordecaiCore` is in `DirectoriesToAlwaysCook` (`Config/DefaultGame.ini`). Do not remove it. New game content outside `/MordecaiCore` that is only reached from C++ needs its own cook rule or a `UPROPERTY` reference from an already-cooked asset.
- `playtest_boot.py`'s `content_cooked` check must PASS: it fails on any `SkipPackage` / `Failed to find object` for a `/MordecaiCore` path in the packaged game's log.

### NO PIE-ONLY BEHAVIOR (Locked)
Gameplay must set itself up the same way in PIE and in a packaged build.

#### DO NOT
- Make gameplay setup depend on `World->IsPlayInEditor()`, `GIsEditor`, `WITH_EDITOR` / `WITH_EDITORONLY_DATA`, `ULyraDeveloperSettings` (Lyra honors `ExperienceOverride` only in PIE), editor-only settings, or unsaved editor state.
- Fix a "works in PIE, broken in the package" bug with another PIE-only path.

#### DO
- Make every Mordecai map resolve the Mordecai experience in a packaged build: use `AMordecaiGameMode` (its `DefaultExperience`, `LyraExperienceDefinition:B_MordecaiArenaExperience`, applies in all builds when no URL option, command-line argument, or World Settings `DefaultGameplayExperience` names one), or set the map's World Settings `DefaultGameplayExperience`.
- Keep Mordecai maps under `/MordecaiCore/Game/Maps` (scanned + AlwaysCook in `Config/DefaultGame.ini`) and player-selectable entries as playlists (`LyraUserFacingExperienceDefinition`) under `/MordecaiCore/System/Playlists` — e.g. `DA_Mordecai_Arena`, created by `Scripts/create_arena_playlist.py`.
- Prove it: `py Scripts/playtest_boot.py --map <map path> --expect-experience <ExperienceName>` (package with `--all-maps` if the map is not in the default cook list). It fails if a different experience is identified — `B_LyraDefaultExperience` means the Mordecai experience did not load.
- Keep editor/debug conveniences purely additive: the packaged path must work without them.

### CONTENT IS COMMITTED (Locked)
Content is real only when it is **saved to disk and committed**. C++ that needs content is not a feature until that content ships with it.

#### DO
- Make every asset-creating editor script save what it creates (`unreal.EditorAssetLibrary.save_asset(path, only_if_is_dirty=False)`, `save_current_level()`, etc.) and confirm the files exist on disk.
- Commit the resulting `.uasset`/`.umap` files (Git LFS) in the **same branch/PR** as the scripts and C++ that need them.
- Run `py Scripts/check_content_committed.py` before calling a story done; it must exit `0`.
  - Check 1: no modified/untracked/deleted `.uasset`/`.umap` in the working tree.
  - Check 2: if asset-saving `Scripts/*.py` files were added/modified since `merge-base(origin/main, HEAD)` (or are modified/untracked in the working tree), that range (or the working tree) must also add/modify `.uasset`/`.umap` files.
  - `--base REF` changes the base; `--json` gives machine-readable output. Exit `1` = fail, `2` = git error / bad ref.

#### DO NOT
- Treat "the script exists" or "PIE passed" as "the content exists".
- Put `[no-content]` in a commit message unless the changed scripts genuinely produce no new content; say why in that commit's body. (A script is exempt from check 2 only if every commit in the range that touched it carries the tag and it has no uncommitted changes.)
- Blindly commit or discard assets that check 1 flags but you did not mean to change — investigate and report them.

---

## 4) Networking Replication: Iris (Locked)

### DO
- Use **Iris replication**.
- Design gameplay as **server-authoritative**.
- Optimize replication via Iris-compatible mechanisms:
  - replicated state minimization
  - update frequency control
  - relevance/scoping mechanisms supported by Iris
  - explicit replication fragments / state descriptors (as appropriate)

### DO NOT
- Implement or propose Replication Graph as the scaling strategy.
- Build systems that require RepGraph routing semantics.

---

## 5) Persistence Boundary (PlayFab Default)

### DO
- Use PlayFab for **persistent meta**:
  - identity/auth
  - profiles
  - account-level inventory/economy meta
  - matchmaking/lobby/party (as applicable)
- Keep **in-match** combat state in UE server runtime.

### DO NOT
- Add a second custom persistence database unless explicitly requested.

### NOTE
- Exact write timing for loot/xp/inventory deltas is still an open decision (see Open Items).

---

## 6) GAS Defaults (Applies Everywhere)

### DO
- Use GAS for:
  - Attributes
  - Abilities
  - Status effects (buffs/debuffs)
  - Gameplay Tags as shared vocabulary
- Prefer data-driven configuration (DataAssets / tables) over hardcoding.

### DO NOT
- Create parallel “status effect systems” outside GAS unless explicitly directed.

---

## 8) Map & World Building (Locked)

### Core Model
- The world is built from **authored map cells**, not open terrain or freeform scenic assembly.
- A cell is the smallest authored world chunk that stands on its own (one traversal idea, one combat/puzzle idea, 2–4 exits, optional landmark/secret).
- The overworld is a **room graph of connected cells**, not open sprawl.
- See `map_style_technical_design_v1.md` for full spatial rules and `agentic_map_pipeline_v1.md` for the production pipeline.

### Source-of-Truth Hierarchy
When editing world content, agents must respect this priority (highest first):
1. **Cell metadata asset** (`DA_Cell_*`)
2. **Gameplay markup layer** (`PM_*` actors)
3. **Blockout geometry**
4. **Encounter data**
5. **Art dressing**

If art conflicts with gameplay markup, **gameplay markup wins**.

### DO
- Create a `DA_Cell_<CellId>` metadata asset before placing any geometry for a new cell.
- Use the gameplay markup layer (`PM_WalkableArea`, `PM_Wall`, `PM_Gate`, etc.) as the source of truth for walkability, collision, ledges, and cover.
- Use **clear height tiers** (`Tier 0: Ground`, `Tier 1: Ledge`, `Tier 2: WallTop`, `Tier -1: Lower`) — each cell declares which tiers it uses.
- Follow the 7-stage pipeline: World Intent → Cell Planning → Blockout → Markup → Encounter → Dressing → Validation.
- Use standardized combat pocket archetypes (`OpenPocket`, `LanePocket`, `RingPocket`, `CrossroadsPocket`, `AmbushPocket`, `HazardPocket`, `ElevationPocket`, `BossAntechamber`).
- Keep combat pocket centers **clear** — clutter goes at edges.
- Use snap-friendly structural grids for gameplay footprints; allow organic variation only for visual dressing.

### DO NOT
- Build a room or cell without the metadata asset first.
- Treat decorative mesh placement as the source of truth for walkability, cover, ledges, jump rules, or path width.
- Build large vague fields with no authored purpose — every space needs a clear identity.
- Use arbitrary slope sculpting unless the biome explicitly requires it.
- Place tall decorative assets where they hide enemies, cover interactables, or obscure gates from the fixed camera.

### Human Approval Required
Agents must **not** autonomously change:
- biome graph structure
- gate type introduction
- height tier rules
- connection socket standards
- progression sequence

Agents **may** act autonomously for:
- blockout refinement within a cell
- dressing polish
- metadata completion
- local validation fixes
- anchor placement adjustments

### Naming Conventions (World Assets)
- Cell metadata: `DA_Cell_<BiomeAbbrev>_<Number>` (e.g., `DA_Cell_VW_01`)
- Level instance: `LI_Cell_<BiomeAbbrev>_<Number>`
- Encounter data: `DA_Encounter_<BiomeAbbrev>_<Number>`
- Gameplay markup: `PM_<Type>_<Variant>` (e.g., `PM_Gate_Explosives_A`)

### Content Folder Structure
```
/Content/World/Biomes/<BiomeName>/Cells/
/Content/World/Biomes/<BiomeName>/Kits/
/Content/World/Biomes/<BiomeName>/Encounters/
/Content/World/Biomes/<BiomeName>/Data/
/Content/World/Biomes/<BiomeName>/Validation/
/Content/WorldShared/CellFramework/
/Content/WorldShared/TraversalMarkers/
/Content/WorldShared/GateMarkers/
/Content/WorldShared/Landmarks/
/Content/WorldShared/Validation/
```

---

## Open Items — Do Not Guess (Scaffold Only)

Agents may create interfaces and TODOs, but must **not finalize behavior** without user input:

1) **Gameplay tag taxonomy & naming rules**
   - namespaces, ownership, review/approval flow

2) **Buff/debuff stacking policy**
   - refresh vs stack behavior
   - unique tags
   - magnitude stacking rules

3) **PlayFab ownership details**
   - authoritative inventory mutation path
   - match end writeback timing
   - loot/xp grant flow

4) **Unidentified item flow details**
   - whether appraisal can happen per-item, batch, or via time-based town process
   - whether identification has a gold/resource/service cost
   - whether unidentified items expose partial hints before identification

5) **Potion field-cap rule exactness**
   - whether potion carry is a hard cap, soft stock cap, or refill-style provisioning rule

---

## 7) Persistence of Changes (Locked)

### DO
- Always make changes **permanent** — write to config files, save assets to disk, or put settings in C++ code.
- When modifying editor settings or blueprint assets via Python scripts, always call the appropriate save method (`EditorAssetLibrary.save_asset`, config file writes, etc.).
- If a change cannot be persisted (e.g., `save_config()` not exposed to Python), immediately implement an alternative permanent solution (C++ code, direct config file edit, etc.).
- **Commit what you save.** Saved `.uasset`/`.umap` files are committed (Git LFS) in the same branch/PR as the code and scripts that depend on them. `py Scripts/check_content_committed.py` must exit `0` before a story is called done (see §3 CONTENT IS COMMITTED).

### DO NOT
- Leave changes as **in-memory-only** fixes that revert on editor/tool restart.
- Rely on Python CDO hacks or runtime-only settings that won't survive a session restart.
- Declare a fix "done" if it only works until the next restart.
- Declare content "done" because an editor script ran or PIE passed — unsaved or uncommitted content is not in the packaged build.

---

## Implementation Behavior When Requirements Are Missing

### DO
- Add explicit `TODO(DECISION)` markers and keep the design flexible.
- Provide 1–2 minimal options in comments if needed, but do not choose.

### DO NOT
- Invent product rules or “pick what seems best” for open items.

---

## Quick Compliance Checklist (for PRs / agent output)

- [ ] UE version assumed: 5.7
- [ ] Camera follows character and does not rotate
- [ ] Inventory uses unlimited carry + auto-stored materials + town gating
- [ ] No bag slot system or specialized bag restrictions
- [ ] Build/test remains local-only (no BuildGraph/Horde/Gauntlet/CI pipelines)
- [ ] Every story: fresh package of the delivered commit (`py Scripts/package_playtest.py`), `py Scripts/playtest_boot.py` exit 0 without `--allow-stale`, every screenshot opened and described (player-facing stories: the feature is visibly on screen)
- [ ] Ladder respected: Tier 1/2 passes and direct-call checks (labeled logic/diagnostic) not cited as proof of playability; every input-driven AC accepted only by a passing Tier 4 playtest (`py Scripts/playtests/usXXX_<name>.py` exit 0 on the same fresh package, no `--allow-stale`, every `bridge_NN.png` opened and described); no `debug_*` or direct-call result cited as Tier 4 evidence
- [ ] No gameplay setup depends on `IsPlayInEditor()` / `WITH_EDITOR` / `LyraDeveloperSettings`; every Mordecai map resolves the Mordecai experience in the package
- [ ] Content saved + committed; `py Scripts/check_content_committed.py` exits 0
- [ ] Story has a Playtest section; playtest run dirs (Tier 3, and Tier 4 for input-driven ACs) cited in completion notes + commit body; failures reported as failures
- [ ] Replication uses Iris (no RepGraph)
- [ ] Persistence respects PlayFab boundary
- [ ] GAS used for abilities/attributes/effects/tags
- [ ] Open items left as TODO(DECISION), not guessed
- [ ] Map cells have metadata asset before geometry
- [ ] Gameplay markup is source of truth for collision/walkability (not art meshes)
- [ ] Height tiers used (not arbitrary slopes)
- [ ] Biome graph / gate / progression changes approved by human
