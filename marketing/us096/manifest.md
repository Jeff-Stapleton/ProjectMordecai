# US-096 in-engine marketing media

- Clip: `weapon-cycling-back-and-forth.mp4` (6.205 real seconds, 30 fps, 1920x1080).
- Content: Tab cycles Longsword → Dagger → Greatsword; Shift+Tab cycles Greatsword → Dagger → Longsword while the player moves through CombatTestMap.
- Stills: `still_01_longsword.png`, `still_02_dagger.png`, `still_03_greatsword.png`, `still_04_dagger.png`, `still_05_longsword.png`.
- Source: game-owned Agent Bridge viewport frames only; no desktop capture.
- Provenance: `source-frame-manifest.json` stores every frame SHA256, real elapsed timestamp, equipped weapon, and sprint state.
- Presentation: launched with `-MordecaiMarketingCapture -NOSCREENMESSAGES`; `label-proof.json` must pass with no actor/debug labels or enemy indicators.
- Static/lock-screen defense: `Scripts/verify_marketing_media.py` decodes the MP4, matches samples to SHA256-pinned viewport frames, requires HUD structure, and requires motion across at least six sample pairs. See `content-check.json`.
