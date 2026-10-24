"""
Massive quality overhaul: modelled workshop fixtures (Geometry Script, own models, no downloads).
  SM_Gen_Workbench        80 x 200 x 90 cm (depth X, width Y, height Z; front = +X): butcher-block top, square-tube
                          steel frame, lower shelf, drawer unit with handles, vice plate - replaces the plywood boxes
  SM_Gen_WorkbenchLarge   100 x 280 x 95 cm, same build, two drawer units
  SM_Gen_WallOutlet       double socket on a surface box (wall at its back, faces +X)
  SM_Gen_LightSwitch      surface switch (faces +X)
  SM_Gen_Conduit          100 cm galvanised conduit with two saddle clamps (stretch along Z), wall at its back (-X)
  SM_Gen_JunctionBox      small grey junction box (faces +X)
Origin at the bottom centre (wall fixtures: bottom centre of the back face).

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


def bench(name, D, W, H, units):
    m = Model(name)
    top = 5.0
    leg = 5.0
    # Butcher-block top: glued strips (alternating shade through the wood texture), rounded edges.
    m.box("Wood", (0, 0, H - top / 2), (D, W, top), bevel=0.6, segments=2)
    for i in range(1, int(W // 12)):
        m.box("TrimBlack", (0, -W / 2 + i * 12.0, H - 0.04), (D - 3.0, 0.15, 0.1))  # glue lines
    # Steel frame: four square-tube legs, rails under the top and above the floor shelf.
    frame = "MetalPaintedGrey"
    for x in (D / 2 - leg / 2 - 3, -D / 2 + leg / 2 + 3):
        for y in (W / 2 - leg / 2 - 4, -W / 2 + leg / 2 + 4):
            m.box(frame, (x, y, (H - top) / 2), (leg, leg, H - top), bevel=0.3)
            m.box("Rubber", (x, y, 0.6), (leg + 1.0, leg + 1.0, 1.2))  # leveling feet
    for x in (D / 2 - leg / 2 - 3, -D / 2 + leg / 2 + 3):
        m.box(frame, (x, 0, H - top - 3.0), (leg - 1.0, W - 8.0, 6.0), bevel=0.3)
        m.box(frame, (x, 0, 9.0), (leg - 1.0, W - 8.0, 4.0), bevel=0.3)
    for y in (W / 2 - leg / 2 - 4, -W / 2 + leg / 2 + 4):
        m.box(frame, (0, y, H - top - 3.0), (D - 6.0, leg - 1.0, 6.0), bevel=0.3)
        m.box(frame, (0, y, 9.0), (D - 6.0, leg - 1.0, 4.0), bevel=0.3)
    # Lower shelf (the toolbox stands on it at z = 12).
    m.box("Wood", (0, 0, 11.0), (D - 8.0, W - 10.0, 2.0), bevel=0.2)
    # Drawer units under the top, front faces +X.
    for uy in units:
        uw = 46.0
        m.box("ToolBlue", (0, uy, H - top - 22.0), (D - 10.0, uw, 34.0), bevel=0.5)
        for k in range(3):
            z = H - top - 10.0 - k * 11.0
            m.box("ToolBlue", (D / 2 - 4.5, uy, z), (1.2, uw - 3.0, 10.0), bevel=0.4)
            m.box("TrimBlack", (D / 2 - 3.8, uy, z - 5.2), (0.4, uw - 3.0, 0.4))
            m.box("Chrome", (D / 2 - 3.2, uy, z + 1.5), (1.4, 18.0, 1.6), bevel=0.4)
    return m


def wall_outlet():
    m = Model("WallOutlet")
    m.box("PlasticWhite", (4.0, 0, 6.0), (8.0, 15.5, 12.0), bevel=0.6, segments=2)
    for y in (-3.6, 3.6):
        m.cyl("PlasticWhite", (8.2, y, 6.0), 2.9, 0.8, rot=(0, 90, 0), segs=28)
        m.cyl("TrimBlack", (8.65, y, 6.0), 2.2, 0.2, rot=(0, 90, 0), segs=24)
        for z in (-0.95, 0.95):
            m.cyl("Chrome", (8.7, y, 6.0 + z), 0.3, 0.2, rot=(0, 90, 0), segs=10)
    m.box("PlasticGrey", (8.1, 0, 0.6), (0.6, 4.0, 0.5))  # cable gland at the bottom
    return m


def light_switch():
    m = Model("LightSwitch")
    m.box("PlasticWhite", (3.5, 0, 4.5), (7.0, 8.5, 9.0), bevel=0.6, segments=2)
    m.box("PlasticWhite", (7.3, 0, 4.5), (0.8, 5.5, 6.0), rot=(0, -8, 0), bevel=0.4)
    return m


def conduit():
    m = Model("Conduit")
    m.cyl("SteelBrushed", (1.6, 0, 50.0), 1.1, 100.0, segs=14)
    for z in (15.0, 85.0):
        m.box("SteelBrushed", (1.2, 0, z), (2.6, 3.6, 2.0))
        for y in (-2.4, 2.4):
            m.cyl("Chrome", (0.6, y, z), 0.45, 0.6, rot=(0, 90, 0), segs=8)
    return m


def junction_box():
    m = Model("JunctionBox")
    m.box("PlasticGrey", (3.5, 0, 5.0), (7.0, 10.0, 10.0), bevel=0.5, segments=2)
    for y in (-3.5, 3.5):
        for z in (1.5, 8.5):
            m.cyl("SteelBrushed", (7.05, y, z), 0.5, 0.2, rot=(0, 90, 0), segs=8)
    return m


def tube_fixture():
    """Twin-tube batten (130 cm) under the ceiling: white steel body, two glowing tubes, end caps. Top at z = 0."""
    m = Model("TubeFixture")
    m.box("PlasticWhite", (0, 0, -3.0), (132.0, 12.0, 6.0), bevel=0.8, segments=2)
    m.box("SteelBrushed", (0, 0, -6.2), (128.0, 9.0, 0.6))  # reflector
    for y in (-2.6, 2.6):
        m.cyl("FluorescentTube", (0, y, -8.2), 1.3, 120.0, rot=(0, 90, 0), segs=12)
    for x in (-61.5, 61.5):
        m.box("PlasticGrey", (x, 0, -8.0), (2.0, 10.0, 4.0), bevel=0.3)
    for x in (-50.0, 50.0):  # ceiling hangers
        m.box("SteelBrushed", (x, 0, -0.3), (3.0, 3.0, 0.6))
    return m


def dirt_mound():
    """Heap of earth and gravel (100 x 100 x 40 cm, stretched per place): overlapping flattened lumps, a few stones."""
    m = Model("DirtMound")
    lumps = [(0, 0, 0, 50, 0.8), (-22, 14, 0, 34, 0.75), (20, -16, 0, 32, 0.7), (16, 22, 0, 26, 0.65), (-20, -22, 0, 28, 0.6),
             (4, 4, 10, 30, 0.7)]
    for x, y, z, r, flat in lumps:
        m.sphere("Concrete", (x, y, z), r, scale=(1.0, 1.0, flat * 0.8), steps=14)
    for k in range(9):
        a = math.radians(k * 41.0)
        rr = 38 + (k % 3) * 6
        m.sphere("Concrete", (math.cos(a) * rr, math.sin(a) * rr, 2.0), 3.0 + (k % 3), scale=(1.2, 1.0, 0.8), steps=6)
    return m


def scrap_heap():
    """Scrap heap in the yard (about 170 x 130 x 110 cm): a ground mound under crumpled sheets, pipes, tyres, a dented
    cabinet, a radiator and bent rods (the scrap pile actor keeps its boxes as collision)."""
    m = Model("ScrapHeap")
    m.sphere("Rust", (0, 0, 0), 80, scale=(1.05, 0.8, 0.32), steps=16)
    sheets = [((-30, 10, 30), (70, 50, 2), (12, 25, 8), "RustHeavy"), ((25, -20, 34), (60, 45, 2), (-15, -18, 40), "Rust"),
              ((0, 25, 48), (55, 40, 2), (28, 8, -30), "MetalPaintedBlue"), ((-10, -25, 52), (50, 35, 2), (-30, 12, 70), "RustHeavy"),
              ((35, 15, 55), (40, 30, 2), (40, -10, 15), "MetalPaintedGreen"), ((-40, -5, 58), (45, 35, 2), (-45, 20, -20), "Rust")]
    for c, s, r, mat in sheets:
        m.box(mat, c, s, rot=r, bevel=0.3)
    m.box("PlasticWhite", (-5, -5, 72), (45, 40, 50), rot=(8, -12, 20), bevel=1.5)  # old washing machine, tipped over
    m.cyl("TrimBlack", (-5 + 12, -5 - 6, 72 + 2), 13, 2, rot=(98, -12, 20), segs=20)
    m.box("RustHeavy", (30, 20, 80), (12, 40, 45), rot=(-20, 10, 70), bevel=0.6)  # radiator
    for k in range(6):
        m.box("RustHeavy", (30 + 0.5 * k, 20 - 15 + 6 * k, 80), (14, 1.2, 46), rot=(-20, 10, 70))
    for i, (c, r) in enumerate((((50, -30, 22), (70, 0, 30)), ((-55, 30, 25), (75, 20, -40)))):
        m.torus("TireRubber", c, 26, 9, rot=r, segs=28, minor=10)
    for c, rot, length in (((-20, 35, 60), (0, 80, 30), 120), ((15, -35, 66), (10, 70, -50), 100), ((0, 0, 95), (0, 60, 10), 80)):
        m.cyl("SteelBrushed", c, 2.5, length, rot=rot, segs=10)
    for c, rot in (((40, -5, 92), (30, 40, 0)), ((-35, -30, 85), (-50, 20, 60)), ((10, 40, 88), (70, 0, 30))):
        m.cyl("Rust", c, 0.8, 70, rot=rot, segs=6)
    return m


BUILDERS = {
    "ScrapHeap": scrap_heap,
    "DirtMound": dirt_mound,
    "TubeFixture": tube_fixture,
    "Workbench": lambda: bench("Workbench", 80.0, 200.0, 90.0, [62.0]),
    "WorkbenchLarge": lambda: bench("WorkbenchLarge", 100.0, 280.0, 95.0, [-95.0, 95.0]),
    "WallOutlet": wall_outlet, "LightSwitch": light_switch, "Conduit": conduit, "JunctionBox": junction_box,
}


def main():
    built, failed = 0, []
    for name, make in BUILDERS.items():
        try:
            m = make()
            # Wall fixtures keep their origin at the back face (normalize centres X).
            if m.save(normalize=name.startswith("Workbench") or name in ("DirtMound", "ScrapHeap")):  # (the fixture keeps its top at z = 0)
                built += 1
            else:
                failed.append(name)
        except Exception as e:
            failed.append(name)
            unreal.log_error(f"WE_MODEL {name} failed: {e}")
    unreal.log(f"WE_WORKSHOP_MODELS_BUILT {built} failed={failed}")


main()
unreal.SystemLibrary.quit_editor()
