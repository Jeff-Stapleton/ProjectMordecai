# US-085 Frontliner marketing media

Three Frontliners close in around the player, land hits, and apply Burning.

- Clip: `frontliners-closing-hit-burning.mp4` — 5.233 s, 30/1 fps, 1920x1080, 157 frames.
- Unique frames: ffmpeg mpdecimate kept 154 / 157 (98.089%; required at least 90%).
- Capture: game-owned, contiguous fixed-timestep frames from the `-MordecaiMarketingCapture -MarketingFrameCapture -BENCHMARK -FPS=30` path; loading screens are absent from numbered frames.
- Presentation: near-duplicate source frames were removed, the 69 changing game frames were timed at 13.2 fps, and linear interpolation produced a 5+ second 30 fps clip without duplicate-frame padding.
- HUD: on. Debug draws, facing arrows, actor labels, and on-screen messages: off and numerically checked.
- Follow-up: death and respawn were not captured after three attempts. The third attempt's capture-only K / DamageSelfDestruct input did not transition Mordecai health to dead state.
- These files are marketing media, not acceptance evidence.

## Stills

- `still_01_frontliners_closing.png`: Three Frontliners close in around the player in the arena.
- `still_02_frontliner_hit.png`: Frontliners surround the player after a hit lowers the health bar.
- `still_03_burning.png`: The Burning status appears while the fire Frontliner attacks the player.
