# Legacy pipeline automation prompts (NOT SCHEDULED)

These are the remaining prompts from the old OpenClaw automation. **Nothing runs them as of 2026-10-19.** All four OpenClaw Mordecai jobs (`mordecai-planner`, `mordecai-worker`, `mordecai-status-report`, `mordecai-nightly-package`) are disabled in `~/.openclaw/cron/jobs.json` (backup: `jobs.json.bak-20261019`). The OpenClaw scheduler itself had not run since 2026-04-12. Planning, sequencing, and scheduling now live in GitHub issues and epics owned by Jeff's coordinator.

- `worker-prompt.txt`: picks the next story and spawns a coding agent that follows the repo-root `CLAUDE.md`.
- `explorer-prompt.txt`: the exploratory playtest (`Specs/story_playtests_v1.md` §5.4). It runs `py Scripts/explore_playtest.py --minutes 20` on the existing package, then has Claude Code triage the report: open the screenshots behind every new finding, and file each confirmed bug in the Known issues backlog of `Specs/agent_playtest_plan_v1.md` or as a Playability Recovery story. Written for no particular scheduler; it was never an OpenClaw job.

Steps that send to Discord through "the message tool" rely on OpenClaw's tooling. Under any other scheduler they need a replacement notification step.

Choosing a new scheduler is a pending decision (see `Specs/agent_playtest_plan_v1.md`, Open decisions). Whatever runs these must be on the local machine, because the pipeline builds, cooks and plays Unreal. Budget at least 90 minutes for a run that packages from scratch.

The old `mordecai-nightly-package` job deleted everything under `PackagedBuilds\`, which includes the playtest build. Don't revive it as-is.
