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
# Late-afternoon sun from the yard side, so it shines through the open door into the workshop.
sun = spawn(unreal.DirectionalLight, (0, 0, 1000), (-32.0, 205.0, 0.0), "Sun", "Lighting")
sun_comp = sun.get_component_by_class(unreal.DirectionalLightComponent)
sun_comp.set_editor_property("atmosphere_sun_light", True)
sun_comp.set_editor_property("mobility", unreal.ComponentMobility.MOVABLE)
sun_comp.set_intensity(75000.0)  # physical daylight (lux); exposure adapts between yard and workshop

sky = spawn(unreal.SkyLight, (0, 0, 800), label="SkyLight", folder="Lighting")
sky_comp = sky.get_component_by_class(unreal.SkyLightComponent)
sky_comp.set_editor_property("mobility", unreal.ComponentMobility.MOVABLE)
sky_comp.set_editor_property("real_time_capture", True)

spawn(unreal.SkyAtmosphere, (0, 0, 0), label="SkyAtmosphere", folder="Lighting")
# Phase 15: volumetric clouds and fog (light shafts through the open door).
spawn(unreal.VolumetricCloud, (0, 0, 0), label="VolumetricCloud", folder="Lighting")
fog = spawn(unreal.ExponentialHeightFog, (0, 0, -100), label="HeightFog", folder="Lighting")
fog_comp = fog.get_component_by_class(unreal.ExponentialHeightFogComponent)
fog_comp.set_editor_property("fog_density", 0.012)
fog_comp.set_editor_property("fog_height_falloff", 0.35)
fog_comp.set_volumetric_fog(True)
fog_comp.set_editor_property("volumetric_fog_scattering_distribution", 0.6)
fog_comp.set_editor_property("volumetric_fog_extinction_scale", 0.6)
sun_comp.set_editor_property("volumetric_scattering_intensity", 1.6)

def pp_set(settings, name, value):
    settings.set_editor_property("override_" + name, True)
    settings.set_editor_property(name, value)

ppv = spawn(unreal.PostProcessVolume, (0, 0, 0), label="PostProcess", folder="Lighting")
ppv.set_editor_property("unbound", True)
pp = ppv.get_editor_property("settings")
# Exposure: the eye adapts between the lamp-lit workshop and the bright yard, but not to extremes.
pp_set(pp, "auto_exposure_method", unreal.AutoExposureMethod.AEM_HISTOGRAM)
pp_set(pp, "auto_exposure_min_brightness", 3.0)
pp_set(pp, "auto_exposure_max_brightness", 15.0)
pp_set(pp, "auto_exposure_bias", 0.4)
pp_set(pp, "auto_exposure_speed_up", 2.0)
pp_set(pp, "auto_exposure_speed_down", 1.5)
pp_set(pp, "bloom_intensity", 0.35)
pp_set(pp, "bloom_threshold", 1.0)
pp_set(pp, "vignette_intensity", 0.32)
pp_set(pp, "film_grain_intensity", 0.04)
pp_set(pp, "ambient_occlusion_intensity", 0.65)
pp_set(pp, "ambient_occlusion_radius", 120.0)
pp_set(pp, "lumen_scene_lighting_quality", 1.0)
pp_set(pp, "lumen_final_gather_quality", 1.5)
pp_set(pp, "white_temp", 6200.0)
pp_set(pp, "color_contrast", unreal.Vector4(1.04, 1.04, 1.04, 1.0))
pp_set(pp, "color_saturation", unreal.Vector4(0.95, 0.95, 0.95, 1.0))
pp_set(pp, "scene_fringe_intensity", 0.0)
# Local exposure keeps the sunlit floor patches by the door from burning out while the eye is adapted to the workshop.
pp_set(pp, "local_exposure_highlight_contrast_scale", 0.6)
pp_set(pp, "local_exposure_shadow_contrast_scale", 0.85)
ppv.set_editor_property("settings", pp)

# Phase 15: fluorescent fixtures - a glowing tube body and a rect light shining down (realistic lumens).
EMISSIVE = unreal.load_asset("/Game/WorkshopEmpire/Materials/FX/M_WEEmissive")
CUBE_MESH = unreal.load_asset("/Engine/BasicShapes/Cube.Cube")


def tube_fixture(label, loc, length=130.0, lumens=2200.0, tags=("WE_WorkshopLight",), folder="Lighting"):
    housing = spawn(unreal.StaticMeshActor, (loc[0], loc[1], loc[2] + 4), label=label + "Housing", folder=folder)
    housing.static_mesh_component.set_static_mesh(CUBE_MESH)
    housing.set_actor_scale3d(unreal.Vector(length / 100.0, 0.11, 0.05))
    metal = "/Game/WorkshopEmpire/Materials/Surfaces/MI_WEPaintedMetal"
    housing.static_mesh_component.set_material(0, unreal.load_asset(metal if unreal.EditorAssetLibrary.does_asset_exist(metal) else "/Game/WorkshopEmpire/Materials/M_WEPlaceholder"))
    housing.static_mesh_component.set_editor_property("cast_shadow", False)
    tube = spawn(unreal.StaticMeshActor, (loc[0], loc[1], loc[2] - 1), label=label + "Tube", folder=folder)
    tube.static_mesh_component.set_static_mesh(CUBE_MESH)
    tube.set_actor_scale3d(unreal.Vector((length - 10) / 100.0, 0.06, 0.03))
    tube.static_mesh_component.set_material(0, EMISSIVE)
    tube.static_mesh_component.set_editor_property("cast_shadow", False)
    tube.static_mesh_component.set_collision_enabled(unreal.CollisionEnabled.NO_COLLISION)
    housing.static_mesh_component.set_collision_enabled(unreal.CollisionEnabled.NO_COLLISION)
    lamp = spawn(unreal.RectLight, (loc[0], loc[1], loc[2] - 4), (-90.0, 0.0, 0.0), label, folder)
    lc = lamp.get_component_by_class(unreal.RectLightComponent)
    lc.set_editor_property("mobility", unreal.ComponentMobility.MOVABLE)
    lc.set_editor_property("intensity_units", unreal.LightUnits.LUMENS)
    lc.set_intensity(lumens)
    lc.set_editor_property("source_width", length - 10)
    lc.set_editor_property("source_height", 8.0)
    lc.set_editor_property("barn_door_angle", 88.0)
    lc.set_attenuation_radius(1400.0)
    lc.set_light_color(unreal.LinearColor(1.0, 0.92, 0.82, 1.0))
    lamp.set_editor_property("tags", list(tags))  # brightened by the LED upgrade
    # A bare tube also shines sideways and up: a weak, shadowless fill keeps the ceiling from going black.
    up = spawn(unreal.PointLight, (loc[0], loc[1], loc[2] - 2), label=label + "Fill", folder=folder)
    uc = up.get_component_by_class(unreal.PointLightComponent)
    uc.set_editor_property("mobility", unreal.ComponentMobility.MOVABLE)
    uc.set_editor_property("intensity_units", unreal.LightUnits.LUMENS)
    uc.set_intensity(lumens * 0.12)
    uc.set_editor_property("source_length", length - 10)
    uc.set_editor_property("cast_shadows", False)
    uc.set_attenuation_radius(700.0)
    uc.set_light_color(unreal.LinearColor(1.0, 0.92, 0.82, 1.0))
    up.set_editor_property("tags", list(tags))
    for part in (housing, tube):
        part.set_editor_property("tags", [t for t in tags if t != "WE_WorkshopLight"])
    return lamp


for i, x in enumerate((-220.0, 160.0)):
    tube_fixture(f"CeilingLamp{i + 1}", (x, 0, 292))

# --- Ground --------------------------------------------------------------------------------------
ground = spawn(unreal.StaticMeshActor, (0, 0, -2), label="Ground", folder="World")
ground.static_mesh_component.set_static_mesh(PLANE)
# Phase 15: asphalt yard (falls back to the old flat ground before the textures are imported).
asphalt = "/Game/WorkshopEmpire/Materials/Surfaces/MI_WEAsphalt"
ground.static_mesh_component.set_material(0, unreal.load_asset(asphalt) if unreal.EditorAssetLibrary.does_asset_exist(asphalt) else unreal.load_asset("/Game/WorkshopEmpire/Materials/MI_WEGround.MI_WEGround"))
ground.set_actor_scale3d(unreal.Vector(900.0, 900.0, 1.0))
# Phase 17: the plane has no thickness - small items dropped fast could fall through it (seen in the multiplayer
# stress test). An invisible 1-m slab below catches everything; the world kills nothing it does not have to.
ground_slab = spawn(unreal.StaticMeshActor, (0, 0, -52), label="GroundCollision", folder="World")
ground_slab.static_mesh_component.set_static_mesh(CUBE_MESH)
ground_slab.set_actor_scale3d(unreal.Vector(900.0, 900.0, 1.0))
ground_slab.static_mesh_component.set_visibility(False)
ground_slab.static_mesh_component.set_editor_property("cast_shadow", False)
ground_slab.set_actor_hidden_in_game(True)

# --- Garage --------------------------------------------------------------------------------------
spawn(unreal.WEGarageBuilding, (0, 0, 0), label="Garage", folder="World")
spawn(unreal.WEGarageDoor, (410, 0, 0), label="GarageDoor", folder="World")

bench = spawn(unreal.WEWorkbench, (-355, 0, 0), label="Workbench", folder="World")
bench.set_editor_property("prop_type", unreal.WEPlaceholderPropType.WORKBENCH)
bench.set_editor_property("size", unreal.Vector(80.0, 200.0, 90.0))
bench.set_editor_property("tint", unreal.LinearColor(0.45, 0.30, 0.18, 1.0))
bench.set_editor_property("has_storage", True)
bench.set_editor_property("storage_name", unreal.Text("Werkbank"))
bench.set_editor_property("storage_slots", 24)
bench.set_editor_property("storage_max_weight_kg", 100.0)

shelf = spawn(unreal.WEPlaceholderProp, (-60, -272, 0), (0.0, 90.0, 0.0), "Shelf", "World")
shelf.set_editor_property("prop_type", unreal.WEPlaceholderPropType.SHELF)
shelf.set_editor_property("size", unreal.Vector(50.0, 180.0, 200.0))
shelf.set_editor_property("tint", unreal.LinearColor(0.30, 0.32, 0.35, 1.0))
shelf.set_editor_property("has_storage", True)
shelf.set_editor_property("storage_name", unreal.Text("Regal"))
shelf.set_editor_property("storage_slots", 15)
shelf.set_editor_property("storage_max_weight_kg", 150.0)


def model(name):
    path = f"/Game/WorkshopEmpire/Art/Models/SM_{name}"
    return unreal.load_asset(path) if unreal.EditorAssetLibrary.does_asset_exist(path) else None


# Phase 15: real steel racks (two side by side) instead of the box shelves.
for s_ in (shelf,):
    s_.set_editor_property("visual_mesh", model("worn_metal_rack"))
    s_.set_editor_property("visual_yaw", 90.0)
    s_.set_editor_property("tile_visual", True)

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
shelf2.set_editor_property("visual_mesh", model("steel_frame_shelves_02"))
shelf2.set_editor_property("visual_yaw", 90.0)
shelf2.set_editor_property("tile_visual", True)
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
# Phase 19: wall mirror (see yourself, change clothes and face with [E]).
spawn(unreal.WEMirror, (230, 300, 45), (0.0, -90.0, 0.0), "Mirror", "World")
# --- Build area: furniture may be placed anywhere inside the garage (Phase 8) ---------------------------
spawn(unreal.WEBuildArea, (0, 0, 150), label="BuildArea_Garage", folder="World")
# --- Scrap heap outside, to the side of the door (Phase 7) ---------------------------------------------
spawn(unreal.WEScrapPile, (760, 260, 0), label="ScrapPile", folder="World")
# --- Staff and logistics (Phase 11) --------------------------------------------------------------
# New employees appear inside the door.
staff_spawn = spawn(unreal.TargetPoint, (300, 60, 100), label="StaffSpawn", folder="Staff")
staff_spawn.set_editor_property("tags", ["WE_StaffSpawn"])
# Company van outside the door, forklift in the corner by the bench (both hidden until bought).
# Phase 17.5: the van parks nose to the street, so it drives out through the lot gate.
spawn(unreal.WEVehicle, (780, -560, 0), (0.0, -90.0, 0.0), label="CompanyVan", folder="Logistics")
# Phase 19: moved out of the corner (it stood in the toolbox and the utility box).
spawn(unreal.WEForklift, (-110, -195, 0), label="Forklift", folder="Logistics")
# Navigation for employees (rebuilt at runtime when furniture moves, see DefaultEngine.ini).
nav = spawn(unreal.NavMeshBoundsVolume, (1000, 700, 150), label="NavBounds", folder="Staff")
nav.set_actor_scale3d(unreal.Vector(23.0, 21.0, 4.0))  # garage, yard and all buildings of Phase 12
# --- Expansion (Phase 12): buildings appear with their upgrade (actors tagged "WEUpgrade.<Id>") ---------
def building(upgrade_id, label, center, yaw, depth, width, height, door_width, door_height, wall_color, area_half, lamps, surfaces=None):
    tag = f"WEUpgrade.{upgrade_id}"
    b = spawn(unreal.WEGarageBuilding, center, (0.0, yaw, 0.0), label, "Expansion")
    for prop, kind in (surfaces or {}).items():
        b.set_editor_property(prop, kind)
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
        # Hall fixtures hang lower than the roof and are brighter (Phase 15).
        tube_fixture(f"{label}Lamp{i + 1}", (lx, ly, min(height - 8, 420)), length=150.0, lumens=4500.0, tags=(tag, "WE_WorkshopLight"), folder="Expansion")


# Werkstatt-Erweiterung: next to the garage (-Y), door to the yard like the garage.
building("WorkshopExtension", "WorkshopExtension", (0, -760, 0), 0.0, 800.0, 700.0, 320.0, 400.0, 260.0, (0.34, 0.33, 0.30),
         (395.0, 345.0), [(-200, -760), (200, -760)], surfaces={"wall_surface": unreal.WESurface.BRICK, "interior_surface": unreal.WESurface.PAINTED_WALL})
# Gewerbehalle: across the yard, door facing the garage (-X).
building("CommercialHall", "CommercialHall", (1700, -200, 0), 180.0, 1400.0, 1400.0, 600.0, 500.0, 450.0, (0.30, 0.34, 0.38),
         (695.0, 695.0), [(1400, -500), (2000, -500), (1400, 100), (2000, 100)], surfaces={"wall_surface": unreal.WESurface.CORRUGATED, "interior_surface": unreal.WESurface.CONCRETE_WALL, "ceiling_surface": unreal.WESurface.ROOF_METAL})
# Fabrikhalle: next to the commercial hall (+Y), door to the yard as well.
building("FactoryHall", "FactoryHall", (1700, 1650, 0), 180.0, 1400.0, 1600.0, 700.0, 600.0, 500.0, (0.32, 0.30, 0.28),
         (695.0, 795.0), [(1400, 1300), (2000, 1300), (1400, 2000), (2000, 2000)], surfaces={"wall_surface": unreal.WESurface.FACTORY_WALL, "interior_surface": unreal.WESurface.BLOCK_WALL, "ceiling_surface": unreal.WESurface.ROOF_METAL})
# The navmesh itself is built at runtime (RuntimeGeneration=Dynamic); its actor must exist in the level.
navmesh = spawn(unreal.RecastNavMesh, (0, 0, 0), label="RecastNavMesh-Default", folder="Staff")
navmesh.set_editor_property("runtime_generation", unreal.RuntimeGenerationType.DYNAMIC)
navmesh.set_editor_property("agent_radius", 34.0)
# --- Player starts (in front of the workbench, all items within reach) --------------------------
for i, y in enumerate((-120.0, -40.0, 40.0, 120.0)):
    spawn(unreal.PlayerStart, (-170, y, 100), (0.0, 180.0, 0.0), f"PlayerStart{i + 1}", "Players")

# --- Decoration (Phase 15, CC0 models): along the walls, out of the way of play ----------------------
def prop_model(name, loc, yaw=0.0, label=None, folder="Decoration", scale=1.0, collide=True):
    mesh = model(name)
    if not mesh:
        return None
    a = spawn(unreal.StaticMeshActor, loc, (0.0, yaw, 0.0), label or f"Deco_{name}", folder)
    a.static_mesh_component.set_static_mesh(mesh)
    a.set_actor_scale3d(unreal.Vector(scale, scale, scale))
    a.static_mesh_component.set_editor_property("mobility", unreal.ComponentMobility.STATIC)
    if not collide:
        a.static_mesh_component.set_collision_enabled(unreal.CollisionEnabled.NO_COLLISION)
    return a


# Workshop
prop_model("korean_fire_extinguisher_01", (385, -285, 0), 180.0)
prop_model("metal_tool_chest", (-360, 255, 0), 0.0)
prop_model("plastic_broom", (380, 280, 0), 200.0)
prop_model("ladder_sectioned_01", (40, 292, 0), 0.0)
prop_model("metal_stool_01", (-300, 115, 0), 30.0)
prop_model("power_box_01", (-398, 205, 150), 0.0, collide=False)
prop_model("utility_box_01", (-398, -150, 0), 0.0)
prop_model("cardboard_box_01", (-30, -280, 160), 10.0, collide=False)  # on top of the rack
prop_model("spray_paint_bottles", (-335, -80, 91), 0.0, collide=False)
# Yard: against the front wall and near the scrap heap, away from the door and the van
prop_model("old_military_compressor", (455, 265, 0), 90.0)
prop_model("portable_welding_cart", (460, -268, 0), 90.0)
prop_model("industrial_wall_lamp", (424, -245, 255), 0.0, collide=False)
prop_model("industrial_wall_lamp", (424, 245, 255), 0.0, collide=False)
prop_model("Barrel_01", (600, 380, 0), 0.0)
prop_model("barrel_03", (655, 400, 0), 40.0)
prop_model("propane_tank", (560, 395, 0), 0.0)
prop_model("old_tyre", (860, 420, 30), 0.0)
prop_model("old_tyre", (860, 420, 47), 30.0)
prop_model("old_tyre", (930, 400, 30), 70.0)
prop_model("cement_bag", (560, -380, 0), 15.0)
prop_model("cement_bag", (560, -380, 18), -10.0)
prop_model("metal_jerrycan", (600, -395, 0), 0.0)
prop_model("portable_generator", (980, -420, 0), 120.0)
prop_model("plastic_crate_01", (650, 330, 0), 25.0)
prop_model("plastic_crate_01", (650, 330, 26), 10.0)
prop_model("security_light", (424, 0, 300), 0.0, collide=False)

# --- Neighbourhood (Phase 17.5): street, power lines, fence, neighbours, clutter, props that come with growth ---
import os, sys
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import world_neighbourhood
world_neighbourhood.build_all(menu=False)

# --- Atmosphere (Phase 15): dust floating in the workshop light, room tone inside, wind and birds outside ---
motes = spawn(unreal.WEFxEmitter, (0, 0, 150), label="DustMotes", folder="Atmosphere")
motes.set_editor_property("kind", unreal.WEFxKind.MOTES)
motes.set_editor_property("box_half_size", unreal.Vector(360, 260, 120))


def ambient(label, sound_name, loc, inner=None, falloff=None):
    a = spawn(unreal.AmbientSound, loc, label=label, folder="Atmosphere")
    comp = a.get_component_by_class(unreal.AudioComponent)
    comp.set_editor_property("sound", unreal.load_asset(f"/Game/WorkshopEmpire/Audio/{sound_name}"))
    if inner is not None:
        comp.set_editor_property("override_attenuation", True)
        att = comp.get_editor_property("attenuation_overrides")
        att.set_editor_property("attenuate", True)
        att.set_editor_property("spatialize", False)
        att.set_editor_property("attenuation_shape_extents", unreal.Vector(inner, 0, 0))
        att.set_editor_property("falloff_distance", falloff)
        comp.set_editor_property("attenuation_overrides", att)
    return a


ambient("RoomTone", "SFX_RoomTone", (0, 0, 150))
ambient("Outdoor", "SFX_Outdoor", (1300, 400, 200), inner=1800.0, falloff=1600.0)

# Phase 17: anything still falling below -20 m is out of the world (items are rescued to the delivery pallet).
world_settings = unreal.EditorLevelLibrary.get_editor_world().get_world_settings()
world_settings.set_editor_property("kill_z", -2000.0)
les.save_current_level()
unreal.log("WE_MAP_BUILT " + MAP_PATH)
unreal.SystemLibrary.quit_editor()
