# US-095 pause-menu marketing media

- Marketing media only; not Tier 3/Tier 4 acceptance evidence.
- Scene: `/MordecaiCore/Game/Maps/L_VW_TileTest` with the pause menu and combat HUD visible.
- Capture: game-owned viewport screenshots at 1920x1080 under `-MordecaiMarketingCapture`, with on-screen messages, TextRender labels, melee debug shapes and enemy activity suppressed.
- `still_01_character.png`: The pause menu with Character selected in the five-tab bar.
- `still_02_inventory.png`: The pause menu with Inventory selected in the five-tab bar.
- `still_03_settings.png`: removed in coordinator media review (2026-11-27). The Settings tab only shows "Coming Soon", which teases an unbuilt feature; its entries in the capture/content JSON are kept as the capture record.
- Inputs: LB moved Character to Settings and Settings to Inventory; RB moved Inventory to Settings; I closed the menu.
- Validation: ffprobe dimensions/format, ffmpeg blackdetect, non-uniform and black-frame checks, scripted green/yellow debug-colour pixel checks, and runtime actor-name/debug-label checks all pass in `content-check.json`.
- Optional clip: omitted because a paused, static menu cannot honestly meet the >=90% mpdecimate unique-frame rule without artificial rapid tab changes.
