"""
Phase 17.5: the neighbourhood around the workshop - an American backyard lot in a run-down industrial area.
Used by build_garage_map.py (the game world) and build_menu_map.py (the main menu backdrop).

  - a street along the south side (cracked asphalt, centre line, kerbs and sidewalks, manholes, hydrant),
    power poles with sagging wires (one drop line to the garage) and street lamps,
  - the company lot fenced with chain-link (gate to the street for the van),
  - neighbours: a row of brick and concrete buildings across the street, a corrugated warehouse to the west,
    a container yard and a long warehouse behind the lot, a box-profile hall to the east,
  - clutter: dumpsters, trash cans and bags, a covered car, pallets, drums, barriers, cones, a barrel stove,
  - growth: props tagged WEUpgrade.<Id> appear with the expansion upgrades (pallet stacks, containers, signs).

Everything is static and uses only CC0 models (Poly Haven), our generated models (SM_Gen_*) and the
world-projected surface materials, so large boxes tile correctly.
Coordinates: garage interior centre = origin, garage door in +X, street to the south (-Y).
"""
import math

import unreal

eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
eal = unreal.EditorAssetLibrary

CUBE = unreal.load_asset("/Engine/BasicShapes/Cube.Cube")
CYLINDER = unreal.load_asset("/Engine/BasicShapes/Cylinder.Cylinder")
PLANE = unreal.load_asset("/Engine/BasicShapes/Plane.Plane")

STREET_Y = -1850.0     # centre line of the street
STREET_HALF = 450.0    # half width of the road
WALK = 250.0           # sidewalk width
LOT = (-1300.0, 2650.0, -1170.0, 2700.0)  # fenced company lot: x0, x1, y0, y1
GATE = (430.0, 1150.0)  # gap in the south fence (x range)

_count = {"actors": 0}


def surface(kind):
    path = f"/Game/WorkshopEmpire/Materials/Surfaces/MI_WE{kind}"
    return eal.load_asset(path) if eal.does_asset_exist(path) else None


def prop_material(name):
    path = f"/Game/WorkshopEmpire/Materials/Props/MI_Prop_{name}"
    return eal.load_asset(path) if eal.does_asset_exist(path) else None


def _spawn_mesh(mesh, loc, rot=(0.0, 0.0, 0.0), scale=(1.0, 1.0, 1.0), label=None, folder="Neighbourhood", tags=None, collide=True, shadow=True, material=None):
    if not mesh:
        return None
    a = eas.spawn_actor_from_class(unreal.StaticMeshActor, unreal.Vector(*loc), unreal.Rotator(roll=rot[2], pitch=rot[0], yaw=rot[1]))
    if label:
        a.set_actor_label(label)
    a.set_folder_path(folder)
    c = a.static_mesh_component
    c.set_static_mesh(mesh)
    c.set_editor_property("mobility", unreal.ComponentMobility.STATIC)
    a.set_actor_scale3d(unreal.Vector(*scale))
    if material:
        for i in range(max(1, c.get_num_materials())):
            c.set_material(i, material)
    if not collide:
        c.set_collision_enabled(unreal.CollisionEnabled.NO_COLLISION)
    if not shadow:
        c.set_editor_property("cast_shadow", False)
    if tags:
        a.set_editor_property("tags", list(tags))
    _count["actors"] += 1
    return a


def model(name):
    for path in (f"/Game/WorkshopEmpire/Art/Models/SM_{name}", f"/Game/WorkshopEmpire/Art/Generated/SM_Gen_{name}"):
        if eal.does_asset_exist(path):
            return eal.load_asset(path)
    return None


def place(name, loc, yaw=0.0, scale=1.0, tags=None, collide=True, folder="Neighbourhood"):
    return _spawn_mesh(model(name), loc, (0.0, yaw, 0.0), (scale, scale, scale), f"N_{name}", folder, tags, collide)


def block(label, center, size, kind, yaw=0.0, tags=None, collide=True, folder="Neighbourhood"):
    """A box of size (x, y, z) standing on center.z, with a world-projected surface."""
    return _spawn_mesh(CUBE, (center[0], center[1], center[2] + size[2] * 0.5), (0.0, yaw, 0.0), (size[0] / 100.0, size[1] / 100.0, size[2] / 100.0),
                       label, folder, tags, collide, True, surface(kind) or prop_material("PlasticGrey"))


def flat(label, center, size, mat, z=0.5, folder="Neighbourhood"):
    """A thin horizontal plate (road surfaces, markings)."""
    return _spawn_mesh(CUBE, (center[0], center[1], z), (0.0, 0.0, 0.0), (size[0] / 100.0, size[1] / 100.0, 0.01), label, folder, None, False, False, mat)


def _part(pack, name):
    path = f"/Game/WorkshopEmpire/Art/Import/{pack}/{pack}/StaticMeshes/{name}"
    return eal.load_asset(path) if eal.does_asset_exist(path) else None


def _pack_part(pack, name, target, yaw, pivot, folder="Neighbourhood", tags=None, collide=False):
    """Pack parts keep their position inside the pack: shift by the pivot so the pivot lands on target."""
    mesh = _part(pack, name)
    if not mesh:
        return None
    r = math.radians(yaw)
    px = pivot[0] * math.cos(r) - pivot[1] * math.sin(r)
    py = pivot[0] * math.sin(r) + pivot[1] * math.cos(r)
    return _spawn_mesh(mesh, (target[0] - px, target[1] - py, target[2]), (0.0, yaw, 0.0), (1.0, 1.0, 1.0), None, folder, tags, collide)


# --- Street ---------------------------------------------------------------------------------------------
def _split(x0, x1, gaps):
    """[x0, x1] without the gap ranges (intersections)."""
    parts, start = [], x0
    for g0, g1 in sorted(gaps):
        if g1 <= start or g0 >= x1:
            continue
        if g0 > start:
            parts.append((start, g0))
        start = max(start, g1)
    if start < x1:
        parts.append((start, x1))
    return parts


def street(length=12000.0, gaps=()):
    """Main street along X; gaps = x ranges of crossing streets (no kerb, sidewalk or centre line there)."""
    road = surface("WornAsphalt") or surface("Asphalt")
    flat("Street", (0.0, STREET_Y), (length, STREET_HALF * 2.0), road, z=0.6)
    yellow = prop_material("PlasticYellow")
    for i in range(int(length / 600.0)):
        x = -length * 0.5 + 300.0 + i * 600.0
        if any(g0 - 150.0 < x < g1 + 150.0 for g0, g1 in gaps):
            continue
        flat(f"StreetLine{i}", (x, STREET_Y), (300.0, 12.0), yellow, z=1.2)
    walk = surface("Sidewalk") or surface("ConcreteFloor")
    for side, y in (("N", STREET_Y + STREET_HALF + WALK * 0.5), ("S", STREET_Y - STREET_HALF - WALK * 0.5)):
        # Raised sidewalk with a concrete kerb along the road edge; lowered in front of the lot gate.
        segments = _split(-length * 0.5, length * 0.5, list(gaps) + ([GATE] if side == "N" else []))
        for j, (x0, x1) in enumerate(segments):
            block(f"Sidewalk{side}{j}", ((x0 + x1) * 0.5, y, 0.0), (x1 - x0, WALK, 12.0), "Sidewalk" if walk else "ConcreteFloor")
            ky = y - WALK * 0.5 if side == "N" else y + WALK * 0.5
            block(f"Kerb{side}{j}", ((x0 + x1) * 0.5, ky, 0.0), (x1 - x0, 16.0, 14.0), "ConcreteWall")
        if side == "N":
            flat("Driveway", ((GATE[0] + GATE[1]) * 0.5, y), (GATE[1] - GATE[0], WALK), walk, z=1.0)
    for x, y in ((-1700.0, STREET_Y - 120.0), (900.0, STREET_Y + 160.0), (3300.0, STREET_Y - 80.0)):
        place("water_manhole_cover", (x, y, 0.8), 15.0, collide=False)
    place("fire_hydrant", (260.0, STREET_Y + STREET_HALF + 60.0, 12.0), 90.0)
    for x in (-2300.0, 1900.0):
        place("metal_trash_can", (x, STREET_Y + STREET_HALF + 200.0, 12.0), 180.0)
    # A short roadwork corner: cones around a manhole.
    for k, (dx, dy) in enumerate(((-80, -70), (70, -80), (90, 60), (-60, 80))):
        place("TrafficCone", (3300.0 + dx, STREET_Y - 80.0 + dy, 0.8), k * 37.0)


# --- Power lines and street lamps -----------------------------------------------------------------------
POLE_Y = STREET_Y + STREET_HALF + WALK - 40.0
WIRE_Z = 575.0


def power_pole(x, transformer=False, folder="Neighbourhood"):
    """A complete pole from the modular pack (preset 1 with transformer, preset 2 without), arms across the street."""
    pack = "modular_electricity_poles"
    prefix, pivot_x = ("preset_01_", -250.0) if transformer else ("preset_02_", -450.0)
    mesh_paths = [p for p in eal.list_assets(f"/Game/WorkshopEmpire/Art/Import/{pack}/{pack}/StaticMeshes", False) if p.split("/")[-1].split(".")[0].startswith(prefix)]
    for p in mesh_paths:
        name = p.split("/")[-1].split(".")[0]
        _pack_part(pack, name, (x, POLE_Y, 0.0), 90.0, (pivot_x, 0.0), folder, collide=name.endswith("_pole"))


def wire(a, b, sag=60.0, segments=7, folder="Neighbourhood", thickness=1.3):
    mat = prop_material("Rubber")
    pts = []
    for i in range(segments + 1):
        t = i / segments
        pts.append(unreal.Vector(a[0] + (b[0] - a[0]) * t, a[1] + (b[1] - a[1]) * t, a[2] + (b[2] - a[2]) * t - sag * 4.0 * t * (1.0 - t)))
    for i in range(segments):
        p, q = pts[i], pts[i + 1]
        d = q - p
        length = d.length()
        mid = (p + q) * 0.5
        rot = unreal.MathLibrary.make_rot_from_z(d.normal())
        _spawn_mesh(CYLINDER, (mid.x, mid.y, mid.z), (rot.pitch, rot.yaw, rot.roll), (thickness / 100.0, thickness / 100.0, length / 100.0), None, folder, None, False, False, mat)


def power_lines(xs, garage_drop):
    for i, x in enumerate(xs):
        power_pole(x, transformer=(i % 3 == 1))
    for x0, x1 in zip(xs[:-1], xs[1:]):
        for dy in (-48.0, 0.0, 48.0):
            wire((x0, POLE_Y + dy, WIRE_Z + (8.0 if dy == 0.0 else 0.0)), (x1, POLE_Y + dy, WIRE_Z + (8.0 if dy == 0.0 else 0.0)), sag=70.0)
    if garage_drop:
        # Service drop from the nearest pole to the garage's front corner.
        pole_x, target = garage_drop
        wire((pole_x, POLE_Y + 20.0, WIRE_Z - 90.0), target, sag=45.0, segments=8, thickness=1.1)


def street_lamps(xs, lights=True, folder="Neighbourhood"):
    """Galvanised pole with a long arm and a cobra-head luminaire over the road (US style)."""
    steel = prop_material("SteelBrushed")
    head = prop_material("MetalPaintedGrey") or steel
    lens = prop_material("PlasticWhite")
    base_y = STREET_Y + STREET_HALF + 35.0
    for x in xs:
        _spawn_mesh(CYLINDER, (x, base_y, 12.0 + 400.0), (0, 0, 0), (0.16, 0.16, 8.0), None, folder, None, True, True, steel)
        _spawn_mesh(CYLINDER, (x, base_y, 12.0 + 12.0), (0, 0, 0), (0.3, 0.3, 0.24), None, folder, None, True, True, head)
        # Arm: rises a little towards the road.
        arm_from = unreal.Vector(x, base_y, 790.0)
        arm_to = unreal.Vector(x, base_y - 210.0, 815.0)
        d = arm_to - arm_from
        rot = unreal.MathLibrary.make_rot_from_z(d.normal())
        mid = (arm_from + arm_to) * 0.5
        _spawn_mesh(CYLINDER, (mid.x, mid.y, mid.z), (rot.pitch, rot.yaw, rot.roll), (0.07, 0.07, d.length() / 100.0), None, folder, None, False, True, steel)
        _spawn_mesh(CUBE, (x, base_y - 235.0, 812.0), (0, 0, 0), (0.32, 0.62, 0.12), None, folder, None, False, True, head)
        _spawn_mesh(CUBE, (x, base_y - 238.0, 805.0), (0, 0, 0), (0.24, 0.44, 0.02), None, folder, None, False, False, lens)
        if lights:
            l = eas.spawn_actor_from_class(unreal.SpotLight, unreal.Vector(x, base_y - 238.0, 798.0), unreal.Rotator(roll=0.0, pitch=-90.0, yaw=0.0))
            l.set_folder_path(folder)
            lc = l.get_component_by_class(unreal.SpotLightComponent)
            lc.set_editor_property("mobility", unreal.ComponentMobility.STATIONARY)
            lc.set_editor_property("intensity_units", unreal.LightUnits.LUMENS)
            lc.set_intensity(2600.0)
            lc.set_attenuation_radius(1500.0)
            lc.set_editor_property("outer_cone_angle", 66.0)
            lc.set_light_color(unreal.LinearColor(1.0, 0.72, 0.42, 1.0))  # sodium-ish
            lc.set_editor_property("cast_shadows", False)
            l.set_editor_property("tags", ["StreetLight"])  # Phase 19: on from dusk to dawn

# --- Fence --------------------------------------------------------------------------------------------
def fence_line(a, b, gap=None, folder="Fence"):
    """Chain-link panels (2 m) with posts from a to b (axis aligned); gap = (from, to) along the line is left open."""
    pack = "modular_chainlink_fence"
    dx, dy = b[0] - a[0], b[1] - a[1]
    length = math.hypot(dx, dy)
    ux, uy = dx / length, dy / length
    yaw = math.degrees(math.atan2(uy, ux))
    n = int(round(length / 200.0))
    step = length / n
    for i in range(n + 1):
        s = i * step
        if gap and gap[0] + 1.0 < s < gap[1] - 1.0:
            continue
        _pack_part(pack, "modular_chainlink_fence_post", (a[0] + ux * s, a[1] + uy * s, 0.0), yaw, (-100.0, -100.0), folder, collide=True)
        if i == n:
            break
        mid = s + step * 0.5
        if gap and gap[0] < mid < gap[1]:
            continue
        p = _pack_part(pack, "modular_chainlink_fence_double", (a[0] + ux * mid, a[1] + uy * mid, 0.0), yaw, (-99.5, 0.0), folder, collide=True)
        if p and abs(step - 200.0) > 2.0:
            p.set_actor_scale3d(unreal.Vector(step / 200.0, 1.0, 1.0))


MENU_ONLY = [False]


def lot_fence():
    x0, x1, y0, y1 = LOT
    fence_line((x0, y0), (x1, y0), gap=(GATE[0] - x0, GATE[1] - x0))
    fence_line((x1, y0), (x1, y1))
    fence_line((x1, y1), (x0, y1))
    fence_line((x0, y1), (x0, y0))
    # Gate posts get concrete blocks, the company sign stands beside the gate.
    place("ConcreteBarrier", (GATE[0] - 170.0, y0 + 60.0, 0.0), 0.0)
    place("ConcreteBarrier", (GATE[1] + 170.0, y0 + 60.0, 0.0), 0.0)
    sign_x = GATE[0] - 60.0
    place("CompanySign", (sign_x, y0 - 40.0, 0.0), 0.0)
    # Phase 20: the company logo and name instead of a fixed "WERKSTATT" (follows the brand from the Ferrit OS).
    if not MENU_ONLY[0]:
        logo = eas.spawn_actor_from_class(unreal.WELogoActor, unreal.Vector(sign_x, y0 - 40.0 - 7.0, 292.0), unreal.Rotator(roll=0.0, pitch=0.0, yaw=-90.0))
        logo.set_folder_path("Neighbourhood")
        comp = logo.get_component_by_class(unreal.WELogoComponent)
        comp.set_editor_property("size", 30.0)
        comp.set_editor_property("name_height", 18.0)
        sign_text(sign_x, y0 - 40.0 - 6.0, ("", "ANKAUF · REPARATUR · RECYCLING"))
    else:
        sign_text(sign_x, y0 - 40.0 - 6.0, ("WERKSTATT", "ANKAUF · REPARATUR · RECYCLING"))


def sign_text(x, y, lines, tags=None, folder="Neighbourhood"):
    """Painted letters on the company sign (facing the street, -Y)."""
    sizes = (34.0, 14.0)
    for i, line in enumerate(lines):
        t = eas.spawn_actor_from_class(unreal.TextRenderActor, unreal.Vector(x, y, 262.0 - i * 34.0), unreal.Rotator(roll=0.0, pitch=0.0, yaw=-90.0))
        t.set_folder_path(folder)
        comp = t.text_render
        comp.set_text(line)
        comp.set_editor_property("horizontal_alignment", unreal.HorizTextAligment.EHTA_CENTER)
        comp.set_editor_property("vertical_alignment", unreal.VerticalTextAligment.EVRTA_TEXT_CENTER)
        comp.set_editor_property("world_size", sizes[min(i, 1)])
        comp.set_text_render_color(unreal.Color(r=140, g=24, b=18, a=255) if i == 0 else unreal.Color(r=40, g=40, b=44, a=255))
        if tags:
            t.set_editor_property("tags", list(tags))


# --- Neighbour buildings ---------------------------------------------------------------------------------
def facade_building(label, x0, x1, front_y, depth, height, wall, facing=1.0, shutters=0, windows=True, roof_edge=True, extras=True):
    """A closed building block; facing +1: the front looks to +Y (towards the street from the south side)."""
    cx = (x0 + x1) * 0.5
    cy = front_y - facing * depth * 0.5
    block(label, (cx, cy, 0.0), (x1 - x0, depth, height), wall)
    fy = front_y + facing * 1.0
    dark = prop_material("Glass") or prop_material("PlasticBlack")
    frame = prop_material("TrimBlack") or prop_material("PlasticBlack")
    width = x1 - x0
    # Roof parapet / edge.
    if roof_edge:
        block(label + "Parapet", (cx, front_y - facing * 15.0, height), (width + 20.0, 30.0, 40.0), "ConcreteWall")
    # Rows of windows (dark glass in a frame), skipping the ground floor where the shutters are.
    if windows:
        floors = max(1, int((height - 120.0) // 300.0))
        cols = max(1, int(width // 320.0))
        for f in range(floors):
            z = 330.0 + f * 300.0
            if z + 160.0 > height - 30.0:
                break
            for c in range(cols):
                wx = x0 + (c + 0.5) * width / cols
                _spawn_mesh(CUBE, (wx, fy + facing * 2.0, z + 80.0), (0, 0, 0), (1.5, 0.06, 1.6), None, "Neighbourhood", None, False, False, frame)
                _spawn_mesh(CUBE, (wx, fy + facing * 4.5, z + 80.0), (0, 0, 0), (1.34, 0.04, 1.44), None, "Neighbourhood", None, False, False, dark)
    # Ground floor: roller shutters (closed shops / loading bays) with a lamp above each.
    for s in range(shutters):
        sx = x0 + (s + 0.5) * width / shutters
        sh = _spawn_mesh(model("rollershutter_door"), (sx, fy + facing * 6.0, 0.0), (0.0, 0.0 if facing > 0 else 180.0, 0.0), (2.4, 1.0, 1.15), None, "Neighbourhood", None, True)
        place("industrial_wall_lamp", (sx, fy + facing * 6.0, 320.0), 90.0 if facing > 0 else -90.0, collide=False)
    if extras:
        # Downpipes at both ends, an AC box and a meter box.
        for px in (x0 + 25.0, x1 - 25.0):
            _spawn_mesh(CYLINDER, (px, fy + facing * 8.0, height * 0.5), (0, 0, 0), (0.08, 0.08, height / 100.0), None, "Neighbourhood", None, False, True, prop_material("SteelBrushed"))
        place("utility_box_02", (x0 + width * 0.18, fy + facing * 25.0, 0.0), 0.0 if facing > 0 else 180.0)
        place("GasMeterBox", (x1 - width * 0.12, fy + facing * 20.0, 0.0), 0.0 if facing > 0 else 180.0)


def neighbours():
    south_front = STREET_Y - STREET_HALF - WALK - 10.0
    facade_building("Nb_South1", -4600.0, -2950.0, south_front, 1200.0, 650.0, "DarkBrick", 1.0, shutters=2)
    facade_building("Nb_South2", -2900.0, -1350.0, south_front, 1000.0, 430.0, "RedBrick", 1.0, shutters=3)
    facade_building("Nb_South3", -1300.0, 300.0, south_front, 900.0, 380.0, "PaintedBrick", 1.0, shutters=2)
    facade_building("Nb_South4", 350.0, 2150.0, south_front, 1300.0, 820.0, "ConcreteWall", 1.0, shutters=1)
    facade_building("Nb_South5", 2200.0, 4300.0, south_front, 1100.0, 520.0, "RedBrick", 1.0, shutters=3)
    # Old car repair shop's lot across the street: a covered car and junk in front of Nb_South3.
    place("covered_car", (-600.0, south_front - 260.0 + 200.0, 0.0), 90.0)
    # West: corrugated warehouse with a gravel strip, its loading side facing the lot.
    block("Nb_WestGravel", (-1700.0, 800.0, 0.0), (700.0, 4200.0, 2.0), "Gravel", collide=False)
    block("Nb_West", (-2900.0, 600.0, 0.0), (2200.0, 3000.0, 620.0), "WornCorrugated")
    for k, y in enumerate((-300.0, 500.0, 1300.0)):
        _spawn_mesh(model("rollershutter_door"), (-1797.0, y, 0.0), (0.0, 90.0, 0.0), (2.8, 1.0, 1.3), None, "Neighbourhood", None, True)
    # North: container yard behind the lot and a long brick warehouse as the backdrop.
    for k, (x, y, yaw, z, kind) in enumerate([(-600.0, 3150.0, 0.0, 0.0, "ContainerBlue"), (100.0, 3150.0, 0.0, 0.0, "ContainerRed"),
                                              (-250.0, 3150.0, 4.0, 259.0, "ContainerRed"), (900.0, 3200.0, 90.0, 0.0, "ContainerBlue"),
                                              (1500.0, 3150.0, 0.0, 0.0, "ContainerRed"), (1500.0, 3150.0, -3.0, 259.0, "ContainerBlue"),
                                              (2300.0, 3180.0, 0.0, 0.0, "ContainerBlue")]):
        place(kind, (x, y, z), yaw)
    block("Nb_NorthYard", (900.0, 3300.0, 0.0), (5200.0, 800.0, 2.0), "Gravel", collide=False)
    block("Nb_North", (1000.0, 4400.0, 0.0), (6400.0, 1400.0, 760.0), "RedBrick")
    block("Nb_NorthParapet", (1000.0, 3710.0, 760.0), (6420.0, 30.0, 40.0), "ConcreteWall")
    # East: box-profile hall.
    block("Nb_East", (3700.0, 900.0, 0.0), (1800.0, 3400.0, 760.0), "BoxProfile")
    for y in (200.0, 1400.0):
        _spawn_mesh(model("rollershutter_door"), (2797.0, y, 0.0), (0.0, 90.0, 0.0), (3.2, 1.0, 1.6), None, "Neighbourhood", None, True)
        place("industrial_wall_lamp", (2797.0, y, 420.0), -90.0, collide=False)


# --- Clutter on the lot ------------------------------------------------------------------------------------
def lot_clutter():
    # West part of the lot (behind/beside the garage): bins, a covered car, the burn barrel, junk.
    place("Dumpster", (-1080.0, 380.0, 0.0), 90.0)
    place("Dumpster", (-1080.0, 650.0, 0.0), 95.0)
    for k, (x, y) in enumerate(((-950.0, 260.0), (-990.0, 210.0), (-930.0, 840.0))):
        place("trashbag", (x, y, 0.0), k * 60.0)
    place("metal_trash_can", (-560.0, 330.0, 0.0), 10.0)
    place("covered_car", (-850.0, -350.0, 0.0), 8.0)
    place("barrel_stove", (-760.0, 1150.0, 0.0), 0.0)
    place("old_tyre", (-700.0, 1260.0, 30.0), 0.0)
    place("wooden_crate_02", (-620.0, 1420.0, 0.0), 30.0)
    place("PalletStack", (-1050.0, 1050.0, 0.0), 90.0)
    place("EmptyPallet", (-1080.0, 1250.0, 0.0), 80.0)
    place("EmptyPallet", (-1070.0, 1260.0, 12.0), 95.0)
    place("DrumPallet", (-560.0, 700.0, 0.0), 0.0)
    place("GasMeterBox", (-430.0, 160.0, 0.0), 90.0)
    place("security_camera_01", (424.0, -290.0, 290.0), 0.0, collide=False)
    place("planter_box_01", (480.0, -1100.0, 0.0), 0.0)
    place("TrafficCone", (980.0, -1060.0, 0.0), 0.0)
    place("TrafficCone", (1040.0, -1010.0, 0.0), 40.0)
    # Front of the workshop extension's wall (stays outside its build area).
    place("utility_box_02", (475.0, -620.0, 0.0), 90.0)


def growth_props():
    """Props that appear with the company's expansion (hidden by the progression until the upgrade is owned)."""
    ext = ["WEUpgrade.WorkshopExtension"]
    hall = ["WEUpgrade.CommercialHall"]
    factory = ["WEUpgrade.FactoryHall"]
    place("PalletStack", (-700.0, -950.0, 0.0), 0.0, tags=ext, folder="Growth")
    place("PalletStack", (-700.0, -950.0, 82.0), 8.0, tags=ext, folder="Growth")
    place("EmptyPallet", (-850.0, -950.0, 0.0), 0.0, tags=ext, folder="Growth")
    place("DrumPallet", (-900.0, -760.0, 0.0), 0.0, tags=ext, folder="Growth")
    place("ContainerBlue", (-950.0, 1750.0, 0.0), 90.0, tags=hall, folder="Growth")
    place("ContainerRed", (-650.0, 1750.0, 0.0), 90.0, tags=hall, folder="Growth")
    place("DrumPallet", (-600.0, 2250.0, 0.0), 0.0, tags=hall, folder="Growth")
    place("DrumPallet", (-470.0, 2250.0, 0.0), 0.0, tags=hall, folder="Growth")
    place("CompanySign", (GATE[1] + 60.0, LOT[2] - 40.0, 0.0), 0.0, tags=hall, folder="Growth")
    sign_text(GATE[1] + 60.0, LOT[2] - 46.0, ("LOGISTIK", "ANLIEFERUNG · ABHOLUNG"), tags=hall, folder="Growth")
    place("Dumpster", (2500.0, 650.0, 0.0), 0.0, tags=factory, folder="Growth")
    place("ConcreteBarrier", (2200.0, 2600.0, 0.0), 0.0, tags=factory, folder="Growth")
    place("PalletStack", (800.0, 2550.0, 0.0), 0.0, tags=factory, folder="Growth")
    place("PalletStack", (950.0, 2550.0, 0.0), 0.0, tags=factory, folder="Growth")
    place("ContainerRed", (-950.0, 2380.0, 0.0), 90.0, tags=factory, folder="Growth")


def build_all(menu=False):
    MENU_ONLY[0] = menu
    import world_city
    street(world_city.MAIN_LENGTH, world_city.main_street_gaps())
    xs = [-4000.0, -2400.0, -800.0, 1500.0, 3000.0, 4500.0]  # none in front of the gate (the van drives out there)
    power_lines(xs, garage_drop=(-800.0, (-424.0, -300.0, 300.0)))
    street_lamps([-3200.0, -1600.0, 0.0, 1600.0, 3200.0], lights=True)
    lot_fence()
    neighbours()
    lot_clutter()
    if not menu:
        growth_props()
    import os
    if not os.environ.get("WE_NO_CITY"):
        world_city.build_city(menu)
    unreal.log(f"WE_NEIGHBOURHOOD actors={_count['actors']}")
    return _count["actors"]
