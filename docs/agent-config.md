# Agent configuration: Claude Code and Cursor

Claude Code and Cursor agents both work on this repo, on Jeff's Windows PC: Claude Code directly, and Cursor in the desktop app, the CLI, and Cloud Agents that run on the self-hosted My Machines worker `ProjectMordecai-JEFFS-LAPTOP`. The Claude Code setup is the source of truth. Each Cursor counterpart is one of three things:

- **Shared:** Cursor reads the Claude file itself.
- **Generated:** `py Scripts/sync_cursor_config.py` derives it from the Claude file.
- **Cursor-only:** facts or enforcement that the Claude setup keeps outside the repo or doesn't need.

## What each tool loads

| Concern | Claude Code | Cursor | Kind |
|---|---|---|---|
| Rulebook | `CLAUDE.md` | `CLAUDE.md`: Cursor reads a root `CLAUDE.md` like `AGENTS.md`, always applied | Shared |
| Cursor facts and checklists | (not needed) | `.cursor/rules/*.mdc`: 2 always-applied rules, 6 scoped by `globs`, 1 by `description` | Cursor-only |
| Skills | `.claude/skills/<name>/` | `.cursor/skills/<name>/`. Cursor would load `.claude/skills/` on its own; with both present it lists the `.cursor/` copy once | Generated |
| MCP server `mordecai-play` | `.mcp.json` | `.cursor/mcp.json` | Generated |
| MCP approval | `enabledMcpjsonServers` in `.claude/settings.local.json` (local) | `agent mcp enable mordecai-play` (CLI), or Customize → MCP (IDE) | Per machine |
| Deny list (force push, `reset --hard`, `clean`) | `permissions.deny` in `.claude/settings.local.json` (local) | `.cursor/hooks.json` + `.cursor/hooks/guard_git.py` on every surface; also `.cursor/cli.json` `deny` and `.cursor/permissions.json` | Cursor-only |
| Allow list | `permissions.allow` in `.claude/settings.json` and `.claude/settings.local.json` (both local) | `.cursor/cli.json` `allow` (CLI), `.cursor/permissions.json` (IDE Auto-review); Cloud Agents never ask | Cursor-only |
| Memory | `~/.claude/projects/<project>/memory/` (per user) | none; its two standing instructions are in `.cursor/rules/mordecai-cursor-worker.mdc` | Cursor-only |
| Hooks, slash commands, subagents | none in the project | `.cursor/hooks.json` only | n/a |

Notes:

- `.claude/settings.json` is excluded in `.git/info/exclude` and `.claude/settings.local.json` is gitignored. Both stay local, because Claude Code writes to them at runtime and packages need a clean tree.
- The Cursor files are committed. Cursor keeps its own runtime state (MCP approvals, auth) under `~/.cursor/projects/`, and the CLI writes "always allow" choices to `~/.cursor/cli-config.json`, so neither dirties the tree.
- Cursor also loads the user-level `~/.claude/skills/` and, while Settings → Agents → Third-Party Imports is on (the default), Claude Code hooks from `.claude/settings*.json` and `~/.claude/settings.json`. On this machine that means the macOS `afplay` sound hooks in `~/.claude/settings.json`, which fail harmlessly on Windows. No project Claude hook exists.

## Keeping them in sync

1. **Edit the Claude side**, never a generated file: a skill (for example a new `unreal-tribal-knowledge` lesson) in `.claude/skills/`, an MCP server in `.mcp.json`.
2. **Regenerate**: `py Scripts/sync_cursor_config.py`.
   - Every skill file is copied as is. Each `SKILL.md` gains Cursor `metadata` and a one-line "generated" banner.
   - In `.mcp.json` args, a repo-relative script path becomes `./<path>`.
   - A generated skill whose source is gone is removed; Cursor-only skills and MCP servers are kept.
3. **Check, then commit both sides together.** `py Scripts/sync_cursor_config.py --check` changes nothing; it exits 0 when the copies match and 1 on drift, listing the files.
   - `CLAUDE.md` ("When to read which skill") tells every agent, Claude Code and Cursor alike, to run the sync and `--check` after any change to `.claude/skills/` or `.mcp.json`.
   - Nothing runs the check automatically: by Jeff's decision (2026-11-01), the Python suite does not test for drift.
4. **After an MCP entry changes, approve it again**: Cursor keys approvals by a hash of the entry (`agent mcp enable mordecai-play`, or approve it in Customize).
5. **Rules:** change a rule in `CLAUDE.md` or `Specs/agent_rules_v2.md`, not in `.cursor/rules/`. `mordecai-core-workflow.mdc` names `CLAUDE.md`'s Steps 1–7 and sections, so renumbering or renaming those means updating it.
6. **Deny list:** a change to Claude's deny list also goes into `guard_git.py` and its tests (`Scripts/tests/test_cursor_git_guard.py`), the `deny` of `.cursor/cli.json`, and `.cursor/permissions.json`.

## Why `./Scripts/...` in `.cursor/mcp.json`

Cursor's docs describe `${workspaceFolder}` interpolation. On this machine (Cursor CLI 2026.10.22, which the worker runs), the CLI passed `${workspaceFolder}/Scripts/mordecai_play_mcp.py` to Python unexpanded, and the server failed to start. The two implementations differ:

- **CLI and worker:** start stdio servers in their own working directory, the repo root here, because `start-worker.ps1` runs `Set-Location` into the repo.
- **Desktop app:** starts them in the user's home folder, but rewrites every argument that starts with `./` to the workspace folder.

`./Scripts/mordecai_play_mcp.py` is the one form that resolves in both.

## Verified on this machine (2026-11-01)

- **A fresh Cursor agent session** (`agent -p` in the repo, CLI 2026.10.22, the worker's binary):
  - Its always-applied rules were `mordecai-cursor-worker.mdc`, `mordecai-core-workflow.mdc` and `CLAUDE.md`.
  - It listed each Unreal skill once, from `.cursor/skills/`.
  - It had the 13 `mordecai-play` tools.
  - Its `git -C . clean -n -- <probe>` was blocked: "Blocked by .cursor/hooks/guard_git.py: git clean is on Project Mordecai's git deny list".
- **Cursor's own loader:** `agent mcp list` reports `mordecai-play: ready`, and `agent mcp list-tools mordecai-play` lists its 13 tools.
- **IDE launch path:** the server started the way the desktop app starts it (the `./` argument expanded, home folder as working directory). It answered `initialize` as `mordecai-play` v1, listed 13 tools, and returned `session_info` without launching the game.
- **Hook:** `guard_git.py` passes 33 deny cases and 21 allow cases, and answers correctly when its JSON input is piped through PowerShell, the way Cursor runs hooks on Windows.

## Not mapped

- **`Workflow` in Claude's allow list, `.claude/scheduled_tasks.lock`, and the Claude runtime paths in `.git/info/exclude`:** Claude Code internals with no Cursor counterpart.
- **Blanket allows** (bare `Bash`, `PowerShell`, `Read`, `Glob`, `Edit(/**)`), which make Claude run everything except the deny list: not committed, because this repo is public. For the same autonomy in the Cursor CLI, add `Shell(*)` and `Write(**)` to your own `~/.cursor/cli-config.json`, or run `agent --force`. Cloud Agents already run without approval.
- **Claude's per-user memory:** Cursor has no file-based project memory. The per-issue entries repeat what `stories/PLAN.md` and `Specs/agent_playtest_plan_v1.md` already record.
- **`stories/agent-prompts/` and `Scripts/explore_playtest.py`:** they drive the `claude` CLI and depend on Claude-only flags (`--strict-mcp-config`, `--allowedTools`, `--max-budget-usd`) and its stream-json transcript. Nothing schedules them, and the choice of scheduler is still open (`Specs/agent_playtest_plan_v1.md`, "Open decisions"). The Cursor equivalents would be Cursor Automations or `agent -p`.
