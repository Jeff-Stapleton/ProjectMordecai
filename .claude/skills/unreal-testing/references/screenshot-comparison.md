# Screenshot Comparison — Full Reference

## Capture Methods

### Functional Screenshot Test Actor

Place in level via Place Actors panel. Uses a camera component.

**Settings:**
- **Resolution** — override per-actor or use default from Editor Preferences > Automation > Screenshots
- **Delay** — seconds before capture
- **Disable Noisy Rendering Features** — turns off AA, Motion Blur, SSR, Auto-Exposure, Contact Shadows
- **Visualize Buffer** — capture specific GBuffer instead of final lit image
- **Tolerance** — Zero, Low (default), Medium, High, Custom
- **Tolerance Amount** — per-channel RGBA values + min/max brightness
- **Maximum Local Error** — acceptable error in localized image chunks (catches hot spots)
- **Maximum Global Error** — total acceptable pixel error percentage
- **Ignore Anti-Aliasing** — search neighboring pixels for expected values
- **Ignore Colors** — compare luminance only

### Functional UI Screenshot Test Actor

Same as above but captures UI instead of scene view.

### Screenshot During Another Functional Test

Use Blueprint nodes:
- `Default Screenshot Options > Gameplay` — disables rendering noise
- `Default Screenshot Options > Rendering` — preserves rendering features
- Call `Take High Res Screenshot` node

### Screenshot from Python

```python
@unreal.AutomationScheduler.add_latent_command
def capture():
    task = unreal.AutomationLibrary.take_high_res_screenshot("test_name")
    while task.is_valid_task() and not task.is_task_done():
        yield
```

Image comparison:
```python
unreal.AutomationLibrary.compare_image_against_reference("image_name")
```

## Ground Truth Workflow

1. **First run** — warning: no ground truth exists
2. Open Screenshot Browser: `Window > Developer Tools > Session Frontend > Screen Comparison tab`
3. Click **Add New** to save as ground truth (adds to source control changelist)
4. **Subsequent runs** — compared against ground truth; failures show in browser

**On failure:**
- **Replace** — delete old ground truth, use new screenshot
- **Add As Alternative** — keep both (for hardware/driver variations); system picks closest match based on metadata. Grayed out for identical devices.

**Before re-running:** Always click **Delete All Reports** for a clean slate.

## Screenshot Storage

Screenshots saved to: `<Project>/Saved/Automation/Comparisons/`

Ground truth bucketed by: `Platform_RHI_ShaderModel`

## Editor Preferences

`Edit > Editor Preferences > Automation > Screenshots` — set default resolution for all screenshot actors.

Individual actor resolution overrides this global default.
