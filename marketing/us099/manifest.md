# US-099 wall-fade marketing media (non-acceptance)

- These stills and this clip are marketing media only, not Tier 3/Tier 4 acceptance evidence.
- Clip: `wall-fade-and-restore.mp4` (10.633s, 30 fps, 1920x1080), captured as per-frame Unreal viewport screenshots with UI under `-BENCHMARK -FPS=30`; the contiguous raw frame numbers are validated before encoding.
- Provenance: retained `continuous-source.mp4` (SHA256 `3dc8cdd96faf571c4f99666cfd1930ed2b6b9a31cb469bb18fdfa3845f4ef06c`) was assembled from the contiguous Unreal frames. `source-frame-manifest.json` is decoded from that source movie, not from the final clip.
- Stills: captured in a separate capped real-time presentation session so their screenshot requests cannot interrupt or compact the continuous clip; each record reads fade state after the screenshot completes.
- Still timing: the still-only session discloses a 2.0s marketing fade so the mid-fade screenshot is ordered between pre/post transitional material states. The continuous clip and all acceptance evidence retain the production 0.25s fade.
- Enemies: inert during this capture through the dedicated `-MordecaiMarketingFreezeAI` flag; the shared `-MordecaiMarketingCapture` presentation flag does not freeze AI.
- Camera: 1000 cm marketing camera (`-MordecaiMarketingCapture`), not the 1800 cm gameplay camera; the closer view keeps the wall transition legible.
- Content: approach, dither transition, fully faded west wall with the player clear, and fully restored wall in frame; the combat HUD remains visible.
- Stills: `still_01_approach.png`, `still_02_mid_fade.png`, `still_03_fully_faded.png`, `still_04_restored.png`.
- Presentation: `Mordecai.Debug.DrawMeleeHitShapes 0`, `ShowFlag.TextRender 0`, `t.MaxFPS 30`, and `-NOSCREENMESSAGES`; label-proof and content checks must pass.
- Motion: ffmpeg mpdecimate kept 319 frames (30.000/s; required >= 25/s).
- Lock/static defense: retained-source samples, explicit visible combat-HUD proof, temporal continuity, and mpdecimate counts are in `content-check.json` and `label-proof.json`.
