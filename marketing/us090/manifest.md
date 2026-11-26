# US090 clean marketing recapture

The player casts Fireball, Stone Skin and Restoration to drain spell points, the bar gains two quarter-point regeneration ticks, enemies kill the player, and respawn refills the bar.

- Map: `/MordecaiCore/Game/Maps/DevTestMap`. DevTestMap is used because the existing dressed `L_VW_TileTest` terrain fixture has no enemies; the existing maps with working enemies are grey test fixtures. No new content was built.
- Clip: `spell-points-drain-regen-respawn.mp4` — 7.467 s, 30/1 fps, 1920x1080, 224 frames.
- Unique frames: ffmpeg mpdecimate kept 219 / 224 (97.768%; required at least 90%).
- Capture: game-owned, contiguous fixed-timestep frames from the US-099 `-MarketingFrameCapture -BENCHMARK -FPS=30` path; no desktop capture.
- HUD: on. Debug draws, facing arrows and on-screen messages: off and verified from runtime state plus scripted green/yellow pixel checks.
- Loading screen: absent from numbered frames after the built-in warm-up.
- AI: frozen only while loading and warming up, then automatically released before numbered frame 1; AI is active throughout retained footage. The full-session `-MordecaiMarketingFreezeAI` flag was not used.
- Edit: One disclosed cut removes the static death-screen wait: raw frames 1-214, then 305-314. Every retained frame is an original rendered Unreal frame.
- Caveat: this is postable marketing media, not acceptance evidence.

## Stills

- `still_01_full.png`: Full spell-point bar before casting.
- `still_02_drained.png`: Spell-point bar drained after three casts.
- `still_03_regen.png`: Spell-point bar partially restored by slow regeneration.
- `still_04_respawn.png`: Spell-point bar full after respawn.
