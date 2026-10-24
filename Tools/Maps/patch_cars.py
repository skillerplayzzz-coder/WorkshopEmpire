"""
Quality update: swaps the parked Kenney cars in the existing maps (L_Garage, L_MainMenu) for the cars of
Tools/Assets/build_cars.py without rebuilding the maps: same places, new model (nose along the old car's heading),
real size, a colour per car. world_city.py places the new cars directly when a map is built from scratch.

  UnrealEditor-Cmd.exe <uproject> -ExecutePythonScript="<this file>" -unattended -RenderOffscreen -nosplash -nosteam
"""
import random

import unreal

eal = unreal.EditorAssetLibrary
les = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)

CARS = ["SedanOld", "Wagon", "Pickup", "Suv", "Hatch", "SedanOld", "Hatch", "Suv"]
PAINTS = ["CarPaintRed", "CarPaintBlue", "CarPaintGreen", "CarPaintBeige", "CarPaintWhite", "CarPaintBlack", "CarPaintSilver",
          "CarPaintBrown", "CarPaintMaroon", "CarPaintWhite", "CarPaintSilver"]
rng = random.Random(1987)


def paint_slot(c):
    """Material slot of the paint (the slot order of a generated mesh is not the build order)."""
    for i in range(c.get_num_materials()):
        m = c.get_material(i)
        if m and m.get_name().startswith("MI_Prop_CarPaint"):
            return i
    return -1


def mesh(name):
    return eal.load_asset(f"/Game/WorkshopEmpire/Art/Generated/SM_Gen_Car_{name}")


def patch(level):
    if not les.load_level(level):
        unreal.log_error(f"WE_CARPATCH cannot load {level}")
        return
    swapped = 0
    shop = 0
    for a in eas.get_all_level_actors():
        if not isinstance(a, unreal.StaticMeshActor):
            continue
        c = a.static_mesh_component
        sm = c.static_mesh
        if not sm or ("/KenneyCars/" not in sm.get_path_name() and "SM_Gen_Car_" not in sm.get_name()):
            continue
        kenney = "/KenneyCars/" in sm.get_path_name()
        loc = a.get_actor_location()
        rot = a.get_actor_rotation()
        if not kenney:
            # Already swapped (a re-run after the car models changed): keep model, place and heading, fix the paint.
            for i in range(c.get_num_materials()):
                c.set_material(i, None)
            slot = paint_slot(c)
            if slot >= 0 and "PickupOld" not in sm.get_name():
                c.set_material(slot, eal.load_asset(f"/Game/WorkshopEmpire/Materials/Props/MI_Prop_{rng.choice(PAINTS)}"))
            swapped += 1
            continue
        # Near the car repair shop (south-east) one is the shop's old pickup.
        name = "PickupOld" if loc.x > 8000.0 and shop == 0 else rng.choice(CARS)
        shop += 1 if name == "PickupOld" else 0
        new = mesh(name)
        if not new:
            unreal.log_error(f"WE_CARPATCH model missing: {name}")
            return
        c.set_static_mesh(new)
        for i in range(c.get_num_materials()):
            c.set_material(i, None)
        if name != "PickupOld" and paint_slot(c) >= 0:
            c.set_material(paint_slot(c), eal.load_asset(f"/Game/WorkshopEmpire/Materials/Props/MI_Prop_{rng.choice(PAINTS)}"))
        # Kenney models are long along Y (placed with yaw + 90); the new ones along X. A little off-straight.
        a.set_actor_rotation(unreal.Rotator(roll=0.0, pitch=0.0, yaw=rot.yaw - 90.0 + rng.uniform(-2.5, 2.5)), False)
        a.set_actor_scale3d(unreal.Vector(1.0, 1.0, 1.0))
        a.set_actor_location(unreal.Vector(loc.x, loc.y, 0.0), False, False)
        a.set_actor_label(f"N_Car_{name}")
        swapped += 1
    les.save_current_level()
    unreal.log(f"WE_CARPATCH {level}: {swapped} cars swapped")


for lvl in ("/Game/WorkshopEmpire/Maps/L_Garage", "/Game/WorkshopEmpire/Maps/L_MainMenu"):
    patch(lvl)
unreal.SystemLibrary.quit_editor()
