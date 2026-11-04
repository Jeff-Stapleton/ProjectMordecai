# Tin Tally — the Mordecai UI design system

Tin Tally is the UI design system for Project Mordecai: main menus, in-game menus and the HUD, built from the
specs in `Specs/` and the UI code in `Source/LyraGame/Mordecai/UI/`. The interface is "the Tally", a pressed-tin
counting machine that prints your progress as tickets, plates and receipts (Dungeon Crawler Carl), drawn as a few
chunky ink-outlined toy pieces over the diorama (the Switch remake of Link's Awakening).

- **Published:** the Design System artifact [Mordecai · Tin Tally](https://claude.ai/artifact/4fBo1Eu7RBLBj511kq9B7v)
  (private to Jeff until shared). It shows every component live, in the Diorama and High Contrast themes.
- **This folder is the source.** The artifact is a build of it. Change things here, then republish.
- Nothing here is game content: it is the design spec an Unreal implementer builds in UMG/CommonUI. Sizes are
  design px at 1920 x 1080 (UMG points = px x 0.75).

## What is here

| Path | What |
|---|---|
| `ds/project/README.md`, `01-04-*.md` | The brand book: idea, voice and copy, HUD and pacing, building it in Unreal, open decisions |
| `ds/BUILD.md` | The build contract: rules D1-D27 every component follows (published as section 05) |
| `ds/project/tokens.json` | Colour, type, spacing, radius and shadow tokens, each with a usage note |
| `ds/project/fonts/` | Rowdies (the Tally's voice) and Atkinson Hyperlegible Next and Mono (the interface) |
| `ds/project/components/<Comp>/` | 44 cards, each a `preview.html` (static HTML rendition) and a `README.md` (the implementation spec) |
| `ds/css/_core.css`, `ds/css/<Comp>.css` | Shared primitives and one partial per component; concatenated into `bundle.css` |
| `ds/js/core.js` | `window.TinTally`: draws glyphs, tally strokes and rarity badges into the previews |
| `ds/glyphs/*.json` | The 117 icons (SVG markup on a 24-unit grid) |
| `pause_gen/`, `generators/` | Generators for the previews that are built by script (listed in `ds/BUILD.md`) |
| `tools/render.py` | `build` (bundle + icon SVGs), `render` (screenshots in both themes), `check` (static checks) |
| `tools/assemble.py`, `tools/icon_ids.txt` | Builds the publish tree and the artifact index; icon name to uploaded asset id |
| `research/` | The game facts gathered from the specs and code on 2026-10-31 (what the system was built from) |
| `directions/` | The three visual directions that were judged; "Tin Tally" (dir_broadcast) won and BUILD.md supersedes it |

## Change something

Read `ds/BUILD.md` first; it is the contract.

1. Edit the component's `ds/css/<Comp>.css`, `README.md` and `preview.html`. If the preview is generated
   (`ds/BUILD.md` lists which), edit its generator in `pause_gen/` or `generators/` and run it (`py -I gen_....py`
   from that folder), never the HTML.
2. Look at it. Rendering needs Google Chrome at its default install path:
   ```
   cd Design/TinTally/tools
   py -I render.py render <Comp>[,<Comp>]   # writes ds/build/shots/<Comp>.diorama.png and .high-contrast.png
   py -I render.py check                    # must report "problems: 0"
   ```
   Open both PNGs. Text never goes under 20 px, and it must hold 4.5:1 contrast in both themes.

## Republish

1. `py -I tools/assemble.py` rebuilds the bundle and writes `build/publish/` (the `project/` tree,
   `design-system.json` and `files_map.json`). It fails if a glyph has no uploaded icon.
2. A new or redrawn icon: upload `ds/assets_out/Icons/<name>.svg` to the artifact as an asset and put its id in
   `tools/icon_ids.txt` (a redrawn icon gets a new id), then run `assemble.py` again.
3. Ask Claude to publish `build/publish` to the artifact URL above. Read the live artifact first: it can be
   edited on its page, and those edits must be merged back into this folder rather than overwritten.
