"""
Master material for buildings, furniture and machines (Phase 15):
  /Game/WorkshopEmpire/Materials/M_WESurface

Textures are projected in world space (WorldAlignedTexture / WorldAlignedNormal), so the scaled
box parts of walls, floors and machines never look stretched. Parameters:
  BaseColorTex, NormalTex, ARMTex (R = ambient occlusion, G = roughness, B = metallic; Poly Haven "arm")
  Tint (multiplied), TileSize (cm per texture repeat), RoughnessScale, MetallicScale,
  Grime (0-1: darker dirt towards the floor on walls and machines), Desaturation + Brightness (repaint a photographed colour)
Defaults are neutral engine textures, so the material works before textures are assigned.

Run after build_placeholder_material.py:
  UnrealEditor-Cmd.exe <WorkshopEmpire.uproject> -ExecutePythonScript="<this file>" -unattended -RenderOffscreen -nosplash
"""
import unreal

FOLDER = "/Game/WorkshopEmpire/Materials"
PATH = FOLDER + "/M_WESurface"
tools = unreal.AssetToolsHelpers.get_asset_tools()
mel = unreal.MaterialEditingLibrary
eal = unreal.EditorAssetLibrary

# Linear white default for the ARM (masks) parameter: an sRGB default would make the material fail to compile.
WHITE_LINEAR = FOLDER + "/T_WEWhiteLinear"
if not eal.does_asset_exist(WHITE_LINEAR):
    import os, struct, zlib
    png_path = os.path.join(unreal.Paths.project_saved_dir(), "T_WEWhiteLinear.png")

    def chunk(kind, data):
        return struct.pack(">I", len(data)) + kind + data + struct.pack(">I", zlib.crc32(kind + data) & 0xffffffff)
    raw = b"".join(b"\x00" + b"\xff\xff\xff" * 4 for _ in range(4))
    with open(png_path, "wb") as fh:
        fh.write(b"\x89PNG\r\n\x1a\n" + chunk(b"IHDR", struct.pack(">IIBBBBB", 4, 4, 8, 2, 0, 0, 0)) + chunk(b"IDAT", zlib.compress(raw)) + chunk(b"IEND", b""))
    task = unreal.AssetImportTask()
    task.set_editor_property("filename", png_path)
    task.set_editor_property("destination_path", FOLDER)
    task.set_editor_property("destination_name", "T_WEWhiteLinear")
    task.set_editor_property("automated", True)
    task.set_editor_property("save", True)
    tools.import_asset_tasks([task])
    white = eal.load_asset(WHITE_LINEAR)
    white.set_editor_property("srgb", False)
    white.set_editor_property("compression_settings", unreal.TextureCompressionSettings.TC_MASKS)
    eal.save_asset(WHITE_LINEAR)

if eal.does_asset_exist(PATH):
    eal.delete_asset(PATH)
mat = tools.create_asset("M_WESurface", FOLDER, unreal.Material, unreal.MaterialFactoryNew())
mat.set_editor_property("used_with_instanced_static_meshes", True)
mat.set_editor_property("used_with_static_lighting", True)
# WorldAlignedNormal returns a world-space normal.
mat.set_editor_property("tangent_space_normal", False)

WAT = unreal.load_asset("/Engine/Functions/Engine_MaterialFunctions01/Texturing/WorldAlignedTexture")
WAN = unreal.load_asset("/Engine/Functions/Engine_MaterialFunctions01/Texturing/WorldAlignedNormal")


def tex_param(name, default, x, y, sampler=None):
    t = mel.create_material_expression(mat, unreal.MaterialExpressionTextureObjectParameter, x, y)
    t.set_editor_property("parameter_name", name)
    t.set_editor_property("texture", unreal.load_asset(default))
    if sampler is not None:
        t.set_editor_property("sampler_type", sampler)
    return t


def scalar(name, value, x, y):
    s = mel.create_material_expression(mat, unreal.MaterialExpressionScalarParameter, x, y)
    s.set_editor_property("parameter_name", name)
    s.set_editor_property("default_value", value)
    return s


def call(func, x, y):
    c = mel.create_material_expression(mat, unreal.MaterialExpressionMaterialFunctionCall, x, y)
    c.set_material_function(func)
    return c


def mul(a, b, x, y, a_out="", b_out=""):
    m = mel.create_material_expression(mat, unreal.MaterialExpressionMultiply, x, y)
    mel.connect_material_expressions(a, a_out, m, "A")
    mel.connect_material_expressions(b, b_out, m, "B")
    return m


tile = scalar("TileSize", 200.0, -1600, 0)
tile3 = mel.create_material_expression(mat, unreal.MaterialExpressionAppendVector, -1400, 0)
tile2 = mel.create_material_expression(mat, unreal.MaterialExpressionAppendVector, -1500, 0)
mel.connect_material_expressions(tile, "", tile2, "A")
mel.connect_material_expressions(tile, "", tile2, "B")
mel.connect_material_expressions(tile2, "", tile3, "A")
mel.connect_material_expressions(tile, "", tile3, "B")

# Base colour * tint * grime.
base_tex = tex_param("BaseColorTex", "/Engine/EngineResources/WhiteSquareTexture", -1600, -400)
base = call(WAT, -1200, -400)
unreal.log("WE_WAT_INPUTS " + str(mel.get_material_expression_input_names(base)))
mel.connect_material_expressions(base_tex, "", base, "TextureObject")
mel.connect_material_expressions(tile3, "", base, "TextureSize")
tint = mat_tint = mel.create_material_expression(mat, unreal.MaterialExpressionVectorParameter, -1000, -250)
tint.set_editor_property("parameter_name", "Tint")
tint.set_editor_property("default_value", unreal.LinearColor(1, 1, 1, 1))
# Optional: take the colour out of a photographed paint (e.g. blue plate) and repaint it with Tint.
desat = mel.create_material_expression(mat, unreal.MaterialExpressionDesaturation, -1000, -450)
mel.connect_material_expressions(base, "XYZ Texture", desat, "")
mel.connect_material_expressions(scalar("Desaturation", 0.0, -1200, -560), "", desat, "Fraction")
bright = mul(desat, scalar("Brightness", 1.0, -1000, -560), -880, -450)
colored = mul(bright, tint, -800, -400, "", "RGB")

# Grime: darker towards the floor (first 80 cm), scaled by "Grime".
wp = mel.create_material_expression(mat, unreal.MaterialExpressionWorldPosition, -1400, -700)
wz = mel.create_material_expression(mat, unreal.MaterialExpressionComponentMask, -1250, -700)
wz.set_editor_property("r", False)
wz.set_editor_property("g", False)
wz.set_editor_property("b", True)
mel.connect_material_expressions(wp, "", wz, "")
h = mel.create_material_expression(mat, unreal.MaterialExpressionDivide, -1100, -700)
mel.connect_material_expressions(wz, "", h, "A")
h.set_editor_property("const_b", 80.0)
hs = mel.create_material_expression(mat, unreal.MaterialExpressionSaturate, -950, -700)
mel.connect_material_expressions(h, "", hs, "")
low = mel.create_material_expression(mat, unreal.MaterialExpressionOneMinus, -820, -700)
mel.connect_material_expressions(hs, "", low, "")
grime = scalar("Grime", 0.0, -820, -560)
gl = mul(low, grime, -680, -650)
darken = mel.create_material_expression(mat, unreal.MaterialExpressionLinearInterpolate, -550, -500)
darken.set_editor_property("const_a", 1.0)
darken.set_editor_property("const_b", 0.55)
mel.connect_material_expressions(gl, "", darken, "Alpha")
final_color = mul(colored, darken, -400, -400)
mel.connect_material_property(final_color, "", unreal.MaterialProperty.MP_BASE_COLOR)

# ARM: AO, roughness, metallic.
arm_tex = tex_param("ARMTex", WHITE_LINEAR, -1600, 250, unreal.MaterialSamplerType.SAMPLERTYPE_MASKS)
arm = call(WAT, -1200, 250)
mel.connect_material_expressions(arm_tex, "", arm, "TextureObject")
mel.connect_material_expressions(tile3, "", arm, "TextureSize")
mask_r = mel.create_material_expression(mat, unreal.MaterialExpressionComponentMask, -900, 150)
mask_r.set_editor_property("r", True); mask_r.set_editor_property("g", False); mask_r.set_editor_property("b", False)
mask_g = mel.create_material_expression(mat, unreal.MaterialExpressionComponentMask, -900, 280)
mask_g.set_editor_property("r", False); mask_g.set_editor_property("g", True); mask_g.set_editor_property("b", False)
mask_b = mel.create_material_expression(mat, unreal.MaterialExpressionComponentMask, -900, 410)
mask_b.set_editor_property("r", False); mask_b.set_editor_property("g", False); mask_b.set_editor_property("b", True)
for m in (mask_r, mask_g, mask_b):
    mel.connect_material_expressions(arm, "XYZ Texture", m, "")
mel.connect_material_property(mask_r, "", unreal.MaterialProperty.MP_AMBIENT_OCCLUSION)
rough = mul(mask_g, scalar("RoughnessScale", 0.75, -900, 550), -650, 300)
mel.connect_material_property(rough, "", unreal.MaterialProperty.MP_ROUGHNESS)
metal = mul(mask_b, scalar("MetallicScale", 0.0, -900, 650), -650, 450)
mel.connect_material_property(metal, "", unreal.MaterialProperty.MP_METALLIC)

# Normal (world space).
nor_tex = tex_param("NormalTex", "/Engine/EngineMaterials/FlatNormal", -1600, 800, unreal.MaterialSamplerType.SAMPLERTYPE_NORMAL)
nor = call(WAN, -1200, 800)
unreal.log("WE_WAN_INPUTS " + str(mel.get_material_expression_input_names(nor)))
mel.connect_material_expressions(nor_tex, "", nor, "TextureObject")
mel.connect_material_expressions(tile3, "", nor, "TextureSize")
# The projected normal points up on faces that look down (ceilings): flip it back towards the
# surface normal wherever it faces away from it (n' = n - 2 * min(dot(n, v), 0) * v).
vn = mel.create_material_expression(mat, unreal.MaterialExpressionVertexNormalWS, -1000, 950)
dot = mel.create_material_expression(mat, unreal.MaterialExpressionDotProduct, -900, 850)
mel.connect_material_expressions(nor, "XYZ Texture", dot, "A")
mel.connect_material_expressions(vn, "", dot, "B")
neg = mel.create_material_expression(mat, unreal.MaterialExpressionMin, -800, 850)
mel.connect_material_expressions(dot, "", neg, "A")
neg.set_editor_property("const_b", 0.0)
twice = mel.create_material_expression(mat, unreal.MaterialExpressionMultiply, -700, 850)
mel.connect_material_expressions(neg, "", twice, "A")
twice.set_editor_property("const_b", -2.0)
along = mul(twice, vn, -600, 900)
fixed = mel.create_material_expression(mat, unreal.MaterialExpressionAdd, -500, 800)
mel.connect_material_expressions(nor, "XYZ Texture", fixed, "A")
mel.connect_material_expressions(along, "", fixed, "B")
mel.connect_material_property(fixed, "", unreal.MaterialProperty.MP_NORMAL)

mel.recompile_material(mat)
eal.save_asset(PATH)
unreal.log("WE_SURFACE_MATERIAL_BUILT")
unreal.SystemLibrary.quit_editor()
