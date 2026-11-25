# US-096 in-engine marketing media

- Clip: `weapon-cycling-back-and-forth.mp4` (6.5 seconds, 30 fps, 1280x720).
- Content: Tab cycles Longsword → Dagger → Greatsword; Shift+Tab cycles Greatsword → Dagger → Longsword while the player moves through CombatTestMap.
- Stills: `still_01_longsword.png`, `still_02_dagger.png`, `still_03_greatsword.png`, `still_04_dagger.png`, `still_05_longsword.png`.
- Source: continuous Unreal packaged-game `-DUMPMOVIE -BENCHMARK -FPS=30` viewport frames; no desktop capture and no slideshow encoding.
- Presentation: `-MordecaiMarketingCapture -NOSCREENMESSAGES`; `label-proof.json` passes with no actor/debug labels or enemy indicators.
- Static/lock-screen defense: `Scripts/verify_marketing_media.py` validates the encoded video against SHA256-pinned game-owned viewport frames, HUD structure, adjacent-frame motion, and 10 fps temporal continuity. See `content-check.json`.
