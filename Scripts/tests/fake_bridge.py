#!/usr/bin/env python3
"""Fake Agent Bridge -- an offline stand-in for the in-game UMordecaiAgentBridgeSubsystem

Speaks the agent bridge protocol of Specs/agent_bridge_v1.md (sections 2-5) over 127.0.0.1 so
Scripts/agent_bridge.py, the MCP server and Tier 4 playtest scripts can be tested without the
packaged game.  It is a protocol double, not evidence: nothing it does says anything about the
real game.

What it implements
  - NDJSON framing: \\n or \\r\\n, blank lines ignored, 1 MiB line limit (bad_request, then the
    rest of the line is discarded), condensed JSON responses, ids echoed verbatim.
  - Per-connection ordering on a 60 Hz "game thread" tick: a command that takes time (key tap,
    axis hold_s, click, screenshot, wait, wait_until) blocks later requests on the SAME
    connection only.  At most 4 connections; the 5th gets one too_many_connections line.
    The tick runs in the C++ order: receive, execute, flush, close finished connections, then
    accept (so a client that just went away does not count against the cap).
  - Peer EOF (section 2) as the C++ FMordecaiAgentBridgeServer: when a client closes its sending
    side (or its socket fails on read), the fake stops reading, still executes every request it
    already received and sends the replies, and closes the connection -- releasing everything
    it holds -- once nothing is queued or pending and the replies are out.  From EOF on the
    connection no longer counts toward the 4-connection limit.  A failed send closes at once
    (its pending command is dropped without a reply).  Pending commands are cancelled only by
    quit, which replies, cancels every pending command ("cancelled"), releases all input and
    stops the server.
  - Held input is the union over connections, like the C++ bridge: a key goes down on its first
    holder and up when its last holder lets go, an axis reads the largest |value| of its
    holders, and state.input.held lists each input once with a "holders" count.  press or tap
    of a key THIS connection already holds is bad_request ("release it first").
  - Every command and error code of section 3-4 and the full wait_until grammar and semantics
    (see Condition).  Key names, aliases and the denied debug keys of section 5.
  - FakeWorld: a tiny DevTestMap.  The pawn moves at 600 uu/s with no acceleration while
    W/A/S/D are held (W=+X, S=-X, D=+Y, A=-Y unless `mapping` says otherwise), blocked by boxes
    (arena walls by default; DEVTESTMAP_OBSTACLES for the real blocks).  The 6 DevTestMap pickups
    at their real positions and labels; the nearest one within focus_range (120 uu, XY) is focused
    and F picks it up (IronOre and TownStone go to the ledger).  I opens the pause menu on the
    inventory tab and Escape opens it on the last tab.  While the menu is open CommonUI's Menu
    mode is simulated: game keys do nothing, and Escape / I close it only when
    escape_closes_menu / i_closes_menu are True (the game: Escape closes it through the CommonUI
    back handler, I does not; escape_closes_menu=False simulates a regression of that fix).
    The inventory tab shows an Identify button for the unidentified amulet at a fixed
    viewport-pixel center (identify_button_center, default (812, 402)); a Left click there
    identifies it ("Mysterious Amulet").  A click's `hit` is the deepest widget Slate hit-tests,
    as in the C++ (Path.Widgets.Last()): "STextBlock" anywhere on a labelled button (its label
    fills it), "SBorder" on the open menu's background, "SViewport" on the game view, null off
    the viewport.  K (unsafe) kills the pawn; hazards kill it too.
  - wait_until limits (Predicate.cpp): longer than 4096 characters, or '!' / '(' nested deeper
    than 64, is bad_request; && / || chains are n-ary (no recursion per term).  A key listed
    twice in `keys` (or an axis twice in `values`, compared case-insensitively after aliases)
    is bad_request, as ParseKeyArgs / ParseAxisArgs.
  - screenshot writes a small valid PNG (bridge_NN.png in the shot dir by default).

Fault injection (Faults): crlf, split_writes (bytes per tick), response_delay_s,
garbage_before_response (malformed + blank lines), stale_before_response (a reply with an
unknown id), drop_after_requests, drop_on_cmd, error_replies {cmd: (code, message)},
no_reply_cmds, screenshot = ok | reply_early | truncated | timeout | failed, stall_s (a stalled
game thread: nothing is accepted, read or answered for that long after start; connects still
complete in the listen backlog, and a backlog socket its client already closed is accepted and
counted before its EOF is read, as in the C++ AcceptConnections), refuse_connections (the
first N connections get too_many_connections).

Usage (tests):
  from tests import fake_bridge
  world = fake_bridge.FakeWorld(escape_closes_menu=False)
  with fake_bridge.FakeBridgeServer(world, token="t", shot_dir=tmp).start() as server:
      bridge = agent_bridge.Bridge.attach(server.port, token="t")

Standalone (poke it with the CLI; Ctrl+C or the quit command stops it):
  py Scripts/tests/fake_bridge.py [--port 0] [--token T] [--shot-dir DIR] [--world-json FILE]
  py Scripts/agent_bridge.py --port <printed port> state

Exit codes (standalone): 0 -- stopped by quit or Ctrl+C;  2 -- could not listen
"""

import argparse
import collections
import json
import math
import os
import re
import socket
import struct
import sys
import threading
import time
import traceback
import uuid
import zlib

# ---------------------------------------------------------------------------
# Protocol constants (Specs/agent_bridge_v1.md)
# ---------------------------------------------------------------------------
BRIDGE_VERSION = 1
DEFAULT_PORT = 47777
MAX_LINE_BYTES = 1024 * 1024
MAX_CONNECTIONS = 4
MAX_CONDITION_LENGTH = 4096   # MordecaiAgentBridge::MaxConditionLength
MAX_CONDITION_DEPTH = 64      # MordecaiAgentBridge::MaxConditionDepth ('!' and '(' levels)
SECTIONS = ("game", "focus", "player", "input", "pickups", "inventory", "pause_menu", "widgets",
            "actors")
VIEWPORT_W, VIEWPORT_H = 1280, 720
DEFAULT_MAP = "/MordecaiCore/Game/Maps/DevTestMap"
DEFAULT_EXPERIENCE = "B_MordecaiArenaExperience"
SCREENSHOT_TIMEOUT_S = 10.0

# World constants (DevTestMap / AMordecaiCharacter, see Scratchpad map/movement-level.md)
WALK_SPEED = 600.0
PAWN_Z = 92.0
CAPSULE_RADIUS = 40.0
FOCUS_RANGE = 120.0
SPELL_COST = 2.0   # legacy single cost (unused): each spell's cost is in SPELL_COSTS

# ---------------------------------------------------------------------------
# Key names (section 5)
# ---------------------------------------------------------------------------
_DIGIT_NAMES = ["Zero", "One", "Two", "Three", "Four", "Five", "Six", "Seven", "Eight", "Nine"]
KEY_ALIASES = {"esc": "Escape", "space": "SpaceBar", "enter": "Enter", "return": "Enter",
               "shift": "LeftShift", "ctrl": "LeftControl", "alt": "LeftAlt",
               "up": "Up", "down": "Down", "left": "Left", "right": "Right"}
KEY_ALIASES.update({str(i): name for i, name in enumerate(_DIGIT_NAMES)})

KEYBOARD_KEYS = frozenset(
    [chr(c) for c in range(ord("A"), ord("Z") + 1)] + _DIGIT_NAMES
    + [f"F{i}" for i in range(1, 13)] + [f"NumPad{d}" for d in _DIGIT_NAMES]
    + ["Escape", "SpaceBar", "Enter", "Tab", "BackSpace", "CapsLock", "LeftShift", "RightShift",
       "LeftControl", "RightControl", "LeftAlt", "RightAlt", "LeftCommand", "RightCommand",
       "Up", "Down", "Left", "Right", "Insert", "Delete", "Home", "End", "PageUp", "PageDown",
       "Tilde", "Semicolon", "Equals", "Comma", "Hyphen", "Underscore", "Period", "Slash",
       "BackSlash", "LeftBracket", "RightBracket", "Apostrophe", "Quote", "Pause", "NumLock",
       "ScrollLock", "Multiply", "Add", "Subtract", "Decimal", "Divide"])
GAMEPAD_BUTTONS = frozenset([
    "Gamepad_FaceButton_Bottom", "Gamepad_FaceButton_Right", "Gamepad_FaceButton_Left",
    "Gamepad_FaceButton_Top", "Gamepad_LeftShoulder", "Gamepad_RightShoulder",
    "Gamepad_LeftTrigger", "Gamepad_RightTrigger", "Gamepad_DPad_Up", "Gamepad_DPad_Down",
    "Gamepad_DPad_Left", "Gamepad_DPad_Right", "Gamepad_Special_Left", "Gamepad_Special_Right",
    "Gamepad_LeftThumbstick", "Gamepad_RightThumbstick"])
MOUSE_BUTTONS = frozenset(["LeftMouseButton", "RightMouseButton", "MiddleMouseButton",
                           "ThumbMouseButton", "ThumbMouseButton2"])
AXIS_RANGES = {"Gamepad_LeftX": (-1.0, 1.0), "Gamepad_LeftY": (-1.0, 1.0),
               "Gamepad_RightX": (-1.0, 1.0), "Gamepad_RightY": (-1.0, 1.0),
               "Gamepad_LeftTriggerAxis": (0.0, 1.0), "Gamepad_RightTriggerAxis": (0.0, 1.0)}
OTHER_AXES = frozenset(["MouseX", "MouseY", "Mouse2D", "MouseWheelAxis", "Gamepad_Left2D",
                        "Gamepad_Right2D"])
DENIED_KEYS = frozenset(["K", "Insert", "Delete", "Home", "End", "PageUp", "PageDown",
                         "NumPadFour", "Tilde", "Semicolon", "Gamepad_Special_Left"]
                        + [f"F{i}" for i in range(1, 13)])
LIVE_KEYS = ("W", "A", "S", "D", "F", "I", "Escape", "Q", "E", "R", "One", "Tab", "BackSlash")
MOUSE_KEY_FOR_BUTTON = {"Left": "LeftMouseButton", "Right": "RightMouseButton",
                        "Middle": "MiddleMouseButton"}
_CANONICAL = {k.lower(): k for k in
              KEYBOARD_KEYS | GAMEPAD_BUTTONS | MOUSE_BUTTONS | set(AXIS_RANGES) | OTHER_AXES}


class ProtocolError(Exception):
    """A command failure that becomes {"ok": false, "error": code, "message": ..., **extra}."""

    def __init__(self, code, message, **extra):
        super().__init__(message)
        self.code = code
        self.message = message
        self.extra = extra


def resolve_key(name):
    """FKey name (case-insensitive, aliases) -> (canonical name, kind).

    kind is "keyboard", "gamepad", "mouse" or "axis".  Unknown names raise bad_key.
    """
    if not isinstance(name, str) or not name.strip():
        raise ProtocolError("bad_key", f"bad key name {name!r}")
    low = name.strip().lower()
    canon = KEY_ALIASES.get(low) or _CANONICAL.get(low)
    if canon is None:
        raise ProtocolError("bad_key", f"unknown key '{name}'")
    if canon in MOUSE_BUTTONS:
        kind = "mouse"
    elif canon in AXIS_RANGES or canon in OTHER_AXES:
        kind = "axis"
    elif canon in GAMEPAD_BUTTONS:
        kind = "gamepad"
    else:
        kind = "keyboard"
    return canon, kind


# ---------------------------------------------------------------------------
# PNG writer (no Pillow needed)
# ---------------------------------------------------------------------------
PNG_SIGNATURE = b"\x89PNG\r\n\x1a\n"


def make_png(width=16, height=9, rgb=(32, 48, 64)):
    """A small, valid, solid-colour RGB PNG."""
    def chunk(tag, data):
        return (struct.pack(">I", len(data)) + tag + data
                + struct.pack(">I", zlib.crc32(tag + data) & 0xFFFFFFFF))
    raw = b"".join(b"\x00" + bytes(rgb) * width for _ in range(height))
    return (PNG_SIGNATURE
            + chunk(b"IHDR", struct.pack(">IIBBBBB", width, height, 8, 2, 0, 0, 0))
            + chunk(b"IDAT", zlib.compress(raw))
            + chunk(b"IEND", b""))


# ---------------------------------------------------------------------------
# wait_until condition language (section 4)
# ---------------------------------------------------------------------------
class ConditionError(ValueError):
    pass


_TOKEN_RE = re.compile(r"""
    (?P<ws>\s+)
  | (?P<num>-?(?:\d+(?:\.\d*)?|\.\d+)(?:[eE][+-]?\d+)?)
  | (?P<str>"(?:[^"\\]|\\.)*"|'(?:[^'\\]|\\.)*')
  | (?P<op>&&|\|\||==|!=|<=|>=|[<>!()\[\].])
  | (?P<ident>[A-Za-z_][A-Za-z0-9_]*)
""", re.VERBOSE)
_CMP_OPS = ("==", "!=", "<", "<=", ">", ">=")
_ESCAPES = {"n": "\n", "t": "\t", "r": "\r", "\\": "\\", '"': '"', "'": "'", "/": "/"}


def _unquote(token):
    body, out, i = token[1:-1], [], 0
    while i < len(body):
        if body[i] == "\\" and i + 1 < len(body):
            out.append(_ESCAPES.get(body[i + 1], body[i + 1]))
            i += 2
        else:
            out.append(body[i])
            i += 1
    return "".join(out)


def _tokenize(text):
    tokens, pos = [], 0
    while pos < len(text):
        m = _TOKEN_RE.match(text, pos)
        if not m:
            raise ConditionError(f"unexpected character {text[pos]!r} at offset {pos}")
        pos = m.end()
        if m.lastgroup == "num" and re.search(r"\.(?!\d)", m.group("num")):
            # The C++ tokenizer needs a digit after the '.': "1." and "1.e5" are malformed.
            raise ConditionError(f"malformed number at position {m.start()}")
        if m.lastgroup != "ws":
            tokens.append((m.lastgroup, m.group(m.lastgroup), m.start()))
    return tokens


def path_text(segments):
    out = segments[0]
    for s in segments[1:]:
        out += f"[{s}]" if isinstance(s, int) else f".{s}"
    return out


class _Parser:
    """Recursive descent over the grammar in Specs/agent_bridge_v1.md section 4.

    As the C++ parser: && and || build n-ary nodes ("and"/"or", [children]), so a long flat chain
    never recurses per term, and each '!' or '(' is one nesting level, at most
    MAX_CONDITION_DEPTH (that bounds the recursion of parse and evaluate).
    """

    def __init__(self, text):
        self.text = text
        self.tokens = _tokenize(text)
        self.i = 0
        self.depth = 0
        self.paths = []

    def enter_nesting(self):
        """One more level of '!' or '(' (C++ EnterNesting)."""
        self.depth += 1
        if self.depth > MAX_CONDITION_DEPTH:
            raise ConditionError("condition nested too deeply")

    def peek(self, offset=0):
        j = self.i + offset
        return self.tokens[j] if j < len(self.tokens) else (None, None, len(self.text))

    def next(self):
        tok = self.peek()
        self.i += 1
        return tok

    def is_op(self, value, offset=0):
        kind, text, _ = self.peek(offset)
        return kind == "op" and text == value

    def expect_op(self, value):
        kind, text, pos = self.next()
        if kind != "op" or text != value:
            raise ConditionError(f"expected '{value}' at offset {pos}, got {text!r}")

    def parse(self):
        if not self.tokens:
            raise ConditionError("empty condition")
        node = self.or_expr()
        if self.i < len(self.tokens):
            _, text, pos = self.peek()
            raise ConditionError(f"unexpected {text!r} at offset {pos}")
        return node

    def or_expr(self):
        first = self.and_expr()
        if not self.is_op("||"):
            return first
        children = [first]
        while self.is_op("||"):
            self.next()
            children.append(self.and_expr())
        return ("or", children)

    def and_expr(self):
        first = self.unary()
        if not self.is_op("&&"):
            return first
        children = [first]
        while self.is_op("&&"):
            self.next()
            children.append(self.unary())
        return ("and", children)

    def unary(self):
        if self.is_op("!"):
            self.next()
            self.enter_nesting()
            node = ("not", self.unary())
            self.depth -= 1
            return node
        return self.primary()

    def primary(self):
        if self.is_op("("):
            self.next()
            self.enter_nesting()
            node = self.or_expr()
            self.depth -= 1
            self.expect_op(")")
            return node
        return self.comparison()

    def comparison(self):
        left = self.operand()
        kind, text, _ = self.peek()
        if kind == "op" and text in _CMP_OPS:
            self.next()
            return ("cmp", text, left, self.operand())
        return ("bool", left)

    def operand(self):
        kind, text, pos = self.peek()
        if kind == "num":
            self.next()
            value = float(text)
            return ("lit", int(value) if value.is_integer() and re.fullmatch(r"-?\d+", text)
                    else value)
        if kind == "str":
            self.next()
            return ("lit", _unquote(text))
        if kind == "ident":
            if text in ("true", "false", "null"):
                self.next()
                return ("lit", {"true": True, "false": False, "null": None}[text])
            if text == "len" and self.is_op("(", 1):
                self.next()
                self.next()
                kind, text, pos = self.peek()
                if kind != "ident" or text in ("true", "false", "null"):
                    # As the C++ parser: len() takes a path, never a literal (len(true), len(5)).
                    raise ConditionError(f"len() needs a path at position {pos} (near "
                                         f"{text!r})" if text is not None else
                                         "len() needs a path at the end of the condition")
                segments = self.path()
                self.expect_op(")")
                return ("len", segments)
            return ("path", self.path())
        raise ConditionError(f"expected a path, len(path) or literal at offset {pos}, "
                             f"got {text!r}" if text is not None else
                             f"expected an operand at the end of the condition")

    def path(self):
        kind, text, pos = self.next()
        if kind != "ident":
            raise ConditionError(f"expected an identifier at offset {pos}, got {text!r}")
        segments = [text]
        while True:
            if self.is_op("."):
                self.next()
                kind, text, pos = self.next()
                if kind != "ident":
                    raise ConditionError(f"expected an identifier after '.' at offset {pos}")
                segments.append(text)
            elif self.is_op("["):
                self.next()
                kind, text, pos = self.next()
                if kind != "num" or not text.isdigit():
                    raise ConditionError(f"expected a non-negative integer index at offset {pos}")
                segments.append(int(text))
                self.expect_op("]")
            else:
                break
        self.paths.append(segments)
        return segments


def _is_number(v):
    return isinstance(v, (int, float)) and not isinstance(v, bool)


def truthy(v):
    """Truthy unless null, false, 0, "" or an empty array/object."""
    if v is None or v is False:
        return False
    if _is_number(v):
        return v != 0
    if isinstance(v, (str, list, dict)):
        return len(v) > 0
    return True


def resolve_path(state, segments):
    """A path that does not resolve is None."""
    cur = state
    for seg in segments:
        if isinstance(seg, int):
            if not isinstance(cur, list) or seg >= len(cur):
                return None
            cur = cur[seg]
        else:
            if not isinstance(cur, dict) or seg not in cur:
                return None
            cur = cur[seg]
    return cur


def compare(op, a, b):
    if op in ("==", "!="):
        if _is_number(a) and _is_number(b):
            equal = float(a) == float(b)
        elif type(a) is type(b) or (a is None and b is None):
            equal = a == b
        else:
            equal = False  # mismatched types are unequal
        return equal if op == "==" else not equal
    if (_is_number(a) and _is_number(b)) or (isinstance(a, str) and isinstance(b, str)):
        return {"<": a < b, "<=": a <= b, ">": a > b, ">=": a >= b}[op]
    return False


def length(v):
    return len(v) if isinstance(v, (list, str, dict)) else 0


class Condition:
    """A parsed wait_until condition.

    .roots lists the root identifiers of every path (the state sections to gather).
    evaluate(state) -> (bool, values); values maps each referenced path text to its last value,
    and each len(path) to the length (key "len(<path>)").
    """

    def __init__(self, text):
        if not isinstance(text, str):
            raise ConditionError("condition must be a string")
        if len(text) > MAX_CONDITION_LENGTH:  # checked before tokenizing, as the C++ Parse
            raise ConditionError(f"condition longer than {MAX_CONDITION_LENGTH} characters")
        parser = _Parser(text)
        self.text = text
        self.tree = parser.parse()
        self.paths = parser.paths
        roots = []
        for p in self.paths:
            if p[0] not in roots:
                roots.append(p[0])
        self.roots = roots

    def evaluate(self, state):
        values = {}
        return truthy(self._eval(self.tree, state, values)), values

    def _operand(self, node, state, values):
        kind = node[0]
        if kind == "lit":
            return node[1]
        if kind == "path":
            v = resolve_path(state, node[1])
            values[path_text(node[1])] = v
            return v
        n = length(resolve_path(state, node[1]))
        values[f"len({path_text(node[1])})"] = n
        return n

    def _eval(self, node, state, values):
        kind = node[0]
        if kind in ("or", "and"):
            # Every child is evaluated so every referenced path reports a value.
            results = [truthy(self._eval(child, state, values)) for child in node[1]]
            return any(results) if kind == "or" else all(results)
        if kind == "not":
            return not truthy(self._eval(node[1], state, values))
        if kind == "cmp":
            return compare(node[1], self._operand(node[2], state, values),
                           self._operand(node[3], state, values))
        return self._operand(node[1], state, values)  # "bool": bare operand


# ---------------------------------------------------------------------------
# The fake DevTestMap
# ---------------------------------------------------------------------------
# Item data from the committed DA_TestItem_* assets and DevTestMap.umap (state-items-ui.md s.3).
PICKUP_SPECS = (
    {"label": "TestPickup_HealthPotion", "item_id": "TestItem_HealthPotion",
     "display_name": "Health Potion", "location": [400.0, 0.0, 30.0], "route": "inventory",
     "type": "Consumable", "rarity": "Common", "priority": 0},
    {"label": "TestPickup_IronOre", "item_id": "TestItem_IronOre", "display_name": "Iron Ore",
     "location": [300.0, 300.0, 30.0], "route": "ledger", "type": "Material", "rarity": "Common",
     "priority": 0},
    {"label": "TestPickup_TownStone", "item_id": "TestItem_TownStone",
     "display_name": "Town Stone", "location": [0.0, 420.0, 30.0], "route": "ledger",
     "type": "TownResource", "rarity": "Common", "priority": 0},
    {"label": "TestPickup_UpgradeKey", "item_id": "TestItem_UpgradeKey",
     "display_name": "Upgrade Key", "location": [-300.0, 300.0, 30.0], "route": "inventory",
     "type": "UpgradeKey", "rarity": "Blue", "priority": 2},
    {"label": "TestPickup_MysteriousAmulet", "item_id": "TestItem_MysteriousAmulet",
     "display_name": "Unidentified Purple Amulet", "identified_name": "Mysterious Amulet",
     "location": [-400.0, 0.0, 30.0], "route": "inventory", "type": "MagicalItem",
     "rarity": "Purple", "priority": 0, "unidentified": True},
    {"label": "TestPickup_RustySword", "item_id": "TestItem_RustySword",
     "display_name": "Rusty Sword", "location": [-300.0, -300.0, 30.0], "route": "inventory",
     "type": "Weapon", "rarity": "Common", "priority": 0},
)
# UMordecaiInventoryComponent sort: SortPriority desc, then this type rank.
TYPE_RANK = {"Weapon": 0, "Armor": 1, "Trinket": 2, "MagicalItem": 3, "UpgradeKey": 4,
             "Quest": 5, "Consumable": 6, "Material": 7}
ENEMY_SPECS = (
    {"label": "BP_MordecaiEnemy_Frontliner_1", "location": [600.0, 0.0, 100.0]},
    {"label": "BP_MordecaiEnemy_Frontliner_2", "location": [-400.0, 500.0, 100.0]},
    {"label": "BP_MordecaiEnemy_Frontliner_3", "location": [-400.0, -500.0, 100.0]},
)
# (x0, x1, y0, y1) boxes.  Arena walls: inner faces at +/-975.
ARENA_WALLS = (
    ("Arena_Wall_N", (-1025.0, 1025.0, 975.0, 1025.0)),
    ("Arena_Wall_S", (-1025.0, 1025.0, -1025.0, -975.0)),
    ("Arena_Wall_E", (975.0, 1025.0, -1025.0, 1025.0)),
    ("Arena_Wall_W", (-1025.0, -975.0, -1025.0, 1025.0)),
)
# The blocks inside the DevTestMap arena (movement-level.md s.5).
DEVTESTMAP_OBSTACLES = (
    (600.0, 1000.0, -25.0, 25.0),      # Wall_01
    (-625.0, -575.0, 250.0, 550.0),    # Wall_02
    (225.0, 375.0, -575.0, -425.0),    # Box_01
    (-450.0, -350.0, -350.0, -250.0),  # Box_02
    (-125.0, 125.0, 575.0, 825.0),     # Box_03
    (-1000.0, -800.0, -715.0, -685.0),  # Cover_01
    (485.0, 515.0, 800.0, 1000.0),     # Cover_02
)
PAUSE_TABS = (("character", "Character"), ("skills", "Skills"), ("feats", "Feats"),
              ("inventory", "Inventory"), ("settings", "Settings"))
INVENTORY_FILTERS = ("All", "Weapons", "Armor", "Trinkets", "Consumables", "Materials", "Quest",
                     "Magical")
# The packaged game's loadout (AMordecaiPlayerController::DefaultSpellAbilities + DA_Spell_*,
# AC-057.1-4): (key, spell_id, display_name); the ability class is MordecaiGA_<spell_id>.  No
# DA_Spell_* sets a CooldownTag (Known issues: spells have no cooldowns), so cooldown_tag is "".
SPELL_SLOTS = (("Q", "Fireball", "Fireball"), ("E", "Blink", "Blink"),
               ("R", "StoneSkin", "Stone Skin"), ("One", "Restoration", "Restoration"))
SPELL_COSTS = {"Fireball": 3.0, "Blink": 2.0, "StoneSkin": 4.0, "Restoration": 3.0}
SPELL_KEYS = {key: spell_id for key, spell_id, _ in SPELL_SLOTS}
DEFAULT_MAPPING = {"W": (1.0, 0.0), "S": (-1.0, 0.0), "D": (0.0, 1.0), "A": (0.0, -1.0)}


def _box(b):
    x0, x1, y0, y1 = (float(v) for v in b)
    return (min(x0, x1), max(x0, x1), min(y0, y1), max(y0, y1))


def _r(v, nd=2):
    return round(float(v), nd)


class FakeWorld:
    """Game state behind the fake bridge.  Thread-safe: every public method takes self.lock.

    Options (all keyword, JSON-friendly so fake_game.py can pass them from a file):
      mapping              {"W": (dx, dy), ...} world direction of each move key (default W=+X,
                           S=-X, D=+Y, A=-Y); pass a wrong one to test walk_to's calibration
      speed                walk speed, uu/s (600)
      start                spawn XY (0, 0); control_yaw: control rotation yaw (0)
      obstacles            extra (x0, x1, y0, y1) boxes, e.g. DEVTESTMAP_OBSTACLES
      arena_walls          include the +/-975 arena walls (True)
      hazards              boxes that kill the pawn on entry
      focus_range          pickup focus distance, XY (120)
      escape_closes_menu   Escape closes an open pause menu (True, as in the game: the CommonUI
                           back handler; False simulates a regression of that fix)
      i_closes_menu        I closes an open pause menu (False, as in the game: no UI action
                           binding)
      identify_button_center / identify_button_size   the amulet's Identify button, viewport px
      boot_s               seconds until the experience is loaded and the pawn exists (0)
      loading_s            the loading screen stays up this long after boot_s (0)
      not_ready_s          commands that need the viewport answer not_ready this long (0)
      respawn_s            seconds from death to respawn at the start (3; None = never)
      app_active           reported focus.app_active (True)
      stick_moves          Gamepad_LeftX/LeftY move the pawn (False: the real stick names are
                           invalid, see Known issues)
      map, experience      reported names
      pickups              None = all six, or a list of labels to place
      enemies              place the three invisible Frontliners (True; they never move)
      lose_pawn_on_f       F loses the pawn instead of picking up (as if it died or was
                           unpossessed that frame): nothing is picked up and every pawn section
                           (player, pickups, inventory) turns valid:false "no pawn yet" (False)
      pickup_adds_nothing  F removes the focused pickup from the world but adds nothing to the
                           inventory or ledger (a broken pickup) (False)
    """

    def __init__(self, *, mapping=None, speed=WALK_SPEED, start=(0.0, 0.0), control_yaw=0.0,
                 obstacles=(), arena_walls=True, hazards=(), focus_range=FOCUS_RANGE,
                 escape_closes_menu=True, i_closes_menu=False,
                 identify_button_center=(812, 402), identify_button_size=(96, 28),
                 boot_s=0.0, loading_s=0.0, not_ready_s=0.0, respawn_s=3.0, app_active=True,
                 stick_moves=False, map=DEFAULT_MAP, experience=DEFAULT_EXPERIENCE,
                 pickups=None, enemies=True, lose_pawn_on_f=False, pickup_adds_nothing=False):
        self.lock = threading.RLock()
        self.t0 = time.monotonic()
        self.mapping = {k: (float(v[0]), float(v[1]))
                        for k, v in (DEFAULT_MAPPING if mapping is None else mapping).items()}
        self.speed = float(speed)
        self.start = (float(start[0]), float(start[1]))
        self.control_yaw = float(control_yaw)
        self.wall_boxes = [(name, _box(b)) for name, b in ARENA_WALLS] if arena_walls else []
        self.obstacles = [b for _, b in self.wall_boxes] + [_box(b) for b in (obstacles or ())]
        self.hazards = [_box(b) for b in (hazards or ())]
        self.focus_range = float(focus_range)
        self.escape_closes_menu = bool(escape_closes_menu)
        self.i_closes_menu = bool(i_closes_menu)
        self.identify_center = (float(identify_button_center[0]), float(identify_button_center[1]))
        self.identify_size = (float(identify_button_size[0]), float(identify_button_size[1]))
        self.boot_s = float(boot_s)
        self.loading_s = float(loading_s)
        self.not_ready_s = float(not_ready_s)
        self.respawn_s = None if respawn_s is None else float(respawn_s)
        self.app_active = bool(app_active)
        self.stick_moves = bool(stick_moves)
        self.map = map
        self.experience = experience
        self.world_pickups = [dict(p, location=list(p["location"])) for p in PICKUP_SPECS
                              if pickups is None or p["label"] in pickups]
        self.enemies = [dict(e, location=list(e["location"])) for e in ENEMY_SPECS] if enemies else []
        self.lose_pawn_on_f = bool(lose_pawn_on_f)
        self.pickup_adds_nothing = bool(pickup_adds_nothing)
        self.pawn_lost = False

        self.pos = [self.start[0], self.start[1]]
        self.vel = [0.0, 0.0]
        self.frame = 0
        self.world_time = 0.0
        self.held = {}             # connection id -> {key: value}
        self.menu_open = False
        self.active_tab = None
        self.last_tab = "character"
        self.menu_open_frame = -10
        self.inventory_filter = "All"
        self.inventory = []        # item instances, pickup order
        self.ledger = {}           # item_id -> {item_id, display_name, count}
        self.dead_since = None
        self.max_health = 100.0
        self.health = 100.0
        self.max_spell_points = 10.0
        self.spell_points = 10.0
        self.weapon_index = 0
        self.kills = 0
        self.cursor = None
        self.virtual_cursor = False
        self.events = []           # (due real time, fn(world))
        self.input_log = []        # (t, "down"|"up", key) for test assertions
        self.casts = []
        self.picked = []
        self._pressed_target = None

    @classmethod
    def from_options(cls, options):
        """Build from a JSON-decoded dict (fake_game.py -FakeWorldJson=)."""
        return cls(**(options or {}))

    # --- clocks and readiness -------------------------------------------------
    def real_time(self):
        return time.monotonic() - self.t0

    @property
    def experience_loaded(self):
        return self.real_time() >= self.boot_s

    @property
    def has_pawn(self):
        return self.experience_loaded and not self.pawn_lost

    @property
    def loading_screen(self):
        return self.real_time() < self.boot_s + self.loading_s

    @property
    def viewport_ready(self):
        return self.real_time() >= self.not_ready_s

    @property
    def ready(self):
        return self.experience_loaded and not self.loading_screen and self.has_pawn

    @property
    def paused(self):
        return self.menu_open

    @property
    def game_input_allowed(self):
        return self.ready and not self.menu_open

    @property
    def is_dead(self):
        return self.dead_since is not None

    @property
    def menu_painted(self):
        return self.menu_open and self.frame - self.menu_open_frame >= 1

    def after(self, seconds, fn):
        """Run fn(world) on the game thread `seconds` from now (real time)."""
        with self.lock:
            self.events.append((self.real_time() + float(seconds), fn))

    # --- simulation -------------------------------------------------------------
    def tick(self, dt):
        with self.lock:
            self.frame += 1
            now = self.real_time()
            due = [e for e in self.events if e[0] <= now]
            if due:
                self.events = [e for e in self.events if e[0] > now]
                for _, fn in due:
                    try:
                        fn(self)
                    except Exception:
                        traceback.print_exc()
            if (self.dead_since is not None and self.respawn_s is not None
                    and now - self.dead_since >= self.respawn_s):
                self.respawn()
            if not self.menu_open:
                self.world_time += dt
            self._move(dt)

    def _held_union(self):
        """{input: value} over every connection; an axis reads its largest |value|."""
        out = {}
        for keys in self.held.values():
            for k, v in keys.items():
                if abs(v) >= abs(out.get(k, 0.0)):
                    out[k] = v
        return out

    def _holders(self):
        """{input: number of connections holding it}."""
        out = {}
        for keys in self.held.values():
            for k in keys:
                out[k] = out.get(k, 0) + 1
        return out

    def _log_ups(self, before):
        """Record an "up" for every key in `before` that no connection holds any more."""
        now = self._held_union()
        for key in sorted(before):
            if key not in now:
                self.input_log.append((_r(self.real_time(), 3), "up", key))

    def _blocked(self, x, y):
        rr = CAPSULE_RADIUS * CAPSULE_RADIUS
        for x0, x1, y0, y1 in self.obstacles:
            cx, cy = min(max(x, x0), x1), min(max(y, y0), y1)
            if (x - cx) ** 2 + (y - cy) ** 2 < rr:
                return True
        return False

    def _move(self, dt):
        if not self.game_input_allowed or self.is_dead:
            self.vel = [0.0, 0.0]
            return
        held = self._held_union()
        vx = vy = 0.0
        for key, (mx, my) in self.mapping.items():
            if key in held:
                vx += mx
                vy += my
        if self.stick_moves:
            lx, ly = held.get("Gamepad_LeftX", 0.0), held.get("Gamepad_LeftY", 0.0)
            if math.hypot(lx, ly) > 0.2:
                vx += ly
                vy += lx
        mag = math.hypot(vx, vy)
        if mag < 1e-9:
            self.vel = [0.0, 0.0]
            return
        if mag > 1.0:
            vx, vy = vx / mag, vy / mag
        vx, vy = vx * self.speed, vy * self.speed
        x, y = self.pos
        nx = x + vx * dt
        if self._blocked(nx, y):
            vx = 0.0
        else:
            x = nx
        ny = y + vy * dt
        if self._blocked(x, ny):
            vy = 0.0
        else:
            y = ny
        self.pos = [x, y]
        self.vel = [vx, vy]
        if any(x0 <= x <= x1 and y0 <= y <= y1 for x0, x1, y0, y1 in self.hazards):
            self.kill_player()

    # --- input ------------------------------------------------------------------
    # Union semantics: a key goes down (input_log "down", gameplay reacts) when its FIRST holder
    # presses it and up ("up") when its LAST holder lets go; the C++ bridge refcounts the same way.
    def key_down(self, conn_id, key):
        with self.lock:
            already = key in self._held_union()
            self.held.setdefault(conn_id, {})[key] = 1.0
            if not already:
                self.input_log.append((_r(self.real_time(), 3), "down", key))
                self._on_key_pressed(key)

    def key_up(self, conn_id, key):
        with self.lock:
            keys = self.held.get(conn_id, {})
            was_held = key in keys
            keys.pop(key, None)
            if was_held:
                self._log_ups([key])
            return was_held

    def holds(self, conn_id, key):
        """True if this connection holds `key`."""
        with self.lock:
            return key in self.held.get(conn_id, {})

    @staticmethod
    def _key_names(names):
        return [k for k in names if k not in AXIS_RANGES and k not in MOUSE_BUTTONS]

    def set_axis(self, conn_id, key, value):
        with self.lock:
            keys = self.held.setdefault(conn_id, {})
            if value == 0:
                keys.pop(key, None)
            else:
                keys[key] = float(value)

    def release_conn(self, conn_id):
        with self.lock:
            before = self._key_names(self.held.pop(conn_id, None) or {})
            self._log_ups(before)
            self._pressed_target = None

    def release_all(self):
        with self.lock:
            before = self._key_names(self._held_union())
            self.held.clear()
            self._log_ups(before)
            self._pressed_target = None

    def move_cursor(self, x, y):
        with self.lock:
            self.cursor = [x, y]
            self.virtual_cursor = True

    def _on_key_pressed(self, key):
        if not self.ready:
            return  # the loading screen eats every input event
        if self.menu_open:
            # CommonUI Menu input mode: no game key reaches Enhanced Input.
            if key == "Escape" and self.escape_closes_menu:
                self.close_menu()
            elif key == "I" and self.i_closes_menu:
                self.close_menu()
            return
        if key == "I":
            self.open_menu("inventory")
        elif key == "Escape":
            self.open_menu(self.last_tab or "character")
        elif self.is_dead:
            return
        elif key == "F":
            if self.lose_pawn_on_f:
                self.pawn_lost = True  # fault: the pawn is gone the frame F goes down
                self.vel = [0.0, 0.0]
                return
            self._try_pickup()
        elif key in SPELL_KEYS:
            cost = SPELL_COSTS[SPELL_KEYS[key]]
            if self.spell_points >= cost:  # SP max 10, no regeneration (as the game)
                self.spell_points -= cost
                self.casts.append(key)
        elif key == "Tab":
            self.weapon_index = (self.weapon_index + 1) % 2
        elif key == "BackSlash":
            self.weapon_index = (self.weapon_index - 1) % 2
        elif key == "K":
            self.kill_player()  # DamageSelfDestruct debug exec binding

    # --- mouse --------------------------------------------------------------------
    def buttons(self):
        """Clickable buttons on screen: {id, label, center, size, enabled, action, widget}."""
        out = []
        if not self.menu_painted:
            return out
        for i, (tab_id, name) in enumerate(PAUSE_TABS):
            out.append({"id": tab_id, "label": name, "center": [240 + 200 * i, 110],
                        "size": [180, 36], "enabled": True, "action": ("tab", tab_id),
                        "widget": "menu"})
        if self.active_tab == "inventory":
            for i, f in enumerate(INVENTORY_FILTERS):
                # The game emits FName-derived button ids lowercased (packaged builds lose FName case)
                out.append({"id": f.lower(), "label": f, "center": [190 + 95 * i, 170], "size": [88, 28],
                            "enabled": True, "action": ("filter", f), "widget": "inventory"})
            unidentified = [it for it in self.sorted_items() if not it["identified"]]
            for n, it in enumerate(unidentified):
                cx, cy = self.identify_center
                out.append({"id": it["instance_id"], "label": "Identify",
                            "center": [cx, cy + 34 * n], "size": list(self.identify_size),
                            "enabled": True, "action": ("identify", it["instance_id"]),
                            "widget": "inventory"})
        return out

    def hit_test(self, x, y):
        """(SWidget type under the point, button or None).

        The type is the DEEPEST widget Slate hit-tests there, as the C++ click's `hit`
        (Path.Widgets.Last()): on a labelled button that is its label's STextBlock (the packaged
        game's Identify click reported "STextBlock"); every fake button is labelled and its label
        is treated as filling it.  The click still goes to the button (SButton semantics in
        mouse_up).  A point outside the viewport hits nothing: (None, None), the C++ hit:null.
        """
        with self.lock:
            if not (0 <= x < VIEWPORT_W and 0 <= y < VIEWPORT_H):
                return None, None
            for b in self.buttons():
                (cx, cy), (w, h) = b["center"], b["size"]
                if abs(x - cx) <= w / 2.0 and abs(y - cy) <= h / 2.0:
                    return ("STextBlock" if b.get("label") else "SButton"), b
            if self.menu_painted:
                return "SBorder", None  # the menu's full-screen BackgroundDim
            return "SViewport", None

    def mouse_down(self, conn_id, x, y, button):
        with self.lock:
            self.move_cursor(x, y)
            self.held.setdefault(conn_id, {})[MOUSE_KEY_FOR_BUTTON[button]] = 1.0
            hit, target = self.hit_test(x, y)
            self._pressed_target = target if button == "Left" else None
            return hit

    def mouse_up(self, conn_id, x, y, button):
        """SButton semantics: the click fires on release over the button that was pressed."""
        with self.lock:
            self.held.get(conn_id, {}).pop(MOUSE_KEY_FOR_BUTTON[button], None)
            hit, target = self.hit_test(x, y)
            pressed, self._pressed_target = self._pressed_target, None
            if (button == "Left" and target is not None and pressed is not None
                    and target["action"] == pressed["action"] and target["enabled"]):
                kind, arg = target["action"]
                if kind == "tab":
                    self.active_tab = arg
                elif kind == "filter":
                    self.inventory_filter = arg
                elif kind == "identify":
                    self.identify(arg)
            return hit

    # --- gameplay helpers (tests may call these directly to set up a scene) ----------
    def open_menu(self, tab="character"):
        with self.lock:
            self.menu_open = True
            self.active_tab = tab
            self.menu_open_frame = self.frame

    def close_menu(self):
        with self.lock:
            if self.menu_open:
                self.last_tab = self.active_tab or self.last_tab
            self.menu_open = False
            self.active_tab = None

    def kill_player(self):
        with self.lock:
            if self.dead_since is None:
                self.dead_since = self.real_time()
                self.health = 0.0
                self.vel = [0.0, 0.0]

    def respawn(self):
        with self.lock:
            self.dead_since = None
            self.health = self.max_health
            self.pos = [self.start[0], self.start[1]]

    def set_position(self, x, y):
        with self.lock:
            self.pos = [float(x), float(y)]

    def focused_pickup(self):
        with self.lock:
            if not self.has_pawn or self.is_dead:
                return None
            best, best_d = None, None
            for p in self.world_pickups:
                d = math.hypot(p["location"][0] - self.pos[0], p["location"][1] - self.pos[1])
                if d <= self.focus_range and (best_d is None or d < best_d):
                    best, best_d = p, d
            return best

    def _in_range_count(self):
        return sum(1 for p in self.world_pickups
                   if math.hypot(p["location"][0] - self.pos[0],
                                 p["location"][1] - self.pos[1]) <= self.focus_range)

    def _try_pickup(self):
        p = self.focused_pickup()
        if p is None:
            return
        self.world_pickups.remove(p)
        self.picked.append(p["label"])
        if self.pickup_adds_nothing:
            return  # fault: the pickup vanished, nothing was stored
        if p.get("route") == "ledger":
            entry = self.ledger.setdefault(p["item_id"], {"item_id": p["item_id"],
                                                          "display_name": p["display_name"],
                                                          "count": 0})
            entry["count"] += 1
        else:
            self.inventory.append({
                "instance_id": uuid.uuid4().hex.upper(), "item_id": p["item_id"],
                "display_name": p["display_name"],
                "true_name": p.get("identified_name", p["display_name"]),
                "quantity": 1, "type": p["type"], "rarity": p["rarity"],
                "identified": not p.get("unidentified", False), "equipped": False,
                "priority": p.get("priority", 0)})

    def identify(self, instance_id):
        with self.lock:
            for it in self.inventory:
                if it["instance_id"] == instance_id and not it["identified"]:
                    it["identified"] = True
                    it["display_name"] = it["true_name"]
                    return True
            return False

    def sorted_items(self):
        return sorted(self.inventory, key=lambda it: (-it["priority"],
                                                      TYPE_RANK.get(it["type"], 99),
                                                      it["display_name"]))

    def sorted_ledger(self):
        return sorted(self.ledger.values(), key=lambda e: e["display_name"])

    # --- state (section 4) -------------------------------------------------------------
    def state(self, sections=None, radius=1500.0, include_all_actors=False):
        with self.lock:
            out = {}
            for name in (sections or SECTIONS):
                if name == "actors":
                    out[name] = self._actors(radius, include_all_actors)
                else:
                    out[name] = getattr(self, "_s_" + name)()
            return out

    def ping_fields(self):
        with self.lock:
            return {"map": self.map,
                    "experience": self.experience if self.experience_loaded else None,
                    "experience_loaded": self.experience_loaded,
                    "loading_screen": self.loading_screen, "has_pawn": self.has_pawn,
                    "ready": self.ready, "paused": self.paused,
                    "real_time_s": _r(self.real_time(), 3), "world_time_s": _r(self.world_time, 3),
                    "frame": self.frame}

    def _s_game(self):
        d = self.ping_fields()
        return {"valid": True, "map": d["map"], "experience": d["experience"],
                "experience_loaded": d["experience_loaded"], "loading_screen": d["loading_screen"],
                "ready": d["ready"], "paused": d["paused"], "real_time_s": d["real_time_s"],
                "world_time_s": d["world_time_s"], "fps": 60.0,
                "viewport_size": [VIEWPORT_W, VIEWPORT_H]}

    def _s_focus(self):
        return {"valid": True, "app_active": self.app_active, "app_has_focus": self.app_active,
                "viewport_has_focus": True, "focused_widget": "SViewport",
                "commonui_input_mode": "Menu" if self.menu_open else "Game",
                "game_input_allowed": self.game_input_allowed,
                "input_type": "MouseAndKeyboard", "virtual_cursor": self.virtual_cursor,
                "cursor": list(self.cursor) if self.cursor else None,
                "mouse_cursor_shown": self.menu_open}

    def _s_player(self):
        if not self.has_pawn:
            return {"valid": False, "reason": "no pawn yet (experience not loaded)"}
        dead = self.is_dead
        spells = []
        for slot, (key, spell_id, name) in enumerate(SPELL_SLOTS):
            cost = SPELL_COSTS[spell_id]
            spells.append({"slot": slot, "key": key, "ability": f"MordecaiGA_{spell_id}",
                           "granted": True, "spell_id": spell_id, "display_name": name,
                           "sp_cost": cost, "cooldown_tag": "", "cooldown_remaining_s": 0.0,
                           "active": False, "can_afford": self.spell_points >= cost})
        return {
            "valid": True, "name": "B_Hero_Mordecai_C_0", "class": "B_Hero_Mordecai_C",
            "location": [_r(self.pos[0]), _r(self.pos[1]), PAWN_Z],
            "rotation": [0.0, 0.0, 0.0],
            "control_rotation": [-50.0, self.control_yaw, 0.0],
            "velocity": [_r(self.vel[0]), _r(self.vel[1]), 0.0],
            "speed": _r(math.hypot(*self.vel)),
            "movement_mode": "None" if dead else "Walking", "is_dead": dead,
            "attributes": {"health": self.health, "max_health": self.max_health,
                           "stamina": 100.0, "max_stamina": 100.0, "posture": 0.0,
                           "max_posture": 100.0, "spell_points": self.spell_points,
                           "max_spell_points": self.max_spell_points},
            "tags": ["Mordecai.State.Dead"] if dead else [],
            "weapon": {"id": ["Weapon_TrainingSword", "Weapon_Staff"][self.weapon_index],
                       "display_name": ["Training Sword", "Staff"][self.weapon_index],
                       "type": ["Sword", "Staff"][self.weapon_index],
                       "index": self.weapon_index, "count": 2},
            "spells": spells, "kills": self.kills}

    def _s_input(self):
        held = []
        holders = self._holders()
        for key, value in sorted(self._held_union().items()):
            kind = ("axis" if key in AXIS_RANGES else
                    "mouse" if key in MOUSE_BUTTONS else "key")
            held.append({"key": key, "kind": kind, "value": value, "holders": holders[key]})
        pressed = ([h["key"] for h in held if h["kind"] != "axis"]
                   if self.game_input_allowed else [])
        return {"valid": True, "held": held, "pressed": pressed,
                "mapping_contexts": ["MordecaiIMC_Runtime"]}

    def _pickup_entry(self, p):
        dx, dy = p["location"][0] - self.pos[0], p["location"][1] - self.pos[1]
        dz = p["location"][2] - PAWN_Z
        return {"label": p["label"], "item_id": p["item_id"], "display_name": p["display_name"],
                "location": list(p["location"]), "distance": _r(math.sqrt(dx * dx + dy * dy + dz * dz), 1)}

    def _prompt_text(self, focused):
        return f"[F] Pick up {focused['display_name']}" if focused else ""

    def _s_pickups(self):
        if not self.has_pawn:
            return {"valid": False, "reason": "no pawn yet"}
        focused = self.focused_pickup()
        world = sorted((self._pickup_entry(p) for p in self.world_pickups),
                       key=lambda e: e["distance"])
        return {"valid": True, "focused": self._pickup_entry(focused) if focused else None,
                "in_range_count": self._in_range_count(),
                "prompt": {"valid": True, "visible": focused is not None,
                           "text": self._prompt_text(focused), "hint_text": "[I] Inventory",
                           "hint_visible": True},
                "world": world}

    def _s_inventory(self):
        if not self.has_pawn:
            return {"valid": False, "reason": "no pawn yet"}
        items = [{k: it[k] for k in ("instance_id", "item_id", "display_name", "quantity", "type",
                                     "rarity", "identified", "equipped")}
                 for it in self.sorted_items()]
        ledger = [dict(e) for e in self.sorted_ledger()]
        return {"valid": True, "count": len(items), "items": items,
                "ledger_count": len(ledger), "ledger": ledger}

    def _row_text(self, it):
        return ("? " if not it["identified"] else "") + it["display_name"]

    def _s_pause_menu(self):
        if not self.menu_open:
            return {"valid": True, "open": False, "active_tab": None, "tabs": [],
                    "inventory_tab": None}
        inv = None
        if self.active_tab == "inventory" and self.menu_painted:
            buttons = {b["id"]: b for b in self.buttons() if b["action"][0] == "identify"}
            rows = []
            for it in self.sorted_items():
                b = buttons.get(it["instance_id"])
                rows.append({"text": self._row_text(it), "instance_id": it["instance_id"],
                             "unidentified": not it["identified"],
                             "identify_button": ({"label": b["label"], "center": list(b["center"]),
                                                  "enabled": b["enabled"]} if b else None)})
            ledger_rows = [f"{e['display_name']}  x{e['count']}" for e in self.sorted_ledger()]
            inv = {"visible": True, "rows": rows, "ledger_rows": ledger_rows,
                   "list_placeholder": {"visible": not rows, "text": "No items"},
                   "ledger_placeholder": {"visible": not ledger_rows,
                                          "text": "No stored resources"}}
        return {"valid": True, "open": True, "active_tab": self.active_tab,
                "tabs": [{"id": t, "name": n} for t, n in PAUSE_TABS], "inventory_tab": inv}

    def _s_widgets(self):
        out = [{"class": "WBP_CombatHUD_C", "name": "WBP_CombatHUD_C_0",
                "rect": [0, 0, VIEWPORT_W, VIEWPORT_H],
                "texts": [f"{self.spell_points:g} / {self.max_spell_points:g}",
                          f"Kills: {self.kills}"], "buttons": []}]
        if self.has_pawn:
            focused = self.focused_pickup()
            out.append({"class": "MordecaiPickupPromptWidget", "name": "PickupPrompt",
                        "rect": [430, 634, 420, 56],
                        "texts": ([self._prompt_text(focused)] if focused else [])
                        + ["[I] Inventory"], "buttons": []})
        if self.menu_painted:
            def public(b):
                return {"id": b["id"], "label": b["label"], "center": list(b["center"]),
                        "enabled": b["enabled"]}
            buttons = self.buttons()
            out.append({"class": "MordecaiPauseMenuWidget", "name": "MordecaiPauseMenuWidget_0",
                        "rect": [140, 40, 1000, 640],
                        "texts": ["Paused"] + [n for _, n in PAUSE_TABS],
                        "buttons": [public(b) for b in buttons if b["widget"] == "menu"]})
            if self.active_tab == "inventory":
                out.append({"class": "MordecaiInventoryWidget", "name": "MordecaiInventoryWidget_0",
                            "rect": [160, 150, 960, 520],
                            "texts": ([self._row_text(it) for it in self.sorted_items()]
                                      + ["Stored Resources"]
                                      + [f"{e['display_name']}  x{e['count']}"
                                         for e in self.sorted_ledger()]),
                            "buttons": [public(b) for b in buttons if b["widget"] == "inventory"]})
        return out

    def _actors(self, radius, include_all):
        px, py = self.pos
        entries = []

        def add(label, cls, native, kind, loc, **extra):
            d = math.sqrt((loc[0] - px) ** 2 + (loc[1] - py) ** 2 + (loc[2] - PAWN_Z) ** 2)
            if d > radius:
                return
            entry = {"label": label, "name": label, "class": cls, "native_class": native,
                     "kind": kind, "location": [_r(v) for v in loc], "distance": _r(d, 1),
                     "hidden": False, "has_visible_mesh": True, "health": None, "is_dead": None,
                     "ai_state": None, "tags": [], "widgets": []}
            entry.update(extra)
            entries.append(entry)

        if self.has_pawn:
            add("B_Hero_Mordecai_C_0", "B_Hero_Mordecai_C", "MordecaiCharacter", "pawn",
                [px, py, PAWN_Z], health={"current": self.health, "max": self.max_health},
                is_dead=self.is_dead, tags=["Mordecai.State.Dead"] if self.is_dead else [])
        for p in self.world_pickups:
            add(p["label"], "MordecaiItemPickup", "MordecaiItemPickup", "pickup", p["location"],
                widgets=[{"component": "NameLabel", "class": "TextRenderComponent",
                          "space": "World", "visible": True}])
        for e in self.enemies:
            add(e["label"], "BP_MordecaiEnemy_Frontliner_C", "MordecaiEnemyCharacter", "enemy",
                e["location"], has_visible_mesh=False, health={"current": 100.0, "max": 100.0},
                is_dead=False, ai_state="Approach",
                widgets=[{"component": "HealthBarWidget", "class": "WBP_EnemyHealthBar_C",
                          "space": "Screen", "visible": True}])
        if include_all:
            for name, (x0, x1, y0, y1) in self.wall_boxes:
                add(name, "StaticMeshActor", "StaticMeshActor", "other",
                    [(x0 + x1) / 2.0, (y0 + y1) / 2.0, 100.0])
        entries.sort(key=lambda e: e["distance"])
        return entries


# ---------------------------------------------------------------------------
# The server
# ---------------------------------------------------------------------------
class Faults:
    """Fault injection for FakeBridgeServer (all off by default)."""

    def __init__(self, *, crlf=False, split_writes=0, response_delay_s=0.0,
                 garbage_before_response=False, stale_before_response=False,
                 drop_after_requests=None, drop_on_cmd=None, error_replies=None,
                 no_reply_cmds=(), screenshot="ok", screenshot_timeout_s=SCREENSHOT_TIMEOUT_S,
                 screenshot_complete_delay_s=0.3, stall_s=0.0, refuse_connections=0):
        if screenshot not in ("ok", "reply_early", "truncated", "timeout", "failed"):
            raise ValueError(f"bad screenshot fault {screenshot!r}")
        self.stall_s = float(stall_s or 0.0)
        self.refuse_connections = int(refuse_connections or 0)
        self.crlf = bool(crlf)
        self.split_writes = int(split_writes or 0)
        self.response_delay_s = float(response_delay_s)
        self.garbage_before_response = bool(garbage_before_response)
        self.stale_before_response = bool(stale_before_response)
        self.drop_after_requests = drop_after_requests
        self.drop_on_cmd = drop_on_cmd
        self.error_replies = dict(error_replies or {})
        self.no_reply_cmds = set(no_reply_cmds or ())
        self.screenshot = screenshot
        self.screenshot_timeout_s = float(screenshot_timeout_s)
        self.screenshot_complete_delay_s = float(screenshot_complete_delay_s)

    @classmethod
    def from_options(cls, options):
        return cls(**(options or {}))


class _Conn:
    def __init__(self, sock, addr, conn_id):
        self.sock = sock
        self.addr = addr
        self.id = conn_id
        self.rbuf = b""
        self.discarding = False
        self.queue = collections.deque()   # ("req", dict) | ("resp", dict) in arrival order
        self.current = None                # (request, generator)
        self.out = collections.deque()     # [release_monotonic, bytes]
        self.closed = False
        self.peer_closed = False           # EOF read: no more requests, the received ones still run
        self.requests_seen = 0


def _num(req, name, default=None, lo=None, hi=None, required=False):
    if name not in req or req[name] is None:
        if required:
            raise ProtocolError("bad_request", f"missing '{name}'")
        return default
    v = req[name]
    if isinstance(v, bool) or not isinstance(v, (int, float)) or not math.isfinite(v):
        raise ProtocolError("bad_request", f"'{name}' must be a number")
    if (lo is not None and v < lo) or (hi is not None and v > hi):
        # The C++ ReadOptionalNumber message: "'%s' must be in %g..%g (got %g)"
        raise ProtocolError("bad_request", f"'{name}' must be in "
                                           f"{'' if lo is None else format(lo, 'g')}.."
                                           f"{'' if hi is None else format(hi, 'g')} "
                                           f"(got {v:g})")
    return v


def _bool(req, name, default=False):
    v = req.get(name, default)
    if v is None:
        return default
    if not isinstance(v, bool):
        raise ProtocolError("bad_request", f"'{name}' must be true or false")
    return v


def _str(req, name, default=None, choices=None):
    v = req.get(name, default)
    if v is None:
        return default
    if not isinstance(v, str):
        raise ProtocolError("bad_request", f"'{name}' must be a string")
    if choices is not None and v not in choices:
        raise ProtocolError("bad_request", f"'{name}' must be one of {', '.join(choices)}")
    return v


class FakeBridgeServer:
    """The fake bridge: a listener plus a 60 Hz game-thread tick, in one background thread.

    FakeBridgeServer(world=None, *, port=0, host="127.0.0.1", token="", pid=None, shot_dir=None,
                     faults=None, build=None, tick_hz=60, on_quit=None)
      port 0 picks a free port (read .port after start()).  pid defaults to this process.
      build is the ping 'build' object (None = no build_info.json).
      on_quit() runs on the server thread after a quit command has been handled.
    Attributes: .world, .faults, .port, .received (every parsed request, in arrival order),
      .quit_event (set when a quit command arrived), .stopped (set when the loop has ended),
      .accepted / .refused (connections served / answered too_many_connections, in total).
    connection_count() counts every connection not closed yet (a peer-EOF one still finishing
    its requests included); open_connection_count() counts those that hold one of the 4 slots.
    """

    def __init__(self, world=None, *, port=0, host="127.0.0.1", token="", pid=None,
                 shot_dir=None, faults=None, build=None, tick_hz=60.0, on_quit=None):
        self.world = world or FakeWorld()
        self.host = host
        self.requested_port = int(port)
        self.port = None
        self.token = token or ""
        self.pid = os.getpid() if pid is None else pid
        self.shot_dir = os.path.abspath(shot_dir or os.getcwd())
        self.faults = faults or Faults()
        self.build = build
        self.tick_hz = float(tick_hz)
        self.on_quit = on_quit
        self.received = []
        self.quit_event = threading.Event()
        self.stopped = threading.Event()
        self.conns = []
        self._listener = None
        self._thread = None
        self._stop = threading.Event()
        self._next_conn_id = 1
        self._shot_counter = 0
        self._quitting = False
        self._quit_conn = None
        self._quit_deadline = None
        self.accepted = 0
        self.refused = 0
        self._refuse_left = self.faults.refuse_connections
        self._stall_until = None
        self._handlers = {"ping": self._h_ping, "state": self._h_state, "key": self._h_key,
                          "axis": self._h_axis, "mouse": self._h_mouse, "click": self._h_click,
                          "screenshot": self._h_screenshot, "wait": self._h_wait,
                          "wait_until": self._h_wait_until, "quit": self._h_quit}

    # --- lifecycle --------------------------------------------------------------------
    def start(self):
        """Bind 127.0.0.1:<port> (no address reuse, like bInReusable=false) and start ticking.

        Raises OSError when the port cannot be bound.
        """
        listener = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        try:
            if hasattr(socket, "SO_EXCLUSIVEADDRUSE"):
                listener.setsockopt(socket.SOL_SOCKET, socket.SO_EXCLUSIVEADDRUSE, 1)
            listener.bind((self.host, self.requested_port))
            listener.listen(8)
            listener.setblocking(False)
        except OSError:
            listener.close()
            raise
        self._listener = listener
        self.port = listener.getsockname()[1]
        if self.faults.stall_s > 0:
            self._stall_until = time.monotonic() + self.faults.stall_s
        self._thread = threading.Thread(target=self._run, name=f"fake-bridge-{self.port}",
                                        daemon=True)
        self._thread.start()
        return self

    def stop(self, timeout=5.0):
        self._stop.set()
        if self._thread is not None and self._thread is not threading.current_thread():
            self._thread.join(timeout)

    def __enter__(self):
        if self._thread is None:
            self.start()
        return self

    def __exit__(self, *exc):
        self.stop()

    def connection_count(self):
        """Connections not closed yet, including peer-EOF ones still running their requests."""
        return len(self.conns)

    def open_connection_count(self):
        """Connections that count toward MAX_CONNECTIONS (C++ CountOpenConnections): a peer
        that sent EOF can never send another request, so it holds no slot."""
        return sum(1 for c in list(self.conns) if not c.closed and not c.peer_closed)

    # --- the loop -----------------------------------------------------------------------
    def _run(self):
        dt_nominal = 1.0 / self.tick_hz
        last = time.monotonic()
        next_tick = last
        try:
            while not self._stop.is_set():
                if self._stall_until is not None:
                    # A stalled game thread: nothing is accepted, read or answered.  Clients'
                    # connects still complete in the listen backlog.
                    if time.monotonic() < self._stall_until:
                        time.sleep(0.005)
                        continue
                    self._stall_until = None
                    last = next_tick = time.monotonic()
                # The C++ Tick order: receive, execute, flush, close finished, then accept.
                for c in list(self.conns):
                    self._recv(c)
                now = time.monotonic()
                dt = min(max(now - last, 0.0), 0.1)
                last = now
                with self.world.lock:
                    self.world.tick(dt)
                    for c in list(self.conns):
                        self._process(c)
                    if self._quitting and self._quit_deadline is None:
                        self._begin_quit()
                for c in list(self.conns):
                    self._flush(c)
                if self._quit_deadline is not None:
                    drained = all(not c.out for c in self.conns)
                    if drained or time.monotonic() >= self._quit_deadline:
                        break
                for c in list(self.conns):
                    self._close_if_finished(c)
                self._accept()
                next_tick += dt_nominal
                pause = next_tick - time.monotonic()
                if pause > 0:
                    time.sleep(pause)
                else:
                    next_tick = time.monotonic()
        except Exception:
            traceback.print_exc()
        finally:
            for c in list(self.conns):
                self._close_socket(c, graceful=True)
            self.conns = []
            try:
                self._listener.close()
            except OSError:
                pass
            self.stopped.set()
            if self._quitting:
                self.quit_event.set()
                if self.on_quit is not None:
                    try:
                        self.on_quit()
                    except Exception:
                        traceback.print_exc()

    def _accept(self):
        while True:
            try:
                sock, addr = self._listener.accept()
            except (BlockingIOError, InterruptedError):
                return
            except OSError:
                return
            # Like the C++ AcceptConnections: a peer-EOF connection holds no slot, but a socket
            # accepted earlier in this loop counts until a later tick reads its EOF (so after a
            # stall, backlog sockets whose clients already gave up fill the slots first).
            if (self.open_connection_count() >= MAX_CONNECTIONS or self._quitting
                    or self._refuse_left > 0):
                if self._refuse_left > 0:
                    self._refuse_left -= 1
                self.refused += 1
                self._refuse(sock)
                continue
            sock.setblocking(False)
            sock.setsockopt(socket.IPPROTO_TCP, socket.TCP_NODELAY, 1)
            self.conns.append(_Conn(sock, addr, self._next_conn_id))
            self._next_conn_id += 1
            self.accepted += 1

    def _refuse(self, sock):
        line = self._encode({"id": None, "ok": False, "cmd": None,
                             "error": "too_many_connections",
                             "message": f"at most {MAX_CONNECTIONS} concurrent connections"})
        try:
            sock.setblocking(True)
            sock.settimeout(1.0)
            sock.sendall(line)
            sock.shutdown(socket.SHUT_WR)
            end = time.monotonic() + 0.5
            sock.settimeout(0.05)
            while time.monotonic() < end:  # drain so close() does not reset the connection
                try:
                    if not sock.recv(65536):
                        break
                except socket.timeout:
                    continue
                except OSError:
                    break
        except OSError:
            pass
        finally:
            sock.close()

    def _recv(self, c):
        while not c.closed and not c.peer_closed:
            try:
                data = c.sock.recv(65536)
            except (BlockingIOError, InterruptedError):
                return
            except OSError:
                data = b""  # as the C++ Receive: a failed Recv is treated like EOF
            if not data:
                # Peer EOF: stop reading; the requests already received still run and reply,
                # and _close_if_finished closes the connection (releasing its input) after them.
                c.peer_closed = True
                return
            c.rbuf += data
            self._split_lines(c)

    def _split_lines(self, c):
        while not c.closed:
            nl = c.rbuf.find(b"\n")
            if nl < 0:
                if len(c.rbuf) > MAX_LINE_BYTES:
                    if not c.discarding:
                        c.queue.append(("resp", self._error_resp(
                            None, None, "bad_request", "request line longer than 1 MiB")))
                    c.discarding = True
                    c.rbuf = b""
                return
            raw, c.rbuf = c.rbuf[:nl], c.rbuf[nl + 1:]
            if c.discarding:
                c.discarding = False
                continue
            if len(raw) > MAX_LINE_BYTES:
                c.queue.append(("resp", self._error_resp(None, None, "bad_request",
                                                         "request line longer than 1 MiB")))
                continue
            raw = raw.rstrip(b"\r")
            if not raw.strip():
                continue
            try:
                req = json.loads(raw.decode("utf-8"))
            except (UnicodeDecodeError, ValueError) as e:
                c.queue.append(("resp", self._error_resp(None, None, "bad_request",
                                                         f"malformed JSON: {e}")))
                continue
            if not isinstance(req, dict):
                c.queue.append(("resp", self._error_resp(None, None, "bad_request",
                                                         "a request must be a JSON object")))
                continue
            c.requests_seen += 1
            self.received.append(req)
            limit = self.faults.drop_after_requests
            if limit is not None and c.requests_seen > limit:
                self._disconnect(c, abrupt=True)
                return
            c.queue.append(("req", req))

    def _process(self, c):
        while not c.closed and not (self._quitting and c is not self._quit_conn):
            if c.current is None:
                if self._quitting or not c.queue:
                    return
                kind, item = c.queue.popleft()
                if kind == "resp":
                    self._send(c, item)
                    continue
                cmd = item.get("cmd")
                if self.faults.drop_on_cmd is not None and cmd == self.faults.drop_on_cmd:
                    self._disconnect(c, abrupt=True)
                    return
                if cmd in self.faults.no_reply_cmds:
                    continue
                c.current = (item, self._dispatch(c, item))
            req, gen = c.current
            try:
                next(gen)
                return  # still running; resumed next tick
            except StopIteration as stop:
                resp = stop.value
            except ProtocolError as e:
                resp = self._error_resp(req.get("id"), req.get("cmd"), e.code, e.message, **e.extra)
            except Exception as e:
                traceback.print_exc()
                resp = self._error_resp(req.get("id"), req.get("cmd"), "failed",
                                        f"{type(e).__name__}: {e}")
            c.current = None
            self._send(c, resp)

    def _begin_quit(self):
        """quit: cancel every pending command, release all input, close on the next ticks."""
        for c in self.conns:
            if c.current is not None:
                req, gen = c.current
                gen.close()
                c.current = None
                self._send(c, self._error_resp(req.get("id"), req.get("cmd"), "cancelled",
                                               "cancelled by quit"))
            while c.queue:
                kind, item = c.queue.popleft()
                if kind == "resp":
                    self._send(c, item)
                else:
                    self._send(c, self._error_resp(item.get("id"), item.get("cmd"), "cancelled",
                                                   "cancelled by quit"))
        self.world.release_all()
        self._quit_deadline = time.monotonic() + 1.0

    def _dispatch(self, c, req):
        cmd = req.get("cmd")
        if "cmd" not in req or not isinstance(cmd, str) or not cmd:
            raise ProtocolError("bad_request", "missing or non-string 'cmd'")
        injected = self.faults.error_replies.get(cmd)
        if injected:
            raise ProtocolError(injected[0], injected[1] if len(injected) > 1 else "injected error")
        handler = self._handlers.get(cmd)
        if handler is None:
            raise ProtocolError("unknown_command", f"unknown command '{cmd}'")
        result = handler(c, req)
        if hasattr(result, "send"):  # a generator: the command takes time
            result = yield from result
        resp = {"id": req.get("id"), "ok": True, "cmd": cmd}
        resp.update(result or {})
        return resp

    # --- sending ------------------------------------------------------------------------
    @staticmethod
    def _error_resp(rid, cmd, code, message, **extra):
        resp = {"id": rid, "ok": False, "cmd": cmd if isinstance(cmd, str) else None,
                "error": code, "message": message}
        resp.update(extra)
        return resp

    def _encode(self, obj):
        eol = "\r\n" if self.faults.crlf else "\n"
        return (json.dumps(obj, separators=(",", ":"), ensure_ascii=False) + eol).encode("utf-8")

    def _send(self, c, resp):
        eol = b"\r\n" if self.faults.crlf else b"\n"
        data = b""
        if self.faults.garbage_before_response:
            data += b"{this is not json" + eol + eol + b"   " + eol
        if self.faults.stale_before_response:
            data += self._encode({"id": "stale-reply", "ok": True, "cmd": "ping"})
        data += self._encode(resp)
        c.out.append([time.monotonic() + self.faults.response_delay_s, data])

    def _flush(self, c):
        now = time.monotonic()
        while c.out and not c.closed:
            release, data = c.out[0]
            if release > now:
                return
            chunk = data[:self.faults.split_writes] if self.faults.split_writes else data
            try:
                sent = c.sock.send(chunk)
            except (BlockingIOError, InterruptedError):
                return
            except OSError:
                self._disconnect(c)
                return
            if sent < len(data):
                c.out[0][1] = data[sent:]
            else:
                c.out.popleft()
            if self.faults.split_writes or sent < len(chunk):
                return  # split writes: one piece per tick

    def _close_socket(self, c, graceful=False):
        c.closed = True
        try:
            if graceful:
                c.sock.shutdown(socket.SHUT_WR)
        except OSError:
            pass
        try:
            c.sock.close()
        except OSError:
            pass

    def _close_if_finished(self, c):
        """After a peer EOF: close once every received request has replied and the replies are
        out (C++ CloseFinishedConnections)."""
        if (c.peer_closed and not c.closed and c.current is None and not c.queue
                and not c.out):
            self._disconnect(c)

    def _disconnect(self, c, abrupt=False):
        """Close a connection: release everything it holds and drop its pending command without
        a reply (the C++ RemoveClosedConnections).  Used after a failed send, for a peer-EOF
        connection whose requests have finished, and by the drop faults.  Only quit cancels a
        pending command with a 'cancelled' reply."""
        if c.closed and c not in self.conns:
            return
        with self.world.lock:
            if c.current is not None:
                c.current[1].close()
                c.current = None
            self.world.release_conn(c.id)
        if abrupt:
            try:  # RST instead of FIN
                c.sock.setsockopt(socket.SOL_SOCKET, socket.SO_LINGER, struct.pack("hh", 1, 0))
            except OSError:
                pass
        self._close_socket(c)
        if c in self.conns:
            self.conns.remove(c)

    # --- command handlers -------------------------------------------------------------------
    def _require_viewport(self):
        if not self.world.viewport_ready:
            raise ProtocolError("not_ready", "no game viewport yet")

    def _input_fields(self):
        w = self.world
        return {"ready": w.ready, "env": {"focus_restored": not w.app_active,
                                          "virtual_cursor": w.virtual_cursor,
                                          "app_active": w.app_active}}

    def _h_ping(self, c, req):
        resp = {"bridge_version": BRIDGE_VERSION, "pid": self.pid, "token": self.token,
                "build": self.build}
        resp.update(self.world.ping_fields())
        return resp

    def _h_state(self, c, req):
        self._require_viewport()
        sections = req.get("sections")
        if sections is None:
            sections = list(SECTIONS)
        elif not isinstance(sections, list) or not all(isinstance(s, str) for s in sections):
            raise ProtocolError("bad_request", "'sections' must be an array of section names")
        unknown = [s for s in sections if s not in SECTIONS]
        if unknown:
            raise ProtocolError("bad_request", f"unknown section(s): {', '.join(unknown)}")
        ordered = []
        for s in sections:
            if s not in ordered:
                ordered.append(s)
        # C++ ParseStateArgs: 0..1e7, and a given radius must be > 0.
        radius = _num(req, "radius", 1500, lo=0, hi=1.0e7)
        if req.get("radius") is not None and radius <= 0:
            raise ProtocolError("bad_request", "'radius' must be greater than 0")
        include_all = _bool(req, "include_all_actors")
        return {"state": self.world.state(ordered, radius, include_all)}

    def _resolve_keys(self, req):
        has_key, has_keys = "key" in req, "keys" in req
        if has_key == has_keys:
            raise ProtocolError("bad_request", "give exactly one of 'key' or 'keys'")
        names = [req["key"]] if has_key else req["keys"]
        if not isinstance(names, list) or not 1 <= len(names) <= 4:
            raise ProtocolError("bad_request", "'keys' must be an array of 1-4 key names")
        unsafe = _bool(req, "unsafe")
        keys = []
        for name in names:
            fkey, kind = resolve_key(name)
            if kind == "mouse":
                raise ProtocolError("bad_key", f"'{fkey}' is a mouse button: use click")
            if kind == "axis":
                raise ProtocolError("bad_key", f"'{fkey}' is an axis: use axis")
            if fkey in DENIED_KEYS and not unsafe:
                raise ProtocolError("bad_key", f"'{fkey}' is a denied debug key "
                                               "(Development exec binding); pass unsafe:true")
            if fkey in keys:  # C++ ParseKeyArgs; names resolve case-insensitively and by alias
                raise ProtocolError("bad_request", f"key '{fkey}' is listed twice")
            keys.append(fkey)
        return keys, has_key

    def _h_key(self, c, req):
        # Order as in the C++ HandleKey: ParseKeyArgs, then not_ready, then the held-key check.
        action = _str(req, "action", "tap", choices=("press", "release", "tap"))
        # hold_s is range-checked for every action and used by tap only (press/release ignore it).
        hold_s = _num(req, "hold_s", 0.1, lo=0, hi=30)
        route = _str(req, "route", "slate", choices=("slate", "viewport"))
        keys, single = self._resolve_keys(req)
        self._require_viewport()
        w = self.world
        if action != "release":
            for k in keys:
                if w.holds(c.id, k):
                    raise ProtocolError("bad_request", f"key '{k}' is already held by this "
                                                       "connection; release it first")
        base = {"keys": keys, "action": action, "route": route}
        if route == "viewport":
            base["diagnostic"] = True
        if action == "press":
            for k in keys:
                w.key_down(c.id, k)
            base.update(self._input_fields())
            return base
        if action == "release":
            was = [w.key_up(c.id, k) for k in keys]
            base["was_held"] = was[0] if single else was
            base.update(self._input_fields())
            return base
        return self._tap(c, keys, hold_s, base)

    def _tap(self, c, keys, hold_s, base):
        w = self.world
        t_start, f_start = time.monotonic(), w.frame
        for k in keys:
            w.key_down(c.id, k)
        while time.monotonic() - t_start < hold_s or w.frame - f_start < 2:
            yield
        for k in keys:
            w.key_up(c.id, k)
        base.update({"held_s": _r(time.monotonic() - t_start, 3), "frames": w.frame - f_start})
        base.update(self._input_fields())
        return base

    def _h_axis(self, c, req):
        self._require_viewport()
        values = req.get("values")
        if not isinstance(values, dict) or not values:
            raise ProtocolError("bad_request", "'values' must be a non-empty object")
        parsed = {}
        for name, v in values.items():
            fkey, kind = resolve_key(name)
            if fkey not in AXIS_RANGES:
                raise ProtocolError("bad_key", f"'{name}' is not one of {', '.join(AXIS_RANGES)}")
            lo, hi = AXIS_RANGES[fkey]
            value = float(_num({"v": v}, "v", required=True, lo=lo, hi=hi))
            if fkey in parsed:  # C++ ParseAxisArgs (e.g. "Gamepad_LeftY" and "gamepad_lefty")
                raise ProtocolError("bad_request", f"axis '{fkey}' is listed twice")
            parsed[fkey] = value
        hold_s = _num(req, "hold_s", None, lo=0, hi=30)
        w = self.world
        for k, v in parsed.items():
            w.set_axis(c.id, k, v)
        base = {"values": parsed}
        if hold_s is None:
            base.update(self._input_fields())
            return base
        return self._axis_hold(c, parsed, hold_s, base)

    def _axis_hold(self, c, parsed, hold_s, base):
        w = self.world
        t_start, f_start = time.monotonic(), w.frame
        while time.monotonic() - t_start < hold_s or w.frame - f_start < 1:
            yield
        for k in parsed:
            w.set_axis(c.id, k, 0.0)
        base.update({"hold_s": hold_s, "held_s": _r(time.monotonic() - t_start, 3),
                     "frames": w.frame - f_start})
        base.update(self._input_fields())
        return base

    PIXEL_LIMIT = 100000.0  # C++ ReadPixel: x and y within +-100000; off-viewport hits nothing

    def _point(self, req, required):
        """C++ ReadPixel / ParseClickArgs: null counts as absent; mouse needs both; click reads
        the point once either is given."""
        has_x, has_y = req.get("x") is not None, req.get("y") is not None
        if not (has_x or has_y) and not required:
            return None
        lim = self.PIXEL_LIMIT
        x = _num(req, "x", None, lo=-lim, hi=lim)
        y = _num(req, "y", None, lo=-lim, hi=lim)
        if x is None or y is None:
            raise ProtocolError("bad_request", "both 'x' and 'y' are required (viewport pixels)")
        return x, y

    def _h_mouse(self, c, req):
        x, y = self._point(req, required=True)
        self._require_viewport()
        self.world.move_cursor(x, y)
        resp = {"x": x, "y": y, "abs": [_r(320 + x * 1.5), _r(224 + y * 1.5)]}
        resp.update(self._input_fields())
        return resp

    def _h_click(self, c, req):
        point = self._point(req, required=False)
        button = _str(req, "button", "Left", choices=("Left", "Right", "Middle"))
        hold_s = _num(req, "hold_s", 0.05, lo=0, hi=30)  # FClickCommandArgs::HoldSeconds = 0.05
        self._require_viewport()
        w = self.world
        if point is None:
            point = tuple(w.cursor) if w.cursor else (VIEWPORT_W / 2.0, VIEWPORT_H / 2.0)
        return self._click(c, point, button, hold_s)

    def _click(self, c, point, button, hold_s):
        w = self.world
        x, y = point
        t_start, f_start = time.monotonic(), w.frame
        hit = w.mouse_down(c.id, x, y, button)
        while time.monotonic() - t_start < hold_s or w.frame - f_start < 1:
            yield
        w.mouse_up(c.id, x, y, button)
        resp = {"x": x, "y": y, "button": button, "hit": hit,
                "held_s": _r(time.monotonic() - t_start, 3), "frames": w.frame - f_start}
        resp.update(self._input_fields())
        return resp

    def _h_screenshot(self, c, req):
        self._require_viewport()
        path = req.get("path")
        if path is not None and (not isinstance(path, str) or not path.strip()):
            raise ProtocolError("bad_request", "'path' must be a non-empty string")
        if path is None:
            self._shot_counter += 1
            path = os.path.join(self.shot_dir, f"bridge_{self._shot_counter:02d}.png")
        elif not os.path.splitext(path)[1]:
            path += ".png"
        return self._screenshot(os.path.abspath(path))

    def _screenshot(self, path):
        f = self.faults
        yield  # the next frame renders it
        if f.screenshot == "timeout":
            t_start = time.monotonic()
            while time.monotonic() - t_start < f.screenshot_timeout_s:
                yield
            raise ProtocolError("timeout", f"screenshot not written within "
                                           f"{f.screenshot_timeout_s:g}s: {path}")
        if f.screenshot == "failed":
            raise ProtocolError("failed", f"screenshot file missing after capture: {path}")
        data = make_png()
        os.makedirs(os.path.dirname(path), exist_ok=True)
        if f.screenshot in ("reply_early", "truncated"):
            with open(path, "wb") as fh:
                fh.write(data[:len(data) // 2])
            if f.screenshot == "reply_early":
                def complete(_world, path=path, data=data):
                    with open(path, "wb") as fh:
                        fh.write(data)
                self.world.after(f.screenshot_complete_delay_s, complete)
            return {"path": path, "bytes": len(data)}
        with open(path, "wb") as fh:
            fh.write(data)
        return {"path": path, "bytes": os.path.getsize(path)}

    def _h_wait(self, c, req):
        seconds = _num(req, "seconds", required=True, lo=0, hi=600)
        return self._wait(seconds)

    def _wait(self, seconds):
        t_start = time.monotonic()
        while time.monotonic() - t_start < seconds:
            yield
        return {"seconds": seconds, "elapsed_s": _r(time.monotonic() - t_start, 3)}

    def _h_wait_until(self, c, req):
        text = req.get("condition")
        if not isinstance(text, str) or not text.strip():
            raise ProtocolError("bad_request", "missing 'condition'")
        try:
            cond = Condition(text)
        except ConditionError as e:
            raise ProtocolError("bad_request", f"bad condition: {e}")  # as the C++
        timeout_s = _num(req, "timeout_s", 10, lo=0, hi=600)
        poll_s = _num(req, "poll_s", 0.1, lo=0, hi=600)
        return self._wait_until(cond, timeout_s, poll_s)

    def _wait_until(self, cond, timeout_s, poll_s):
        sections = [r for r in cond.roots if r in SECTIONS]
        t_start = time.monotonic()
        next_eval = t_start
        while True:
            now = time.monotonic()
            elapsed = now - t_start
            if now >= next_eval or elapsed >= timeout_s:
                state = self.world.state(sections) if sections else {}
                ok, values = cond.evaluate(state)
                if ok:
                    return {"elapsed_s": _r(elapsed, 3), "values": values}
                if elapsed >= timeout_s:
                    raise ProtocolError("timeout", f"condition not met within {timeout_s:g}s: "
                                                   f"{cond.text}", values=values,
                                        elapsed_s=_r(elapsed, 3))
                next_eval = now + poll_s
            yield

    def _h_quit(self, c, req):
        self._quitting = True
        self._quit_conn = c
        return {}


# ---------------------------------------------------------------------------
# Standalone
# ---------------------------------------------------------------------------
def _load_json(path):
    if not path:
        return None
    with open(path, "r", encoding="utf-8-sig") as f:
        return json.load(f)


def main(argv=None):
    parser = argparse.ArgumentParser(description="Offline fake of the in-game agent bridge.")
    parser.add_argument("--port", type=int, default=0, help="TCP port (default: a free one)")
    parser.add_argument("--token", default="", help="token echoed by ping")
    parser.add_argument("--shot-dir", default=None, help="default screenshot directory")
    parser.add_argument("--world-json", default=None, help="JSON file of FakeWorld options")
    parser.add_argument("--faults-json", default=None, help="JSON file of Faults options")
    args = parser.parse_args(argv)
    world = FakeWorld.from_options(_load_json(args.world_json))
    server = FakeBridgeServer(world, port=args.port, token=args.token, shot_dir=args.shot_dir,
                              faults=Faults.from_options(_load_json(args.faults_json)))
    try:
        server.start()
    except OSError as e:
        print(f"[Fake-Bridge] failed to listen on 127.0.0.1:{args.port}: {e}", file=sys.stderr)
        return 2
    print(f"[Fake-Bridge] listening on 127.0.0.1:{server.port}", flush=True)
    try:
        while not server.stopped.wait(0.25):
            pass
    except KeyboardInterrupt:
        server.stop()
    return 0


if __name__ == "__main__":
    sys.exit(main())
