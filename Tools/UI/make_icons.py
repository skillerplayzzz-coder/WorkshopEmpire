"""
Phase 17.5: draws the UI icons, the hazard stripe, the Ferrit OS logo and wallpaper as PNG files into
SourceArt/UI (not in Git; Tools/UI/import_ui.py imports them into /Game/WorkshopEmpire/UI).

Plain Python 3, no packages: every icon is a signed distance field on a 24-unit grid (like common icon
sets), rendered with a one-pixel anti-aliased edge. White glyph on transparent - the UI tints it.
  python Tools/UI/make_icons.py
"""
import math
import os
import struct
import zlib

ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), "..", ".."))
OUT = os.path.join(ROOT, "SourceArt", "UI")
SIZE = 128


# --- PNG ---------------------------------------------------------------------------------------------
def write_png(path, width, height, rgba_rows):
    raw = b"".join(b"\x00" + bytes(row) for row in rgba_rows)

    def chunk(tag, data):
        c = struct.pack(">I", len(data)) + tag + data
        return c + struct.pack(">I", zlib.crc32(tag + data) & 0xFFFFFFFF)

    os.makedirs(os.path.dirname(path), exist_ok=True)
    with open(path, "wb") as f:
        f.write(b"\x89PNG\r\n\x1a\n")
        f.write(chunk(b"IHDR", struct.pack(">IIBBBBB", width, height, 8, 6, 0, 0, 0)))
        f.write(chunk(b"IDAT", zlib.compress(raw, 9)))
        f.write(chunk(b"IEND", b""))


# --- SDF primitives (24-unit grid, y down) ------------------------------------------------------------
def circle(cx, cy, r):
    return lambda x, y: math.hypot(x - cx, y - cy) - r


def ring(cx, cy, r, w):
    return lambda x, y: abs(math.hypot(x - cx, y - cy) - r) - w / 2


def rbox(cx, cy, hw, hh, r=0.0, angle=0.0):
    ca, sa = math.cos(math.radians(angle)), math.sin(math.radians(angle))

    def f(x, y):
        px, py = x - cx, y - cy
        lx, ly = px * ca + py * sa, -px * sa + py * ca
        qx, qy = abs(lx) - hw + r, abs(ly) - hh + r
        return math.hypot(max(qx, 0), max(qy, 0)) + min(max(qx, qy), 0) - r
    return f


def seg(ax, ay, bx, by, w):
    def f(x, y):
        pax, pay, bax, bay = x - ax, y - ay, bx - ax, by - ay
        h = max(0.0, min(1.0, (pax * bax + pay * bay) / (bax * bax + bay * bay + 1e-9)))
        return math.hypot(pax - bax * h, pay - bay * h) - w / 2
    return f


def poly(points):
    def f(x, y):
        d = float("inf")
        inside = False
        n = len(points)
        for i in range(n):
            ax, ay = points[i]
            bx, by = points[(i + 1) % n]
            ex, ey = bx - ax, by - ay
            wx, wy = x - ax, y - ay
            h = max(0.0, min(1.0, (wx * ex + wy * ey) / (ex * ex + ey * ey + 1e-9)))
            d = min(d, math.hypot(wx - ex * h, wy - ey * h))
            if (ay > y) != (by > y) and x < ax + (y - ay) * ex / (ey + 1e-12):
                inside = not inside
        return -d if inside else d
    return f


def polyline(points, w):
    parts = [seg(points[i][0], points[i][1], points[i + 1][0], points[i + 1][1], w) for i in range(len(points) - 1)]
    return union(*parts)


def arc(cx, cy, r, a0, a1, w):
    """Arc from angle a0 to a1 (degrees, 0 = +x, clockwise on screen because y is down)."""
    def f(x, y):
        ang = math.degrees(math.atan2(y - cy, x - cx)) % 360
        lo, hi = a0 % 360, a1 % 360
        within = lo <= ang <= hi if lo <= hi else (ang >= lo or ang <= hi)
        if within:
            return abs(math.hypot(x - cx, y - cy) - r) - w / 2
        e0 = (cx + r * math.cos(math.radians(a0)), cy + r * math.sin(math.radians(a0)))
        e1 = (cx + r * math.cos(math.radians(a1)), cy + r * math.sin(math.radians(a1)))
        return min(math.hypot(x - e0[0], y - e0[1]), math.hypot(x - e1[0], y - e1[1])) - w / 2
    return f


def union(*fs):
    return lambda x, y: min(f(x, y) for f in fs)


def sub(a, *bs):
    return lambda x, y: max(a(x, y), *(-b(x, y) for b in bs))


def inter(a, b):
    return lambda x, y: max(a(x, y), b(x, y))


def outline(f, w):
    return lambda x, y: abs(f(x, y)) - w / 2


def rotate(f, cx, cy, angle):
    ca, sa = math.cos(math.radians(-angle)), math.sin(math.radians(-angle))
    return lambda x, y: f(cx + (x - cx) * ca - (y - cy) * sa, cy + (x - cx) * sa + (y - cy) * ca)


def star(cx, cy, r_out, r_in, n=5):
    pts = []
    for i in range(n * 2):
        a = math.radians(-90 + i * 180 / n)
        r = r_out if i % 2 == 0 else r_in
        pts.append((cx + r * math.cos(a), cy + r * math.sin(a)))
    return poly(pts)


S = 1.6  # standard stroke width in grid units


# --- Icons ----------------------------------------------------------------------------------------------
def gear(cx=12, cy=12, r=6.2, teeth=8, hole=2.6):
    body = circle(cx, cy, r)
    tooth = [rotate(rbox(cx, cy - r - 1.1, 1.4, 1.9, 0.4), cx, cy, i * 360 / teeth) for i in range(teeth)]
    return sub(union(body, *tooth), circle(cx, cy, hole))


def person(cx=12, top=3.5, scale=1.0):
    head = circle(cx, top + 3.6 * scale, 3.4 * scale)
    body = inter(circle(cx, top + 18.5 * scale, 8.0 * scale), rbox(cx, top + 15.0 * scale, 8 * scale, 4.3 * scale))
    return union(head, body)


def wrench():
    head = sub(circle(7.0, 7.0, 4.6), rbox(4.6, 4.6, 1.6, 3.6, 0, 45), circle(7.0, 7.0, 1.6))
    handle = seg(8.5, 8.5, 19.5, 19.5, 3.0)
    return union(head, handle)


def truck():
    cargo = rbox(8.5, 11, 6.5, 5, 0.8)
    cab = poly([(15.5, 7.5), (19.5, 7.5), (22.5, 11.5), (22.5, 16), (15.5, 16)])
    window = poly([(17, 9), (19, 9), (21, 11.8), (17, 11.8)])
    wheels = union(circle(6.5, 17.5, 2.6), circle(18.5, 17.5, 2.6))
    hubs = union(circle(6.5, 17.5, 1.0), circle(18.5, 17.5, 1.0))
    return sub(union(sub(union(cargo, cab), window), wheels), hubs)


def euro_coin():
    rim = ring(12, 12, 9.2, S)
    c = arc(13.2, 12, 4.6, 40, 320, 1.7)
    bars = union(seg(7.2, 10.6, 12.8, 10.6, 1.4), seg(7.2, 13.4, 12.2, 13.4, 1.4))
    return union(rim, c, bars)


ICONS = {
    "Cart": lambda: union(polyline([(2.5, 4), (5.5, 4), (8, 15.5), (19, 15.5)], S),
                          outline(poly([(6.2, 7), (21, 7), (19.2, 13), (7.5, 13)]), S),
                          circle(9, 19.5, 1.8), circle(17.5, 19.5, 1.8)),
    "Gavel": lambda: union(rotate(union(rbox(12, 7, 5.5, 2.6, 0.6), rbox(12, 7, 6.5, 1.2, 0.4)), 12, 9, -45),
                           seg(11.5, 11.5, 19, 19, 2.6), rbox(9, 20.5, 6.5, 1.3, 0.5)),
    "Clipboard": lambda: union(outline(rbox(12, 13, 7, 8.5, 1.2), S), rbox(12, 4.6, 3.6, 1.8, 0.8),
                               seg(8.5, 11, 15.5, 11, S), seg(8.5, 14.5, 15.5, 14.5, S), seg(8.5, 18, 13, 18, S)),
    "Person": lambda: person(),
    "People": lambda: union(person(8.5, 5.5, 0.82), sub(person(16, 3.5, 0.9), circle(8.5, 14, 4.9))),
    "Chart": lambda: union(polyline([(3.5, 3), (3.5, 20.5), (21, 20.5)], S), rbox(8, 15.5, 1.6, 3.5, 0.3),
                           rbox(12.5, 12, 1.6, 7, 0.3), rbox(17, 14, 1.6, 5, 0.3)),
    "Factory": lambda: sub(poly([(2.5, 21), (2.5, 9), (8, 12.5), (8, 9), (13.5, 12.5), (13.5, 9), (19, 12.5), (19, 3), (21.5, 3), (21.5, 21)]),
                           rbox(6, 17, 1.4, 1.6), rbox(11, 17, 1.4, 1.6), rbox(16, 17, 1.4, 1.6)),
    "Wrench": wrench,
    "Truck": truck,
    "Gear": gear,
    "Power": lambda: poly([(13.5, 2), (5, 13.5), (11, 13.5), (9.5, 22), (19, 9.5), (12.8, 9.5)]),
    "Euro": euro_coin,
    "Warning": lambda: union(outline(poly([(12, 3), (22, 20.5), (2, 20.5)]), S * 1.1), seg(12, 9, 12, 14.5, 2.0), circle(12, 17.6, 1.2)),
    "Lock": lambda: union(sub(rbox(12, 15, 7, 6, 1.2), circle(12, 14.2, 1.4), rbox(12, 16.5, 0.6, 2)), arc(12, 9, 4.3, 180, 360, 2.0),
                          seg(7.7, 9, 7.7, 10, 2.0), seg(16.3, 9, 16.3, 10, 2.0)),
    "Unlock": lambda: union(sub(rbox(12, 15, 7, 6, 1.2), circle(12, 14.2, 1.4), rbox(12, 16.5, 0.6, 2)), arc(16.5, 7.5, 4.3, 180, 360, 2.0),
                            seg(20.8, 7.5, 20.8, 9, 2.0)),
    "Star": lambda: star(12, 12.6, 10, 4.2),
    "StarOutline": lambda: outline(star(12, 12.6, 9.4, 4.0), S),
    "Check": lambda: polyline([(4.5, 12.5), (9.5, 17.5), (19.5, 6.5)], 2.4),
    "Close": lambda: union(seg(6, 6, 18, 18, 2.2), seg(18, 6, 6, 18, 2.2)),
    "Minus": lambda: seg(6, 12, 18, 12, 2.2),
    "Plus": lambda: union(seg(6, 12, 18, 12, 2.2), seg(12, 6, 12, 18, 2.2)),
    "Info": lambda: union(ring(12, 12, 9.2, S), seg(12, 10.8, 12, 17, 2.0), circle(12, 7.4, 1.25)),
    "Save": lambda: sub(rbox(12, 12, 8.5, 8.5, 1.2), rbox(11, 6.2, 4.8, 2.6, 0.3), rbox(12, 15.8, 5.5, 3.6, 0.3)),
    "Play": lambda: poly([(7, 4), (20, 12), (7, 20)]),
    "Exit": lambda: union(polyline([(13, 6.5), (13, 3.5), (4, 3.5), (4, 20.5), (13, 20.5), (13, 17.5)], S),
                          seg(8.5, 12, 20, 12, S), polyline([(16.5, 8.5), (20, 12), (16.5, 15.5)], S)),
    "Bulb": lambda: union(sub(circle(12, 10, 6.6), circle(12, 10, 5.0)), seg(9.6, 15.5, 9.6, 17.6, S), seg(14.4, 15.5, 14.4, 17.6, S),
                          rbox(12, 18.6, 3.2, 1.0, 0.4), rbox(12, 21, 2.2, 0.8, 0.4), polyline([(10.2, 11), (12, 9), (13.8, 11)], 1.2)),
    "Roller": lambda: union(rbox(11, 5.5, 8, 3, 1.2), polyline([(19, 5.5), (21.5, 5.5), (21.5, 11), (12, 11), (12, 14)], S), rbox(12, 18, 1.6, 4.3, 0.6)),
    "Box": lambda: union(outline(poly([(12, 2.5), (21, 7), (21, 17), (12, 21.5), (3, 17), (3, 7)]), S),
                         polyline([(3, 7), (12, 11.5), (21, 7)], S), seg(12, 11.5, 12, 21.5, S)),
    "Tag": lambda: sub(poly([(3, 3), (12.5, 3), (21.5, 12), (12, 21.5), (3, 12.5)]), circle(7.8, 7.8, 1.9)),
    "Screw": lambda: union(sub(circle(12, 6.5, 4.6), seg(9.2, 6.5, 14.8, 6.5, 1.2), seg(12, 3.7, 12, 9.3, 1.2)),
                           rbox(12, 15, 1.6, 6.5, 0.4), polyline([(10, 12), (14, 13.5)], 0.9), polyline([(10, 15), (14, 16.5)], 0.9),
                           polyline([(10, 18), (14, 19.5)], 0.9)),
    "Hand": lambda: union(rbox(12, 16, 6, 5.5, 2.5), seg(7.2, 8, 7.2, 14, 2.2), seg(10.4, 5, 10.4, 13, 2.2), seg(13.6, 4.5, 13.6, 13, 2.2),
                          seg(16.8, 6, 16.8, 13.5, 2.2), seg(19, 13.5, 21, 10.5, 2.2)),
    "Search": lambda: union(ring(10, 10, 6, S * 1.1), seg(14.5, 14.5, 20.5, 20.5, 2.4)),
    "Clock": lambda: union(ring(12, 12, 9.2, S), seg(12, 12, 12, 6.5, S), seg(12, 12, 16, 14, S)),
    # Phase 19: day and night in the HUD clock.
    "Sun": lambda: union(circle(12, 12, 4.6), *[seg(12 + 7.2 * math.cos(math.radians(a)), 12 + 7.2 * math.sin(math.radians(a)), 12 + 10 * math.cos(math.radians(a)), 12 + 10 * math.sin(math.radians(a)), S) for a in range(0, 360, 45)]),
    "Moon": lambda: sub(circle(11.5, 12, 8.6), circle(16, 9, 7.4)),
    "Home": lambda: union(sub(poly([(12, 3), (21.5, 11.5), (19, 11.5), (19, 21), (5, 21), (5, 11.5), (2.5, 11.5)]), rbox(12, 17.5, 2.2, 3.6))),
    "Trash": lambda: union(sub(poly([(5.5, 7.5), (18.5, 7.5), (17, 21), (7, 21)]), seg(10, 10.5, 10.5, 18, 1.3), seg(14, 10.5, 13.5, 18, 1.3)),
                           rbox(12, 5.5, 8, 1.1, 0.4), rbox(12, 3.6, 2.6, 1.0, 0.4)),
    "Recycle": lambda: union(*[rotate(union(arc(12, 12, 7.6, 205, 290, 2.2), poly([(14.8, 2.4), (19.6, 5.8), (14.4, 8.2)])), 12, 12, i * 120) for i in range(3)]),
    "Folder": lambda: union(poly([(2.5, 5), (9.5, 5), (11.5, 7.5), (21.5, 7.5), (21.5, 19.5), (2.5, 19.5)])),
    "Mail": lambda: union(outline(rbox(12, 12, 9.5, 7, 1.0), S), polyline([(3, 6), (12, 13), (21, 6)], S)),
    "Bell": lambda: union(poly([(12, 3.5), (16.5, 6.5), (17.5, 14), (20.5, 17.5), (3.5, 17.5), (6.5, 14), (7.5, 6.5)]), circle(12, 20, 2.0)),
    "ChevronRight": lambda: polyline([(9, 5), (16, 12), (9, 19)], 2.4),
    "ChevronLeft": lambda: polyline([(15, 5), (8, 12), (15, 19)], 2.4),
    "ChevronDown": lambda: polyline([(5, 9), (12, 16), (19, 9)], 2.4),
    "Window": lambda: union(outline(rbox(12, 12, 9.5, 8, 1.0), S), rbox(12, 5.6, 9.5, 1.6)),
    "Grip": lambda: union(*[circle(9 + 6 * (i % 2), 6 + 6 * (i // 2), 1.6) for i in range(6)]),
    "Multimeter": lambda: union(sub(rbox(10, 12, 6.5, 9.5, 1.5), rbox(10, 7.5, 4.3, 2.8, 0.5), ring(10, 15.5, 2.4, 1.2)),
                                polyline([(16.5, 16), (20, 16), (21, 21.5)], 1.2)),
    "Screwdriver": lambda: rotate(union(rbox(12, 6.5, 3.0, 5.0, 1.4), rbox(12, 15.5, 0.9, 5.5), poly([(11.1, 20.5), (12.9, 20.5), (12, 22.5)])), 12, 12, 45),
    "Pliers": lambda: union(polyline([(4, 21), (10, 12.5), (12.5, 3.5)], 2.2), polyline([(20, 21), (14, 12.5), (11.5, 3.5)], 2.2), circle(12, 11.5, 1.8)),
    "SideCutter": lambda: union(polyline([(5, 21), (10.5, 12.5)], 2.4), polyline([(19, 21), (13.5, 12.5)], 2.4),
                                poly([(9.5, 13), (12, 3), (12.2, 12)]), poly([(14.5, 13), (12, 3), (11.8, 12)]), circle(12, 12, 1.8)),
    "Hammer": lambda: union(rbox(12.5, 5.5, 8, 2.6, 0.8), seg(12.5, 7, 12.5, 21.5, 2.6)),
    "SolderingIron": lambda: rotate(union(rbox(12, 6, 2.6, 5.5, 1.2), rbox(12, 14, 1.2, 3.5), poly([(11.2, 17.5), (12.8, 17.5), (12, 22)])), 12, 12, 40),
    "AngleGrinder": lambda: union(rbox(14, 12, 7.5, 2.8, 1.2), circle(6.5, 11, 4.5), seg(16, 9, 16, 5.5, 2.2)),
    "Disassemble": lambda: union(sub(rbox(9, 12, 6, 7, 1.0), rbox(9, 12, 3.8, 4.8, 0.3)), poly([(17, 6), (21.5, 9), (17, 12)]), seg(14, 9, 18, 9, 1.4)),
    "Repair": lambda: union(wrench(), rbox(18, 6, 1.0, 3.2), rbox(18, 6, 3.2, 1.0)),
    "Paint": lambda: union(poly([(4, 13), (13, 4), (20, 11), (11, 20)]), circle(19.5, 19.5, 2.2)),
    "Floor": lambda: union(*[rbox(6 + 6 * (i % 3), 6 + 6 * (i // 3), 2.6, 2.6, 0.4) for i in range(9)]),
    "Lamp": lambda: union(seg(12, 2, 12, 7, S), poly([(6, 13), (8.5, 7), (15.5, 7), (18, 13)]), arc(12, 14, 3, 0, 180, S),
                          seg(5, 18.5, 3.5, 20.5, 1.2), seg(19, 18.5, 20.5, 20.5, 1.2), seg(12, 19.5, 12, 22, 1.2)),
    "Door": lambda: union(outline(rbox(12, 12.5, 8.5, 9, 0.5), S), seg(4, 8, 20, 8, 1.1), seg(4, 11.5, 20, 11.5, 1.1), seg(4, 15, 20, 15, 1.1), seg(4, 18.5, 20, 18.5, 1.1)),
    "Building": lambda: sub(rbox(12, 13, 8.5, 9, 0.5), *[rbox(8.5 + 7 * (i % 2), 8 + 5 * (i // 2), 1.8, 1.5) for i in range(4)], rbox(12, 19, 2, 3)),
    "Shield": lambda: poly([(12, 2.5), (20.5, 5.5), (19.5, 13), (12, 21.5), (4.5, 13), (3.5, 5.5)]),
    "Sell": lambda: union(sub(poly([(3, 3), (12.5, 3), (21.5, 12), (12, 21.5), (3, 12.5)]), circle(7.8, 7.8, 1.9), seg(10.5, 13.5, 15, 9, 1.4))),
    "Download": lambda: union(seg(12, 3.5, 12, 15, 2.2), polyline([(7, 10.5), (12, 15.5), (17, 10.5)], 2.2), seg(4, 20, 20, 20, 2.2)),
    "Desktop": lambda: union(outline(rbox(12, 10, 9.5, 6.5, 0.8), S), seg(12, 16.5, 12, 19.5, 2), seg(8, 20.5, 16, 20.5, 2)),
    "Keyboard": lambda: union(outline(rbox(12, 12, 10, 6, 0.8), S), *[rbox(6 + 3 * (i % 5), 10 + 3 * (i // 5), 0.8, 0.8) for i in range(10)]),
    "Medal": lambda: union(poly([(8, 2.5), (11, 2.5), (13.5, 9), (10.5, 10)]), poly([(16, 2.5), (13, 2.5), (10.5, 9), (13.5, 10)]),
                           sub(circle(12, 15.5, 6), star(12, 15.8, 3.8, 1.6))),
    "Map": lambda: union(outline(poly([(3, 5), (9, 3), (15, 5), (21, 3), (21, 19), (15, 21), (9, 19), (3, 21)]), S), seg(9, 3, 9, 19, S), seg(15, 5, 15, 21, S)),
}


def render_icon(name, f, size=SIZE):
    scale = size / 24.0
    rows = []
    for py in range(size):
        row = bytearray()
        y = (py + 0.5) / scale
        for px in range(size):
            x = (px + 0.5) / scale
            d = f(x, y) * scale
            a = max(0.0, min(1.0, 0.5 - d))
            row += bytes((255, 255, 255, int(round(a * 255))))
        rows.append(row)
    write_png(os.path.join(OUT, "Icons", f"T_Icon_{name}.png"), size, size, rows)


# --- Textures ---------------------------------------------------------------------------------------------
def hazard_stripe():
    """64x64 tile: diagonal safety stripes (amber / near black), tiles seamlessly."""
    n = 64
    rows = []
    for y in range(n):
        row = bytearray()
        for x in range(n):
            t = ((x + y) % 32) / 32.0
            # Soft edges between the bands (two anti-aliased edges per period).
            edge = min(abs(t - 0.0), abs(t - 0.5), abs(t - 1.0)) * 32.0
            amber = 1.0 if t < 0.5 else 0.0
            a = min(1.0, edge / 0.8)
            v = amber if a >= 1.0 else 0.5 + (amber - 0.5) * a
            r, g, b = (int(232 * v + 22 * (1 - v)), int(170 * v + 22 * (1 - v)), int(38 * v + 24 * (1 - v)))
            row += bytes((r, g, b, 255))
        rows.append(row)
    write_png(os.path.join(OUT, "T_HazardStripe.png"), n, n, rows)


def os_logo():
    """Ferrit OS logo: hexagon nut with an F cut out."""
    size = 256
    hexagon = poly([(12 + 10.5 * math.cos(math.radians(a)), 12 + 10.5 * math.sin(math.radians(a))) for a in range(30, 390, 60)])
    f_shape = union(rbox(10.2, 12, 1.4, 6), rbox(12.6, 7.4, 3.8, 1.4), rbox(12.0, 11.8, 3.2, 1.3))
    render_icon_custom("T_FerritLogo", sub(hexagon, f_shape), size)


def render_icon_custom(name, f, size):
    scale = size / 24.0
    rows = []
    for py in range(size):
        row = bytearray()
        y = (py + 0.5) / scale
        for px in range(size):
            x = (px + 0.5) / scale
            a = max(0.0, min(1.0, 0.5 - f(x, y) * scale))
            row += bytes((255, 255, 255, int(round(a * 255))))
        rows.append(row)
    write_png(os.path.join(OUT, f"{name}.png"), size, size, rows)


def wallpaper():
    """960x540 desktop background: dark blue-grey gradient, fine blueprint grid, soft light from the upper left."""
    w, h = 960, 540
    rows = []
    for y in range(h):
        row = bytearray()
        for x in range(w):
            u, v = x / w, y / h
            glow = math.exp(-((u - 0.22) ** 2 * 3.0 + (v - 0.25) ** 2 * 4.0))
            base = 0.10 + 0.10 * glow - 0.04 * v
            grid = 1.0 if (x % 48 == 0 or y % 48 == 0) else (0.45 if (x % 12 == 0 or y % 12 == 0) else 0.0)
            r = base * 0.75 + grid * 0.020
            g = base * 0.92 + grid * 0.030
            b = base * 1.20 + grid * 0.045
            row += bytes((min(255, int(r * 255)), min(255, int(g * 255)), min(255, int(b * 255)), 255))
        rows.append(row)
    write_png(os.path.join(OUT, "T_FerritWallpaper.png"), w, h, rows)


def panel_shadow():
    """Phase 19: 64x64 soft shadow for panels, drawn as a 9-slice box (16-px soft border, solid middle)."""
    n, m = 64, 16
    rows = []
    for y in range(n):
        row = bytearray()
        for x in range(n):
            dx = max(m - x - 0.5, 0.0, x + 0.5 - (n - m))
            dy = max(m - y - 0.5, 0.0, y + 0.5 - (n - m))
            d = math.hypot(dx, dy) / m
            t = max(0.0, 1.0 - d)
            a = t * t * (3 - 2 * t) * 0.6
            row += bytes((0, 0, 0, int(round(a * 255))))
        rows.append(row)
    write_png(os.path.join(OUT, "T_PanelShadow.png"), n, n, rows)


def main():
    for name, make in ICONS.items():
        render_icon(name, make())
    panel_shadow()
    hazard_stripe()
    os_logo()
    wallpaper()
    print(f"WE_ICONS_DONE {len(ICONS)} icons")


if __name__ == "__main__":
    main()
