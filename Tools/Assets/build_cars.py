"""
Quality update: parked cars for the town (no brands, no logos), built with Geometry Script - replaces the toy-like
Kenney Car Kit. No free, realistic car models could be downloaded without an account (Poly Haven only has the
covered car), so these are modelled here: a lower body extruded from a side profile with rounded edges and cut-out
wheel arches, a glass cabin with painted roof and pillars, tyres with rounded sidewalls and real rims, chrome or
plastic bumpers, head- and taillights (off), indicators, grille, mirrors, door handles and seams, number plates
(blank) and an underbody.

Space (cm): nose at +X, ground at z = 0 (tyres stand on it), origin at the bottom centre. The paint is the slot with
an MI_Prop_CarPaint* material (world_city.py swaps it per car for colour variety; the slot order of a generated mesh
is not the build order). Three LODs per car.

  SM_Gen_Car_SedanOld   American sedan of the 80s (chrome bumpers, quad headlights, whitewalls)
  SM_Gen_Car_Wagon      station wagon of the same family
  SM_Gen_Car_Pickup     full-size pickup with an open bed
  SM_Gen_Car_PickupOld  the old workshop pickup (faded paint, rusty bumpers, roof rack)
  SM_Gen_Car_Suv        boxy 90s SUV with the spare wheel on the tailgate
  SM_Gen_Car_Hatch      everyday compact hatchback

Run after build_prop_material.py:
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
P = bm.P
sme = bm.sme


def extrude_xz(m, mat, pts, y0, thickness, bevel=0.0, segments=1, cuts=None):
    """Polygon in the XZ plane, extruded from y0 towards -Y (rounded edges with bevel > 0)."""
    part = unreal.DynamicMesh()
    bm.CURRENT_ID[0] = m._mid(mat)
    P.append_simple_extrude_polygon(part, bm.popts(bm.CURRENT_ID[0]), bm.xf((0, y0, 0), (-90, 0, 0)),
                                    [unreal.Vector2D(x, z) for x, z in pts], thickness, 0, True, bm.BASE)
    bm.fix_orientation(part)
    return m._finish(part, mat, bevel, cuts, segments)


def both_sides(fn, half):
    for s in (-1.0, 1.0):
        fn(s, half)


def wheel(m, x, y_out, r, width, side, rim="Aluminium", whitewall=False, style="disc"):
    """Wheel at axle x, outer tyre face at y_out (side -1 or 1)."""
    yc = y_out - side * width * 0.5
    side_r = min(9.0, width * 0.42)
    # Tyre: tread core plus two rounded sidewalls.
    m.cyl("TireRubber", (x, yc, r), r - 1.5, width - side_r * 1.2, rot=(90, 0, 0), segs=40)
    for k in (-1.0, 1.0):
        m.torus("TireRubber", (x, yc + k * (width * 0.5 - side_r * 0.85), r), r - side_r, side_r, rot=(90, 0, 0), segs=40, minor=10)
    face = y_out + side * 0.2
    rim_r = r * 0.6
    if whitewall:
        m.torus("PlasticWhite", (x, face - side * 0.6, r), rim_r + 4.5, 2.2, rot=(90, 0, 0), segs=40, minor=6)
    # Rim: dish, lip, hub and spokes or a hubcap.
    m.cyl("TrimBlack", (x, face - side * 2.5, r), rim_r - 1.0, 3.0, rot=(90, 0, 0), segs=32)
    m.torus(rim, (x, face - side * 0.8, r), rim_r, 1.1, rot=(90, 0, 0), segs=36, minor=6)
    if style == "disc":
        # (brushed, not a mirror: a chrome disc showed the red paint and looked like a red rim)
        m.cyl("Aluminium" if rim == "Chrome" else rim, (x, face - side * 1.2, r), rim_r - 0.8, 1.2, rot=(90, 0, 0), segs=36)
        for k in range(10):  # ribs of an 80s hubcap
            a = math.radians(k * 36.0)
            m.box("TrimBlack", (x + math.cos(a) * rim_r * 0.62, face - side * 0.3, r + math.sin(a) * rim_r * 0.62), (rim_r * 0.42, 0.4, 1.0), rot=(0, -k * 36.0, 0))
    else:
        for k in range(style if isinstance(style, int) else 5):
            n = style if isinstance(style, int) else 5
            a = k * 360.0 / n
            ra = math.radians(a)
            m.box(rim, (x + math.cos(ra) * rim_r * 0.5, face - side * 1.0, r + math.sin(ra) * rim_r * 0.5), (rim_r * 0.9, 1.6, 3.4), rot=(0, -a, 0), bevel=0.4)
    m.cyl("Chrome", (x, face - side * 0.6, r), rim_r * 0.28, 1.6, rot=(90, 0, 0), segs=20)
    for k in range(5):  # wheel nuts
        a = math.radians(k * 72.0 + 18.0)
        m.cyl("SteelBrushed", (x + math.cos(a) * rim_r * 0.4, face - side * 0.2, r + math.sin(a) * rim_r * 0.4), 0.8, 1.0, rot=(90, 0, 0), segs=8)


def car(name, paint, L, W, r, tyre_w, axles, body, cabin, roof, pillars, opts):
    """
    body:    lower body profile (x, z) at full width
    cabin:   glass greenhouse profile (inset by opts['tumble'] on each side)
    roof:    roof slab profile (painted)
    pillars: list of pillar strips (x, z), painted, on both sides of the cabin
    """
    m = Model(f"Car_{name}")
    half = W * 0.5
    tumble = opts.get("tumble", 12.0)
    old = opts.get("old", False)
    trim = "Chrome" if old else "TrimBlack"
    bumper = opts.get("bumper", trim)
    arch_r = r + 5.0
    # Lower body (slot 0 = paint), wheel arches cut out, bed or load space cut from the top.
    cuts = [Model.cut_cyl((ax, 0, r), arch_r, W + 20.0, rot=(90, 0, 0), segs=36) for ax in axles]
    cuts += [Model.cut_box(c, s) for c, s in opts.get("body_cuts", [])]
    extrude_xz(m, paint, body, half, W, bevel=opts.get("bevel", 5.0), segments=2, cuts=cuts)
    # Wheel wells (dark, so one cannot look through the arch) and the underbody.
    for ax in axles:
        # (inboard of the tyres: its end caps must not lie on the wheel faces)
        m.cyl("TrimBlack", (ax, 0, r), arch_r - 0.8, W - 2.0 * tyre_w - 14.0, rot=(90, 0, 0), segs=32)
    m.box("TrimBlack", ((axles[0] + axles[1]) * 0.5, 0, opts["floor"] - 3.0), (abs(axles[0] - axles[1]) + 2 * r, W - 26.0, 6.0))
    # Cabin: glass all round, the roof and the pillars painted over it.
    extrude_xz(m, "CarGlass", cabin, half - tumble, W - 2 * tumble, bevel=1.5)
    # Interior behind the tinted glass: dashboard, steering wheel, seats.
    inner = W - 2 * tumble - 6.0
    front_x, rear_x = cabin[1][0], cabin[0][0]
    belt0 = opts["belt"]
    seat_z = opts["floor"] + 26.0
    m.box("CarInterior", ((front_x + rear_x) * 0.5, 0, belt0 - 22.0), (abs(front_x - rear_x) - 6.0, inner, 4.0))  # floor
    m.box("CarInterior", (front_x - 14.0, 0, belt0 - 4.0), (26.0, inner, 14.0), bevel=2.0)  # dashboard
    m.torus("TrimBlack", (front_x - 34.0, -W * 0.2, belt0 + 4.0), 17.0, 1.6, rot=(0, 60, 0), segs=24, minor=6)
    seat_x = front_x - opts.get("seat_back", 80.0)
    for s in (-1.0, 1.0):
        m.box("CarInterior", (seat_x, s * W * 0.2, seat_z), (48.0, 48.0, 12.0), bevel=3.0)
        m.box("CarInterior", (seat_x - 26.0, s * W * 0.2, seat_z + 30.0), (10.0, 46.0, 58.0), rot=(0, -12, 0), bevel=3.0)
        m.box("CarInterior", (seat_x - 28.0, s * W * 0.2, seat_z + 66.0), (8.0, 24.0, 16.0), bevel=2.5)
    if opts.get("rear_seat", True):
        m.box("CarInterior", (seat_x - 95.0, 0, seat_z), (46.0, inner - 6.0, 12.0), bevel=3.0)
        m.box("CarInterior", (seat_x - 120.0, 0, seat_z + 28.0), (10.0, inner - 6.0, 54.0), rot=(0, -14, 0), bevel=3.0)
    extrude_xz(m, paint, roof, half - tumble + 1.0, W - 2 * tumble + 2.0, bevel=2.5, segments=2)
    for pts in pillars:
        for s in (-1.0, 1.0):
            y0 = s * (half - tumble + 1.4) + (0.0 if s > 0 else 1.4)
            extrude_xz(m, paint, pts, y0, 1.4)
    # Windscreen and rear window frames: a dark rubber strip along the beltline of the cabin.
    belt = opts["belt"]
    x_front = max(p[0] for p in body)
    x_rear = min(p[0] for p in body)
    for s in (-1.0, 1.0):
        y = s * (half + 0.15)
        m.box(trim, ((cabin[0][0] + cabin[1][0]) * 0.5, y, belt - 0.8), (abs(cabin[1][0] - cabin[0][0]), 0.8, 1.4))
        # Door seams, handles and a mirror.
        for dx in opts["doors"]:
            m.box("TrimBlack", (dx, y, (opts["floor"] + belt) * 0.5 + 2.0), (0.5, 0.5, belt - opts["floor"] - 6.0))
        for hx in opts["handles"]:
            m.box(trim if old else paint, (hx, y + s * 0.4, belt - 9.0), (9.0, 1.2, 2.2), bevel=0.4)
        mx = opts["mirror_x"]
        m.box("TrimBlack", (mx, s * (half + 2.5), belt + 4.0), (3.0, 5.0, 2.0))
        m.box(trim if old else paint, (mx - 2.0, s * (half + 6.5), belt + 7.0), (6.0, 6.0, 9.0), bevel=1.2)
        m.box("ScreenDark", (mx - 5.1, s * (half + 6.5), belt + 7.0), (0.3, 5.0, 7.5))
        if opts.get("side_strip"):
            m.box(trim, ((x_front + x_rear) * 0.5, y + s * 0.3, opts["side_strip"]), (abs(x_front - x_rear) - 40.0, 0.8, 2.0))
    # Front: bumper, grille, headlights, indicators, plate.
    fz = opts["front_z"]  # (bumper z, light z)
    m.box(bumper, (x_front + 3.0, 0, fz[0]), (8.0, W - 4.0, 13.0), bevel=2.5, segments=2)
    m.box("TrimBlack", (x_front + 0.6, 0, fz[1]), (1.5, opts.get("grille_w", W * 0.42), opts.get("grille_h", 16.0)))
    for k in range(5):
        m.box(trim, (x_front + 1.2, 0, fz[1] - 6.0 + k * 3.0), (0.6, opts.get("grille_w", W * 0.42) - 2.0, 0.6))
    for s in (-1.0, 1.0):
        if opts.get("quad"):
            for j, yy in enumerate((0.27, 0.38)):
                m.box("HeadlightOff", (x_front + 0.6, s * W * yy, fz[1]), (1.6, 14.0, 10.0), bevel=0.5)
                m.box(trim, (x_front + 0.3, s * W * yy, fz[1]), (1.2, 15.6, 11.6))
        elif opts.get("round"):
            m.cyl("HeadlightOff", (x_front + 1.0, s * W * 0.33, fz[1]), 9.0, 2.0, rot=(0, 90, 0), segs=24)
            m.torus(trim, (x_front + 1.6, s * W * 0.33, fz[1]), 9.2, 1.0, rot=(0, 90, 0), segs=24, minor=6)
        else:
            m.box("HeadlightOff", (x_front + 0.4, s * W * 0.33, fz[1] + 1.0), (2.0, W * 0.2, 12.0), bevel=1.0)
        m.box("IndicatorOff", (x_front + 0.3, s * (half - 8.0), fz[1] - 10.0), (1.5, 10.0, 4.0), bevel=0.5)
    m.box("TrimBlack", (x_front + 7.6, 0, fz[0] + 1.0), (0.6, 54.0, 13.0))
    m.box("PlasticWhite", (x_front + 8.0, 0, fz[0] + 1.0), (0.6, 52.0, 11.0))
    # Rear: bumper, taillights, plate, exhaust.
    rz = opts["rear_z"]
    m.box(bumper, (x_rear - 3.0, 0, rz[0]), (8.0, W - 4.0, 13.0), bevel=2.5, segments=2)
    for s in (-1.0, 1.0):
        m.box("TaillightOff", (x_rear - 0.5, s * (half - opts.get("tail_w", 30.0) * 0.5 - 6.0), rz[1]), (2.0, opts.get("tail_w", 30.0), opts.get("tail_h", 14.0)), bevel=0.8)
        m.box("IndicatorOff", (x_rear - 0.6, s * (half - 6.0 - opts.get("tail_w", 30.0) - 4.0), rz[1]), (1.6, 7.0, opts.get("tail_h", 14.0) - 2.0))
    plate_z = rz[2] if len(rz) > 2 else rz[1]
    m.box("TrimBlack", (x_rear - 0.9, 0, plate_z), (0.6, 54.0, 13.0))
    m.box("PlasticWhite", (x_rear - 1.3, 0, plate_z), (0.6, 52.0, 11.0))
    m.cyl("SteelBrushed", (x_rear - 2.0, -half * 0.55, opts["floor"] - 2.0), 2.6, 18.0, rot=(0, 90, 0), segs=14)
    # Extras.
    for extra in opts.get("extras", []):
        extra(m, half)
    # Wheels.
    for ax in axles:
        for s in (-1.0, 1.0):
            wheel(m, ax, s * (half - 3.0), r, tyre_w, s, rim=opts.get("rim", "Aluminium"), whitewall=opts.get("whitewall", False), style=opts.get("rim_style", "disc"))
    sm = m.save(normalize=True)
    if sm:
        unreal.log(f"WE_CARS {name}: {len(m.mats)} materials {m.mats}, {sm.get_num_sections(0)} sections, slots "
                   f"{[str(sm.get_material(i).get_name()) if sm.get_material(i) else '-' for i in range(len(sm.static_materials))]}")
        set_lods(sm)
    return sm


def set_lods(sm):
    """Three LODs (full, half, quarter of the triangles): cars along the street are often far away."""
    try:
        options = unreal.EditorScriptingMeshReductionOptions()
        settings = []
        for percent, screen in ((1.0, 1.0), (0.45, 0.3), (0.18, 0.12)):
            s = unreal.EditorScriptingMeshReductionSettings()
            s.set_editor_property("percent_triangles", percent)
            s.set_editor_property("screen_size", screen)
            settings.append(s)
        options.set_editor_property("reduction_settings", settings)
        options.set_editor_property("auto_compute_lod_screen_size", False)
        n = sme.set_lods(sm, options)
        unreal.EditorAssetLibrary.save_loaded_asset(sm)
        unreal.log(f"WE_CARS {sm.get_name()}: {n} LODs, {sm.get_num_triangles(0)} triangles in LOD 0")
    except Exception as e:
        unreal.log_warning(f"WE_CARS LODs failed on {sm.get_name()}: {e}")


# --- The cars -------------------------------------------------------------------------------------------------
def sedan_old():
    body = [(-262, 20), (264, 20), (266, 56), (262, 80), (150, 88), (110, 92), (-120, 92), (-152, 90), (-258, 87), (-264, 60)]
    cabin = [(-120, 92), (110, 92), (50, 137), (-80, 139)]
    roof = [(-83, 136.5), (53, 134.5), (50, 139.5), (-80, 141)]
    pillars = [[(110, 92), (101, 92), (46, 137), (53, 136)], [(-12, 92), (-4, 92), (-4, 139), (-12, 139)],
               [(-120, 92), (-98, 92), (-66, 139), (-81, 140)]]
    return car("SedanOld", "CarPaintMaroon", 528, 190, 32, 21, (150, -142), body, cabin, roof, pillars, dict(
        old=True, quad=True, whitewall=True, rim="Chrome", rim_style="disc", floor=22, belt=92, doors=(108, -2, -112),
        handles=(30, -82), mirror_x=96, side_strip=66, front_z=(36, 68), rear_z=(38, 76, 58), tail_w=40, tail_h=12, grille_w=96, grille_h=18))


def wagon():
    body = [(-262, 20), (264, 20), (266, 56), (262, 80), (150, 88), (110, 92), (-258, 92), (-264, 60)]
    cabin = [(-258, 92), (110, 92), (50, 137), (-240, 139), (-260, 118)]
    roof = [(-243, 136.5), (53, 134.5), (50, 139.5), (-240, 141)]
    pillars = [[(110, 92), (101, 92), (46, 137), (53, 136)], [(-12, 92), (-4, 92), (-4, 139), (-12, 139)],
               [(-140, 92), (-130, 92), (-130, 139), (-140, 139)], [(-262, 92), (-246, 92), (-232, 139), (-244, 140)]]

    def rack(m, half):
        for s in (-1.0, 1.0):
            m.box("Chrome", (-90, s * (half - 22.0), 144.0), (300.0, 2.0, 2.0))
            for x in (-220.0, 40.0):
                m.box("TrimBlack", (x, s * (half - 22.0), 142.0), (4.0, 3.0, 4.0))
    return car("Wagon", "CarPaintBeige", 528, 190, 32, 21, (150, -142), body, cabin, roof, pillars, dict(
        old=True, round=False, whitewall=False, rim="SteelBrushed", rim_style="disc", floor=22, belt=92, doors=(108, -2, -112),
        handles=(30, -82), mirror_x=96, side_strip=66, front_z=(36, 68), rear_z=(38, 74, 58), tail_w=26, tail_h=24, extras=[rack]))


def pickup(name, paint, old):
    body = [(-266, 34), (268, 34), (270, 66), (266, 98), (160, 102), (118, 104), (-264, 104), (-268, 98)]
    cabin = [(-6, 104), (118, 104), (72, 160), (-2, 162)]
    roof = [(-5, 159.5), (75, 158), (72, 162.5), (-2, 164)]
    pillars = [[(118, 104), (108, 104), (68, 160), (76, 159)], [(-6, 104), (8, 104), (10, 162), (-2, 162)]]
    bed = [((-140.0, 0.0, 112.0), (244.0, 182.0, 76.0))]  # open bed, floor at z = 74

    def bed_details(m, half):
        m.box("TrimBlack", (-140.0, 0.0, 74.5), (240.0, 176.0, 1.0))  # bed liner
        for k in range(6):
            m.box("TrimBlack", (-140.0, -75.0 + k * 30.0, 75.5), (236.0, 3.0, 1.2))
        for s in (-1.0, 1.0):
            m.box("Chrome" if old else "TrimBlack", (-140.0, s * (half - 4.0), 105.0), (244.0, 8.0, 2.0))  # bed rails
        if old:  # roof rack / ladder rack of a workshop truck
            for x in (-250.0, -40.0):
                for s in (-1.0, 1.0):
                    m.box("MetalPaintedGrey", (x, s * (half - 6.0), 130.0), (4.0, 4.0, 52.0))
                m.box("MetalPaintedGrey", (x, 0.0, 156.0), (4.0, 2 * half - 8.0, 4.0))
            for s in (-1.0, 1.0):
                m.box("MetalPaintedGrey", (-145.0, s * (half - 6.0), 156.0), (214.0, 4.0, 4.0))
    return car(name, paint, 534, 200, 36, 25, (168, -152), body, cabin, roof, pillars, dict(
        old=old, bumper="Rust" if old else "Chrome", rim="SteelBrushed" if old else "Aluminium", rim_style=5 if not old else "disc",
        floor=36, belt=104, doors=(116, 2), handles=(52,), mirror_x=104, side_strip=None, body_cuts=bed, bevel=4.0, rear_seat=False, seat_back=70.0,
        front_z=(46, 80), rear_z=(46, 88, 62), tail_w=14, tail_h=30, grille_w=110, grille_h=24, quad=old, extras=[bed_details]))


def suv():
    body = [(-236, 32), (238, 32), (241, 60), (237, 96), (130, 100), (98, 102), (-233, 102), (-236, 98)]
    cabin = [(-233, 102), (98, 102), (58, 168), (-229, 170)]
    roof = [(-232, 167.5), (61, 166), (58, 170.5), (-229, 172)]
    pillars = [[(98, 102), (88, 102), (54, 168), (62, 167)], [(-12, 102), (-3, 102), (-3, 170), (-12, 170)],
               [(-120, 102), (-110, 102), (-110, 170), (-120, 170)], [(-235, 102), (-218, 102), (-216, 170), (-230, 171)]]

    def spare(m, half):
        m.cyl("TireRubber", (-246.0, 0.0, 115.0), 34.0, 22.0, rot=(0, 90, 0), segs=36)
        m.cyl("TrimBlack", (-257.5, 0.0, 115.0), 24.0, 1.5, rot=(0, 90, 0), segs=28)
        for s in (-1.0, 1.0):
            m.box("Chrome", (-130.0, s * (half - 16.0), 175.0), (200.0, 2.5, 2.5))
    return car("Suv", "CarPaintGreen", 476, 190, 36, 24, (140, -138), body, cabin, roof, pillars, dict(
        old=False, bumper="PlasticGrey", rim="Aluminium", rim_style=6, floor=34, belt=102, doors=(96, -5, -116),
        handles=(30, -70), mirror_x=86, side_strip=None, front_z=(44, 78), rear_z=(44, 92, 66), tail_w=12, tail_h=36, grille_w=88, grille_h=20,
        bevel=6.0, extras=[spare]))


def hatch():
    body = [(-200, 22), (203, 22), (206, 48), (199, 76), (112, 84), (80, 86), (-196, 86), (-201, 76)]
    cabin = [(-196, 86), (80, 86), (24, 141), (-165, 142), (-198, 110)]
    roof = [(-168, 139.5), (27, 138.5), (24, 143), (-165, 144)]
    pillars = [[(80, 86), (71, 86), (20, 141), (27, 140)], [(-30, 86), (-22, 86), (-22, 142), (-30, 142)],
               [(-198, 86), (-170, 86), (-150, 142), (-166, 143), (-198, 112)]]
    return car("Hatch", "CarPaintBlue", 406, 170, 29, 18, (125, -122), body, cabin, roof, pillars, dict(
        old=False, bumper="PlasticGrey", rim="SteelBrushed", rim_style="disc", floor=24, belt=86, doors=(78, -30),
        handles=(-14,), mirror_x=66, side_strip=None, front_z=(34, 62), rear_z=(36, 78, 58), tail_w=18, tail_h=20, grille_w=70, grille_h=10,
        bevel=7.0))


BUILDERS = {"SedanOld": sedan_old, "Wagon": wagon, "Pickup": lambda: pickup("Pickup", "CarPaintRed", False),
            "PickupOld": lambda: pickup("PickupOld", "CarPaintFaded", True), "Suv": suv, "Hatch": hatch}


def main():
    built, failed = 0, []
    for name, make in BUILDERS.items():
        try:
            if make():
                built += 1
            else:
                failed.append(name)
        except Exception as e:
            unreal.log_error(f"WE_CARS {name} failed: {e}")
            failed.append(name)
    unreal.log(f"WE_CARS_BUILT {built} failed={failed}")


main()
unreal.SystemLibrary.quit_editor()
