"""
Phase 20: models for the workshop atmosphere (no downloads, Geometry Script like build_models.py):
  SM_Gen_CoffeeMachine   filter coffee machine with glass jug on the hot plate and a mug (origin bottom centre)
  SM_Gen_CatBody         "Schraube", the workshop cat: ginger tabby body (pivot = body centre)
  SM_Gen_CatHead         head with ears, eyes, muzzle and whiskers (pivot = neck)
  SM_Gen_CatLeg          leg with paw (pivot = hip / shoulder, hangs down 16 cm)
  SM_Gen_CatTail         curved tail (pivot = tail root, points backwards along -X)
The cat parts keep their pivots (save(normalize=False)); AWEWorkshopCat animates them.
Also creates the prop material instances the parts need (MI_Prop_FurGinger, FurGingerDark, FurCream, NosePink, CatEye, Coffee).

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

# name: (tint, roughness, emissive)
FUR = {
    "FurGinger": ((0.48, 0.22, 0.07), 0.92, None),
    "FurGingerDark": ((0.24, 0.09, 0.025), 0.92, None),
    "FurCream": ((0.78, 0.64, 0.46), 0.9, None),
    "NosePink": ((0.62, 0.30, 0.30), 0.6, None),
    "CatEye": ((0.30, 0.48, 0.08), 0.12, (0.015, 0.025, 0.0)),
    "Coffee": ((0.045, 0.022, 0.01), 0.15, None),
}


def make_materials():
    parent = unreal.load_asset("/Game/WorkshopEmpire/Materials/M_WEProp")
    for name, (tint, rough, emissive) in FUR.items():
        path = f"{bm.PROPS}/MI_Prop_{name}"
        mi = eal.load_asset(path) if eal.does_asset_exist(path) else tools.create_asset(
            f"MI_Prop_{name}", bm.PROPS, unreal.MaterialInstanceConstant, unreal.MaterialInstanceConstantFactoryNew())
        mi.set_editor_property("parent", parent)
        mel.set_material_instance_vector_parameter_value(mi, "Tint", unreal.LinearColor(*tint, 1.0))
        mel.set_material_instance_scalar_parameter_value(mi, "Roughness", rough)
        mel.set_material_instance_scalar_parameter_value(mi, "Metallic", 0.0)
        if emissive:
            mel.set_material_instance_vector_parameter_value(mi, "Emissive", unreal.LinearColor(*emissive, 1.0))
        eal.save_asset(path)


def coffee_machine():
    m = Model("CoffeeMachine")
    m.box("PlasticBlack", (0, 0, 1.5), (24.0, 28.0, 3.0), bevel=0.8, segments=2)             # base with hot plate
    m.box("PlasticBlack", (0, 8.0, 18.0), (22.0, 12.0, 33.0), bevel=1.2, segments=2)          # rear column
    m.box("PlasticBlack", (0, -1.0, 34.0), (22.0, 30.0, 5.0), bevel=1.2, segments=2)          # top with filter
    m.box("GlassClear", (0, 14.2, 19.0), (17.0, 1.6, 22.0), bevel=0.3)                         # water tank
    m.box("SteelBrushed", (0, -6.0, 31.0), (11.0, 11.0, 1.2))                                  # filter holder
    m.cyl("SteelBrushed", (0, -5.0, 3.4), 7.2, 0.8, segs=28)                                   # hot plate
    m.cyl("Coffee", (0, -5.0, 7.6), 6.2, 7.6, segs=28)                                         # coffee in the jug
    m.cyl("GlassClear", (0, -5.0, 10.4), 6.6, 13.2, segs=28)                                   # glass jug
    m.cyl("PlasticBlack", (0, -5.0, 17.3), 6.9, 1.4, segs=28)                                  # lid
    m.box("PlasticBlack", (0, -12.6, 10.5), (1.6, 1.6, 9.0), bevel=0.4)                       # handle
    m.box("PlasticBlack", (0, -11.6, 14.6), (1.6, 3.0, 1.4))
    m.box("PlasticBlack", (0, -11.6, 6.4), (1.6, 3.0, 1.4))
    m.box("PlasticRed", (8.0, -14.1, 1.5), (3.2, 0.5, 1.6), bevel=0.2)                        # switch
    m.box("LedRed", (4.5, -14.1, 1.5), (0.8, 0.4, 0.8))
    # A mug beside the machine.
    m.cyl("PlasticWhite", (17.5, -8.0, 4.5), 3.8, 9.0, segs=24)
    m.cyl("Coffee", (17.5, -8.0, 8.85), 3.4, 0.4, segs=24)
    m.torus("PlasticWhite", (21.6, -8.0, 4.8), 2.2, 0.55, rot=(90, 0, 0), segs=16, minor=6)
    return m


def cat_body():
    m = Model("CatBody")
    m.sphere("FurGinger", (0, 0, 0), 1.0, scale=(18.0, 7.8, 8.2), steps=20)
    m.sphere("FurGinger", (9.0, 0, 0.5), 1.0, scale=(9.0, 7.4, 8.0), steps=16)                # chest
    m.sphere("FurGinger", (-10.0, 0, 0.3), 1.0, scale=(9.0, 8.2, 8.4), steps=16)              # haunches
    m.sphere("FurGinger", (15.5, 0, 3.0), 1.0, scale=(5.0, 5.2, 6.0), steps=12)               # neck
    m.sphere("FurCream", (4.0, 0, -4.0), 1.0, scale=(13.0, 6.0, 4.6), steps=16)               # belly
    m.sphere("FurCream", (13.0, 0, -1.0), 1.0, scale=(4.0, 5.0, 5.5), steps=12)               # chest bib
    for x in (-13.0, -7.5, -2.0, 3.5, 9.0):                                                    # tabby stripes
        m.sphere("FurGingerDark", (x, 0, 2.0), 1.0, scale=(1.6, 7.9, 6.3), steps=12)
    return m


def cat_head():
    m = Model("CatHead")
    m.sphere("FurGinger", (5.0, 0, 3.0), 1.0, scale=(6.4, 6.4, 5.8), steps=18)
    m.sphere("FurGinger", (6.0, 0, 1.0), 1.0, scale=(5.5, 7.2, 4.4), steps=14)                # cheeks
    m.sphere("FurCream", (10.4, 0, 0.6), 1.0, scale=(2.8, 3.9, 2.5), steps=12)                # muzzle
    m.sphere("FurCream", (9.6, 0, -1.4), 1.0, scale=(2.4, 2.6, 1.6), steps=10)                # chin
    m.sphere("NosePink", (13.1, 0, 1.7), 1.0, scale=(0.7, 1.0, 0.7), steps=8)
    for s in (-1.0, 1.0):
        m.sphere("CatEye", (9.6, s * 2.9, 4.0), 1.0, scale=(0.9, 1.35, 1.35), steps=12)
        m.sphere("PlasticBlack", (10.35, s * 3.0, 4.0), 1.0, scale=(0.3, 0.32, 1.05), steps=8)   # slit pupils
        m.cone("FurGinger", (3.5, s * 3.9, 9.4), 2.9, 0.25, 5.2, rot=(s * -16.0, -8.0, 0), segs=12)    # ears
        m.cone("NosePink", (4.1, s * 3.8, 9.2), 1.9, 0.2, 3.8, rot=(s * -16.0, -8.0, 0), segs=10)
        m.sphere("FurGingerDark", (3.0, s * 2.0, 7.6), 1.0, scale=(1.0, 0.5, 0.9), steps=6)    # forehead marks
        for k, pitch in ((0, 84.0), (1, 96.0)):
            m.cyl("PlasticWhite", (11.0, s * 5.4, 0.9 - k * 0.7), 0.06, 7.0, rot=(0, pitch, s * 72.0), segs=4)  # whiskers
    return m


def cat_leg():
    m = Model("CatLeg")
    m.cyl("FurGinger", (0, 0, -7.5), 1.9, 15.0, segs=12)
    m.sphere("FurGinger", (0, 0, -1.0), 1.0, scale=(2.6, 2.2, 3.0), steps=10)                 # thigh / shoulder
    m.sphere("FurCream", (0.8, 0, -14.6), 1.0, scale=(2.5, 2.2, 1.4), steps=10)              # paw
    return m


def cat_tail():
    m = Model("CatTail")
    pts = [(0.0, 0.0), (-8.0, 0.4), (-15.5, 2.0), (-21.5, 5.0), (-25.5, 9.0)]
    radii = [1.7, 1.55, 1.4, 1.25, 1.1]
    for i in range(len(pts) - 1):
        (x0, z0), (x1, z1) = pts[i], pts[i + 1]
        dx, dz = x1 - x0, z1 - z0
        length = math.hypot(dx, dz)
        pitch = math.degrees(math.atan2(-dx, dz))  # turns the cylinder's Z axis onto the segment
        mat = "FurGingerDark" if i % 2 else "FurGinger"
        m.cyl(mat, ((x0 + x1) / 2, 0, (z0 + z1) / 2), radii[i], length, rot=(0, pitch, 0), segs=10)
        m.sphere(mat, (x0, 0, z0), radii[i], steps=8)
    m.sphere("FurGingerDark", (pts[-1][0], 0, pts[-1][1]), radii[-1], steps=8)              # dark tip
    return m


def main():
    make_materials()
    built = 0
    for make, normalize in ((coffee_machine, True), (cat_body, False), (cat_head, False), (cat_leg, False), (cat_tail, False)):
        try:
            if make().save(normalize=normalize):
                built += 1
        except Exception as e:  # keep going, report at the end
            unreal.log_error(f"WE_MODEL {make.__name__} failed: {e}")
    unreal.log(f"WE_ATMOSPHERE_MODELS {built}/5")
    unreal.SystemLibrary.quit_editor()


main()
