#!/usr/bin/env python3
"""PDCM KiCad 9 Schematic Generator.

Generates a complete hierarchical schematic for the PDCM (Power Distribution
& Control Module) with 46 TC4427A-driven output channels + 1 DRV8876 H-bridge.

Usage: /c/Python314/python generate_schematic.py
"""

import uuid
import json
import os
import re
import math
from pathlib import Path

# ============================================================
# Constants
# ============================================================
KICAD_LIB_DIR = r"C:\Program Files\KiCad\9.0\share\kicad\symbols"
OUTPUT_DIR = os.path.dirname(os.path.abspath(__file__))
PROJECT_NAME = "PDCM"
KICAD_VERSION = 20250114
GRID = 2.54
FINE_GRID = 1.27

ROOT_UUID = "c0000000-0000-4000-8000-000000000001"
SHEET_UUIDS = {
    "power_input":    "d0000000-0000-4000-8000-000000000001",
    "mcu":            "d0000000-0000-4000-8000-000000000002",
    "can_bus":        "d0000000-0000-4000-8000-000000000003",
    "gate_drivers_a": "d0000000-0000-4000-8000-000000000004",
    "gate_drivers_b": "d0000000-0000-4000-8000-000000000005",
    "output_stage_a": "d0000000-0000-4000-8000-000000000006",
    "output_stage_b": "d0000000-0000-4000-8000-000000000007",
    "hbridge":        "d0000000-0000-4000-8000-000000000008",
    "switch_inputs":  "d0000000-0000-4000-8000-000000000009",
    "connectors":     "d0000000-0000-4000-8000-00000000000a",
}

_ref_counters = {}
_symbol_cache = {}
_pin_cache = {}
_unit_pin_cache = {}


# ============================================================
# Helpers
# ============================================================
def g(val):
    return round(round(val / FINE_GRID) * FINE_GRID, 4)

def new_uuid():
    return str(uuid.uuid4())

def next_ref(prefix):
    _ref_counters[prefix] = _ref_counters.get(prefix, 0) + 1
    return f"{prefix}{_ref_counters[prefix]}"

def reset_refs():
    _ref_counters.clear()

def pin_pos(inst_x, inst_y, angle_deg, rel_x, rel_y):
    """Transform symbol-space pin (rel_x, rel_y) to schematic coordinates.
    KiCad symbol space is Y-up, schematic is Y-down, rotation is CW in schematic."""
    a = math.radians(angle_deg)
    ax = inst_x + rel_x * math.cos(a) - rel_y * math.sin(a)
    ay = inst_y - rel_x * math.sin(a) - rel_y * math.cos(a)
    return (g(ax), g(ay))


# ============================================================
# S-Expression Parser
# ============================================================
def tokenize_sexpr(text):
    tokens = []
    i, n = 0, len(text)
    while i < n:
        c = text[i]
        if c in '()':
            tokens.append(c); i += 1
        elif c == '"':
            j = i + 1
            while j < n and text[j] != '"':
                if text[j] == '\\': j += 1
                j += 1
            tokens.append(text[i:j+1]); i = j + 1
        elif c in ' \t\n\r':
            i += 1
        else:
            j = i
            while j < n and text[j] not in '() \t\n\r"': j += 1
            tokens.append(text[i:j]); i = j
    return tokens

def parse_sexpr(text):
    tokens = tokenize_sexpr(text)
    result, _ = _parse_tokens(tokens, 0)
    return result

def _parse_tokens(tokens, pos):
    if tokens[pos] == '(':
        lst = []
        pos += 1
        while pos < len(tokens) and tokens[pos] != ')':
            item, pos = _parse_tokens(tokens, pos)
            lst.append(item)
        return lst, pos + 1
    else:
        return tokens[pos], pos + 1


# ============================================================
# Symbol Extraction from KiCad Libraries
# ============================================================
def _find_symbol_in_file(lib_file, symbol_name):
    path = os.path.join(KICAD_LIB_DIR, lib_file)
    with open(path, 'r', encoding='utf-8') as f:
        content = f.read()
    target = f'(symbol "{symbol_name}"'
    idx = content.find(target)
    if idx == -1:
        raise ValueError(f"Symbol '{symbol_name}' not found in {lib_file}")
    while idx != -1:
        end_quote = idx + len(target)
        if end_quote < len(content) and content[end_quote] in '\n\r\t ':
            break
        idx = content.find(target, idx + 1)
    if idx == -1:
        raise ValueError(f"Symbol '{symbol_name}' not found at top level in {lib_file}")
    depth, i = 0, idx
    while i < len(content):
        if content[i] == '(': depth += 1
        elif content[i] == ')':
            depth -= 1
            if depth == 0: return content[idx:i+1]
        i += 1
    raise ValueError(f"Unmatched parens for symbol '{symbol_name}'")

def _resolve_extends(lib_file, child_name, child_text):
    m = re.search(r'\(extends "([^"]+)"\)', child_text)
    if not m: return child_text
    parent_name = m.group(1)
    parent_text = _find_symbol_in_file(lib_file, parent_name)
    if '(extends ' in parent_text:
        parent_text = _resolve_extends(lib_file, parent_name, parent_text)
    child_props = {}
    for pm in re.finditer(r'\(property "([^"]+)" "([^"]*)"', child_text):
        child_props[pm.group(1)] = pm.group(2)
    result = parent_text
    result = result.replace(f'(symbol "{parent_name}"', f'(symbol "{child_name}"', 1)
    result = result.replace(f'"{parent_name}_', f'"{child_name}_')
    for prop_key, prop_val in child_props.items():
        old_prop = f'(property "{prop_key}" "'
        idx = result.find(old_prop)
        if idx != -1:
            val_start = idx + len(old_prop)
            val_end = result.find('"', val_start)
            if val_end != -1:
                result = result[:val_start] + prop_val + result[val_end:]
    return result

def extract_symbol_text(lib_file, symbol_name):
    cache_key = f"{lib_file}:{symbol_name}"
    if cache_key in _symbol_cache: return _symbol_cache[cache_key]
    text = _find_symbol_in_file(lib_file, symbol_name)
    if '(extends ' in text:
        text = _resolve_extends(lib_file, symbol_name, text)
    _symbol_cache[cache_key] = text
    return text

def extract_pins(lib_file, symbol_name):
    cache_key = f"{lib_file}:{symbol_name}"
    if cache_key in _pin_cache: return _pin_cache[cache_key]
    text = extract_symbol_text(lib_file, symbol_name)
    parsed = parse_sexpr(text)
    pins = _find_pins_recursive(parsed)
    _pin_cache[cache_key] = pins
    return pins

def extract_unit_pins(lib_file, symbol_name):
    cache_key = f"{lib_file}:{symbol_name}"
    if cache_key in _unit_pin_cache: return _unit_pin_cache[cache_key]
    text = extract_symbol_text(lib_file, symbol_name)
    unit_pins = {}
    lines = text.split('\n')
    current_unit = None
    for line in lines:
        m = re.search(rf'\(symbol\s+"{re.escape(symbol_name)}_(\d+)_(\d+)"', line)
        if m:
            current_unit = int(m.group(1))
            if current_unit not in unit_pins: unit_pins[current_unit] = set()
        m2 = re.search(r'\(number\s+"([^"]+)"', line)
        if m2 and current_unit is not None:
            unit_pins[current_unit].add(m2.group(1))
    if not unit_pins:
        all_pins = extract_pins(lib_file, symbol_name)
        unit_pins[1] = set(all_pins.keys())
    _unit_pin_cache[cache_key] = unit_pins
    return unit_pins

def get_unit_pins(lib_id, unit):
    if lib_id in SYMBOL_MAP:
        lib_file, sym_name = SYMBOL_MAP[lib_id]
        umap = extract_unit_pins(lib_file, sym_name)
        return umap.get(unit)
    return None

def _find_pins_recursive(node):
    pins = {}
    if not isinstance(node, list) or len(node) == 0: return pins
    if node[0] == 'pin':
        at_x, at_y, num = None, None, None
        for item in node:
            if isinstance(item, list) and len(item) >= 3 and item[0] == 'at':
                at_x, at_y = float(item[1]), float(item[2])
            elif isinstance(item, list) and len(item) >= 2 and item[0] == 'number':
                num = item[1].strip('"')
        if num is not None and at_x is not None:
            pins[num] = (at_x, at_y)
    else:
        for item in node:
            if isinstance(item, list): pins.update(_find_pins_recursive(item))
    return pins

def make_lib_symbol(lib_name, symbol_name, lib_file):
    text = extract_symbol_text(lib_file, symbol_name)
    old = f'(symbol "{symbol_name}"'
    new = f'(symbol "{lib_name}:{symbol_name}"'
    return text.replace(old, new, 1)


# ============================================================
# Symbol -> Library File Mapping
# ============================================================
SYMBOL_MAP = {
    "Device:R": ("Device.kicad_sym", "R"),
    "Device:C": ("Device.kicad_sym", "C"),
    "Device:C_Polarized": ("Device.kicad_sym", "C_Polarized"),
    "Device:D": ("Device.kicad_sym", "D"),
    "Device:D_TVS": ("Device.kicad_sym", "D_TVS"),
    "Device:D_Zener": ("Device.kicad_sym", "D_Zener"),
    "Device:D_Schottky": ("Device.kicad_sym", "D_Schottky"),
    "Device:LED": ("Device.kicad_sym", "LED"),
    "Device:L": ("Device.kicad_sym", "L"),
    "Device:Fuse": ("Device.kicad_sym", "Fuse"),
    "Device:Crystal": ("Device.kicad_sym", "Crystal"),
    "Transistor_FET:IRLZ44N": ("Transistor_FET.kicad_sym", "IRLZ44N"),
    "Transistor_FET:IRF9540N": ("Transistor_FET.kicad_sym", "IRF9540N"),
    "Interface_CAN_LIN:MCP2562-E-SN": ("Interface_CAN_LIN.kicad_sym", "MCP2562-E-SN"),
    "Driver_FET:TC4427xOA": ("Driver_FET.kicad_sym", "TC4427xOA"),
    "Regulator_Switching:LM2596S-5": ("Regulator_Switching.kicad_sym", "LM2596S-5"),
    "Regulator_Linear:AMS1117-3.3": ("Regulator_Linear.kicad_sym", "AMS1117-3.3"),
    "Amplifier_Current:INA180A1": ("Amplifier_Current.kicad_sym", "INA180A1"),
    "Connector_Generic:Conn_01x02": ("Connector_Generic.kicad_sym", "Conn_01x02"),
    "Connector_Generic:Conn_01x03": ("Connector_Generic.kicad_sym", "Conn_01x03"),
    "Connector_Generic:Conn_01x04": ("Connector_Generic.kicad_sym", "Conn_01x04"),
    "Connector_Generic:Conn_01x05": ("Connector_Generic.kicad_sym", "Conn_01x05"),
    "Connector_Generic:Conn_01x06": ("Connector_Generic.kicad_sym", "Conn_01x06"),
    "Connector_Generic:Conn_01x08": ("Connector_Generic.kicad_sym", "Conn_01x08"),
    "Connector_Generic:Conn_01x10": ("Connector_Generic.kicad_sym", "Conn_01x10"),
    "Connector_Generic:Conn_01x12": ("Connector_Generic.kicad_sym", "Conn_01x12"),
    "Connector_Generic:Conn_01x16": ("Connector_Generic.kicad_sym", "Conn_01x16"),
    "Connector_Generic:Conn_02x05_Odd_Even": ("Connector_Generic.kicad_sym", "Conn_02x05_Odd_Even"),
}

POWER_SYMBOLS = {
    "power:+5V": ("power.kicad_sym", "+5V"),
    "power:+12V": ("power.kicad_sym", "+12V"),
    "power:+3.3V": ("power.kicad_sym", "+3.3V"),
    "power:GND": ("power.kicad_sym", "GND"),
    "power:PWR_FLAG": ("power.kicad_sym", "PWR_FLAG"),
}

TWO_PIN = {"1": (0, 3.81), "2": (0, -3.81)}

def get_pins(lib_id):
    if lib_id in SYMBOL_MAP:
        lib_file, sym_name = SYMBOL_MAP[lib_id]
        return extract_pins(lib_file, sym_name)
    elif lib_id in POWER_SYMBOLS:
        return {"1": (0, 0)}
    elif lib_id.startswith("PDCM:"):
        return CUSTOM_PINS.get(lib_id, {})
    return {}


# ============================================================
# KiCad S-Expression Element Builders
# ============================================================
def sch_header(sheet_uuid, paper="A3"):
    return f"""(kicad_sch
\t(version {KICAD_VERSION})
\t(generator "pdcm_generator")
\t(generator_version "1.0")
\t(uuid "{sheet_uuid}")
\t(paper "{paper}")
"""

def sch_footer():
    return "\t(embedded_fonts no)\n)\n"

def lib_symbols_section(lib_ids):
    lines = ["\t(lib_symbols"]
    seen = set()
    for lib_id in lib_ids:
        if lib_id in seen: continue
        seen.add(lib_id)
        if lib_id in SYMBOL_MAP:
            lib_name = lib_id.split(":")[0]
            lib_file, sym_name = SYMBOL_MAP[lib_id]
            sym_text = make_lib_symbol(lib_name, sym_name, lib_file)
        elif lib_id in POWER_SYMBOLS:
            lib_file, sym_name = POWER_SYMBOLS[lib_id]
            sym_text = make_lib_symbol("power", sym_name, lib_file)
        elif lib_id.startswith("PDCM:"):
            sym_name = lib_id.split(":")[1]
            sym_text = get_custom_symbol_text(sym_name)
        else:
            continue
        indented = "\n".join("\t\t" + l for l in sym_text.strip().split("\n"))
        lines.append(indented)
    lines.append("\t)")
    return "\n".join(lines) + "\n"

def symbol_instance(lib_id, ref, value, x, y, angle=0, unit=1,
                    sheet_path=None, pin_count=0, dnp=False, extra_props=None):
    uid = new_uuid()
    lines = []
    lines.append(f'\t(symbol')
    lines.append(f'\t\t(lib_id "{lib_id}")')
    lines.append(f'\t\t(at {g(x)} {g(y)} {angle})')
    lines.append(f'\t\t(unit {unit})')
    lines.append(f'\t\t(exclude_from_sim no)')
    lines.append(f'\t\t(in_bom yes)')
    lines.append(f'\t\t(on_board yes)')
    lines.append(f'\t\t(dnp {"yes" if dnp else "no"})')
    lines.append(f'\t\t(uuid "{uid}")')
    is_power = lib_id.startswith("power:")
    ref_hide = ' (hide yes)' if is_power else ''
    lines.append(f'\t\t(property "Reference" "{ref}"')
    lines.append(f'\t\t\t(at {g(x+2)} {g(y)} 0)')
    lines.append(f'\t\t\t(effects (font (size 1.27 1.27)){ref_hide}))')
    val_hide = ' (hide yes)' if is_power else ''
    lines.append(f'\t\t(property "Value" "{value}"')
    lines.append(f'\t\t\t(at {g(x)} {g(y+2)} 0)')
    lines.append(f'\t\t\t(effects (font (size 1.27 1.27)){val_hide}))')
    lines.append(f'\t\t(property "Footprint" ""')
    lines.append(f'\t\t\t(at {x} {y} 0)')
    lines.append(f'\t\t\t(effects (font (size 1.27 1.27)) (hide yes)))')
    lines.append(f'\t\t(property "Datasheet" "~"')
    lines.append(f'\t\t\t(at {x} {y} 0)')
    lines.append(f'\t\t\t(effects (font (size 1.27 1.27)) (hide yes)))')
    lines.append(f'\t\t(property "Description" ""')
    lines.append(f'\t\t\t(at {x} {y} 0)')
    lines.append(f'\t\t\t(effects (font (size 1.27 1.27)) (hide yes)))')
    if extra_props:
        for k, v in extra_props.items():
            lines.append(f'\t\t(property "{k}" "{v}"')
            lines.append(f'\t\t\t(at {x} {y} 0)')
            lines.append(f'\t\t\t(effects (font (size 1.27 1.27)) (hide yes)))')
    pins = get_pins(lib_id)
    unit_pin_set = get_unit_pins(lib_id, unit)
    if pins:
        for pnum in sorted(pins.keys(), key=lambda x: (len(x), x)):
            if unit_pin_set is not None and pnum not in unit_pin_set: continue
            lines.append(f'\t\t(pin "{pnum}" (uuid "{new_uuid()}"))')
    elif pin_count > 0:
        for i in range(1, pin_count + 1):
            lines.append(f'\t\t(pin "{i}" (uuid "{new_uuid()}"))')
    if sheet_path is None: sheet_path = f"/{ROOT_UUID}"
    lines.append(f'\t\t(instances')
    lines.append(f'\t\t\t(project "{PROJECT_NAME}"')
    lines.append(f'\t\t\t\t(path "{sheet_path}"')
    lines.append(f'\t\t\t\t\t(reference "{ref}")')
    lines.append(f'\t\t\t\t\t(unit {unit}))')
    lines.append(f'\t\t\t))')
    lines.append(f'\t)')
    return "\n".join(lines) + "\n"

def wire(x1, y1, x2, y2):
    return (f'\t(wire\n\t\t(pts (xy {g(x1)} {g(y1)}) (xy {g(x2)} {g(y2)}))\n'
            f'\t\t(stroke (width 0) (type default))\n'
            f'\t\t(uuid "{new_uuid()}")\n\t)\n')

def junction(x, y):
    return (f'\t(junction (at {g(x)} {g(y)}) (diameter 0) (color 0 0 0 0)\n'
            f'\t\t(uuid "{new_uuid()}")\n\t)\n')

def no_connect(x, y):
    return f'\t(no_connect (at {g(x)} {g(y)}) (uuid "{new_uuid()}"))\n'

def global_label(name, x, y, angle=0, shape="bidirectional"):
    return (f'\t(global_label "{name}"\n'
            f'\t\t(shape {shape})\n'
            f'\t\t(at {g(x)} {g(y)} {angle})\n'
            f'\t\t(effects (font (size 1.27 1.27)))\n'
            f'\t\t(uuid "{new_uuid()}")\n'
            f'\t\t(property "Intersheetrefs" "${{INTERSHEET_REFS}}"\n'
            f'\t\t\t(at 0 0 0)\n'
            f'\t\t\t(effects (font (size 1.27 1.27)) (hide yes))\n'
            f'\t\t)\n\t)\n')

def local_label(name, x, y, angle=0):
    return (f'\t(label "{name}" (at {g(x)} {g(y)} {angle})\n'
            f'\t\t(effects (font (size 1.27 1.27)))\n'
            f'\t\t(uuid "{new_uuid()}")\n\t)\n')

def text_note(text, x, y):
    return (f'\t(text "{text}" (at {x} {y} 0)\n'
            f'\t\t(effects (font (size 2 2)))\n'
            f'\t\t(uuid "{new_uuid()}")\n\t)\n')


# ============================================================
# Comp & SheetBuilder Classes
# ============================================================
class Comp:
    def __init__(self, lib_id, ref, value, x, y, angle=0, unit=1,
                 sheet_path=None, dnp=False, extra_props=None):
        self.lib_id = lib_id
        self.ref = ref
        self.value = value
        self.x, self.y = g(x), g(y)
        self.angle = angle
        self.unit = unit
        self.sheet_path = sheet_path
        self.dnp = dnp
        self.extra_props = extra_props
        self._pins = get_pins(lib_id)

    def pin(self, num):
        num = str(num)
        if num in self._pins:
            rx, ry = self._pins[num]
            return pin_pos(self.x, self.y, self.angle, rx, ry)
        raise ValueError(f"Pin {num} not found on {self.ref} ({self.lib_id})")

    def sexpr(self):
        return symbol_instance(
            self.lib_id, self.ref, self.value,
            self.x, self.y, self.angle, self.unit,
            self.sheet_path, len(self._pins) if self._pins else 0,
            self.dnp, self.extra_props)


class SheetBuilder:
    def __init__(self, sheet_name, sheet_uuid=None, paper="A3"):
        self.sheet_name = sheet_name
        self.sheet_uuid = sheet_uuid or new_uuid()
        self.paper = paper
        self.components = []
        self.wires = []
        self.labels = []
        self.junctions = []
        self.no_connects = []
        self.notes = []
        self._lib_ids = set()
        if sheet_name == "root":
            self.sheet_path = f"/{ROOT_UUID}"
        else:
            self.sheet_path = f"/{ROOT_UUID}/{SHEET_UUIDS.get(sheet_name, self.sheet_uuid)}"

    def add(self, comp):
        if isinstance(comp, Comp):
            comp.sheet_path = self.sheet_path
            self.components.append(comp)
            self._lib_ids.add(comp.lib_id)
            return comp
        return None

    def place(self, lib_id, ref, value, x, y, angle=0, unit=1, dnp=False, extra_props=None):
        c = Comp(lib_id, ref, value, x, y, angle, unit, self.sheet_path, dnp, extra_props)
        return self.add(c)

    def R(self, ref, value, x, y, angle=0):
        return self.place("Device:R", ref, value, x, y, angle)
    def C(self, ref, value, x, y, angle=0):
        return self.place("Device:C", ref, value, x, y, angle)
    def CP(self, ref, value, x, y, angle=0):
        return self.place("Device:C_Polarized", ref, value, x, y, angle)
    def D(self, ref, value, x, y, angle=0):
        return self.place("Device:D", ref, value, x, y, angle)

    def wire(self, x1, y1, x2, y2):
        self.wires.append(wire(x1, y1, x2, y2))
    def wire_pin(self, comp, pin_num, x2, y2):
        px, py = comp.pin(pin_num)
        self.wires.append(wire(px, py, x2, y2))
    def junction(self, x, y):
        self.junctions.append(junction(x, y))
    def no_connect(self, x, y):
        self.no_connects.append(no_connect(x, y))
    def glabel(self, name, x, y, angle=0, shape="bidirectional"):
        self.labels.append(global_label(name, x, y, angle, shape))
    def llabel(self, name, x, y, angle=0):
        self.labels.append(local_label(name, x, y, angle))
    def note(self, text, x, y):
        self.notes.append(text_note(text, x, y))

    def pwr(self, name, x, y, angle=0):
        lib_id = f"power:{name}"
        ref = next_ref("#PWR")
        return self.place(lib_id, ref, name, x, y, angle)
    def gnd(self, x, y): return self.pwr("GND", x, y)
    def v5(self, x, y): return self.pwr("+5V", x, y)
    def v12(self, x, y): return self.pwr("+12V", x, y)
    def v33(self, x, y): return self.pwr("+3.3V", x, y)

    def build(self):
        parts = [sch_header(self.sheet_uuid, self.paper),
                 lib_symbols_section(self._lib_ids)]
        for item in self.junctions: parts.append(item)
        for item in self.no_connects: parts.append(item)
        for item in self.wires: parts.append(item)
        for item in self.labels: parts.append(item)
        for item in self.notes: parts.append(item)
        for comp in self.components: parts.append(comp.sexpr())
        parts.append(sch_footer())
        return "".join(parts)


# ============================================================
# PDCM Channel Definitions
# ============================================================
# (ch_num, name, tier, shunt_mohm, pwm_capable)
# ch 24 is H-bridge (DRV8876), not in this list
CHANNEL_DEFS = [
    # ADR-010: Universal output circuit — two shunt sizes only
    # 10mΩ × 36 channels (heavy, 1–16A) + 50mΩ × 10 channels (light, 0.3–3.3A)
    (0,  "FUEL_PUMP",      1, 10,  True),
    (1,  "FAN_1",          1, 10,  True),
    (2,  "FAN_2",          1, 10,  True),
    (3,  "BLOWER",         1, 10,  True),
    (4,  "AC_CLUTCH",      1, 10,  False),
    (5,  "LOW_BEAM_L",     1, 10,  True),
    (6,  "LOW_BEAM_R",     1, 10,  True),
    (7,  "HIGH_BEAM_L",    1, 10,  True),
    (8,  "HIGH_BEAM_R",    1, 10,  True),
    (9,  "TURN_L",         1, 10,  False),
    (10, "TURN_R",         1, 10,  False),
    (11, "BRAKE_L",        1, 10,  False),
    (12, "BRAKE_R",        1, 10,  False),
    (13, "REVERSE",        1, 10,  False),
    (14, "DRL",            1, 10,  True),
    (15, "INTERIOR",       1, 10,  True),
    (16, "COURTESY",       1, 10,  True),
    (17, "HORN",           1, 10,  False),
    (18, "WIPER",          1, 10,  False),
    (19, "ACCESSORY",      1, 10,  False),
    (20, "FRONT_AXLE",     1, 10,  False),
    (21, "SEAT_HTR_L",     1, 10,  True),
    (22, "SEAT_HTR_R",     1, 10,  True),
    (23, "LIGHT_BAR",      1, 10,  False),
    # ch 24 = H-bridge, skip
    (25, "AMP_REMOTE_1",   2, 50,  False),
    (26, "AMP_REMOTE_2",   2, 50,  False),
    (27, "HEADUNIT_EN",    2, 50,  False),
    (28, "CAM_FRONT",      3, 10,  False),
    (29, "CAM_REAR",       3, 10,  False),
    (30, "CAM_SIDE",       3, 10,  False),
    (31, "PARK_SENS",      3, 10,  False),
    (32, "RADAR_BSM",      3, 10,  False),
    (33, "GCM_POWER",      3, 10,  False),
    (34, "GPS_CELL",       3, 10,  False),
    (35, "DASH_CAM",       3, 10,  False),
    (36, "FUTURE_MOD",     3, 10,  False),
    (37, "ROCK_LIGHTS",    3, 10,  False),
    (38, "BED_LIGHTS",     3, 10,  False),
    (39, "PUDDLE_LIGHTS",  3, 10,  False),
    (40, "FUTURE_EXT",     3, 50,  False),
    (41, "EXPANSION_1",    3, 50,  False),
    (42, "EXPANSION_2",    3, 50,  False),
    (43, "EXPANSION_3",    3, 50,  False),
    (44, "EXPANSION_4",    3, 50,  False),
    (45, "EXPANSION_5",    3, 50,  False),
    (46, "EXPANSION_6",    3, 50,  False),
]

# TC4427A channel pairs: list of (ic_num, ch_a, ch_b)
TC4427A_PAIRS = []
tc_channels = [cd[0] for cd in CHANNEL_DEFS]
for i in range(0, len(tc_channels), 2):
    ic_num = i // 2 + 1
    ch_a = tc_channels[i]
    ch_b = tc_channels[i + 1] if i + 1 < len(tc_channels) else None
    TC4427A_PAIRS.append((ic_num, ch_a, ch_b))

SWITCH_DEFS = [
    "TURN_L", "TURN_R", "HIGH_BEAM", "FLASH_PASS",
    "HAZARD", "HORN", "REVERSE", "AC_REQ",
    "WIPER_INT", "WIPER_LO", "WIPER_HI", "WASHER",
    "BRAKE_SW1", "BRAKE_SW2", "START_BTN",
]

# Channel name lookup
CH_NAME = {cd[0]: cd[1] for cd in CHANNEL_DEFS}
CH_SHUNT = {cd[0]: cd[3] for cd in CHANNEL_DEFS}


# ============================================================
# Custom Symbol Pin Definitions
# ============================================================
CUSTOM_PINS = {}

def _s32k358_pins():
    """S32K358 172-pin HDQFP symbol. Pins grouped by function.
    Left: 46 gate driver outputs + 4 H-bridge + 15 switches = 65 pins
    Right: 46 current sense ADC + 3 sensor ADC = 49 pins
    Top: VDD, CAN, crystal, reset, JTAG = 15 pins
    Bottom: GND pins = 5 pins
    Remaining: additional power/ground (38 pins)
    """
    pins = {}
    # Left side: output GPIOs (pin 1-46 skip ch 24 for H-bridge)
    left_x = -35.56
    for i in range(46):
        pin_num = i + 1  # pins 1-46
        y = -82.55 + i * 2.54
        pins[str(pin_num)] = (left_x, y)

    # Left side continued: H-bridge control (pins 47-50)
    for i, name in enumerate(["47", "48", "49", "50"]):
        y = -82.55 + (46 + i) * 2.54
        pins[name] = (left_x, y)

    # Left side continued: switch inputs (pins 51-65)
    for i in range(15):
        pin_num = 51 + i
        y = -82.55 + (50 + i) * 2.54
        pins[str(pin_num)] = (left_x, y)

    # Right side: current sense ADC (pins 66-111)
    right_x = 35.56
    for i in range(46):
        pin_num = 66 + i
        y = -82.55 + i * 2.54
        pins[str(pin_num)] = (right_x, y)

    # Right side: sensor ADC (pins 112-114) — BATT_ADC, 4WD_ADC, HB_CS_ADC
    for i in range(3):
        pin_num = 112 + i
        y = -82.55 + (46 + i) * 2.54
        pins[str(pin_num)] = (right_x, y)

    # Top: power + CAN + crystal + reset + JTAG (pins 115-129)
    top_y = -87.63
    for i in range(15):
        pin_num = 115 + i
        x = -17.78 + i * 2.54
        pins[str(pin_num)] = (x, top_y)

    # Bottom: ground (pins 130-134)
    bot_y = 87.63
    for i in range(5):
        pin_num = 130 + i
        x = -5.08 + i * 2.54
        pins[str(pin_num)] = (x, bot_y)

    # Remaining pins 135-172: additional power/ground on separate row below bottom
    # Offset by 5.08mm to avoid overlapping with bottom GND pins (130-134)
    extra_y = bot_y + 5.08
    for i in range(38):
        pin_num = 135 + i
        x = -45.72 + i * 2.54
        pins[str(pin_num)] = (x, extra_y)

    return pins

CUSTOM_PINS["PDCM:S32K358"] = _s32k358_pins()

def _drv8876_pins():
    """DRV8876 8-pin WSON. Pinout:
    1=VM, 2=GND(PAD), 3=IN1, 4=IN2, 5=nSLEEP, 6=IPROPI, 7=nFAULT, 8=OUT1/2
    Using simplified 8-pin arrangement."""
    return {
        "1": (-12.7, -5.08),    # VM (motor supply)
        "2": (0, 10.16),        # GND/PAD
        "3": (-12.7, 0),        # IN1
        "4": (-12.7, 5.08),     # IN2
        "5": (-12.7, -10.16),   # nSLEEP (active low = sleep)
        "6": (12.7, 5.08),      # IPROPI (current sense)
        "7": (12.7, 0),         # nFAULT
        "8": (12.7, -5.08),     # OUT (motor terminal)
    }

CUSTOM_PINS["PDCM:DRV8876"] = _drv8876_pins()


# ============================================================
# Custom Symbol Generators
# ============================================================
def get_custom_symbol_text(sym_name):
    if sym_name == "S32K358": return _gen_s32k358_symbol()
    elif sym_name == "DRV8876": return _gen_drv8876_symbol()
    return ""

def _gen_sym_property(prop, val, hide=False):
    h = ' (hide yes)' if hide else ''
    return (f'\t(property "{prop}" "{val}"\n'
            f'\t\t(at 0 0 0)\n'
            f'\t\t(effects (font (size 1.27 1.27)){h})\n\t)')

def _gen_s32k358_symbol():
    """S32K358 172-pin MCU symbol."""
    pins_data = CUSTOM_PINS["PDCM:S32K358"]
    lines = ['(symbol "PDCM:S32K358"',
             '\t(exclude_from_sim no)', '\t(in_bom yes)', '\t(on_board yes)']
    lines.append(_gen_sym_property("Reference", "U"))
    lines.append(_gen_sym_property("Value", "S32K358"))
    lines.append(_gen_sym_property("Footprint", "", True))
    lines.append(_gen_sym_property("Datasheet", "~", True))
    lines.append(_gen_sym_property("Description",
                 "NXP S32K358 Dual CM7 MCU, AEC-Q100, HDQFP-172", True))

    # Body rectangle
    lines.append('\t(symbol "S32K358_0_1"')
    lines.append('\t\t(rectangle')
    lines.append('\t\t\t(start -33.02 -85.09)')
    lines.append('\t\t\t(end 33.02 90.17)')
    lines.append('\t\t\t(stroke (width 0.254) (type default))')
    lines.append('\t\t\t(fill (type background))')
    lines.append('\t\t)')
    lines.append('\t)')

    # Pins
    lines.append('\t(symbol "S32K358_1_1"')

    # Output GPIO pins 1-46 (left side, pointing right → angle 0)
    for i in range(46):
        pn = str(i + 1)
        px, py = pins_data[pn]
        ch = i if i < 24 else i + 1  # skip ch 24 (H-bridge)
        if i < 24:
            pname = f"OUT_{i}"
        else:
            pname = f"OUT_{i+1}"
        lines.append(f'\t\t(pin bidirectional line (at {px} {py} 0) (length 2.54)')
        lines.append(f'\t\t\t(name "{pname}" (effects (font (size 1.016 1.016))))')
        lines.append(f'\t\t\t(number "{pn}" (effects (font (size 1.016 1.016)))))')

    # H-bridge pins 47-50
    hb_names = ["HB_IN1", "HB_IN2", "HB_NSLEEP", "HB_NFAULT"]
    for i, name in enumerate(hb_names):
        pn = str(47 + i)
        px, py = pins_data[pn]
        ptype = "input" if name == "HB_NFAULT" else "output"
        lines.append(f'\t\t(pin bidirectional line (at {px} {py} 0) (length 2.54)')
        lines.append(f'\t\t\t(name "{name}" (effects (font (size 1.016 1.016))))')
        lines.append(f'\t\t\t(number "{pn}" (effects (font (size 1.016 1.016)))))')

    # Switch input pins 51-65
    for i, sw_name in enumerate(SWITCH_DEFS):
        pn = str(51 + i)
        px, py = pins_data[pn]
        lines.append(f'\t\t(pin bidirectional line (at {px} {py} 0) (length 2.54)')
        lines.append(f'\t\t\t(name "SW_{sw_name}" (effects (font (size 1.016 1.016))))')
        lines.append(f'\t\t\t(number "{pn}" (effects (font (size 1.016 1.016)))))')

    # ADC pins 66-111 (right side, pointing left → angle 180)
    for i in range(46):
        pn = str(66 + i)
        px, py = pins_data[pn]
        ch = i if i < 24 else i + 1
        if i < 24:
            pname = f"CSENSE_{i}"
        else:
            pname = f"CSENSE_{i+1}"
        lines.append(f'\t\t(pin bidirectional line (at {px} {py} 180) (length 2.54)')
        lines.append(f'\t\t\t(name "{pname}" (effects (font (size 1.016 1.016))))')
        lines.append(f'\t\t\t(number "{pn}" (effects (font (size 1.016 1.016)))))')

    # Sensor ADC pins 112-114
    sensor_names = ["BATT_ADC", "4WD_ADC", "HB_CS_ADC"]
    for i, name in enumerate(sensor_names):
        pn = str(112 + i)
        px, py = pins_data[pn]
        lines.append(f'\t\t(pin bidirectional line (at {px} {py} 180) (length 2.54)')
        lines.append(f'\t\t\t(name "{name}" (effects (font (size 1.016 1.016))))')
        lines.append(f'\t\t\t(number "{pn}" (effects (font (size 1.016 1.016)))))')

    # Top pins 115-129 (power, CAN, crystal, JTAG — angle 270 = pointing down)
    top_names = ["VDD_HV_1", "VDD_HV_2", "VDD_HV_3", "VDD_HV_4", "VDDA",
                 "CAN_TX", "CAN_RX", "XTAL_IN", "XTAL_OUT", "nRESET",
                 "JTAG_TCK", "JTAG_TMS", "JTAG_TDI", "JTAG_TDO", "JTAG_TRST"]
    top_types = ["power_in"]*5 + ["bidirectional"]*2 + ["passive"]*2 + ["bidirectional"] + ["bidirectional"]*5
    for i, (name, ptype) in enumerate(zip(top_names, top_types)):
        pn = str(115 + i)
        px, py = pins_data[pn]
        lines.append(f'\t\t(pin {ptype} line (at {px} {py} 270) (length 2.54)')
        lines.append(f'\t\t\t(name "{name}" (effects (font (size 1.016 1.016))))')
        lines.append(f'\t\t\t(number "{pn}" (effects (font (size 1.016 1.016)))))')

    # Bottom pins 130-134 (ground — angle 90 = pointing up)
    gnd_names = ["VSS_1", "VSS_2", "VSS_3", "VSS_4", "VSSA"]
    for i, name in enumerate(gnd_names):
        pn = str(130 + i)
        px, py = pins_data[pn]
        lines.append(f'\t\t(pin power_in line (at {px} {py} 90) (length 2.54)')
        lines.append(f'\t\t\t(name "{name}" (effects (font (size 1.016 1.016))))')
        lines.append(f'\t\t\t(number "{pn}" (effects (font (size 1.016 1.016)))))')

    # Extra power/ground pins 135-172
    for i in range(38):
        pn = str(135 + i)
        px, py = pins_data[pn]
        name = f"VSS_{6+i}" if i % 2 == 0 else f"VDD_{i//2+1}"
        ptype = "power_in"
        lines.append(f'\t\t(pin {ptype} line (at {px} {py} 90) (length 2.54)')
        lines.append(f'\t\t\t(name "{name}" (effects (font (size 1.016 1.016))))')
        lines.append(f'\t\t\t(number "{pn}" (effects (font (size 1.016 1.016)))))')

    lines.append('\t)')
    lines.append('\t(embedded_fonts no)')
    lines.append(')')
    return "\n".join(lines)


def _gen_drv8876_symbol():
    """DRV8876 H-bridge motor driver symbol."""
    pins = CUSTOM_PINS["PDCM:DRV8876"]
    lines = ['(symbol "PDCM:DRV8876"',
             '\t(exclude_from_sim no)', '\t(in_bom yes)', '\t(on_board yes)']
    lines.append(_gen_sym_property("Reference", "U"))
    lines.append(_gen_sym_property("Value", "DRV8876"))
    lines.append(_gen_sym_property("Footprint", "", True))
    lines.append(_gen_sym_property("Datasheet", "~", True))
    lines.append(_gen_sym_property("Description",
                 "TI DRV8876 H-Bridge Motor Driver, AEC-Q100, WSON-8", True))

    lines.append('\t(symbol "DRV8876_0_1"')
    lines.append('\t\t(rectangle (start -10.16 -12.7) (end 10.16 12.7)')
    lines.append('\t\t\t(stroke (width 0.254) (type default))')
    lines.append('\t\t\t(fill (type background)))')
    lines.append('\t)')

    pin_defs = [
        ("1", "VM",      "power_in",   0),
        ("2", "GND",     "power_in",   90),
        ("3", "IN1",     "input",      0),
        ("4", "IN2",     "input",      0),
        ("5", "nSLEEP",  "input",      0),
        ("6", "IPROPI",  "output",     180),
        ("7", "nFAULT",  "output",     180),
        ("8", "OUT",     "output",     180),
    ]
    lines.append('\t(symbol "DRV8876_1_1"')
    for pnum, pname, ptype, pdir in pin_defs:
        px, py = pins[pnum]
        lines.append(f'\t\t(pin {ptype} line (at {px} {py} {pdir}) (length 2.54)')
        lines.append(f'\t\t\t(name "{pname}" (effects (font (size 1.016 1.016))))')
        lines.append(f'\t\t\t(number "{pnum}" (effects (font (size 1.016 1.016)))))')
    lines.append('\t)')
    lines.append('\t(embedded_fonts no)')
    lines.append(')')
    return "\n".join(lines)


def gen_custom_sym_lib():
    """Generate PDCM.kicad_sym custom symbol library file."""
    lines = ['(kicad_symbol_lib',
             f'\t(version {KICAD_VERSION})',
             '\t(generator "pdcm_generator")',
             '\t(generator_version "1.0")']
    for sym_name in ["S32K358", "DRV8876"]:
        sym_text = get_custom_symbol_text(sym_name)
        sym_text = sym_text.replace(f'(symbol "PDCM:{sym_name}"',
                                     f'(symbol "{sym_name}"', 1)
        indented = "\n".join("\t" + l for l in sym_text.split("\n"))
        lines.append(indented)
    lines.append(')')
    return "\n".join(lines) + "\n"


# ============================================================
# Voltage Divider Helper
# ============================================================
def add_voltage_divider(sb, x, y, r_top_ref, r_top_val, r_bot_ref, r_bot_val,
                        c_ref, c_val, input_label, output_label, gnd_flag=True):
    sb.glabel(input_label, x - 5, y, 0, "input")
    sb.wire(x - 5, y, x, y)
    rt = sb.R(r_top_ref, r_top_val, x, y + 3.81)
    tap_y = y + 7.62
    rb = sb.R(r_bot_ref, r_bot_val, x, tap_y + 3.81)
    cap_x = x + 7.62
    cf = sb.C(c_ref, c_val, cap_x, tap_y + 3.81)
    sb.wire(x, tap_y, cap_x, tap_y)
    sb.junction(x, tap_y)
    sb.glabel(output_label, cap_x + 5, tap_y, 0, "output")
    sb.wire(cap_x, tap_y, cap_x + 5, tap_y)
    gnd_y = tap_y + 7.62
    if gnd_flag:
        sb.gnd(x, gnd_y)
        sb.gnd(cap_x, gnd_y)
    return (x, tap_y)


# ============================================================
# Circuit Helper: Output Channel (MOSFET + shunt + INA180)
# ============================================================
def _wire_output_channel(sb, bx, by, ch_num, shunt_mohm):
    """Place and wire one output channel: IRFZ44N + shunt + INA180.
    bx, by = base position for this channel block.
    Returns nothing; all connections via global labels.
    Layout (vertical, top to bottom):
      GD_OUT_N label → gate resistor → MOSFET gate
      MOSFET drain ← LOAD_N label
      MOSFET source → shunt → GND
      INA180: IN+ to source, IN- to GND side, OUT → CSENSE_N
    """
    name = CH_NAME.get(ch_num, f"CH_{ch_num}")

    # Gate resistor (100Ω, horizontal at 90°)
    rg = sb.R(next_ref("R"), "100", bx, by, 90)
    rg_l = rg.pin("1")  # left at 90°
    rg_r = rg.pin("2")  # right at 90°

    # Gate driver output label → gate resistor left
    sb.glabel(f"GD_OUT_{ch_num}", rg_l[0] - 3, rg_l[1], 180, "input")
    sb.wire(rg_l[0] - 3, rg_l[1], rg_l[0], rg_l[1])

    # Gate pulldown (10kΩ, vertical, 180° so Pin 2=top, Pin 1=bottom)
    rpd = sb.R(next_ref("R"), "10k", bx + 10, by + 5, 180)
    rpd_top = rpd.pin("2")  # top (signal from gate)
    rpd_bot = rpd.pin("1")  # bottom (GND)

    # Wire gate R right → junction → pulldown top → MOSFET gate
    jx = bx + 10
    jy = rg_r[1]
    sb.wire(rg_r[0], rg_r[1], jx, jy)
    sb.wire(jx, jy, jx, rpd_top[1])
    sb.junction(jx, jy)

    # Pulldown bottom to GND
    sb.gnd(rpd_bot[0], rpd_bot[1] + 3)
    sb.wire(rpd_bot[0], rpd_bot[1], rpd_bot[0], rpd_bot[1] + 3)

    # MOSFET — IRLZ44N lib symbol, actual BOM part is IRFZ44N (same G/D/S pinout)
    q = sb.place("Transistor_FET:IRLZ44N", next_ref("Q"), "IRFZ44N",
                  bx + 15, by + 5)
    gate_x, gate_y = q.pin("1")
    drain_x, drain_y = q.pin("2")
    src_x, src_y = q.pin("3")

    # Wire gate junction to MOSFET gate
    sb.wire(jx, jy, gate_x, gate_y)

    # Load label at drain
    sb.glabel(f"LOAD_{ch_num}", drain_x + 8, drain_y, 0, "bidirectional")
    sb.wire(drain_x, drain_y, drain_x + 8, drain_y)

    # Shunt resistor: source → shunt → GND (180° so Pin 2=top, Pin 1=bottom)
    rs = sb.R(next_ref("R"), f"{shunt_mohm}m", src_x, src_y + 5, 180)
    rs_top = rs.pin("2")  # top (connects to source)
    rs_bot = rs.pin("1")  # bottom (GND side)

    # Wire source to shunt top
    sb.wire(src_x, src_y, rs_top[0], rs_top[1])

    # Sense node (between MOSFET source and shunt top)
    # Use src_x + 7 for IN+ route to avoid grid-snapping collision with CSENSE label
    sense_x = src_x + 7
    sense_y = src_y

    # INA180A1 current sense amplifier
    # KiCad pin map: 1=OUT, 2=GND, 3=IN+(+), 4=IN-(-), 5=V+(power)
    try:
        ina = sb.place("Amplifier_Current:INA180A1", next_ref("U"),
                        "INA180A1", bx + 30, by + 10)
        # Wire V+ (pin 5) to +3.3V — pin 5 is at top of symbol
        vs_x, vs_y = ina.pin("5")
        sb.v33(vs_x, vs_y - 3)
        sb.wire(vs_x, vs_y, vs_x, vs_y - 3)
        # Wire GND (pin 2) — pin 2 is at bottom
        ignd_x, ignd_y = ina.pin("2")
        sb.gnd(ignd_x, ignd_y + 3)
        sb.wire(ignd_x, ignd_y, ignd_x, ignd_y + 3)
        # Wire OUT (pin 1) → CSENSE label — pin 1 is at right
        out_x, out_y = ina.pin("1")
        sb.glabel(f"CSENSE_{ch_num}", out_x + 5, out_y, 0, "output")
        sb.wire(out_x, out_y, out_x + 5, out_y)
        # IN+ (pin 3) to MOSFET source (sense node) — pin 3 is at left-top
        inp_x, inp_y = ina.pin("3")
        sb.wire(inp_x, inp_y, sense_x, inp_y)
        sb.wire(sense_x, inp_y, sense_x, sense_y)
        sb.wire(sense_x, sense_y, src_x, src_y)
        sb.junction(src_x, src_y)
        # IN- (pin 4) to GND side of shunt — pin 4 is at left-bottom
        inn_x, inn_y = ina.pin("4")
        inn_route_x = rs_bot[0] + 15
        sb.wire(inn_x, inn_y, inn_route_x, inn_y)
        sb.wire(inn_route_x, inn_y, inn_route_x, rs_bot[1])
        sb.wire(inn_route_x, rs_bot[1], rs_bot[0], rs_bot[1])
        sb.junction(rs_bot[0], rs_bot[1])
    except (ValueError, KeyError) as e:
        # INA180 symbol not found in library — use CSENSE label directly
        print(f"  WARNING: INA180A1 placement failed for ch {ch_num}: {e}")
        sb.glabel(f"CSENSE_{ch_num}", sense_x + 3, sense_y, 0, "output")
        sb.wire(src_x, src_y, sense_x + 3, sense_y)
        sb.junction(src_x, src_y)

    # Shunt bottom to GND
    sb.gnd(rs_bot[0], rs_bot[1] + 3)
    sb.wire(rs_bot[0], rs_bot[1], rs_bot[0], rs_bot[1] + 3)

    # Channel name annotation
    sb.note(name, bx, by - 5)


def _wire_gate_driver_pair(sb, bx, by, ic_num, ch_a, ch_b):
    """Place one TC4427A IC with bypass cap, wired to two channel labels.
    TC4427xOA pins: 2=IN_A, 3=GND, 4=IN_B, 5=GND_B, 6=VDD, 7=OUT_A, 8=OUT_B(NC)
    Actually TC4427xOA: 1=NC, 2=IN_A, 3=GND, 4=IN_B, 5=OUT_B, 6=VDD, 7=OUT_A, 8=NC
    Let me use the actual KiCad pin numbers.
    """
    ref = f"U_GD{ic_num}"
    tc = sb.place("Driver_FET:TC4427xOA", ref, "TC4427A", bx, by)

    # VDD (pin 6) → +12V
    vdd_x, vdd_y = tc.pin("6")
    sb.v12(vdd_x, vdd_y - 5)
    sb.wire(vdd_x, vdd_y, vdd_x, vdd_y - 5)

    # GND (pin 3) → GND
    gnd_x, gnd_y = tc.pin("3")
    sb.gnd(gnd_x, gnd_y + 5)
    sb.wire(gnd_x, gnd_y, gnd_x, gnd_y + 5)

    # Bypass cap (100nF) VDD to GND
    cb = sb.C(next_ref("C"), "100n", bx + 15, by)
    cb_top = cb.pin("2")  # top
    cb_bot = cb.pin("1")  # bottom
    sb.wire(cb_top[0], cb_top[1], cb_top[0], vdd_y)
    sb.wire(cb_top[0], vdd_y, vdd_x, vdd_y)
    sb.junction(vdd_x, vdd_y)
    sb.wire(cb_bot[0], cb_bot[1], cb_bot[0], gnd_y)
    sb.wire(cb_bot[0], gnd_y, gnd_x, gnd_y)
    sb.junction(gnd_x, gnd_y)

    # Channel A: IN_A (pin 2) ← MCU_OUT_N, OUT_A (pin 7) → GD_OUT_N
    in_a_x, in_a_y = tc.pin("2")
    name_a = CH_NAME.get(ch_a, f"CH_{ch_a}")
    sb.glabel(f"MCU_OUT_{ch_a}", in_a_x - 8, in_a_y, 180, "input")
    sb.wire(in_a_x, in_a_y, in_a_x - 8, in_a_y)

    out_a_x, out_a_y = tc.pin("7")
    sb.glabel(f"GD_OUT_{ch_a}", out_a_x + 8, out_a_y, 0, "output")
    sb.wire(out_a_x, out_a_y, out_a_x + 8, out_a_y)

    # Channel B: IN_B (pin 4) ← MCU_OUT_N, OUT_B (pin 5) → GD_OUT_N
    if ch_b is not None:
        in_b_x, in_b_y = tc.pin("4")
        sb.glabel(f"MCU_OUT_{ch_b}", in_b_x - 8, in_b_y, 180, "input")
        sb.wire(in_b_x, in_b_y, in_b_x - 8, in_b_y)

        out_b_x, out_b_y = tc.pin("5")
        sb.glabel(f"GD_OUT_{ch_b}", out_b_x + 8, out_b_y, 0, "output")
        sb.wire(out_b_x, out_b_y, out_b_x + 8, out_b_y)
    else:
        # No channel B — no-connect on unused pins
        in_b_x, in_b_y = tc.pin("4")
        sb.no_connect(in_b_x, in_b_y)
        out_b_x, out_b_y = tc.pin("5")
        sb.no_connect(out_b_x, out_b_y)

    # No-connect on NC pins (1, 8)
    for nc_pin in ["1", "8"]:
        try:
            nc_x, nc_y = tc.pin(nc_pin)
            sb.no_connect(nc_x, nc_y)
        except (ValueError, KeyError):
            pass

    # IC label
    sb.note(f"GD{ic_num}: {name_a}" + (f" / {CH_NAME.get(ch_b, '')}" if ch_b else ""),
            bx - 5, by - 15)


def _wire_switch_input(sb, bx, by, sw_name):
    """Place switch input conditioning: 1kΩ series + TVS + GPIO label."""
    # Series resistor (1kΩ, horizontal at 90°)
    rs = sb.R(next_ref("R"), "1k", bx, by, 90)
    rs_l = rs.pin("1")
    rs_r = rs.pin("2")

    # Input label (from connector)
    sb.glabel(f"SW_{sw_name}_IN", rs_l[0] - 5, rs_l[1], 180, "input")
    sb.wire(rs_l[0] - 5, rs_l[1], rs_l[0], rs_l[1])

    # TVS to GND after resistor
    tvs = sb.place("Device:D_TVS", next_ref("D"), "PESD3V3", bx + 10, by + 5, 90)
    tvs_k = tvs.pin("1")  # cathode (top at 90°)
    tvs_a = tvs.pin("2")  # anode (bottom at 90°)

    # Wire resistor right → TVS cathode junction → GPIO label
    sb.wire(rs_r[0], rs_r[1], tvs_k[0], rs_r[1])
    if abs(rs_r[1] - tvs_k[1]) > 0.1:
        sb.wire(tvs_k[0], rs_r[1], tvs_k[0], tvs_k[1])
    sb.junction(tvs_k[0], rs_r[1])

    # GPIO label
    sb.glabel(f"SW_{sw_name}", tvs_k[0] + 8, rs_r[1], 0, "output")
    sb.wire(tvs_k[0], rs_r[1], tvs_k[0] + 8, rs_r[1])

    # TVS anode to GND
    sb.gnd(tvs_a[0], tvs_a[1] + 3)
    sb.wire(tvs_a[0], tvs_a[1], tvs_a[0], tvs_a[1] + 3)


# ============================================================
# Sheet Generators
# ============================================================

def gen_power_input():
    """Power input: 12V TVS + reverse polarity P-FET + LM2596S-5 (5V) + AMS1117 (3.3V)."""
    reset_refs()
    sb = SheetBuilder("power_input", SHEET_UUIDS["power_input"])
    sb.note("POWER INPUT — TVS + Reverse Polarity + 5V + 3.3V Regulators", 30, 15)

    # --- Input section ---
    bx, by = 40, 50
    f1 = sb.place("Device:Fuse", "F1", "10A", bx, by)
    f1_top = f1.pin("1"); f1_bot = f1.pin("2")
    sb.glabel("+12V_BAT", f1_top[0] - 10, f1_top[1], 180, "input")
    sb.wire(f1_top[0] - 10, f1_top[1], f1_top[0], f1_top[1])

    # TVS (bidirectional, 90° → vertical: pin 2 top, pin 1 bottom)
    tvs = sb.place("Device:D_TVS", "D1", "SMBJ16A", bx + 12, by + 10, 90)
    tvs_top = tvs.pin("2"); tvs_bot = tvs.pin("1")
    # Horizontal bus wire from fuse to TVS X position
    sb.wire(f1_bot[0], f1_bot[1], tvs_top[0], f1_bot[1])
    sb.junction(tvs_top[0], f1_bot[1])
    # Vertical drop from bus to TVS top pin
    if abs(f1_bot[1] - tvs_top[1]) > 0.1:
        sb.wire(tvs_top[0], f1_bot[1], tvs_top[0], tvs_top[1])
    # TVS bottom to GND
    sb.gnd(tvs_bot[0], tvs_bot[1] + 3)
    sb.wire(tvs_bot[0], tvs_bot[1], tvs_bot[0], tvs_bot[1] + 3)

    # P-FET reverse polarity
    fet = sb.place("Transistor_FET:IRF9540N", "Q1", "IRF9540N", bx + 35, by)
    gate = fet.pin("1"); drain = fet.pin("2"); src = fet.pin("3")
    sb.wire(tvs_top[0], f1_bot[1], src[0], f1_bot[1])
    if abs(f1_bot[1] - src[1]) > 0.1:
        sb.wire(src[0], f1_bot[1], src[0], src[1])
    rg = sb.R("R1", "100k", bx + 25, by - 12, 90)
    rg_l = rg.pin("1"); rg_r = rg.pin("2")
    sb.wire(gate[0], gate[1], gate[0], rg_l[1])
    sb.wire(gate[0], rg_l[1], rg_l[0], rg_l[1])
    sb.wire(rg_r[0], rg_r[1], drain[0], rg_r[1])
    if abs(rg_r[1] - drain[1]) > 0.1:
        sb.wire(drain[0], rg_r[1], drain[0], drain[1])
    sb.junction(drain[0], drain[1])

    # +12V output
    sb.glabel("+12V", drain[0] + 15, drain[1], 0, "output")
    sb.wire(drain[0], drain[1], drain[0] + 15, drain[1])

    # PWR_FLAG on +12V
    pf12 = sb.place("power:PWR_FLAG", next_ref("#FLG"), "PWR_FLAG",
                     drain[0] + 15, drain[1] - 5)
    sb.wire(pf12.pin("1")[0], pf12.pin("1")[1], pf12.pin("1")[0], drain[1])
    sb.junction(drain[0] + 15, drain[1])

    # Bulk cap
    c1 = sb.CP("C1", "100u/25V", bx + 60, by + 10)
    sb.wire(c1.pin("2")[0], c1.pin("2")[1], c1.pin("2")[0], drain[1])
    sb.wire(c1.pin("2")[0], drain[1], drain[0], drain[1])
    sb.gnd(c1.pin("1")[0], c1.pin("1")[1] + 3)
    sb.wire(c1.pin("1")[0], c1.pin("1")[1], c1.pin("1")[0], c1.pin("1")[1] + 3)

    # --- LM2596S-5 Buck (12V → 5V) ---
    rx, ry = 40, 120
    sb.note("5V Buck — LM2596S-5", 30, 105)
    reg = sb.place("Regulator_Switching:LM2596S-5", "U1", "LM2596S-5", rx + 30, ry)
    vin = reg.pin("1"); out = reg.pin("2"); rgnd = reg.pin("3")
    fb = reg.pin("4"); onoff = reg.pin("5")

    sb.v12(vin[0] - 10, vin[1])
    sb.wire(vin[0] - 10, vin[1], vin[0], vin[1])
    c2 = sb.C("C2", "100n", rx + 10, ry + 10)
    sb.wire(c2.pin("2")[0], c2.pin("2")[1], c2.pin("2")[0], vin[1])
    sb.wire(c2.pin("2")[0], vin[1], vin[0], vin[1])
    sb.junction(vin[0], vin[1])
    sb.gnd(c2.pin("1")[0], c2.pin("1")[1] + 3)
    sb.wire(c2.pin("1")[0], c2.pin("1")[1], c2.pin("1")[0], c2.pin("1")[1] + 3)
    sb.gnd(rgnd[0], rgnd[1] + 5)
    sb.wire(rgnd[0], rgnd[1], rgnd[0], rgnd[1] + 5)

    # ON/OFF pull to GND (always on)
    sb.gnd(onoff[0], onoff[1] + 5)
    sb.wire(onoff[0], onoff[1], onoff[0], onoff[1] + 5)

    # Inductor
    l1 = sb.place("Device:L", "L1", "33uH", rx + 55, ry, 90)
    l1_l = l1.pin("1"); l1_r = l1.pin("2")
    sb.wire(out[0], out[1], l1_l[0], out[1])
    if abs(out[1] - l1_l[1]) > 0.1:
        sb.wire(l1_l[0], out[1], l1_l[0], l1_l[1])

    # Schottky
    d2 = sb.place("Device:D_Schottky", "D2", "SS54", rx + 48, ry + 12, 90)
    sb.wire(d2.pin("1")[0], d2.pin("1")[1], d2.pin("1")[0], out[1])
    sb.wire(d2.pin("1")[0], out[1], out[0], out[1])
    sb.junction(out[0], out[1])
    sb.gnd(d2.pin("2")[0], d2.pin("2")[1] + 3)
    sb.wire(d2.pin("2")[0], d2.pin("2")[1], d2.pin("2")[0], d2.pin("2")[1] + 3)

    out_rail_x, out_rail_y = l1_r[0], l1_r[1]
    sb.wire(fb[0], fb[1], out_rail_x, fb[1])
    if abs(fb[1] - out_rail_y) > 0.1:
        sb.wire(out_rail_x, fb[1], out_rail_x, out_rail_y)
    sb.junction(out_rail_x, out_rail_y)

    # Output caps
    c3 = sb.CP("C3", "220u/10V", rx + 75, ry + 10)
    sb.wire(c3.pin("2")[0], c3.pin("2")[1], c3.pin("2")[0], out_rail_y)
    sb.wire(c3.pin("2")[0], out_rail_y, out_rail_x, out_rail_y)
    sb.gnd(c3.pin("1")[0], c3.pin("1")[1] + 3)
    sb.wire(c3.pin("1")[0], c3.pin("1")[1], c3.pin("1")[0], c3.pin("1")[1] + 3)

    sb.glabel("+5V", out_rail_x + 15, out_rail_y, 0, "output")
    sb.wire(out_rail_x, out_rail_y, out_rail_x + 15, out_rail_y)

    # PWR_FLAG on +5V
    pf5 = sb.place("power:PWR_FLAG", next_ref("#FLG"), "PWR_FLAG",
                    out_rail_x + 15, out_rail_y - 5)
    sb.wire(pf5.pin("1")[0], pf5.pin("1")[1], pf5.pin("1")[0], out_rail_y)
    sb.junction(out_rail_x + 15, out_rail_y)

    # --- AMS1117-3.3 LDO (5V → 3.3V) ---
    lx, ly = 40, 200
    sb.note("3.3V LDO — AMS1117-3.3", 30, 185)

    try:
        ldo = sb.place("Regulator_Linear:AMS1117-3.3", "U2", "AMS1117-3.3", lx + 30, ly)
        ldo_in = ldo.pin("3")   # VIN
        ldo_out = ldo.pin("2")  # VOUT
        ldo_gnd = ldo.pin("1")  # GND
    except (ValueError, KeyError):
        # Fallback: just place labels for 3.3V
        sb.glabel("+3.3V", lx + 60, ly, 0, "output")
        sb.v5(lx, ly)
        sb.wire(lx, ly, lx + 60, ly)
        # PWR_FLAGs
        pf_gnd = sb.place("power:PWR_FLAG", next_ref("#FLG"), "PWR_FLAG", bx + 12, by + 25)
        sb.gnd(pf_gnd.pin("1")[0], pf_gnd.pin("1")[1] + 3)
        sb.wire(pf_gnd.pin("1")[0], pf_gnd.pin("1")[1], pf_gnd.pin("1")[0], pf_gnd.pin("1")[1] + 3)
        return sb.build()

    sb.v5(ldo_in[0] - 10, ldo_in[1])
    sb.wire(ldo_in[0] - 10, ldo_in[1], ldo_in[0], ldo_in[1])

    # Input cap
    ci = sb.C(next_ref("C"), "100n", lx + 15, ly + 8)
    sb.wire(ci.pin("2")[0], ci.pin("2")[1], ci.pin("2")[0], ldo_in[1])
    sb.wire(ci.pin("2")[0], ldo_in[1], ldo_in[0], ldo_in[1])
    sb.junction(ldo_in[0], ldo_in[1])
    sb.gnd(ci.pin("1")[0], ci.pin("1")[1] + 3)
    sb.wire(ci.pin("1")[0], ci.pin("1")[1], ci.pin("1")[0], ci.pin("1")[1] + 3)

    sb.gnd(ldo_gnd[0], ldo_gnd[1] + 5)
    sb.wire(ldo_gnd[0], ldo_gnd[1], ldo_gnd[0], ldo_gnd[1] + 5)

    # Output cap + label
    co = sb.C(next_ref("C"), "22u", lx + 50, ly + 8)
    sb.wire(co.pin("2")[0], co.pin("2")[1], co.pin("2")[0], ldo_out[1])
    sb.wire(co.pin("2")[0], ldo_out[1], ldo_out[0], ldo_out[1])
    sb.gnd(co.pin("1")[0], co.pin("1")[1] + 3)
    sb.wire(co.pin("1")[0], co.pin("1")[1], co.pin("1")[0], co.pin("1")[1] + 3)

    sb.glabel("+3.3V", ldo_out[0] + 15, ldo_out[1], 0, "output")
    sb.wire(ldo_out[0], ldo_out[1], ldo_out[0] + 15, ldo_out[1])

    # PWR_FLAGs
    pf_gnd = sb.place("power:PWR_FLAG", next_ref("#FLG"), "PWR_FLAG", bx + 12, by + 25)
    sb.gnd(pf_gnd.pin("1")[0], pf_gnd.pin("1")[1] + 3)
    sb.wire(pf_gnd.pin("1")[0], pf_gnd.pin("1")[1], pf_gnd.pin("1")[0], pf_gnd.pin("1")[1] + 3)

    return sb.build()


def gen_can_bus():
    """Single MCP2562FD CAN transceiver + termination."""
    reset_refs()
    sb = SheetBuilder("can_bus", SHEET_UUIDS["can_bus"])
    sb.note("CAN BUS — MCP2562FD Transceiver", 30, 15)

    bx, by = 80, 60
    u = sb.place("Interface_CAN_LIN:MCP2562-E-SN", "U_CAN1", "MCP2562FD", bx, by)

    # TXD (pin 1)
    txd = u.pin("1")
    sb.glabel("CAN_TX", txd[0] - 8, txd[1], 180, "input")
    sb.wire(txd[0], txd[1], txd[0] - 8, txd[1])
    # RXD (pin 4)
    rxd = u.pin("4")
    sb.glabel("CAN_RX", rxd[0] - 8, rxd[1], 180, "output")
    sb.wire(rxd[0], rxd[1], rxd[0] - 8, rxd[1])
    # VDD (pin 3) → +5V
    vdd = u.pin("3")
    sb.v5(vdd[0], vdd[1] - 5)
    sb.wire(vdd[0], vdd[1], vdd[0], vdd[1] - 5)
    # VSS (pin 2) → GND
    vss = u.pin("2")
    sb.gnd(vss[0], vss[1] + 5)
    sb.wire(vss[0], vss[1], vss[0], vss[1] + 5)
    # Vio (pin 5) → VDD
    vio = u.pin("5")
    sb.wire(vio[0], vio[1], vio[0], vdd[1])
    sb.wire(vio[0], vdd[1], vdd[0], vdd[1])
    sb.junction(vdd[0], vdd[1])
    # STBY (pin 8) → GND
    stby = u.pin("8")
    sb.wire(stby[0], stby[1], stby[0], vss[1])
    sb.wire(stby[0], vss[1], vss[0], vss[1])
    sb.junction(vss[0], vss[1])
    # CANH (pin 7)
    canh = u.pin("7")
    sb.glabel("CAN_H", canh[0] + 5, canh[1], 0, "bidirectional")
    sb.wire(canh[0], canh[1], canh[0] + 5, canh[1])
    # CANL (pin 6)
    canl = u.pin("6")
    sb.glabel("CAN_L", canl[0] + 5, canl[1], 0, "bidirectional")
    sb.wire(canl[0], canl[1], canl[0] + 5, canl[1])
    # Bypass cap
    cb = sb.C(next_ref("C"), "100n", bx + 20, by)
    sb.wire(cb.pin("2")[0], cb.pin("2")[1], cb.pin("2")[0], vdd[1])
    sb.wire(cb.pin("2")[0], vdd[1], vdd[0], vdd[1])
    sb.wire(cb.pin("1")[0], cb.pin("1")[1], cb.pin("1")[0], vss[1])
    sb.wire(cb.pin("1")[0], vss[1], vss[0], vss[1])
    # Termination resistor
    rt = sb.R("R_TERM", "120", 80, 130, 90)
    sb.glabel("CAN_H", rt.pin("1")[0] - 3, rt.pin("1")[1], 180, "bidirectional")
    sb.wire(rt.pin("1")[0], rt.pin("1")[1], rt.pin("1")[0] - 3, rt.pin("1")[1])
    sb.glabel("CAN_L", rt.pin("2")[0] + 3, rt.pin("2")[1], 0, "bidirectional")
    sb.wire(rt.pin("2")[0], rt.pin("2")[1], rt.pin("2")[0] + 3, rt.pin("2")[1])

    return sb.build()


def gen_mcu():
    """S32K358 MCU with decoupling, crystal, reset, JTAG, all pin labels."""
    reset_refs()
    sb = SheetBuilder("mcu", SHEET_UUIDS["mcu"], paper="A1")
    sb.note("S32K358 MCU — HDQFP-172", 30, 15)

    mcu = sb.place("PDCM:S32K358", "U_MCU", "S32K358", 150, 130)

    # --- Output GPIO pins (1-46) → MCU_OUT_N labels ---
    for i in range(46):
        pin_num = str(i + 1)
        ch = i if i < 24 else i + 1  # skip ch 24
        px, py = mcu.pin(pin_num)
        sb.glabel(f"MCU_OUT_{ch}", px - 8, py, 180, "output")
        sb.wire(px, py, px - 8, py)

    # --- H-bridge control (pins 47-50) ---
    hb_labels = ["HB_IN1", "HB_IN2", "HB_NSLEEP", "HB_NFAULT"]
    for i, label in enumerate(hb_labels):
        pin_num = str(47 + i)
        px, py = mcu.pin(pin_num)
        shape = "input" if label == "HB_NFAULT" else "output"
        sb.glabel(label, px - 8, py, 180, shape)
        sb.wire(px, py, px - 8, py)

    # --- Switch input pins (51-65) → SW_xxx labels ---
    for i, sw_name in enumerate(SWITCH_DEFS):
        pin_num = str(51 + i)
        px, py = mcu.pin(pin_num)
        sb.glabel(f"SW_{sw_name}", px - 8, py, 180, "input")
        sb.wire(px, py, px - 8, py)

    # --- Current sense ADC (pins 66-111) → CSENSE_N labels ---
    for i in range(46):
        pin_num = str(66 + i)
        ch = i if i < 24 else i + 1
        px, py = mcu.pin(pin_num)
        sb.glabel(f"CSENSE_{ch}", px + 8, py, 0, "input")
        sb.wire(px, py, px + 8, py)

    # --- Sensor ADC (pins 112-114) ---
    sensor_labels = ["BATT_ADC", "4WD_ADC", "HB_CS_ADC"]
    for i, label in enumerate(sensor_labels):
        pin_num = str(112 + i)
        px, py = mcu.pin(pin_num)
        sb.glabel(label, px + 8, py, 0, "input")
        sb.wire(px, py, px + 8, py)

    # --- Top pins (115-129): power, CAN, crystal, JTAG ---
    top_labels = ["VDD_HV_1", "VDD_HV_2", "VDD_HV_3", "VDD_HV_4", "VDDA",
                  "CAN_TX", "CAN_RX", "XTAL_IN", "XTAL_OUT", "nRESET",
                  "JTAG_TCK", "JTAG_TMS", "JTAG_TDI", "JTAG_TDO", "JTAG_TRST"]
    for i, label in enumerate(top_labels):
        pin_num = str(115 + i)
        px, py = mcu.pin(pin_num)
        if label.startswith("VDD"):
            sb.v33(px, py - 8)
            sb.wire(px, py, px, py - 8)
        elif label == "VDDA":
            sb.v33(px, py - 8)
            sb.wire(px, py, px, py - 8)
        elif label in ("CAN_TX", "CAN_RX"):
            shape = "output" if "TX" in label else "input"
            sb.glabel(label, px, py - 8, 90, shape)
            sb.wire(px, py, px, py - 8)
        elif label.startswith("XTAL"):
            sb.llabel(label, px, py - 8, 90)
            sb.wire(px, py, px, py - 8)
        elif label == "nRESET":
            sb.llabel(label, px, py - 8, 90)
            sb.wire(px, py, px, py - 8)
        else:  # JTAG
            sb.llabel(label, px, py - 8, 90)
            sb.wire(px, py, px, py - 8)

    # --- Bottom pins (130-134): ground ---
    for i in range(5):
        pin_num = str(130 + i)
        px, py = mcu.pin(pin_num)
        sb.gnd(px, py + 5)
        sb.wire(px, py, px, py + 5)

    # --- Extra power/ground pins (135-172) ---
    # Route upward (py - 5) to avoid crossing bottom GND pin wires
    for i in range(38):
        pin_num = str(135 + i)
        px, py = mcu.pin(pin_num)
        if i % 2 == 0:
            sb.gnd(px, py - 5)
            sb.wire(px, py, px, py - 5)
        else:
            sb.v33(px, py - 5)
            sb.wire(px, py, px, py - 5)

    # --- Decoupling caps (100nF × 8 + 10uF bulk) ---
    dcap_x = 250
    for i in range(8):
        c = sb.C(next_ref("C"), "100n", dcap_x, 50 + i * 15)
        sb.v33(c.pin("2")[0], c.pin("2")[1] - 3)
        sb.wire(c.pin("2")[0], c.pin("2")[1], c.pin("2")[0], c.pin("2")[1] - 3)
        sb.gnd(c.pin("1")[0], c.pin("1")[1] + 3)
        sb.wire(c.pin("1")[0], c.pin("1")[1], c.pin("1")[0], c.pin("1")[1] + 3)

    cbulk = sb.CP(next_ref("C"), "10u", dcap_x + 15, 50)
    sb.v33(cbulk.pin("2")[0], cbulk.pin("2")[1] - 3)
    sb.wire(cbulk.pin("2")[0], cbulk.pin("2")[1], cbulk.pin("2")[0], cbulk.pin("2")[1] - 3)
    sb.gnd(cbulk.pin("1")[0], cbulk.pin("1")[1] + 3)
    sb.wire(cbulk.pin("1")[0], cbulk.pin("1")[1], cbulk.pin("1")[0], cbulk.pin("1")[1] + 3)

    # --- Crystal ---
    xtal = sb.place("Device:Crystal", next_ref("Y"), "16MHz", 250, 170)
    xp1 = xtal.pin("1"); xp2 = xtal.pin("2")
    sb.llabel("XTAL_IN", xp1[0] - 5, xp1[1], 180)
    sb.wire(xp1[0], xp1[1], xp1[0] - 5, xp1[1])
    sb.llabel("XTAL_OUT", xp2[0] + 5, xp2[1])
    sb.wire(xp2[0], xp2[1], xp2[0] + 5, xp2[1])
    # Load caps
    cl1 = sb.C(next_ref("C"), "20p", 240, 180)
    sb.wire(cl1.pin("2")[0], cl1.pin("2")[1], xp1[0], cl1.pin("2")[1])
    sb.wire(xp1[0], cl1.pin("2")[1], xp1[0], xp1[1])
    sb.junction(xp1[0], xp1[1])
    sb.gnd(cl1.pin("1")[0], cl1.pin("1")[1] + 3)
    sb.wire(cl1.pin("1")[0], cl1.pin("1")[1], cl1.pin("1")[0], cl1.pin("1")[1] + 3)
    cl2 = sb.C(next_ref("C"), "20p", 260, 180)
    sb.wire(cl2.pin("2")[0], cl2.pin("2")[1], xp2[0], cl2.pin("2")[1])
    sb.wire(xp2[0], cl2.pin("2")[1], xp2[0], xp2[1])
    sb.junction(xp2[0], xp2[1])
    sb.gnd(cl2.pin("1")[0], cl2.pin("1")[1] + 3)
    sb.wire(cl2.pin("1")[0], cl2.pin("1")[1], cl2.pin("1")[0], cl2.pin("1")[1] + 3)

    # --- Reset circuit ---
    sb.note("Reset", 250, 200)
    rr = sb.R(next_ref("R"), "10k", 260, 210)
    sb.v33(rr.pin("2")[0], rr.pin("2")[1] - 3)
    sb.wire(rr.pin("2")[0], rr.pin("2")[1], rr.pin("2")[0], rr.pin("2")[1] - 3)
    sb.llabel("nRESET", rr.pin("1")[0] + 5, rr.pin("1")[1])
    sb.wire(rr.pin("1")[0], rr.pin("1")[1], rr.pin("1")[0] + 5, rr.pin("1")[1])
    cr = sb.C(next_ref("C"), "100n", 270, 220)
    sb.wire(cr.pin("2")[0], cr.pin("2")[1], rr.pin("1")[0], cr.pin("2")[1])
    sb.wire(rr.pin("1")[0], cr.pin("2")[1], rr.pin("1")[0], rr.pin("1")[1])
    sb.junction(rr.pin("1")[0], rr.pin("1")[1])
    sb.gnd(cr.pin("1")[0], cr.pin("1")[1] + 3)
    sb.wire(cr.pin("1")[0], cr.pin("1")[1], cr.pin("1")[0], cr.pin("1")[1] + 3)

    # --- JTAG header (2x5 Cortex Debug) ---
    sb.note("JTAG/SWD Debug Header", 250, 240)
    try:
        jh = sb.place("Connector_Generic:Conn_02x05_Odd_Even", next_ref("J"),
                       "JTAG", 270, 260)
        jtag_map = {"1": "JTAG_TCK", "2": "JTAG_TMS", "3": "JTAG_TDI",
                    "4": "JTAG_TDO", "5": "JTAG_TRST"}
        for pn, label in jtag_map.items():
            px, py = jh.pin(pn)
            sb.llabel(label, px - 8, py, 180)
            sb.wire(px, py, px - 8, py)
        # VCC and GND on remaining pins
        for pn in ["6", "7", "8", "9", "10"]:
            try:
                px, py = jh.pin(pn)
                if pn in ("6", "8", "10"):
                    sb.gnd(px + 5, py)
                    sb.wire(px, py, px + 5, py)
                else:
                    sb.v33(px + 5, py)
                    sb.wire(px, py, px + 5, py)
            except (ValueError, KeyError):
                pass
    except (ValueError, KeyError):
        sb.note("(JTAG header — add manually)", 250, 260)

    return sb.build()


def gen_gate_drivers_a():
    """TC4427A ICs #1-12 for channels 0-23 (Tier 1)."""
    reset_refs()
    sb = SheetBuilder("gate_drivers_a", SHEET_UUIDS["gate_drivers_a"], paper="A2")
    sb.note("GATE DRIVERS A — TC4427A #1-12 (Channels 0-23)", 30, 15)

    # 12 ICs in 4×3 grid
    for idx in range(12):
        col = idx % 4
        row = idx // 4
        bx = 30 + col * 90
        by = 40 + row * 90
        ic_num = idx + 1
        ch_a = idx * 2
        ch_b = idx * 2 + 1
        _wire_gate_driver_pair(sb, bx, by, ic_num, ch_a, ch_b)

    return sb.build()


def gen_gate_drivers_b():
    """TC4427A ICs #13-23 for channels 25-46 (Tier 2-3)."""
    reset_refs()
    sb = SheetBuilder("gate_drivers_b", SHEET_UUIDS["gate_drivers_b"], paper="A2")
    sb.note("GATE DRIVERS B — TC4427A #13-23 (Channels 25-46)", 30, 15)

    # Channels 25-46 = 22 channels = 11 ICs
    tier23_channels = [cd[0] for cd in CHANNEL_DEFS if cd[0] >= 25]
    for idx in range(0, len(tier23_channels), 2):
        ic_idx = idx // 2
        col = ic_idx % 4
        row = ic_idx // 4
        bx = 30 + col * 90
        by = 40 + row * 90
        ic_num = 13 + ic_idx
        ch_a = tier23_channels[idx]
        ch_b = tier23_channels[idx + 1] if idx + 1 < len(tier23_channels) else None
        _wire_gate_driver_pair(sb, bx, by, ic_num, ch_a, ch_b)

    return sb.build()


def gen_output_stage_a():
    """MOSFET + shunt + INA180 for channels 0-23."""
    reset_refs()
    sb = SheetBuilder("output_stage_a", SHEET_UUIDS["output_stage_a"], paper="A1")
    sb.note("OUTPUT STAGE A — Channels 0-23 (MOSFET + Shunt + INA180)", 30, 15)

    for idx in range(24):
        ch = idx
        col = idx % 6
        row = idx // 6
        bx = 30 + col * 65
        by = 40 + row * 60
        shunt = CH_SHUNT.get(ch, 100)
        _wire_output_channel(sb, bx, by, ch, shunt)

    return sb.build()


def gen_output_stage_b():
    """MOSFET + shunt + INA180 for channels 25-46."""
    reset_refs()
    sb = SheetBuilder("output_stage_b", SHEET_UUIDS["output_stage_b"], paper="A1")
    sb.note("OUTPUT STAGE B — Channels 25-46 (MOSFET + Shunt + INA180)", 30, 15)

    tier23_channels = [cd for cd in CHANNEL_DEFS if cd[0] >= 25]
    for idx, (ch, name, tier, shunt, pwm) in enumerate(tier23_channels):
        col = idx % 6
        row = idx // 6
        bx = 30 + col * 65
        by = 40 + row * 60
        _wire_output_channel(sb, bx, by, ch, shunt)

    return sb.build()


def gen_hbridge():
    """DRV8876 H-bridge for 4WD encoder motor."""
    reset_refs()
    sb = SheetBuilder("hbridge", SHEET_UUIDS["hbridge"])
    sb.note("H-BRIDGE — DRV8876 (4WD Encoder Motor, Ch 24)", 30, 15)

    bx, by = 100, 80
    drv = sb.place("PDCM:DRV8876", "U_HB", "DRV8876", bx, by)

    # VM (pin 1) → +12V
    vm = drv.pin("1")
    sb.v12(vm[0] - 8, vm[1])
    sb.wire(vm[0] - 8, vm[1], vm[0], vm[1])

    # GND (pin 2) → GND
    gnd_p = drv.pin("2")
    sb.gnd(gnd_p[0], gnd_p[1] + 5)
    sb.wire(gnd_p[0], gnd_p[1], gnd_p[0], gnd_p[1] + 5)

    # IN1 (pin 3) → HB_IN1 label
    in1 = drv.pin("3")
    sb.glabel("HB_IN1", in1[0] - 8, in1[1], 180, "input")
    sb.wire(in1[0], in1[1], in1[0] - 8, in1[1])

    # IN2 (pin 4) → HB_IN2 label
    in2 = drv.pin("4")
    sb.glabel("HB_IN2", in2[0] - 8, in2[1], 180, "input")
    sb.wire(in2[0], in2[1], in2[0] - 8, in2[1])

    # nSLEEP (pin 5) → HB_NSLEEP label + pullup
    nslp = drv.pin("5")
    sb.glabel("HB_NSLEEP", nslp[0] - 8, nslp[1], 180, "input")
    sb.wire(nslp[0], nslp[1], nslp[0] - 8, nslp[1])

    # IPROPI (pin 6) → sense resistor → ADC label
    # Horizontal resistor (90°): Pin 2=left (signal), Pin 1=right (GND)
    # Straight horizontal wire avoids the corner-routing pin-not-connected issue
    ipr = drv.pin("6")
    rs = sb.R(next_ref("R"), "1k", ipr[0] + 15, ipr[1], 90)
    # At 90°: Pin 1 = left, Pin 2 = right
    rs_left = rs.pin("1")   # left pin
    rs_right = rs.pin("2")  # right pin
    # HB_CS_ADC label at midpoint - split wire into two segments for proper connection
    label_x = ipr[0] + 8
    sb.wire(ipr[0], ipr[1], label_x, ipr[1])          # IPROPI → label
    sb.wire(label_x, ipr[1], rs_left[0], rs_left[1])   # label → R left pin
    sb.glabel("HB_CS_ADC", label_x, ipr[1], 0, "output")
    # Right pin to GND below
    sb.gnd(rs_right[0], rs_right[1] + 5)
    sb.wire(rs_right[0], rs_right[1], rs_right[0], rs_right[1] + 5)

    # nFAULT (pin 7) → label + pullup to +3.3V
    nflt = drv.pin("7")
    sb.glabel("HB_NFAULT", nflt[0] + 8, nflt[1], 0, "output")
    sb.wire(nflt[0], nflt[1], nflt[0] + 8, nflt[1])
    # Pullup resistor at 0°: Pin 1=top (+3.3V), Pin 2=bottom (signal)
    rpu = sb.R(next_ref("R"), "10k", nflt[0] + 8, nflt[1] - 8)
    rpu_top = rpu.pin("1")   # top pin
    rpu_bot = rpu.pin("2")   # bottom pin
    # +3.3V to top pin
    sb.v33(rpu_top[0], rpu_top[1] - 3)
    sb.wire(rpu_top[0], rpu_top[1], rpu_top[0], rpu_top[1] - 3)
    # Bottom pin to nFAULT junction
    sb.wire(rpu_bot[0], rpu_bot[1], nflt[0] + 8, nflt[1])
    sb.junction(nflt[0] + 8, nflt[1])

    # OUT (pin 8) → motor connector label
    outp = drv.pin("8")
    sb.glabel("MOTOR_OUT", outp[0] + 8, outp[1], 0, "output")
    sb.wire(outp[0], outp[1], outp[0] + 8, outp[1])

    # Bypass caps
    cb1 = sb.C(next_ref("C"), "100n", bx + 25, by - 15)
    sb.v12(cb1.pin("2")[0], cb1.pin("2")[1] - 3)
    sb.wire(cb1.pin("2")[0], cb1.pin("2")[1], cb1.pin("2")[0], cb1.pin("2")[1] - 3)
    sb.gnd(cb1.pin("1")[0], cb1.pin("1")[1] + 3)
    sb.wire(cb1.pin("1")[0], cb1.pin("1")[1], cb1.pin("1")[0], cb1.pin("1")[1] + 3)

    cb2 = sb.CP(next_ref("C"), "100u", bx + 35, by - 15)
    sb.v12(cb2.pin("2")[0], cb2.pin("2")[1] - 3)
    sb.wire(cb2.pin("2")[0], cb2.pin("2")[1], cb2.pin("2")[0], cb2.pin("2")[1] - 3)
    sb.gnd(cb2.pin("1")[0], cb2.pin("1")[1] + 3)
    sb.wire(cb2.pin("1")[0], cb2.pin("1")[1], cb2.pin("1")[0], cb2.pin("1")[1] + 3)

    return sb.build()


def gen_switch_inputs():
    """Switch conditioning circuits + battery divider + 4WD pot."""
    reset_refs()
    sb = SheetBuilder("switch_inputs", SHEET_UUIDS["switch_inputs"], paper="A2")
    sb.note("SWITCH INPUTS + SENSOR ADC", 30, 15)

    # 15 switch inputs in 2 rows of 8
    for idx, sw_name in enumerate(SWITCH_DEFS):
        col = idx % 8
        row = idx // 8
        bx = 30 + col * 50
        by = 40 + row * 50
        _wire_switch_input(sb, bx, by, sw_name)

    # Battery voltage divider
    add_voltage_divider(sb, 30, 160,
                        next_ref("R"), "10k", next_ref("R"), "3.3k",
                        next_ref("C"), "100n",
                        "+12V", "BATT_ADC")

    # 4WD position potentiometer divider
    add_voltage_divider(sb, 120, 160,
                        next_ref("R"), "10k", next_ref("R"), "3.3k",
                        next_ref("C"), "100n",
                        "4WD_POS_RAW", "4WD_ADC")

    return sb.build()


def gen_connectors():
    """13 Deutsch connectors grouped by truck routing zone.

    See docs/connector_pinout.md for full pin assignments and part numbers.
    Connector series: HD (battery), DT (loads 14-18 AWG), DTM (signals 20-24 AWG).
    """
    reset_refs()
    sb = SheetBuilder("connectors", SHEET_UUIDS["connectors"], paper="A1")
    sb.note("CONNECTORS — Deutsch HD / DT / DTM", 30, 15)
    sb.note("Grouped by truck routing zone — see docs/connector_pinout.md", 30, 22)

    # ── Connector definitions ─────────────────────────────────
    # Each entry: (ref, pin_count, value_label, pins)
    # Pin types: "LABEL:name" = global label, "GND" = power GND, "NC" = no-connect,
    #            "PWR:name" = power symbol (+5V, +12V)
    CONNECTOR_DEFS = [
        # J1 — Battery power (HD 2-pin)
        ("J1", 2, "PWR_BATT", [
            "LABEL:+12V_BAT",              # pin 1
            "GND",                          # pin 2
        ]),
        # J2 — CAN FD bus (DTM 4-pin)
        ("J2", 4, "CAN_BUS", [
            "LABEL:CAN_H",                 # pin 1
            "LABEL:CAN_L",                 # pin 2
            "GND",                          # pin 3 — shield drain
            "PWR:+12V",                     # pin 4 — transceiver supply
        ]),
        # J3 — Engine bay loads (DT 8-pin)
        ("J3", 8, "ENGINE_BAY", [
            "LABEL:LOAD_0",                # pin 1 — fuel pump
            "LABEL:LOAD_1",                # pin 2 — fan 1
            "LABEL:LOAD_2",                # pin 3 — fan 2
            "LABEL:LOAD_4",                # pin 4 — A/C clutch
            "LABEL:LOAD_17",               # pin 5 — horn
            "LABEL:LOAD_20",               # pin 6 — front axle actuator
            "NC",                           # pin 7 — spare
            "NC",                           # pin 8 — spare
        ]),
        # J4 — Front lighting (DT 12-pin)
        ("J4", 12, "FRONT_LIGHTS", [
            "LABEL:LOAD_5",                # pin 1 — low beam L
            "LABEL:LOAD_6",                # pin 2 — low beam R
            "LABEL:LOAD_7",                # pin 3 — high beam L
            "LABEL:LOAD_8",                # pin 4 — high beam R
            "LABEL:LOAD_9",                # pin 5 — turn signal L
            "LABEL:LOAD_10",               # pin 6 — turn signal R
            "LABEL:LOAD_14",               # pin 7 — DRL
            "LABEL:LOAD_23",               # pin 8 — light bar
            "NC",                           # pin 9 — spare
            "NC",                           # pin 10 — spare
            "NC",                           # pin 11 — spare
            "NC",                           # pin 12 — spare
        ]),
        # J5 — Rear lighting (DT 6-pin)
        ("J5", 6, "REAR_LIGHTS", [
            "LABEL:LOAD_11",               # pin 1 — brake light L
            "LABEL:LOAD_12",               # pin 2 — brake light R
            "LABEL:LOAD_13",               # pin 3 — reverse lights
            "LABEL:LOAD_38",               # pin 4 — bed lights
            "NC",                           # pin 5 — spare
            "NC",                           # pin 6 — spare
        ]),
        # J6 — Cabin / firewall (DT 8-pin)
        ("J6", 8, "CABIN", [
            "LABEL:LOAD_3",                # pin 1 — blower motor
            "LABEL:LOAD_18",               # pin 2 — wiper motor
            "LABEL:LOAD_15",               # pin 3 — interior light
            "LABEL:LOAD_16",               # pin 4 — courtesy light
            "LABEL:LOAD_19",               # pin 5 — accessory power
            "LABEL:LOAD_21",               # pin 6 — seat heater L
            "LABEL:LOAD_22",               # pin 7 — seat heater R
            "NC",                           # pin 8 — spare
        ]),
        # J7 — 4WD motor + position sensor (DT 6-pin)
        ("J7", 6, "4WD_TCASE", [
            "LABEL:MOTOR_OUT",             # pin 1 — H-bridge out A
            "GND",                          # pin 2 — H-bridge out B / motor return
            "LABEL:4WD_POS_RAW",           # pin 3 — pot wiper
            "PWR:+5V",                      # pin 4 — pot supply
            "GND",                          # pin 5 — pot ground
            "NC",                           # pin 6 — spare
        ]),
        # J8 — Steering column stalks (DTM 8-pin)
        ("J8", 8, "STEER_COL", [
            "LABEL:SW_TURN_L_IN",          # pin 1
            "LABEL:SW_TURN_R_IN",          # pin 2
            "LABEL:SW_HIGH_BEAM_IN",       # pin 3
            "LABEL:SW_FLASH_PASS_IN",      # pin 4
            "LABEL:SW_HORN_IN",            # pin 5
            "GND",                          # pin 6 — switch common
            "NC",                           # pin 7 — spare
            "NC",                           # pin 8 — spare
        ]),
        # J9 — Dash switches (DTM 8-pin)
        ("J9", 8, "DASH_SW", [
            "LABEL:SW_HAZARD_IN",          # pin 1
            "LABEL:SW_AC_REQ_IN",          # pin 2
            "LABEL:SW_WIPER_INT_IN",       # pin 3
            "LABEL:SW_WIPER_LO_IN",        # pin 4
            "LABEL:SW_WIPER_HI_IN",        # pin 5
            "LABEL:SW_WASHER_IN",          # pin 6
            "LABEL:SW_START_BTN_IN",       # pin 7
            "GND",                          # pin 8 — switch common
        ]),
        # J10 — Brake switches ONLY (DTM 4-pin) — SAFETY ISOLATED
        ("J10", 4, "BRAKE_SW", [
            "LABEL:SW_BRAKE_SW1_IN",       # pin 1
            "GND",                          # pin 2 — SW1 dedicated return
            "LABEL:SW_BRAKE_SW2_IN",       # pin 3
            "GND",                          # pin 4 — SW2 dedicated return
        ]),
        # J11 — Reverse + transmission (DTM 4-pin)
        ("J11", 4, "TRANS_SW", [
            "LABEL:SW_REVERSE_IN",         # pin 1
            "GND",                          # pin 2 — switch return
            "NC",                           # pin 3 — spare
            "NC",                           # pin 4 — spare
        ]),
        # J12 — Electronics / modules (DT 12-pin)
        ("J12", 12, "ELECTRONICS", [
            "LABEL:LOAD_25",               # pin 1 — amp remote 1
            "LABEL:LOAD_26",               # pin 2 — amp remote 2
            "LABEL:LOAD_27",               # pin 3 — HeadUnit enable
            "LABEL:LOAD_28",               # pin 4 — front camera
            "LABEL:LOAD_29",               # pin 5 — rear camera
            "LABEL:LOAD_30",               # pin 6 — side cameras
            "LABEL:LOAD_31",               # pin 7 — parking sensors
            "LABEL:LOAD_32",               # pin 8 — radar / BSM
            "LABEL:LOAD_33",               # pin 9 — GCM power
            "LABEL:LOAD_34",               # pin 10 — GPS / cellular
            "LABEL:LOAD_35",               # pin 11 — dash cam
            "LABEL:LOAD_36",               # pin 12 — future module
        ]),
        # J13 — Exterior aux + expansion (DT 12-pin)
        ("J13", 12, "EXT_AUX", [
            "LABEL:LOAD_37",               # pin 1 — rock lights
            "LABEL:LOAD_39",               # pin 2 — puddle / underbody
            "LABEL:LOAD_40",               # pin 3 — future exterior
            "LABEL:LOAD_41",               # pin 4 — expansion 1
            "LABEL:LOAD_42",               # pin 5 — expansion 2
            "LABEL:LOAD_43",               # pin 6 — expansion 3
            "LABEL:LOAD_44",               # pin 7 — expansion 4
            "LABEL:LOAD_45",               # pin 8 — expansion 5
            "LABEL:LOAD_46",               # pin 9 — expansion 6
            "NC",                           # pin 10 — spare
            "NC",                           # pin 11 — spare
            "NC",                           # pin 12 — spare
        ]),
    ]

    # ── Layout: 4 columns × 4 rows ───────────────────────────
    COLS, COL_W, ROW_H = 4, 100, 70
    START_X, START_Y = 35, 40

    for idx, (ref, pin_count, value_label, pins) in enumerate(CONNECTOR_DEFS):
        col = idx % COLS
        row = idx // COLS
        cx = START_X + col * COL_W
        cy = START_Y + row * ROW_H

        conn_sym = f"Conn_01x{pin_count:02d}"
        lib_id = f"Connector_Generic:{conn_sym}"
        j = sb.place(lib_id, ref, value_label, cx, cy)

        for pin_idx, pin_def in enumerate(pins):
            pn = str(pin_idx + 1)
            try:
                ppx, ppy = j.pin(pn)
            except (ValueError, KeyError):
                break

            if pin_def == "NC":
                sb.no_connect(ppx, ppy)
            elif pin_def == "GND":
                sb.gnd(ppx + 8, ppy)
                sb.wire(ppx, ppy, ppx + 8, ppy)
            elif pin_def.startswith("PWR:"):
                pwr_name = pin_def[4:]
                sb.pwr(pwr_name, ppx + 8, ppy)
                sb.wire(ppx, ppy, ppx + 8, ppy)
            elif pin_def.startswith("LABEL:"):
                label_name = pin_def[6:]
                sb.glabel(label_name, ppx + 8, ppy, 0, "bidirectional")
                sb.wire(ppx, ppy, ppx + 8, ppy)

    return sb.build()


def gen_root():
    """Root schematic with 10 hierarchical sheet blocks."""
    reset_refs()
    lines = []
    lines.append(sch_header(ROOT_UUID, "A3"))
    lines.append('\t(lib_symbols\n\t)\n')

    lines.append(text_note("PDCM — Power Distribution & Control Module", 50, 20))
    lines.append(text_note("Black Frog Built — 1998 Chevy Silverado", 50, 30))
    lines.append(text_note("47 TC4427A Channels + 1 DRV8876 H-Bridge", 50, 40))

    sheets = [
        ("Power Input",     "power_input.kicad_sch",     "power_input",     30,  60),
        ("MCU (S32K358)",   "mcu.kicad_sch",             "mcu",             130, 60),
        ("CAN Bus",         "can_bus.kicad_sch",          "can_bus",         230, 60),
        ("Gate Drivers A",  "gate_drivers_a.kicad_sch",   "gate_drivers_a",  330, 60),
        ("Gate Drivers B",  "gate_drivers_b.kicad_sch",   "gate_drivers_b",  30,  120),
        ("Output Stage A",  "output_stage_a.kicad_sch",   "output_stage_a",  130, 120),
        ("Output Stage B",  "output_stage_b.kicad_sch",   "output_stage_b",  230, 120),
        ("H-Bridge",        "hbridge.kicad_sch",          "hbridge",         330, 120),
        ("Switch Inputs",   "switch_inputs.kicad_sch",    "switch_inputs",   30,  180),
        ("Connectors",      "connectors.kicad_sch",       "connectors",      130, 180),
    ]

    for page_num, (title, filename, key, sx, sy) in enumerate(sheets, start=2):
        sheet_uuid = SHEET_UUIDS[key]
        w, h = 80, 40
        lines.append(f'\t(sheet (at {sx} {sy}) (size {w} {h})')
        lines.append(f'\t\t(exclude_from_sim no)')
        lines.append(f'\t\t(in_bom yes)')
        lines.append(f'\t\t(on_board yes)')
        lines.append(f'\t\t(dnp no)')
        lines.append(f'\t\t(stroke (width 0.1524) (type solid))')
        lines.append(f'\t\t(fill (color 0 0 0 0.0000))')
        lines.append(f'\t\t(uuid "{sheet_uuid}")')
        lines.append(f'\t\t(property "Sheetname" "{title}"')
        lines.append(f'\t\t\t(at {sx} {sy - 1} 0)')
        lines.append(f'\t\t\t(effects (font (size 1.524 1.524)) (justify left bottom)))')
        lines.append(f'\t\t(property "Sheetfile" "{filename}"')
        lines.append(f'\t\t\t(at {sx} {sy + h + 1} 0)')
        lines.append(f'\t\t\t(effects (font (size 1.27 1.27)) (justify left top))')
        lines.append(f'\t\t)')
        lines.append(f'\t\t(instances')
        lines.append(f'\t\t\t(project "{PROJECT_NAME}"')
        lines.append(f'\t\t\t\t(path "/{ROOT_UUID}"')
        lines.append(f'\t\t\t\t\t(page "{page_num}")')
        lines.append(f'\t\t\t\t)')
        lines.append(f'\t\t\t)')
        lines.append(f'\t\t)')
        lines.append(f'\t)')

    lines.append(f'\t(sheet_instances')
    lines.append(f'\t\t(path "/"')
    lines.append(f'\t\t\t(page "1"))')
    lines.append(f'\t)')
    lines.append(sch_footer())
    return "\n".join(lines)


# ============================================================
# Project File Generators
# ============================================================
def gen_kicad_pro():
    proj = {
        "board": {"design_settings": {"defaults": {}, "diff_pair_dimensions": [],
                   "drc_exclusions": [], "rules": {}, "track_widths": [],
                   "via_dimensions": []}},
        "boards": [],
        "libraries": {"pinned_footprint_libs": [], "pinned_symbol_libs": ["PDCM"]},
        "meta": {"filename": "PDCM.kicad_pro", "version": 1},
        "net_settings": {"classes": [], "meta": {"version": 0}},
        "pcbnew": {"page_layout_descr_file": ""},
        "schematic": {"drawing": {"default_bus_thickness": 12,
                      "default_line_thickness": 6, "default_text_size": 50}},
        "sheets": [], "text_variables": {}
    }
    return json.dumps(proj, indent=2) + "\n"

def gen_sym_lib_table():
    return ('(sym_lib_table\n  (version 7)\n'
            '  (lib (name "PDCM")(type "KiCad")'
            '(uri "${KIPRJMOD}/PDCM.kicad_sym")(options "")'
            '(descr "PDCM custom symbols"))\n)\n')

def gen_fp_lib_table():
    return "(fp_lib_table\n  (version 7)\n)\n"


# ============================================================
# Main
# ============================================================
def write_file(filename, content):
    path = os.path.join(OUTPUT_DIR, filename)
    with open(path, 'w', encoding='utf-8', newline='\n') as f:
        f.write(content)
    print(f"  Generated: {filename} ({len(content)} bytes)")

def main():
    print(f"PDCM Schematic Generator")
    print(f"Output directory: {OUTPUT_DIR}")
    print(f"KiCad library: {KICAD_LIB_DIR}")
    print()

    if not os.path.isdir(KICAD_LIB_DIR):
        print(f"ERROR: KiCad symbol library not found at {KICAD_LIB_DIR}")
        return 1

    print("Generating files...")

    # Project files
    write_file("PDCM.kicad_pro", gen_kicad_pro())
    write_file("sym-lib-table", gen_sym_lib_table())
    write_file("fp-lib-table", gen_fp_lib_table())
    write_file("PDCM.kicad_sym", gen_custom_sym_lib())

    # Sub-sheets
    write_file("power_input.kicad_sch", gen_power_input())
    write_file("can_bus.kicad_sch", gen_can_bus())
    write_file("mcu.kicad_sch", gen_mcu())
    write_file("gate_drivers_a.kicad_sch", gen_gate_drivers_a())
    write_file("gate_drivers_b.kicad_sch", gen_gate_drivers_b())
    write_file("output_stage_a.kicad_sch", gen_output_stage_a())
    write_file("output_stage_b.kicad_sch", gen_output_stage_b())
    write_file("hbridge.kicad_sch", gen_hbridge())
    write_file("switch_inputs.kicad_sch", gen_switch_inputs())
    write_file("connectors.kicad_sch", gen_connectors())

    # Root schematic (last)
    write_file("PDCM.kicad_sch", gen_root())

    print()
    print("Done! Open PDCM.kicad_pro in KiCad 9 to view.")
    return 0

if __name__ == "__main__":
    exit(main())
