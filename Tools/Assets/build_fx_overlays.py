"""
Phase 18d: see-through material of the build preview (the furniture model shown green where it fits, red where not):
  /Game/WorkshopEmpire/Materials/FX/M_WEGhost  (translucent, unlit; vector parameter "Color", scalar "Opacity")

Run any time after the project compiles:
  UnrealEditor-Cmd.exe <uproject> -ExecutePythonScript="<this file>" -unattended -RenderOffscreen -nosplash -nosteam
"""
import unreal

FX = "/Game/WorkshopEmpire/Materials/FX"
mel = unreal.MaterialEditingLibrary
eal = unreal.EditorAssetLibrary


def ghost():
    path = f"{FX}/M_WEGhost"
    if eal.does_asset_exist(path):
        eal.delete_asset(path)
    mat = unreal.AssetToolsHelpers.get_asset_tools().create_asset("M_WEGhost", FX, unreal.Material, unreal.MaterialFactoryNew())
    mat.set_editor_property("blend_mode", unreal.BlendMode.BLEND_TRANSLUCENT)
    mat.set_editor_property("shading_model", unreal.MaterialShadingModel.MSM_UNLIT)
    mat.set_editor_property("two_sided", True)
    color = mel.create_material_expression(mat, unreal.MaterialExpressionVectorParameter, -700, -200)
    color.set_editor_property("parameter_name", "Color")
    color.set_editor_property("default_value", unreal.LinearColor(0.2, 0.85, 0.3, 1.0))
    opacity = mel.create_material_expression(mat, unreal.MaterialExpressionScalarParameter, -700, 200)
    opacity.set_editor_property("parameter_name", "Opacity")
    opacity.set_editor_property("default_value", 0.35)
    fres = mel.create_material_expression(mat, unreal.MaterialExpressionFresnel, -700, 0)
    fres.set_editor_property("exponent", 2.0)
    fres.set_editor_property("base_reflect_fraction", 0.1)
    # Edges glow brighter than faces: the shape reads clearly without hiding what is behind it.
    lift = mel.create_material_expression(mat, unreal.MaterialExpressionAdd, -500, 0)
    lift.set_editor_property("const_b", 0.45)
    mel.connect_material_expressions(fres, "", lift, "A")
    emissive = mel.create_material_expression(mat, unreal.MaterialExpressionMultiply, -300, -150)
    mel.connect_material_expressions(color, "", emissive, "A")
    mel.connect_material_expressions(lift, "", emissive, "B")
    mel.connect_material_property(emissive, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)
    op = mel.create_material_expression(mat, unreal.MaterialExpressionMultiply, -300, 150)
    mel.connect_material_expressions(lift, "", op, "A")
    mel.connect_material_expressions(opacity, "", op, "B")
    mel.connect_material_property(op, "", unreal.MaterialProperty.MP_OPACITY)
    mel.recompile_material(mat)
    eal.save_asset(path)
    unreal.log("WE_FX M_WEGhost")


ghost()
unreal.SystemLibrary.quit_editor()
