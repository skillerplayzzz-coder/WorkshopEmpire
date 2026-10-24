"""
Massive quality overhaul: outdoor fixes in the existing maps (L_Garage, L_MainMenu) without rebuilding them.
  - the earth heaps at the edges of the playable area were rotated boxes: real mounds (SM_Gen_DirtMound) now,
    same place and size, same ground material

  UnrealEditor-Cmd.exe <uproject> -ExecutePythonScript="<this file>" -unattended -RenderOffscreen -nosplash -nosteam
"""
import unreal

eal = unreal.EditorAssetLibrary
les = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
MOUND = eal.load_asset("/Game/WorkshopEmpire/Art/Generated/SM_Gen_DirtMound")


def patch(level):
    if not les.load_level(level):
        return
    done = 0
    for a in eas.get_all_level_actors():
        if not isinstance(a, unreal.StaticMeshActor) or not a.get_actor_label().startswith("DirtHeap"):
            continue
        c = a.static_mesh_component
        if c.static_mesh == MOUND:
            continue
        org, ext = a.get_actor_bounds(False)
        s = a.get_actor_scale3d()
        mat = c.get_material(0)
        c.set_static_mesh(MOUND)
        c.set_material(0, mat)
        b = MOUND.get_bounding_box()
        size = b.max - b.min
        # same footprint, a little lower than the box was (heaps slope)
        a.set_actor_scale3d(unreal.Vector(s.x * 100.0 / size.x, s.y * 100.0 / size.y, (ext.z * 2.0 * 0.85) / size.z))
        loc = a.get_actor_location()
        a.set_actor_location(unreal.Vector(loc.x, loc.y, 0.0), False, False)
        done += 1
    les.save_current_level()
    unreal.log(f"WE_WORLDPATCH {level}: {done} heaps")


for lvl in ("/Game/WorkshopEmpire/Maps/L_Garage", "/Game/WorkshopEmpire/Maps/L_MainMenu"):
    patch(lvl)
unreal.SystemLibrary.quit_editor()
