"""
Phase 19: material of the workshop mirror (AWEMirror). The glass shows the picture of the mirror's scene capture,
flipped left/right. The UVs come from the world position on the glass (Center, Right, Up, Size are set by the
actor), so it does not depend on the mesh's own UVs.

  /Game/WorkshopEmpire/Materials/M_WEMirror

Run: UnrealEditor-Cmd.exe <uproject> -ExecutePythonScript="<this file>" -unattended -RenderOffscreen -nosplash -nosteam
"""
import unreal

eal = unreal.EditorAssetLibrary
mel = unreal.MaterialEditingLibrary
tools = unreal.AssetToolsHelpers.get_asset_tools()
FOLDER = "/Game/WorkshopEmpire/Materials"
PATH = f"{FOLDER}/M_WEMirror"

UV_HLSL = """
float3 d = WP - Center;
float u = dot(d, Right) / max(Size.x, 1.0) + 0.5;
float v = 0.5 - dot(d, Up) / max(Size.y, 1.0);
return float2(1.0 - u, v);
"""


def vec(m, name, value, x, y):
    p = mel.create_material_expression(m, unreal.MaterialExpressionVectorParameter, x, y)
    p.set_editor_property("parameter_name", name)
    p.set_editor_property("default_value", unreal.LinearColor(*value))
    return p


def main():
    if eal.does_asset_exist(PATH):
        eal.delete_asset(PATH)
    m = tools.create_asset("M_WEMirror", FOLDER, unreal.Material, unreal.MaterialFactoryNew())
    m.set_editor_property("shading_model", unreal.MaterialShadingModel.MSM_UNLIT)
    wp = mel.create_material_expression(m, unreal.MaterialExpressionWorldPosition, -1100, -200)
    custom = mel.create_material_expression(m, unreal.MaterialExpressionCustom, -800, -200)
    custom.set_editor_property("code", UV_HLSL)
    custom.set_editor_property("output_type", unreal.CustomMaterialOutputType.CMOT_FLOAT2)
    inputs = []
    for n in ("WP", "Center", "Right", "Up", "Size"):
        ci = unreal.CustomInput()
        ci.set_editor_property("input_name", n)
        inputs.append(ci)
    custom.set_editor_property("inputs", inputs)
    mel.connect_material_expressions(wp, "", custom, "WP")
    for i, (n, v) in enumerate((("Center", (0, 0, 0, 1)), ("Right", (0, 1, 0, 1)), ("Up", (0, 0, 1, 1)), ("Size", (70, 150, 0, 1)))):
        mel.connect_material_expressions(vec(m, n, v, -1100, -60 + i * 120), "RGB", custom, n)
    tex = mel.create_material_expression(m, unreal.MaterialExpressionTextureSampleParameter2D, -500, -200)
    tex.set_editor_property("parameter_name", "Capture")
    tex.set_editor_property("texture", unreal.load_asset("/Engine/EngineResources/Black"))
    mel.connect_material_expressions(custom, "", tex, "UVs")
    # A hint of glass: slightly cool and a touch darker than the room.
    tint = mel.create_material_expression(m, unreal.MaterialExpressionMultiply, -250, -200)
    mel.connect_material_expressions(tex, "RGB", tint, "A")
    mel.connect_material_expressions(vec(m, "Tint", (0.9, 0.95, 0.97, 1), -500, 50), "RGB", tint, "B")
    # The capture is the finished picture (colour grading and exposure included). The inverse eye adaptation
    # undoes the exposure of the player's camera, so the glass shows it as it is, whatever the room brightness.
    exposure = mel.create_material_expression(m, unreal.MaterialExpressionScalarParameter, -250, 50)
    exposure.set_editor_property("parameter_name", "Exposure")
    exposure.set_editor_property("default_value", 1.15)
    out = mel.create_material_expression(m, unreal.MaterialExpressionMultiply, -100, -150)
    mel.connect_material_expressions(tint, "", out, "A")
    mel.connect_material_expressions(exposure, "", out, "B")
    inverse = mel.create_material_expression(m, unreal.MaterialExpressionEyeAdaptationInverse, 50, -150)
    mel.connect_material_expressions(out, "", inverse, "")
    mel.connect_material_property(inverse, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)
    mel.recompile_material(m)
    eal.save_asset(PATH)
    unreal.log("WE_MIRROR material " + PATH)


main()
unreal.SystemLibrary.quit_editor()
