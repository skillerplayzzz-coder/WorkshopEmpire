"""
Phase 17.5: procedural models built with Geometry Script (no downloads): devices and materials of the item list,
the workshop PC (tower, monitor, keyboard, mouse), the company vans and props of the industrial neighbourhood.
Every model is made of bevelled boxes, cylinders, tori and lathed profiles with material slots for the M_WEProp
instances (build_prop_material.py). Output: /Game/WorkshopEmpire/Art/Generated/SM_Gen_<Name> (real size in cm,
origin at the bottom centre, box collision).

Run after build_prop_material.py (overwrites the generated meshes):
  UnrealEditor-Cmd.exe <uproject> -ExecutePythonScript="<this file>" -unattended -RenderOffscreen -nosplash -nosteam
  Optional: -WEModels=Name1,Name2 to rebuild only some.
"""
import math
import sys

import unreal

P = unreal.GeometryScript_Primitives
E = unreal.GeometryScript_MeshEdits
M = unreal.GeometryScript_Materials
U = unreal.GeometryScript_UVs
N = unreal.GeometryScript_Normals
B = unreal.GeometryScript_MeshBooleans
MOD = unreal.GeometryScript_MeshModeling
NEW = unreal.GeometryScript_NewAssetUtils
eal = unreal.EditorAssetLibrary
sme = unreal.get_editor_subsystem(unreal.StaticMeshEditorSubsystem)
CENTER = unreal.GeometryScriptPrimitiveOriginMode.CENTER
BASE = unreal.GeometryScriptPrimitiveOriginMode.BASE
OUT = "/Game/WorkshopEmpire/Art/Generated"
PROPS = "/Game/WorkshopEmpire/Materials/Props"


def mat_path(key):
    return f"{PROPS}/MI_Prop_{key}"


def xf(loc=(0, 0, 0), rot=(0, 0, 0), scale=(1, 1, 1)):
    """rot = (roll, pitch, yaw) in degrees."""
    return unreal.Transform(unreal.Vector(*loc), unreal.Rotator(roll=rot[0], pitch=rot[1], yaw=rot[2]), unreal.Vector(*scale))


IDENT = xf()


def popts(material_id=0):
    o = unreal.GeometryScriptPrimitiveOptions()
    o.set_editor_property("polygroup_mode", unreal.GeometryScriptPrimitivePolygroupMode.PER_FACE)
    o.set_editor_property("material_id", material_id)
    return o


def fix_orientation(mesh):
    """Polygon winding decides which way an extrusion faces; an inside-out part gets turned the right way."""
    result = unreal.GeometryScript_MeshQueries.get_mesh_volume_area(mesh)
    volume = result[1] if isinstance(result, tuple) else 0.0
    if volume < 0.0:
        N.flip_normals(mesh)


CURRENT_ID = [0]  # material id of the part being built (cutters use it too, so cut faces keep the part's material)


def bool_opts():
    o = unreal.GeometryScriptMeshBooleanOptions()
    o.set_editor_property("fill_holes", True)
    o.set_editor_property("allow_empty_result", True)
    return o


class Model:
    def __init__(self, name):
        self.name = name
        self.mesh = unreal.DynamicMesh()
        self.mats = []

    # --- parts ---------------------------------------------------------------------------------------------
    def _mid(self, key):
        if key not in self.mats:
            self.mats.append(key)
        return self.mats.index(key)

    def _finish(self, part, mat, bevel=0.0, cuts=None, segments=1):
        if bevel > 0.0:
            bo = unreal.GeometryScriptMeshBevelOptions()
            bo.set_editor_property("bevel_distance", bevel)
            bo.set_editor_property("subdivisions", segments)
            try:
                MOD.apply_mesh_polygroup_bevel(part, bo)
            except Exception as e:  # bevel is cosmetic
                unreal.log_warning(f"WE_MODEL bevel failed on {self.name}: {e}")
        for cutter in cuts or []:
            B.apply_mesh_boolean(part, IDENT, cutter, IDENT, unreal.GeometryScriptBooleanOperation.SUBTRACT, bool_opts())
        E.append_mesh(self.mesh, part, IDENT)
        return part

    def box(self, mat, c, s, rot=(0, 0, 0), bevel=0.0, cuts=None, segments=1):
        part = unreal.DynamicMesh()
        CURRENT_ID[0] = self._mid(mat)
        P.append_box(part, popts(CURRENT_ID[0]), xf(c, rot), s[0], s[1], s[2], 0, 0, 0, CENTER)
        return self._finish(part, mat, bevel, cuts, segments)

    def cyl(self, mat, c, r, h, rot=(0, 0, 0), segs=24, bevel=0.0, cuts=None):
        """Cylinder along its local Z, centred on c."""
        part = unreal.DynamicMesh()
        CURRENT_ID[0] = self._mid(mat)
        P.append_cylinder(part, popts(CURRENT_ID[0]), xf(c, rot), r, h, segs, 0, True, CENTER)
        return self._finish(part, mat, bevel, cuts)

    def cone(self, mat, c, r0, r1, h, rot=(0, 0, 0), segs=24):
        part = unreal.DynamicMesh()
        CURRENT_ID[0] = self._mid(mat)
        P.append_cone(part, popts(CURRENT_ID[0]), xf(c, rot), r0, r1, h, segs, 0, True, CENTER)
        return self._finish(part, mat)

    def torus(self, mat, c, big, small, rot=(0, 0, 0), segs=32, minor=12):
        part = unreal.DynamicMesh()
        CURRENT_ID[0] = self._mid(mat)
        P.append_torus(part, popts(CURRENT_ID[0]), xf(c, rot), unreal.GeometryScriptRevolveOptions(), big, small, segs, minor, CENTER)
        return self._finish(part, mat)

    def sphere(self, mat, c, r, scale=(1, 1, 1), steps=16):
        part = unreal.DynamicMesh()
        CURRENT_ID[0] = self._mid(mat)
        P.append_sphere_lat_long(part, popts(CURRENT_ID[0]), xf(c, (0, 0, 0), scale), r, steps, steps, CENTER)
        return self._finish(part, mat)

    def extrude(self, mat, pts, h, c=(0, 0, 0), rot=(0, 0, 0), bevel=0.0, cuts=None):
        """Polygon in local XY, extruded along local Z by h (from the base)."""
        part = unreal.DynamicMesh()
        CURRENT_ID[0] = self._mid(mat)
        P.append_simple_extrude_polygon(part, popts(CURRENT_ID[0]), xf(c, rot), [unreal.Vector2D(x, y) for x, y in pts], h, 0, True, BASE)
        fix_orientation(part)
        return self._finish(part, mat, bevel, cuts)

    def revolve(self, mat, profile, c=(0, 0, 0), rot=(0, 0, 0), steps=32):
        """Profile points (radius, height) lathed around local Z."""
        part = unreal.DynamicMesh()
        CURRENT_ID[0] = self._mid(mat)
        P.append_revolve_path(part, popts(CURRENT_ID[0]), xf(c, rot), [unreal.Vector2D(r, z) for r, z in profile], unreal.GeometryScriptRevolveOptions(), steps, True)
        fix_orientation(part)
        return self._finish(part, mat)

    # --- cutters (for booleans) -------------------------------------------------------------------------------
    @staticmethod
    def cut_box(c, s, rot=(0, 0, 0)):
        m = unreal.DynamicMesh()
        P.append_box(m, popts(0), xf(c, rot), s[0], s[1], s[2], 0, 0, 0, CENTER)
        return m

    @staticmethod
    def cut_cyl(c, r, h, rot=(0, 0, 0), segs=24):
        m = unreal.DynamicMesh()
        P.append_cylinder(m, popts(0), xf(c, rot), r, h, segs, 0, True, CENTER)
        return m

    # --- output ---------------------------------------------------------------------------------------------
    def save(self, nanite=False):
        # Origin at the bottom centre, whatever the parts did.
        bb = unreal.GeometryScript_MeshQueries.get_mesh_bounding_box(self.mesh)
        unreal.GeometryScript_MeshTransforms.translate_mesh(self.mesh, unreal.Vector(-(bb.min.x + bb.max.x) * 0.5, -(bb.min.y + bb.max.y) * 0.5, -bb.min.z))
        box = unreal.Transform(unreal.Vector(0, 0, 0), unreal.Rotator(0, 0, 0), unreal.Vector(100, 100, 100))
        U.set_mesh_u_vs_from_box_projection(self.mesh, 0, box, unreal.GeometryScriptMeshSelection(), 2)
        so = unreal.GeometryScriptSplitNormalsOptions()
        so.set_editor_property("split_by_opening_angle", True)
        so.set_editor_property("opening_angle_deg", 35.0)
        N.compute_split_normals(self.mesh, so, unreal.GeometryScriptCalculateNormalsOptions())
        path = f"{OUT}/SM_Gen_{self.name}"
        if eal.does_asset_exist(path):
            eal.delete_asset(path)
        opts = unreal.GeometryScriptCreateNewStaticMeshAssetOptions()
        opts.set_editor_property("enable_recompute_normals", False)
        opts.set_editor_property("enable_recompute_tangents", True)
        opts.set_editor_property("enable_collision", False)
        opts.set_editor_property("enable_nanite", nanite)
        result = NEW.create_new_static_mesh_asset_from_mesh(self.mesh, path, opts)
        sm = result[0] if isinstance(result, tuple) else result
        if not sm:
            unreal.log_error(f"WE_MODEL {self.name}: asset not created")
            return None
        for i, key in enumerate(self.mats):
            mat = unreal.load_asset(mat_path(key))
            if mat:
                sm.set_material(i, mat)
        sme.remove_collisions(sm)
        sme.add_simple_collisions(sm, unreal.ScriptCollisionShapeType.BOX)
        eal.save_asset(path)
        b = sm.get_bounding_box()
        unreal.log(f"WE_MODEL {self.name} mats={len(self.mats)} size=({b.max.x - b.min.x:.0f},{b.max.y - b.min.y:.0f},{b.max.z - b.min.z:.0f})")
        return sm


# =============================================================================================================
# Devices and materials (items)
# =============================================================================================================
def old_computer():
    m = Model("OldComputer")
    w, d, h = 19.0, 44.0, 42.0  # x = width, y = depth, z = height; front faces -Y
    vents = [Model.cut_box((w / 2, 6 + i * 3.2, 26), (2.0, 1.4, 14)) for i in range(5)]
    m.box("PlasticBeige", (0, 0, h / 2), (w, d, h), bevel=0.8, cuts=vents)
    # Front bezel with two 5.25" bays, a 3.5" floppy, power button and LEDs.
    m.box("PlasticBeige", (0, -d / 2 - 0.6, h / 2), (w - 0.6, 1.4, h - 0.6), bevel=0.4)
    for z in (36.0, 31.0):
        m.box("PlasticGrey", (0, -d / 2 - 1.4, z), (14.6, 0.6, 4.2), bevel=0.2)
        m.box("PlasticBlack", (0, -d / 2 - 1.75, z - 0.6), (8.0, 0.2, 0.5))
    m.box("PlasticGrey", (0, -d / 2 - 1.35, 25.0), (10.0, 0.5, 2.8), bevel=0.15)
    m.box("PlasticBlack", (-1.0, -d / 2 - 1.65, 25.2), (6.5, 0.2, 0.35))
    m.cyl("PlasticGrey", (5.0, -d / 2 - 1.5, 12.0), 1.3, 1.0, rot=(90, 0, 0), segs=20)
    m.box("LedGreen", (-5.0, -d / 2 - 1.4, 14.0), (0.8, 0.4, 0.4))
    m.box("LedAmber", (-5.0, -d / 2 - 1.4, 12.5), (0.8, 0.4, 0.4))
    for i in range(6):  # ventilation slits at the bottom of the bezel
        m.box("PlasticGrey", (0, -d / 2 - 1.35, 3.0 + i * 1.1), (13.0, 0.3, 0.45))
    # Back: PSU fan grill, slot covers, feet.
    m.box("SteelBrushed", (0, d / 2 + 0.25, h / 2), (w - 1.0, 0.5, h - 1.0))
    m.torus("SteelBrushed", (-3.0, d / 2 + 0.6, 34.0), 3.6, 0.25, rot=(90, 0, 0), segs=24, minor=6)
    m.torus("SteelBrushed", (-3.0, d / 2 + 0.6, 34.0), 2.2, 0.22, rot=(90, 0, 0), segs=20, minor=6)
    for i in range(5):
        m.box("SteelBrushed", (5.0, d / 2 + 0.6, 6.0 + i * 2.4), (1.6, 0.4, 2.0))
    for x in (-w / 2 + 2, w / 2 - 2):
        for y in (-d / 2 + 3, d / 2 - 3):
            m.cyl("Rubber", (x, y, 0.4), 1.2, 0.8, segs=12)
    return m


def power_supply():
    m = Model("PowerSupply")
    w, d, h = 15.0, 14.0, 8.6
    m.box("SteelBrushed", (0, 0, h / 2), (w, d, h), bevel=0.25)
    m.cyl("PlasticBlack", (0, 0, h + 0.05), 5.6, 0.3, segs=32)
    for r in (5.2, 4.0, 2.8, 1.6):
        m.torus("Chrome", (0, 0, h + 0.3), r, 0.12, segs=28, minor=6)
    m.box("Chrome", (0, 0, h + 0.3), (11.0, 0.25, 0.25))
    m.box("Chrome", (0, 0, h + 0.3), (0.25, 11.0, 0.25))
    m.box("PlasticBlack", (-w / 2 - 0.2, -3.5, h / 2), (0.4, 4.5, 2.6))  # power socket
    m.box("PlasticRed", (-w / 2 - 0.25, 3.0, h / 2), (0.4, 1.4, 1.0))  # switch
    for i, (y, z) in enumerate([(2.0, 3.0), (3.3, 3.6), (4.5, 2.6), (2.6, 5.2)]):  # cable bundle
        m.cyl("PlasticBlack", (w / 2 + 6.0, y, z), 0.45, 12.0, rot=(0, 90, 0), segs=8)
    m.box("PlasticBlack", (w / 2 + 12.5, 3.2, 3.6), (2.4, 4.6, 2.0))
    return m


def graphics_card(name="GraphicsCard", shroud="PlasticBlack", accent="PlasticRed"):
    m = Model(name)
    m.box("PCB", (0, 0, 0.8), (24.0, 0.16, 11.0))  # card stands on its PCB edge (y = thickness)
    m.box(shroud, (1.0, 1.4, 0.8 + 0.0), (22.0, 2.6, 10.0), bevel=0.3)
    m.box(accent, (1.0, 2.75, 4.6), (22.0, 0.2, 0.6))
    for x in (-4.0, 6.0):
        m.cyl("PlasticBlack", (x, 2.8, 0.8), 3.9, 0.3, rot=(90, 0, 0), segs=28)
        m.cyl("PlasticGrey", (x, 2.9, 0.8), 1.2, 0.4, rot=(90, 0, 0), segs=16)
        for k in range(7):
            m.box("PlasticGrey", (x, 2.95, 0.8), (6.8, 0.15, 0.6), rot=(0, k * 180 / 7, 0))
    m.box("SteelBrushed", (-12.4, 0.8, 0.8), (0.3, 2.2, 12.0))  # slot bracket
    for i in range(14):  # gold contacts
        m.box("Gold", (-4.0 + i * 0.6, 0.0, -4.95), (0.35, 0.2, 0.9))
    # Lift so the bottom is z = 0.
    m.mesh = translate(m.mesh, (0, 0, 5.4))
    return m


def translate(mesh, d):
    unreal.GeometryScript_MeshTransforms.translate_mesh(mesh, unreal.Vector(*d))
    return mesh


def ram_module():
    m = Model("RamModule")
    m.box("PCB", (0, 0, 1.6), (13.3, 0.12, 3.1))
    for i in range(8):
        m.box("Chip", (-5.6 + i * 1.6, 0.13, 1.9), (1.1, 0.14, 1.0))
    for i in range(30):
        m.box("Gold", (-6.2 + i * 0.42, 0.0, 0.15), (0.26, 0.14, 0.3))
    m.box("PCB", (2.0, 0, 0.1), (0.3, 0.12, 0.25))  # notch area
    return m


def mainboard():
    m = Model("Mainboard")
    m.box("PCB", (0, 0, 0.1), (30.5, 24.4, 0.2))
    m.box("SteelBrushed", (2.0, 3.0, 0.9), (4.6, 4.6, 1.4), bevel=0.15)  # CPU socket
    m.box("Aluminium", (2.0, 3.0, 2.6), (5.6, 5.6, 2.0))
    for i in range(9):
        m.box("Aluminium", (2.0 - 2.4 + i * 0.6, 3.0, 4.6), (0.25, 5.6, 2.2))
    for i in range(4):  # RAM slots
        m.box("PlasticBlack", (9.5 + i * 1.1, 3.0, 0.6), (0.7, 13.5, 0.9))
    for i in range(5):  # expansion slots
        m.box("PlasticGrey" if i % 2 else "PlasticBlack", (-3.0, -4.0 - i * 2.0, 0.55), (17.0, 0.7, 0.9))
    m.box("Aluminium", (-9.0, 7.0, 1.4), (3.5, 3.5, 2.6))  # chipset cooler
    for i in range(12):  # capacitors
        m.cyl("PlasticBlack", (-12.0 + (i % 6) * 1.2, 10.5 - (i // 6) * 1.2, 1.1), 0.45, 1.8, segs=10)
    m.box("SteelBrushed", (-14.5, 2.0, 2.2), (1.5, 15.0, 4.0))  # I/O shield block
    m.box("PlasticBlack", (13.0, -9.0, 0.9), (4.0, 1.2, 1.4))  # ATX connector
    return m


def electric_motor(name="ElectricMotor", body_mat="MetalPaintedBlue", r=8.5, length=24.0):
    m = Model(name)
    cz = r + 2.5
    m.cyl(body_mat, (0, 0, cz), r, length, rot=(0, 90, 0), segs=32)
    for i in range(11):  # cooling fins
        x = -length / 2 + 2.0 + i * (length - 4.0) / 10
        m.cyl(body_mat, (x, 0, cz), r + 0.9, 0.5, rot=(0, 90, 0), segs=32)
    for sx in (-1, 1):  # end shields
        m.cyl("MetalPaintedGrey", (sx * (length / 2 + 1.2), 0, cz), r - 0.8, 2.4, rot=(0, 90, 0), segs=28, bevel=0.2)
    m.cyl("Chrome", (length / 2 + 5.0, 0, cz), 1.1, 6.0, rot=(0, 90, 0), segs=16)  # shaft
    m.box("Chrome", (length / 2 + 6.5, 0, cz + 1.0), (3.0, 0.4, 0.4))  # key
    m.cyl("PlasticBlack", (-length / 2 - 3.0, 0, cz), r - 2.0, 1.4, rot=(0, 90, 0), segs=24)  # fan cover
    for sy in (-1, 1):  # feet
        m.box(body_mat, (0, sy * (r - 2.0), 1.2), (length * 0.7, 4.0, 2.4), bevel=0.2)
    m.box("MetalPaintedGrey", (0, 0, cz + r + 2.2), (7.0, 7.0, 4.4), bevel=0.4)  # terminal box
    m.cyl("PlasticBlack", (0, 4.0, cz + r + 1.6), 0.9, 2.0, rot=(90, 0, 0), segs=12)
    return m


def scrap_pile(name, mats, seed, count=9, size=20.0):
    import random
    rnd = random.Random(seed)
    m = Model(name)
    for i in range(count):
        mat = mats[i % len(mats)]
        a, b = rnd.uniform(0.3, 1.0) * size, rnd.uniform(0.2, 0.7) * size
        z = rnd.uniform(0.1, 0.5) * size * 0.5
        c = (rnd.uniform(-0.3, 0.3) * size, rnd.uniform(-0.3, 0.3) * size, z)
        rot = (rnd.uniform(-35, 35), rnd.uniform(-30, 30), rnd.uniform(0, 180))
        if i % 3 == 2:
            m.cyl(mat, c, rnd.uniform(0.8, 1.8), a, rot=(rot[0], 90, rot[2]), segs=10)
        else:
            m.box(mat, c, (a, b, rnd.uniform(0.3, 0.8)), rot=rot)
    return m


def big_bag(name, fill_mat, size=60.0):
    m = Model(name)
    m.box("PlasticWhite", (0, 0, size * 0.45), (size, size, size * 0.9), bevel=size * 0.08, segments=2)
    m.sphere(fill_mat, (0, 0, size * 0.9), size * 0.42, scale=(1, 1, 0.35), steps=20)
    for sx in (-1, 1):
        for sy in (-1, 1):
            m.torus("PlasticWhite", (sx * size * 0.42, sy * size * 0.42, size * 0.95), 3.0, 0.8, rot=(90, 0, 45 * sx * sy), segs=16, minor=6)
    return m


def bale(name, mat, size=(60.0, 45.0, 40.0)):
    m = Model(name)
    m.box(mat, (0, 0, size[2] / 2), size, bevel=1.5)
    for x in (-size[0] * 0.3, size[0] * 0.3):  # steel straps
        m.box("SteelBrushed", (x, 0, size[2] / 2), (1.8, size[1] + 0.6, size[2] + 0.6))
    return m


def ingot(name, mat):
    m = Model(name)
    m.extrude(mat, [(-12, -4), (12, -4), (10, 4), (-10, 4)], 4.5, c=(0, 0, 0), rot=(-90, 0, 0), bevel=0.2)
    m.mesh = translate(m.mesh, (0, 2.25, 0))
    return m


def wire_spool():
    m = Model("CopperWireSpool")
    m.cyl("Wood", (0, 0, 1.0), 13.0, 2.0, segs=32)
    m.cyl("Wood", (0, 0, 21.0), 13.0, 2.0, segs=32)
    m.cyl("Copper", (0, 0, 11.0), 10.5, 18.0, segs=36)
    for i in range(9):
        m.torus("Copper", (0, 0, 2.6 + i * 2.0), 10.6, 0.9, segs=32, minor=8)
    m.cyl("Cardboard", (0, 0, 11.0), 3.0, 22.4, segs=20)
    return m


def cable_coil(name="CopperCable", mat="PlasticBlack", plug=False):
    m = Model(name)
    for i in range(5):
        m.torus(mat, (i * 0.3, 0, 1.0 + i * 0.95), 9.0 - i * 0.25, 0.55, segs=36, minor=8)
    if plug:
        m.box("PlasticWhite", (11.0, 0, 1.0), (4.0, 3.2, 2.2), bevel=0.4)
        m.cyl("Chrome", (13.4, -0.7, 1.0), 0.25, 1.6, rot=(0, 90, 0), segs=8)
        m.cyl("Chrome", (13.4, 0.7, 1.0), 0.25, 1.6, rot=(0, 90, 0), segs=8)
    else:
        m.cyl("Copper", (10.0, 0, 1.0), 0.5, 3.0, rot=(0, 90, 0), segs=8)
    return m


def bracket_set():
    m = Model("SteelBracketSet")
    for i in range(6):
        y = -6 + i * 2.4
        m.box("SteelBrushed", (0, y, 0.25), (12.0, 2.0, 0.5))
        m.box("SteelBrushed", (5.75, y, 3.0), (0.5, 2.0, 6.0))
    m.box("Cardboard", (0, 0, 7.0), (14.0, 15.5, 0.3))
    return m


def screw_box():
    m = Model("SheetMetalScrews")
    m.box("Cardboard", (0, 0, 2.5), (10.0, 6.0, 5.0), bevel=0.15)
    m.box("PlasticWhite", (0, -3.02, 2.8), (7.0, 0.1, 2.6))
    for i in range(10):  # a few screws on top
        x, y = -3.5 + (i % 5) * 1.6, -1.2 + (i // 5) * 2.2
        m.cyl("Chrome", (x, y, 5.6), 0.35, 1.8, rot=(0, 90, 20 * i), segs=8)
        m.cyl("Chrome", (x - 0.9, y, 5.6), 0.65, 0.3, rot=(0, 90, 20 * i), segs=10)
    return m


def capacitors():
    m = Model("Capacitors")
    m.box("Cardboard", (0, 0, 0.3), (12.0, 8.0, 0.6))
    colors = ["PlasticBlack", "MetalPaintedBlue", "PlasticBlack", "PCB"]
    for i in range(12):
        x, y = -4.5 + (i % 4) * 3.0, -2.5 + (i // 4) * 2.5
        r = 0.6 + (i % 3) * 0.25
        m.cyl(colors[i % 4], (x, y, 0.6 + 1.2 + r * 0.4), r, 2.4 + r, segs=14)
        m.cyl("Aluminium", (x, y, 0.6 + 2.4 + r * 1.4), r * 0.95, 0.15, segs=14)
    return m


def gold_cpu():
    m = Model("GoldCpu")
    m.box("Chip", (0, 0, 0.25), (5.0, 5.0, 0.5))
    m.box("Gold", (0, 0, 0.55), (2.6, 2.6, 0.12))
    for i in range(14):
        for j in range(14):
            if 3 <= i <= 10 and 3 <= j <= 10:
                continue
            m.cyl("Gold", (-2.2 + i * 0.34, -2.2 + j * 0.34, -0.2), 0.05, 0.5, segs=6)
    m.mesh = translate(m.mesh, (0, 0, 0.45))
    return m


def vintage_radio():
    m = Model("VintageRadio")
    m.box("Wood", (0, 0, 14.0), (42.0, 18.0, 28.0), bevel=2.0, segments=2)
    m.box("Cardboard", (-6.0, -9.05, 15.0), (24.0, 0.2, 18.0), bevel=0.5)  # speaker cloth
    for i in range(7):
        m.box("Wood", (-6.0, -9.2, 7.5 + i * 2.6), (24.0, 0.4, 0.4))
    m.box("PlasticWhite", (12.0, -9.1, 20.0), (12.0, 0.3, 5.0), bevel=0.3)  # dial
    m.box("PlasticRed", (12.0, -9.3, 20.0), (0.3, 0.2, 4.0))
    for x in (9.0, 15.0):
        m.cyl("PlasticBeige", (x, -9.6, 10.0), 1.6, 1.4, rot=(90, 0, 0), segs=20, bevel=0.2)
    for x in (-18.0, 18.0):
        m.cyl("Wood", (x, 0, 0.4), 1.4, 0.8, segs=12)
    return m


def soldering_iron(name="SolderingIron", station=False):
    m = Model(name)
    if station:
        m.box("PlasticGrey", (0, 0, 4.5), (16.0, 12.0, 9.0), bevel=0.8)
        m.box("ScreenDark", (-2.0, -6.05, 5.5), (8.0, 0.2, 3.0))
        m.box("LedRed", (-2.0, -6.2, 5.5), (5.0, 0.1, 1.6))
        m.cyl("PlasticBlack", (5.0, -6.3, 4.5), 1.4, 1.0, rot=(90, 0, 0), segs=16)
        m.cyl("SteelBrushed", (14.0, 0, 6.0), 2.4, 10.0, rot=(0, -25, 0), segs=16)  # holder spiral
        for i in range(8):
            m.torus("SteelBrushed", (11.5 + i * 0.9, 0, 3.6 + i * 0.5), 2.0, 0.15, rot=(0, -25 + 90, 0), segs=16, minor=6)
    off = (0, 0, 2.0) if not station else (0, 9.0, 1.5)
    m.cyl("PlasticBlack", (off[0] - 4.0, off[1], off[2]), 1.3, 10.0, rot=(0, 90, 0), segs=16, bevel=0.2)
    m.cyl("SteelBrushed", (off[0] + 4.5, off[1], off[2]), 0.5, 7.0, rot=(0, 90, 0), segs=12)
    m.cone("Copper", (off[0] + 9.0, off[1], off[2]), 0.35, 0.05, 2.0, rot=(0, 90, 0), segs=10)
    m.cyl("PlasticBlack", (off[0] - 10.5, off[1], off[2]), 0.4, 3.0, rot=(0, 90, 0), segs=8)
    return m


def pallet(name, load=None):
    m = Model(name)
    for x in (-50, 0, 50):  # bearers
        m.box("Wood", (x, 0, 4.9), (10.0, 80.0, 9.8))
    for y in (-35, 0, 35):
        m.box("Wood", (0, y, 0.9), (120.0, 10.0, 1.8))
    for i in range(7):
        m.box("Wood", (0, -37.5 + i * 12.5, 11.0), (120.0, 9.5, 2.2))
    if load == "brackets":
        for i in range(8):
            m.box("Cardboard", (-36 + (i % 4) * 24, -20 + (i // 4) * 40, 24.0), (22.0, 36.0, 24.0), bevel=0.6)
        m.box("PlasticWhite", (0, 0, 36.2), (100.0, 78.0, 0.4))
    elif load == "cables":
        for i in range(6):
            mm = (-40 + (i % 3) * 40, -18 + (i // 3) * 36)
            for k in range(4):
                m.torus("PlasticBlack" if i % 2 else "PlasticYellow", (mm[0], mm[1], 14.0 + k * 2.6), 15.0, 1.4, segs=28, minor=8)
    return m


def bucket(name="PreciousConcentrate"):
    m = Model(name)
    m.revolve("PlasticWhite", [(0, 0), (11.0, 0), (13.0, 24.0), (13.6, 24.6), (13.6, 26.0), (0, 26.0)], steps=36)
    m.torus("PlasticWhite", (0, 0, 18.0), 12.6, 0.5, segs=32, minor=6)
    m.box("PlasticYellow", (0, -12.9, 12.0), (12.0, 0.6, 8.0), rot=(0, 0, 0))
    m.torus("SteelBrushed", (0, 0, 26.0), 13.2, 0.3, rot=(90, 0, 0), segs=24, minor=6)
    return m


def plastic_parts():
    return scrap_pile("PlasticParts", ["PlasticBlack", "PlasticGrey", "PlasticWhite", "PlasticRed"], 7, 10, 18.0)


def glass_cullet():
    return scrap_pile("GlassCullet", ["GlassClear", "Glass"], 3, 12, 14.0)


def e_scrap():
    return scrap_pile("ElectronicScrap", ["PCB", "Chip", "PlasticBlack", "Copper"], 11, 12, 18.0)


# =============================================================================================================
# Workshop PC (Ferrit OS station)
# =============================================================================================================
def pc_tower():
    m = Model("PCTower")
    w, d, h = 20.0, 42.0, 44.0
    m.box("PlasticBlack", (0, 0, h / 2), (w, d, h), bevel=0.6)
    m.box("PlasticGrey", (0, -d / 2 - 0.4, h / 2), (w - 1.0, 0.8, h - 1.0), bevel=0.3)
    m.box("PlasticBlack", (0, -d / 2 - 0.9, 36.0), (14.0, 0.3, 4.0), bevel=0.15)
    m.cyl("PlasticBlack", (0, -d / 2 - 0.95, 28.0), 1.4, 0.5, rot=(90, 0, 0), segs=20)
    m.torus("LedGreen", (0, -d / 2 - 1.2, 28.0), 1.45, 0.12, rot=(90, 0, 0), segs=24, minor=6)
    for i in range(10):
        m.box("PlasticBlack", (0, -d / 2 - 0.9, 4.0 + i * 1.6), (15.0, 0.25, 0.7))
    m.box("Glass", (w / 2 + 0.05, 0, h / 2 + 1.0), (0.3, d - 6.0, h - 8.0))
    return m


def monitor():
    m = Model("Monitor")
    # Screen panel: 52 x 30 cm visible, slot "ScreenDark" is replaced by the live screen at runtime.
    m.box("PlasticBlack", (0, 0, 38.0), (56.0, 3.2, 34.0), bevel=0.6)
    m.box("ScreenDark", (0, -1.62, 38.6), (52.0, 0.1, 29.4))
    m.box("PlasticBlack", (0, -1.7, 22.3), (8.0, 0.2, 1.2))  # logo-free chin badge
    m.box("LedGreen", (24.0, -1.7, 21.8), (0.6, 0.15, 0.4))
    m.box("PlasticBlack", (0, 4.0, 18.0), (8.0, 3.0, 33.0), bevel=0.4)  # neck
    m.extrude("PlasticBlack", [(-12, -10), (12, -10), (14, 8), (-14, 8)], 1.4, c=(0, 2.0, 0), bevel=0.3)
    return m


def keyboard():
    m = Model("Keyboard")
    m.box("PlasticBlack", (0, 0, 1.3), (44.0, 14.5, 2.6), rot=(4, 0, 0), bevel=0.4)
    rows = [15, 14, 14, 13, 12]
    for r, count in enumerate(rows):
        width = 41.0 / count
        for k in range(count):
            x = -20.5 + width * (k + 0.5)
            m.box("PlasticGrey", (x, -5.4 + r * 2.5, 2.9 + r * 0.17), (width - 0.45, 2.0, 0.9), bevel=0.15)
    m.box("PlasticGrey", (0, -5.4 - 2.5, 2.75), (18.0, 2.0, 0.9), bevel=0.15)  # space bar row
    return m


def mouse():
    m = Model("Mouse")
    m.sphere("PlasticBlack", (0, 0, 1.6), 3.2, scale=(0.95, 1.65, 0.6), steps=24)
    m.box("PlasticBlack", (0, 0, 0.5), (5.8, 9.5, 1.0), bevel=0.4)
    m.cyl("PlasticGrey", (0, -2.6, 3.4), 0.45, 0.35, rot=(0, 90, 0), segs=12)
    return m


# =============================================================================================================
# Vans (body + separate wheel; the vehicle actor turns the wheels)
# =============================================================================================================
def van(name, paint, old):
    m = Model(name)
    L, W = 520.0, 200.0
    # Side profile (x forward = +X, z up), extruded across the width.
    prof = [(-260, 32), (-260, 238), (-235, 248), (95, 248), (128, 238), (178, 150), (240, 128), (258, 112), (262, 60), (255, 32)]
    half = W / 2
    arches = [Model.cut_cyl((x, 0, 40.0), 41.0, W + 10, rot=(90, 0, 0), segs=32) for x in (-165.0, 165.0)]
    windows = [
        Model.cut_box((150, half, 190), (60, 12, 62), rot=(0, 0, 0)),
        Model.cut_box((150, -half, 190), (60, 12, 62)),
        Model.cut_box((60, half, 196), (70, 12, 58)),
        Model.cut_box((60, -half, 196), (70, 12, 58)),
    ]
    body = m.extrude(paint, prof, W, c=(0, half, 0), rot=(-90, 0, 0), bevel=3.0, cuts=arches + windows)
    # Windscreen and side windows (glass set slightly in).
    m.extrude("Glass", [(130, 236), (176, 152), (178, 150), (128, 238)], W - 18, c=(0, half - 9, 0), rot=(-90, 0, 0))
    for y in (half - 2.5, -half + 2.5):
        m.box("Glass", (150, y, 190), (58, 2, 60))
        m.box("Glass", (60, y, 196), (68, 2, 56))
    # Door seams, sliding door rail, handles.
    for y in (half + 0.4, -half - 0.4):
        for x in (110.0, 22.0, -110.0):
            m.box("TrimBlack", (x, y, 140), (1.2, 1.0, 200))
        m.box("TrimBlack", (-40, y, 162), (150, 1.0, 2.0))
        m.box("TrimBlack", (90, y, 120), (14, 2.4, 3.0), bevel=0.6)
        m.box("TrimBlack", (-10, y, 120), (14, 2.4, 3.0), bevel=0.6)
    m.box("TrimBlack", (-261, 0, 140), (1.0, 1.2, 200))  # rear doors split
    m.box("TrimBlack", (-262, -20, 130), (2.4, 12.0, 3.0), bevel=0.6)
    # Bumpers, grille, lights, plates.
    m.box("TrimBlack" if old else paint, (262, 0, 46), (16, W + 4, 22), bevel=3.0)
    m.box("TrimBlack", (-262, 0, 44), (14, W + 4, 20), bevel=3.0)
    m.box("TrimBlack", (262, 0, 92), (6, 120, 28), bevel=1.5)
    for i in range(4):
        m.box("Chrome" if not old else "SteelBrushed", (265.5, 0, 82 + i * 6), (1.0, 112, 1.6))
    for y in (-72, 72):
        m.box("Headlight", (262, y, 96), (5, 34, 18), bevel=1.5)
        m.box("Indicator", (260, y + (14 if y > 0 else -14), 78), (4, 10, 6), bevel=1.0)
        m.box("Taillight", (-262, y, 120), (4, 16, 40), bevel=1.0)
    m.box("PlasticWhite", (270, 0, 48), (1.0, 52, 11))
    m.box("PlasticWhite", (-269, 30, 70), (1.0, 52, 11))
    # Mirrors.
    for y in (half + 18, -half - 18):
        m.box("TrimBlack", (150, y, 176), (6, 4, 26), bevel=1.2)
        m.box("TrimBlack", (156, y * 0.94, 176), (12, 16, 3))
    # Underbody, step, fuel cap.
    m.box("TrimBlack", (0, 0, 22), (430, W - 20, 12))
    m.box("TrimBlack", (100, half + 1, 36), (60, 6, 4))
    m.cyl("TrimBlack", (-120, -half - 0.5, 120), 6.0, 1.0, rot=(90, 0, 0), segs=20)
    if old:
        # Roof rack with ladder, dents marked by rust on the paint material.
        for x in (-200, -100, 0):
            m.box("SteelBrushed", (x, 0, 256), (4, W - 30, 4))
        for y in (-80, 80):
            m.box("SteelBrushed", (-100, y, 252), (220, 4, 4))
            for x in (-200, 0):
                m.box("SteelBrushed", (x, y, 250), (4, 4, 6))
        m.box("Aluminium", (-100, 30, 262), (200, 10, 3))
        for i in range(10):
            m.box("Aluminium", (-190 + i * 20, 30, 262), (2, 36, 2))
        m.box("RustHeavy", (-200, half + 0.6, 60), (40, 1.0, 18))
    else:
        # Modern: accent stripe, roof spoiler, side indicators.
        for y in (half + 0.5, -half - 0.5):
            m.box("VanNewAccent", (-30, y, 100), (440, 1.0, 10))
        m.box(paint, (-230, 0, 250), (40, W - 10, 4), bevel=1.0)
    return m


def wheel(name="VanWheel"):
    m = Model(name)
    m.revolve("TireRubber", [(26, -11), (37, -11.5), (40.5, -8), (41, 0), (40.5, 8), (37, 11.5), (26, 11)], rot=(90, 0, 0), steps=40)
    m.cyl("SteelBrushed", (0, 0, 0), 26.5, 18.0, rot=(90, 0, 0), segs=32)
    m.cyl("Chrome", (0, -9.5, 0), 9.0, 2.0, rot=(90, 0, 0), segs=24)
    for i in range(5):
        a = math.radians(i * 72)
        m.cyl("Chrome", (math.cos(a) * 6.0, -10.8, math.sin(a) * 6.0), 1.1, 1.6, rot=(90, 0, 0), segs=8)
    for i in range(8):
        a = math.radians(i * 45 + 22.5)
        m.cyl("TrimBlack", (math.cos(a) * 17.0, -9.2, math.sin(a) * 17.0), 3.2, 1.2, rot=(90, 0, 0), segs=12)
    m.mesh = translate(m.mesh, (0, 0, 0))
    return m


# =============================================================================================================
# Neighbourhood props
# =============================================================================================================
def dumpster():
    m = Model("Dumpster")
    L, W, H = 200.0, 120.0, 120.0
    m.extrude("DumpsterGreen", [(-W / 2, 15), (W / 2 + 10, 15), (W / 2 + 18, H), (-W / 2, H)], L, c=(-L / 2, 0, 0), rot=(-90, 0, 90), bevel=1.5)
    for x in (-L / 2 + 6, L / 2 - 6):
        m.box("DumpsterGreen", (x, 0, H / 2 + 8), (4, W + 22, H - 10))
    for i in range(5):
        m.box("DumpsterGreen", (-80 + i * 40, W / 2 + 15, 60), (6, 4, 80))
    m.box("PlasticBlack", (0, -4, H + 4), (L + 4, W + 26, 4), rot=(0, 0, 0), bevel=1.0)  # lid
    for x in (-L / 2 + 20, L / 2 - 20):
        for y in (-W / 2 + 15, W / 2 - 15):
            m.cyl("Rubber", (x, y, 8), 8.0, 6.0, rot=(90, 0, 0), segs=16)
            m.box("SteelBrushed", (x, y, 13), (10, 8, 4))
    for x in (-L / 2 - 4, L / 2 + 4):
        m.cyl("SteelBrushed", (x, 0, 95), 2.5, 40, rot=(90, 0, 0), segs=10)
    return m


def shipping_container(name, mat):
    m = Model(name)
    L, W, H = 605.0, 244.0, 259.0
    m.box(mat, (0, 0, H / 2), (L, W, H), bevel=1.0)
    for i in range(int(L / 18)):  # corrugation on the long sides
        x = -L / 2 + 10 + i * 18
        for y in (W / 2 + 1.5, -W / 2 - 1.5):
            m.box(mat, (x, y, H / 2), (8.0, 3.0, H - 30))
    for x in (-L / 2 + 4, L / 2 - 4):  # corner posts and rails
        for y in (-W / 2 + 4, W / 2 - 4):
            m.box("RustHeavy", (x, y, H / 2), (10, 10, H))
    for z in (8, H - 8):
        for y in (-W / 2 + 3, W / 2 - 3):
            m.box("RustHeavy", (0, y, z), (L, 8, 14))
    # Doors at +X with locking bars.
    m.box(mat, (L / 2 + 1.0, 0, H / 2), (2.0, W - 16, H - 24))
    for y in (-80, -30, 30, 80):
        m.cyl("SteelBrushed", (L / 2 + 4, y, H / 2), 1.6, H - 30, segs=10)
        m.box("SteelBrushed", (L / 2 + 6, y, H / 2 - 20), (2, 4, 20))
    m.box("RustHeavy", (L / 2 + 2, 0, H / 2), (3, 2, H - 24))
    return m


def concrete_barrier():
    m = Model("ConcreteBarrier")
    m.extrude("Concrete", [(-30, 0), (30, 0), (12, 25), (8, 81), (-8, 81), (-12, 25)], 300.0, c=(-150, 0, 0), rot=(-90, 0, 90), bevel=1.5)
    m.mesh = translate(m.mesh, (0, 0, 0))
    return m


def traffic_cone():
    m = Model("TrafficCone")
    m.box("PlasticBlack", (0, 0, 1.5), (36, 36, 3), bevel=1.0)
    m.cone("PlasticRed", (0, 0, 37.0), 13.0, 2.5, 68.0, segs=24)
    m.cone("PlasticWhite", (0, 0, 46.0), 8.3, 7.0, 8.0, segs=24)
    m.cone("PlasticWhite", (0, 0, 28.0), 11.0, 9.8, 7.0, segs=24)
    return m


def oil_drum_stack():
    m = Model("DrumPallet")
    for x in (-25, 25):
        for y in (-25, 25):
            m.cyl("MetalPaintedBlue" if (x + y) % 2 else "MetalPainted", (x, y, 56), 28.5, 88.0, segs=28, bevel=0.6)
            for z in (30, 82):
                m.torus("MetalPaintedGrey", (x, y, z), 28.8, 1.0, segs=28, minor=6)
    m.box("Wood", (0, 0, 6), (120, 120, 12))
    return m


def utility_pole_box():
    m = Model("GasMeterBox")
    m.box("PlasticYellow", (0, 0, 60), (60, 30, 120), bevel=2.0)
    m.box("PlasticYellow", (0, -15.5, 60), (52, 1.5, 100), bevel=0.8)
    m.box("PlasticBlack", (18, -16.5, 80), (6, 2, 8))
    return m


def pallet_stack():
    m = Model("PalletStack")
    for i in range(6):
        z = i * 14.0
        for x in (-50, 0, 50):
            m.box("Wood", (x, 0, z + 4.9), (10.0, 80.0, 9.8), rot=(0, 0, (i % 2) * 1.5))
        for j in range(5):
            m.box("Wood", (0, -32 + j * 16, z + 11.0), (120.0, 10.0, 2.2))
    return m


def street_sign():
    m = Model("CompanySign")
    for x in (-110, 110):
        m.cyl("SteelBrushed", (x, 0, 150), 4.0, 300.0, segs=12)
    m.box("MetalPaintedGrey", (0, 0, 255), (240, 6, 80), bevel=1.5)
    m.box("PlasticWhite", (0, -3.2, 255), (228, 0.5, 70))
    return m


BUILDERS = {
    # items
    "OldComputer": old_computer, "PowerSupply": power_supply, "GraphicsCard": graphics_card,
    "RareGpu": lambda: graphics_card("RareGpu", "MetalPaintedGreen", "Gold"), "RamModule": ram_module, "Mainboard": mainboard,
    "ElectricMotor": electric_motor, "ServoMotor": lambda: electric_motor("ServoMotor", "PlasticBlack", 6.5, 20.0),
    "IndustrialMotorLarge": lambda: electric_motor("IndustrialMotorLarge", "MetalPaintedGreen", 18.0, 48.0),
    "SteelScrap": lambda: scrap_pile("SteelScrap", ["Rust", "RustHeavy", "SteelBrushed"], 5, 10, 34.0),
    "AluminiumScrap": lambda: scrap_pile("AluminiumScrap", ["Aluminium", "SteelBrushed"], 9, 10, 30.0),
    "PlasticParts": plastic_parts, "GlassCullet": glass_cullet, "ElectronicScrap": e_scrap,
    "ShredElectronics": lambda: big_bag("ShredElectronics", "PCB"), "ShredMetal": lambda: big_bag("ShredMetal", "Rust"),
    "CopperGranulate": lambda: big_bag("CopperGranulate", "Copper", 45.0),
    "SteelBale": lambda: bale("SteelBale", "Rust"), "AluBale": lambda: bale("AluBale", "Aluminium"), "CopperBale": lambda: bale("CopperBale", "Copper", (45.0, 35.0, 30.0)),
    "AluminiumIngot": lambda: ingot("AluminiumIngot", "Aluminium"), "CopperIngot": lambda: ingot("CopperIngot", "Copper"),
    "CopperWireSpool": wire_spool, "CopperCable": cable_coil, "ExtensionCable": lambda: cable_coil("ExtensionCable", "PlasticYellow", True),
    "SteelBracketSet": bracket_set, "SheetMetalScrews": screw_box, "Capacitors": capacitors, "GoldCpu": gold_cpu,
    "VintageRadio": vintage_radio, "SolderingIron": soldering_iron, "SolderingStation": lambda: soldering_iron("SolderingStation", True),
    "PalletBrackets": lambda: pallet("PalletBrackets", "brackets"), "PalletCables": lambda: pallet("PalletCables", "cables"),
    "PreciousConcentrate": bucket,
    # workshop PC
    "PCTower": pc_tower, "Monitor": monitor, "Keyboard": keyboard, "Mouse": mouse,
    # vans
    "VanOld": lambda: van("VanOld", "VanOld", True), "VanModern": lambda: van("VanModern", "VanNew", False), "VanWheel": wheel,
    # neighbourhood
    "Dumpster": dumpster, "ContainerRed": lambda: shipping_container("ContainerRed", "Container"),
    "ContainerBlue": lambda: shipping_container("ContainerBlue", "ContainerBlue"), "ConcreteBarrier": concrete_barrier,
    "TrafficCone": traffic_cone, "DrumPallet": oil_drum_stack, "GasMeterBox": utility_pole_box, "PalletStack": pallet_stack,
    "CompanySign": street_sign, "EmptyPallet": lambda: pallet("EmptyPallet"),
}

only = None
for arg in sys.argv[1:] + unreal.SystemLibrary.get_command_line().split():
    if arg.startswith("-WEModels="):
        only = set(arg.split("=", 1)[1].split(","))
built, failed = 0, []
for name, make in BUILDERS.items():
    if only and name not in only:
        continue
    try:
        model = make()
        if model.save():
            built += 1
        else:
            failed.append(name)
    except Exception as e:
        failed.append(name)
        unreal.log_error(f"WE_MODEL {name} failed: {e}")
unreal.log(f"WE_MODELS_BUILT {built} failed={failed}")
unreal.SystemLibrary.quit_editor()
