"""
Phase 18c: player characters from Quaternius "Universal Base Characters" (CC0, standard version; the ZIP is
extracted to SourceArt/Quaternius/UBC - not in Git, see Docs/ASSET_SOURCES.md).

  /Game/Characters/Quaternius/Male|Female/...   skeletal meshes of the base bodies (glTF via Interchange)
  /Game/Characters/Quaternius/Hair/<Name>/...    hair, beard and eyebrows rigged to the head bone
  /Game/Characters/Quaternius/Skin/T_*_Light     the light skin textures (the glTFs only reference the dark ones)
  /Game/Characters/Quaternius/M_WEAvatar         body material: skin tone + clothes painted by body region
  /Game/Characters/Quaternius/M_WEHair           hair material: any hair colour from the texture's shading
  /Game/Characters/Quaternius/Accessories/SM_Gen_<Cap|Beanie|Sunglasses>_<Male|Female>  in body space

The bodies are animated at runtime from the UE mannequin's animation (UWEAvatarComponent copies the bone
motion), so no animation assets are needed here.

Run once after the project compiles (skips imports that exist; materials and accessories are rebuilt):
  UnrealEditor-Cmd.exe <uproject> -ExecutePythonScript="<this file>" -unattended -RenderOffscreen -nosplash -nosteam
"""
import builtins
import os
import sys

import unreal

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
builtins.WE_MODELS_LIBRARY = True
import build_models as bm  # noqa: E402

ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), "..", ".."))
SRC = os.path.join(ROOT, "SourceArt", "Quaternius", "UBC")
DEST = "/Game/Characters/Quaternius"
eal = unreal.EditorAssetLibrary
mel = unreal.MaterialEditingLibrary
tools = unreal.AssetToolsHelpers.get_asset_tools()

BODIES = {"Male": "Superhero_Male_FullBody", "Female": "Superhero_Female_FullBody"}
HAIR = ["Hair_Long", "Hair_Buns", "Hair_Buzzed", "Hair_BuzzedFemale", "Hair_SimpleParted", "Hair_Beard", "Eyebrows_Female", "Eyebrows_Regular"]
LIGHT_SKIN = {"Male": "T_Superhero_Male_Ligh.png", "Female": "T_Superhero_Female_Light_BaseColor.png"}


def import_file(path, dest, name=None):
    task = unreal.AssetImportTask()
    task.set_editor_property("filename", path)
    task.set_editor_property("destination_path", dest)
    if name:
        task.set_editor_property("destination_name", name)
    task.set_editor_property("automated", True)
    task.set_editor_property("save", True)
    task.set_editor_property("replace_existing", True)
    tools.import_asset_tasks([task])


def find(dest, cls):
    for p in eal.list_assets(dest, True):
        a = eal.load_asset(p)
        if isinstance(a, cls):
            return a
    return None


def imports():
    meshes = {}
    for gender, name in BODIES.items():
        dest = f"{DEST}/{gender}"
        mesh = find(dest, unreal.SkeletalMesh)
        if not mesh:
            import_file(os.path.join(SRC, "Base Characters", "Godot - UE", name + ".gltf"), dest)
            mesh = find(dest, unreal.SkeletalMesh)
        meshes[gender] = mesh
        b = mesh.get_bounds() if mesh else None
        unreal.log(f"WE_CHAR {gender}: {mesh.get_path_name() if mesh else None} origin=({b.origin.x:.1f},{b.origin.y:.1f},{b.origin.z:.1f}) "
                   f"extent=({b.box_extent.x:.1f},{b.box_extent.y:.1f},{b.box_extent.z:.1f})" if b else "")
        tex = f"{DEST}/Skin/T_{gender}_Light"
        if not eal.does_asset_exist(tex):
            import_file(os.path.join(SRC, "Base Characters", "Textures", LIGHT_SKIN[gender]), f"{DEST}/Skin", f"T_{gender}_Light")
    hair = {}
    hair_dir = os.path.join(SRC, "Hairstyles", "Rigged to Head Bone", "glTF (Godot -Unreal)")
    for h in HAIR:
        dest = f"{DEST}/Hair/{h}"
        mesh = find(dest, unreal.SkeletalMesh)
        if not mesh:
            import_file(os.path.join(hair_dir, h + ".gltf"), dest)
            mesh = find(dest, unreal.SkeletalMesh)
        hair[h] = mesh
        if mesh:
            b = mesh.get_bounds()
            unreal.log(f"WE_CHAR hair {h}: {mesh.get_path_name()} origin=({b.origin.x:.1f},{b.origin.y:.1f},{b.origin.z:.1f}) extent=({b.box_extent.x:.1f},{b.box_extent.y:.1f},{b.box_extent.z:.1f})")
    return meshes, hair


# --- Materials --------------------------------------------------------------------------------------------
def new_material(name):
    path = f"{DEST}/{name}"
    if eal.does_asset_exist(path):
        eal.delete_asset(path)
    return tools.create_asset(name, DEST, unreal.Material, unreal.MaterialFactoryNew()), path


def scalar(m, name, value, x, y):
    s = mel.create_material_expression(m, unreal.MaterialExpressionScalarParameter, x, y)
    s.set_editor_property("parameter_name", name)
    s.set_editor_property("default_value", value)
    return s


def vector(m, name, value, x, y):
    v = mel.create_material_expression(m, unreal.MaterialExpressionVectorParameter, x, y)
    v.set_editor_property("parameter_name", name)
    v.set_editor_property("default_value", unreal.LinearColor(*value, 1.0))
    return v


def sampler_for(tex):
    """The sampler type has to match the texture (a mismatch fails the whole material)."""
    c = tex.get_editor_property("compression_settings")
    if c == unreal.TextureCompressionSettings.TC_NORMALMAP:
        return unreal.MaterialSamplerType.SAMPLERTYPE_NORMAL
    if c == unreal.TextureCompressionSettings.TC_MASKS:
        return unreal.MaterialSamplerType.SAMPLERTYPE_MASKS
    if c == unreal.TextureCompressionSettings.TC_GRAYSCALE:
        return unreal.MaterialSamplerType.SAMPLERTYPE_GRAYSCALE if tex.get_editor_property("srgb") else unreal.MaterialSamplerType.SAMPLERTYPE_LINEAR_GRAYSCALE
    return unreal.MaterialSamplerType.SAMPLERTYPE_COLOR if tex.get_editor_property("srgb") else unreal.MaterialSamplerType.SAMPLERTYPE_LINEAR_COLOR


def texture(m, name, path, x, y, sampler=None):
    tex = unreal.load_asset(path)
    t = mel.create_material_expression(m, unreal.MaterialExpressionTextureSampleParameter2D, x, y)
    t.set_editor_property("parameter_name", name)
    t.set_editor_property("texture", tex)
    t.set_editor_property("sampler_type", sampler_for(tex))
    unreal.log(f"WE_CHAR texture {name}: {path} -> {sampler_for(tex)}")
    return t


AVATAR_HLSL = """
float zN = P.z / max(Height, 1.0);
float ax = abs(P.x);
float front = (P.y * FrontSign) > 0.0 ? 1.0 : 0.0;
float shoe = zN < ShoeZ ? 1.0 : 0.0;
float pants = (zN < WaistZ ? 1.0 : 0.0) * (1.0 - shoe) * PantsOn;
float shoulderTop = NeckZ + 0.05;
float torso = (zN >= WaistZ && ((ax < 9.0 && zN < NeckZ) || (ax >= 9.0 && ax < TorsoX && zN < shoulderTop))) ? 1.0 : 0.0;
float sleeve = (ax >= TorsoX && ax < SleeveX && zN > ArmZ && zN < shoulderTop + 0.03) ? 1.0 : 0.0;
float shirt = saturate(torso + sleeve) * ShirtOn;
float knee = KneePads * front * (abs(zN - 0.29) < 0.03 ? 1.0 : 0.0);
float pocket = PocketBand * front * ((zN > 0.585 && zN < 0.635 && ax < 13.0) ? 1.0 : 0.0);
float grain = 0.93 + 0.07 * frac(sin(dot(floor(P.xz * 1.7), float2(12.9898, 78.233))) * 43758.5453);
float3 c = Skin;
// Phase 19 faces: stubble on the jaw, freckles on the cheeks and nose, lipstick and blush (EyeZ = eye height).
float fx = abs(P.x);
float dz = P.z - EyeZ;
float hashS = frac(sin(dot(floor(float2(P.x, P.z) * 9.0), float2(12.9898, 78.233))) * 43758.5453);
float lipD = abs(dz + LipOffset);
float mouth = (lipD < 1.1 && fx < 2.9) ? 1.0 : 0.0;
float jaw = smoothstep(-13.9, -12.9, dz) * (1.0 - smoothstep(-5.6, -4.2, dz)) * smoothstep(2.0, 4.5, P.y) * (1.0 - smoothstep(6.6, 8.0, fx));
float nose = (fx < 2.0 && dz > -5.8) ? 1.0 : 0.0;
c = lerp(c, StubbleColor, Stubble * jaw * (1.0 - nose) * (1.0 - mouth) * (hashS > 0.4 ? 0.55 : 0.3));
float hashF = frac(sin(dot(floor(float2(P.x, P.z) * 5.5), float2(39.346, 11.135))) * 24634.6345);
float cheeks = smoothstep(-5.8, -4.8, dz) * (1.0 - smoothstep(-2.6, -1.6, dz)) * (1.0 - smoothstep(4.6, 5.8, fx)) * smoothstep(4.5, 6.5, P.y);
c = lerp(c, c * float3(0.68, 0.5, 0.38), Freckles * cheeks * (hashF > 0.87 ? 0.75 : 0.0));
float lips = (lipD < 0.95 && fx < 2.6 - lipD * 1.1 && P.y > 5.5) ? 1.0 : 0.0;
c = lerp(c, LipColor, Makeup * lips * 0.8);
float blush = (fx > 2.8 && fx < 6.4 && dz > -5.2 && dz < -2.0 && P.y > 3.5) ? 1.0 : 0.0;
c = lerp(c, c * float3(1.12, 0.8, 0.8), Makeup * blush * 0.55);
c = lerp(c, Pants * (1.0 - 0.35 * knee) * grain, pants);
c = lerp(c, Shirt * (1.0 - 0.25 * pocket) * grain, shirt);
float stripe = Stripes * saturate((abs(zN - 0.30) < 0.012 ? 1.0 : 0.0) + (abs(zN - 0.665) < 0.012 ? 1.0 : 0.0)) * saturate(pants + shirt);
c = lerp(c, Accent, stripe);
c = lerp(c, Accent, Zipper * front * (ax < 0.7 ? 1.0 : 0.0) * shirt * (zN < NeckZ - 0.01 ? 1.0 : 0.0));
c = lerp(c, Shoe, shoe);
return float4(c, saturate(pants + shirt + shoe));
"""


def avatar_material(meshes):
    m, path = new_material("M_WEAvatar")
    pos_class = unreal.MaterialExpressionPreSkinnedPosition
    pos = mel.create_material_expression(m, pos_class, -1400, -600)
    light = texture(m, "SkinLight", f"{DEST}/Skin/T_Male_Light", -1400, -400)
    dark_path = [p for p in eal.list_assets(f"{DEST}/Male", True) if "Dark" in p][0].split(".")[0]
    dark = texture(m, "SkinDark", dark_path, -1400, -150)
    blend = mel.create_material_expression(m, unreal.MaterialExpressionLinearInterpolate, -1100, -300)
    mel.connect_material_expressions(light, "RGB", blend, "A")
    mel.connect_material_expressions(dark, "RGB", blend, "B")
    mel.connect_material_expressions(scalar(m, "SkinBlend", 0.0, -1300, 50), "", blend, "Alpha")
    tinted = mel.create_material_expression(m, unreal.MaterialExpressionMultiply, -950, -300)
    mel.connect_material_expressions(blend, "", tinted, "A")
    mel.connect_material_expressions(vector(m, "SkinTint", (1.0, 1.0, 1.0), -1150, 100), "RGB", tinted, "B")
    custom = mel.create_material_expression(m, unreal.MaterialExpressionCustom, -600, -200)
    custom.set_editor_property("code", AVATAR_HLSL)
    custom.set_editor_property("output_type", unreal.CustomMaterialOutputType.CMOT_FLOAT4)
    names = ["P", "Skin", "Height", "FrontSign", "ShoeZ", "WaistZ", "NeckZ", "ArmZ", "TorsoX", "SleeveX", "PantsOn", "ShirtOn",
             "KneePads", "PocketBand", "Stripes", "Zipper", "EyeZ", "Stubble", "Freckles", "Makeup", "LipOffset", "Pants", "Shirt", "Shoe", "Accent", "StubbleColor", "LipColor"]
    inputs = []
    for n in names:
        ci = unreal.CustomInput()
        ci.set_editor_property("input_name", n)
        inputs.append(ci)
    custom.set_editor_property("inputs", inputs)
    defaults = {"Height": 182.0, "FrontSign": 1.0, "ShoeZ": 0.05, "WaistZ": 0.56, "NeckZ": 0.82, "ArmZ": 0.70, "TorsoX": 20.0, "SleeveX": 34.0,
                "PantsOn": 1.0, "ShirtOn": 1.0, "KneePads": 0.0, "PocketBand": 0.0, "Stripes": 0.0, "Zipper": 0.0,
                "EyeZ": 168.0, "Stubble": 0.0, "Freckles": 0.0, "Makeup": 0.0, "LipOffset": 7.0}
    colors = {"Pants": (0.12, 0.16, 0.28), "Shirt": (0.5, 0.5, 0.5), "Shoe": (0.05, 0.05, 0.05), "Accent": (0.8, 0.8, 0.7),
              "StubbleColor": (0.05, 0.035, 0.025), "LipColor": (0.45, 0.06, 0.08)}
    # Pre-skinned positions exist only in the vertex shader: carried over with a vertex interpolator.
    interp = mel.create_material_expression(m, unreal.MaterialExpressionVertexInterpolator, -1200, -600)
    mel.connect_material_expressions(pos, "", interp, "")
    mel.connect_material_expressions(interp, "", custom, "P")
    mel.connect_material_expressions(tinted, "", custom, "Skin")
    y = 0
    for n in names[2:]:
        node = scalar(m, n, defaults[n], -900, 200 + y) if n in defaults else vector(m, n, colors[n], -900, 200 + y)
        mel.connect_material_expressions(node, "" if n in defaults else "RGB", custom, n)
        y += 60
    mask = mel.create_material_expression(m, unreal.MaterialExpressionComponentMask, -350, -100)
    mask.set_editor_property("r", True)
    mask.set_editor_property("g", True)
    mask.set_editor_property("b", True)
    mask.set_editor_property("a", False)
    mel.connect_material_expressions(custom, "", mask, "")
    mel.connect_material_property(mask, "", unreal.MaterialProperty.MP_BASE_COLOR)
    cloth = mel.create_material_expression(m, unreal.MaterialExpressionComponentMask, -350, 50)
    cloth.set_editor_property("a", True)
    for ch in ("r", "g", "b"):
        cloth.set_editor_property(ch, False)
    mel.connect_material_expressions(custom, "", cloth, "")
    rough_path = [p for p in eal.list_assets(f"{DEST}/Male", True) if "Roughness" in p][0].split(".")[0]
    rough_tex = texture(m, "RoughTex", rough_path, -700, 300)
    rough = mel.create_material_expression(m, unreal.MaterialExpressionLinearInterpolate, -200, 150)
    mel.connect_material_expressions(rough_tex, "R", rough, "A")
    rough.set_editor_property("const_b", 0.88)
    mel.connect_material_expressions(cloth, "", rough, "Alpha")
    mel.connect_material_property(rough, "", unreal.MaterialProperty.MP_ROUGHNESS)
    normal_path = [p for p in eal.list_assets(f"{DEST}/Male", True) if "Normal" in p][0].split(".")[0]
    normal = texture(m, "NormalTex", normal_path, -700, 550)
    nlerp = mel.create_material_expression(m, unreal.MaterialExpressionLinearInterpolate, -200, 450)
    mel.connect_material_expressions(normal, "RGB", nlerp, "A")
    flat = mel.create_material_expression(m, unreal.MaterialExpressionConstant3Vector, -400, 650)
    flat.set_editor_property("constant", unreal.LinearColor(0.0, 0.0, 1.0, 1.0))
    mel.connect_material_expressions(flat, "", nlerp, "B")
    mel.connect_material_expressions(cloth, "", nlerp, "Alpha")
    mel.connect_material_property(nlerp, "", unreal.MaterialProperty.MP_NORMAL)
    m.set_editor_property("used_with_skeletal_mesh", True)
    mel.recompile_material(m)
    eal.save_asset(path)
    unreal.log("WE_CHAR material M_WEAvatar")


def hair_material(hair):
    m, path = new_material("M_WEHair")
    m.set_editor_property("blend_mode", unreal.BlendMode.BLEND_MASKED)
    m.set_editor_property("two_sided", True)
    m.set_editor_property("used_with_skeletal_mesh", True)
    tex_path = [p for p in eal.list_assets(f"{DEST}/Hair/Hair_SimpleParted", True) if "BaseColor" in p][0].split(".")[0]
    tex = texture(m, "HairTex", tex_path, -900, -100)
    gray = mel.create_material_expression(m, unreal.MaterialExpressionDesaturation, -650, -150)
    mel.connect_material_expressions(tex, "RGB", gray, "")
    # Light and dark strands of the texture keep their contrast; the colour comes from HairColor.
    boost = mel.create_material_expression(m, unreal.MaterialExpressionMultiply, -450, -150)
    mel.connect_material_expressions(gray, "", boost, "A")
    boost.set_editor_property("const_b", 2.2)
    col = mel.create_material_expression(m, unreal.MaterialExpressionMultiply, -250, -150)
    mel.connect_material_expressions(boost, "", col, "A")
    mel.connect_material_expressions(vector(m, "HairColor", (0.2, 0.12, 0.06), -450, 0), "RGB", col, "B")
    mel.connect_material_property(col, "", unreal.MaterialProperty.MP_BASE_COLOR)
    # Hide = 1 makes a hair part invisible (the brows built into the body are replaced by separate brows).
    keep = mel.create_material_expression(m, unreal.MaterialExpressionOneMinus, -450, 200)
    mel.connect_material_expressions(scalar(m, "Hide", 0.0, -650, 200), "", keep, "")
    opacity = mel.create_material_expression(m, unreal.MaterialExpressionMultiply, -250, 200)
    mel.connect_material_expressions(tex, "A", opacity, "A")
    mel.connect_material_expressions(keep, "", opacity, "B")
    mel.connect_material_property(opacity, "", unreal.MaterialProperty.MP_OPACITY_MASK)
    mel.connect_material_property(scalar(m, "Roughness", 0.55, -250, 100), "", unreal.MaterialProperty.MP_ROUGHNESS)
    mel.recompile_material(m)
    eal.save_asset(path)
    unreal.log("WE_CHAR material M_WEHair")


# --- Accessories (in body space, fitted to the head of each body) ----------------------------------------------
def head_box(hair, gender):
    mesh = hair.get("Hair_Buzzed" if gender == "Male" else "Hair_BuzzedFemale")
    b = mesh.get_bounds()
    o, e = b.origin, b.box_extent
    return (o.x, o.y, o.z), (e.x, e.y, e.z)


def accessories(hair):
    folder = f"{DEST}/Accessories"
    for gender in ("Male", "Female"):
        (cx, cy, cz), (ex, ey, ez) = head_box(hair, gender)
        top = cz + ez
        r = max(ex, ey) * 1.06
        face = 1.0  # the bodies look along +Y (eyebrows and beard sit at +Y; FrontSign of M_WEAvatar)
        # Baseball cap: crown (half sphere), band, a brim to the front.
        m = bm.Model(f"Cap_{gender}")
        rc = r * 0.97
        m.sphere("PlasticRed", (cx, cy, top - rc * 0.68), rc, scale=(1.0, 1.05, 0.72), steps=20)
        m.cyl("PlasticRed", (cx, cy, top - rc * 0.70), rc, 2.6, segs=28)
        m.box("PlasticRed", (cx, cy + face * (rc + 5.5), top - rc * 0.74), (rc * 1.45, 12.0, 0.8), rot=(0, -8.0 * face, 0), bevel=0.3)
        m.cyl("PlasticRed", (cx, cy, top + 0.4), 0.9, 1.2, segs=8)
        m.save(normalize=False, folder=folder)
        # Beanie: rounded knit hat with a folded cuff.
        m = bm.Model(f"Beanie_{gender}")
        m.sphere("PlasticGrey", (cx, cy, top - r * 0.55), r * 1.03, scale=(1.0, 1.04, 0.9), steps=20)
        m.torus("PlasticGrey", (cx, cy, top - r * 0.88), r * 1.02, 2.2, segs=32, minor=8)
        m.save(normalize=False, folder=folder)
        # Sunglasses at eye height, in front of the face.
        m = bm.Model(f"Sunglasses_{gender}")
        brows = hair.get("Eyebrows_Regular" if gender == "Male" else "Eyebrows_Female").get_bounds().origin
        eye_z = brows.z - 2.4
        fy = cy + face * (ey + 1.2)
        for side in (-1.0, 1.0):
            m.box("ScreenDark", (cx + side * 3.4, fy, eye_z), (5.4, 0.4, 3.2), bevel=0.4)
            m.box("TrimBlack", (cx + side * 3.4, fy - face * 0.1, eye_z + 1.8), (5.8, 0.6, 0.5))
            m.box("TrimBlack", (cx + side * (ex + 0.4), cy + face * ey * 0.25, eye_z + 1.0), (0.4, ey * 1.6, 0.5))
        m.box("TrimBlack", (cx, fy, eye_z + 1.2), (1.6, 0.6, 0.5))
        m.save(normalize=False, folder=folder)
        unreal.log(f"WE_CHAR accessories {gender}: head centre=({cx:.1f},{cy:.1f},{cz:.1f}) top={top:.1f} r={r:.1f}")


def main():
    meshes, hair = imports()
    avatar_material(meshes)
    hair_material(hair)
    accessories(hair)
    unreal.SystemLibrary.quit_editor()


main()
