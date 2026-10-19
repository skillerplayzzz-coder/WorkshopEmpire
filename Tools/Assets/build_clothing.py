"""
Phase 19: real clothes and more faces for the player figures (no downloads).

The Quaternius base bodies (import_characters.py) come without clothes. Every garment here is cut out of the
body itself with Geometry Script: the body regions of a garment (torso, sleeves, legs, feet) are kept, pushed
outwards and given a thickness, then collars, belts, pockets, hoods, knee pads, soles and so on are added. The
skin weights are transferred from the body, so a garment is a skeletal mesh on the body's skeleton and follows
the body as a follower mesh (UWEAvatarComponent).

  /Game/Characters/Quaternius/Clothing/SK_<Male|Female>_<Piece>   garments, slots Main / Accent / Trim
  /Game/Characters/Quaternius/M_WECloth                            fabric material (Tint, Pattern)
  /Game/Characters/Quaternius/Hair/Hair_Moustache|Hair_Goatee      cut from the full beard

Run after import_characters.py:
  UnrealEditor-Cmd.exe <uproject> -ExecutePythonScript="<this file>" -unattended -RenderOffscreen -nosplash -nosteam
"""
import math

import unreal

AU = unreal.GeometryScript_AssetUtils
Q = unreal.GeometryScript_MeshQueries
E = unreal.GeometryScript_MeshEdits
MOD = unreal.GeometryScript_MeshModeling
BW = unreal.GeometryScript_BoneWeights
NEW = unreal.GeometryScript_NewAssetUtils
MAT = unreal.GeometryScript_Materials
P = unreal.GeometryScript_Primitives
N = unreal.GeometryScript_Normals
L = unreal.GeometryScript_List
eal = unreal.EditorAssetLibrary
mel = unreal.MaterialEditingLibrary
tools = unreal.AssetToolsHelpers.get_asset_tools()

DEST = "/Game/Characters/Quaternius"
OUT = f"{DEST}/Clothing"
CENTER = unreal.GeometryScriptPrimitiveOriginMode.CENTER

# Body proportions, the same numbers UWEAvatarComponent gives M_WEAvatar (the painted clothes underneath).
DIMS = {
    "Male": {"H": 182.0, "TorsoX": 20.0, "NeckZ": 0.82, "WaistZ": 0.56, "ArmZ": 0.70, "Short": 34.0, "Long": 73.0},
    "Female": {"H": 178.0, "TorsoX": 18.0, "NeckZ": 0.815, "WaistZ": 0.56, "ArmZ": 0.70, "Short": 31.0, "Long": 65.0},
}


def fn(lib, *names):
    for n in names:
        if hasattr(lib, n):
            return getattr(lib, n)
    raise AttributeError(f"{lib} has none of {names}")


def pick(result, cls):
    """The out parameter of a given type from a Geometry Script call (they also return the mesh)."""
    for r in (result if isinstance(result, tuple) else (result,)):
        if isinstance(r, cls):
            return r
    raise TypeError(f"no {cls} in {result}")


def index_list(ids):
    r = L.convert_array_to_index_list(list(ids), unreal.GeometryScriptIndexType.TRIANGLE)
    return pick(r, unreal.GeometryScriptIndexList)


def first(result):
    return result[0] if isinstance(result, tuple) else result


def xf(loc=(0, 0, 0), rot=(0, 0, 0), scale=(1, 1, 1)):
    """rot = (roll, pitch, yaw) in degrees."""
    return unreal.Transform(unreal.Vector(*loc), unreal.Rotator(roll=rot[0], pitch=rot[1], yaw=rot[2]), unreal.Vector(*scale))


def popts(material_id):
    o = unreal.GeometryScriptPrimitiveOptions()
    o.set_editor_property("material_id", material_id)
    return o


# --- Body data --------------------------------------------------------------------------------------------------
class Body:
    def __init__(self, gender):
        self.gender = gender
        name = f"Superhero_{gender}_FullBody"
        self.asset = unreal.load_asset(f"{DEST}/{gender}/{name}/SkeletalMeshes/{name}")
        self.skeleton = self.asset.get_editor_property("skeleton")
        self.mesh = self.copy()
        self.verts = [(v.x, v.y, v.z) for v in L.convert_vector_list_to_array(pick(Q.get_all_vertex_positions(self.mesh, False), unreal.GeometryScriptVectorList))]
        self.tris = [(t.x, t.y, t.z) for t in L.convert_triangle_list_to_array(pick(Q.get_all_triangle_indices(self.mesh, False), unreal.GeometryScriptTriangleList))]
        self.d = dict(DIMS[gender])
        self.d["H"] = max(v[2] for v in self.verts)
        zs = [v[2] for v in self.verts]
        unreal.log(f"WE_CLOTH {gender}: {len(self.verts)} verts, {len(self.tris)} tris, z {min(zs):.1f}..{max(zs):.1f}, "
                   f"x {min(v[0] for v in self.verts):.1f}..{max(v[0] for v in self.verts):.1f}, y {min(v[1] for v in self.verts):.1f}..{max(v[1] for v in self.verts):.1f}")

    def copy(self):
        m = unreal.DynamicMesh()
        AU.copy_mesh_from_skeletal_mesh(self.asset, m, unreal.GeometryScriptCopyMeshFromAssetOptions(), unreal.GeometryScriptMeshReadLOD())
        return m

    def centroid(self, t):
        a, b, c = (self.verts[i] for i in self.tris[t])
        return ((a[0] + b[0] + c[0]) / 3.0, (a[1] + b[1] + c[1]) / 3.0, (a[2] + b[2] + c[2]) / 3.0)

    # Axes of torso, legs and arms (for draping the cloth outwards from them).
    def axes(self):
        if hasattr(self, "_axes"):
            return self._axes
        d, H = self.d, self.d["H"]
        # Crotch: the lowest height where the body is closed between the legs.
        crotch = 0.3 * H
        for k in range(200):
            z = 0.3 * H + k * 0.5
            if any(abs(v[0]) < 1.2 and abs(v[2] - z) < 0.6 for v in self.verts):
                crotch = z
                break
        torso, legs, arms = {}, {-1: {}, 1: {}}, {-1: {}, 1: {}}
        for v in self.verts:
            x, y, z = v
            ax = abs(x)
            zi = int(z // 2)
            if z >= crotch - 2.0 and ax < 12.0:
                torso.setdefault(zi, []).append(y)
            if z < crotch + 4.0 and ax < 24.0 and ax > 0.5:
                legs[1 if x > 0 else -1].setdefault(zi, []).append((x, y))
            if z / H > d["ArmZ"] - 0.04 and ax > d["TorsoX"] - 6.0:
                arms[1 if x > 0 else -1].setdefault(int(ax // 2), []).append((y, z))
        self._axes = {
            "crotch": crotch,
            "torso": {k: sum(v) / len(v) for k, v in torso.items()},
            "legs": {s: {k: (sum(p[0] for p in v) / len(v), sum(p[1] for p in v) / len(v)) for k, v in legs[s].items()} for s in (-1, 1)},
            "arms": {s: {k: (sum(p[0] for p in v) / len(v), sum(p[1] for p in v) / len(v)) for k, v in arms[s].items()} for s in (-1, 1)},
        }
        unreal.log(f"WE_CLOTH {self.gender}: crotch at {crotch:.1f}")
        return self._axes

    @staticmethod
    def _near(table, key):
        if key in table:
            return table[key]
        for off in range(1, 40):
            for k in (key - off, key + off):
                if k in table:
                    return table[k]
        return None

    def radial(self, p, kind):
        """(axis point, unit radial direction, radius, along-axis direction) of a point for a body part."""
        a = self.axes()
        x, y, z = p
        if kind == "arm":
            s = 1 if x > 0 else -1
            cy, cz = self._near(a["arms"][s], int(abs(x) // 2)) or (0.0, z)
            dy, dz = y - cy, z - cz
            r = math.hypot(dy, dz) or 1e-3
            return (x, cy, cz), (0.0, dy / r, dz / r), r, (1.0, 0.0, 0.0)
        if kind == "leg":
            s = 1 if x > 0 else -1
            cx, cy = self._near(a["legs"][s], int(z // 2)) or (s * 10.0, 0.0)
        else:
            cx, cy = 0.0, self._near(a["torso"], int(z // 2)) or 0.0
        dx, dy = x - cx, y - cy
        r = math.hypot(dx, dy) or 1e-3
        return (cx, cy, z), (dx / r, dy / r, 0.0), r, (0.0, 0.0, 1.0)

    # Cross sections (centre and the outline radius per direction) for rings around neck, waist and so on.
    def section(self, z, max_ax, side=0, band=1.6):
        pts = [v for v in self.verts if abs(v[2] - z) < band and abs(v[0]) < max_ax and (side == 0 or v[0] * side > 0)]
        if not pts:
            return None
        cx = sum(p[0] for p in pts) / len(pts)
        cy = sum(p[1] for p in pts) / len(pts)
        bins = [0.0] * 36
        for p in pts:
            a = math.atan2(p[1] - cy, p[0] - cx)
            i = int((a + math.pi) / (2 * math.pi) * 36) % 36
            bins[i] = max(bins[i], math.hypot(p[0] - cx, p[1] - cy))
        # Empty bins take the neighbours' radius.
        for _ in range(3):
            for i in range(36):
                if bins[i] == 0.0:
                    bins[i] = max(bins[(i - 1) % 36], bins[(i + 1) % 36])
        return {"c": (cx, cy, z), "r": bins, "front": max(p[1] for p in pts), "back": min(p[1] for p in pts),
                "minx": min(p[0] for p in pts), "maxx": max(p[0] for p in pts)}


# --- Regions (body space: Z up, the figure looks along +Y, arms along X in the T-pose) -------------------------------
def shirt_region(d, sleeve, hem, neck):
    H, tx = d["H"], d["TorsoX"]
    shoulder_top = d["NeckZ"] + 0.05

    def keep(p):
        zn, ax = p[2] / H, abs(p[0])
        torso = zn >= hem and ((ax < 9.0 and zn < neck) or (ax >= 9.0 and ax < tx and zn < shoulder_top))
        arm = tx <= ax < sleeve and zn > d["ArmZ"] and zn < shoulder_top + 0.03
        return torso or arm
    return keep


def pants_region(d, top, bottom_cm):
    H = d["H"]

    def keep(p):
        return p[2] >= bottom_cm and p[2] / H < top and abs(p[0]) < 24.0
    return keep


def shoe_region(height_cm):
    def keep(p):
        return p[2] < height_cm
    return keep


# --- Garment ------------------------------------------------------------------------------------------------------
class Garment:
    """One garment: body triangles kept by a region, pushed out and thickened, plus extra parts."""

    def __init__(self, body, name):
        self.body = body
        self.name = name
        self.mesh = None
        self.extras = unreal.DynamicMesh()

    def drape(self, m, ease, iterations=14):
        """Loose cloth instead of a second skin: the cut is smoothed (hollows get bridged, the hem straightened),
        never closer to the skin than it was, then pushed outwards from the limb or torso axis by ease(p, kind)."""
        b = self.body
        d, H = b.d, b.d["H"]
        pos = [(v.x, v.y, v.z) for v in L.convert_vector_list_to_array(pick(Q.get_all_vertex_positions(m, False), unreal.GeometryScriptVectorList))]
        tris = [(t.x, t.y, t.z) for t in L.convert_triangle_list_to_array(pick(Q.get_all_triangle_indices(m, True), unreal.GeometryScriptTriangleList))]
        nbrs, edges = {}, {}
        for a, bb, c in tris:
            for u, v in ((a, bb), (bb, c), (c, a)):
                nbrs.setdefault(u, set()).add(v)
                nbrs.setdefault(v, set()).add(u)
                key = (min(u, v), max(u, v))
                edges[key] = edges.get(key, 0) + 1
        bnbrs = {}
        for (u, v), n in edges.items():
            if n == 1:
                bnbrs.setdefault(u, set()).add(v)
                bnbrs.setdefault(v, set()).add(u)
        used = list(nbrs.keys())
        orig = list(pos)
        cur = list(pos)
        crotch = b.axes()["crotch"]
        # No bridging between the legs (it would make a web in the crotch).
        rate = {v: 0.0 if (abs(orig[v][0]) < 9.0 and crotch - 34.0 < orig[v][2] < crotch + 3.0) else 0.5 for v in used}
        for _ in range(iterations):
            nxt = list(cur)
            for v in used:
                ns = bnbrs.get(v) or nbrs[v]
                ax = sum(cur[n][0] for n in ns) / len(ns)
                ay = sum(cur[n][1] for n in ns) / len(ns)
                az = sum(cur[n][2] for n in ns) / len(ns)
                p = cur[v]
                k = rate[v]
                nxt[v] = (p[0] + (ax - p[0]) * k, p[1] + (ay - p[1]) * k, p[2] + (az - p[2]) * k)
            cur = nxt

        def smooth(e0, e1, x):
            t = min(1.0, max(0.0, (x - e0) / (e1 - e0)))
            return t * t * (3 - 2 * t)

        out = list(pos)
        for v in used:
            o, s = orig[v], cur[v]
            ax = abs(o[0])
            wa = smooth(d["TorsoX"] - 3.0, d["TorsoX"] + 4.0, ax) * (1.0 if o[2] / H > d["ArmZ"] - 0.03 else 0.0)
            wl = smooth(crotch + 3.0, crotch - 5.0, o[2]) * (1.0 - wa)
            if o[2] > crotch - 8.0:
                wl *= smooth(1.5, 6.0, ax)  # the middle of the crotch belongs to neither leg
            wt = max(0.0, 1.0 - wa - wl)
            acc, wsum = [0.0, 0.0, 0.0], 0.0
            for kind, w in (("arm", wa), ("leg", wl), ("torso", wt)):
                if w <= 0.0:
                    continue
                a, dr, r_o, along = b.radial(o, kind)
                r_s = (s[0] - a[0]) * dr[0] + (s[1] - a[1]) * dr[1] + (s[2] - a[2]) * dr[2]
                radius = max(r_s, r_o + 0.35) + ease(o, kind)
                base = (s[0], a[1], a[2]) if kind == "arm" else (a[0], a[1], s[2])
                p = (base[0] + dr[0] * radius, base[1] + dr[1] * radius, base[2] + dr[2] * radius)
                acc = [acc[i] + p[i] * w for i in range(3)]
                wsum += w
            out[v] = tuple(c / wsum for c in acc) if wsum > 0 else o
        E.set_all_mesh_vertex_positions(m, pick(L.convert_array_to_vector_list([unreal.Vector(*p) for p in out]), unreal.GeometryScriptVectorList))

    def cut(self, keep, offset, thickness, material=None, drape=True):
        """material(centroid) -> slot id (0 main, 1 accent, 2 trim) for the kept triangles."""
        b = self.body
        m = b.copy()
        fn(MAT, "enable_material_i_ds", "enable_material_ids")(m)
        fn(MAT, "clear_material_i_ds", "clear_material_ids")(m, 0)
        drop, by_id = [], {1: [], 2: []}
        for t in range(len(b.tris)):
            c = b.centroid(t)
            if not keep(c):
                drop.append(t)
            elif material:
                mid = material(c)
                if mid in by_id:
                    by_id[mid].append(t)
        for mid, tris in by_id.items():
            if tris:
                MAT.set_material_id_on_triangles(m, index_list(tris), mid, False)
        E.delete_triangles_from_mesh(m, index_list(drop), False)
        if callable(offset) or drape:
            self.drape(m, offset if callable(offset) else (lambda p, kind, o=offset: o))
        else:
            o = unreal.GeometryScriptMeshOffsetOptions()
            o.set_editor_property("offset_distance", offset)
            o.set_editor_property("solve_steps", 3)
            o.set_editor_property("smooth_alpha", 0.05)
            MOD.apply_mesh_offset(m, o)
        s = unreal.GeometryScriptMeshOffsetOptions()
        s.set_editor_property("offset_distance", thickness)
        s.set_editor_property("solve_steps", 2)
        s.set_editor_property("smooth_alpha", 0.0)
        MOD.apply_mesh_shell(m, s)
        if self.mesh is None:
            self.mesh = m
        else:
            E.append_mesh(self.mesh, m, xf())
        return self

    # Extra parts (in body space).
    def box(self, mid, c, s, rot=(0, 0, 0)):
        P.append_box(self.extras, popts(mid), xf(c, rot), s[0], s[1], s[2], 0, 0, 0, CENTER)

    def cyl(self, mid, c, r, h, rot=(0, 0, 0), segs=10):
        P.append_cylinder(self.extras, popts(mid), xf(c, rot), r, h, segs, 0, True, CENTER)

    def sphere(self, mid, c, r, scale=(1, 1, 1), steps=14):
        P.append_sphere_lat_long(self.extras, popts(mid), xf(c, (0, 0, 0), scale), r, steps, steps, CENTER)

    def ring(self, mid, sec, out, height, thick, z=None, arc=None, segs=48):
        """A band around a body section: boxes along the outline, `out` cm off the skin. arc=(from, to) in degrees
        (0 = +X, 90 = front) limits it to a part of the way round."""
        if not sec:
            return
        cx, cy, cz = sec["c"]
        z = cz if z is None else z
        # Smoothed outline, so the band runs round instead of following every bump.
        radii = [sum(sec["r"][(i + k) % 36] for k in range(-2, 3)) / 5.0 for i in range(36)]
        for i in range(segs):
            a0 = -math.pi + (i + 0.5) * 2 * math.pi / segs
            deg = math.degrees(a0)
            if arc and not (arc[0] <= deg <= arc[1]):
                continue
            r = radii[int((a0 + math.pi) / (2 * math.pi) * 36) % 36] + out + thick * 0.5
            length = 2 * math.pi * r / segs * 1.12
            self.box(mid, (cx + math.cos(a0) * r, cy + math.sin(a0) * r, z), (length, thick, height), (0, 0, deg + 90.0))

    def save(self):
        b = self.body
        if self.mesh is None:
            return None
        E.append_mesh(self.mesh, self.extras, xf())
        # Skin weights from the body: every point follows the closest place on the skin.
        src = b.copy()
        to = unreal.GeometryScriptTransferBoneWeightsOptions()
        BW.transfer_bone_weights_from_mesh(src, self.mesh, to)
        so = unreal.GeometryScriptSplitNormalsOptions()
        so.set_editor_property("split_by_opening_angle", True)
        so.set_editor_property("opening_angle_deg", 50.0)
        N.compute_split_normals(self.mesh, so, unreal.GeometryScriptCalculateNormalsOptions())
        path = f"{OUT}/SK_{b.gender}_{self.name}"
        if eal.does_asset_exist(path):
            eal.delete_asset(path)
        opts = unreal.GeometryScriptCreateNewSkeletalMeshAssetOptions()
        opts.set_editor_property("enable_recompute_normals", False)
        opts.set_editor_property("enable_recompute_tangents", True)
        cloth = unreal.load_asset(f"{DEST}/M_WECloth")
        opts.set_editor_property("materials", {"Main": cloth, "Accent": cloth, "Trim": cloth})
        sk = first(NEW.create_new_skeletal_mesh_asset_from_mesh(self.mesh, b.skeleton, path, opts))
        if not sk:
            unreal.log_error(f"WE_CLOTH {path}: not created")
            return None
        eal.save_asset(path)
        tris = fn(Q, "get_num_triangle_i_ds", "get_num_triangle_ids")(self.mesh)
        unreal.log(f"WE_CLOTH {path}: {tris} tris")
        return sk


# --- The wardrobe ---------------------------------------------------------------------------------------------------
def wardrobe(b):
    d, H = b.d, b.d["H"]
    gender = b.gender
    neck = b.section(d["NeckZ"] * H + 1.5, 9.0)
    waist = b.section(d["WaistZ"] * H, 22.0)
    hip = b.section((d["WaistZ"] - 0.04) * H, 24.0)
    chest_z = (d["NeckZ"] - 0.10) * H
    chest = b.section(chest_z, 16.0)
    belly = b.section((d["WaistZ"] + 0.05) * H, 18.0)
    knees = {s: b.section(0.29 * H, 24.0, s) for s in (-1, 1)}
    thighs = {s: b.section(0.43 * H, 24.0, s) for s in (-1, 1)}
    feet = {s: b.section(3.0, 24.0, s, band=3.0) for s in (-1, 1)}
    unreal.log(f"WE_CLOTH {gender}: neck r={max(neck['r']):.1f} c={neck['c']}, waist front={waist['front']:.1f} back={waist['back']:.1f}, "
               f"chest front={chest['front']:.1f}, feet {[(round(f['minx'], 1), round(f['maxx'], 1), round(f['back'], 1), round(f['front'], 1)) for f in feet.values()]}")
    W = d["WaistZ"]
    pants_bottom = 8.0
    crotch = b.axes()["crotch"]

    def tube(top, base, flare):
        """Ease of a garment: `top` on the torso and arms, legs from `base` at the crotch to base + flare at the hem."""
        def ease(p, kind):
            if kind == "leg":
                return base + flare * min(1.0, max(0.0, (crotch - p[2]) / max(crotch - pants_bottom, 1.0)))
            return top
        return ease

    def zip_strip(g, mid, z0, z1, out, width=1.4):
        """Boxes up the front, following the chest."""
        z = z0
        while z < z1:
            sec = b.section(z, 6.0, band=1.2)
            if sec:
                g.box(mid, (0.0, sec["front"] + out, z + 1.0), (width, 0.5, 2.2))
            z += 2.0

    # 0: work overall - one piece with long sleeves, reflective stripes, zip and a chest pocket.
    g = Garment(b, "Overall")
    g.cut(lambda p: shirt_region(d, d["Long"], W - 0.02, d["NeckZ"])(p) or pants_region(d, W + 0.02, pants_bottom)(p), tube(1.8, 1.4, 1.6), 0.45)
    g.ring(0, neck, 2.0, 3.2, 1.2, z=d["NeckZ"] * H)
    g.ring(1, b.section(0.665 * H, d["TorsoX"] - 1.0), 2.2, 2.6, 0.5)
    for s in (-1, 1):
        g.ring(1, b.section(0.30 * H, 24.0, s), 2.0 + 1.6 * (crotch - 0.30 * H) / max(crotch - pants_bottom, 1.0) + 0.3, 2.6, 0.5)
    zip_strip(g, 2, (W - 0.02) * H, d["NeckZ"] * H - 3.0, 2.35)
    px = 7.5 if gender == "Male" else 8.0
    g.box(0, (px, chest["front"] + 2.7, chest_z + 1.0), (9.0, 0.9, 10.0))
    g.box(1, (px, chest["front"] + 3.2, chest_z + 5.2), (9.4, 1.0, 2.6))
    g.save()

    # 1: polo shirt + chinos with a belt.
    g = Garment(b, "Polo")
    g.cut(shirt_region(d, d["Short"], W + 0.028, d["NeckZ"]), tube(1.4, 0, 0), 0.35)
    g.ring(0, neck, 1.7, 2.6, 1.0, z=d["NeckZ"] * H + 0.4)
    g.box(1, (0.0, chest["front"] + 2.0, d["NeckZ"] * H - 5.5), (2.4, 0.6, 9.0))
    for k in range(2):
        g.cyl(2, (0.0, chest["front"] + 2.4, d["NeckZ"] * H - 3.0 - k * 3.5), 0.45, 0.5, rot=(90, 0, 0), segs=8)
    g.save()
    g = Garment(b, "Chinos")
    g.cut(pants_region(d, W + 0.025, pants_bottom), tube(0.9, 1.1, 1.6), 0.35)
    g.ring(1, waist, 1.3, 3.4, 0.7, z=(W + 0.012) * H)
    g.box(2, (0.0, waist["front"] + 2.4, (W + 0.012) * H), (5.0, 0.8, 3.6))
    g.save()

    # 2: hoodie (hood, kangaroo pocket, drawstrings, ribbed hem and cuffs) + jeans.
    g = Garment(b, "Hoodie")
    rib = lambda p: 1 if (p[2] / H < W - 0.012 or abs(p[0]) > d["Long"] - 5.0) else 0  # noqa: E731
    g.cut(shirt_region(d, d["Long"], W - 0.03, d["NeckZ"] + 0.012), tube(2.6, 0, 0), 0.5, rib)
    nx, ny, nz = neck["c"]
    g.sphere(0, (nx, neck["back"] - 3.0, d["NeckZ"] * H + 0.5), 10.0, scale=(1.3, 0.8, 0.6))
    g.ring(0, neck, 2.6, 3.4, 1.4, z=d["NeckZ"] * H + 1.5)
    g.box(0, (0.0, belly["front"] + 3.6, (W + 0.035) * H), (24.0, 1.2, 12.0))
    for s in (-1, 1):
        g.cyl(1, (s * 3.2, chest["front"] + 3.4, d["NeckZ"] * H - 8.0), 0.35, 12.0, segs=6)
    g.save()
    g = Garment(b, "Jeans")
    g.cut(pants_region(d, W + 0.025, pants_bottom), tube(0.9, 1.0, 1.4), 0.4)
    g.save()

    # 3: t-shirt + work trousers (knee pads, side pockets, belt).
    g = Garment(b, "TShirt")
    g.cut(shirt_region(d, d["Short"] - 2.0, W + 0.028, d["NeckZ"] - 0.008), tube(1.3, 0, 0), 0.3)
    g.ring(1, neck, 1.5, 1.6, 0.7, z=(d["NeckZ"] - 0.008) * H)
    g.save()
    g = Garment(b, "WorkPants")
    g.cut(pants_region(d, W + 0.025, pants_bottom), tube(1.0, 1.5, 1.8), 0.45)
    g.ring(2, waist, 1.5, 3.6, 0.8, z=(W + 0.012) * H)
    g.box(1, (0.0, waist["front"] + 2.7, (W + 0.012) * H), (4.6, 0.9, 3.8))
    for s in (-1, 1):
        k = knees[s]
        knee_out = 1.5 + 1.8 * (crotch - 0.29 * H) / max(crotch - pants_bottom, 1.0) + 0.45
        g.box(2, (k["c"][0], k["front"] + knee_out + 0.7, 0.29 * H), (10.0, 1.4, 11.0))
        t = thighs[s]
        side_x = t["maxx"] if s > 0 else t["minx"]
        g.box(1, (side_x + s * 2.6, t["c"][1], 0.40 * H), (1.2, 12.0, 14.0))
    g.save()

    # 4: jacket (stand-up collar, zip, pockets, hem band) + trousers with a belt.
    g = Garment(b, "Jacket")
    hem = lambda p: 2 if p[2] / H < W - 0.045 else 0  # noqa: E731
    g.cut(shirt_region(d, d["Long"] + 1.0, W - 0.06, d["NeckZ"] + 0.01), tube(3.0, 0, 0), 0.6, hem)
    g.ring(0, neck, 3.2, 6.0, 1.4, z=d["NeckZ"] * H + 1.8)
    zip_strip(g, 1, (W - 0.06) * H, d["NeckZ"] * H, 3.7, 1.0)
    for s in (-1, 1):
        g.box(2, (s * 9.0, belly["front"] + 4.0, (W - 0.01) * H), (10.0, 1.0, 1.4))
        g.box(2, (s * 7.5, chest["front"] + 3.8, chest_z + 2.0), (8.0, 1.0, 1.2))
    g.save()
    g = Garment(b, "Trousers")
    g.cut(pants_region(d, W + 0.025, pants_bottom), tube(0.9, 1.1, 1.6), 0.35)
    g.ring(1, waist, 1.3, 3.4, 0.7, z=(W + 0.012) * H)
    g.box(2, (0.0, waist["front"] + 2.4, (W + 0.012) * H), (4.6, 0.8, 3.6))
    g.save()

    # Shoes: sneakers (white, light sole), work shoes (toe cap), boots (higher, laces). The lowest band is the sole.
    for name, height, off, thick, cap, laces in (("Sneakers", 11.0, 0.9, 0.9, False, True), ("WorkShoes", 13.0, 1.0, 1.1, True, False),
                                                 ("Boots", 26.0, 2.2, 1.0, False, True)):
        g = Garment(b, name)
        g.cut(shoe_region(height), off, thick, lambda p: 1 if p[2] < 2.6 else 0, drape=False)
        for s in (-1, 1):
            f = feet[s]
            fx = (f["minx"] + f["maxx"]) * 0.5
            if cap:
                g.sphere(2, (fx, f["front"] - 3.0, 3.4), 4.8, scale=(0.9, 0.85, 0.6))
            if laces:
                for k in range(3 if height < 15 else 5):
                    if height < 15:
                        g.box(2, (fx, f["front"] - 8.5 - k * 1.7, 5.6 + k * 1.2), (3.8, 0.5, 0.5), rot=(0, 0, 0))
                    else:
                        g.box(2, (fx, f["front"] - 9.0 - k * 0.9, 7.0 + k * 3.0), (3.8, 0.5, 0.5), rot=(0, 0, 0))
        g.save()

# --- Beard variants -------------------------------------------------------------------------------------------------
def beard_variants():
    asset = unreal.load_asset(f"{DEST}/Hair/Hair_Beard/Hair_Beard/SkeletalMeshes/Hair_Beard")
    if not asset:
        unreal.log_error("WE_CLOTH beard missing")
        return
    src = unreal.DynamicMesh()
    AU.copy_mesh_from_skeletal_mesh(asset, src, unreal.GeometryScriptCopyMeshFromAssetOptions(), unreal.GeometryScriptMeshReadLOD())
    verts = [(v.x, v.y, v.z) for v in L.convert_vector_list_to_array(pick(Q.get_all_vertex_positions(src, False), unreal.GeometryScriptVectorList))]
    tris = [(t.x, t.y, t.z) for t in L.convert_triangle_list_to_array(pick(Q.get_all_triangle_indices(src, False), unreal.GeometryScriptTriangleList))]
    xs, ys, zs = [v[0] for v in verts], [v[1] for v in verts], [v[2] for v in verts]
    cx, ex = (min(xs) + max(xs)) * 0.5, (max(xs) - min(xs)) * 0.5
    z0, hz = min(zs), max(zs) - min(zs)
    y0, hy = min(ys), max(ys) - min(ys)
    unreal.log(f"WE_CLOTH beard: {len(verts)} verts x {min(xs):.1f}..{max(xs):.1f} y {min(ys):.1f}..{max(ys):.1f} z {min(zs):.1f}..{max(zs):.1f}")
    # Histogram of the front-most part per height (helps placing the cuts).
    for k in range(10):
        band = [v for v in verts if z0 + hz * k / 10 <= v[2] < z0 + hz * (k + 1) / 10]
        if band:
            unreal.log(f"WE_CLOTH beard band {k}: n={len(band)} |x|max={max(abs(v[0] - cx) for v in band):.1f} y {min(v[1] for v in band):.1f}..{max(v[1] for v in band):.1f}")
    skeleton = asset.get_editor_property("skeleton")

    def make(name, keep):
        m = unreal.DynamicMesh()
        AU.copy_mesh_from_skeletal_mesh(asset, m, unreal.GeometryScriptCopyMeshFromAssetOptions(), unreal.GeometryScriptMeshReadLOD())
        drop = []
        for t, (a, b, c) in enumerate(tris):
            p = tuple((verts[a][i] + verts[b][i] + verts[c][i]) / 3.0 for i in range(3))
            u = (p[0] - cx) / max(ex, 0.01)
            v = (p[2] - z0) / max(hz, 0.01)
            w = (p[1] - y0) / max(hy, 0.01)
            if not keep(u, v, w):
                drop.append(t)
        E.delete_triangles_from_mesh(m, index_list(drop), False)
        folder = f"{DEST}/Hair/{name}/{name}/SkeletalMeshes"
        path = f"{folder}/{name}"
        if eal.does_asset_exist(path):
            eal.delete_asset(path)
        opts = unreal.GeometryScriptCreateNewSkeletalMeshAssetOptions()
        hair_mat = unreal.load_asset(f"{DEST}/M_WEHair")
        opts.set_editor_property("materials", {"Hair": hair_mat})
        sk = first(NEW.create_new_skeletal_mesh_asset_from_mesh(m, skeleton, path, opts))
        if sk:
            eal.save_asset(path)
        unreal.log(f"WE_CLOTH beard variant {name}: kept {len(tris) - len(drop)} of {len(tris)} tris -> {bool(sk)}")

    # Moustache: the band over the upper lip, at the front. Goatee: chin and moustache in the middle.
    make("Hair_Moustache", lambda u, v, w: 0.52 <= v <= 0.74 and abs(u) < 0.42 and w > 0.55)
    make("Hair_Goatee", lambda u, v, w: abs(u) < 0.36 and w > 0.45 and (v < 0.48 or 0.52 <= v <= 0.74))


# --- Fabric material --------------------------------------------------------------------------------------------------
CLOTH_HLSL = """
float h = frac(sin(dot(floor(P.xz * 2.1 + P.yy * 0.7), float2(12.9898, 78.233))) * 43758.5453);
float grain = 0.93 + 0.07 * h;
float twill = 0.86 + 0.14 * step(0.5, frac((P.x * 0.7 + P.z) * 1.8));
float rib = 0.88 + 0.12 * step(0.5, frac((P.x + P.y) * 1.3));
float m = grain;
m = lerp(m, twill * grain, saturate(Pattern));
m = lerp(m, rib * grain, saturate(Pattern - 1.0));
return Tint * m;
"""


def cloth_material():
    path = f"{DEST}/M_WECloth"
    if eal.does_asset_exist(path):
        eal.delete_asset(path)
    m = tools.create_asset("M_WECloth", DEST, unreal.Material, unreal.MaterialFactoryNew())
    pos = mel.create_material_expression(m, unreal.MaterialExpressionPreSkinnedPosition, -1200, -300)
    interp = mel.create_material_expression(m, unreal.MaterialExpressionVertexInterpolator, -1000, -300)
    mel.connect_material_expressions(pos, "", interp, "")
    custom = mel.create_material_expression(m, unreal.MaterialExpressionCustom, -600, -200)
    custom.set_editor_property("code", CLOTH_HLSL)
    custom.set_editor_property("output_type", unreal.CustomMaterialOutputType.CMOT_FLOAT3)
    inputs = []
    for n in ("P", "Tint", "Pattern"):
        ci = unreal.CustomInput()
        ci.set_editor_property("input_name", n)
        inputs.append(ci)
    custom.set_editor_property("inputs", inputs)
    mel.connect_material_expressions(interp, "", custom, "P")
    tint = mel.create_material_expression(m, unreal.MaterialExpressionVectorParameter, -900, -100)
    tint.set_editor_property("parameter_name", "Tint")
    tint.set_editor_property("default_value", unreal.LinearColor(0.3, 0.3, 0.3, 1.0))
    mel.connect_material_expressions(tint, "RGB", custom, "Tint")
    pattern = mel.create_material_expression(m, unreal.MaterialExpressionScalarParameter, -900, 50)
    pattern.set_editor_property("parameter_name", "Pattern")
    pattern.set_editor_property("default_value", 0.0)
    mel.connect_material_expressions(pattern, "", custom, "Pattern")
    mel.connect_material_property(custom, "", unreal.MaterialProperty.MP_BASE_COLOR)
    rough = mel.create_material_expression(m, unreal.MaterialExpressionScalarParameter, -400, 100)
    rough.set_editor_property("parameter_name", "Roughness")
    rough.set_editor_property("default_value", 0.86)
    mel.connect_material_property(rough, "", unreal.MaterialProperty.MP_ROUGHNESS)
    spec = mel.create_material_expression(m, unreal.MaterialExpressionScalarParameter, -400, 200)
    spec.set_editor_property("parameter_name", "Specular")
    spec.set_editor_property("default_value", 0.3)
    mel.connect_material_property(spec, "", unreal.MaterialProperty.MP_SPECULAR)
    metal = mel.create_material_expression(m, unreal.MaterialExpressionScalarParameter, -400, 300)
    metal.set_editor_property("parameter_name", "Metallic")
    metal.set_editor_property("default_value", 0.0)
    mel.connect_material_property(metal, "", unreal.MaterialProperty.MP_METALLIC)
    m.set_editor_property("used_with_skeletal_mesh", True)
    mel.recompile_material(m)
    eal.save_asset(path)
    unreal.log("WE_CLOTH material M_WECloth")


def main():
    cloth_material()
    for gender in ("Male", "Female"):
        wardrobe(Body(gender))
    beard_variants()
    unreal.SystemLibrary.quit_editor()


main()
