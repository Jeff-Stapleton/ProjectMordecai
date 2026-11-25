# US-099 in-engine marketing media

- Clip: `wall-fade-and-restore.mp4` (5.361 real seconds, 30 fps, 1920x1080).
- Content: the player walks with real WASD from the arena centre behind the near wall, holds while the dithered wall fades, then uses real Blink/dodge/WASD input to escape the Frontliner swarm while the wall restores; the combat HUD remains visible.
- Stills: `still_01_approach.png`, `still_02_approach.png`, `still_03_faded.png`, `still_04_depart.png`, `still_05_restored.png`.
- Source: game-owned Agent Bridge viewport frames only; no desktop capture and no `-DUMPMOVIE`.
- Anti-static evidence: 21/21 unique SHA-256 source frames; `content-check.json` decodes the MP4 and checks HUD structure, source matching, motion, and 10 fps temporal continuity.
- Presentation: `-MordecaiMarketingCapture -NOSCREENMESSAGES`; `ShowFlag.TextRender 0` suppresses world TextRender labels; `label-proof.json` passes with no actor/debug labels or enemy indicators.
