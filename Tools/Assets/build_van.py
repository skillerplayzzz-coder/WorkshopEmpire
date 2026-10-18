"""
Phase 18b: the company van as an American full-size cargo van (long hood, high box body, no logos).
Built in van space (cm): nose at +X, ground at z = 0, centre of the wheelbase near x = 0. Not normalised,
so every part lines up with the others and with the constants in AWEVehicle (WEVehicle.cpp, "Van layout"):
  SM_Gen_<Variant>_Body     body, cab interior (seats, dash, steering wheel), cargo bay (floor, liners,
                            shelf, wheel wells), lights, bumpers, underbody
  SM_Gen_<Variant>_DoorFL   driver door      (hinge x = 146, y = -100), opens with yaw +
  SM_Gen_<Variant>_DoorFR   passenger door   (hinge x = 146, y = +100), opens with yaw -
  SM_Gen_<Variant>_DoorRL   rear door left   (hinge x = -281, y = -98), opens with yaw +
  SM_Gen_<Variant>_DoorRR   rear door right  (hinge x = -281, y = +98), opens with yaw -
  SM_Gen_VanWheel           wheel, origin at its centre, axis Y, hub cap facing -Y (radius 38)
Variants: VanOld (worn white, chrome bumpers, roof rack), VanModern (blue, black bumpers, stripe).

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

HW = 100.0           # half width of the body
FRONT_AXLE, REAR_AXLE, WHEEL_R, TRACK = 175.0, -170.0, 38.0, 86.0
ROOF = 212.0
SILL = 38.0
FLOOR = 62.0         # cargo floor (top)
# Front door opening in the side wall (x, z); the front edge follows the windscreen pillar.
DOOR_OPENING = [(42, 42), (146, 42), (146, 120), (97, 198), (42, 198)]
DOOR_PANEL = [(43, 43), (145, 43), (145, 119.5), (97.5, 196.5), (43, 196.5)]
DOOR_WINDOW = [(52, 128), (141, 128), (141, 121.5), (100, 189), (52, 189)]


def poly_cut(pts, y0, thickness):
    """Cutter: polygon in the XZ plane, from y0 towards -Y."""
    m = unreal.DynamicMesh()
    P.append_simple_extrude_polygon(m, bm.popts(0), bm.xf((0, y0, 0), (-90, 0, 0)), [unreal.Vector2D(x, z) for x, z in pts], thickness, 0, True, bm.BASE)
    bm.fix_orientation(m)
    return m


def panel(m, mat, pts, y0, thickness, cuts=None, bevel=0.0):
    """Plate with the outline pts (XZ plane), from y0 towards -Y."""
    return m.extrude(mat, pts, thickness, c=(0, y0, 0), rot=(-90, 0, 0), bevel=bevel, cuts=cuts)


def beam(m, mat, a, b, w, h):
    """Box from point a to point b (in the XZ plane at y = a[1]), w across (Y), h thick."""
    dx, dz = b[0] - a[0], b[2] - a[2]
    length = math.hypot(dx, dz)
    pitch = math.degrees(math.atan2(dz, dx))
    m.box(mat, ((a[0] + b[0]) / 2, a[1], (a[2] + b[2]) / 2), (length, w, h), rot=(0, pitch, 0))


# =============================================================================================================
def body(name, paint, old):
    m = Model(name)
    chrome = "Chrome" if old else "TrimBlack"
    # --- Side walls with the door openings and the rear wheel arches ---------------------------------------
    side = [(-280, SILL), (150, SILL), (150, 120), (97, 206), (88, ROOF), (-280, ROOF)]
    for y0 in (-HW + 2.0, HW):
        cuts = [poly_cut(DOOR_OPENING, y0 + 5, 12),
                Model.cut_cyl((REAR_AXLE, y0, SILL), WHEEL_R + 8, 20, rot=(90, 0, 0), segs=32)]
        panel(m, paint, side, y0, 2.0, cuts=cuts)
    # Roof with stiffening ribs, rear header and sill; floor; firewall.
    m.box(paint, (-96, 0, ROOF + 1.5), (372, 2 * HW, 3), bevel=1.0)
    beam(m, paint, (97, 0, 206), (88, 0, ROOF + 1.5), 2 * HW, 3)
    for x in range(-250, 80, 40):
        m.box(paint, (x, 0, ROOF + 3.4), (4, 2 * HW - 12, 1.2), bevel=0.4)
    m.box(paint, (-279, 0, 204), (4, 2 * HW, 16))
    m.box(paint, (-279, 0, 50), (4, 2 * HW, 24))
    m.box("Wood", (-120, 0, FLOOR - 2), (318, 2 * HW - 4, 4))
    m.box("TrimBlack", (95, 0, 46), (108, 2 * HW - 4, 4))
    m.box("TrimBlack", (150, 0, 80), (4, 2 * HW - 4, 84))
    # --- Front clip: fenders, hood, grille, lights, bumper ----------------------------------------------------
    clip = [(148, SILL), (250, SILL), (262, 48), (265, 100), (258, 110), (150, 121)]
    panel(m, paint, clip, HW, 2 * HW, bevel=3.0,
          cuts=[Model.cut_cyl((FRONT_AXLE, 0, SILL), WHEEL_R + 8, 2 * HW + 20, rot=(90, 0, 0), segs=32)])
    m.box("TrimBlack", (205, 0, 70), (80, 110, 56), bevel=2.0)   # engine block, seen through the arches
    for y in (-HW + 18, HW - 18):                                  # hood seams
        beam(m, "TrimBlack", (151, y, 121.2), (257, y, 110.6), 0.8, 0.5)
    beam(m, "TrimBlack", (151, 0, 121.6), (151.5, 0, 121.6), 2 * HW - 30, 0.6)
    m.box("TrimBlack", (265.4, 0, 80), (2, 112, 34), bevel=0.5)    # grille
    for i in range(5):
        m.box(chrome, (266.6, 0, 66 + i * 7), (1.0, 110, 1.6))
    m.box(chrome, (266.4, 0, 80), (1.2, 116, 2.0) if old else (0.8, 116, 1.0))
    for y in (-76.0, 76.0):
        m.box("Chrome", (264.3, y, 90), (2.4, 36, 24), bevel=0.8)  # headlight bezel
        m.box("Headlight", (265.3, y, 90), (1.6, 31, 19), bevel=0.6)
        m.box("Indicator", (265.0, y, 72.5), (1.8, 31, 7), bevel=0.5)
        m.box("Indicator", (238, y / abs(y) * (HW + 0.4), 95), (10, 1.0, 4), bevel=0.3)   # side marker
        m.box("Taillight", (-262, y / abs(y) * (HW + 0.4), 95), (8, 1.0, 4), bevel=0.3)
    m.box(chrome, (271, 0, 52), (14, 2 * HW + 6, 20), bevel=3.0)    # bumper
    m.box("PlasticWhite", (278.3, 0, 52), (0.8, 30, 15))            # plate (blank)
    beam(m, "TrimBlack", (150, 0, 121.4), (97, 0, 206), 2 * HW - 4, 1.2)  # windscreen frame
    beam(m, "Glass", (149.6, 0, 121.6), (97.4, 0, 205.4), 2 * HW - 10, 0.8)
    # --- Rear: corner lights, bumper with step, plate --------------------------------------------------------
    for y in (-94.0, 94.0):
        m.box("Taillight", (-280.6, y, 135), (2.0, 10, 56), bevel=0.4)
        m.box("Headlight", (-280.6, y, 99), (2.0, 10, 12), bevel=0.4)
    m.box("TrimBlack", (-287, 0, 50), (14, 2 * HW + 4, 18), bevel=2.0)
    m.box("SteelBrushed", (-291, 0, 59.4), (6, 60, 1.2))
    m.box("PlasticWhite", (-294.2, 30, 50), (0.8, 30, 15))
    # --- Underbody: frame, axles, differential, tank, exhaust, wheel houses ---------------------------------
    for y in (-48.0, 48.0):
        m.box("TrimBlack", (0, y, 40), (500, 8, 12))
    for x in (FRONT_AXLE, REAR_AXLE):
        m.cyl("TrimBlack", (x, 0, WHEEL_R), 5.5, 2 * TRACK - 20, rot=(90, 0, 0), segs=12)
        for y in (-HW + 10, HW - 10):
            m.box("TrimBlack", (x, y, 64), (98, 20, 4))
    m.sphere("TrimBlack", (REAR_AXLE, 0, WHEEL_R), 14, scale=(1.0, 0.9, 0.8), steps=12)
    m.box("TrimBlack", (-60, 58, 34), (70, 38, 22), bevel=2.0)
    m.cyl("SteelBrushed", (-40, -62, 30), 3.5, 380, rot=(0, 90, 0), segs=10)
    m.cyl("SteelBrushed", (-231, -62, 30), 4.0, 6, rot=(0, 90, 0), segs=12)
    # --- Cargo bay: liners, wheel wells, shelf, tie-down rails, dome light ----------------------------------
    for y in (-HW + 3.0, HW - 3.0):
        m.box("Wood", (-120, y, 106), (318, 1.2, 86))
        m.box("SteelBrushed", (-120, y - math.copysign(0.8, y), 128), (310, 0.8, 3))
        m.box("TrimBlack", (REAR_AXLE, y - math.copysign(10, y), 74), (96, 22, 26), bevel=2.0)
    for x in (-250.0, -160.0, -70.0):
        for y in (78.0, 95.0):
            m.box("SteelBrushed", (x, y, 118), (3, 3, 112))
    for z in (104.0, 138.0, 172.0):
        m.box("MetalPaintedGrey", (-160, 86.5, z), (186, 18, 1.6))
        m.box("SteelBrushed", (-160, 77.5, z + 2.5), (186, 0.6, 5))   # lip
    m.box("MetalPaintedGrey", (40, 0, 136), (3, 2 * HW - 4, 148))      # bulkhead behind the seats
    m.box("PlasticWhite", (-120, 0, ROOF - 1.0), (30, 10, 1.6))
    m.box("PlasticWhite", (60, 0, ROOF - 1.0), (18, 8, 1.6))
    # --- Cab: dashboard, gauges, steering wheel, engine cover, seats, mirrors, visors ------------------------
    beam(m, "TrimBlack", (148, 0, 120), (118, 0, 124), 2 * HW - 8, 16)
    m.box("TrimBlack", (132, 0, 104), (26, 2 * HW - 8, 18))
    m.box("TrimBlack", (122, -48, 120), (4, 42, 14), bevel=1.0)
    for dy in (-10.0, 10.0):
        m.cyl("ScreenDark", (119.8, -48 + dy, 121), 5.0, 0.6, rot=(0, 90, 0), segs=20)
        m.cyl("PlasticWhite", (119.6, -48 + dy, 121), 0.4, 0.4, rot=(0, 90, 0), segs=8)
    m.box("ScreenDark", (126, 10, 118), (2, 24, 8))                   # radio
    for dy in (-82.0, 82.0):
        m.box("TrimBlack", (127, dy, 124), (4, 10, 6))                # vents
    m.torus("TrimBlack", (108, -48, 120), 19, 1.8, rot=(0, 59.5, 0), segs=32, minor=8)
    m.box("TrimBlack", (108, -48, 120), (3.0, 34, 1.2), rot=(0, 59.5, 0))   # spoke bar across the wheel
    m.cyl("TrimBlack", (108, -48, 120), 4.0, 3.0, rot=(0, 59.5, 0), segs=16)
    beam(m, "TrimBlack", (108, -48, 120), (138, -48, 102), 6, 6)
    m.box("TrimBlack", (112, 0, 62), (62, 38, 28), bevel=4.0)        # engine cover between the seats
    for y in (-48.0, 48.0):
        m.box("TrimBlack", (72, y, 56), (40, 40, 16))
        m.box("PlasticGrey", (70, y, 70), (52, 50, 12), bevel=3.0)
        m.box("PlasticGrey", (44, y, 104), (10, 50, 62), rot=(0, 10, 0), bevel=3.0)
        m.box("PlasticGrey", (38, y, 144), (8, 26, 16), rot=(0, 10, 0), bevel=2.0)
        m.box("PlasticGrey", (96, y, 200), (22, 40, 1.5))            # sun visor
    m.box("TrimBlack", (100, 0, 196), (1.5, 24, 7), bevel=0.5)        # rear-view mirror
    m.cyl("SteelBrushed", (-160, HW + 0.5, 92), 6.5, 1.0, rot=(90, 0, 0), segs=20)  # fuel filler
    if old:
        # Roof rack with a ladder; rust at the arches and sills.
        for x in (-230.0, -130.0, -30.0, 50.0):
            m.box("SteelBrushed", (x, 0, ROOF + 10), (4, 2 * HW - 24, 4))
        for y in (-80.0, 80.0):
            m.box("SteelBrushed", (-90, y, ROOF + 7), (290, 4, 4))
            for x in (-230.0, -30.0, 50.0):
                m.box("SteelBrushed", (x, y, ROOF + 5), (4, 4, 7))
        m.box("Aluminium", (-90, 30, ROOF + 15), (260, 10, 3))
        for i in range(13):
            m.box("Aluminium", (-210 + i * 20, 30, ROOF + 15), (2, 36, 2))
        for y in (-HW - 0.3, HW + 0.3):
            m.box("RustHeavy", (REAR_AXLE + 30, y, 46), (40, 0.6, 10))
            m.box("Rust", (-245, y, 42), (30, 0.6, 6))
    else:
        for y in (-HW - 0.4, HW + 0.4):
            m.box("VanNewAccent", (-100, y, 108), (356, 0.8, 9))
    return m


def door_front(name, paint, side):
    """side = -1 (driver, left) or +1 (passenger)."""
    m = Model(name)
    y0 = HW + 0.6 if side > 0 else -HW + 2.4
    panel(m, paint, DOOR_PANEL, y0, 3.0, cuts=[poly_cut(DOOR_WINDOW, y0 + 5, 12)])
    yo = side * (HW + 0.6)
    panel(m, "Glass", DOOR_WINDOW, (HW - 0.6) if side > 0 else (-HW + 1.2), 0.6)
    m.box("TrimBlack", (60, yo + side * 1.0, 132), (14, 2.2, 3.0), bevel=0.6)      # handle
    m.cyl("Chrome", (73, yo + side * 0.6, 132), 0.9, 1.2, rot=(90, 0, 0), segs=10)
    # Big rectangular mirror on two arms.
    for z in (140.0, 158.0):
        m.box("TrimBlack", (136, side * (HW + 8), z), (2.5, 18, 2.5))
    m.box("TrimBlack", (136, side * (HW + 18), 149), (5, 17, 30), bevel=1.0)
    m.box("Glass", (133.4, side * (HW + 18), 149), (0.4, 14.5, 27))
    # Inner trim panel, arm rest, window crank.
    yi = side * (HW - 2.6)
    m.box("PlasticGrey", (92, yi, 90), (98, 1.2, 66))
    m.box("TrimBlack", (90, yi - side * 3.0, 104), (40, 6, 4), bevel=1.0)
    m.cyl("TrimBlack", (70, yi - side * 1.5, 112), 4.0, 1.0, rot=(90, 0, 0), segs=12)
    # Rubber seal along the window frame.
    beam(m, "TrimBlack", (52, yo, 189.5), (100, yo, 189.5), 1.2, 1.0)
    return m


def door_rear(name, paint, side):
    """side = -1 (left, hinge at y = -98) or +1 (right); the doors meet at y = 0."""
    m = Model(name)
    yc = side * 48.9
    m.box(paint, (-281.6, yc, 129), (3.0, 97.4, 134), bevel=0.6,
          cuts=[Model.cut_box((-281.6, yc, 166), (10, 70, 34))])
    m.box("Glass", (-281.4, yc, 166), (0.6, 71, 35))
    m.box("TrimBlack", (-283.2, yc, 166), (0.6, 74, 1.2))
    for z in (82.0, 128.0, 176.0):
        m.box("TrimBlack", (-283.3, side * 96.0, z), (2.2, 4.0, 8.0))    # hinges
    if side > 0:
        m.box("TrimBlack", (-283.6, 7.0, 122), (2.0, 3.0, 14), bevel=0.5)  # handle
        m.box("TrimBlack", (-283.4, 0.6, 129), (1.0, 1.4, 130))            # overlap strip
    m.box("MetalPaintedGrey", (-279.8, yc, 112), (0.8, 90, 90))              # inner panel
    return m


def wheel():
    m = Model("VanWheel")
    m.revolve("TireRubber", [(25, -11), (34, -11.5), (37.3, -9.5), (37.8, -6), (37.8, 6), (37.3, 9.5), (34, 11.5), (25, 11)], rot=(90, 0, 0), steps=48)
    for k in range(40):  # tread lugs
        a = k * 9.0
        r = math.radians(a)
        m.box("TireRubber", (37.9 * math.cos(r), (-4.5 if k % 2 else 4.5), 37.9 * math.sin(r)), (3.4, 8.0, 0.9), rot=(0, a - 90.0, 0))
    m.cyl("SteelBrushed", (0, 0, 0), 25.5, 19.0, rot=(90, 0, 0), segs=36)
    m.cyl("MetalPaintedGrey", (0, -8.0, 0), 22.0, 2.0, rot=(90, 0, 0), segs=36)
    for i in range(8):
        a = math.radians(i * 45 + 22.5)
        m.cyl("TrimBlack", (math.cos(a) * 15.5, -9.2, math.sin(a) * 15.5), 3.0, 0.6, rot=(90, 0, 0), segs=14)
    m.cyl("Chrome", (0, -9.6, 0), 8.5, 2.2, rot=(90, 0, 0), segs=24)
    for i in range(8):
        a = math.radians(i * 45)
        m.cyl("Chrome", (math.cos(a) * 6.2, -11.0, math.sin(a) * 6.2), 1.0, 1.8, rot=(90, 0, 0), segs=6)
    m.torus("SteelBrushed", (0, -9.5, 0), 25.0, 0.8, rot=(90, 0, 0), segs=36, minor=6)  # rim lip
    return m


def wheel_into(m, c, r, side, rim="SteelBrushed"):
    """A simple wheel baked into a parked car (side = -1 left, +1 right; the face points outwards)."""
    x, y, z = c
    k = r / 38.0
    m.revolve("TireRubber", [(25 * k, -10 * k), (34 * k, -10.5 * k), (37.6 * k, -7 * k), (37.6 * k, 7 * k), (34 * k, 10.5 * k), (25 * k, 10 * k)],
              c=(x, y, z), rot=(90, 0, 0), steps=28)
    m.cyl(rim, (x, y, z), 25 * k, 17 * k, rot=(90, 0, 0), segs=24)
    m.cyl("Chrome", (x, y + side * 8.6 * k, z), 9 * k, 1.5 * k, rot=(90, 0, 0), segs=16)


def parked_car(name, paint, kind="sedan"):
    """Parked cars of the neighbourhood (one mesh each, wheels included; nose +X, ground z = 0)."""
    m = Model(name)
    hw = 90.0 if kind == "sedan" else 100.0
    r = 32.0 if kind == "sedan" else 38.0
    axles = (145.0, -140.0) if kind == "sedan" else (170.0, -160.0)
    nose, tail = (242.0, -240.0) if kind == "sedan" else (268.0, -262.0)
    belt = 92.0 if kind == "sedan" else 112.0
    low = [(tail, 28), (tail, belt - 8), (tail + 30, belt), (nose - 120, belt), (nose - 10, belt - 14), (nose, belt - 30), (nose, 28)]
    panel(m, paint, low, hw, 2 * hw, bevel=4.0,
          cuts=[Model.cut_cyl((x, 0, r), r + 6, 2 * hw + 20, rot=(90, 0, 0), segs=28) for x in axles])
    if kind == "sedan":
        green = [(-150, belt), (-98, 136), (42, 138), (96, belt)]
        panel(m, "Glass", green, hw - 8, 2 * hw - 16)
        m.box(paint, (-28, 0, 138.5), (140, 2 * hw - 20, 4), bevel=1.5)
        for y in (-hw + 9, hw - 9):
            beam(m, paint, (96, y, belt), (42, y, 138), 6, 5)        # A pillar
            beam(m, paint, (-150, y, belt), (-98, y, 136), 12, 5)    # C pillar
            m.box(paint, (-25, y, 115), (6, 3, 44))                   # B pillar
    else:
        cab = [(-40, belt), (-40, 178), (60, 180), (120, belt)]
        panel(m, "Glass", cab, hw - 8, 2 * hw - 16)
        m.box(paint, (10, 0, 180), (104, 2 * hw - 20, 4), bevel=1.5)
        m.box(paint, (-41, 0, 145), (4, 2 * hw - 16, 66))
        for y in (-hw + 9, hw - 9):
            beam(m, paint, (120, y, belt), (60, y, 180), 7, 5)
            m.box(paint, (-38, y, 145), (8, 4, 66))
            m.box(paint, (-150, y, belt + 18), (220, 4, 36))         # bed sides
        m.box(paint, (-259, 0, belt + 18), (4, 2 * hw - 8, 36))      # tailgate
        m.box("TrimBlack", (-150, 0, belt - 4), (220, 2 * hw - 12, 3))  # bed floor
    for y in (-hw + 26, hw - 26):
        m.box("Headlight", (nose + 0.5, y, belt - 26), (2, 30, 12), bevel=0.5)
        m.box("Taillight", (tail - 0.5, y, belt - 18), (2, 24, 14), bevel=0.5)
    m.box("TrimBlack", (nose + 1, 0, belt - 26), (2, 2 * hw - 90, 12))   # grille
    m.box("Chrome" if kind == "pickup" else "TrimBlack", (nose + 4, 0, 42), (8, 2 * hw + 4, 14), bevel=2.0)
    m.box("Chrome" if kind == "pickup" else "TrimBlack", (tail - 4, 0, 42), (8, 2 * hw + 4, 14), bevel=2.0)
    for y in (-hw - 6, hw + 6):
        m.box("TrimBlack", ((96 if kind == "sedan" else 118), y, belt + 14), (6, 10, 10), bevel=1.0)   # mirrors
    for x in axles:
        for side in (-1, 1):
            wheel_into(m, (x, side * (hw - 10), r), r, side)
    m.box("TrimBlack", (0, 0, 24), ((nose - tail) - 60, 2 * hw - 40, 8))
    return m


CARS = {
    "CarSedanRed": ("PlasticRed", "sedan"), "CarSedanBlue": ("VanNewAccent", "sedan"), "CarSedanGrey": ("MetalPaintedGrey", "sedan"),
    "CarSedanBeige": ("PlasticBeige", "sedan"), "CarPickupGreen": ("MetalPaintedGreen", "pickup"), "CarPickupWhite": ("VanNew", "pickup"),
}

VARIANTS = {"VanOld": ("VanOld", True), "VanModern": ("VanNew", False)}


def main():
    built, failed = 0, []
    jobs = [("VanWheel", wheel)]
    for variant, (paint, old) in VARIANTS.items():
        jobs += [
            (f"{variant}_Body", lambda v=variant, p=paint, o=old: body(f"{v}_Body", p, o)),
            (f"{variant}_DoorFL", lambda v=variant, p=paint: door_front(f"{v}_DoorFL", p, -1)),
            (f"{variant}_DoorFR", lambda v=variant, p=paint: door_front(f"{v}_DoorFR", p, 1)),
            (f"{variant}_DoorRL", lambda v=variant, p=paint: door_rear(f"{v}_DoorRL", p, -1)),
            (f"{variant}_DoorRR", lambda v=variant, p=paint: door_rear(f"{v}_DoorRR", p, 1)),
        ]
    for car, (paint, kind) in CARS.items():
        jobs.append((car, lambda c=car, p=paint, k=kind: parked_car(c, p, k)))
    only = None
    for arg in unreal.SystemLibrary.get_command_line().split():
        if arg.startswith("-WEModels="):
            only = set(arg.split("=", 1)[1].split(","))
    for name, make in jobs:
        if only and name not in only:
            continue
        try:
            if make().save(normalize=False):
                built += 1
            else:
                failed.append(name)
        except Exception as e:
            failed.append(name)
            unreal.log_error(f"WE_VAN {name} failed: {e}")
    unreal.log(f"WE_VAN_BUILT {built} failed={failed}")
    unreal.SystemLibrary.quit_editor()


main()
