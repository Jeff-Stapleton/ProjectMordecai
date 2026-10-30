# Agent Bridge v1: protocol and design

**Status:** Phase 2 of `Specs/agent_playtest_plan_v1.md`. This is the contract between the in-game bridge (C++), the Python client (`Scripts/agent_bridge.py`), the MCP server (`Scripts/mordecai_play_mcp.py`) and the Tier 4 playtests (`Scripts/playtests/`).
**Last updated:** 2026-10-28 (`actors[].text_labels`, issues #30 and #38).

## 1. Purpose and the one rule

An agent drives the **packaged** game with real input and reads game state.

**The rule:** input commands only inject OS-level events into Slate (`FSlateApplication::Process*Event`). From there, every event takes the path a keyboard, mouse or gamepad event takes:
1. Slate input preprocessors (CommonInput, CommonUI, and the loading-screen input eater).
2. The user-0 focus path.
3. `SViewport` → `FSceneViewport` → `ULyraGameViewportClient` (CommonUI action router) → `APlayerController` → Enhanced Input mapping contexts → bindings.

Clicks are hit-tested by Slate.

The bridge never calls gameplay or UI functions as a stand-in for input. It has no `debug_*` commands in v1. If any are added later, their names must start with `debug_`, their responses must carry `"debug": true`, and they are never acceptance evidence.

The bridge only emulates the *environment* of a focused game window, so that a backgrounded or locked machine behaves like a player at the keyboard:
- It keeps Slate accepting device input while the app is inactive (`SetHandleDeviceInputWhenApplicationNotActive(true)`).
- It restores user-0 keyboard focus to the game viewport only when the user-0 focus path holds **no live widget at all** (a game launched on a locked desktop), and never while the CommonUI router is in `Menu` mode. A focus path whose leaf widget died (for example the Identify button, destroyed when the rows are rebuilt) still routes keys along its live part for a player, so the bridge leaves it alone.
- It uses a virtual (in-memory) cursor, because the OS cursor cannot be moved on a locked desktop. The bridge never moves the OS cursor.

Each input response reports in `env` whether it did any of these (`focus_restored`, and `virtual_cursor` read live from Slate: the cursor user's cursor is not the platform cursor).

## 2. Process model

**Enabling the bridge**
- `UMordecaiAgentBridgeSubsystem` (`UGameInstanceSubsystem`) is created only in non-Shipping builds launched with `-AgentBridge` (an exact token: `-AgentBridgePort=` alone does not enable it).
- Its socket and injection code sits inside `#if !UE_BUILD_SHIPPING`. Pure helpers, such as parsing and predicates, compile in every configuration so tests build everywhere.

**Command line**

| Flag | Default | Meaning |
|---|---|---|
| `-AgentBridge` | off | Enable the bridge |
| `-AgentBridgePort=<n>` | `47777` | TCP port on `127.0.0.1`. The Python client picks a free port for every launch. |
| `-AgentBridgeToken=<s>` | empty | Echoed by `ping` so the client can prove it reached the process it launched |
| `-AgentBridgeShotDir=<dir>` | `<ProjectSavedDir>/Playtests/Bridge` | Default screenshot directory |

**Listener**
- It binds `127.0.0.1` only, never `0.0.0.0` (no firewall prompt, no remote access).
- The listen socket is the bridge's own: `FTcpSocketBuilder(...).AsNonBlocking().AsReusable(false).BoundToEndpoint(127.0.0.1:<port>).Listening(8)`. Not reusable, so a second game cannot share the port.
- On success it logs `LogMordecaiAgentBridge: Display: AgentBridge: listening on 127.0.0.1:<port>`. A bind failure logs `LogMordecaiAgentBridge: Error: AgentBridge: failed to listen on 127.0.0.1:<port>` and leaves the bridge inert.

**Threading**
- There is no listener thread. Everything (accept, recv, parse, execute, send) runs on the game thread in an `FTSTicker` callback every frame; pending connections are accepted with `HasPendingConnection` + `Accept` (at most 16 per frame). A client that connects as soon as the port is bound is simply accepted on the next tick.
- Per tick: receive, execute, held input, flush, close finished connections, then accept (so a client that just went away does not count against the cap).
- The ticker runs while the game is paused (the pause menu pauses the world) and during the loading screen. It does not run during a blocking map load.

**Time.** `wait` and `timeout_s` are **real time** (`FPlatformTime::Seconds()`), never world time.

**Hold time (`hold_s` of `key tap`, `axis` and `click`) is simulated time: the input goes up at the frame boundary nearest to the point where the frames the game simulated with it down add up to `hold_s`** (`FMordecaiAgentBridgeHoldClock`; B3, US-100).
- What counts:
  - Each frame after the one the input went down in counts its frame delta (`FApp::GetDeltaTime`, also while the world is paused).
  - Each delta is capped at the world's `MaxUndilatedFrameTime` (0.4 s), because `AWorldSettings::FixupDeltaSeconds` simulates no more.
  - The bridge ticks at the end of a frame, after the world tick. When deciding, it takes the time the current frame has taken so far as a lower bound on the next frame's delta, and releases the input now if that lands nearer to `hold_s`.
- Why not wall time: a long frame's delta is simulated by the frame after it, so a hold judged on wall time went up at the end of a long frame, before the game had simulated it.
  - A frame hitch inside a hold left the player almost no input: W held 0.464 s of wall time over 3 frames moved the player 3 cm (`Saved/Playtests/20261029-132452-us054combat`).
  - The packaged game hitches after a `screenshot` and during its first launches after a package.
- Effect of a hitch:
  - A long hold stays down through it: it takes longer in wall time.
  - A quick tap (for example a 0.05 s light-attack click) goes up before the hitch is simulated, as a player's tap would, instead of reading as a heavy-attack hold.
  - `held_s` (wall time) can therefore be above or below `hold_s`.
  - `simulated_s` is within half a frame of `hold_s` (at most 0.2 s, half the 0.4 s frame cap), after at least `MinFrames` frames (tap 2, `axis` and `click` 1). A check that judges a distance or a duration against a hold can read `simulated_s`. In `Saved/Playtests/20261029-175605-us001`, `move_a` held A for `hold_s` 0.4 across a hitch: 0.712 s of wall time, 0.283 s simulated, 105 cm against the 100 cm threshold.
- The input goes up anyway 10 s of wall time past `hold_s`, so a key cannot stay down for good.
- Responses report `held_s` (wall time), `simulated_s` (the simulated time it was down) and `frames`.
- Auto-repeat still runs on wall time, as a real device's does. A tap that must stay down for its minimum frames through a hitch longer than the repeat delay can get one repeat event. No playtest reads gamepad or keyboard repeats.

**Connections**
- At most 4 concurrent connections. The 5th receives one error line `{"id":null,"ok":false,"cmd":null,"error":"too_many_connections",...}`; the bridge then shuts down its write side (FIN after the line) and reads and discards its input for up to 1 s or until the client closes, so the client reads the line instead of a reset.
- Each connection has its own receive buffer, send buffer (partial sends are retried) and request queue.
- **Peer EOF (half-close).** When a client closes its sending side, the bridge stops reading but still executes every request it already received, in order, and sends the replies; it closes the connection once nothing is queued or pending and the replies are out, or when a send fails. So `echo '{"cmd":"quit"}' | ncat 127.0.0.1 <port>` works. From EOF on, the connection no longer counts toward the 4-connection limit (it can never send another request). A client that dies mid-command keeps that command's input until the command finishes (its own `hold_s` of simulated time, at most 30 s; at most `hold_s` + 10 s of wall time), then the connection closes and releases everything it held; a pending `wait`/`wait_until` holds no input and simply runs out.
- **Held input is one server-wide union.** Slate sees one keyboard, one pad and one mouse:
  - A key goes down when its first holder presses it and up when its last holder releases it (reference count per key, across connections). A connection's `release` (or disconnect) removes only its own hold: if another connection still holds the key, no up event is sent.
  - An axis sends one sample per frame: the held value with the largest magnitude across connections (the earliest holder wins a tie).
  - A connection that presses or taps a key **it** already holds gets `bad_request` `"key '<K>' is already held by this connection; release it first"`. Pressing a key another connection holds just adds a holder (no second down event).
  - A `click` on a mouse button any connection is still holding gets `bad_request` `"button <B> is already held"`.
- **Closing a connection releases everything that connection holds** (keys, axes, mouse buttons). The bridge closes it when a send fails, when the client stops reading its replies, or after a peer EOF once its received requests have finished (above). Pending commands are cancelled only by `quit`.
- `quit` and subsystem shutdown release everything on every connection.

## 3. Framing

- UTF-8 newline-delimited JSON (NDJSON) in both directions, one JSON object per line.
- Both `\n` and `\r\n` are accepted. Blank lines are ignored.
- Maximum line length is 1 MiB. Longer input gets a `bad_request` reply and the buffer is discarded up to the next newline.
- Responses never contain a raw newline (condensed JSON writer).

**Request**

```json
{"id": 7, "cmd": "key", "key": "F", "action": "tap"}
```

- `id` is any JSON number or string. It is echoed verbatim. If it is missing, the response has `"id": null`.
- `cmd` is required. All other arguments sit at the top level next to `cmd`.

**Response**

```json
{"id": 7, "ok": true, "cmd": "key", ...result fields...}
{"id": 7, "ok": false, "cmd": "key", "error": "bad_key", "message": "unknown key 'Foo'"}
```

**Ordering.** Per connection, requests run strictly in arrival order. A command that takes time blocks later requests on the same connection until it replies: `key tap`, `axis` with `hold_s`, `click`, `screenshot`, `wait` and `wait_until`. Separate connections run independently.

**Error codes**

| Code | Meaning |
|---|---|
| `bad_request` | Malformed JSON, missing or ill-typed argument, or a value out of range |
| `unknown_command` | No such `cmd` |
| `bad_key` | Unknown key name, wrong key kind for the command, or a denied debug key (§5) |
| `not_ready` | No game viewport, world or Slate yet (very early boot) |
| `timeout` | `wait_until` condition not met in time, or a screenshot not written within 10 s |
| `failed` | The command could not complete (for example, the screenshot file is missing afterwards) |
| `too_many_connections` | See Connections in §2 |
| `cancelled` | The pending command was cancelled by `quit` |

## 4. Commands

Every successful response carries `"ok": true` and `"cmd"`. Every input command (`key`, `axis`, `mouse`, `click`) also carries:
- `"ready"`: the same value as `ping.ready`.
- `"env"`: `{"focus_restored": bool, "virtual_cursor": bool, "app_active": bool}`, which says whether the bridge had to emulate the focused-window environment for this command (§1). `virtual_cursor` is live Slate state (for `click`, true only if the in-memory cursor was active both at the down and at the up).

### `ping`

```json
{"id":1,"ok":true,"cmd":"ping","bridge_version":1,"pid":1234,"token":"<-AgentBridgeToken>",
 "build":{"commit":"<sha>","branch":"...","dirty":false,"built_at":"..."} | null,
 "map":"/MordecaiCore/Game/Maps/DevTestMap","experience":"B_MordecaiArenaExperience",
 "experience_loaded":true,"loading_screen":false,"has_pawn":true,"ready":true,
 "paused":false,"real_time_s":12.3,"world_time_s":9.1,"frame":734}
```

- `build` is read once from `build_info.json`. The bridge looks in the exe's directory and up to 6 parent directories. In a packaged playtest build the file is at `PackagedBuilds/Playtest/build_info.json`. If no file is found, `build` is `null`.
- `experience` is the loaded experience's asset name without `Default__` or a `_C` suffix. This is the same normalization `playtest_boot.experience_name()` uses.
- `ready` is `experience_loaded && !loading_screen && has_pawn`. While the loading screen is up (CommonLoadingScreen's input preprocessor), Slate eats every input event, exactly as it does for a player.

### `state`

Arguments:
- `sections` (optional): an array drawn from `game`, `focus`, `player`, `input`, `pickups`, `inventory`, `pause_menu`, `widgets` and `actors`. The default is all of them.
- `radius` (default 1500): the radius for `actors`.
- `include_all_actors` (default false): include level geometry.

The response is `{"ok":true,"cmd":"state","state":{<section>: ...}}`.

A section that cannot be gathered, for example `player` before the pawn exists, is still present, with `"valid": false` and a `"reason"` string. The bridge never invents values.

**Units and coordinates**
- Vectors are `[x,y,z]` and rotators are `[pitch,yaw,roll]`.
- World units are cm.
- **Screen coordinates are viewport pixels**: the origin is the top-left of the game viewport, and the space is the same as `engine_NN.png` (1280×720 in playtests).

```
game:      {valid, map, experience, experience_loaded, loading_screen, ready, paused, real_time_s, world_time_s,
            fps, viewport_size:[w,h]}
focus:     {valid, app_active, app_has_focus, viewport_has_focus, focused_widget:"<SWidget type>"|null,
            commonui_input_mode:"Game"|"Menu"|"All"|null, game_input_allowed:bool|null,
            input_type:"MouseAndKeyboard"|"Gamepad"|"Touch"|null, virtual_cursor, cursor:[x,y]|null,
            mouse_cursor_shown}
player:    {valid, name, class, location, rotation, control_rotation, velocity, speed, movement_mode, is_dead,
            attributes:{health,max_health,stamina,max_stamina,posture,max_posture,spell_points,max_spell_points},
            tags:[...], weapon:{id,display_name,type,index,count}|null,
            spells:[{slot,key,ability,spell_id,display_name,sp_cost,cooldown_tag,cooldown_remaining_s,active,can_afford}],
            kills}
input:     {valid, held:[{key,kind:"key"|"axis"|"mouse",value,holders}], pressed:[keys PlayerInput reports down],
            mapping_contexts:[names]}
pickups:   {valid, focused:{label,item_id,display_name,location,distance}|null, in_range_count,
            prompt:{valid, visible, text, hint_text, hint_visible},
            world:[{label,item_id,display_name,location,distance}]}
inventory: {valid, count, items:[{instance_id,item_id,display_name,quantity,type,rarity,identified,equipped}],
            ledger_count, ledger:[{item_id,display_name,count}]}
pause_menu:{valid, open, active_tab, tabs:[{id,name}],
            inventory_tab:{visible, rows:[{text, instance_id, unidentified,
                           identify_button:{label,center:[x,y],enabled}|null}], ledger_rows:[text],
                           list_placeholder, ledger_placeholder}|null}
widgets:   [{class, name, rect:[x,y,w,h], texts:[str], buttons:[{id,label,center:[x,y],enabled}]}]
actors:    [{label, name, class, native_class, kind:"enemy"|"pickup"|"pawn"|"projectile"|"illusion"|"other",
             location, distance, hidden, has_visible_mesh, health:{current,max}|null, is_dead|null,
             ai_state|null, tags:[...], widgets:[{component,class,space,visible}],
             text_labels:[{component,text,visible,facing|null,upright|null}]}]
```

**Notes**
- **Labels.** `label` is `AActor::GetActorNameOrLabel()`, which gives editor labels such as `TestPickup_HealthPotion` in Development packages. Never call `GetActorLabel()`: it is editor-only.
- **Player HP and attributes** come from `UMordecaiAttributeSet` on the PlayerState ASC. Never use `ULyraHealthComponent` for the player: it reads an unrelated set and always shows 100. For enemies, health comes from their own ASC's `UMordecaiAttributeSet`.
- **`pickups.prompt.text`** is the text actually rendered in the HUD prompt widget's `PromptText` text block, and `visible` is its real visibility. They are not the widget's cached model.
- **`widgets`** lists only user widgets whose native class name starts with `Mordecai` and whose Slate widget chain is visible all the way up to an `SWindow`. Pooled and stale CommonUI instances fail that test and are skipped.
- **`actors[].widgets` and `actors[].text_labels`.** `widgets` lists the actor's `UWidgetComponent`s (enemy health bars); `visible` is `IsWidgetVisible()`. `text_labels` lists its `UTextRenderComponent`s (a pickup's `NameLabel`), in component order, `[]` when it has none. A component is in only one of the two lists. The actor filter is unchanged: a text label alone does not make an actor interesting.
  - `component`: the component's object name (`NameLabel`).
  - `text`: the text the component renders. A pickup's label is its `UMordecaiItemLibrary::GetDisplayName`, so an unidentified item reads its partial name (`Unidentified Purple Amulet`).
  - `visible`: the label is drawn: the component is registered and visible, not hidden in game, and its actor is not hidden. A visible label with empty `text` draws nothing; check both.
  - `facing`: the label's readable face (its world +X axis) dotted with the camera's backward vector. 1 = square to the screen, 0 = edge-on, below 0 = the camera sees its back (mirrored).
  - `upright`: the direction its text runs (its world −Y axis) dotted with the camera's right vector. 1 = reads left to right, 0 = runs up or down the screen (turned 90°), −1 = upside down.
  - Why those axes: a TextRender lays its glyphs out at local (0, −x, −y) (engine `TextRenderComponent.cpp`), so its face is local +X, its text runs along local −Y and its up is local +Z.
  - `facing` and `upright` are cosines in [−1, 1] (3 decimals), measured against the rotation of the local player's camera as last computed (`APlayerCameraManager::GetCameraRotation()` of the `player` section's controller), not against its position. Both are `null` while there is no camera manager or it has never updated: the bridge never assumes the diorama default. The camera transform itself is still not reported (B2).
- **`pause_menu.inventory_tab.rows`** are the rows rendered in `ItemListBox`, so `text` is exactly what is drawn. `identify_button.center` is in viewport pixels and becomes valid one frame after the menu has painted.
- **`input.held`** has one entry per held input across all connections (§2 union): `value` is the union value (1 for keys and buttons, the largest-magnitude value for an axis) and `holders` is how many connections hold it. The threshold buttons the bridge synthesizes from axes (`axis` below) are not listed in `held`.
- **`input.pressed`** lists the keys `UPlayerInput::IsPressed` reports among the bridge-held keys, the synthesized threshold buttons and the live Mordecai keys. A key the bridge holds but PlayerInput does not see means the input was eaten upstream.
- **Ids are canonical strings.** A cooked (packaged) game does not preserve `FName` case (`WITH_CASE_PRESERVING_NAME` is `WITH_EDITORONLY_DATA`), so `FName::ToString()` returns whichever spelling was registered first in the process: the tab id `FName("inventory")` prints `Inventory` (UHT registers the `BindToInventory` parameter `Inventory` first) and `character` prints `Character`. The bridge therefore emits:
  - `pause_menu.active_tab` and `pause_menu.tabs[].id`: **lowercase** (`"inventory"`, `"character"`, `"skills"`, `"feats"`, `"settings"`). `tabs[].name` is display text and keeps its case.
  - `widgets[].buttons[].id` of `UMordecaiTabButton`s: an Identify button's id is the row's GUID in **exactly the `instance_id` form** (32 uppercase hex digits, `FGuid::ToString()`); every other tab-button id (pause-menu tabs, inventory filters) is **lowercase** (`"inventory"`, `"all"`, `"weapons"`). Other buttons use their object name.
  - `pause_menu.inventory_tab.rows[].instance_id`: the Identify button's id parsed with `FGuid::Parse` and re-formatted with `FGuid::ToString()`, so it always equals `inventory.items[].instance_id` (rows without a button take the model's GUID).
  - Clients may still compare button ids case-insensitively.
  - **Remaining FName-derived strings (audit).** These print the first-registered spelling in a packaged build and are not canonicalized: `inventory.items[].item_id` and `inventory.ledger[].item_id` (`UMordecaiItemDefinition::ItemId`), `pickups.*.item_id`, key names in `keys`, `input.held[].key`, `input.pressed`, `player.weapon.id`, `player.spells[].spell_id`/`ability`, class and object names (`class`, `name`, `native_class`, `focused_widget`, `mapping_contexts`), and `pickups.*.label` / `actors[].label` when an actor has no label (then it is the object name). Enum strings (`type`, `rarity`, `input_type`) are unaffected in practice (UHT registers the full `EEnum::Value` name). The 2026-10-20 packaged run reported all of these in their expected case (`TestItem_*`, `W`/`F`/`I`/`Escape`, `MordecaiIMC_Runtime`); a collision needs another FName with the same letters registered earlier. Compare them exactly only when they come from the same build; otherwise compare case-insensitively.

### `key`

```json
{"cmd":"key","key":"F","action":"tap"}
{"cmd":"key","keys":["W","D"],"action":"tap","hold_s":0.3}
{"cmd":"key","key":"W","action":"press"}
{"cmd":"key","key":"W","action":"release"}
```

**Arguments**
- `key`, or `keys` (1-4 keys, applied together).
- `action`: `press`, `release` or `tap`. The default is `tap`.
- `hold_s`: for `tap` only. The default is 0.1 and the maximum is 30.
- `unsafe`: default false (§5).
- `route`: default `"slate"`.

**Actions**
- `press`: key down. The key stays held until `release`, the connection closes, or `quit`. The response returns once the down event has been processed.
- `release`: key up. Releasing a key that this connection does not hold succeeds with `was_held:false` and sends nothing.
- `tap`: down, then hold until the game has simulated `hold_s` with it down, for 2 frames at least (§2 Hold time), then up. The response returns after the release. With `keys`, all go down in one frame and all come up in one frame.
- `press` or `tap` of a key this connection already holds is `bad_request`: `"key '<K>' is already held by this connection; release it first"`.
- All of these act on the server-wide union (§2): Slate gets the down only on the first holder and the up only when the last holder lets go.

**Response**

```json
{"ok":true,"cmd":"key","keys":["W","D"],"action":"tap","route":"slate","held_s":0.31,"simulated_s":0.3,"frames":18,"ready":true,"env":{...}}
```

**How events are injected**
- A down event is `FSlateApplication::ProcessKeyDownEvent(FKeyEvent(Key, Mods, UserIndex 0, bIsRepeat=false, CharCode, KeyCode))`. The codes come from `FInputKeyManager::GetCodesFromKey`.
- `Mods` reflects the modifier keys the bridge holds (Shift, Ctrl, Alt, Cmd), never the real keyboard.
- Held keys **auto-repeat the way the devices do** (`bIsRepeat=true` down events, at most one per key per frame):
  - **Keyboard (Windows typematic):** only the **most recently pressed** held keyboard key repeats, first after 0.5 s, then about 30 times a second. When another key goes down the repeat moves to it, and the earlier key does not resume, even after the newer key is released. So after a `FlushPressedKeys` (CommonUI menu flush, viewport focus loss) only the newest key comes back through `Input.AutoReconcilePressedEventsOnFirstRepeat`, as for a player; older held keys stay released until pressed again.
  - **Gamepad buttons (XInput):** every held button repeats on its own schedule, first `InitialButtonRepeatDelay` after its press, then every `ButtonRepeatDelay` (read from `[/Script/Engine.InputSettings]` in Input.ini like `XInputInterface.cpp`; BaseInput.ini: 0.2 s / 0.1 s, so about 9 repeats in the first second). The next repeat is scheduled from the frame that sent the previous one, as XInput does.
- Up is `ProcessKeyUpEvent`.
- Gamepad buttons (`Gamepad_FaceButton_Bottom` …) use the same calls with the default input device (`IPlatformInputDeviceMapper::Get().GetDefaultInputDevice()`), which maps to platform user 0.
- Mouse buttons are rejected here with `bad_key`. Use `click`.
- `route:"viewport"` is diagnostic only and is never acceptance evidence. It injects at `FSceneViewport::OnKeyDown/OnKeyUp`, below the preprocessors and focus routing. Its responses carry `"diagnostic": true`.

### `axis`

```json
{"cmd":"axis","values":{"Gamepad_LeftX":0.0,"Gamepad_LeftY":1.0},"hold_s":0.5}
{"cmd":"axis","values":{"Gamepad_LeftY":1.0}}
{"cmd":"axis","values":{"Gamepad_LeftY":0.0}}
```

**Arguments**
- `values`: a map of `Gamepad_LeftX`, `Gamepad_LeftY`, `Gamepad_RightX`, `Gamepad_RightY`, `Gamepad_LeftTriggerAxis`, `Gamepad_RightTriggerAxis` to values. Sticks take −1..1 and triggers take 0..1.
- `hold_s` (optional): with it, the values are applied, sent every frame until the game has simulated `hold_s` with them (1 frame at least; §2 Hold time), then set to 0, and the response returns after the reset with `held_s`, `simulated_s` and `frames`. Without it, the values persist (sent every frame) until the next `axis` for that key. A value of 0 releases this connection's hold.

Events are injected with `FSlateApplication::ProcessAnalogInputEvent(FAnalogInputEvent(...))`. `FSceneViewport` negates `Gamepad_RightY` on that path itself, as it does for a real pad.

- **Exactly one analog sample per frame** per held axis (the union value, §2), sent in the bridge tick. The command itself sends nothing for a held value; `UPlayerInput` sums the samples it gets in a frame, so a second one would double the value. A release (the union value reaching 0) sends one 0 sample at once, because gamepad axes latch their last sample.
- **Threshold buttons, as a real XInput pad sends them** (`XInputInterface.cpp` `SendControllerEvents`): when the raw value passes the pad's threshold, the matching digital key goes down through the same Slate path as a `key` gamepad button, and up when it falls back; held threshold buttons auto-repeat like gamepad buttons.
  - Raw values: sticks `value × 32767` (positive) or `× 32768` (negative), rounded; triggers `value × 255`, rounded.
  - `Gamepad_LeftX` → `Gamepad_LeftStick_Left` / `Gamepad_LeftStick_Right`, `Gamepad_LeftY` → `Gamepad_LeftStick_Down` / `Gamepad_LeftStick_Up`, raw beyond ±7849 (`XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE`, about 0.24).
  - `Gamepad_RightX` / `Gamepad_RightY` → `Gamepad_RightStick_Left`/`_Right` / `_Down`/`_Up`, raw beyond ±8689 (`XINPUT_GAMEPAD_RIGHT_THUMB_DEADZONE`, about 0.266).
  - `Gamepad_LeftTriggerAxis` / `Gamepad_RightTriggerAxis` → `Gamepad_LeftTrigger` / `Gamepad_RightTrigger`, raw above 30 (`XINPUT_GAMEPAD_TRIGGER_THRESHOLD`, about 0.12). The engine's dynamic trigger release dead zone is off by default (`Input.TriggerDynamicReleaseDeadZone` = 1) and is not modelled.
  - Order within a frame matches XInput: the analog sample, then the button transitions.

### `mouse`

`{"cmd":"mouse","x":640,"y":360}` moves the virtual cursor to a viewport pixel position and injects `ProcessMouseMoveEvent`.

- Mouse commands switch Slate's cursor user to the in-memory cursor (`UsePlatformCursorForCursorUser(false)`), so Slate's own synthesized mouse moves use the bridge position and a locked desktop (where the OS cursor cannot move) does not matter.
- Slate swaps back to the OS cursor by itself in two cases: CommonInput switching the input method to MouseAndKeyboard (for example the first mouse event after a `key Gamepad_*`; `UCommonInputSubsystem::RecalculateCurrentInputType`), and more than 3 real OS mouse moves over the window (`FSlateApplication`'s platform-move counter). A human moving the real mouse therefore **can** disturb hover between commands, as it would for a player sharing the machine. The bridge re-installs the in-memory cursor at the bridge position on every `mouse` and `click`, after each injected mouse event, and every frame while a bridge mouse button is down.
- The response is `{"ok":true,"cmd":"mouse","x":640,"y":360,"abs":[ax,ay],...}`.

### `click`

`{"cmd":"click","x":812,"y":402,"button":"Left","hold_s":0.05}`

- `x` and `y` are optional. Without them, the click lands at the current virtual cursor.
- `button` is `Left`, `Right` or `Middle`.
- Sequence:
  1. Frame N: move to (x, y) (a non-synthetic `ProcessMouseMoveEvent`) and `ProcessMouseButtonDownEvent`, then re-install the in-memory cursor there.
  2. Every frame while the button is down: re-install the in-memory cursor at (x, y).
  3. Once the game has simulated `hold_s` with the button down (1 frame at least; §2 Hold time): a non-synthetic `ProcessMouseMoveEvent` at (x, y), which re-hovers the pressed `SButton` (the mouse captor gets `OnMouseEnter` again) if a synthesized move un-hovered it meanwhile, then `ProcessMouseButtonUpEvent` at the same position. A player's cursor is on the button when they let go; `SButton::OnMouseButtonUp` only clicks while hovered.
  4. The response returns after the up event.
- Slate hit-tests the position, so a click on something covered by another widget lands on the cover, as it would for a player.
- The response includes `"hit": "<SWidget type under the cursor>"|null`, taken from Slate's widget path at the click position.

### `screenshot`

`{"cmd":"screenshot","path":"C:/.../bridge_01.png"}`. `path` is optional; the default is `<ShotDir>/bridge_NN.png`, numbered per process.

1. The bridge waits until `!FScreenshotRequest::IsScreenshotRequested()`. The slot is shared with `-PlaytestCapture`.
2. It requests `FScreenshotRequest::RequestScreenshot(path, bShowUI=true, false)`.
3. It completes on `FScreenshotRequest::OnScreenshotRequestProcessed()` once the file exists with size > 0.

Response: `{"ok":true,"cmd":"screenshot","path":"...","bytes":123456}`. If the file has not appeared within 10 s (for example, a minimized window renders nothing), the result is a `timeout` error.

### `wait`

`{"cmd":"wait","seconds":1.5}` replies after that much real time. The range is 0..600.

### `wait_until`

`{"cmd":"wait_until","condition":"inventory.count>=4","timeout_s":10,"poll_s":0.1}`

- The condition is evaluated against the `state` JSON every `poll_s` (minimum one frame) until it is true or `timeout_s` passes (default 10, maximum 600).
- The bridge gathers only the sections named by the root identifiers of the condition's paths.
- Success: `{"ok":true,"cmd":"wait_until","elapsed_s":1.2,"values":{"inventory.count":4}}`.
- Timeout: `{"ok":false,"error":"timeout","message":"...","elapsed_s":10.02,"values":{...last values of every referenced path...}}`.
- A condition that does not parse is a `bad_request` whose message starts `bad condition: `. Limits (checked before anything is evaluated, so no input can overflow the game-thread stack):
  - longer than 4096 characters: `"bad condition: condition longer than 4096 characters"`;
  - more than 64 levels of `!` / `(` nesting (each `!` and each `(` is one level; flat `&&` / `||` chains are not nesting): `"bad condition: condition nested too deeply"`.

**Grammar**

```
expr       := or
or         := and ( "||" and )*
and        := unary ( "&&" unary )*
unary      := "!" unary | primary
primary    := "(" expr ")" | comparison
comparison := operand ( ("=="|"!="|"<"|"<="|">"|">=") operand )?
operand    := path | "len(" path ")" | literal
path       := ident ( "." ident | "[" int "]" )*        ident := [A-Za-z_][A-Za-z0-9_]*
literal    := number | "..." | '...' | true | false | null
```

**Semantics**
- A path that does not resolve is `null`.
- A bare operand is truthy unless it is `null`, `false`, `0`, `""` or an empty array/object.
- `==` and `!=` compare numbers numerically, strings exactly, booleans and `null` by value. Mismatched types are unequal.
- `<`, `<=`, `>` and `>=` need two numbers or two strings (ordinal). Any other combination is false.
- `len()` gives the length of an array, string or object, and 0 for `null`.
- Example: `pause_menu.open==true && pause_menu.active_tab=="inventory" && len(pause_menu.inventory_tab.rows)==4`.

### `quit`

The bridge replies `{"ok":true,"cmd":"quit"}`, flushes every connection, releases all held input, cancels pending commands (`cancelled`), then calls `RequestEngineExit(TEXT("AgentBridge quit"))` on the next tick.

## 5. Key names

- Keys are UE `FKey` names, matched case-insensitively. Validation uses `FKey::IsValid()`.
- The bridge resolves a few aliases:

| Alias | FKey |
|---|---|
| `Esc` | `Escape` |
| `Space` | `SpaceBar` |
| `Enter` / `Return` | `Enter` |
| `Shift` | `LeftShift` |
| `Ctrl` | `LeftControl` |
| `Alt` | `LeftAlt` |
| `0`-`9` | `Zero`-`Nine` |
| `Up` / `Down` / `Left` / `Right` | the `*` arrow keys (`Up`, `Down`, `Left`, `Right`) |

- **Denied unless `"unsafe": true`.** These are Development debug exec bindings with side effects: `K` (DamageSelfDestruct), `Insert`, `Delete`, `Home`, `End`, `PageUp`, `PageDown`, `NumPadFour`, `F1`-`F12`, `Tilde`, `Semicolon`, `Gamepad_Special_Left`.
- **Live Mordecai bindings** (`UMordecaiHeroComponent::EnsureMordecaiInputBound`, runtime IMC `MordecaiIMC_Runtime`):

| Key | Action |
|---|---|
| `W` / `A` / `S` / `D` | Move: W=+X, S=−X, D=+Y, A=−Y while control yaw is 0 |
| `F` | Interact (pick up the focused pickup) |
| `I` | Open the pause menu on the Inventory tab |
| `Escape` | Open the pause menu |
| `Q` / `E` / `R` / `One` | Spell slots 0-3 |
| `Tab` / `BackSlash` | Next / previous weapon |
| `LeftMouseButton` (a `click`) / `Gamepad_RightTriggerAxis` | Released before 0.4 s: light attack. Held 0.4 s or more: heavy attack on the release (`Mordecai.Attack.Heavy` in `player.tags`). Pressed while blocking: parry (US-083) |
| `RightMouseButton` / `Gamepad_LeftTriggerAxis` (held) | Block; the release ends it |
| `LeftAlt` / `Gamepad_FaceButton_Right` | Dodge |
| `SpaceBar` / `Gamepad_FaceButton_Bottom` | Jump |
| `LeftShift` | Sprint toggle (a second tap walks again) |

  Block and parry on the mouse need the right mouse button held across reads, which the playtest client cannot do (B2); on the gamepad, `axis` without `hold_s` holds LT / RT across reads (`Scripts/playtests/us083_combat_input.py`). Gamepad sticks and L3 use invalid key names today (Known issues).

## 6. Python client (`Scripts/agent_bridge.py`)

The client uses the standard library only, plus optional Pillow.

**Classes**
- `BridgeError(Exception)` has `.kind` set to `"gameplay"` or `"infra"` and `.response` holding the raw response.
- `BridgeConnection(port, host="127.0.0.1")` is one socket with `TCP_NODELAY`.
  - `request(cmd, timeout=None, **args) -> dict`. A lock serializes calls. Each call assigns an incrementing id, sends one line, and reads lines until the matching id arrives.
  - `ok:false` raises `BridgeError`: `timeout` and `bad_key` are gameplay errors, the rest are infra.
  - The socket timeout is the expected duration of the command plus 15 s.
  - Every request and response is appended to `<run_dir>/bridge.jsonl` when a run dir is set. That file is the audit trail showing only input commands were used.
- `Bridge`
  - `launch(map=DEFAULT_MAP, expect_experience=..., frontend=False, allow_stale=False, detach=False, run_suffix="bridge", timeout=240, extra_args=())`:
    - Runs the `playtest_boot` staleness gate and refuses a STALE build unless `allow_stale` is set.
    - Picks a free port and a uuid token.
    - Launches with `playtest_boot`'s launch helper plus `-AgentBridge -AgentBridgePort= -AgentBridgeToken= -AgentBridgeShotDir=<run_dir>`.
    - Keeps a kill-on-close job unless `detach` is set.
    - Waits for the experience through the game.log tail, then connects and polls `ping` until `ready` (token checked).
  - `attach(port, token=None)` connects to an already running bridge.
  - Methods: `ping()`, `state(sections=None, radius=1500)`, `press(key)`, `release(key)`, `tap(key_or_keys, hold_s=0.1)`, `hold(key_or_keys, seconds)` (a tap with `hold_s`), `axis(values, hold_s=None)`, `mouse(x, y)`, `click(x=None, y=None, button="Left")`, `click_button(label=None, button_id=None)`, `screenshot(path=None) -> path`, `wait(s)`, `wait_until(condition, timeout_s=10)`, `wait_for(predicate_fn, timeout_s, sections, poll_s=0.1)`, `walk_to(x, y, tol=50, timeout_s=None)`, `quit()` and `close()`.
  - `click_button` finds the button's center in `state.widgets` / `pause_menu`, then sends `click`. That is aiming, as a player does with their eyes; Slate still decides what the click hits.
  - `quit()` sends `quit` and waits for the process to exit, killing it if needed.
- **Session file** (CLI only): `Saved/Playtests/agent_bridge_session.json` holds `{pid, port, token, run_dir, exe, commit, started_at}`.
  - `py Scripts/agent_bridge.py launch` runs detached and writes the file. Later CLI calls read it.
  - `quit` deletes it.
  - A pid that is no longer alive means the session is stale; the file is removed and the call is an infra error.

**`walk_to` (closed loop)**
1. Read `player.location` and `player.control_rotation`.
2. Assert the yaw is within 1° of 0. Otherwise raise a gameplay error, because the WASD basis would be rotated.
3. Each pulse:
   - Project the remaining XY delta onto forward (+X) and right (+Y).
   - Choose W/S and/or D/A for each component above `max(tol*0.5, 20)`. Diagonals go in one `keys` tap.
   - Hold for `clamp(dist/600 - 0.05, 0.05, 0.4)` s.
   - Re-read the state.
4. The first pulse calibrates: the displacement must have a positive dot product with the intended direction, otherwise raise "movement mapping mismatch".
5. Stuck: after three pulses with under 10 cm of total progress, side-step perpendicular for 0.25 s, alternating sides. After 3 side-steps raise `BridgeError("stuck at (x, y), target (tx, ty), dist d")`.
6. Abort with a gameplay error if `player.is_dead` or the pause menu opens.
7. The default timeout is `3*dist/600 + 3` s.
8. Return `{"final": [x,y], "dist": d, "pulses": n}`.

**CLI**

```
py Scripts/agent_bridge.py launch [--map M | --frontend] [--allow-stale]
py Scripts/agent_bridge.py ping | state [--sections a,b] [--radius R]
py Scripts/agent_bridge.py press F [--hold 0.1]        (a tap; CLI connections close, which releases keys)
py Scripts/agent_bridge.py axis Gamepad_LeftY=1 --hold 0.5
py Scripts/agent_bridge.py mouse 640 360 | click [X Y] | click-button "Identify"
py Scripts/agent_bridge.py walk-to 400 0 [--tol 50]
py Scripts/agent_bridge.py screenshot [PATH] | wait 1.5 | wait-until "inventory.count>=4" [--timeout 10]
py Scripts/agent_bridge.py quit
```

The CLI prints JSON to stdout. Exit codes are 0 (ok), 1 (gameplay failure: `timeout`, `bad_key`, `walk_to` stuck) and 2 (infra).

## 7. MCP server (`Scripts/mordecai_play_mcp.py`)

- It is a stdio server on the `mcp` 2.x SDK (`from mcp.server.mcpserver import MCPServer, Image`), pinned in `Scripts/requirements.txt`.
- It is registered project-scoped in `.mcp.json` as `mordecai-play` with `command: "py"`. Its tools are `mcp__mordecai-play__*`.
- **Tools:** `launch_game`, `get_state`, `press_key`, `hold_key`, `release_key`, `walk_to`, `click`, `click_button`, `screenshot` (returns the image), `wait_until` and `quit_game`.
- Every tool uses `structured_output=False`.
- A lock serializes tool calls.
- `BridgeError` is converted to `ToolError` so the agent sees the message.
- The server owns at most one game, in a kill-on-close job, so the game dies with the server.
- Exploratory play may use `allow_stale` only when the call says so explicitly. The tool result then carries a `STALE BUILD — not evidence` banner.

## 8. Tier 4 playtests (`Scripts/playtests/usXXX_<name>.py`)

- Each playtest writes `Saved/Playtests/<YYYYmmdd-HHMMSS>-usXXX/` containing:
  - `report.md` and `report.json`, with steps as checks (PASS, FAIL, SKIP)
  - `game.log`
  - `bridge.jsonl`
  - `bridge_NN.png`
- Exit codes:
  - 0: every check passes.
  - 1: any gameplay FAIL.
  - 2: infra problem: STALE BUILD, launch failure, no connection or a crash.
- `--allow-stale` is diagnostic only and is recorded in the report.
- The report includes the `focus` section at start and end, so a run on an unfocused or locked machine documents the environment it ran in.

## 9. Known issues the bridge must expose, not hide

These were found while mapping Phase 2. They are game bugs to file, and the bridge must not work around them.
- **The pause menu could not be closed with real input (confirmed 2026-10-20, `Saved/Playtests/20261020-144643-us079`, step `menu_closes_escape`).** While it is open, CommonUI's Menu input mode blocks every key without a UI action binding, and the widget was not a back handler. **Game fix (separate commit):** `UMordecaiPauseMenuWidget` sets `bIsBackHandler = true`, so CommonUI registers its default back action (`B_CommonInputData` → `DT_UniversalActions` row `DefaultBack`: keyboard `Escape`, gamepad `Gamepad_FaceButton_Right`; Enhanced Input support is off, so the data-table action is used). The default back handling deactivates the widget, and `NativeOnDeactivated` closes the menu through `UMordecaiPauseMenuSubsystem::ClosePauseMenu`. Verified with real input in the packaged game: `Saved/Playtests/20261020-192656-us079` (diagnostic build of the uncommitted tree), `menu_closes_escape` PASS. `I` still does not close the menu (it has no UI action binding).
- **Gamepad stick and L3 mappings use invalid key names.** `Gamepad_LeftStick2D`, `Gamepad_RightStick2D` and `Gamepad_LeftThumbstickButton` do not exist; the real names are `Gamepad_Left2D`, `Gamepad_Right2D` and `Gamepad_LeftThumbstick`.
- **Facing and aim are pinned to +X.** The diorama camera writes control rotation every frame.
- **Not bound (FIXED by US-083):** sprint, jump, attacks, block and dodge are bound since US-083 (§5). Left Shift no longer crouches (N23).
- **Pickup name labels were turned 90° and unreported (FIXED, issues #30 and #38).** The bridge did not report `UTextRenderComponent`s (B1), and the label's fixed rotation (45, 90, 0) made its text run up the screen under the diorama camera (yaw 0, looking along +X). The label is now a billboard that turns to the local camera every frame, and `actors[].text_labels[].facing` / `upright` expose any label that is not square and level to the camera; us079 `pickup_labels` asserts both.
- **Spells have no cooldown tag.**
- **The three red bars** are the health bars of three invisible `BP_MordecaiEnemy_Frontliner` enemies. They have no mesh.
