# Issue #148 stamina block stills

Two game-owned 1920x1080 viewport stills captured in the Mordecai arena with the combat HUD on.

- `stamina-block-ready.png`: Block held before an incoming hit.
- `stamina-block-spent.png`: Stamina reduced by 18.750 after a blocked hit.
- Capture: `-MordecaiMarketingCapture -NOSCREENMESSAGES -ForceRes`; TextRender disabled.
- Numerical validation: both stills are 1920x1080, non-uniform, HUD-structured, below the black-frame threshold, and pass green/yellow debug-colour pixel checks.
- Runtime validation: combat HUD present; actor labels, enemy indicators, debug text, facing arrow, and presentation violations absent.
- No image or video was opened or delegated for review.
