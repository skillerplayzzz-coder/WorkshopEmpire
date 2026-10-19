"""
Phase 19: the warehouse forklift as a real (compact) counterbalance forklift instead of yellow boxes.
Built in forklift space (cm): forks pointing to +X, ground z = 0, about centred on x = 0 so it fits the
AWEForklift footprint (210 x 110 x 210). The driver stands at x = -45 (AWEForklift::GetSeatOffset); the mast
rails sit at the sides and the roof is an open grid, so the driver can see past the mast and up.
  SM_Gen_Forklift

  UnrealEditor-Cmd.exe <uproject> -ExecutePythonScript="<this file>" -unattended -RenderOffscreen -nosplash -nosteam
"""
import builtins
import os
import sys

import unreal

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
builtins.WE_MODELS_LIBRARY = True
import build_models as bm  # noqa: E402

Model = bm.Model

PAINT = "PlasticYellow"
DARK = "MetalPaintedGrey"
FRAME = "TrimBlack"


def wheel(m, x, y, r, w):
    m.cyl("TireRubber", (x, y, r), r, w, rot=(90, 0, 0), segs=28, bevel=1.5)
    m.cyl("SteelBrushed", (x, y + (w * 0.5 + 0.4) * (1 if y > 0 else -1), r), r * 0.55, 1.2, rot=(90, 0, 0), segs=20)
    m.cyl(DARK, (x, y + (w * 0.5 + 0.8) * (1 if y > 0 else -1), r), r * 0.18, 1.6, rot=(90, 0, 0), segs=12)


def bar(m, mat, a, b, t):
    """A square tube from a to b (only along one axis or tilted in XZ)."""
    import math
    dx, dy, dz = b[0] - a[0], b[1] - a[1], b[2] - a[2]
    length = math.sqrt(dx * dx + dy * dy + dz * dz)
    c = ((a[0] + b[0]) * 0.5, (a[1] + b[1]) * 0.5, (a[2] + b[2]) * 0.5)
    pitch = math.degrees(math.atan2(dx, dz)) if abs(dy) < 1e-3 else 0.0
    yaw = 90.0 if abs(dy) > 1e-3 and abs(dx) < 1e-3 and abs(dz) < 1e-3 else 0.0
    if abs(dz) < 1e-3 and abs(dy) < 1e-3:
        m.box(mat, c, (length, t, t), bevel=0.4)
    elif yaw:
        m.box(mat, c, (t, length, t), bevel=0.4)
    else:
        m.box(mat, c, (t, t, length), rot=(0, pitch, 0), bevel=0.4)


def forklift():
    m = Model("Forklift")
    # Chassis and counterweight (the heavy rounded back).
    m.box(PAINT, (-30, 0, 45), (130, 100, 50), bevel=4)
    m.box(DARK, (-88, 0, 72), (34, 104, 66), bevel=8)
    m.box(DARK, (-100, 0, 40), (12, 90, 30), bevel=3)                      # tow hitch block
    m.box(PAINT, (-30, 0, 72), (70, 96, 6), bevel=2)                       # floor plate around the seat
    # Engine cover / hood with the seat on it, side steps.
    m.box(PAINT, (-60, 0, 86), (40, 70, 22), bevel=5)
    m.box("PlasticBlack", (-55, 0, 103), (30, 44, 10), bevel=4)            # seat cushion
    m.box("PlasticBlack", (-71, 0, 122), (8, 44, 36), rot=(0, -10, 0), bevel=3)  # backrest
    for s in (-1, 1):
        m.box(FRAME, (-20, s * 52, 30), (40, 6, 4), bevel=1)               # step
    # Dashboard and steering wheel in front of the driver.
    m.box(PAINT, (12, 0, 88), (26, 92, 34), bevel=4)
    m.cyl(FRAME, (2, 0, 112), 3, 34, rot=(0, -35, 0), segs=12)             # steering column
    m.torus("PlasticBlack", (-7, 0, 127), 13, 1.6, rot=(0, -35, 0), segs=28, minor=8)
    m.box("PlasticBlack", (18, 18, 106), (6, 6, 10), bevel=1)               # lift levers
    m.box("PlasticBlack", (18, 26, 106), (6, 6, 10), bevel=1)
    # Wheels (big drive wheels under the mast, small steering wheels under the counterweight).
    for s in (-1, 1):
        wheel(m, 18, s * 45, 24, 18)
        wheel(m, -78, s * 41, 18, 14)
        m.box(PAINT, (18, s * 45, 50), (54, 22, 6), bevel=2)                # mudguard
    # Overhead guard: four slim posts and an open roof grid (the driver looks through it).
    for s in (-1, 1):
        m.box(FRAME, (26, s * 46, 148), (5, 5, 116), bevel=0.5)
        m.box(FRAME, (-80, s * 46, 155), (5, 5, 100), bevel=0.5)
        m.box(FRAME, (-27, s * 46, 205), (112, 5, 5), bevel=0.5)
    for x in (26, -80):
        m.box(FRAME, (x, 0, 205), (5, 97, 5), bevel=0.5)
    for y in (-27, -9, 9, 27):
        m.box(FRAME, (-28, y, 205), (104, 3, 2), bevel=0.3)
    m.box("LedAmber", (-60, 0, 210), (9, 9, 6), bevel=2)                    # beacon
    m.box("Headlight", (26, -46, 196), (4, 8, 6), bevel=1)
    m.box("Headlight", (26, 46, 196), (4, 8, 6), bevel=1)
    # Mast: two outer rails at the sides, an inner pair, cross members, lift cylinders behind the rails.
    for s in (-1, 1):
        m.box(DARK, (46, s * 36, 104), (9, 7, 196), bevel=0.8)
        m.box(DARK, (50, s * 29, 110), (6, 6, 196), bevel=0.6)
        m.cyl("SteelBrushed", (41, s * 40, 70), 3, 120, segs=12)
    m.box(DARK, (47, 0, 200), (10, 80, 7), bevel=0.8)
    m.box(DARK, (47, 0, 22), (10, 80, 8), bevel=0.8)
    # Carriage with a load backrest grid, and two fork tines.
    m.box(DARK, (56, 0, 30), (6, 80, 26), bevel=0.8)
    m.box(DARK, (56, 0, 72), (4, 80, 4), bevel=0.4)
    m.box(DARK, (56, 0, 110), (4, 80, 4), bevel=0.4)
    for y in (-38, -19, 0, 19, 38):
        m.box(DARK, (56, y, 91), (3, 3, 40), bevel=0.3)
    for s in (-1, 1):
        m.box("SteelBrushed", (60, s * 22, 26), (5, 11, 40), bevel=0.5)    # fork shank
        m.box("SteelBrushed", (82, s * 22, 5), (48, 11, 4), bevel=0.5)     # fork blade
        m.box("SteelBrushed", (105, s * 22, 4), (4, 11, 2.5), bevel=0.5)   # tapered tip
    # Warning stripes on the counterweight.
    for i in range(5):
        m.box("PlasticBlack", (-105.2, -36 + i * 18, 72), (0.6, 7, 40), rot=(35, 0, 0))
    m.box("Taillight", (-105.3, -40, 98), (0.8, 8, 5), bevel=0.5)
    m.box("Taillight", (-105.3, 40, 98), (0.8, 8, 5), bevel=0.5)
    return m


ok = forklift().save(normalize=False)
unreal.log(f"WE_FORKLIFT_BUILT {1 if ok else 0}")
unreal.SystemLibrary.quit_editor()
