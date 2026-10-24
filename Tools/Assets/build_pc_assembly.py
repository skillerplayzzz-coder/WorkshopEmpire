"""
Phase 18a: the old office PC as a real assembly. Every part is its own model at its mounting position in the
device's space (Tools/Assets/pc_assembly_layout.py), so all parts at identity form the complete PC:
  /Game/WorkshopEmpire/Art/Generated/PCA/SM_Gen_PCA_<PartId>   parts for the workbench (AWEAssemblyView)
  /Game/WorkshopEmpire/Art/Generated/SM_Gen_Fastener<Kind>     screw, thumbscrew, plug, clip, lever (+Z = out)
  /Game/WorkshopEmpire/Art/Generated/SM_Gen_<Item>             loose items made from the same parts (lying flat):
      OldComputer (all parts), GraphicsCard, RamModule, CpuCooler, Processor, PowerSupply, Mainboard, HardDrive,
      SsdDrive, CaseFan
  /Game/WorkshopEmpire/Materials/FX/M_WEHighlightHover, M_WEHighlightSelect (overlay outlines in the work view)

Run after build_prop_material.py and before build_item_definitions.py:
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
import pc_assembly_layout as L  # noqa: E402

Model = bm.Model
P = bm.P
PCA = "/Game/WorkshopEmpire/Art/Generated/PCA"
FX = "/Game/WorkshopEmpire/Materials/FX"
RAM_Y = L.RAM_Y
mtx = unreal.GeometryScript_MeshTransforms


def seg(m, mat, a, b, r, segs=8):
    """Cylinder (cable, rod) from point a to point b."""
    a, b = unreal.Vector(*a), unreal.Vector(*b)
    d = b - a
    length = d.length()
    if length < 0.01:
        return
    part = unreal.DynamicMesh()
    bm.CURRENT_ID[0] = m._mid(mat)
    rot = unreal.MathLibrary.make_rot_from_z(d)
    t = unreal.Transform((a + b) * 0.5, rot, unreal.Vector(1, 1, 1))
    P.append_cylinder(part, bm.popts(bm.CURRENT_ID[0]), t, r, length, segs, 0, True, bm.CENTER)
    m._finish(part, mat)


def cable(m, mats, pts, r=0.22, spread=0.5):
    """Bundle of parallel wires along a polyline (one wire per material)."""
    for k, mat in enumerate(mats):
        off = (k - (len(mats) - 1) / 2.0) * spread
        for a, b in zip(pts[:-1], pts[1:]):
            seg(m, mat, (a[0], a[1], a[2] + off), (b[0], b[1], b[2] + off), r)


def fan_blades(m, mat, c, axis, radius, count=7, length=2.6, width=1.1):
    """Blades around an axis: 'x' (cooler fan), 'y' (case fan), 'z' (GPU fans)."""
    for k in range(count):
        a = k * 360.0 / count
        ca, sa = math.cos(math.radians(a)), math.sin(math.radians(a))
        if axis == "z":   # yaw turns X towards Y
            m.box(mat, (c[0] + radius * ca, c[1] + radius * sa, c[2]), (length, width, 0.15), rot=(0, 0, a))
        elif axis == "y":  # pitch turns X towards Z
            m.box(mat, (c[0] + radius * ca, c[1], c[2] + radius * sa), (length, 0.15, width), rot=(0, a, 0))
        else:              # roll turns Y towards -Z
            m.box(mat, (c[0], c[1] + radius * ca, c[2] - radius * sa), (0.15, length, width), rot=(a, 0, 0))


# =============================================================================================================
# Parts (body coordinates: the body's bottom is z = 0, saved shifted up by FEET)
# =============================================================================================================
def side_panel():
    m = Model("PCA_SidePanel")
    vents = [Model.cut_box((-9.25, 7.0 + i * 2.4, 25.0), (2.0, 1.1, 11.0)) for i in range(5)]
    m.box("PlasticBeige", (-9.25, 0.0, 21.0), (0.5, 43.8, 41.6), bevel=0.25, cuts=vents)
    m.box("PlasticBeige", (-8.6, 22.15, 21.0), (1.8, 0.3, 41.6))       # rear flange the screws go through
    m.box("PlasticBeige", (-8.85, -21.75, 21.0), (0.8, 0.3, 41.0))     # front hook
    return m


def case_shell():
    m = Model("PCA_Case")
    # Outer skin (painted sheet metal), chassis (bare steel).
    m.box("PlasticBeige", (9.25, 0.0, 21.0), (0.5, 44.0, 42.0), bevel=0.2)
    m.box("PlasticBeige", (0.0, 0.0, 41.75), (19.0, 44.0, 0.5), bevel=0.2)
    m.box("MetalPaintedGrey", (0.0, 0.0, 0.25), (19.0, 44.0, 0.5))
    rear_cuts = [
        Model.cut_box((0.0, 21.75, 37.2), (14.0, 2.0, 7.6)),                     # power supply
        Model.cut_cyl((-3.0, 21.75, 27.0), 3.7, 2.0, rot=(90, 0, 0), segs=28),     # case fan
        Model.cut_box((6.0, 21.75, 25.5), (3.8, 2.0, 11.0)),                      # I/O shield
    ] + [Model.cut_box((1.05, 21.75, z), (10.9, 2.0, 1.3)) for z in (14.0, 12.0, 10.0, 8.0, 6.0)]
    m.box("MetalPaintedGrey", (0.0, 21.75, 21.0), (18.0, 0.5, 41.0), cuts=rear_cuts)
    for z in (12.0, 10.0, 8.0, 6.0):  # covers of the empty slots
        m.box("SteelBrushed", (1.05, 21.42, z), (11.4, 0.15, 1.75))
    for r in (3.3, 2.3, 1.3):  # fan guard
        m.torus("SteelBrushed", (-3.0, 21.95, 27.0), r, 0.12, rot=(90, 0, 0), segs=24, minor=6)
    m.box("SteelBrushed", (-3.0, 21.95, 27.0), (7.2, 0.2, 0.25))
    m.box("SteelBrushed", (-3.0, 21.95, 27.0), (0.25, 0.2, 7.2))
    m.box("SteelBrushed", (-3.95, 20.75, 10.5), (0.5, 1.3, 12.0))  # rail the slot brackets are screwed to
    # Front: steel frame with the upper 5.25" bay open, beige bezel in front of it.
    m.box("MetalPaintedGrey", (0.0, -21.75, 21.0), (18.0, 0.5, 41.0), cuts=[Model.cut_box((0.0, -21.75, 37.5), (14.8, 2.0, 4.4))])
    m.box("PlasticBeige", (0.0, -22.7, 21.0), (18.6, 1.4, 41.6), bevel=0.4, cuts=[Model.cut_box((0.0, -22.7, 37.5), (14.8, 3.0, 4.4))])
    m.box("PlasticGrey", (0.0, -23.42, 32.8), (14.6, 0.1, 4.2), bevel=0.05)  # blank 5.25" cover
    m.box("PlasticGrey", (0.0, -23.42, 26.0), (10.0, 0.1, 2.8), bevel=0.05)  # blank 3.5" cover
    m.cyl("PlasticGrey", (5.0, -23.55, 12.0), 1.3, 0.5, rot=(90, 0, 0), segs=20)  # power button
    m.box("LedGreen", (-5.0, -23.45, 14.0), (0.8, 0.15, 0.4))
    m.box("LedAmber", (-5.0, -23.45, 12.5), (0.8, 0.15, 0.4))
    for i in range(6):
        m.box("PlasticGrey", (0.0, -23.42, 3.0 + i * 1.1), (13.0, 0.1, 0.45))
    # Drive cages: 5.25" above, 3.5" below (with a window so the drives are visible).
    for x in (-7.65, 7.45):
        m.box("MetalPaintedGrey", (x, -12.7, 35.5), (0.3, 17.4, 12.0))
    for x in (-6.45, 4.65):
        m.box("MetalPaintedGrey", (x, -13.45, 12.5), (0.3, 15.9, 19.0), cuts=[Model.cut_box((x, -14.0, 11.25), (1.0, 5.0, 9.5))])
    # Brass standoffs under the mainboard screws.
    for y in (-2.6, 9.0, 20.2):
        for z in (3.3, 31.8):
            m.cyl("Gold", (8.55, y, z), 0.32, 0.9, rot=(0, 90, 0), segs=10)
    # Front panel wires (power button, LEDs) to the board.
    cable(m, ["PlasticBlack", "PlasticWhite"], [(4.0, -21.4, 12.0), (6.5, -10.0, 4.5), (7.4, -3.0, 4.5)], r=0.12, spread=0.3)
    # Feet.
    for x in (-7.5, 7.5):
        for y in (-19.0, 19.0):
            m.cyl("Rubber", (x, y, -0.4), 1.2, 0.8, segs=14)
    return m


def mainboard_part(name="PCA_Mainboard"):
    m = Model(name)
    bx = L.BOARD_X
    m.box("PCB", (bx, 8.9, 17.75), (0.16, 24.6, 30.5))
    top = bx - 0.08  # component side
    m.box("PlasticWhite", (top - 0.45, 12.0, 26.0), (0.9, 5.4, 5.4))  # CPU socket
    m.box("SteelBrushed", (top - 0.5, 15.0, 23.2), (0.3, 0.3, 0.6))     # lever catch
    for y in RAM_Y:
        m.box("PlasticBlack", (top - 0.45, y, 24.65), (0.9, 0.7, 14.6))
    m.box("PlasticBlack", (top - 0.4, 12.0, 14.0), (0.8, 8.0, 0.8))      # AGP slot
    for z in (10.0, 8.0, 6.0):
        m.box("PlasticWhite", (top - 0.38, 11.0, z), (0.75, 8.5, 0.75))  # PCI slots
    m.box("Aluminium", (top - 0.7, 8.0, 19.0), (1.4, 3.5, 3.5), bevel=0.05)  # northbridge cooler
    for i in range(6):
        m.box("Aluminium", (top - 1.6, 6.6 + i * 0.55, 19.0), (0.5, 0.2, 3.5))
    m.box("Chip", (top - 0.15, 2.0, 9.0), (0.3, 2.5, 2.5))               # southbridge
    m.box("Chip", (top - 0.12, -1.0, 6.0), (0.24, 1.6, 0.9))             # BIOS
    m.cyl("Chrome", (top - 0.15, 1.5, 4.5), 1.0, 0.3, rot=(0, 90, 0), segs=20)  # CMOS battery
    m.box("PlasticWhite", (top - 0.7, -2.0, 27.5), (1.4, 1.2, 5.2))      # ATX power connector
    m.box("PlasticWhite", (top - 0.5, 16.4, 32.0), (1.0, 1.0, 1.0))      # 4-pin CPU power
    m.box("PlasticWhite", (top - 0.3, 7.2, 31.6), (0.6, 0.8, 0.3))       # CPU fan header
    m.box("PlasticWhite", (top - 0.3, 17.0, 18.6), (0.6, 0.8, 0.3))      # case fan header
    for z in (20.5, 14.5):
        m.box("PlasticBlack", (top - 0.5, -2.6, z), (1.0, 0.8, 5.0))      # IDE
    for i, (y, z) in enumerate([(8.5, 22.0), (8.5, 24.0), (8.5, 28.0), (8.5, 30.0), (15.8, 29.5), (15.8, 22.4), (11.0, 30.5), (13.5, 30.5),
                                (10.0, 21.4), (13.5, 21.4), (3.0, 12.0), (4.0, 12.0), (5.0, 12.0), (19.0, 8.0), (19.0, 10.0)]):
        m.cyl("PlasticBlack" if i % 3 else "MetalPaintedBlue", (top - 0.8, y, z), 0.4, 1.6, rot=(0, 90, 0), segs=10)
    # Rear I/O block with ports.
    m.box("SteelBrushed", (6.0, 20.4, 25.5), (3.8, 2.6, 11.0))
    for z in (29.6, 28.0):
        m.cyl("PlasticBlack", (6.0, 21.72, z), 0.55, 0.1, rot=(90, 0, 0), segs=14)
    for z in (26.2, 25.2):
        m.box("SteelBrushed", (6.0, 21.75, z), (1.4, 0.15, 0.55))
        m.box("PlasticBlack", (6.0, 21.8, z), (1.1, 0.1, 0.3))
    m.box("Chrome", (6.3, 21.75, 22.6), (1.0, 0.15, 2.6))   # serial
    m.box("MetalPaintedBlue", (5.0, 21.75, 22.6), (0.6, 0.15, 3.0))  # parallel
    return m


def cpu_part(name="PCA_Cpu"):
    m = Model(name)
    x = L.BOARD_X - 0.08 - 0.9  # socket top
    m.box("PCB", (x - 0.12, 12.0, 26.0), (0.24, 4.9, 4.9))
    m.box("Aluminium", (x - 0.3, 12.0, 26.0), (0.14, 3.4, 3.4), bevel=0.03)
    m.box("Gold", (x - 0.25, 13.9, 24.1), (0.05, 0.5, 0.5))  # pin-1 mark
    return m


def cooler_part(name="PCA_CpuCooler"):
    m = Model(name)
    base = L.BOARD_X - 0.08 - 0.9 - 0.37  # on top of the CPU
    m.box("Copper", (base - 0.25, 12.0, 26.0), (0.5, 6.4, 6.4))
    for i in range(12):
        m.box("Aluminium", (base - 1.85, 9.0 + i * 0.55, 26.0), (2.7, 0.25, 6.4))
    fx = base - 3.7
    m.box("PlasticBlack", (fx, 12.0, 26.0), (1.0, 6.6, 6.6), bevel=0.1, cuts=[Model.cut_cyl((fx, 12.0, 26.0), 3.0, 2.0, rot=(0, 90, 0), segs=28)])
    m.cyl("PlasticBlack", (fx, 12.0, 26.0), 1.0, 0.8, rot=(0, 90, 0), segs=20)
    m.cyl("PlasticWhite", (fx - 0.41, 12.0, 26.0), 0.8, 0.04, rot=(0, 90, 0), segs=20)
    fan_blades(m, "PlasticGrey", (fx, 12.0, 26.0), "x", 1.95, 7, 2.0, 0.9)
    cable(m, ["PlasticBlack", "PlasticRed", "PlasticYellow"], [(fx + 0.3, 9.2, 29.2), (5.0, 7.2, 31.6), (6.3, 7.2, 31.6)], r=0.1, spread=0.22)
    return m


def ram_part(stick=0):
    # Quality update: one stick per part (slot RAM_Y[stick]).
    m = Model("PCA_Ram" if stick == 0 else "PCA_Ram2")
    slot_top = L.BOARD_X - 0.08 - 0.9
    for y in (RAM_Y[stick],):
        m.box("PCB", (slot_top - 1.55 + 0.3, y, 24.65), (3.1, 0.12, 13.3))
        for i in range(8):
            m.box("Chip", (slot_top - 1.6, y - 0.13, 19.0 + i * 1.6), (1.0, 0.14, 1.1))
        m.box("PlasticWhite", (slot_top - 2.4, y + 0.08, 24.65), (0.9, 0.04, 5.0))
    return m


def gpu_part():
    m = Model("PCA_GraphicsCard")
    m.box("PCB", (1.7, 9.35, 14.0), (10.6, 23.7, 0.16))                      # x -3.6..7.0
    m.box("Gold", (7.1, 12.0, 14.0), (0.4, 7.5, 0.18))
    m.box("PlasticBlack", (1.3, 9.0, 12.7), (9.4, 21.5, 2.4), bevel=0.3)    # shroud (fans face down)
    m.box("PlasticRed", (-3.4, 9.0, 12.7), (0.2, 21.5, 0.6))
    for y in (3.0, 14.0):
        m.cyl("PlasticGrey", (1.3, y, 11.45), 3.9, 0.15, segs=28)
        m.cyl("PlasticBlack", (1.3, y, 11.4), 1.2, 0.3, segs=16)
        fan_blades(m, "PlasticGrey", (1.3, y, 11.35), "z", 2.2, 7, 2.6, 0.9)
    m.box("PlasticBlack", (-3.2, 8.0, 14.55), (0.8, 1.6, 0.9))                # 6-pin power socket
    for i in range(6):  # memory chips on the back
        m.box("Chip", (2.0 + (i % 2) * 2.4, 4.0 + (i // 2) * 4.5, 14.15), (1.2, 1.2, 0.1))
    m.box("SteelBrushed", (1.75, 21.35, 14.0), (12.3, 0.15, 1.9))             # slot bracket
    m.box("SteelBrushed", (-4.3, 20.75, 14.0), (0.2, 1.3, 1.9))                # bracket tab
    m.box("MetalPaintedBlue", (2.0, 21.55, 14.0), (3.0, 0.4, 1.2))            # VGA
    m.box("PlasticWhite", (5.6, 21.55, 14.0), (2.6, 0.4, 1.0))                # DVI
    return m


def psu_part(name="PCA_PowerSupply", cables=True):
    m = Model(name)
    m.box("SteelBrushed", (0.0, 14.5, 37.2), (15.0, 13.8, 8.6), bevel=0.2)   # x -7.5..7.5, y 7.6..21.4
    for r in (3.2, 2.2, 1.2):
        m.torus("Chrome", (-2.5, 21.5, 37.2), r, 0.12, rot=(90, 0, 0), segs=24, minor=6)
    m.box("Chrome", (-2.5, 21.5, 37.2), (6.6, 0.15, 0.2))
    m.box("PlasticBlack", (-2.5, 21.42, 37.2), (6.6, 0.05, 6.6))
    m.box("PlasticBlack", (4.5, 21.5, 38.6), (2.4, 0.3, 1.8))   # power socket
    m.box("PlasticRed", (4.5, 21.5, 35.6), (1.0, 0.3, 1.3))     # switch
    m.box("PlasticWhite", (-7.53, 14.5, 37.2), (0.05, 9.0, 5.0))  # rating label
    for i in range(7):
        m.box("PlasticBlack", (-4.0 + i * 1.3, 7.58, 37.2), (0.5, 0.05, 6.0))
    if cables:
        # 20-pin to the board, 4-pin to the CPU area, Molex to the hard drive and the CD drive.
        cable(m, ["PlasticYellow", "PlasticRed", "PlasticBlack", "PlasticBlack"], [(3.0, 7.6, 34.0), (3.5, 2.0, 30.0), (3.5, -2.0, 27.5)], r=0.2, spread=0.42)
        cable(m, ["PlasticYellow", "PlasticBlack"], [(4.5, 7.6, 33.6), (3.9, 14.0, 32.0), (3.9, 16.4, 32.0)], r=0.18, spread=0.36)
        cable(m, ["PlasticYellow", "PlasticBlack", "PlasticRed"], [(-3.0, 7.6, 34.5), (-1.0, 2.0, 22.0), (2.8, -1.0, 12.0), (2.8, -3.0, 9.3)], r=0.18, spread=0.36)
        cable(m, ["PlasticYellow", "PlasticBlack", "PlasticRed"], [(5.5, 7.6, 37.8), (4.5, 3.0, 37.5), (4.5, 1.7, 37.5)], r=0.18, spread=0.36)
    return m


def hdd_part(name="PCA_HardDrive"):
    m = Model(name)
    m.box("Aluminium", (-0.9, -13.65, 9.3), (10.2, 14.7, 2.6), bevel=0.12)
    m.box("PlasticWhite", (-0.9, -14.5, 10.62), (8.0, 8.0, 0.04))
    m.box("PCB", (-0.9, -11.5, 7.95), (9.0, 9.5, 0.1))
    m.box("PlasticBlack", (-1.5, -6.15, 9.3), (5.0, 0.3, 0.9))
    m.box("PlasticWhite", (2.8, -6.15, 9.3), (2.0, 0.3, 0.9))
    for y in (-18.0, -10.0):  # threaded holes the cage screws go into
        m.cyl("SteelBrushed", (-6.02, y, 9.3), 0.2, 0.05, rot=(0, 90, 0), segs=10)
    return m


def ssd_part(name="PCA_Ssd"):
    m = Model(name)
    m.box("SteelBrushed", (-0.9, -15.5, 12.9), (10.2, 10.6, 0.2))     # 3.5" adapter
    for x in (-5.95, 4.15):
        m.box("SteelBrushed", (x, -15.5, 13.3), (0.1, 10.6, 1.0))
    m.box("PlasticBlack", (-0.9, -15.5, 13.35), (7.0, 10.0, 0.7), bevel=0.08)
    m.box("PlasticRed", (-0.9, -16.5, 13.71), (5.0, 4.0, 0.03))
    m.box("PlasticBlack", (-1.0, -10.42, 13.35), (2.4, 0.2, 0.4))
    return m


def case_fan_part(name="PCA_CaseFan"):
    m = Model(name)
    c = (-3.0, 20.15, 27.0)
    m.box("PlasticBlack", c, (8.0, 2.5, 8.0), bevel=0.15, cuts=[Model.cut_cyl(c, 3.8, 3.0, rot=(90, 0, 0), segs=28)])
    m.cyl("PlasticBlack", c, 1.4, 2.0, rot=(90, 0, 0), segs=20)
    m.cyl("PlasticWhite", (c[0], c[1] - 1.02, c[2]), 1.1, 0.04, rot=(90, 0, 0), segs=20)
    fan_blades(m, "PlasticBlack", c, "y", 2.55, 7, 2.6, 1.3)
    cable(m, ["PlasticBlack", "PlasticRed", "PlasticYellow"], [(1.0, 18.9, 23.2), (4.0, 17.0, 18.6), (7.0, 17.0, 18.6)], r=0.1, spread=0.22)
    return m


def cd_part(name="PCA_CdDrive"):
    m = Model(name)
    m.box("SteelBrushed", (0.0, -11.6, 37.5), (14.6, 20.0, 4.2), bevel=0.08)
    m.box("PlasticBeige", (0.0, -22.6, 37.5), (14.6, 2.0, 4.2), bevel=0.15)
    m.box("PlasticGrey", (0.0, -23.62, 38.3), (12.4, 0.05, 0.25))
    m.box("PlasticGrey", (5.5, -23.64, 36.4), (1.4, 0.08, 0.5))
    m.box("LedGreen", (3.8, -23.64, 36.4), (0.4, 0.06, 0.2))
    m.cyl("PlasticGrey", (-5.6, -23.62, 36.4), 0.18, 0.06, rot=(90, 0, 0), segs=10)  # emergency eject
    m.box("PlasticBlack", (-2.5, -1.45, 37.5), (5.0, 0.3, 0.9))
    m.box("PlasticWhite", (4.5, -1.45, 37.5), (2.0, 0.3, 0.9))
    return m


PART_BUILDERS = {
    "SidePanel": side_panel, "GraphicsCard": gpu_part, "Ram": ram_part, "CpuCooler": cooler_part, "Cpu": cpu_part,
    "HardDrive": hdd_part, "Ssd": ssd_part, "CaseFan": case_fan_part, "PowerSupply": psu_part, "Mainboard": mainboard_part,
    "CdDrive": cd_part, "Case": case_shell, "Ram2": lambda: ram_part(1),
}


# =============================================================================================================
# Fasteners: origin at the mounting point, +Z points out (the way they come off).
# =============================================================================================================
def fastener_screw():
    m = Model("FastenerScrew")
    m.cyl("Chrome", (0, 0, 0.11), 0.42, 0.22, segs=16)
    m.box("SteelBrushed", (0, 0, 0.22), (0.5, 0.09, 0.04))
    m.box("SteelBrushed", (0, 0, 0.22), (0.09, 0.5, 0.04))
    m.cyl("SteelBrushed", (0, 0, -0.5), 0.16, 1.0, segs=10)
    return m


def fastener_thumbscrew():
    m = Model("FastenerThumbscrew")
    m.cyl("Chrome", (0, 0, 0.25), 0.6, 0.5, segs=20)
    for k in range(16):
        a = math.radians(k * 22.5)
        m.box("Chrome", (0.6 * math.cos(a), 0.6 * math.sin(a), 0.25), (0.12, 0.12, 0.5), rot=(0, 0, k * 22.5))
    m.cyl("SteelBrushed", (0, 0, -0.5), 0.17, 1.0, segs=10)
    return m


def fastener_plug():
    m = Model("FastenerPlug")
    m.box("PlasticWhite", (0, 0, 0.5), (1.6, 0.7, 1.0), bevel=0.06)
    m.box("PlasticWhite", (0.86, 0, 0.55), (0.12, 0.35, 0.6))  # latch
    for k, mat in enumerate(["PlasticYellow", "PlasticBlack", "PlasticBlack", "PlasticRed"]):
        m.cyl(mat, (-0.6 + k * 0.4, 0, 2.5), 0.15, 3.0, segs=8)
    return m


def fastener_clip():
    m = Model("FastenerClip")
    m.box("PlasticWhite", (0, 0, 0.55), (0.45, 0.8, 1.1), bevel=0.05)
    m.box("PlasticWhite", (0.3, 0, 1.0), (0.3, 0.8, 0.2))  # hook over the module
    return m


def fastener_lever():
    m = Model("FastenerLever")
    m.cyl("SteelBrushed", (2.5, 0, 0.15), 0.12, 5.0, rot=(0, 90, 0), segs=8)
    m.box("SteelBrushed", (5.0, 0, 0.45), (0.25, 0.25, 0.9))
    m.box("PlasticWhite", (0.0, 0, 0.1), (0.5, 0.5, 0.3))
    return m


# =============================================================================================================
# Loose items (lying flat) from the same geometry
# =============================================================================================================
def ram_module_item():
    m = Model("RamModule")
    m.box("PCB", (0, 0, 0.06), (13.3, 3.1, 0.12))
    for i in range(8):
        m.box("Chip", (-5.6 + i * 1.6, -0.1, 0.19), (1.1, 1.0, 0.14))
    for i in range(30):
        m.box("Gold", (-6.2 + i * 0.42, 1.4, 0.13), (0.26, 0.3, 0.02))
    m.box("PlasticWhite", (0, -0.9, 0.14), (5.0, 0.9, 0.04))
    return m


def as_item(builder, item_name, rot):
    m = builder()
    m.name = item_name
    mtx.transform_mesh(m.mesh, bm.xf((0, 0, 0), rot))
    return m


ITEM_MODELS = {
    "GraphicsCard": lambda: as_item(gpu_part, "GraphicsCard", (180, 0, 0)),
    "RamModule": ram_module_item,
    "CpuCooler": lambda: as_item(cooler_part, "CpuCooler", (0, -90, 0)),
    "Processor": lambda: as_item(cpu_part, "Processor", (0, -90, 0)),
    "PowerSupply": lambda: as_item(lambda: psu_part("PowerSupply", cables=True), "PowerSupply", (0, 0, 0)),
    "Mainboard": lambda: as_item(mainboard_part, "Mainboard", (0, -90, 0)),
    "HardDrive": lambda: as_item(hdd_part, "HardDrive", (0, 0, 0)),
    "SsdDrive": lambda: as_item(ssd_part, "SsdDrive", (0, 0, 0)),
    "CaseFan": lambda: as_item(case_fan_part, "CaseFan", (90, 0, 0)),
}


# =============================================================================================================
# Highlight overlays
# =============================================================================================================
def highlight_material(name, color, strength):
    mel = unreal.MaterialEditingLibrary
    path = f"{FX}/{name}"
    if bm.eal.does_asset_exist(path):
        bm.eal.delete_asset(path)
    mat = unreal.AssetToolsHelpers.get_asset_tools().create_asset(name, FX, unreal.Material, unreal.MaterialFactoryNew())
    mat.set_editor_property("blend_mode", unreal.BlendMode.BLEND_TRANSLUCENT)
    mat.set_editor_property("shading_model", unreal.MaterialShadingModel.MSM_UNLIT)
    fres = mel.create_material_expression(mat, unreal.MaterialExpressionFresnel, -600, 0)
    fres.set_editor_property("exponent", 2.5)
    fres.set_editor_property("base_reflect_fraction", 0.08)
    col = mel.create_material_expression(mat, unreal.MaterialExpressionConstant3Vector, -600, -200)
    col.set_editor_property("constant", color)
    mul = mel.create_material_expression(mat, unreal.MaterialExpressionMultiply, -300, -150)
    mel.connect_material_expressions(col, "", mul, "A")
    lift = mel.create_material_expression(mat, unreal.MaterialExpressionAdd, -450, 50)
    lift.set_editor_property("const_b", 0.25)
    mel.connect_material_expressions(fres, "", lift, "A")
    mel.connect_material_expressions(lift, "", mul, "B")
    gain = mel.create_material_expression(mat, unreal.MaterialExpressionMultiply, -150, -150)
    gain.set_editor_property("const_b", strength)
    mel.connect_material_expressions(mul, "", gain, "A")
    mel.connect_material_property(gain, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)
    op = mel.create_material_expression(mat, unreal.MaterialExpressionMultiply, -150, 100)
    op.set_editor_property("const_b", 0.6)
    mel.connect_material_expressions(lift, "", op, "A")
    mel.connect_material_property(op, "", unreal.MaterialProperty.MP_OPACITY)
    mel.recompile_material(mat)
    bm.eal.save_asset(path)
    unreal.log(f"WE_PCA material {name}")


class _Shared:
    """Stands in for Model while the closed PC is built: every part builder draws into the same model."""

    cut_box = staticmethod(bm.Model.cut_box)
    cut_cyl = staticmethod(bm.Model.cut_cyl)

    def __init__(self, target):
        self.target = target

    def __call__(self, name):
        return self.target


def main():
    global Model
    built, failed = 0, []
    for pid, make in PART_BUILDERS.items():
        try:
            m = make()
            bm.translate(m.mesh, (0, 0, L.FEET))
            if m.save(normalize=False, folder=PCA):
                built += 1
            else:
                failed.append(pid)
        except Exception as e:
            failed.append(pid)
            unreal.log_error(f"WE_PCA {pid} failed: {e}")
    # The closed PC (the loose OldComputer item): all parts drawn into one model, in the same space.
    try:
        assembled = bm.Model("OldComputer")
        Model = _Shared(assembled)
        for make in PART_BUILDERS.values():
            make()
        Model = bm.Model
        bm.translate(assembled.mesh, (0, 0, L.FEET))
        if assembled.save(normalize=False):
            built += 1
    except Exception as e:
        Model = bm.Model
        failed.append("OldComputer")
        unreal.log_error(f"WE_PCA OldComputer failed: {e}")
    for make in (fastener_screw, fastener_thumbscrew, fastener_plug, fastener_clip, fastener_lever):
        try:
            if make().save(normalize=False):
                built += 1
        except Exception as e:
            failed.append(make.__name__)
            unreal.log_error(f"WE_PCA {make.__name__} failed: {e}")
    for name, make in ITEM_MODELS.items():
        try:
            if make().save():
                built += 1
        except Exception as e:
            failed.append(name)
            unreal.log_error(f"WE_PCA item {name} failed: {e}")
    try:
        highlight_material("M_WEHighlightHover", unreal.LinearColor(1.0, 0.75, 0.2), 2.0)
        highlight_material("M_WEHighlightSelect", unreal.LinearColor(0.2, 0.85, 1.0), 3.0)
    except Exception as e:
        failed.append("highlight")
        unreal.log_error(f"WE_PCA highlight failed: {e}")
    unreal.log(f"WE_PCA_BUILT {built} failed={failed}")
    unreal.SystemLibrary.quit_editor()


main()
