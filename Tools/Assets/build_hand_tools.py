"""
Phase 21: hand tools and bench props for the work view (Geometry Script like build_models.py, no downloads).
Tools keep their working point at the origin and point down: the tip/bit/jaws are at (0,0,0), the handle goes up +Z
(the hammer's face is at the origin, its handle runs along +X). AWEToolHand moves them with the cursor and onto screws.
  SM_Gen_Tool_Screwdriver, _CordlessDrill, _Pliers, _SideCutter, _Hammer, _Multimeter (probe), _SolderingIron,
  _AngleGrinder, _AirDuster, _PasteSyringe, _Brush
Bench props (origin bottom centre): SM_Gen_ScrewBowl (magnetic parts bowl), SM_Gen_PartBox (open cardboard box),
SM_Gen_BoxFlap (flap, hinge along its X axis at y = 0), SM_Gen_PasteBlob (thermal paste), SM_Gen_DustLayer.

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
eal = unreal.EditorAssetLibrary
mel = unreal.MaterialEditingLibrary
tools = unreal.AssetToolsHelpers.get_asset_tools()

EXTRA = {
    "ToolBlue": ((0.03, 0.12, 0.42), 0.45, None),
    "Paste": ((0.55, 0.57, 0.6), 0.35, None),
    "Dust": ((0.085, 0.075, 0.065), 1.0, None),
    "Bristle": ((0.12, 0.09, 0.06), 0.9, None),
    "Pegboard": ((0.42, 0.31, 0.19), 0.85, None),        # hardboard
    "PegHole": ((0.02, 0.016, 0.012), 1.0, None),
    "ToolOutline": ((0.9, 0.88, 0.8), 0.9, None),        # painted shadow-board outline
    "ParcelTape": ((0.45, 0.32, 0.16), 0.35, None),      # brown packing tape
}


def make_materials():
    parent = unreal.load_asset("/Game/WorkshopEmpire/Materials/M_WEProp")
    for name, (tint, rough, emissive) in EXTRA.items():
        path = f"{bm.PROPS}/MI_Prop_{name}"
        mi = eal.load_asset(path) if eal.does_asset_exist(path) else tools.create_asset(
            f"MI_Prop_{name}", bm.PROPS, unreal.MaterialInstanceConstant, unreal.MaterialInstanceConstantFactoryNew())
        mi.set_editor_property("parent", parent)
        mel.set_material_instance_vector_parameter_value(mi, "Tint", unreal.LinearColor(*tint, 1.0))
        mel.set_material_instance_scalar_parameter_value(mi, "Roughness", rough)
        eal.save_asset(path)


def screwdriver():
    m = Model("Tool_Screwdriver")
    m.box("SteelBrushed", (0, 0, 0.35), (0.55, 0.12, 0.7))                       # flat blade tip
    m.cone("SteelBrushed", (0, 0, 1.2), 0.12, 0.3, 1.0, segs=10)
    m.cyl("SteelBrushed", (0, 0, 5.8), 0.3, 9.0, segs=12)                        # shaft
    m.cyl("PlasticRed", (0, 0, 15.0), 1.35, 9.5, segs=20, bevel=0.3)            # handle
    for i in range(6):
        a = i * 60.0
        m.box("TrimBlack", (math.cos(math.radians(a)) * 1.3, math.sin(math.radians(a)) * 1.3, 15.0), (0.35, 0.35, 8.0), rot=(0, 0, a))
    m.sphere("PlasticRed", (0, 0, 19.8), 1.3, scale=(1, 1, 0.5), steps=14)
    return m


def cordless_drill():
    m = Model("Tool_CordlessDrill")
    m.cyl("SteelBrushed", (0, 0, 2.6), 0.22, 5.2, segs=8)                        # bit
    m.box("SteelBrushed", (0, 0, 0.3), (0.45, 0.1, 0.6))
    m.cyl("TrimBlack", (0, 0, 6.6), 1.25, 2.8, segs=18, bevel=0.2)               # chuck
    m.cyl("PlasticYellow", (0, 0, 14.0), 2.9, 12.0, segs=22, bevel=0.6)         # motor housing
    m.cyl("TrimBlack", (0, 0, 20.4), 2.6, 0.8, segs=22)
    m.box("PlasticYellow", (-6.5, 0, 16.0), (10.0, 3.0, 3.6), bevel=0.8)        # grip
    m.box("PlasticRed", (-2.6, 0, 14.6), (1.0, 1.2, 1.6), bevel=0.2)            # trigger
    m.box("TrimBlack", (-12.6, 0, 16.0), (3.6, 6.5, 8.0), bevel=0.8)            # battery
    return m


def pliers(name, handle, jaw_len):
    m = Model(name)
    for s in (-1.0, 1.0):
        m.box("SteelBrushed", (s * 0.35, 0, jaw_len / 2), (0.6, 0.5, jaw_len), bevel=0.1)          # jaws
        m.box("SteelBrushed", (s * 0.6, 0, jaw_len + 1.6), (0.8, 0.6, 3.2), rot=(0, s * 6.0, 0))
        m.box(handle, (s * 1.4, 0, jaw_len + 9.0), (1.1, 0.9, 11.0), rot=(0, s * 6.0, 0), bevel=0.3)
    m.cyl("SteelBrushed", (0, 0, jaw_len + 0.6), 0.5, 1.1, rot=(90, 0, 0), segs=12)                # pivot
    return m


def hammer():
    m = Model("Tool_Hammer")
    m.cyl("SteelBrushed", (0, 0, 2.6), 1.4, 5.2, segs=16, bevel=0.2)            # head, face down at the origin
    m.box("SteelBrushed", (0, 0, 7.0), (2.0, 1.6, 3.6), bevel=0.3)
    m.box("Wood", (11.0, 0, 5.0), (22.0, 1.8, 2.4), bevel=0.4)                 # handle along +X
    m.box("TrimBlack", (18.0, 0, 5.0), (8.0, 2.1, 2.7), bevel=0.4)
    return m


def multimeter_probe():
    m = Model("Tool_Multimeter")
    m.cyl("SteelBrushed", (0, 0, 1.0), 0.07, 2.0, segs=6)
    m.cone("PlasticRed", (0, 0, 2.6), 0.2, 0.5, 1.2, segs=12)
    m.cyl("PlasticRed", (0, 0, 7.6), 0.5, 9.0, segs=12)
    m.cyl("PlasticRed", (0, 0, 3.9), 0.75, 0.4, segs=12)                        # finger guard
    m.cyl("TrimBlack", (0, 0, 16.0), 0.18, 8.0, segs=6)                         # lead
    return m


def soldering_iron():
    m = Model("Tool_SolderingIron")
    m.cone("SteelBrushed", (0, 0, 1.0), 0.05, 0.3, 2.0, segs=8)
    m.cyl("SteelBrushed", (0, 0, 5.5), 0.32, 7.0, segs=10)
    m.cyl("TrimBlack", (0, 0, 10.0), 0.9, 2.0, segs=14)
    m.cyl("ToolBlue", (0, 0, 15.0), 1.0, 8.0, segs=16, bevel=0.2)
    m.cyl("TrimBlack", (0, 0, 21.0), 0.2, 4.0, segs=6)
    return m


def angle_grinder():
    m = Model("Tool_AngleGrinder")
    m.cyl("TrimBlack", (0, 0, 5.5), 5.5, 0.3, rot=(90, 0, 0), segs=36)          # disc, edge touches the origin
    m.cyl("SteelBrushed", (0, 0, 5.5), 4.0, 1.4, rot=(90, 90, 0), segs=24)      # guard half (approximation)
    m.box("ToolBlue", (0, 3.5, 9.0), (4.5, 6.0, 5.0), bevel=0.6)               # gear head
    m.cyl("ToolBlue", (0, 3.5, 20.0), 2.8, 18.0, segs=20, bevel=0.6)           # motor body
    return m


def air_duster():
    m = Model("Tool_AirDuster")
    m.cyl("PlasticRed", (0, 0, 2.0), 0.12, 4.0, segs=6)                         # straw (nozzle at the origin)
    m.box("PlasticWhite", (0, 0, 4.8), (1.4, 1.2, 1.6), bevel=0.3)            # trigger head
    m.cyl("SteelBrushed", (0, 0, 13.0), 2.6, 15.0, segs=24, bevel=0.4)          # can
    m.cyl("PlasticBlue" if False else "ToolBlue", (0, 0, 11.0), 2.65, 6.0, segs=24)   # label band
    return m


def paste_syringe():
    m = Model("Tool_PasteSyringe")
    m.cone("Paste", (0, 0, 0.6), 0.12, 0.4, 1.2, segs=10)
    m.cyl("GlassClear", (0, 0, 4.2), 0.6, 6.0, segs=14)
    m.cyl("Paste", (0, 0, 3.0), 0.5, 3.6, segs=14)
    m.cyl("PlasticWhite", (0, 0, 7.6), 1.1, 0.3, segs=14)
    m.cyl("PlasticWhite", (0, 0, 9.6), 0.25, 4.0, segs=8)                        # plunger
    m.cyl("PlasticWhite", (0, 0, 11.8), 0.9, 0.3, segs=14)
    return m


def brush():
    m = Model("Tool_Brush")
    m.box("Bristle", (0, 0, 1.0), (2.4, 0.8, 2.0))
    m.box("SteelBrushed", (0, 0, 2.4), (2.6, 1.0, 0.9))
    m.cyl("Wood", (0, 0, 8.0), 0.5, 10.0, segs=10)
    return m


def screw_bowl():
    m = Model("ScrewBowl")
    m.revolve("SteelBrushed", [(0.0, 0.0), (5.0, 0.0), (6.0, 0.6), (6.6, 2.0), (6.9, 2.2), (6.9, 2.0), (6.3, 1.6), (5.4, 0.4), (0.0, 0.4)], steps=40)
    m.cyl("TrimBlack", (0, 0, 0.05), 4.6, 0.1, segs=32)                         # rubber magnet base
    return m


def part_box():
    m = Model("PartBox")
    w, d, h, t = 14.0, 10.0, 6.0, 0.25
    m.box("Cardboard", (0, 0, t / 2), (w, d, t))
    m.box("Cardboard", (0, d / 2 - t / 2, h / 2), (w, t, h))
    m.box("Cardboard", (0, -d / 2 + t / 2, h / 2), (w, t, h))
    m.box("Cardboard", (w / 2 - t / 2, 0, h / 2), (t, d, h))
    m.box("Cardboard", (-w / 2 + t / 2, 0, h / 2), (t, d, h))
    m.box("PlasticWhite", (0, 0, t + 0.6), (w - 1.0, d - 1.0, 1.0))             # foam insert
    return m


def box_flap():
    m = Model("BoxFlap")
    m.box("Cardboard", (0, 2.5, 0.12), (14.0, 5.0, 0.25))                        # hinge along X at y = 0
    return m


def paste_blob():
    m = Model("PasteBlob")
    m.sphere("Paste", (0, 0, 0), 1.0, scale=(1.0, 1.0, 0.35), steps=12)
    return m


def dust_layer():
    m = Model("DustLayer")
    m.box("Dust", (0, 0, 0.05), (10.0, 10.0, 0.1))
    for i in range(14):
        a = i * 2.4
        r = 1.5 + (i % 5) * 0.8
        m.sphere("Dust", (math.cos(a) * r, math.sin(a) * r, 0.1), 0.6 + (i % 3) * 0.3, scale=(1, 1, 0.35), steps=6)
    return m


def pegboard():
    """Tool wall board: back at x = 0, front at x = 1.6, 150 wide (y), 140 high (z), origin bottom centre of the back."""
    m = Model("Pegboard")
    m.box("Pegboard", (0.8, 0, 70.0), (1.6, 150.0, 140.0))
    for y in range(20):
        for z in range(18):
            m.box("PegHole", (1.62, -71.25 + y * 7.5, 6.0 + z * 7.6), (0.06, 0.8, 0.8))
    for z in (0.0, 140.0):                                                          # wooden battens top and bottom
        m.box("Wood", (1.4, 0, z), (2.8, 152.0, 3.0), bevel=0.3)
    for y in (-75.0, 75.0):
        m.box("Wood", (1.4, y, 61.5), (2.8, 4.0, 163.0), bevel=0.3)                   # side posts down to the floor (20 below)
    return m


def parcel():
    """Shipping carton 40 x 30 x 25 with packing tape and a label (origin bottom centre)."""
    m = Model("Parcel")
    m.box("Cardboard", (0, 0, 12.5), (40.0, 30.0, 25.0), bevel=0.4)
    m.box("ParcelTape", (0, 0, 25.05), (40.4, 5.0, 0.12))                         # tape over the top seam
    for x in (-20.1, 20.1):
        m.box("ParcelTape", (x, 0, 19.5), (0.12, 5.0, 11.0))                     # and down the ends
    m.box("PlasticWhite", (8.0, -6.0, 25.08), (12.0, 8.0, 0.1))                  # address label
    m.box("TrimBlack", (8.0, -6.0, 25.14), (9.0, 1.2, 0.05))                     # barcode
    return m


def peg_hook():
    """Pegboard hook: origin on the board front, sticks out along +x and bends up at the tip."""
    m = Model("PegHook")
    m.cyl("SteelBrushed", (3.0, 0, 0), 0.22, 6.0, rot=(0, 90, 0), segs=8)
    m.cyl("SteelBrushed", (6.0, 0, 0.8), 0.22, 1.8, segs=8)
    m.box("SteelBrushed", (0.15, 0, 0.6), (0.3, 1.0, 2.2))
    return m


BUILDERS = [
    (screwdriver, False), (cordless_drill, False), (lambda: pliers("Tool_Pliers", "PlasticRed", 4.0), False),
    (lambda: pliers("Tool_SideCutter", "PlasticYellow", 2.2), False), (hammer, False), (multimeter_probe, False),
    (soldering_iron, False), (angle_grinder, False), (air_duster, False), (paste_syringe, False), (brush, False),
    (screw_bowl, True), (part_box, True), (box_flap, False), (paste_blob, False), (dust_layer, True),
    (pegboard, False), (peg_hook, False), (parcel, True),
]


def main():
    make_materials()
    built = 0
    for make, normalize in BUILDERS:
        try:
            if make().save(normalize=normalize):
                built += 1
        except Exception as e:  # keep going, report at the end
            unreal.log_error(f"WE_MODEL failed: {e}")
    unreal.log(f"WE_HAND_TOOLS {built}/{len(BUILDERS)}")
    unreal.SystemLibrary.quit_editor()


main()
