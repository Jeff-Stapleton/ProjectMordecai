# Control Bindings V1.1

*Converted from control_bindings_v1.1.xlsx*

## Sheet1

| Action | Category | Controller | MKB | Notes |
| --- | --- | --- | --- | --- |
| Move | Movement | Left Stick | W/A/S/D | 360° analog vs digital. Movement always available. |
| Sprint (Toggle) | Movement | L3 (click) | Left Shift (toggle) | Sprint increases speed; may increase stamina drain/noise. |
| Look / Aim Direction | Camera/Aim | Right Stick | Mouse movement | No target lock; aim sets facing and attack direction. |
| Sneak (Toggle) | Stealth | R3 (click) — not yet bound | Ctrl — not yet bound | Not yet bound: no code maps a Sneak input. Design intent: reduces detection + footstep noise; reduces move speed. |
| Light Attack | Combat | RT (tap) | Left Mouse (tap) | Primary attack. |
| Heavy / Charged Attack | Combat | RT (hold) | Left Mouse (hold) | Charge time depends on weapon. |
| Block | Combat | LT (hold) | Right Mouse (hold) | Block drains stamina; stability scales with skills/gear. |
| Parry | Combat | LT + RT | Right Mouse + Left Mouse (timed) | Timed click while blocking; high risk/high reward. |
| Use Skill A | Skills | LB (tap) | Q (tap) | Executes equipped Skill A (Spell1). Fires on press. |
| Select Skill A (Radial) | Skills | LB (hold) — not implemented yet | Q (hold) — not implemented yet | Not implemented yet: no hold input or radial in code; LB/Q fire Skill A on press. Design intent: Option B, radial does NOT slow/pause time; selection is committed. |
| Use Skill B | Skills | RB (tap) | E (tap) | Executes equipped Skill B (Spell2). Fires on press. |
| Select Skill B (Radial) | Skills | RB (hold) — not implemented yet | E (hold) — not implemented yet | Not implemented yet: no hold input or radial in code; RB/E fire Skill B on press. Design intent: Option B, radial does NOT slow/pause time; selection is committed. |
| Use Skill C | Skills | — (not yet bound) | R | Executes equipped Skill C (Spell3). Fires on press. No gamepad button bound. |
| Use Skill D | Skills | D-pad Up | 1 | Executes equipped Skill D (Spell4). Fires on press. These keys were planned for Use Item A (1) and Use Item C (D-pad Up). |
| Jump | Movement | A | Space | Traversal + combat spacing. |
| Dodge | Combat | B | Left Alt | Always possible; effectiveness depends on stamina tier. |
| Interact | World | X | F | Talk, loot, activate, climb prompts. |
| Throw (equipped throwable) | Combat/Utility | Y (tap) — not yet bound | G (tap) — not yet bound | Not yet bound: no code maps a Throw input; Y currently cycles to the next weapon. Design intent: throws selected throwable (knife/bomb/vial). |
| Weapon Set Swap (Set 1 ↔ Set 2) | Combat/Loadout | Y (press): next weapon | Tab: next weapon; Shift+Tab or Backslash: previous weapon | Implemented as cycling through available weapons (US-078 / US-096), not a two-set swap. Y fires on press, not hold; no previous-weapon gamepad button. Design intent: fast swap on KBM; hold on controller to avoid accidental swaps. |
| Use Item A | Items | D-pad Down — not yet bound | 1 — not yet bound | Not yet bound: no code maps item-use inputs; 1 is used by Use Skill D. Design intent: quickslot item use. |
| Use Item B | Items | D-pad Left — not yet bound | 2 — not yet bound | Not yet bound: no code maps item-use inputs. Design intent: quickslot item use. |
| Use Item C | Items | D-pad Up — not yet bound | 3 — not yet bound | Not yet bound: no code maps item-use inputs; D-pad Up is used by Use Skill D. Design intent: quickslot item use. |
| Use Item D | Items | D-pad Right — not yet bound | 4 — not yet bound | Not yet bound: no code maps item-use inputs. Design intent: quickslot item use. |
| Pause / Menu | System | Start/Menu | Esc | Inventory, character sheet, settings. |
| Inventory (Toggle) | System | — | I | Opens the pause menu on the Inventory tab; I in the open menu closes it. Closes on key-up (a held I closes on release), and the key-up of the press that opened the menu is ignored. No gamepad binding (use Start/Menu). Source: US-095. |
| Previous / Next Menu Tab | System (Menu) | LB / RB | — (click tab headers) | Pause menu open only; wraps at either end. No keyboard key bound. In game LB/RB use skills; the open menu blocks game input. Source: US-095. |
| Map / Objective | System | View/Select — not yet bound | M — not yet bound | Not yet bound: no code maps a Map input. Design intent: Metroidvania navigation support. |

