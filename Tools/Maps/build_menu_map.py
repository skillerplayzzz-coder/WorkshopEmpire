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
CUBE = unreal.load_asset("/Engine/BasicShapes/Cube.Cube")
PLANE = unreal.load_asset("/Engine/BasicShapes/Plane.Plane")


def spawn(cls, loc, rot=(0.0, 0.0, 0.0), label=None):
    actor = eas.spawn_actor_from_class(cls, unreal.Vector(*loc), unreal.Rotator(roll=rot[2], pitch=rot[0], yaw=rot[1]))
    if label:
        actor.set_actor_label(label)
    return actor


def box(label, loc, size, yaw=0.0):
    a = spawn(unreal.StaticMeshActor, loc, (0.0, yaw, 0.0), label)
    a.static_mesh_component.set_static_mesh(CUBE)
    a.set_actor_scale3d(unreal.Vector(size[0] / 100.0, size[1] / 100.0, size[2] / 100.0))
    return a


if unreal.EditorAssetLibrary.does_asset_exist(MAP_PATH):
    les.load_level("/Engine/Maps/Templates/Template_Default")
    unreal.EditorAssetLibrary.delete_asset(MAP_PATH)
les.new_level(MAP_PATH, False)

# Evening light.
sun = spawn(unreal.DirectionalLight, (0, 0, 1000), (-12.0, 200.0, 0.0), "Sun")
sun_comp = sun.get_component_by_class(unreal.DirectionalLightComponent)
sun_comp.set_editor_property("atmosphere_sun_light", True)
sun_comp.set_editor_property("mobility", unreal.ComponentMobility.MOVABLE)
sun_comp.set_intensity(4.0)
sun_comp.set_light_color(unreal.LinearColor(1.0, 0.72, 0.5, 1.0))
sky = spawn(unreal.SkyLight, (0, 0, 800), label="SkyLight")
sky_comp = sky.get_component_by_class(unreal.SkyLightComponent)
sky_comp.set_editor_property("mobility", unreal.ComponentMobility.MOVABLE)
sky_comp.set_editor_property("real_time_capture", True)
spawn(unreal.SkyAtmosphere, (0, 0, 0), label="SkyAtmosphere")
spawn(unreal.ExponentialHeightFog, (0, 0, -100), label="HeightFog")

ground = spawn(unreal.StaticMeshActor, (0, 0, -2), label="Ground")
ground.static_mesh_component.set_static_mesh(PLANE)
ground.static_mesh_component.set_material(0, unreal.load_asset("/Game/WorkshopEmpire/Materials/MI_WEGround.MI_WEGround"))
ground.set_actor_scale3d(unreal.Vector(80.0, 80.0, 1.0))

# The garage with its door, a warm lamp inside and a few crates and pallets in the yard.
spawn(unreal.WEGarageBuilding, (0, 0, 0), label="Garage")
spawn(unreal.WEGarageDoor, (410, 0, 0), label="GarageDoor")
lamp = spawn(unreal.PointLight, (520, 0, 300), label="YardLamp")
lc = lamp.get_component_by_class(unreal.PointLightComponent)
lc.set_editor_property("mobility", unreal.ComponentMobility.MOVABLE)
lc.set_intensity(60.0)
lc.set_attenuation_radius(1400.0)
lc.set_light_color(unreal.LinearColor(1.0, 0.82, 0.6, 1.0))
box("Pallet", (620, 260, 7), (120, 100, 14))
box("Crate1", (620, 260, 50), (70, 60, 70), 12.0)
box("Crate2", (690, 330, 35), (60, 50, 50), -20.0)
box("Barrel", (560, -300, 50), (55, 55, 100))
box("Scrap1", (760, -260, 20), (90, 70, 40), 30.0)
box("Scrap2", (790, -180, 15), (50, 40, 30), -10.0)

cam = spawn(unreal.CameraActor, (1150, -520, 210), (-6.0, 152.0, 0.0), "MenuCamera")
cam.camera_component.set_editor_property("field_of_view", 70.0)

world = unreal.EditorLevelLibrary.get_editor_world()
world.get_world_settings().set_editor_property("default_game_mode", unreal.WEMenuGameMode)

les.save_current_level()
unreal.log("WE_MENU_BUILT " + MAP_PATH)
unreal.SystemLibrary.quit_editor()
