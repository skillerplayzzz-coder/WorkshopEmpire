"""
Materials for the particle effects (Phase 15, AWEFx), all for instanced static meshes with one
per-instance custom float (0 = fade, 0..1):
  /Game/WorkshopEmpire/Materials/FX/M_WEFxGlow   additive, unlit: sparks, embers, dust motes in light  (Color, Intensity)
  /Game/WorkshopEmpire/Materials/FX/M_WEFxSmoke  translucent, lit, soft edges: smoke, dust, steam    (Color, Opacity)
  /Game/WorkshopEmpire/Materials/FX/M_WEEmissive  opaque, unlit glow for lamp tubes and furnace mouths (Color, Intensity)

Run after build_placeholder_material.py:
  UnrealEditor-Cmd.exe <WorkshopEmpire.uproject> -ExecutePythonScript="<this file>" -unattended -RenderOffscreen -nosplash
"""
import unreal

FOLDER = "/Game/WorkshopEmpire/Materials/FX"
tools = unreal.AssetToolsHelpers.get_asset_tools()
mel = unreal.MaterialEditingLibrary
eal = unreal.EditorAssetLibrary


def new_material(name):
    path = f"{FOLDER}/{name}"
    if eal.does_asset_exist(path):
        eal.delete_asset(path)
    mat = tools.create_asset(name, FOLDER, unreal.Material, unreal.MaterialFactoryNew())
    mat.set_editor_property("used_with_instanced_static_meshes", True)
    return mat, path


def param_vector(mat, name, value, x, y):
    p = mel.create_material_expression(mat, unreal.MaterialExpressionVectorParameter, x, y)
    p.set_editor_property("parameter_name", name)
    p.set_editor_property("default_value", unreal.LinearColor(*value))
    return p


def param_scalar(mat, name, value, x, y):
    p = mel.create_material_expression(mat, unreal.MaterialExpressionScalarParameter, x, y)
    p.set_editor_property("parameter_name", name)
    p.set_editor_property("default_value", value)
    return p


def fade(mat, x, y):
    f = mel.create_material_expression(mat, unreal.MaterialExpressionPerInstanceCustomData, x, y)
    f.set_editor_property("data_index", 0)
    f.set_editor_property("const_default_value", 1.0)
    return f


def multiply(mat, a, b, x, y, a_out="", b_out=""):
    m = mel.create_material_expression(mat, unreal.MaterialExpressionMultiply, x, y)
    mel.connect_material_expressions(a, a_out, m, "A")
    mel.connect_material_expressions(b, b_out, m, "B")
    return m


# Glow: additive sparks / embers / motes.
glow, glow_path = new_material("M_WEFxGlow")
glow.set_editor_property("blend_mode", unreal.BlendMode.BLEND_ADDITIVE)
glow.set_editor_property("shading_model", unreal.MaterialShadingModel.MSM_UNLIT)
glow.set_editor_property("two_sided", True)
c = param_vector(glow, "Color", (1.0, 0.45, 0.1, 1.0), -700, -100)
i = param_scalar(glow, "Intensity", 20.0, -700, 100)
f = fade(glow, -700, 250)
ci = multiply(glow, c, i, -450, 0, "RGB")
cif = multiply(glow, ci, f, -250, 50)
mel.connect_material_property(cif, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)
mel.recompile_material(glow)
eal.save_asset(glow_path)

# Smoke: lit translucent spheres with soft (fresnel) edges.
smoke, smoke_path = new_material("M_WEFxSmoke")
smoke.set_editor_property("blend_mode", unreal.BlendMode.BLEND_TRANSLUCENT)
smoke.set_editor_property("shading_model", unreal.MaterialShadingModel.MSM_DEFAULT_LIT)
smoke.set_editor_property("translucency_lighting_mode", unreal.TranslucencyLightingMode.TLM_VOLUMETRIC_NON_DIRECTIONAL)
c = param_vector(smoke, "Color", (0.35, 0.34, 0.33, 1.0), -700, -150)
mel.connect_material_property(c, "RGB", unreal.MaterialProperty.MP_BASE_COLOR)
o = param_scalar(smoke, "Opacity", 0.45, -700, 50)
f = fade(smoke, -700, 200)
fr = mel.create_material_expression(smoke, unreal.MaterialExpressionFresnel, -700, 350)
fr.set_editor_property("exponent", 1.5)
inv = mel.create_material_expression(smoke, unreal.MaterialExpressionOneMinus, -500, 350)
mel.connect_material_expressions(fr, "", inv, "")
# Edge falloff cubed: dense only in the middle, so the spheres read as soft puffs, not balls.
soft = mel.create_material_expression(smoke, unreal.MaterialExpressionPower, -380, 350)
mel.connect_material_expressions(inv, "", soft, "Base")
soft.set_editor_property("const_exponent", 3.0)
of = multiply(smoke, o, f, -450, 100)
ofe = multiply(smoke, of, soft, -250, 200)
# Fade where the puff cuts into floor or walls.
df = mel.create_material_expression(smoke, unreal.MaterialExpressionDepthFade, -120, 250)
df.set_editor_property("fade_distance_default", 40.0)
mel.connect_material_expressions(ofe, "", df, "Opacity")
mel.connect_material_property(df, "", unreal.MaterialProperty.MP_OPACITY)
mel.recompile_material(smoke)
eal.save_asset(smoke_path)

# Emissive: lamp tubes, glowing furnace mouth (not for ISM fading, but ISM-safe).
emis, emis_path = new_material("M_WEEmissive")
emis.set_editor_property("shading_model", unreal.MaterialShadingModel.MSM_UNLIT)
c = param_vector(emis, "Color", (1.0, 0.95, 0.85, 1.0), -600, -50)
i = param_scalar(emis, "Intensity", 8.0, -600, 150)
ci = multiply(emis, c, i, -350, 50, "RGB")
mel.connect_material_property(ci, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)
mel.recompile_material(emis)
eal.save_asset(emis_path)

unreal.log("WE_FX_MATERIALS_BUILT")
unreal.SystemLibrary.quit_editor()
