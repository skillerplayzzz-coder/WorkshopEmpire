"""
Imports the parked cars of the town from Kenney's "Car Kit" (CC0, SourceArt/Kenney/CarKit, see Docs/ASSET_SOURCES.md):
brand-free sedans, hatchback, SUVs, van and pickup, each as one static mesh (body and wheels combined) with the kit's
colour atlas. world_city.py places them (scaled to real car sizes there).

  UnrealEditor-Cmd.exe <WorkshopEmpire.uproject> -ExecutePythonScript="<this file>" -unattended -RenderOffscreen -nosplash
"""
import os

import unreal

PROJECT = unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir())
SRC = os.path.join(PROJECT, "SourceArt", "Kenney", "CarKit", "Models", "FBX format")
DEST = "/Game/WorkshopEmpire/Art/Import/KenneyCars"
CARS = ["sedan", "sedan-sports", "hatchback-sports", "suv", "suv-luxury", "van", "delivery", "truck"]

tasks = []
for car in CARS:
    path = os.path.join(SRC, car + ".fbx")
    if not os.path.exists(path):
        unreal.log_error(f"WE_CARS missing {path}")
        continue
    options = unreal.FbxImportUI()
    options.set_editor_property("import_mesh", True)
    options.set_editor_property("import_as_skeletal", False)
    options.set_editor_property("import_materials", True)
    options.set_editor_property("import_textures", True)
    options.set_editor_property("import_animations", False)
    options.set_editor_property("mesh_type_to_import", unreal.FBXImportType.FBXIT_STATIC_MESH)
    sm = options.static_mesh_import_data
    sm.set_editor_property("combine_meshes", True)
    sm.set_editor_property("auto_generate_collision", True)
    sm.set_editor_property("generate_lightmap_u_vs", True)
    task = unreal.AssetImportTask()
    task.set_editor_property("filename", path)
    task.set_editor_property("destination_path", DEST)
    task.set_editor_property("destination_name", "SM_Car_" + car.replace("-", "_"))
    task.set_editor_property("automated", True)
    task.set_editor_property("replace_existing", True)
    task.set_editor_property("save", True)
    task.set_editor_property("options", options)
    tasks.append(task)
unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks(tasks)

for car in CARS:
    asset = f"{DEST}/SM_Car_{car.replace('-', '_')}"
    mesh = unreal.EditorAssetLibrary.load_asset(asset) if unreal.EditorAssetLibrary.does_asset_exist(asset) else None
    if not mesh:
        unreal.log_error(f"WE_CARS not imported: {car}")
        continue
    b = mesh.get_bounding_box()
    unreal.log(f"WE_CARS {car}: min={b.min} max={b.max} materials={mesh.get_num_sections(0)}")
unreal.SystemLibrary.quit_editor()
