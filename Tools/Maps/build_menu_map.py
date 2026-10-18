"""
Builds the main menu level /Game/WorkshopEmpire/Maps/L_MainMenu (Phase 13): the garage at dusk as a
backdrop, a camera, and the world setting "menu game mode" (AWEMenuGameMode shows the menu).

Run after build_placeholder_material.py (overwrites the map):
  UnrealEditor-Cmd.exe <WorkshopEmpire.uproject> -ExecutePythonScript="<this file>" -unattended -RenderOffscreen -nosplash
"""
import unreal

MAP_PATH = "/Game/WorkshopEmpire/Maps/L_MainMenu"

les = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
PLANE = unreal.load_asset("/Engine/BasicShapes/Plane.Plane")


def spawn(cls, loc, rot=(0.0, 0.0, 0.0), label=None):
    actor = eas.spawn_actor_from_class(cls, unreal.Vector(*loc), unreal.Rotator(roll=rot[2], pitch=rot[0], yaw=rot[1]))
    if label:
        actor.set_actor_label(label)
    return actor


if unreal.EditorAssetLibrary.does_asset_exist(MAP_PATH):
    les.load_level("/Engine/Maps/Templates/Template_Default")
    unreal.EditorAssetLibrary.delete_asset(MAP_PATH)
les.new_level(MAP_PATH, False)

# Evening light.
sun = spawn(unreal.DirectionalLight, (0, 0, 1000), (-12.0, 200.0, 0.0), "Sun")
sun_comp = sun.get_component_by_class(unreal.DirectionalLightComponent)
sun_comp.set_editor_property("atmosphere_sun_light", True)
sun_comp.set_editor_property("mobility", unreal.ComponentMobility.MOVABLE)
sun_comp.set_intensity(40000.0)
sun_comp.set_light_color(unreal.LinearColor(1.0, 0.72, 0.5, 1.0))
sky = spawn(unreal.SkyLight, (0, 0, 800), label="SkyLight")
sky_comp = sky.get_component_by_class(unreal.SkyLightComponent)
sky_comp.set_editor_property("mobility", unreal.ComponentMobility.MOVABLE)
sky_comp.set_editor_property("real_time_capture", True)
spawn(unreal.SkyAtmosphere, (0, 0, 0), label="SkyAtmosphere")
spawn(unreal.ExponentialHeightFog, (0, 0, -100), label="HeightFog")

ground = spawn(unreal.StaticMeshActor, (0, 0, -2), label="Ground")
ground.static_mesh_component.set_static_mesh(PLANE)
asphalt = "/Game/WorkshopEmpire/Materials/Surfaces/MI_WEAsphalt"
ground.static_mesh_component.set_material(0, unreal.load_asset(asphalt if unreal.EditorAssetLibrary.does_asset_exist(asphalt) else "/Game/WorkshopEmpire/Materials/MI_WEGround.MI_WEGround"))
ground.set_actor_scale3d(unreal.Vector(120.0, 120.0, 1.0))

# The garage with its door, a yard lamp and some clutter in the yard.
spawn(unreal.WEGarageBuilding, (0, 0, 0), label="Garage")
spawn(unreal.WEGarageDoor, (410, 0, 0), label="GarageDoor")
lamp = spawn(unreal.PointLight, (520, 0, 300), label="YardLamp")
lc = lamp.get_component_by_class(unreal.PointLightComponent)
lc.set_editor_property("mobility", unreal.ComponentMobility.MOVABLE)
lc.set_intensity(60.0)
lc.set_attenuation_radius(1400.0)
lc.set_light_color(unreal.LinearColor(1.0, 0.82, 0.6, 1.0))


def prop(name, loc, yaw=0.0):
    """Phase 15: CC0 model if imported, otherwise nothing (the backdrop still works without)."""
    path = f"/Game/WorkshopEmpire/Art/Models/SM_{name}"
    if not unreal.EditorAssetLibrary.does_asset_exist(path):
        return None
    a = spawn(unreal.StaticMeshActor, loc, (0.0, yaw, 0.0), f"Deco_{name}")
    a.static_mesh_component.set_static_mesh(unreal.load_asset(path))
    return a


prop("wooden_crate_01", (620, 260, 0), 12.0)
prop("wooden_crate_01", (690, 340, 0), -20.0)
prop("cardboard_box_01", (640, 255, 52), 30.0)
prop("Barrel_01", (560, -300, 0))
prop("barrel_03", (610, -350, 0), 40.0)
prop("old_tyre", (780, -240, 30), 0.0)
prop("old_tyre", (780, -240, 47), 25.0)
prop("portable_welding_cart", (455, -265, 0), 90.0)
prop("old_military_compressor", (455, 265, 0), 90.0)
prop("industrial_wall_lamp", (424, -245, 255))
prop("industrial_wall_lamp", (424, 245, 255))
prop("plastic_crate_01", (720, 150, 0), 15.0)
prop("metal_jerrycan", (600, -400, 0), 0.0)
# Warm light spilling out of the workshop.
inside = spawn(unreal.PointLight, (200, 0, 250), label="InsideLamp")
ic = inside.get_component_by_class(unreal.PointLightComponent)
ic.set_editor_property("mobility", unreal.ComponentMobility.MOVABLE)
ic.set_intensity(80.0)
ic.set_attenuation_radius(900.0)
ic.set_light_color(unreal.LinearColor(1.0, 0.8, 0.55, 1.0))
# Phase 17.5: the same neighbourhood as in the game (street, power lines, fence, neighbours).
import os, sys
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import world_neighbourhood
world_neighbourhood.build_all(menu=True)

ppv = spawn(unreal.PostProcessVolume, (0, 0, 0), label="PostProcess")
ppv.set_editor_property("unbound", True)
pp = ppv.get_editor_property("settings")
for name, value in (("vignette_intensity", 0.45), ("bloom_intensity", 0.5), ("film_grain_intensity", 0.05)):
    pp.set_editor_property("override_" + name, True)
    pp.set_editor_property(name, value)
ppv.set_editor_property("settings", pp)
cam = spawn(unreal.CameraActor, (1150, -520, 210), (-6.0, 152.0, 0.0), "MenuCamera")
cam.camera_component.set_editor_property("field_of_view", 70.0)
cam.camera_component.set_editor_property("constrain_aspect_ratio", False)  # no black bars at 4:3 or 21:9

world = unreal.EditorLevelLibrary.get_editor_world()
world.get_world_settings().set_editor_property("default_game_mode", unreal.WEMenuGameMode)

les.save_current_level()
unreal.log("WE_MENU_BUILT " + MAP_PATH)
unreal.SystemLibrary.quit_editor()
