"""
Massive quality overhaul: fixes and dresses the starting garage in the existing maps (L_Garage, L_MainMenu) without
rebuilding them. build_garage_map.py carries the same placements for a map built from scratch.

  - fuse box (power_box_01) turned to face the room and mounted flush on the wall, conduit up to the ceiling
  - every decoration inside the garage pushed out of the walls (broom, utility box, extinguisher, box on the rack ...)
  - PC desk 12 cm off the wall (the desk lamp reached into it)
  - workbench: modelled bench (SM_Gen_Workbench) instead of plywood boxes
  - ceiling fixtures: Poly Haven fluorescent lights instead of two flat boxes
  - electrical installation: outlets at the bench and the desk, light switch by the door, conduits, junction box
  - lived-in details: wall clock, dartboard, hose reel, drawer cabinet, cans and a pipe wrench

  UnrealEditor-Cmd.exe <uproject> -ExecutePythonScript="<this file>" -unattended -RenderOffscreen -nosplash -nosteam
"""
import unreal

eal = unreal.EditorAssetLibrary
les = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)

X0, X1, Y0, Y1 = -400.0, 400.0, -300.0, 300.0  # inner faces of the garage walls
CEILING = 300.0
GAP = 0.4


def mesh(name):
    path = f"/Game/WorkshopEmpire/Art/Generated/SM_Gen_{name[4:]}" if name.startswith("Gen:") else f"/Game/WorkshopEmpire/Art/Models/SM_{name}"
    return eal.load_asset(path) if eal.does_asset_exist(path) else None


def bounds(a):
    org, ext = a.get_actor_bounds(False)
    return org, ext


def by_label(prefix):
    return [a for a in eas.get_all_level_actors() if a.get_actor_label().startswith(prefix)]


def push_out_of_walls(a):
    """Moves the actor so its bounds stay GAP cm inside the inner wall faces (only actors inside the garage)."""
    org, ext = bounds(a)
    # Only things standing inside the room (the security camera and the meter box outside belong on the outer walls).
    pivot = a.get_actor_location()
    if not (X0 < pivot.x < X1 and Y0 < pivot.y < Y1) or ext.z > 200:
        return False
    loc = a.get_actor_location()
    dx = dy = 0.0
    if org.x - ext.x < X0 + GAP:
        dx = X0 + GAP - (org.x - ext.x)
    elif org.x + ext.x > X1 - GAP and org.x < X1:
        dx = X1 - GAP - (org.x + ext.x)
    if org.y - ext.y < Y0 + GAP:
        dy = Y0 + GAP - (org.y - ext.y)
    elif org.y + ext.y > Y1 - GAP:
        dy = Y1 - GAP - (org.y + ext.y)
    if abs(dx) < 0.05 and abs(dy) < 0.05:
        return False
    a.set_actor_location(unreal.Vector(loc.x + dx, loc.y + dy, loc.z), False, False)
    return True


def place(name, loc, yaw=0.0, label=None, scale=(1.0, 1.0, 1.0), collide=False, folder="Decoration"):
    m = mesh(name)
    if not m:
        unreal.log_warning(f"WE_OVERHAUL missing model {name}")
        return None
    label = label or f"Deco_{name.replace('Gen:', '')}"
    for old in by_label(label):
        if old.get_actor_label() == label:
            eas.destroy_actor(old)
    a = eas.spawn_actor_from_class(unreal.StaticMeshActor, unreal.Vector(*loc), unreal.Rotator(roll=0.0, pitch=0.0, yaw=yaw))
    a.set_actor_label(label)
    a.set_folder_path(folder)
    c = a.static_mesh_component
    c.set_static_mesh(m)
    c.set_editor_property("mobility", unreal.ComponentMobility.STATIC)
    a.set_actor_scale3d(unreal.Vector(*scale))
    if not collide:
        c.set_collision_enabled(unreal.CollisionEnabled.NO_COLLISION)
    return a


def on_wall(name, wall, along, z, label=None, scale=1.0, flat=False):
    """Wall fixture (own models face +X with their back at x = 0) on a wall ('W' x=-400, 'E' x=+400, 'S' y=-300,
    'N' y=+300), facing into the room and flush with it. flat: a picture-like model, its thin side goes to the wall."""
    yaw = {"W": 0.0, "E": 180.0, "S": 90.0, "N": -90.0}[wall]
    pos = {"W": (X0, along), "E": (X1, along), "S": (along, Y0), "N": (along, Y1)}[wall]
    a = place(name, (pos[0], pos[1], z), yaw, label, (scale, scale, scale))
    if a and flat:
        turn_flat_side_to_wall(a, wall)
    if a:
        # Flush: the bounds' wall-side face exactly on the wall.
        org, ext = bounds(a)
        loc = a.get_actor_location()
        if wall == "W":
            a.set_actor_location(unreal.Vector(loc.x + (X0 + GAP) - (org.x - ext.x), loc.y, loc.z), False, False)
        elif wall == "E":
            a.set_actor_location(unreal.Vector(loc.x + (X1 - GAP) - (org.x + ext.x), loc.y, loc.z), False, False)
        elif wall == "S":
            a.set_actor_location(unreal.Vector(loc.x, loc.y + (Y0 + GAP) - (org.y - ext.y), loc.z), False, False)
        else:
            a.set_actor_location(unreal.Vector(loc.x, loc.y + (Y1 - GAP) - (org.y + ext.y), loc.z), False, False)
    return a


def turn_flat_side_to_wall(a, wall):
    """Picture-like models (clock, dartboard): thin along their own Y - turn that axis to the wall normal."""
    org, ext = bounds(a)
    thin_y = ext.y < ext.x
    want_thin_x = wall in ("W", "E")
    if thin_y == want_thin_x:
        r = a.get_actor_rotation()
        a.set_actor_rotation(unreal.Rotator(roll=0.0, pitch=0.0, yaw=r.yaw + 90.0), False)


def overhaul(level):
    if not les.load_level(level):
        unreal.log_error(f"WE_OVERHAUL cannot load {level}")
        return
    if not by_label("Workbench"):
        unreal.log(f"WE_OVERHAUL {level}: no garage, skipped")
        return
    log = []
    # 1. Fuse box: front (logo side, local -Y) to the room (+X), flush on the back wall, conduit to the ceiling.
    for a in by_label("Deco_power_box_01"):
        a.set_actor_rotation(unreal.Rotator(roll=0.0, pitch=0.0, yaw=90.0), False)
        loc = a.get_actor_location()
        a.set_actor_location(unreal.Vector(loc.x, 210.0, 150.0), False, False)
        org, ext = bounds(a)
        a.set_actor_location(unreal.Vector(a.get_actor_location().x + (X0 + GAP) - (org.x - ext.x), 210.0, 150.0), False, False)
        org, ext = bounds(a)
        top = org.z + ext.z
        c = on_wall("Gen:Conduit", "W", org.y, top, "Deco_Conduit_FuseBox")
        if c:
            c.set_actor_scale3d(unreal.Vector(1.0, 1.0, (CEILING - top) / 100.0))
        log.append(f"fuse box at {org} size {ext}")
    # 2. Everything decorative inside the garage out of the walls.
    pushed = []
    for a in eas.get_all_level_actors():
        lab = a.get_actor_label()
        if isinstance(a, unreal.StaticMeshActor) and (lab.startswith("Deco_") or lab.startswith("N_")) and "Conduit" not in lab:
            if push_out_of_walls(a):
                pushed.append(lab)
    log.append(f"pushed out of walls: {pushed}")
    # 2b. The side cutter stood upright at the front edge of the bench and toppled onto the lower shelf: lying flat.
    for a in by_label("Item_SideCutter"):
        a.set_actor_location(unreal.Vector(-348.0, 40.0, 92.0), False, False)
        a.set_actor_rotation(unreal.Rotator(roll=0.0, pitch=90.0, yaw=20.0), False)
    # 3. PC desk off the wall.
    for a in by_label("MarketTerminal"):
        loc = a.get_actor_location()
        a.set_actor_location(unreal.Vector(loc.x, 250.0, loc.z), False, False)
    # 4. Workbench: the modelled bench (the boxes stay as collision).
    bench_mesh = mesh("Gen:Workbench")
    for a in by_label("Workbench"):
        if isinstance(a, unreal.WEWorkbench) and bench_mesh:
            a.set_editor_property("visual_mesh", bench_mesh)
            a.set_editor_property("visual_yaw", 0.0)
            a.set_editor_property("tile_visual", False)
    # 5. Ceiling fixtures: a modelled twin-tube batten (glowing tubes) instead of a flat box and a glowing box.
    fixture = mesh("Gen:TubeFixture")
    for a in [x for x in eas.get_all_level_actors() if "Lamp" in x.get_actor_label() and x.get_actor_label().endswith(("Housing", "Tube"))]:
        lab = a.get_actor_label()
        if lab.endswith("Housing") and fixture and isinstance(a, unreal.StaticMeshActor):
            if a.static_mesh_component.static_mesh == fixture:
                continue  # already done (re-run)
            a.static_mesh_component.set_static_mesh(fixture)
            for i in range(a.static_mesh_component.get_num_materials()):
                a.static_mesh_component.set_material(i, None)
            a.set_actor_scale3d(unreal.Vector(1.0, 1.0, 1.0))
            a.set_actor_rotation(unreal.Rotator(roll=0.0, pitch=0.0, yaw=0.0), False)
            loc = a.get_actor_location()
            a.set_actor_location(unreal.Vector(loc.x, loc.y, loc.z + 2.3), False, False)  # top of the old box = under the ceiling
            log.append(f"{lab}: twin-tube fixture")
        elif lab.endswith("Tube") and isinstance(a, unreal.StaticMeshActor):
            eas.destroy_actor(a)
    # 6. Electrical installation.
    on_wall("Gen:WallOutlet", "W", -118.0, 98.0, "Deco_Outlet_Bench")
    on_wall("Gen:Conduit", "W", -118.0, 110.0, "Deco_Conduit_Bench")
    on_wall("Gen:WallOutlet", "N", -250.0, 30.0, "Deco_Outlet_Desk")
    on_wall("Gen:LightSwitch", "N", 360.0, 125.0, "Deco_LightSwitch")
    sw = on_wall("Gen:Conduit", "N", 360.0, 134.0, "Deco_Conduit_Switch")
    jb = on_wall("Gen:JunctionBox", "W", 210.0, 286.0, "Deco_JunctionBox")
    for a, top in ((by_label("Deco_Conduit_Bench"), 110.0), ([sw] if sw else [], 134.0)):
        for c in a:
            c.set_actor_scale3d(unreal.Vector(1.0, 1.0, (CEILING - top) / 100.0))
    # 7. Lived-in details.
    on_wall("wall_clock", "N", -60.0, 232.0, "Deco_wall_clock", flat=True)
    on_wall("dartboard", "S", 300.0, 170.0, "Deco_dartboard", flat=True)
    on_wall("garden_hose_wall_mounted_01", "W", -250.0, 150.0, "Deco_hose_reel")
    cab = place("drawer_cabinet", (128.0, 270.0, 0.0), 180.0, "Deco_drawer_cabinet", collide=True)
    if cab:
        org, ext = bounds(cab)
        loc = cab.get_actor_location()
        cab.set_actor_location(unreal.Vector(loc.x, loc.y + (Y1 - GAP) - (org.y + ext.y), 0.0), False, False)
    for name, loc, yaw in (("small_oil_can_01", (-372.0, 225.0, 0.0), 30.0), ("can_rusted", (385.0, 250.0, 0.0), 0.0),
                           ("plastic_jerrycan", (370.0, -200.0, 0.0), 80.0)):
        place(name, loc, yaw)
    # Re-check: nothing new in a wall.
    for a in eas.get_all_level_actors():
        lab = a.get_actor_label()
        if isinstance(a, unreal.StaticMeshActor) and lab.startswith("Deco_") and "Conduit" not in lab and "Outlet" not in lab and "Switch" not in lab and "Junction" not in lab:
            push_out_of_walls(a)
    les.save_current_level()
    unreal.log(f"WE_OVERHAUL {level}: " + " | ".join(str(x) for x in log))


for lvl in ("/Game/WorkshopEmpire/Maps/L_Garage", "/Game/WorkshopEmpire/Maps/L_MainMenu"):
    overhaul(lvl)
unreal.SystemLibrary.quit_editor()
