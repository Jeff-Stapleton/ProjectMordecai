# US096 clean marketing recapture

The player uses Tab from Longsword to Dagger to Greatsword, then Shift+Tab back through Dagger to Longsword; the weapon HUD plate updates each time.

- Map: `/MordecaiCore/Game/Maps/DevTestMap`. DevTestMap is used because the existing dressed `L_VW_TileTest` terrain fixture has no enemies; the existing maps with working enemies are grey test fixtures. No new content was built.
- Clip: `weapon-cycling-forward-and-back.mp4` — 6.767 s, 30/1 fps, 1920x1080, 203 frames.
- Unique frames: ffmpeg mpdecimate kept 193 / 203 (95.074%; required at least 90%).
- Capture: game-owned, contiguous fixed-timestep frames from the US-099 `-MarketingFrameCapture -BENCHMARK -FPS=30` path; no desktop capture.
- HUD: on. Debug draws, facing arrows and on-screen messages: off and verified from runtime state plus scripted green/yellow pixel checks.
- Loading screen: absent from numbered frames after the built-in warm-up.
- AI: frozen only while loading and warming up, then automatically released before numbered frame 1; AI is active throughout retained footage. The full-session `-MordecaiMarketingFreezeAI` flag was not used.
- Edit: No editorial frame cut.
- Caveat: this is postable marketing media, not acceptance evidence.

## Stills

- `still_01_longsword.png`: Longsword shown in the weapon HUD plate.
- `still_02_dagger.png`: Dagger shown after Tab.
- `still_03_greatsword.png`: Greatsword shown after Tab.
- `still_04_longsword_return.png`: Longsword shown after Shift+Tab cycling back.
