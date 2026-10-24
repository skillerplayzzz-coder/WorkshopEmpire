"""
Quality overhaul: single variants of Poly Haven files that hold two versions side by side (import_assets.py merges all
parts of a file, so the street showed two hydrants in a row). Uses the parts import_assets.py already imported under
Art/Import/<model>, keeps those whose name contains the given text, centres them and merges them into SM_<variant>.

  UnrealEditor-Cmd.exe <WorkshopEmpire.uproject> -ExecutePythonScript="<this file>" -unattended -RenderOffscreen -nosplash -nosteam
"""
import unreal

ART = "/Game/WorkshopEmpire/Art"
eal = unreal.EditorAssetLibrary
sme = unreal.get_editor_subsystem(unreal.StaticMeshEditorSubsystem)
eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
les = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)

# variant name: (source model, text the kept parts contain)
VARIANTS = {"fire_hydrant_aged": ("fire_hydrant", "aged")}

les.new_level("/Temp/WEVariantTemp", False)  # scratch world for merging (never saved)
done, failed = 0, []
for vid, (mid, keep) in VARIANTS.items():
    final = f"{ART}/Models/SM_{vid}"
    if eal.does_asset_exist(final):
        done += 1
        continue
    parts = [eal.load_asset(p) for p in eal.list_assets(f"{ART}/Import/{mid}", True)]
    parts = [p for p in parts if isinstance(p, unreal.StaticMesh) and keep in p.get_name()]
    if not parts:
        failed.append(vid)
        continue
    boxes = [p.get_bounding_box() for p in parts]
    offset = unreal.Vector(-(min(b.min.x for b in boxes) + max(b.max.x for b in boxes)) * 0.5,
                           -(min(b.min.y for b in boxes) + max(b.max.y for b in boxes)) * 0.5, 0)
    actors = [eas.spawn_actor_from_object(p, offset, unreal.Rotator(0, 0, 0)) for p in parts]
    options = unreal.MergeStaticMeshActorsOptions()
    options.set_editor_property("base_package_name", f"{ART}/Models/{vid}")  # the merge adds the "SM_" prefix itself
    options.set_editor_property("destroy_source_actors", True)
    options.set_editor_property("spawn_merged_actor", False)
    settings = options.get_editor_property("mesh_merging_settings")
    settings.set_editor_property("merge_materials", False)
    settings.set_editor_property("pivot_point_at_zero", True)
    options.set_editor_property("mesh_merging_settings", settings)
    sme.merge_static_mesh_actors(actors, options)
    mesh = eal.load_asset(final)
    if not mesh:
        failed.append(vid)
        continue
    sme.remove_collisions(mesh)
    sme.add_simple_collisions(mesh, unreal.ScriptCollisionShapeType.BOX)
    eal.save_asset(final)
    b = mesh.get_bounding_box()
    unreal.log(f"WE_MODEL {vid} parts={len(parts)} size=({b.max.x - b.min.x:.1f},{b.max.y - b.min.y:.1f},{b.max.z - b.min.z:.1f})")
    done += 1
unreal.log(f"WE_VARIANTS done={done} failed={failed}")
unreal.SystemLibrary.quit_editor()
