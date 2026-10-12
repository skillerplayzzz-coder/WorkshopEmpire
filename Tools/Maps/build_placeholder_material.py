"""
Creates the placeholder material used by all block-out geometry:
  /Game/WorkshopEmpire/Materials/M_WEPlaceholder   (vector param "Color", scalar param "Roughness")
  /Game/WorkshopEmpire/Materials/MI_WEGround       (outdoor ground)

Run BEFORE build_garage_map.py (C++ classes reference the material in their constructors):
  UnrealEditor-Cmd.exe <WorkshopEmpire.uproject> -ExecutePythonScript="<this file>" -unattended -RenderOffscreen -nosplash
"""
import unreal

FOLDER = "/Game/WorkshopEmpire/Materials"
MAT_PATH = FOLDER + "/M_WEPlaceholder"
GROUND_PATH = FOLDER + "/MI_WEGround"

tools = unreal.AssetToolsHelpers.get_asset_tools()
mel = unreal.MaterialEditingLibrary
eal = unreal.EditorAssetLibrary

for path in (GROUND_PATH, MAT_PATH):
    if eal.does_asset_exist(path):
        eal.delete_asset(path)

mat = tools.create_asset("M_WEPlaceholder", FOLDER, unreal.Material, unreal.MaterialFactoryNew())
# Walls/floors/furniture are instanced static meshes; without this flag the default material is used in game.
mat.set_editor_property("used_with_instanced_static_meshes", True)

color = mel.create_material_expression(mat, unreal.MaterialExpressionVectorParameter, -400, -100)
color.set_editor_property("parameter_name", "Color")
color.set_editor_property("default_value", unreal.LinearColor(0.6, 0.6, 0.6, 1.0))
mel.connect_material_property(color, "", unreal.MaterialProperty.MP_BASE_COLOR)

rough = mel.create_material_expression(mat, unreal.MaterialExpressionScalarParameter, -400, 150)
rough.set_editor_property("parameter_name", "Roughness")
rough.set_editor_property("default_value", 0.75)
mel.connect_material_property(rough, "", unreal.MaterialProperty.MP_ROUGHNESS)

mel.recompile_material(mat)
eal.save_asset(MAT_PATH)

ground = tools.create_asset("MI_WEGround", FOLDER, unreal.MaterialInstanceConstant, unreal.MaterialInstanceConstantFactoryNew())
ground.set_editor_property("parent", mat)
mel.set_material_instance_vector_parameter_value(ground, "Color", unreal.LinearColor(0.12, 0.13, 0.12, 1.0))
mel.set_material_instance_scalar_parameter_value(ground, "Roughness", 0.9)
eal.save_asset(GROUND_PATH)

unreal.log("WE_MATERIALS_BUILT")
unreal.SystemLibrary.quit_editor()
