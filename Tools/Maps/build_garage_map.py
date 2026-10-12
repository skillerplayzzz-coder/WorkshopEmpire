"""
Builds the Phase 1 placeholder level /Game/WorkshopEmpire/Maps/L_Garage from scratch.

Run (overwrites the map):
  UnrealEditor-Cmd.exe <WorkshopEmpire.uproject> -ExecutePythonScript="<this file>" -unattended -nullrhi -nosplash

Everything is a placeholder made from engine basic shapes; real assets come in Phase 15.
Coordinates: garage interior floor centre = origin, door in the +X wall, workbench at the back (-X).
"""
import unreal

MAP_PATH = "/Game/WorkshopEmpire/Maps/L_Garage"

les = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)

CUBE = unreal.load_asset("/Engine/BasicShapes/Cube.Cube")
CYLINDER = unreal.load_asset("/Engine/BasicShapes/Cylinder.Cylinder")
PLANE = unreal.load_asset("/Engine/BasicShapes/Plane.Plane")


def spawn(cls, loc, rot=(0.0, 0.0, 0.0), label=None, folder=None):
    actor = eas.spawn_actor_from_class(cls, unreal.Vector(*loc), unreal.Rotator(roll=rot[2], pitch=rot[0], yaw=rot[1]))
    if label:
        actor.set_actor_label(label)
    if folder:
        actor.set_folder_path(folder)
    return actor


def item(label, name, desc, kg, mesh, loc, scale, tint, rot=(0.0, 0.0, 0.0)):
    a = spawn(unreal.WEPickupItem, loc, rot, label, "Items")
    a.set_actor_scale3d(unreal.Vector(*scale))
    a.set_editor_property("item_mesh", mesh)
    a.set_editor_property("display_name", unreal.Text(name))
    a.set_editor_property("description", unreal.Text(desc))
    a.set_editor_property("weight_kg", kg)
    a.set_editor_property("tint", unreal.LinearColor(*tint, 1.0))
    return a


# --- Level -------------------------------------------------------------------------------------
if unreal.EditorAssetLibrary.does_asset_exist(MAP_PATH):
    # L_Garage is the editor startup map: switch away first, deleting a loaded world crashes the editor.
    les.load_level("/Engine/Maps/Templates/Template_Default")
    unreal.EditorAssetLibrary.delete_asset(MAP_PATH)
les.new_level(MAP_PATH, False)

# --- Environment & light -----------------------------------------------------------------------
sun = spawn(unreal.DirectionalLight, (0, 0, 1000), (-38.0, 35.0, 0.0), "Sun", "Lighting")
sun_comp = sun.get_component_by_class(unreal.DirectionalLightComponent)
sun_comp.set_editor_property("atmosphere_sun_light", True)
sun_comp.set_editor_property("mobility", unreal.ComponentMobility.MOVABLE)
sun_comp.set_intensity(8.0)

sky = spawn(unreal.SkyLight, (0, 0, 800), label="SkyLight", folder="Lighting")
sky_comp = sky.get_component_by_class(unreal.SkyLightComponent)
sky_comp.set_editor_property("mobility", unreal.ComponentMobility.MOVABLE)
sky_comp.set_editor_property("real_time_capture", True)

spawn(unreal.SkyAtmosphere, (0, 0, 0), label="SkyAtmosphere", folder="Lighting")
spawn(unreal.ExponentialHeightFog, (0, 0, -100), label="HeightFog", folder="Lighting")

ppv = spawn(unreal.PostProcessVolume, (0, 0, 0), label="PostProcess", folder="Lighting")
ppv.set_editor_property("unbound", True)
pp = ppv.get_editor_property("settings")
pp.set_editor_property("override_auto_exposure_bias", True)
pp.set_editor_property("auto_exposure_bias", -0.6)
ppv.set_editor_property("settings", pp)

for i, x in enumerate((-220.0, 160.0)):
    lamp = spawn(unreal.PointLight, (x, 0, 270), label=f"CeilingLamp{i + 1}", folder="Lighting")
    lc = lamp.get_component_by_class(unreal.PointLightComponent)
    lc.set_editor_property("mobility", unreal.ComponentMobility.MOVABLE)
    lc.set_intensity(30.0)
    lc.set_attenuation_radius(1200.0)
    lc.set_light_color(unreal.LinearColor(1.0, 0.86, 0.68, 1.0))

# --- Ground --------------------------------------------------------------------------------------
ground = spawn(unreal.StaticMeshActor, (0, 0, -2), label="Ground", folder="World")
ground.static_mesh_component.set_static_mesh(PLANE)
ground.static_mesh_component.set_material(0, unreal.load_asset("/Game/WorkshopEmpire/Materials/MI_WEGround.MI_WEGround"))
ground.set_actor_scale3d(unreal.Vector(120.0, 120.0, 1.0))

# --- Garage --------------------------------------------------------------------------------------
spawn(unreal.WEGarageBuilding, (0, 0, 0), label="Garage", folder="World")
spawn(unreal.WEGarageDoor, (410, 0, 0), label="GarageDoor", folder="World")

bench = spawn(unreal.WEPlaceholderProp, (-355, 0, 0), label="Workbench", folder="World")
bench.set_editor_property("prop_type", unreal.WEPlaceholderPropType.WORKBENCH)
bench.set_editor_property("size", unreal.Vector(80.0, 200.0, 90.0))
bench.set_editor_property("tint", unreal.LinearColor(0.45, 0.30, 0.18, 1.0))

shelf = spawn(unreal.WEPlaceholderProp, (-60, -272, 0), (0.0, 90.0, 0.0), "Shelf", "World")
shelf.set_editor_property("prop_type", unreal.WEPlaceholderPropType.SHELF)
shelf.set_editor_property("size", unreal.Vector(50.0, 180.0, 200.0))
shelf.set_editor_property("tint", unreal.LinearColor(0.30, 0.32, 0.35, 1.0))

# --- Items ---------------------------------------------------------------------------------------
item("Item_Computer", "Alter Computer",
     "Ein ausgedienter Büro-PC. Das Gehäuse ist verbeult, aber vielleicht steckt noch etwas Brauchbares drin.",
     9.0, CUBE, (-350, -40, 112), (0.45, 0.20, 0.42), (0.72, 0.69, 0.60))
item("Item_Monitor", "Röhrenmonitor",
     "Schwer, staubig und längst aus der Mode. Kupfer und Glas lassen sich vielleicht verwerten.",
     14.0, CUBE, (-355, 65, 110), (0.40, 0.40, 0.38), (0.15, 0.15, 0.16))
item("Item_Screwdriver", "Schraubendreher",
     "Ein einfacher Kreuzschlitz-Schraubendreher. Der Griff ist abgenutzt.",
     0.2, CYLINDER, (-335, 25, 93), (0.03, 0.03, 0.22), (0.75, 0.10, 0.08), (90.0, 0.0, 0.0))
item("Item_Toolbox", "Werkzeugkiste",
     "Eine Metallkiste für Handwerkzeug. Im Moment fast leer.",
     6.0, CUBE, (-290, -125, 12), (0.45, 0.22, 0.22), (0.10, 0.25, 0.65))
item("Item_Motor", "Alter Elektromotor",
     "Ein schwerer Drehstrommotor. Zum Tragen braucht es später einen Hubwagen oder Hilfe.",
     45.0, CYLINDER, (-285, 125, 23), (0.35, 0.35, 0.45), (0.25, 0.35, 0.28))
item("Item_CableBox", "Karton mit Kabeln",
     "Ein Karton voller alter Kabel. Gutes Kupfer, wenn man es abisoliert.",
     3.0, CUBE, (-60, -272, 56), (0.35, 0.30, 0.25), (0.55, 0.42, 0.26))

# --- Player starts (in front of the workbench, all items within reach) --------------------------
for i, y in enumerate((-120.0, -40.0, 40.0, 120.0)):
    spawn(unreal.PlayerStart, (-170, y, 100), (0.0, 180.0, 0.0), f"PlayerStart{i + 1}", "Players")

les.save_current_level()
unreal.log("WE_MAP_BUILT " + MAP_PATH)
unreal.SystemLibrary.quit_editor()
