# Story Playtests and Exploratory Play v1

**Status:** Phase 3 of `Specs/agent_playtest_plan_v1.md`. This is the contract between the playtest kit (`Scripts/playtest_kit.py`), the per-story Tier 4 scripts (`Scripts/playtests/usXXX_<name>.py`), the story runner (`Scripts/playtest_story.py`), the story template's Playtest section (`stories/PLANNER.md` §7) and the exploratory agent (`Scripts/explore_playtest.py`, `Scripts/explorer/charter.md`, the `mordecai-play` MCP server).
**Builds on:** `Specs/agent_bridge_v1.md` (bridge protocol, Python client, MCP server).
**Last updated:** 2026-10-29 (test maps per script, the kit's `map` check, stories with several scripts: US-100, issue #46).

## 1. What Phase 3 adds

| Piece | File | What it does |
|---|---|---|
| Playtest kit | `Scripts/playtest_kit.py` | The run framework every Tier 4 script shares: launch/attach, STALE gate, checks, step helpers that map 1:1 to bridge calls, screenshots, crash and log checks, reports, AC acceptance, exit codes |
| Story scripts | `Scripts/playtests/usXXX_<name>.py` | One per player-facing feature: a `PLAN` of steps and the code that plays them |
| Story runner | `Scripts/playtest_story.py` | `US-XXX` → its Tier 3 boot run + its Tier 4 script on the same package, one summary with per-AC acceptance; `--all` is the Playability Recovery sweep |
| Executable Playtest section | `stories/PLANNER.md` §7 | The story's Steps table is generated from the script's `PLAN` (`--plan`) and checked against it (`--lint`) |
| Exploratory agent | `Scripts/explore_playtest.py`, `Scripts/explorer/charter.md` | A headless Claude Code session plays the packaged game through the `mordecai-play` MCP tools for N minutes and files findings; the harness adds automatic detectors and writes a report |
| MCP additions | `Scripts/mordecai_play_mcp.py` | `report_finding`, `session_info`, `--run-dir`, `--budget-s` |
| Pipeline prompts | `stories/agent-prompts/` | Worker runs Tier 1→4 per story; the explorer runs nightly after the planner; the planner keeps per-feature Playable Status from the sweep |

**The rules do not change:** real input only (`key`, `axis`, `mouse`, `click`); acceptance only from a script run that launched a FRESH package built from a clean, committed tree; a FAIL is a game bug to fix or file, never something to work around in a script. Phase 3 makes those rules cheap to follow for every story.

## 2. The playtest kit (`Scripts/playtest_kit.py`)

Standard library only (plus optional Pillow), like `agent_bridge.py`. It is the framework `Scripts/playtests/us079_inventory.py` grew; us079 is refactored onto it with unchanged behaviour (its offline tests keep passing).

### 2.1 A story script

```python
import playtest_kit as kit
from playtest_kit import Step

class Run(kit.PlaytestRun):
    STORY = "US-078"                       # owning story; the run dir suffix defaults to "us078"
    COVERS = ("US-077",)                   # other stories whose ACs this script proves
    FEATURES = ("weapons", "weapon_hud")   # Playable Status is tracked per feature
    TITLE = "US-078 weapon cycling playtest (Tier 4, real input)"
    MAP = kit.COMBAT_TEST_MAP              # or kit.MOVEMENT_TEST_MAP, kit.DEFAULT_MAP (DevTestMap); or FRONTEND = True
    EXPERIENCE = kit.DEFAULT_EXPERIENCE
    SCREENSHOTS = {"start": "...", "cycled": "..."}   # label -> what the frame must show
    KNOWN_ISSUES = ("...",)                # informational, listed in the report
    PLAN = (
        Step("hud_visible", "Boot: the combat HUD is on screen", proves=("AC-078.9",)),
        Step("cycle_next", "tap Tab -> player.weapon.index changes and the weapon HUD text follows",
             proves=("AC-078.4", "US-077:AC-077.2")),
        Step("cycle_prev", "tap BackSlash -> back to the first weapon", proves=("AC-078.5",),
             known_issue="..."),            # expected to FAIL today (still a FAIL; see 2.4)
    )
    PENDING = {"AC-078.7": "bridge cannot drive: gamepad shoulder buttons are not mapped"}

    def steps(self):
        self.expect("hud_visible", 'pickups.prompt.hint_visible==true', timeout_s=5)
        self.shot("start")
        ok, _ = self.press("cycle_next", "Tab", then="player.weapon.index==1", timeout_s=2)
        self.press("cycle_prev", "BackSlash", then="player.weapon.index==0", timeout_s=2,
                   needs=ok, needs_why="not run: cycle_next failed")
        self.shot("cycled")

if __name__ == "__main__":
    raise SystemExit(kit.main(Run))
```

### 2.2 `PlaytestRun` class attributes

| Attribute | Default | Meaning |
|---|---|---|
| `STORY` | required | Owning story id, `US-NNN` |
| `COVERS` | `()` | Other story ids whose ACs `PLAN` proves (qualified ids, 2.3) |
| `FEATURES` | `()` | Feature names (vocabulary in 6.2) |
| `TITLE` | `"<STORY> playtest (Tier 4, real input)"` | Report heading |
| `RUN_SUFFIX` | `"us" + digits of STORY` | Run dir suffix: `Saved/Playtests/<YYYYmmdd-HHMMSS>-<suffix>/`. It must be unique across scripts: a story's second script sets its own (us054_combat.py: `"us054combat"`), and `--lint` of the owning story fails on a duplicate |
| `MAP` / `EXPERIENCE` / `FRONTEND` | DevTestMap / `B_MordecaiArenaExperience` / `False` | Launch path. `FRONTEND = True` launches the front end and expects `B_LyraFrontEnd_Experience`. Every script declares its `MAP` explicitly (US-100). The kit's `map` check (2.3) FAILs a run that loaded another map. The constants come from `playtest_boot.py`, re-exported by the kit; every map below is in the default package cook and resolves `B_MordecaiArenaExperience`:<br>• `kit.MOVEMENT_TEST_MAP` (`/MordecaiCore/Game/Maps/MovementTestMap`; no enemies, lane L1, steps): us001_movement.py, us083_combat_input.py, us069_pause_menu.py<br>• `kit.COMBAT_TEST_MAP` (`/MordecaiCore/Game/Maps/CombatTestMap`; TestDummy at (600, 600), TestAttacker at (2100, 0)): us054_combat.py, us057_magic.py, us078_weapons.py<br>• `kit.DEFAULT_MAP` (`/MordecaiCore/Game/Maps/DevTestMap`; three Frontliners, six pickups): us054_arena.py, us079_inventory.py |
| `SCREENSHOTS` | `{}` | label → what the frame must show (copied into the report for the reader) |
| `KNOWN_ISSUES` | `()` | Strings listed in the report |
| `PLAN` | required | Ordered `Step`s (2.3) |
| `PENDING` | `{}` | AC id → `"bridge cannot drive: <reason>"` (reported, never claimed) |
| `WALK_TOL`, `TAP_S` | `60.0`, `0.1` | Defaults for the step helpers |

### 2.3 `Step` and the PLAN

`Step(check, does, proves=(), kind="gameplay", known_issue=None)` (a frozen dataclass):
- `check`: the check name the script records (snake_case, unique in the plan).
- `does`: one line, the player's action and the expected result, in bridge terms (`walk_to(400, 0)`, `tap F`, `wait_until pickups.focused.label=="..."`). It is the Steps table row (§4).
- `proves`: AC ids this check proves. An id is written exactly as in the story file (`AC-078.4`, `AC-1.2.3`, `AC3`). An AC of a story other than `STORY` is qualified: `US-077:AC-077.2`.
- `kind`: `gameplay` or `infra` (as in `playtest_boot` checks).
- `known_issue`: set when the check is EXPECTED to FAIL today because of a filed bug (cite the Known issues entry). It never changes the check's status or the exit code; the report and the sweep mark the FAIL as `known` instead of `NEW`, and a PASS of such a check is flagged `FIXED?` (the bug may be gone: re-check the backlog).

The kit adds its own checks around the plan (they need no `Step`): `build_fresh`, `launch` | `attach`, `bridge_ready`, `experience`, `map`, `screenshot_<label>`, `no_crash`, `content_cooked`, `log_errors`, `completed` (only on a script error or Ctrl+C).
- `map` (US-100) compares the loaded map (`ping.map`, the long package name) with the script's launch map (`launch_map()`). It strips any `.Object` suffix from either and compares case-insensitively.
  - A mismatch is a gameplay FAIL. It blocks the plan as a wrong experience does: every map shares `B_MordecaiArenaExperience`, so the experience check alone cannot tell them apart.
  - A `FRONTEND` run is not compared; the check records INFO.
  - Attach mode compares too.

Plan enforcement:
- A check recorded under a name not in `PLAN` and not a kit check is still recorded, and the report lists it under `undeclared_checks` (a WARN line in `report.md`): fix the script.
- After the run, every `PLAN` check that was never recorded is added as `SKIP` with `not reached`, so the report always lists the whole plan.
- `PLAN` may be built with comprehensions (us079 builds four steps per pickup).

### 2.4 AC acceptance in the report

`report.json["acs"]` has one entry per AC named in any `proves` or in `PENDING`:

```json
{"id": "AC-078.4", "story": "US-078", "checks": ["cycle_next"], "status": "PASS",
 "accepted": true, "known_issue": null, "pending": null}
```

- `status`: `PASS` (every check PASS, or WARN/INFO, and at least one PASS) | `FAIL` (any FAIL) | `NOT RUN` (no FAIL, any SKIP) | `PENDING` (in `PENDING`, never claimed).
- `accepted`: `status == "PASS"` AND the run's evidence is `ACCEPTANCE CANDIDATE` (a launched, fresh, clean-tree package, no `--allow-stale`, exit 0). A diagnostic run never accepts anything.
- An AC is never accepted by a check of kind `infra` alone.

### 2.5 Step helpers (1:1 with bridge calls)

Each helper records exactly one check, returns `(ok, data)` like `attempt()`, and takes `needs=True, needs_why=None` (a failed prerequisite SKIPs it) plus `kind="gameplay"`. Every wait FAILs on timeout with the last values of every path in its condition. All input goes through `agent_bridge.Bridge`'s real-input methods.

| Helper | Bridge calls | PASS when |
|---|---|---|
| `walk(check, x, y, tol=WALK_TOL, timeout_s=None)` | `walk_to` | within `tol` of (x, y) |
| `expect(check, condition, timeout_s=5)` | `wait_until` | condition true in time |
| `press(check, key_or_keys, then=None, timeout_s=3, hold_s=TAP_S)` | `key tap`, then `wait_until then` | the tap succeeded and `then` (if given) became true |
| `hold(check, key_or_keys, seconds, then=None, timeout_s=3)` | `key tap` with `hold_s=seconds`, then `wait_until` | as `press` |
| `axis(check, values, hold_s, then=None, timeout_s=3)` | `axis`, then `wait_until` | as `press` |
| `click_button(check, label=None, button_id=None, then=None, timeout_s=3)` | `state` (aim), `click`, then `wait_until` | the button was on screen, clicked, and `then` became true |
| `click(check, x, y, button="Left", then=None, timeout_s=3)` | `click`, then `wait_until` | as `press` |
| `observe(check, sections, predicate, describe, timeout_s=3)` | `state` polled (`Bridge.wait_for`) | `predicate(state)` true in time; `describe(state)` is the detail |
| `measure(check, fn)` | anything read-only | `fn(bridge) -> (status, detail, data)`, the escape hatch for compound checks |
| `shot(label, needs=True, needs_why=None)` | `screenshot` | file written; a near-uniform frame is a WARN (no visual evidence) |

`self.read(sections)` returns a `state` dict for computing expectations (it records no check). `self.attempt(name, fn, kind, needs, needs_why)` stays available for anything the helpers do not cover; `fn()` returns `(status, detail, data)`.

The helpers never call a `debug_*` command, never send `route:"viewport"`, and never use a denied debug key (the bridge refuses them anyway).

### 2.6 The run

`kit.main(RunClass, argv=None)` parses the common CLI and returns the exit code:

```
--allow-stale        STALE package, diagnostic only (recorded)
--keep-running       leave the launched game running (prints the quit command)
--attach-port N [--token T]   play an already running game (always diagnostic)
--timeout S          experience load timeout (240)
--exe PATH           game binary (default: the packaged playtest)
--run-dir DIR        (hidden) write the run here instead of a new timestamped dir
--plan               print the PLAN as the story's Steps table (markdown) and exit 0; no game
--extra-arg, --ready-timeout, --test-exe-argv, --test-skip-freshness   (hidden, as in us079)
```

`execute()`: STALE gate (refused before a run dir exists, exit 2), run dir, `start()` (launch or attach, `build_fresh`, `bridge_ready`, `experience`, `map`; a wrong experience or a wrong map SKIPs the rest), the subclass's `steps()`, then `finish()` (final focus/state, `no_crash`, quit or detach, `game.log` → `content_cooked` and `log_errors`, plan completion, reports, summary). A lost bridge connection SKIPs everything after it. Exit codes: 0 every check passed (WARN allowed), 1 a gameplay FAIL, 2 infra.

Hooks for subclasses: `report_extra() -> dict` (merged into `report.json`), `md_sections() -> list[str]` (markdown inserted after the Checks table; us079 uses both for its Pickups and Inventory sections).

### 2.7 Reports

`Saved/Playtests/<YYYYmmdd-HHMMSS>-<suffix>/` holds `report.md`, `report.json`, `game.log`, `bridge.jsonl` and `bridge_NN.png`, exactly as in `Specs/agent_bridge_v1.md` §8. `report.json` adds to the us079 fields: `story`, `covers`, `features`, `plan` (`[{check, does, proves, kind, known_issue}]`), `acs` (2.4), `undeclared_checks`, `known_failures` (FAIL checks with a `known_issue`), `new_failures` (other FAIL checks), `fixed_candidates` (PASS checks with a `known_issue`). `report.md` adds an "Acceptance criteria" table (AC | checks | status | accepted) and marks each FAIL `known` or `NEW`.

## 3. The story runner (`Scripts/playtest_story.py`)

```
py Scripts/playtest_story.py US-078            Tier 3 boot (the script's launch path) + the Tier 4 script, same package
py Scripts/playtest_story.py US-078 --tier4-only
py Scripts/playtest_story.py US-078 --plan     the Steps table for the story's Playtest section (offline)
py Scripts/playtest_story.py US-078 --lint     story file <-> script consistency (offline)
py Scripts/playtest_story.py --all             the sweep: every Tier 4 script, one Tier 3 boot per launch path
py Scripts/playtest_story.py --all --only US-054,US-078
py Scripts/playtest_story.py --list            stories, their scripts, the newest run of each
Options: --allow-stale (diagnostic only; passed to every child and recorded)
```

**Finding a story's script.** Import every `Scripts/playtests/us*.py` (import has no side effects: `main` is guarded) and read its `Run` class (the one `PlaytestRun` subclass in the module). A story maps to every script whose `STORY` equals it (its owners). Only when it has no owner does it map to the scripts whose `COVERS` contain it. None → exit 2 with "US-XXX has no Tier 4 playtest script" (a story with no input-driven AC needs none; then run `playtest_boot.py` alone).

**Stories with several scripts** (US-100). One script launches one map, so a story whose checks need two maps owns two scripts. US-054 owns `us054_arena.py` (DevTestMap, enemy AI) and `us054_combat.py` (CombatTestMap, melee/dodge/death/respawn/reset). Such a story:
- has one Steps table per script in its Playtest section, each directly after the `<!-- Scripts/playtests/<script>.py -->` marker that `--plan` prints before it. `--lint` compares each script's PLAN with the table under its own marker. A story with no markers keeps the single-script rule: its first Steps table is compared with its one script.
- gives every script a distinct `RUN_SUFFIX` (the run dirs and the `tier4-<suffix>.log` console logs are keyed by it). `--lint` of a story fails when one of its scripts shares its `RUN_SUFFIX` (case-insensitive) with another discovered script; `test_the_real_scripts_have_unique_run_suffixes` checks every real script.
- is accepted per AC across all its runs: a FAIL of any check that proves an AC, in any of the story's scripts' runs, beats a PASS elsewhere, and the AC is not accepted.

**Story files.** `stories/{backlog,in-progress,done}/US-XXX-*.md`. AC ids are parsed from checklist lines in the Acceptance Criteria section: `- [ ] AC-079.3: ...`, `- [x] **AC-054.1**`, `- [ ] AC3 ...` (the id is the first `AC`-token on the line).

**One run** (`US-XXX`):
1. The STALE gate once (exit 2 unless `--allow-stale`).
2. Tier 3: `py Scripts/playtest_boot.py` once per distinct launch path of the story's scripts (`--frontend`, or `--map M --expect-experience E` when not the default), in a subprocess. Its run dir is the newest `*-boot` dir created after the start (or the `Run dir:` line it prints).
3. Tier 4: each of the story's scripts in a subprocess with `--run-dir <dir>`, paired with the boot of its own map.
4. `Saved/Playtests/<YYYYmmdd-HHMMSS>-story-usXXX/summary.md` and `summary.json`: the build (commit, fresh), every run dir with its exit code and evidence line, per-AC acceptance of the story (from the Tier 4 `acs`; with several scripts a FAIL anywhere wins), every screenshot path to open (`engine_loading.png`, `engine_NN.png`, `bridge_NN.png`), known vs NEW failures, and the suggested Playable Status line per feature (6.3).
5. Exit: 0 both passed, 1 a gameplay FAIL in either, 2 infra in either (infra wins).

**The sweep** (`--all`): one STALE gate; one Tier 3 boot per distinct launch path (DevTestMap unless `--only`, plus each map or front end that a script launches: today MovementTestMap and CombatTestMap); every Tier 4 script in story order, sequentially (one game at a time); `Saved/Playtests/<ts>-sweep/summary.md|json` with a row per script (story, features, PASS/FAIL counts, NEW failures, known failures, run dir), the per-feature Playable Status lines ready to paste into `stories/PLANNER_LOG.md`, and every screenshot to open. A script that crashes or exits 2 does not stop the sweep. Exit: 2 if any child had an infra failure, else 1 if any FAIL, else 0.

**`--plan`** prints the script's PLAN as the Steps table (§4), one table per script, each after its `<!-- Scripts/playtests/<script>.py -->` marker. **`--lint`** (offline) exits 0 only when:
- the story file exists, and its Playtest section names every script path;
- every `proves` and `PENDING` id of each script exists in the named story's AC list;
- each script's Steps table (the one under its marker, or the first table for a single-script story with no markers), if present, lists exactly that plan's checks in order;
- none of the story's scripts shares its `RUN_SUFFIX` (case-insensitive) with another discovered script. A clash between two other stories' scripts fails their lint, not this one's; `test_the_real_scripts_have_unique_run_suffixes` (offline) checks every real script.

It also prints, as information, the story's ACs that no step proves (logic ACs are expected there).

## 4. The executable Playtest section

The story template's Playtest section (`stories/PLANNER.md` §7) gains two fields:

```markdown
- **Tier 4 script:** `Scripts/playtests/usXXX_<name>.py` — run: `py Scripts/playtest_story.py US-XXX`
- **Steps:** (generated: `py Scripts/playtest_story.py US-XXX --plan`; checked: `--lint`)

| # | Check | Player step (bridge calls) → expected | Proves |
|---|---|---|---|
| 1 | `walk_HealthPotion` | `walk_to(400, 0)` → within 60 cm | AC-079.2 |
| 2 | `pickup_HealthPotion` | `tap F` → `wait_until` the pickup leaves `pickups.world` and inventory grows | AC-079.2 |
```

The table is the script's `PLAN`, so the story's steps and the script's checks cannot drift: `--lint` fails when they do. A story with several scripts pastes the whole `--plan` output: one table per script, each under its `<!-- Scripts/playtests/<script>.py -->` marker (§3). Each input-driven AC names its check(s) (`verified by check \`cycle_next\` of Scripts/playtests/us078_weapons.py`), and the run's `acs` table says whether it was accepted.

## 5. The exploratory agent

### 5.1 What it is

A headless Claude Code session (`claude -p`) whose only tools are the `mordecai-play` MCP tools. It free-plays the packaged game for N minutes under a charter (`Scripts/explorer/charter.md`): move everywhere, fight, cast every spell, cycle weapons, open every menu and tab, combine inputs (menu while moving, spells during a menu, rapid taps), and look at every screenshot. It reports crashes, stuck states, wrong or missing UI, errors, and anything a player would call a bug, through `report_finding`. It is exploration, not acceptance: nothing it does accepts an AC.

### 5.2 MCP server additions (`Scripts/mordecai_play_mcp.py`)

- `--run-dir DIR`: the launched game's run dir (otherwise `Saved/Playtests/<ts>-mcp/`). `findings.jsonl` is written there.
- `--budget-s S`: a play-time budget counted from the first `launch_game`. Every tool result then ends with `time left: Ns`. After the budget, input tools (`press_key`, `hold_key`, `walk_to`, `click`, `click_button`, `wait_until`) are refused with "time budget used up: report your findings and call quit_game"; `get_state`, `screenshot`, `report_finding`, `session_info`, `release_key` and `quit_game` still work.
- Tool `report_finding(severity, title, details, repro=None, expected=None, known_issue=None)`:
  - `severity`: `crash` | `blocker` | `major` | `minor` | `cosmetic` | `question`.
  - Appends one JSON line to `<run_dir>/findings.jsonl`: `{id, time, severity, title, details, repro, expected, known_issue, screenshot (the newest bridge_NN.png), player_location, map, build_commit, source: "agent"}` and returns its id (`F01`, `F02`, ...).
  - It works without a running game (the finding is still written, with `player_location: null`) as long as a run dir exists; before any launch it is refused.
- Tool `session_info()`: run dir, build commit and freshness banner, play time used and left, number of findings so far, screenshots taken.

### 5.3 The harness (`Scripts/explore_playtest.py`)

```
py Scripts/explore_playtest.py                       10 minutes on DevTestMap
py Scripts/explore_playtest.py --minutes 20 --focus "spells and the pause menu"
py Scripts/explore_playtest.py --frontend            start at the front end
py Scripts/explore_playtest.py --allow-stale         a stale package (findings are against the packaged commit; recorded)
py Scripts/explore_playtest.py --dry-run             print the claude command and the prompt, run nothing
Options: --model M, --max-budget-usd D (default 10), --charter FILE, --agent-cmd JSON (tests)
```

1. STALE gate (exit 2 unless `--allow-stale`; a stale run's report names the packaged commit and says the findings apply to it). `claude` on PATH (else exit 2).
2. Run dir `Saved/Playtests/<YYYYmmdd-HHMMSS>-explore/`; writes `mcp_config.json` there: the `mordecai-play` server with `--run-dir <run_dir> --budget-s <minutes*60>`.
3. The prompt: the charter, the time budget, the focus (if any), the Known issues backlog section of `Specs/agent_playtest_plan_v1.md` (so known bugs are marked `known_issue`, not re-filed as new), the live bindings (`Specs/agent_bridge_v1.md` §5) and the run dir.
4. Runs `claude -p <prompt> --mcp-config <run_dir>/mcp_config.json --strict-mcp-config --tools "" --allowedTools "mcp__mordecai-play__*" --permission-mode dontAsk --output-format stream-json --verbose --no-session-persistence --max-budget-usd D [--model M]` in a kill-on-close job, stdout to `agent.jsonl`, stderr to `agent.stderr.log`. Wall clock: `minutes*60 + 300` s, then the tree is killed (the MCP server dies with it, and the game with the server).
5. Automatic detectors over the run dir (they need no agent):
   - `crash`: crash markers in `game.log` (`playtest_boot.CRASH_MARKERS`), or the game exited while the agent was still playing.
   - `stuck`: `walk_to` failures (`stuck at (x, y)`), `wait_until` timeouts, lost connections, from `bridge.jsonl`.
   - `log_errors`: `game.log` errors and warnings (`playtest_boot.summarize_log`), `LogMordecai*` errors listed first.
   - `missing_content`: `/MordecaiCore` packages requested but not cooked.
   - `blank_frames`: near-uniform `bridge_NN.png` (`playtest_boot.analyze_image`).
6. `report.md` / `report.json`: build and freshness, duration, agent exit and cost (from the stream's `result` event), tool-call counts, the findings table (agent findings with severity, known vs new; then the detector findings), the agent's final summary text, every screenshot path (open them), transcript paths. Exit: 0 no `crash`/`blocker` finding and no detector crash; 1 any `crash` or `blocker` finding (agent or detector), or any new `major`; 2 infra (no claude CLI, STALE without `--allow-stale`, the agent never launched the game).

### 5.4 Nightly

`stories/agent-prompts/explorer-prompt.txt` holds the scheduler-agnostic nightly step: after the planner, `py Scripts/explore_playtest.py --minutes 20`; read `report.md`, open the screenshots behind every new finding, and file each confirmed new bug in the Known issues backlog of `Specs/agent_playtest_plan_v1.md` (or as a recovery story when it breaks a done feature). Nothing is scheduled yet: choosing a scheduler is Jeff's open decision (`Specs/agent_playtest_plan_v1.md`, Open decisions).

## 6. Playable Status

### 6.1 Per feature, from runs

A feature is PLAYABLE only from a fresh-package run. The runner and the sweep write the line `stories/PLANNER.md` §6 defines, per feature in the script's `FEATURES`:
- `PLAYABLE (input) (Tier 4 Saved/Playtests/<run>-usXXX/report.md + Tier 3 Saved/Playtests/<run>-boot/report.md @ <commit>)` when both runs are acceptance candidates with exit 0.
- `NOT PLAYABLE (playtest FAIL: <failed checks, NEW first>; Tier 4 <run dir> @ <commit>)` when the Tier 4 run failed.
- `PLAYABLE (boot) — input not yet accepted (<reason>)` when only Tier 3 passed (Tier 4 not run, diagnostic, or PENDING ACs).
- A diagnostic run (stale, dirty, attached, `--allow-stale`) never yields PLAYABLE; its line says `DIAGNOSTIC`.

### 6.2 Feature vocabulary

movement, camera, melee_attack, combo, dodge, block_parry, posture_stagger, stamina, sprint, projectiles_aim, attributes, skills, feats, status_effects, spells, spell_hud, weapons, weapon_hud, inventory, pickups, identification, enemies, enemy_ai, combat_hud, death_respawn, kill_counter, damage_numbers, enemy_indicators, combo_counter, pause_menu, character_sheet, skill_tree_ui, feat_ui, front_end_menu.

### 6.3 The recovery sweep

`py Scripts/playtest_story.py --all` on a fresh package is the Playability Recovery epic's measurement (`Specs/agent_playtest_plan_v1.md` Phase 3 item 2). Every NEW failure is a game bug: it becomes a recovery story (one per broken feature, normal template) or a Known issues entry; every FAIL with a `known_issue` already has one. The sweep's Playable Status lines go into `stories/PLANNER_LOG.md`.

## 7. Testing the tooling offline

`py Scripts/run_python_tests.py` runs everything under `Scripts/tests/` with fakes only:
- `test_playtest_kit.py`: the kit against `tests/fake_bridge.py` (plan enforcement, SKIP propagation, AC acceptance, known/NEW failures, `--plan`, reports, exit codes).
- `test_us079_playtest.py`: unchanged expectations after the refactor.
- `test_<story>_playtest.py`: each story script against `FakeWorld` (subclassed in the test file when a feature needs behaviour the shared fake lacks).
- `test_playtest_story.py`: story/AC parsing, script discovery, `--lint`, `--plan`, one run and the sweep with fake children (`tests/fake_game.py`, or stub scripts), summary contents and exit codes.
- `test_explore_playtest.py`: the harness with a fake agent command (`--agent-cmd`) that drives the MCP session or writes `findings.jsonl` and a stream; detectors on synthetic run dirs; report and exit codes.
- `test_mordecai_play_mcp.py`: `report_finding`, `session_info`, the budget.
