"""
Phase 18e: the town around the neighbourhood (world_neighbourhood.py builds the street in front of the lot).

  - Two crossing streets (west and east) with intersections: crosswalks, stop lines, traffic lights, lamps.
  - The main street and the crossing streets run on to the horizon; where the playable area ends they are
    closed by a road construction site (barriers, construction fence, signs, a dirt heap). Between the roads
    the border is a lot fence (corrugated sheet walls, chain link) - no invisible wall in the middle of a road.
  - Inside: a gas station, a parking lot with parked cars, a car repair shop, a snack bar, a vacant lot with a
    billboard, parked cars along the kerbs, weeds along walls and fences, a few street trees.
  - Outside: rows of buildings along the roads beyond the closures, warehouses behind the border and a city
    skyline in the distance (simple blocks with window bands - cheap to render).

Everything static. Coordinates as in world_neighbourhood.py (garage centre = origin, main street along X at
STREET_Y). Playable area: BORDER (x0, x1, y0, y1).
"""
import math
import random

import unreal

import world_neighbourhood as wn

MAIN_LENGTH = 32000.0
CROSS_X = (-5600.0, 5800.0)          # centre lines of the crossing streets (they run along Y)
CROSS_LENGTH = 32000.0
HALF = wn.STREET_HALF
WALK = wn.WALK
CLEAR = HALF + WALK                  # road + sidewalk from the centre line
BORDER = (-6450.0, 9300.0, -4550.0, 5450.0)

eas = wn.eas
rng = random.Random(1808)


def main_street_gaps():
    return [(x - CLEAR, x + CLEAR) for x in CROSS_X]


def mat(name):
    return wn.prop_material(name)


def box(label, center, size, material, yaw=0.0, collide=True, shadow=True, folder="City"):
    """Box standing on center.z with a prop material (not world projected)."""
    return wn._spawn_mesh(wn.CUBE, (center[0], center[1], center[2] + size[2] * 0.5), (0.0, yaw, 0.0),
                          (size[0] / 100.0, size[1] / 100.0, size[2] / 100.0), label, folder, None, collide, shadow, mat(material))


def cyl(center, radius, height, material, collide=True, folder="City"):
    return wn._spawn_mesh(wn.CYLINDER, (center[0], center[1], center[2] + height * 0.5), (0, 0, 0),
                          (radius / 50.0, radius / 50.0, height / 100.0), None, folder, None, collide, True, mat(material))


def text(x, y, z, yaw, line, size, color, folder="City"):
    t = eas.spawn_actor_from_class(unreal.TextRenderActor, unreal.Vector(x, y, z), unreal.Rotator(roll=0.0, pitch=0.0, yaw=yaw))
    t.set_folder_path(folder)
    c = t.text_render
    c.set_text(line)
    c.set_editor_property("horizontal_alignment", unreal.HorizTextAligment.EHTA_CENTER)
    c.set_editor_property("vertical_alignment", unreal.VerticalTextAligment.EVRTA_TEXT_CENTER)
    c.set_editor_property("world_size", size)
    c.set_text_render_color(unreal.Color(r=color[0], g=color[1], b=color[2], a=255))
    return t


# --- Crossing streets ----------------------------------------------------------------------------------------
def cross_street(x):
    road = wn.surface("WornAsphalt") or wn.surface("Asphalt")
    wn.flat(f"Cross{x:.0f}", (x, 0.0), (HALF * 2.0, CROSS_LENGTH), road, z=0.62)
    yellow = mat("PlasticYellow")
    white = mat("PlasticWhite")
    gap = (wn.STREET_Y - CLEAR, wn.STREET_Y + CLEAR)
    for i in range(int(CROSS_LENGTH / 600.0)):
        y = -CROSS_LENGTH * 0.5 + 300.0 + i * 600.0
        if gap[0] - 150.0 < y < gap[1] + 150.0:
            continue
        wn.flat(f"CrossLine{x:.0f}_{i}", (x, y), (12.0, 300.0), yellow, z=1.25)
    for side in (-1.0, 1.0):
        wx = x + side * (HALF + WALK * 0.5)
        for j, (y0, y1) in enumerate(wn._split(-CROSS_LENGTH * 0.5, CROSS_LENGTH * 0.5, [gap])):
            wn.block(f"CrossWalk{x:.0f}{side:+.0f}{j}", (wx, (y0 + y1) * 0.5, 0.0), (WALK, y1 - y0, 12.0), "Sidewalk")
            wn.block(f"CrossKerb{x:.0f}{side:+.0f}{j}", (x + side * (HALF + 8.0), (y0 + y1) * 0.5, 0.0), (16.0, y1 - y0, 14.0), "ConcreteWall")
    # Crosswalks (zebra) on all four arms and stop lines in front of them.
    for k in range(9):
        off = -HALF + 50.0 + k * 100.0
        for dy in (-1.0, 1.0):
            wn.flat(f"Zebra{x:.0f}a{k}{dy:+.0f}", (x + off, wn.STREET_Y + dy * (HALF + 170.0)), (50.0, 260.0), white, z=1.3)
        for dx in (-1.0, 1.0):
            wn.flat(f"Zebra{x:.0f}b{k}{dx:+.0f}", (x + dx * (HALF + 170.0), wn.STREET_Y + off), (260.0, 50.0), white, z=1.3)
    for dy in (-1.0, 1.0):
        wn.flat(f"Stop{x:.0f}{dy:+.0f}", (x + dy * HALF * 0.5, wn.STREET_Y + dy * (HALF + 330.0)), (HALF, 30.0), white, z=1.3)
    for dx in (-1.0, 1.0):
        wn.flat(f"StopB{x:.0f}{dx:+.0f}", (x + dx * (HALF + 330.0), wn.STREET_Y - dx * HALF * 0.5), (30.0, HALF), white, z=1.3)
    # Traffic lights on the four corners, arms over the road.
    for cx, cy, yaw in ((x - CLEAR + 60, wn.STREET_Y - CLEAR + 60, 0.0), (x + CLEAR - 60, wn.STREET_Y + CLEAR - 60, 180.0),
                        (x + CLEAR - 60, wn.STREET_Y - CLEAR + 60, 90.0), (x - CLEAR + 60, wn.STREET_Y + CLEAR - 60, -90.0)):
        traffic_light(cx, cy, yaw)


def traffic_light(x, y, yaw):
    """Mast with an arm over the road (along yaw) and a signal head facing the oncoming traffic."""
    cyl((x, y, 0.0), 12.0, 620.0, "MetalPaintedGrey")
    r = math.radians(yaw)
    ux, uy = math.cos(r), math.sin(r)
    arm_len = 520.0
    wn._spawn_mesh(wn.CYLINDER, (x + ux * arm_len * 0.5, y + uy * arm_len * 0.5, 600.0), (90.0, yaw, 0.0),
                   (0.12, 0.12, arm_len / 100.0), None, "City", None, False, True, mat("MetalPaintedGrey"))
    hx, hy = x + ux * (arm_len - 40.0), y + uy * (arm_len - 40.0)
    box(None, (hx, hy, 470.0), (34.0, 34.0, 110.0), "TrimBlack", yaw, collide=False)
    # The lenses face along -(arm normal) - towards traffic that drives into the intersection.
    nx, ny = -uy, ux
    for k, lens in enumerate(("Taillight", "Indicator", "LedGreen")):
        wn._spawn_mesh(wn.CYLINDER, (hx + nx * 18.0, hy + ny * 18.0, 560.0 - k * 34.0), (90.0, yaw + 90.0, 0.0),
                       (0.22, 0.22, 0.03), None, "City", None, False, False, mat(lens))
    box(None, (x + nx * 14.0, y + ny * 14.0, 120.0), (16.0, 16.0, 26.0), "MetalPaintedGrey", yaw, collide=False)  # push button box


def lamp(x, y, toward_yaw, lights=True):
    """US street light: mast, arm towards the road (toward_yaw), cobra head."""
    steel, head, lens = mat("SteelBrushed"), mat("MetalPaintedGrey"), mat("PlasticWhite")
    wn._spawn_mesh(wn.CYLINDER, (x, y, 412.0), (0, 0, 0), (0.16, 0.16, 8.0), None, "City", None, True, True, steel)
    r = math.radians(toward_yaw)
    ux, uy = math.cos(r), math.sin(r)
    wn._spawn_mesh(wn.CYLINDER, (x + ux * 105.0, y + uy * 105.0, 802.0), (83.0, toward_yaw, 0.0), (0.07, 0.07, 2.1), None, "City", None, False, True, steel)
    hx, hy = x + ux * 230.0, y + uy * 230.0
    box(None, (hx, hy, 806.0), (62.0, 32.0, 12.0), "MetalPaintedGrey", toward_yaw, collide=False)
    box(None, (hx, hy, 804.0), (44.0, 24.0, 2.0), "PlasticWhite", toward_yaw, collide=False, shadow=False)
    if lights:
        l = eas.spawn_actor_from_class(unreal.SpotLight, unreal.Vector(hx, hy, 798.0), unreal.Rotator(roll=0.0, pitch=-90.0, yaw=0.0))
        l.set_folder_path("City")
        lc = l.get_component_by_class(unreal.SpotLightComponent)
        lc.set_editor_property("mobility", unreal.ComponentMobility.STATIONARY)
        lc.set_editor_property("intensity_units", unreal.LightUnits.LUMENS)
        lc.set_intensity(2600.0)
        lc.set_attenuation_radius(1500.0)
        lc.set_editor_property("outer_cone_angle", 66.0)
        lc.set_light_color(unreal.LinearColor(1.0, 0.72, 0.42, 1.0))
        lc.set_editor_property("cast_shadows", False)
        l.set_editor_property("tags", ["StreetLight"])  # Phase 19: on from dusk to dawn


# --- Borders --------------------------------------------------------------------------------------------------
def closure(center, along_x, width):
    """Road construction site across a road: barriers, construction fence, signs, a dirt heap behind."""
    cx, cy = center
    n = int(width // 200.0)
    for i in range(n + 1):
        s = -width * 0.5 + i * width / n
        px, py = (cx, cy + s) if along_x else (cx + s, cy)
        wn.place("ConcreteBarrier", (px, py, 0.0), 90.0 if along_x else 0.0, folder="City")
    # Mobile construction fence right behind the barriers (closes the sidewalks too).
    off = 120.0
    if along_x:
        wn.fence_line((cx + math.copysign(off, cx), cy - width * 0.5 - WALK), (cx + math.copysign(off, cx), cy + width * 0.5 + WALK), folder="City")
    else:
        wn.fence_line((cx - width * 0.5 - WALK, cy + math.copysign(off, cy)), (cx + width * 0.5 + WALK, cy + math.copysign(off, cy)), folder="City")
    # Sign "Straße gesperrt" on a stand, cones, warning lights, a dirt heap and a site container behind.
    sx, sy = (cx - math.copysign(140.0, cx), cy - 150.0) if along_x else (cx - 150.0, cy - math.copysign(140.0, cy))
    yaw = (90.0 if cx < 0 else -90.0) if along_x else (180.0 if cy < 0 else 0.0)
    box(None, (sx, sy, 0.0), (8.0, 8.0, 160.0), "SteelBrushed", yaw, collide=True)
    box(None, (sx, sy, 140.0), (150.0, 6.0, 90.0) if not along_x else (6.0, 150.0, 90.0), "PlasticWhite", 0.0, collide=False)
    face = -1.0 if (cx > 0 if along_x else cy > 0) else 1.0
    tx, ty = (sx + face * 4.0, sy) if along_x else (sx, sy + face * 4.0)
    # Text faces into the playable area.
    t_yaw = (180.0 if cx > 0 else 0.0) if along_x else (-90.0 if cy > 0 else 90.0)
    text(tx, ty, 200.0, t_yaw, "STRASSE GESPERRT", 15.0, (170, 20, 16))
    text(tx, ty, 172.0, t_yaw, "Baustelle - bitte wenden", 10.0, (30, 30, 34))
    for k in range(5):
        s = -width * 0.4 + k * width * 0.2
        px, py = (cx - math.copysign(70.0, cx), cy + s) if along_x else (cx + s, cy - math.copysign(70.0, cy))
        wn.place("TrafficCone", (px, py, 0.8), rng.uniform(0, 90), folder="City")
    hx, hy = (cx + math.copysign(700.0, cx), cy) if along_x else (cx, cy + math.copysign(700.0, cy))
    wn.block("DirtHeap", (hx, hy, 0.0), (500.0, 400.0, 120.0), "Gravel", yaw=rng.uniform(0, 40))
    wn.place("ContainerBlue", (hx + (0 if along_x else 600.0), hy + (600.0 if along_x else 0.0), 0.0), 90.0 if along_x else 0.0, folder="City")


def border_wall(a, b, gaps):
    """Lot boundary between the roads: corrugated sheet wall on posts (2.6 m), with weeds at its foot."""
    horizontal = abs(b[1] - a[1]) < 1.0
    lo, hi = (min(a[0], b[0]), max(a[0], b[0])) if horizontal else (min(a[1], b[1]), max(a[1], b[1]))
    for j, (s0, s1) in enumerate(wn._split(lo, hi, gaps)):
        mid = (s0 + s1) * 0.5
        c = (mid, a[1], 0.0) if horizontal else (a[0], mid, 0.0)
        size = (s1 - s0, 10.0, 260.0) if horizontal else (10.0, s1 - s0, 260.0)
        wn.block(f"BorderWall{j}", c, size, "RustyCorrugated")
        steps = int((s1 - s0) // 300.0)
        for k in range(steps + 1):
            s = s0 + k * (s1 - s0) / max(1, steps)
            pc = (s, a[1], 0.0) if horizontal else (a[0], s, 0.0)
            cyl(pc, 5.0, 270.0, "SteelBrushed", collide=False)
            if rng.random() < 0.6:
                weeds((pc[0] + rng.uniform(-60, 60), pc[1] + rng.uniform(-60, 60)), 3)


def borders():
    x0, x1, y0, y1 = BORDER
    for x in (x0, x1):
        closure((x, wn.STREET_Y), True, HALF * 2.0)
    for cxs in CROSS_X:
        for y in (y0, y1):
            closure((cxs, y), False, HALF * 2.0)
    # The walls reach a little under the closure fences (no gap to slip through).
    road_x = [(x - CLEAR + 20.0, x + CLEAR - 20.0) for x in CROSS_X]
    road_y = [(wn.STREET_Y - CLEAR + 20.0, wn.STREET_Y + CLEAR - 20.0)]
    border_wall((x0, y0), (x1, y0), road_x)
    border_wall((x0, y1), (x1, y1), road_x)
    border_wall((x0, y0), (x0, y1), road_y)
    border_wall((x1, y0), (x1, y1), road_y)


# --- Vegetation ---------------------------------------------------------------------------------------------
VEG = "/Game/WorkshopEmpire/Art/Import/{0}/{0}/StaticMeshes/{1}"
WEEDS = [("weed_plant_02", "weed_plant_02_a_LOD0", 2.6), ("weed_plant_02", "weed_plant_02_b_LOD0", 2.6), ("nettle_plant", "nettle_plant_tall_a_LOD0", 3.2),
         ("nettle_plant", "nettle_plant_tall_b_LOD0", 3.2), ("dandelion_01", "dandelion_01_a_LOD0", 1.6), ("shrub_03", "shrub_03_a", 2.2), ("shrub_03", "shrub_03_b", 2.2)]
BUSHES = [("shrub_02", "shrub_02_a"), ("shrub_02", "shrub_02_b"), ("shrub_02", "shrub_02_d")]


def veg(pack, part, x, y, scale=1.0, yaw=None):
    mesh = unreal.EditorAssetLibrary.load_asset(VEG.format(pack, part)) if unreal.EditorAssetLibrary.does_asset_exist(VEG.format(pack, part)) else None
    if not mesh:
        return None
    b = mesh.get_bounding_box()
    cx, cy = (b.min.x + b.max.x) * 0.5 * scale, (b.min.y + b.max.y) * 0.5 * scale
    yaw = rng.uniform(0, 360) if yaw is None else yaw
    r = math.radians(yaw)
    px, py = cx * math.cos(r) - cy * math.sin(r), cx * math.sin(r) + cy * math.cos(r)
    return wn._spawn_mesh(mesh, (x - px, y - py, -b.min.z * scale), (0.0, yaw, 0.0), (scale, scale, scale), None, "Vegetation", None,
                          part.startswith("tree"), True)


def weeds(at, count, spread=50.0):
    for _ in range(count):
        pack, part, scale = rng.choice(WEEDS)
        veg(pack, part, at[0] + rng.uniform(-spread, spread), at[1] + rng.uniform(-spread, spread), scale * rng.uniform(0.8, 1.3))


def bush(at, scale=1.0):
    pack, part = rng.choice(BUSHES)
    veg(pack, part, at[0], at[1], scale * rng.uniform(0.85, 1.15))


def tree(at, scale=1.5):
    veg("tree_small_02", "tree_small_02", at[0], at[1], scale)


# --- Places -------------------------------------------------------------------------------------------------
CAR_DIR = "/Game/WorkshopEmpire/Art/Import/KenneyCars"
CAR_SCALE = 1.25  # the kit's cars are 2.6 m long; a uniform scale keeps the wheels round


def parked(name, x, y, yaw):
    """A parked car (Kenney Car Kit, CC0, Tools/Assets/import_cars.py). The models are long along Y: turned to X."""
    path = f"{CAR_DIR}/SM_Car_{name}"
    mesh = unreal.EditorAssetLibrary.load_asset(path) if unreal.EditorAssetLibrary.does_asset_exist(path) else None
    if not mesh:
        unreal.log_warning(f"WE_CITY car model missing: {path} (run Tools/Assets/import_cars.py)")
        return None
    return wn._spawn_mesh(mesh, (x, y, 0.0), (0.0, yaw + 90.0, 0.0), (CAR_SCALE, CAR_SCALE, CAR_SCALE), f"N_Car_{name}", "City")


CARS = ["sedan", "sedan_sports", "hatchback_sports", "suv", "suv_luxury", "van", "delivery", "truck"]


def gas_station():
    """North-east corner of the east intersection: canopy over two pump islands, shop, price sign."""
    x0, x1, y0, y1 = 6600.0, 9100.0, wn.STREET_Y + CLEAR + 10.0, 800.0
    wn.block("GasPad", ((x0 + x1) * 0.5, (y0 + y1) * 0.5, 0.0), (x1 - x0, y1 - y0, 2.0), "PaintedConcrete", collide=False)
    cx, cy = 7500.0, -350.0
    for dx in (-550.0, 550.0):
        for dy in (-330.0, 330.0):
            cyl((cx + dx, cy + dy, 0.0), 18.0, 470.0, "PlasticWhite")
    box("GasCanopy", (cx, cy, 470.0), (1400.0, 900.0, 50.0), "PlasticWhite")
    for dy in (-452.0, 452.0):
        box(None, (cx, cy + dy, 478.0), (1404.0, 6.0, 36.0), "PlasticRed", collide=False)
    for dx in (-702.0, 702.0):
        box(None, (cx + dx, cy, 478.0), (6.0, 904.0, 36.0), "PlasticRed", collide=False)
    text(cx, cy - 456.0, 497.0, -90.0, "TANKSTELLE", 30.0, (250, 250, 250))
    for dx in (-300.0, 300.0):
        box(None, (cx + dx, cy, 0.0), (120.0, 520.0, 16.0), "Concrete")
        for dy in (-150.0, 150.0):
            box(None, (cx + dx, cy + dy, 16.0), (55.0, 45.0, 165.0), "PlasticWhite")
            box(None, (cx + dx + 28.0, cy + dy, 120.0), (2.0, 32.0, 24.0), "ScreenDark", collide=False)
            box(None, (cx + dx - 28.0, cy + dy, 120.0), (2.0, 32.0, 24.0), "ScreenDark", collide=False)
            box(None, (cx + dx, cy + dy, 181.0), (57.0, 47.0, 22.0), "PlasticRed", collide=False)
            for side in (-1.0, 1.0):
                cyl((cx + dx + side * 32.0, cy + dy + 15.0, 40.0), 1.6, 110.0, "TrimBlack", collide=False)
    for dx in (-400.0, 400.0):
        light = eas.spawn_actor_from_class(unreal.PointLight, unreal.Vector(cx + dx, cy, 440.0), unreal.Rotator(0, 0, 0))
        light.set_folder_path("City")
        lc = light.get_component_by_class(unreal.PointLightComponent)
        lc.set_editor_property("mobility", unreal.ComponentMobility.STATIONARY)
        lc.set_editor_property("intensity_units", unreal.LightUnits.LUMENS)
        lc.set_intensity(4000.0)
        lc.set_attenuation_radius(900.0)
        lc.set_editor_property("cast_shadows", False)
        light.set_editor_property("tags", ["StreetLight"])  # Phase 19: on from dusk to dawn
    # Shop with a glass front, an ice box and an air pump; the price sign at the corner.
    wn.facade_building("GasShop", 8100.0, 9050.0, 500.0, 700.0, 380.0, "PaintedBrick", -1.0, shutters=0)
    box(None, (8575.0, 498.0, 0.0), (700.0, 6.0, 240.0), "Glass", collide=False)
    box(None, (8300.0, 470.0, 0.0), (90.0, 60.0, 110.0), "PlasticWhite")
    text(8300.0, 438.0, 80.0, -90.0, "EIS", 14.0, (30, 80, 160))
    box(None, (8850.0, 460.0, 0.0), (50.0, 40.0, 130.0), "PlasticRed")
    wn.place("metal_trash_can", (7950.0, 420.0, 0.0), 0.0, folder="City")
    px, py = 6700.0, wn.STREET_Y + CLEAR + 150.0
    cyl((px, py, 0.0), 14.0, 650.0, "MetalPaintedGrey")
    box("PriceSign", (px, py, 520.0), (40.0, 230.0, 200.0), "PlasticWhite")
    box(None, (px - 21.0, py, 690.0), (2.0, 230.0, 40.0), "PlasticRed", collide=False)
    for k, line in enumerate(("SUPER     3.49", "DIESEL    3.89", "AUTOGAS  2.19")):
        text(px - 22.0, py, 660.0 - k * 50.0, 180.0, line, 24.0, (20, 20, 24))
    text(px - 22.0, py, 692.0, 180.0, "TANKSTELLE", 22.0, (250, 250, 250))
    parked(CARS[1], cx - 300.0, cy + 260.0, 90.0)
    bush((6750.0, 650.0), 0.9)
    bush((7000.0, 680.0), 0.8)


def parking_lot():
    """North-west corner of the west intersection: striped stalls, parked cars, light poles, a tree island."""
    x0, x1, y0, y1 = -4850.0, -4080.0, -1250.0, 2000.0
    wn.flat("ParkingLot", ((x0 + x1) * 0.5, (y0 + y1) * 0.5), (x1 - x0, y1 - y0), wn.surface("Asphalt") or wn.surface("WornAsphalt"), z=0.9)
    white = mat("PlasticWhite")
    for k in range(12):
        y = y0 + 150.0 + k * 270.0
        if y > y1 - 100.0:
            break
        wn.flat(f"Stall{k}", (x0 + 260.0, y), (500.0, 10.0), white, z=1.4)
        box(None, (x0 + 60.0, y + 135.0, 0.0), (20.0, 160.0, 12.0), "Concrete", collide=False)  # wheel stop
        if k < 11 and rng.random() < 0.7:
            parked(rng.choice(CARS), x0 + 300.0, y + 135.0, 180.0)
    for y in (y0 + 500.0, y1 - 500.0):
        lamp(x1 - 60.0, y, 180.0, lights=False)
    box(None, (x1 - 100.0, (y0 + y1) * 0.5, 0.0), (120.0, 400.0, 18.0), "Concrete")
    tree((x1 - 100.0, (y0 + y1) * 0.5), 1.4)
    weeds((x1 - 100.0, (y0 + y1) * 0.5 + 150.0), 4, 40.0)


def billboard(x, y, yaw, lines):
    """Two-pole billboard with a painted ad (no brands)."""
    r = math.radians(yaw)
    ux, uy = math.cos(r), math.sin(r)
    for s in (-250.0, 250.0):
        cyl((x + ux * s, y + uy * s, 0.0), 14.0, 520.0, "MetalPaintedGrey")
    box("Billboard", (x, y, 500.0), (700.0, 20.0, 300.0), "PlasticWhite", yaw)
    box(None, (x - uy * 14.0, y + ux * 14.0, 500.0), (700.0, 6.0, 40.0), "PlasticRed", yaw, collide=False)
    box(None, (x + uy * 30.0, y - ux * 30.0, 480.0), (700.0, 60.0, 6.0), "SteelBrushed", yaw, collide=False)  # catwalk
    nx, ny = -uy, ux
    for k, (line, size, col) in enumerate(lines):
        text(x - nx * 12.0, y - ny * 12.0, 735.0 - k * 75.0, yaw - 90.0, line, size, col)


def repair_shop():
    """South-east corner: a small car repair shop with two bays and cars waiting."""
    front = wn.STREET_Y - CLEAR - 10.0
    wn.facade_building("AutoShop", 6650.0, 8200.0, front, 1000.0, 450.0, "WhitewashedBrick", 1.0, shutters=2)
    box(None, (7425.0, front + 2.0, 470.0), (1300.0, 6.0, 70.0), "MetalPaintedBlue", collide=False)
    text(7425.0, front + 6.0, 505.0, 90.0, "AUTO SERVICE · REIFEN · OEL", 34.0, (245, 245, 245))
    parked(CARS[4], 8500.0, front - 300.0, 90.0)
    parked(CARS[2], 8700.0, front - 650.0, 95.0)
    wn.facade_building("Warehouse_SE", 8250.0, 9250.0, front, 1300.0, 650.0, "BoxProfile", 1.0, shutters=1, windows=False)


def snack_bar():
    """South-west corner: a small snack bar with a terrace."""
    front = wn.STREET_Y - CLEAR - 10.0
    wn.facade_building("SnackBar", -6400.0, -4950.0, front, 800.0, 360.0, "RedBrick", 1.0, shutters=0)
    box(None, (-5675.0, front + 2.0, 260.0), (1100.0, 6.0, 60.0), "PlasticYellow", collide=False)
    text(-5675.0, front + 6.0, 290.0, 90.0, "DINER · 24 STUNDEN", 34.0, (150, 20, 16))
    box(None, (-5675.0, front + 1.0, 0.0), (900.0, 4.0, 220.0), "Glass", collide=False)


def vacant_lots():
    """Fenced vacant lot west of the north warehouse (weeds, gravel, a billboard to the street)."""
    wn.block("VacantGravel", (-3400.0, 3700.0, 0.0), (2400.0, 3000.0, 1.5), "Gravel", collide=False)
    for _ in range(40):
        weeds((rng.uniform(-4500, -2300), rng.uniform(2300, 5200)), 2, 80.0)
    for _ in range(6):
        bush((rng.uniform(-4600, -2400), rng.uniform(2400, 5200)))
    for t in ((-4300.0, 3000.0), (-2600.0, 4800.0)):
        tree(t, 1.6)
    billboard(-3300.0, 2350.0, 0.0, [("ALTMETALL ANKAUF", 60.0, (170, 20, 16)), ("Faire Preise - Bargeld sofort", 34.0, (30, 30, 34)),
                                       ("Industriestrasse 12", 26.0, (30, 30, 34))])
    wn.place("old_tyre", (-2900.0, 3300.0, 30.0), 0.0, folder="City")
    wn.place("Dumpster", (4900.0, 3500.0, 0.0), 90.0, folder="City")
    for _ in range(14):
        weeds((rng.uniform(4650, 5050), rng.uniform(2700, 5300)), 2, 60.0)


def kerbside_cars():
    """Cars parked along the kerbs (not in front of the gate, the hydrant or the intersections)."""
    for x in (-3900.0, -2700.0, -1500.0, 2300.0, 3500.0, 4600.0):
        parked(rng.choice(CARS), x, wn.STREET_Y - HALF + 120.0, 0.0)
    for x in (-3300.0, 2000.0, 3900.0):
        parked(rng.choice(CARS), x, wn.STREET_Y + HALF - 120.0, 180.0)
    for y in (300.0, 1400.0, 3600.0):
        parked(rng.choice(CARS), CROSS_X[1] - HALF + 120.0, y, 90.0)
    for y in (-3200.0, 2800.0):
        parked(rng.choice(CARS), CROSS_X[0] + HALF - 120.0, y, -90.0)


def street_furniture():
    for x in CROSS_X:
        for y in (-3800.0, -200.0, 1800.0, 3800.0):
            lamp(x + HALF + 35.0, y, 180.0, lights=False)
    for x in (-5000.0, 6300.0, 7900.0):
        lamp(x, wn.STREET_Y + HALF + 35.0, -90.0, lights=False)
    # Street trees on the far sidewalks, weeds along the kerbs.
    for x, y in ((-4300.0, wn.STREET_Y - HALF - 140.0), (2600.0, wn.STREET_Y - HALF - 140.0), (6900.0, wn.STREET_Y - HALF - 140.0)):
        tree((x, y), 1.3)
        box(None, (x, y, 0.0), (140.0, 140.0, 14.0), "Concrete", collide=False)
    for _ in range(30):
        x = rng.uniform(-6000, 9000)
        if 300 < x < 1300:
            continue
        weeds((x, wn.STREET_Y + rng.choice((-1, 1)) * (HALF + WALK - 10.0)), 2, 20.0)
    # Weeds along the lot fence (the company's own lot).
    lx0, lx1, ly0, ly1 = wn.LOT
    for _ in range(26):
        side = rng.randrange(3)
        at = (rng.uniform(lx0, lx1), ly1 + 25.0) if side == 0 else ((lx0 - 25.0, rng.uniform(ly0, ly1)) if side == 1 else (lx1 + 25.0, rng.uniform(ly0, ly1)))
        weeds(at, 2, 25.0)


# --- Outside the playable area -------------------------------------------------------------------------------
def outskirts():
    """Buildings along the roads beyond the closures, warehouses behind the border walls, a skyline far away."""
    x0, x1, y0, y1 = BORDER
    walls = ["RedBrick", "DarkBrick", "ConcreteWall", "PaintedBrick", "WhitewashedBrick", "BoxProfile"]
    north_front = wn.STREET_Y + CLEAR + 10.0
    south_front = wn.STREET_Y - CLEAR - 10.0
    for direction, start, end in ((-1.0, x0 - 900.0, -15500.0), (1.0, x1 + 900.0, 15500.0)):
        x = start
        while (x > end) if direction < 0 else (x < end):
            w = rng.uniform(1200.0, 2400.0)
            a, b = (x - w, x) if direction < 0 else (x, x + w)
            wn.facade_building("Out_N", a, b, north_front, 1200.0, rng.uniform(400.0, 1100.0), rng.choice(walls), -1.0, shutters=0, extras=False)
            wn.facade_building("Out_S", a, b, south_front, 1200.0, rng.uniform(400.0, 1100.0), rng.choice(walls), 1.0, shutters=0, extras=False)
            x += direction * (w + rng.uniform(150.0, 500.0))
    for cx in CROSS_X:
        for direction, start, end in ((-1.0, y0 - 900.0, -15500.0), (1.0, y1 + 900.0, 15500.0)):
            y = start
            while (y > end) if direction < 0 else (y < end):
                h = rng.uniform(1200.0, 2200.0)
                a, b = (y - h, y) if direction < 0 else (y, y + h)
                for side in (-1.0, 1.0):
                    bx = cx + side * (CLEAR + 10.0 + 600.0)
                    wn.block("Out_X", (bx, (a + b) * 0.5, 0.0), (1200.0, h, rng.uniform(400.0, 1000.0)), rng.choice(walls))
                y += direction * (h + rng.uniform(150.0, 500.0))
    # Big blocks behind the border walls (between the crossing streets).
    for (bx0, bx1) in ((-4800.0, 5000.0), (6600.0, 9000.0)):
        for by, depth in ((y1 + 900.0, 1400.0), (y0 - 900.0, -1400.0)):
            x = bx0
            while x < bx1:
                w = rng.uniform(1500.0, 3000.0)
                wn.block("Out_Back", (min(x + w * 0.5, bx1 - 300), by + depth * 0.5, 0.0), (min(w, bx1 - x), abs(depth), rng.uniform(600.0, 1200.0)), rng.choice(walls))
                x += w + 300.0
    skyline()


def skyline():
    """Downtown in the distance: towers with window bands (one actor per band), north-east of the town."""
    glass = mat("Glass")
    for k in range(9):
        cx = -14000.0 + k * 3600.0 + rng.uniform(-700, 700)
        cy = 42000.0 + rng.uniform(-3000, 4000)
        w, d, h = rng.uniform(2200, 3600), rng.uniform(2000, 3000), rng.uniform(4500, 10000)
        wn.block("Tower", (cx, cy, 0.0), (w, d, h), rng.choice(["ConcretePanels", "ConcreteWall", "PaintedConcrete"]), collide=False)
        for z in range(600, int(h) - 300, 420):
            wn._spawn_mesh(wn.CUBE, (cx, cy - d * 0.5 - 2.0, z), (0, 0, 0), (w * 0.94 / 100.0, 0.04, 1.9), None, "Skyline", None, False, False, glass)


def build_city(menu=False):
    for x in CROSS_X:
        cross_street(x)
    borders()
    gas_station()
    parking_lot()
    repair_shop()
    snack_bar()
    vacant_lots()
    kerbside_cars()
    street_furniture()
    outskirts()
    unreal.log(f"WE_CITY actors={wn._count['actors']}")
