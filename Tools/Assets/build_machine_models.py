"""
Massive quality overhaul: industrial machines, conveyors and mesh boxes as modelled objects (Geometry Script, own
models) instead of painted boxes. Each model fills exactly its furniture footprint (X depth, Y width, Z height; origin at
the bottom centre; the furniture actor fits the model into its box, so the measures matter):

  SM_Gen_M_Shredder 110x110x150   SM_Gen_M_Sorter 100x180x140      SM_Gen_M_Press 120x100x170
  SM_Gen_M_Assembler 120x160x160  SM_Gen_M_Packer 160x200x180      SM_Gen_M_Furnace 200x200x260
  SM_Gen_M_Refinery 180x220x230   SM_Gen_M_Conveyor 100x50x45      SM_Gen_M_ConveyorLong 200x50x45
  SM_Gen_M_MaterialStore 100x120x80                                SM_Gen_M_BigMaterialStore 200x240x100

Common parts: steel skid on feet, bevelled cabinets with panel seams, vents, doors with hinges and handles, control
panel (buttons, emergency stop, display), signal tower, motors with cooling fins, hazard edging, cable conduits.

  UnrealEditor-Cmd.exe <uproject> -ExecutePythonScript="<this file>" -unattended -RenderOffscreen -nosplash -nosteam
"""
import builtins
import math
import os
import sys

import unreal

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
builtins.WE_MODELS_LIBRARY = True
import build_models as bm  # noqa: E402

Model = bm.Model


# --- Building blocks ----------------------------------------------------------------------------------------
def skid(m, D, W, h=10.0):
    """Steel base frame on four levelling feet; returns its top height."""
    for y in (-W / 2 + 4, W / 2 - 4):
        m.box("MetalPaintedGrey", (0, y, h - 4), (D - 2, 8, 8), bevel=0.4)
    for x in (-D / 2 + 4, D / 2 - 4):
        m.box("MetalPaintedGrey", (x, 0, h - 4), (8, W - 2, 8), bevel=0.4)
    for x in (-D / 2 + 6, D / 2 - 6):
        for y in (-W / 2 + 6, W / 2 - 6):
            m.cyl("SteelBrushed", (x, y, 1.0), 3.0, 2.0, segs=12)
            m.cyl("SteelBrushed", (x, y, 3.0), 0.8, 3.0, segs=8)
    # hazard edging along the front
    n = int((W - 10) // 10)
    for i in range(n):
        m.box("PlasticYellow" if i % 2 == 0 else "TrimBlack", (D / 2 - 0.3, -W / 2 + 5 + (i + 0.5) * (W - 10) / n, h - 4), (0.6, (W - 10) / n, 5.0))
    return h


def cabinet(m, paint, c, s, doors=1, vents=True, door_side="+X"):
    """Bevelled cabinet with seams, a door (hinges, handle) and vent slots on the front face."""
    m.box(paint, c, s, bevel=1.4, segments=2)
    fx = c[0] + s[0] / 2 + 0.15
    if door_side == "+X":
        dw = (s[1] - 10) / max(doors, 1)
        for d in range(doors):
            dy = c[1] - s[1] / 2 + 5 + dw * (d + 0.5)
            m.box("TrimBlack", (fx, dy, c[2]), (0.3, dw - 2, s[2] - 12))           # door gap
            m.box(paint, (fx + 0.3, dy, c[2]), (0.6, dw - 3, s[2] - 13), bevel=0.3)  # door leaf
            m.box("Chrome", (fx + 1.2, dy + dw / 2 - 6, c[2]), (1.2, 2.0, 10.0), bevel=0.4)  # handle
            for z in (c[2] - s[2] / 2 + 14, c[2] + s[2] / 2 - 14):
                m.cyl("SteelBrushed", (fx + 0.6, dy - dw / 2 + 3, z), 0.8, 5.0, segs=10)  # hinges
            if vents:
                for k in range(5):
                    m.box("TrimBlack", (fx + 0.7, dy, c[2] - s[2] / 2 + 14 + k * 2.6), (0.3, dw * 0.5, 1.0))
    # warning label
    m.box("PlasticYellow", (fx + 0.7, c[1] - s[1] / 2 + 9, c[2] + s[2] / 2 - 10), (0.2, 7.0, 6.0))
    m.box("TrimBlack", (fx + 0.8, c[1] - s[1] / 2 + 9, c[2] + s[2] / 2 - 10), (0.2, 4.0, 0.6), rot=(0, 0, 45))


def control_panel(m, c, facing=1.0):
    """Upright control box: display, start (green), stop (red), selector and an emergency stop; c = centre of its back."""
    x, y, z = c
    m.box("MachineGrey", (x + facing * 5, y, z), (10, 26, 34), bevel=0.8, segments=2)
    fx = x + facing * 10.2
    m.box("ScreenDark", (fx, y, z + 8), (0.4, 16, 9))
    m.box("LedGreen", (fx + facing * 0.1, y - 6, z + 11), (0.3, 1.0, 1.0))
    for dy, mat in ((-6, "LedGreen"), (0, "PlasticRed"), (6, "PlasticGrey")):
        m.cyl(mat, (fx + facing * 0.8, y + dy, z - 4), 1.6, 1.6, rot=(0, 90, 0), segs=16)
    m.cyl("PlasticYellow", (fx + facing * 0.3, y, z - 12), 4.2, 0.6, rot=(0, 90, 0), segs=20)
    m.cyl("PlasticRed", (fx + facing * 1.6, y, z - 12), 2.6, 2.4, rot=(0, 90, 0), segs=20)  # emergency stop
    m.box("SteelBrushed", (x + facing * 2, y, z - 26), (3, 3, 18))  # stand pipe to the machine
    return fx


def signal_tower(m, c):
    x, y, z = c
    m.cyl("SteelBrushed", (x, y, z + 6), 1.0, 12, segs=10)
    for i, mat in enumerate(("LedGreen", "LedAmber", "LedRed")):
        m.cyl(mat, (x, y, z + 14 + i * 5.2), 3.0, 5.0, segs=16)
    m.cyl("TrimBlack", (x, y, z + 29.6), 3.1, 0.8, segs=16)


def motor(m, c, length, r, axis="Y"):
    """Electric motor with fins and a terminal box; axis Y or X."""
    rot = (90, 0, 0) if axis == "Y" else (0, 90, 0)
    m.cyl("MachineBlue", c, r, length, rot=rot, segs=24, bevel=0.0)
    for k in range(10):
        a = k * 36.0
        dx = math.cos(math.radians(a)) * r
        dz = math.sin(math.radians(a)) * r
        if axis == "Y":
            m.box("MachineBlue", (c[0] + dx, c[1], c[2] + dz), (1.0, length * 0.8, 2.4), rot=(0, -a, 0))
        else:
            m.box("MachineBlue", (c[0], c[1] + dx, c[2] + dz), (length * 0.8, 1.0, 2.4), rot=(a, 0, 0))
    m.box("MachineBlue", (c[0], c[1], c[2] + r + 3), (10, 10, 6), bevel=0.6)  # terminal box
    end = (c[0], c[1] + (length / 2 + 1) * (1 if axis == "Y" else 0), c[2]) if axis == "Y" else (c[0] + length / 2 + 1, c[1], c[2])
    m.cyl("TrimBlack", end, r * 0.8, 2.0, rot=rot, segs=20)  # fan cover


def conduit(m, a, b, r=1.2):
    """Straight cable conduit between two points (only along one axis)."""
    d = [b[i] - a[i] for i in range(3)]
    length = math.sqrt(sum(v * v for v in d))
    mid = tuple((a[i] + b[i]) / 2 for i in range(3))
    if abs(d[2]) >= max(abs(d[0]), abs(d[1])):
        m.cyl("TrimBlack", mid, r, length, segs=10)
    elif abs(d[0]) >= abs(d[1]):
        m.cyl("TrimBlack", mid, r, length, rot=(0, 90, 0), segs=10)
    else:
        m.cyl("TrimBlack", mid, r, length, rot=(90, 0, 0), segs=10)


def hopper(m, paint, c, top, bottom, h):
    """Square funnel (frustum with four sides) with a dark mouth; c = centre of its base."""
    m.cone(paint, (c[0], c[1], c[2] + h / 2), bottom * 0.7071, top * 0.7071, h, rot=(0, 0, 45), segs=4)
    m.box("TrimBlack", (c[0], c[1], c[2] + h - 1.2), (top - 8, top - 8, 1.0))
    m.box("SteelBrushed", (c[0], c[1], c[2] + h + 0.8), (top + 2, top + 2, 1.6), cuts=[Model.cut_box((c[0], c[1], c[2] + h + 0.8), (top - 6, top - 6, 4))])


# --- Machines ---------------------------------------------------------------------------------------------
def shredder():
    m = Model("M_Shredder")
    D, W, H = 110.0, 110.0, 150.0
    paint = "MachineYellow"
    z = skid(m, D, W)
    cabinet(m, paint, (0, -8, z + 42), (D - 14, W - 30, 84), doors=1)
    m.box("MachineGrey", (0, -8, z + 88), (D - 20, W - 36, 8), bevel=0.8)  # cutting chamber
    hopper(m, paint, (0, -8, z + 92), 100.0, 60.0, H - z - 94)
    for yy in (-14.0, -2.0):  # the two cutter shafts, seen through the mouth
        m.cyl("SteelBrushed", (0, yy - 8 + 8, z + 95), 4.5, 54.0, rot=(0, 90, 0), segs=16)
    motor(m, (-10, W / 2 - 13, z + 30), 22.0, 11.0, axis="Y")
    m.box(paint, (12, W / 2 - 13, z + 62), (30, 22, 30), bevel=1.0)  # gearbox
    m.box("MachineGrey", (D / 2 - 10, -8, z + 10), (12, 40, 20), bevel=0.6)  # discharge chute
    m.box("TrimBlack", (D / 2 - 3.8, -8, z + 8), (0.4, 34, 14))
    control_panel(m, (D / 2 - 13, W / 2 - 14, z + 80))
    signal_tower(m, (-D / 2 + 8, W / 2 - 8, z + 84))
    conduit(m, (-D / 2 + 8, W / 2 - 8, z + 2), (-D / 2 + 8, W / 2 - 8, z + 84))
    return m


def sorter():
    m = Model("M_Sorter")
    D, W, H = 100.0, 180.0, 140.0
    paint = "MachineTeal"
    z = skid(m, D, W)
    cabinet(m, paint, (-10, 0, z + 38), (D - 30, W - 10, 76), doors=3)
    # vibrating screen deck on top, sloping to the front, with side walls
    m.box("MachineGrey", (-6, 0, z + 82), (D - 34, W - 12, 6), rot=(0, -6, 0), bevel=0.6)
    for y in (-W / 2 + 8, W / 2 - 8):
        m.box(paint, (-6, y, z + 92), (D - 34, 3, 18), rot=(0, -6, 0), bevel=0.4)
    for i in range(9):
        m.box("SteelBrushed", (-6 - 30 + i * 7.5, 0, z + 86), (0.8, W - 18, 0.8), rot=(0, -6, 0))
    hopper(m, paint, (-D / 2 + 18, 0, z + 96), 46.0, 30.0, H - z - 98)
    # three discharge chutes into bins at the front
    for i, y in enumerate((-55.0, 0.0, 55.0)):
        m.box("MachineGrey", (D / 2 - 20, y, z + 56), (16, 30, 8), rot=(0, 30, 0), bevel=0.4)
        m.box("ContainerBlue" if i == 1 else "PlasticGrey", (D / 2 - 12, y, z + 18), (22, 38, 34), bevel=1.0,
              cuts=[Model.cut_box((D / 2 - 12, y, z + 25), (18, 34, 34))])
    motor(m, (-D / 2 + 10, 0, z + 20), 30.0, 10.0, axis="Y")
    control_panel(m, (D / 2 - 32, -W / 2 + 18, z + 92))
    signal_tower(m, (-D / 2 + 6, W / 2 - 8, z + 76))
    return m


def press():
    m = Model("M_Press")
    D, W, H = 120.0, 100.0, 170.0
    paint = "MachineRed"
    z = skid(m, D, W)
    # press box with a front door, two columns, head with the hydraulic cylinder
    cabinet(m, paint, (0, 0, z + 34), (D - 20, W - 10, 68), doors=1, vents=False)
    for y in (-W / 2 + 10, W / 2 - 10):
        for x in (-D / 2 + 16, D / 2 - 26):
            m.box(paint, (x, y, z + 92), (12, 12, 50), bevel=0.8)
    m.box(paint, (-5, 0, z + 124), (D - 18, W - 6, 22), bevel=1.2, segments=2)  # head
    m.cyl("MachineGrey", (-5, 0, z + 141), 16, 12, segs=24)  # hydraulic cylinder on the head
    m.cyl("Chrome", (-5, 0, z + 98), 7.0, 32, segs=20)  # ram
    m.box("SteelBrushed", (-5, 0, z + 80), (60, 60, 4), bevel=0.4)  # platen
    # hydraulic unit at the back: tank, motor, hoses
    m.box("MachineGrey", (-D / 2 + 6, 0, z + 24), (12, 60, 40), bevel=0.8)
    motor(m, (-D / 2 + 6, 0, z + 52), 22.0, 8.0, axis="Y")
    for y in (-8.0, 8.0):
        conduit(m, (-D / 2 + 10, y, z + 44), (-D / 2 + 10, y, z + 124), r=1.4)
    control_panel(m, (D / 2 - 13, W / 2 - 14, z + 92))
    signal_tower(m, (D / 2 - 26, -W / 2 + 10, z + 117))
    return m


def assembler():
    m = Model("M_Assembler")
    D, W, H = 120.0, 160.0, 160.0
    paint = "MachineBlue"
    z = skid(m, D, W)
    m.box(paint, (0, 0, z + 20), (D - 6, W - 6, 40), bevel=1.2, segments=2)  # machine table
    # guarded cell: posts, rails, clear panels
    top = H - 8
    for x in (-D / 2 + 4, D / 2 - 4):
        for y in (-W / 2 + 4, W / 2 - 4):
            m.box("MachineYellow", (x, y, (z + 40 + top) / 2), (5, 5, top - z - 40))
    for x in (-D / 2 + 4, D / 2 - 4):
        m.box("MachineYellow", (x, 0, top), (5, W - 4, 5))
        m.box("GlassClear", (x, 0, (z + 44 + top) / 2), (0.8, W - 12, top - z - 50))
    for y in (-W / 2 + 4, W / 2 - 4):
        m.box("MachineYellow", (0, y, top), (D - 4, 5, 5))
        m.box("GlassClear", (0, y, (z + 44 + top) / 2), (D - 12, 0.8, top - z - 50))
    m.box("MachineGrey", (0, 0, H - 3), (D - 4, W - 4, 6), bevel=0.6)  # roof with the cable tray
    # robot arm inside: base, turret, two links, gripper
    m.cyl("MachineOrange", (0, -20, z + 46), 14, 12, segs=24)
    m.cyl("MachineOrange", (0, -20, z + 58), 9, 12, segs=20)
    m.box("MachineOrange", (8, -20, z + 76), (8, 10, 34), rot=(0, 25, 0), bevel=1.0)
    m.box("MachineOrange", (22, -20, z + 90), (30, 8, 8), rot=(0, -15, 0), bevel=1.0)
    m.box("SteelBrushed", (37, -20, z + 84), (4, 10, 10))
    # parts tray and a feeder
    m.box("PlasticGrey", (14, 36, z + 44), (40, 40, 6), bevel=0.6)
    m.box("MachineGrey", (-30, 46, z + 54), (24, 24, 30), bevel=0.8)
    control_panel(m, (D / 2 - 13, -W / 2 + 22, z + 60))
    signal_tower(m, (-D / 2 + 8, -W / 2 + 8, z + 40))
    return m


def packer():
    m = Model("M_Packer")
    D, W, H = 160.0, 200.0, 180.0
    z = skid(m, D, W, 8.0)
    # turntable with a pallet and a wrapped stack, mast with the film carriage
    m.cyl("MachineGrey", (-10, -10, z + 4), 70, 8, segs=40)
    m.cyl("SteelBrushed", (-10, -10, z + 8.5), 66, 1.0, segs=40)
    for x in (-50, -10, 30):
        m.box("Wood", (x, -10, z + 13), (10, 80, 8))
    for i in range(5):
        m.box("Wood", (-10, -45 + i * 17.5, z + 18), (110, 9, 2))
    m.box("Cardboard", (-10, -10, z + 52), (100, 74, 64), bevel=1.0)
    m.box("GlassClear", (-10, -10, z + 52), (102, 76, 56))  # stretch film
    m.box("MachineGreen", (D / 2 - 14, W / 2 - 20, H / 2), (20, 20, H - 4), bevel=1.0)  # mast
    m.box("MachineGreen", (D / 2 - 28, W / 2 - 20, z + 70), (16, 26, 30), bevel=1.0)  # carriage
    m.cyl("PlasticWhite", (D / 2 - 38, W / 2 - 20, z + 70), 6, 26, segs=20)  # film roll
    m.box("MachineGreen", (D / 2 - 14, -W / 2 + 22, z + 50), (24, 34, 100), bevel=1.2, segments=2)  # control cabinet
    control_panel(m, (D / 2 - 13, -W / 2 + 22, z + 74))
    signal_tower(m, (D / 2 - 14, W / 2 - 20, H - 32))
    conduit(m, (D / 2 - 14, -W / 2 + 40, z + 6), (D / 2 - 14, W / 2 - 30, z + 6))
    return m


def furnace():
    m = Model("M_Furnace")
    D, W, H = 200.0, 200.0, 260.0
    z = skid(m, D, W, 12.0)
    m.box("MachineBlue", (0, 0, z + 72), (D - 20, W - 20, 144), bevel=2.0, segments=2)  # steel jacket
    for y in (-W / 2 + 10, W / 2 - 10):  # bracing bands
        for zz in (z + 20, z + 72, z + 124):
            m.box("MachineGrey", (0, y, zz), (D - 16, 4, 8), bevel=0.4)
    # charging door with refractory lining and the glowing gap
    fx = D / 2 - 14
    m.box("Refractory", (fx + 1, 0, z + 70), (4, 90, 70), bevel=1.0)
    m.box("FurnaceGlow", (fx + 3.2, 0, z + 70), (0.4, 80, 3))
    m.box("MachineGrey", (fx + 5, 0, z + 82), (6, 96, 36), bevel=1.0)
    m.cyl("SteelBrushed", (fx + 9, 0, z + 82), 1.6, 70, rot=(90, 0, 0), segs=12)  # handle bar
    # tilt spout and a ladle
    m.box("Refractory", (fx + 8, W / 2 - 40, z + 30), (18, 18, 8), rot=(0, -20, 0))
    # hood and flue
    m.box("MachineGrey", (0, 0, z + 154), (D - 50, W - 50, 20), bevel=1.0)
    m.cone("MachineGrey", (0, 0, z + 178), 60, 22, 28, segs=24)
    m.cyl("MachineGrey", (0, 0, (z + 192 + H) / 2), 18, H - z - 192, segs=24)
    m.torus("SteelBrushed", (0, 0, H - 30), 18.5, 1.4, segs=24, minor=6)
    # burner with gas pipes
    m.cyl("MachineRed", (-D / 2 + 10, W / 2 - 30, z + 40), 10, 26, rot=(0, 90, 0), segs=20)
    conduit(m, (-D / 2 + 4, W / 2 - 30, z + 40), (-D / 2 + 4, W / 2 - 30, z + 140), r=2.2)
    control_panel(m, (D / 2 - 13, -W / 2 + 26, z + 100))
    signal_tower(m, (-D / 2 + 14, -W / 2 + 14, z + 144))
    return m


def refinery():
    m = Model("M_Refinery")
    D, W, H = 180.0, 220.0, 230.0
    z = skid(m, D, W, 10.0)
    m.box("MachineGrey", (0, 0, z + 2), (D - 6, W - 6, 4))  # drip tray
    # reactor vessel with a domed top, two tanks, piping, a platform with railing
    m.cyl("SteelBrushed", (-20, -40, z + 70), 42, 130, segs=36)
    m.sphere("SteelBrushed", (-20, -40, z + 135), 42, scale=(1, 1, 0.45), steps=20)
    for zz in (z + 25, z + 70, z + 115):
        m.torus("MachineGrey", (-20, -40, zz), 42.5, 1.6, segs=36, minor=6)
    for y in (45.0, 85.0):
        m.cyl("MachineGreen", (-45, y, z + 55), 18, 100, segs=28)
        m.sphere("MachineGreen", (-45, y, z + 105), 18, scale=(1, 1, 0.5), steps=16)
    for i, zz in enumerate((z + 40, z + 90)):
        conduit(m, (-45, 45, zz), (-20, 45, zz), r=2.4)
        conduit(m, (-20, 45, zz), (-20, 2, zz), r=2.4)
    conduit(m, (-20, -40, z + 152), (-20, -40, H - 8), r=3.0)
    conduit(m, (-20, -40, H - 8), (60, -40, H - 8), r=3.0)
    conduit(m, (60, -40, H - 8), (60, -40, z + 60), r=3.0)
    # control cabinet and filter press at the front
    cabinet(m, "MachineGreen", (60, 50, z + 55), (50, 90, 110), doors=2)
    m.box("PlasticGrey", (60, -40, z + 30), (40, 50, 60), bevel=1.0)
    for i in range(6):
        m.box("PlasticWhite", (60, -40 - 20 + i * 8, z + 32), (42, 2, 50))
    control_panel(m, (D / 2 - 13, -W / 2 + 26, z + 80))
    signal_tower(m, (60, 50, z + 110))
    return m


def conveyor(name, L):
    m = Model(name)
    D, W, H = L, 50.0, 45.0
    belt_top = H - 3.0  # the guide rails reach the footprint height
    # frame: side members, legs with cross braces, rollers, belt, end drums, guide rails
    for y in (-W / 2 + 2, W / 2 - 2):
        m.box("MetalPaintedGrey", (0, y, belt_top - 6), (D, 4, 10), bevel=0.4)
        m.box("SteelBrushed", (0, y, belt_top + 2.5), (D, 1.2, 5))  # guide rail
    legs = max(2, int(L // 90) + 1)
    for i in range(legs):
        x = -D / 2 + 8 + i * (D - 16) / (legs - 1)
        for y in (-W / 2 + 4, W / 2 - 4):
            m.box("MetalPaintedGrey", (x, y, (belt_top - 11) / 2), (5, 5, belt_top - 11))
            m.cyl("Rubber", (x, y, 1.0), 3.2, 2.0, segs=12)
        m.box("MetalPaintedGrey", (x, 0, 12), (4, W - 8, 4))
    for i in range(int(L // 12)):
        x = -D / 2 + 6 + i * 12
        m.cyl("SteelBrushed", (x, 0, belt_top - 4.5), 2.4, W - 10, rot=(90, 0, 0), segs=12)
    m.box("Rubber", (0, 0, belt_top - 1.2), (D - 6, W - 12, 2.4))
    for x in (-D / 2 + 3, D / 2 - 3):
        m.cyl("SteelBrushed", (x, 0, belt_top - 3.5), 3.5, W - 9, rot=(90, 0, 0), segs=16)
    motor(m, (-D / 2 + 12, W / 2 + 0.0 - 12, 18), 14.0, 6.0, axis="Y")
    # direction arrows on the side
    for y in (-W / 2 + 0.1, W / 2 - 0.1):
        m.box("PlasticYellow", (D / 2 - 18, y, belt_top - 6), (10, 0.3, 2))
    return m


def mesh_box(name, D, W, H):
    """Steel mesh box (Gitterbox): frame, wire grid sides, solid floor, feet; open top."""
    m = Model(name)
    feet = 12.0
    for x in (-D / 2 + 6, D / 2 - 6):
        for y in (-W / 2 + 6, W / 2 - 6):
            m.box("MetalPaintedGrey", (x, y, feet / 2), (10, 10, feet))
    m.box("SteelBrushed", (0, 0, feet + 1.5), (D, W, 3))
    for x in (-D / 2 + 1.5, D / 2 - 1.5):
        for y in (-W / 2 + 1.5, W / 2 - 1.5):
            m.box("MetalPaintedGrey", (x, y, (feet + H) / 2), (3, 3, H - feet))
    for z in (H - 1.5, feet + 3):
        for x in (-D / 2 + 1.5, D / 2 - 1.5):
            m.box("MetalPaintedGrey", (x, 0, z), (3, W, 3))
        for y in (-W / 2 + 1.5, W / 2 - 1.5):
            m.box("MetalPaintedGrey", (0, y, z), (D, 3, 3))
    step = 10.0
    for x in (-D / 2 + 1.0, D / 2 - 1.0):
        for k in range(1, int(W // step)):
            m.box("SteelBrushed", (x, -W / 2 + k * step, (feet + H) / 2), (0.6, 0.6, H - feet - 3))
        for k in range(1, int((H - feet) // step)):
            m.box("SteelBrushed", (x, 0, feet + k * step), (0.6, W - 3, 0.6))
    for y in (-W / 2 + 1.0, W / 2 - 1.0):
        for k in range(1, int(D // step)):
            m.box("SteelBrushed", (-D / 2 + k * step, y, (feet + H) / 2), (0.6, 0.6, H - feet - 3))
        for k in range(1, int((H - feet) // step)):
            m.box("SteelBrushed", (0, y, feet + k * step), (D - 3, 0.6, 0.6))
    m.box("PlasticWhite", (D / 2 + 0.2, 0, H - 14), (0.3, 18, 10))  # label holder
    return m


BUILDERS = {
    "M_Shredder": shredder, "M_Sorter": sorter, "M_Press": press, "M_Assembler": assembler, "M_Packer": packer,
    "M_Furnace": furnace, "M_Refinery": refinery,
    "M_Conveyor": lambda: conveyor("M_Conveyor", 100.0), "M_ConveyorLong": lambda: conveyor("M_ConveyorLong", 200.0),
    "M_MaterialStore": lambda: mesh_box("M_MaterialStore", 100.0, 120.0, 80.0),
    "M_BigMaterialStore": lambda: mesh_box("M_BigMaterialStore", 200.0, 240.0, 100.0),
}


def main():
    only = None
    for arg in unreal.SystemLibrary.get_command_line().split():
        if arg.startswith("-WEModels="):
            only = set(arg.split("=", 1)[1].split(","))
    built, failed = 0, []
    for name, make in BUILDERS.items():
        if only and name not in only:
            continue
        try:
            if make().save():
                built += 1
            else:
                failed.append(name)
        except Exception as e:
            failed.append(name)
            unreal.log_error(f"WE_MODEL {name} failed: {e}")
    unreal.log(f"WE_MACHINE_MODELS_BUILT {built} failed={failed}")


main()
unreal.SystemLibrary.quit_editor()
