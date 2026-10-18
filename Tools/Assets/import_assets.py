"""
Imports the CC0 source assets from SourceArt/ (see download_assets.py, Docs/ASSET_SOURCES.md):
  Models   -> /Game/WorkshopEmpire/Art/Models/SM_<id>   (glTF via Interchange; parts merged into one
              mesh in model space, box collision, materials/textures under Art/Import/<id>)
  Textures -> /Game/WorkshopEmpire/Art/Textures/<id>/  and the surfaces
              /Game/WorkshopEmpire/Materials/Surfaces/MI_WE<Kind> (instances of M_WESurface)
  Sounds   -> /Game/WorkshopEmpire/Audio/Kenney/<name>   (Kenney impact + interface, 3D attenuation)

Run after build_surface_material.py and build_audio.py:
  UnrealEditor-Cmd.exe <WorkshopEmpire.uproject> -ExecutePythonScript="<this file>" -unattended -RenderOffscreen -nosplash -nosteam
"""
import os

import unreal

ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), "..", ".."))
SRC = os.path.join(ROOT, "SourceArt")
ART = "/Game/WorkshopEmpire/Art"
SURFACES = "/Game/WorkshopEmpire/Materials/Surfaces"
eal = unreal.EditorAssetLibrary
tools = unreal.AssetToolsHelpers.get_asset_tools()
mel = unreal.MaterialEditingLibrary
sme = unreal.get_editor_subsystem(unreal.StaticMeshEditorSubsystem)
eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
les = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)


def import_file(path, dest, name=None):
    task = unreal.AssetImportTask()
    task.set_editor_property("filename", path)
    task.set_editor_property("destination_path", dest)
    if name:
        task.set_editor_property("destination_name", name)
    task.set_editor_property("automated", True)
    task.set_editor_property("replace_existing", True)
    task.set_editor_property("save", True)
    tools.import_asset_tasks([task])
    return list(task.get_editor_property("imported_object_paths"))


# --- Models ------------------------------------------------------------------------------------------
# Some glTF files hold several variants side by side; only one is kept (parts whose name contains the text are dropped).
DROP_PARTS = {"rollershutter_door": "graffiti", "metal_trash_can": "rust"}
VEGETATION = {"shrub_02", "shrub_03", "shrub_04", "weed_plant_02", "nettle_plant", "dandelion_01", "grass_bermuda_01", "tree_small_02"}
les.new_level("/Temp/WEMergeTemp", False)  # scratch world for merging (in /Temp, never saved)
models_dir = os.path.join(SRC, "PolyHaven", "models")
done, failed = 0, []
for mid in sorted(os.listdir(models_dir)) if os.path.isdir(models_dir) else []:
    gltf = os.path.join(models_dir, mid, mid + ".gltf")
    final = f"{ART}/Models/SM_{mid}"
    if not os.path.exists(gltf):
        continue
    if eal.does_asset_exist(final):
        done += 1  # already imported (re-importing over loaded assets is not safe)
        continue
    import_dest = f"{ART}/Import/{mid}"
    parts = [eal.load_asset(p) for p in eal.list_assets(import_dest, True) if isinstance(eal.load_asset(p), unreal.StaticMesh)]
    if not parts:  # only import once: re-importing over loaded assets is not safe
        import_file(gltf, import_dest)
        parts = [eal.load_asset(p) for p in eal.list_assets(import_dest, True) if isinstance(eal.load_asset(p), unreal.StaticMesh)]
    offset = unreal.Vector(0, 0, 0)
    if mid in DROP_PARTS:
        parts = [p for p in parts if DROP_PARTS[mid] not in p.get_name()]
        # Re-centre the kept variant on the origin (X/Y).
        boxes = [p.get_bounding_box() for p in parts]
        if boxes:
            offset = unreal.Vector(-(min(b.min.x for b in boxes) + max(b.max.x for b in boxes)) * 0.5,
                                   -(min(b.min.y for b in boxes) + max(b.max.y for b in boxes)) * 0.5, 0)
    if not parts:
        failed.append(mid)
        continue
    if eal.does_asset_exist(final):
        eal.delete_asset(final)
    if len(parts) == 1 and offset.length() < 1.0:
        # Single mesh: a copy under the stable name (materials stay where they were imported).
        eal.duplicate_asset(parts[0].get_path_name().split(".")[0], final)
    else:
        # Parts are baked into model space by the importer: placed at the origin they form the model.
        actors = [eas.spawn_actor_from_object(p, offset, unreal.Rotator(0, 0, 0)) for p in parts]
        options = unreal.MergeStaticMeshActorsOptions()
        options.set_editor_property("base_package_name", f"{ART}/Models/{mid}")  # the merge adds the "SM_" prefix itself
        options.set_editor_property("destroy_source_actors", True)
        options.set_editor_property("spawn_merged_actor", False)
        settings = options.get_editor_property("mesh_merging_settings")
        settings.set_editor_property("merge_materials", False)
        settings.set_editor_property("pivot_point_at_zero", True)
        options.set_editor_property("mesh_merging_settings", settings)
        sme.merge_static_mesh_actors(actors, options)
        eal.save_asset(final)
    mesh = eal.load_asset(final)
    if not mesh:
        failed.append(mid)
        continue
    # Simple box collision (items are physics objects; furniture models use their box parts instead).
    # Small props: no Nanite (some have translucent glass, which Nanite cannot draw).
    # Phase 18e: vegetation is dense (the tree has millions of triangles): Nanite on, no collision for small plants.
    nanite = mesh.get_editor_property("nanite_settings")
    nanite.set_editor_property("enabled", mid in VEGETATION)
    mesh.set_editor_property("nanite_settings", nanite)
    sme.remove_collisions(mesh)
    if mid not in VEGETATION or mid.startswith("tree"):
        sme.add_simple_collisions(mesh, unreal.ScriptCollisionShapeType.BOX)
    eal.save_asset(final)
    b = mesh.get_bounding_box()
    size = b.max - b.min
    unreal.log(f"WE_MODEL {mid} parts={len(parts)} size=({size.x:.1f},{size.y:.1f},{size.z:.1f}) min_z={b.min.z:.1f}")
    done += 1

# Phase 17.5: the fence and pole packs are used part by part (world_neighbourhood.py). Their wire material is
# translucent, which Nanite cannot draw: the parts are plain meshes.
for pack in ("modular_chainlink_fence", "modular_electricity_poles"):
    for p in eal.list_assets(f"{ART}/Import/{pack}/{pack}/StaticMeshes", False):
        part = eal.load_asset(p)
        if isinstance(part, unreal.StaticMesh) and part.get_editor_property("nanite_settings").enabled:
            s = part.get_editor_property("nanite_settings")
            s.set_editor_property("enabled", False)
            part.set_editor_property("nanite_settings", s)
            eal.save_asset(p)

# --- Surface textures --------------------------------------------------------------------------------
TEX = {
    # Kind: (texture id, tile size cm, desaturation, brightness, metallic scale, roughness scale)
    "ConcreteFloor": ("garage_floor", 320.0, 0.0, 1.0, 0.0, 1.0),
    "Asphalt": ("asphalt_02", 450.0, 0.0, 1.0, 0.0, 1.0),
    "PaintedWall": ("beige_wall_001", 260.0, 0.0, 1.05, 0.0, 1.0),
    "ConcreteWall": ("concrete_wall_008", 320.0, 0.0, 1.0, 0.0, 1.0),
    "Brick": ("brick_wall_005", 220.0, 0.0, 1.0, 0.0, 1.0),
    "BlockWall": ("concrete_block_wall", 260.0, 0.0, 1.0, 0.0, 1.0),
    "Corrugated": ("corrugated_iron_02", 220.0, 0.0, 1.0, 1.0, 1.0),
    "FactoryWall": ("factory_wall", 320.0, 0.0, 1.0, 1.0, 1.0),
    "RoofMetal": ("corrugated_iron_02", 220.0, 0.6, 0.8, 1.0, 1.0),
    "PaintedMetal": ("blue_metal_plate", 160.0, 1.0, 1.6, 1.0, 1.0),
    "RustyMetal": ("rusty_painted_metal", 160.0, 0.0, 1.0, 1.0, 1.0),
    "TreadPlate": ("metal_plate", 120.0, 0.0, 1.0, 1.0, 1.0),
    "Plywood": ("plywood", 180.0, 0.0, 1.0, 0.0, 1.0),
    "Osb": ("oriented_strand_board", 160.0, 0.0, 1.0, 0.0, 1.0),
    # Phase 17.5: streets, neighbours and floor coverings
    "CrackedAsphalt": ("road_damaged", 500.0, 0.0, 1.0, 0.0, 1.0),
    "WornAsphalt": ("worn_asphalt", 450.0, 0.0, 1.0, 0.0, 1.0),
    "Sidewalk": ("concrete_pavement", 240.0, 0.0, 1.0, 0.0, 1.0),
    "RedBrick": ("red_bricks_04", 200.0, 0.0, 1.0, 0.0, 1.0),
    "PaintedBrick": ("painted_worn_brick", 220.0, 0.0, 1.0, 0.0, 1.0),
    "DarkBrick": ("dark_brick_wall", 220.0, 0.0, 1.0, 0.0, 1.0),
    "RustyCorrugated": ("rusty_corrugated_iron", 240.0, 0.0, 1.0, 1.0, 1.0),
    "WornCorrugated": ("worn_corrugated_iron", 240.0, 0.0, 1.0, 1.0, 1.0),
    "RustedShutter": ("rusted_shutter", 260.0, 0.0, 1.0, 1.0, 1.0),
    "PaintedShutter": ("painted_metal_shutter", 260.0, 0.0, 1.0, 1.0, 1.0),
    "RubberTiles": ("rubber_tiles", 120.0, 0.0, 1.0, 0.0, 1.0),
    "PaintedConcrete": ("painted_concrete", 320.0, 0.0, 1.0, 0.0, 1.0),
    "AntiSlip": ("anti_slip_concrete", 260.0, 0.0, 1.0, 0.0, 1.0),
    "WoodFloor": ("wood_floor_worn", 200.0, 0.0, 1.0, 0.0, 1.0),
    "Linoleum": ("old_linoleum_flooring_01", 220.0, 0.0, 1.0, 0.0, 1.0),
    "FloorTiles": ("floor_tiles_06", 180.0, 0.0, 1.0, 0.0, 1.0),
    "Plaster": ("plastered_wall_04", 280.0, 0.0, 1.0, 0.0, 1.0),
    "WhitewashedBrick": ("whitewashed_brick", 220.0, 0.0, 1.0, 0.0, 1.0),
    "SteelPlate": ("metal_plate_02", 160.0, 0.0, 1.0, 1.0, 1.0),
    "GreenRust": ("green_metal_rust", 200.0, 0.0, 1.0, 1.0, 1.0),
    "Gravel": ("gravel_floor", 300.0, 0.0, 1.0, 0.0, 1.0),
    "BoxProfile": ("box_profile_metal_sheet", 220.0, 0.0, 1.0, 1.0, 1.0),
    "ConcretePanels": ("concrete_panels", 400.0, 0.0, 1.0, 0.0, 1.0),
}
surface_parent = eal.load_asset("/Game/WorkshopEmpire/Materials/M_WESurface")
textures_dir = os.path.join(SRC, "PolyHaven", "textures")
imported_tex = {}
for kind, (tid, tile, desat, bright, metal, rough) in TEX.items():
    if tid not in imported_tex:
        maps = {}
        folder = os.path.join(textures_dir, tid)
        for fn in sorted(os.listdir(folder)) if os.path.isdir(folder) else []:
            short = "diff" if "_diff_" in fn else "nor" if "_nor_gl_" in fn else "arm" if "_arm_" in fn else None
            if not short:
                continue
            name = f"T_{tid}_{short}"
            dest = f"{ART}/Textures/{tid}"
            if not eal.does_asset_exist(f"{dest}/{name}"):
                import_file(os.path.join(folder, fn), dest, name)
            tex = eal.load_asset(f"{dest}/{name}")
            if tex and short != "diff":
                tex.set_editor_property("srgb", False)
                tex.set_editor_property("compression_settings",
                                        unreal.TextureCompressionSettings.TC_NORMALMAP if short == "nor" else unreal.TextureCompressionSettings.TC_MASKS)
                eal.save_asset(f"{dest}/{name}")
            maps[short] = tex
        imported_tex[tid] = maps
    maps = imported_tex[tid]
    path = f"{SURFACES}/MI_WE{kind}"
    mi = eal.load_asset(path) if eal.does_asset_exist(path) else tools.create_asset(f"MI_WE{kind}", SURFACES, unreal.MaterialInstanceConstant, unreal.MaterialInstanceConstantFactoryNew())
    mi.set_editor_property("parent", surface_parent)
    if maps.get("diff"):
        mel.set_material_instance_texture_parameter_value(mi, "BaseColorTex", maps["diff"])
    if maps.get("nor"):
        mel.set_material_instance_texture_parameter_value(mi, "NormalTex", maps["nor"])
    if maps.get("arm"):
        mel.set_material_instance_texture_parameter_value(mi, "ARMTex", maps["arm"])
    mel.set_material_instance_scalar_parameter_value(mi, "TileSize", tile)
    mel.set_material_instance_scalar_parameter_value(mi, "Desaturation", desat)
    mel.set_material_instance_scalar_parameter_value(mi, "Brightness", bright)
    mel.set_material_instance_scalar_parameter_value(mi, "MetallicScale", metal)
    mel.set_material_instance_scalar_parameter_value(mi, "RoughnessScale", rough)
    eal.save_asset(path)
    unreal.log(f"WE_SURFACE {kind} <- {tid} maps={sorted(k for k, v in maps.items() if v)}")

# --- Kenney sounds -----------------------------------------------------------------------------------
KEEP = ("footstep_concrete", "impactMetal_light", "impactMetal_medium", "impactMetal_heavy", "impactWood_light", "impactWood_medium",
        "impactPlank_medium", "impactPlate_light", "impactPlate_medium", "impactSoft_medium", "impactGeneric_light", "impactTin_medium",
        "click", "select", "switch", "confirmation", "error", "drop", "toggle", "open", "close")
att_small = eal.load_asset("/Game/WorkshopEmpire/Audio/SA_WESmall")
sounds = 0
for root, _, files in os.walk(os.path.join(SRC, "Kenney")):
    for fn in sorted(files):
        if not fn.endswith(".ogg") or not fn.startswith(KEEP):
            continue
        name = os.path.splitext(fn)[0]
        dest = "/Game/WorkshopEmpire/Audio/Kenney"
        if not eal.does_asset_exist(f"{dest}/{name}"):
            import_file(os.path.join(root, fn), dest, name)
        wave = eal.load_asset(f"{dest}/{name}")
        if wave:
            ui = not name.startswith(("footstep", "impact"))
            wave.set_editor_property("attenuation_settings", None if ui else att_small)
            wave.set_editor_property("volume", 0.5 if ui else 0.8)
            eal.save_asset(f"{dest}/{name}")
            sounds += 1

# The scratch level is not needed.
for temp in ("/Game/WorkshopEmpire/Art/_MergeTemp", "/Game/WorkshopEmpire/Art/_MergeTemp2"):
    if eal.does_asset_exist(temp):
        les.load_level("/Engine/Maps/Templates/Template_Default")
        eal.delete_asset(temp)

unreal.log(f"WE_ASSETS_IMPORTED models={done} failed={failed} surfaces={len(TEX)} sounds={sounds}")
unreal.SystemLibrary.quit_editor()
