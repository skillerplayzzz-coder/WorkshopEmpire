"""
Builds the Phase 1 placeholder level /Game/WorkshopEmpire/Maps/L_Garage from scratch.

Run after build_placeholder_material.py, build_audio.py, build_item_definitions.py, build_procurement.py, build_economy.py, build_progression.py, build_machines.py and build_workshop.py (overwrites the map):
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


def item(label, asset, loc, quantity=1, condition=unreal.WEItemCondition.USED, quality=50, rot=(0.0, 0.0, 0.0)):
    """Places a world item for the item definition Data Asset DA_Item_<asset>."""
    a = spawn(unreal.WEPickupItem, loc, rot, label, "Items")
    inst = unreal.WEItemInstance()
    inst.set_editor_property("definition", unreal.load_asset(f"/Game/WorkshopEmpire/Data/Items/DA_Item_{asset}"))
    inst.set_editor_property("quantity", quantity)
    inst.set_editor_property("condition", condition)
    inst.set_editor_property("quality", quality)
    a.set_editor_property("item", inst)
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
    lamp.set_editor_property("tags", ["WE_WorkshopLight"])  # brightened by the LED upgrade

# --- Ground --------------------------------------------------------------------------------------
ground = spawn(unreal.StaticMeshActor, (0, 0, -2), label="Ground", folder="World")
ground.static_mesh_component.set_static_mesh(PLANE)
ground.static_mesh_component.set_material(0, unreal.load_asset("/Game/WorkshopEmpire/Materials/MI_WEGround.MI_WEGround"))
ground.set_actor_scale3d(unreal.Vector(120.0, 120.0, 1.0))

# --- Garage --------------------------------------------------------------------------------------
spawn(unreal.WEGarageBuilding, (0, 0, 0), label="Garage", folder="World")
spawn(unreal.WEGarageDoor, (410, 0, 0), label="GarageDoor", folder="World")

bench = spawn(unreal.WEWorkbench, (-355, 0, 0), label="Workbench", folder="World")
bench.set_editor_property("prop_type", unreal.WEPlaceholderPropType.WORKBENCH)
bench.set_editor_property("size", unreal.Vector(80.0, 200.0, 90.0))
bench.set_editor_property("tint", unreal.LinearColor(0.45, 0.30, 0.18, 1.0))
bench.set_editor_property("has_storage", True)
bench.set_editor_property("storage_name", unreal.Text("Werkbank"))
bench.set_editor_property("storage_slots", 12)
bench.set_editor_property("storage_max_weight_kg", 60.0)

shelf = spawn(unreal.WEPlaceholderProp, (-60, -272, 0), (0.0, 90.0, 0.0), "Shelf", "World")
shelf.set_editor_property("prop_type", unreal.WEPlaceholderPropType.SHELF)
shelf.set_editor_property("size", unreal.Vector(50.0, 180.0, 200.0))
shelf.set_editor_property("tint", unreal.LinearColor(0.30, 0.32, 0.35, 1.0))
shelf.set_editor_property("has_storage", True)
shelf.set_editor_property("storage_name", unreal.Text("Regal"))
shelf.set_editor_property("storage_slots", 15)
shelf.set_editor_property("storage_max_weight_kg", 150.0)

# Second shelf, hidden until the "SecondShelf" upgrade is bought (Phase 5).
shelf2 = spawn(unreal.WEPlaceholderProp, (150, -272, 0), (0.0, 90.0, 0.0), "Shelf2", "World")
shelf2.set_editor_property("prop_type", unreal.WEPlaceholderPropType.SHELF)
shelf2.set_editor_property("size", unreal.Vector(50.0, 180.0, 200.0))
shelf2.set_editor_property("tint", unreal.LinearColor(0.20, 0.35, 0.45, 1.0))
shelf2.set_editor_property("has_storage", True)
shelf2.set_editor_property("storage_name", unreal.Text("Regal 2"))
shelf2.set_editor_property("storage_slots", 15)
shelf2.set_editor_property("storage_max_weight_kg", 150.0)
shelf2.set_editor_property("required_upgrade", "SecondShelf")
# --- Items (definitions: Tools/Maps/build_item_definitions.py) -----------------------------------
COND = unreal.WEItemCondition
# Bulky things to disassemble stand on the floor; the left half of the bench is the work spot.
item("Item_Computer", "OldComputer", (-270, -70, 22), condition=COND.DEFECTIVE, quality=35)
item("Item_Monitor", "CrtMonitor", (-262, 75, 20), condition=COND.DAMAGED, quality=40)
item("Item_Toolbox", "Toolbox", (-290, -150, 12), condition=COND.USED, quality=60)
item("Item_Motor", "ElectricMotor", (-200, 175, 23), condition=COND.DEFECTIVE, quality=30)
item("Item_CableBox", "CableBox", (-60, -272, 56), condition=COND.SCRAP, quality=20)
# Small parts and tools on the right half of the bench.
item("Item_Screwdriver", "Screwdriver", (-335, 15, 93), condition=COND.USED, quality=55, rot=(90.0, 0.0, 0.0))
item("Item_SideCutter", "SideCutter", (-330, 42, 92), condition=COND.USED, quality=60)
item("Item_CopperCable", "CopperCable", (-368, 30, 93), quantity=6, condition=COND.USED, quality=60, rot=(0.0, 0.0, 90.0))
item("Item_Ram", "RamModule", (-370, 65, 92), condition=COND.USED, quality=50)
item("Item_Screws", "SheetMetalScrews", (-330, 85, 93), quantity=50, condition=COND.VERY_GOOD, quality=70)
# --- Market terminal and delivery pallet (Phase 4) ---------------------------------------------------
terminal = spawn(unreal.WETraderTerminal, (-150, 262, 0), (0.0, -90.0, 0.0), "MarketTerminal", "World")
terminal.set_editor_property("traders", [
    unreal.load_asset("/Game/WorkshopEmpire/Data/Economy/DA_Trader_Scrapyard"),
    unreal.load_asset("/Game/WorkshopEmpire/Data/Economy/DA_Trader_UsedGoods"),
    unreal.load_asset("/Game/WorkshopEmpire/Data/Economy/DA_Trader_ToolShop"),
    unreal.load_asset("/Game/WorkshopEmpire/Data/Economy/DA_Trader_SpareParts"),
    unreal.load_asset("/Game/WorkshopEmpire/Data/Economy/DA_Trader_Returns"),
])
spawn(unreal.WEDeliveryZone, (320, -220, 6), label="DeliveryPallet", folder="World")
# --- Build area: furniture may be placed anywhere inside the garage (Phase 8) ---------------------------
spawn(unreal.WEBuildArea, (0, 0, 150), label="BuildArea_Garage", folder="World")
# --- Scrap heap outside, to the side of the door (Phase 7) ---------------------------------------------
spawn(unreal.WEScrapPile, (760, 260, 0), label="ScrapPile", folder="World")
# --- Staff and logistics (Phase 11) --------------------------------------------------------------
# New employees appear inside the door.
staff_spawn = spawn(unreal.TargetPoint, (300, 60, 100), label="StaffSpawn", folder="Staff")
staff_spawn.set_editor_property("tags", ["WE_StaffSpawn"])
# Company van outside the door, forklift in the corner by the bench (both hidden until bought).
spawn(unreal.WEVehicle, (760, -200, 0), label="CompanyVan", folder="Logistics")
spawn(unreal.WEForklift, (-290, -215, 0), label="Forklift", folder="Logistics")
# Navigation for employees (rebuilt at runtime when furniture moves, see DefaultEngine.ini).
nav = spawn(unreal.NavMeshBoundsVolume, (1000, 700, 150), label="NavBounds", folder="Staff")
nav.set_actor_scale3d(unreal.Vector(23.0, 21.0, 4.0))  # garage, yard and all buildings of Phase 12
# --- Expansion (Phase 12): buildings appear with their upgrade (actors tagged "WEUpgrade.<Id>") ---------
def building(upgrade_id, label, center, yaw, depth, width, height, door_width, door_height, wall_color, area_half, lamps):
    tag = f"WEUpgrade.{upgrade_id}"
    b = spawn(unreal.WEGarageBuilding, center, (0.0, yaw, 0.0), label, "Expansion")
    b.set_editor_property("inner_depth", depth)
    b.set_editor_property("inner_width", width)
    b.set_editor_property("inner_height", height)
    b.set_editor_property("door_width", door_width)
    b.set_editor_property("door_height", door_height)
    b.set_editor_property("wall_color", unreal.LinearColor(*wall_color, 1.0))
    b.set_editor_property("tags", [tag])
    # The door opening is in the local +X wall.
    rad = 3.14159265 * yaw / 180.0
    import math
    dx = math.cos(rad) * (depth * 0.5 + 10.0)
    dy = math.sin(rad) * (depth * 0.5 + 10.0)
    d = spawn(unreal.WEGarageDoor, (center[0] + dx, center[1] + dy, 0), (0.0, yaw, 0.0), f"{label}Door", "Expansion")
    d.set_editor_property("door_width", door_width)
    d.set_editor_property("door_height", door_height)
    d.set_editor_property("tags", [tag])
    a = spawn(unreal.WEBuildArea, (center[0], center[1], 150), label=f"{label}BuildArea", folder="Expansion")
    a.set_editor_property("half_size", unreal.Vector2D(*area_half))
    a.set_editor_property("tags", [tag])
    for i, (lx, ly) in enumerate(lamps):
        lamp = spawn(unreal.PointLight, (lx, ly, height - 30), label=f"{label}Lamp{i + 1}", folder="Expansion")
        lc = lamp.get_component_by_class(unreal.PointLightComponent)
        lc.set_editor_property("mobility", unreal.ComponentMobility.MOVABLE)
        lc.set_intensity(40.0)
        lc.set_attenuation_radius(1600.0)
        lc.set_light_color(unreal.LinearColor(1.0, 0.9, 0.75, 1.0))
        lamp.set_editor_property("tags", [tag, "WE_WorkshopLight"])


# Werkstatt-Erweiterung: next to the garage (-Y), door to the yard like the garage.
building("WorkshopExtension", "WorkshopExtension", (0, -760, 0), 0.0, 800.0, 700.0, 320.0, 400.0, 260.0, (0.34, 0.33, 0.30),
         (395.0, 345.0), [(-200, -760), (200, -760)])
# Gewerbehalle: across the yard, door facing the garage (-X).
building("CommercialHall", "CommercialHall", (1700, -200, 0), 180.0, 1400.0, 1400.0, 600.0, 500.0, 450.0, (0.30, 0.34, 0.38),
         (695.0, 695.0), [(1400, -500), (2000, -500), (1400, 100), (2000, 100)])
# Fabrikhalle: next to the commercial hall (+Y), door to the yard as well.
building("FactoryHall", "FactoryHall", (1700, 1650, 0), 180.0, 1400.0, 1600.0, 700.0, 600.0, 500.0, (0.32, 0.30, 0.28),
         (695.0, 795.0), [(1400, 1300), (2000, 1300), (1400, 2000), (2000, 2000)])
# The navmesh itself is built at runtime (RuntimeGeneration=Dynamic); its actor must exist in the level.
navmesh = spawn(unreal.RecastNavMesh, (0, 0, 0), label="RecastNavMesh-Default", folder="Staff")
navmesh.set_editor_property("runtime_generation", unreal.RuntimeGenerationType.DYNAMIC)
navmesh.set_editor_property("agent_radius", 34.0)
# --- Player starts (in front of the workbench, all items within reach) --------------------------
for i, y in enumerate((-120.0, -40.0, 40.0, 120.0)):
    spawn(unreal.PlayerStart, (-170, y, 100), (0.0, 180.0, 0.0), f"PlayerStart{i + 1}", "Players")

les.save_current_level()
unreal.log("WE_MAP_BUILT " + MAP_PATH)
unreal.SystemLibrary.quit_editor()
