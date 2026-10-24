"""
Massive quality overhaul: outdoor fixes in the existing maps (L_Garage, L_MainMenu) without rebuilding them.
  - the earth heaps at the edges of the playable area were rotated boxes: real mounds (SM_Gen_DirtMound) now,
    same place and size, same ground material
  - traffic signals: modelled heads with one lamp lit instead of a box with all three lenses glowing

  UnrealEditor-Cmd.exe <uproject> -ExecutePythonScript="<this file>" -unattended -RenderOffscreen -nosplash -nosteam
"""
import math

import unreal

eal = unreal.EditorAssetLibrary
les = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
MOUND = eal.load_asset("/Game/WorkshopEmpire/Art/Generated/SM_Gen_DirtMound")
SIGNAL = eal.load_asset("/Game/WorkshopEmpire/Art/Generated/SM_Gen_TrafficSignal")
HYDRANT = eal.load_asset("/Game/WorkshopEmpire/Art/Models/SM_fire_hydrant_aged")
LENS_MATERIALS =("MI_Prop_Taillight", "MI_Prop_Indicator", "MI_Prop_LedGreen")
LIT = {n: eal.load_asset(f"/Game/WorkshopEmpire/Materials/Props/MI_Prop_{n}") for n in ("SignalGreen", "SignalRed")}


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
    signals = patch_traffic_lights()
    hydrants = 0
    for a in eas.get_all_level_actors():
        if HYDRANT and isinstance(a, unreal.StaticMeshActor) and a.static_mesh_component.static_mesh \
                and a.static_mesh_component.static_mesh.get_name() == "SM_fire_hydrant":
            a.static_mesh_component.set_static_mesh(HYDRANT)  # one hydrant, not the clean and the aged one side by side
            hydrants += 1
    boards = 0
    for a in eas.get_all_level_actors():
        # The company sign beside the gate (the logo at 292 cm on the white board): dark letters.
        if isinstance(a, unreal.WELogoActor) and abs(a.get_actor_location().z - 292.0) < 1.0:
            a.get_component_by_class(unreal.WELogoComponent).set_editor_property("on_light_board", True)
            boards += 1
    les.save_current_level()
    unreal.log(f"WE_WORLDPATCH {level}: {done} heaps, {signals} traffic signals, {hydrants} hydrants, {boards} sign boards")


def material_name(c, i):
    m = c.get_material(i)
    return m.get_name() if m else ""


def patch_traffic_lights():
    """The signal heads were a black box with three flat cylinders - all three lit at once. Now a modelled head, one
    lamp lit: green for the arms along X (main street), red for the cross direction."""
    if not SIGNAL:
        return 0
    actors = [a for a in eas.get_all_level_actors() if isinstance(a, unreal.StaticMeshActor) and a.static_mesh_component.static_mesh]
    lenses = [a for a in actors if a.static_mesh_component.static_mesh.get_name() == "Cylinder"
              and abs(a.get_actor_scale3d().z - 0.03) < 0.005 and material_name(a.static_mesh_component, 0) in LENS_MATERIALS]
    heads = [a for a in actors if a.static_mesh_component.static_mesh.get_name() == "Cube"
             and abs(a.get_actor_scale3d().x - 0.34) < 0.01 and abs(a.get_actor_scale3d().z - 1.1) < 0.01
             and material_name(a.static_mesh_component, 0) == "MI_Prop_TrimBlack"]
    done = 0
    for head in heads:
        hl = head.get_actor_location()
        mine = [l for l in lenses if abs(l.get_actor_location().x - hl.x) < 40.0 and abs(l.get_actor_location().y - hl.y) < 40.0]
        if len(mine) != 3:
            continue
        arm_yaw = head.get_actor_rotation().yaw
        r = math.radians(arm_yaw)
        nx, ny = -math.sin(r), math.cos(r)  # lens direction (towards the traffic)
        bottom = hl.z - 55.0  # the box pivot sits in its middle (110 cm high)
        sig = eas.spawn_actor_from_class(unreal.StaticMeshActor, unreal.Vector(hl.x - nx * 12.0, hl.y - ny * 12.0, bottom),
                                         unreal.Rotator(roll=0.0, pitch=0.0, yaw=arm_yaw + 90.0))
        sig.set_actor_label(f"TrafficSignal_{done}")
        sig.set_folder_path("City")
        c = sig.static_mesh_component
        c.set_static_mesh(SIGNAL)
        c.set_collision_enabled(unreal.CollisionEnabled.NO_COLLISION)
        green = abs(math.cos(r)) > 0.7
        on_name, on_mat = ("MI_Prop_SignalGreenOff", LIT["SignalGreen"]) if green else ("MI_Prop_SignalRedOff", LIT["SignalRed"])
        for i in range(c.get_num_materials()):
            if material_name(c, i) == on_name:
                c.set_material(i, on_mat)
        for l in mine:
            eas.destroy_actor(l)
        eas.destroy_actor(head)
        done += 1
    return done


for lvl in ("/Game/WorkshopEmpire/Maps/L_Garage", "/Game/WorkshopEmpire/Maps/L_MainMenu"):
    patch(lvl)
unreal.SystemLibrary.quit_editor()
