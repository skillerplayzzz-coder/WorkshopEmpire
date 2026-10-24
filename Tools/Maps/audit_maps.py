"""
Massive Quality Overhaul: automatic audit of the placed objects in L_Garage and L_MainMenu.
Writes Saved/Audit/<map>.txt and logs a summary (WE_AUDIT ...):
  - basic shapes (engine cubes/cylinders/spheres) and generated placeholder boxes
  - missing meshes / materials (None or the engine default material)
  - non-uniform or extreme scales
  - objects below the floor, floating (bottom well above anything under them is not checked - only z < -5)
  - objects in the walls of the garage (x = +-400, y = +-300 shell): mesh bounds crossing a wall plane
  - actors with physics simulated in the map (props that may jitter)

  UnrealEditor-Cmd.exe <uproject> -ExecutePythonScript="<this file>" -unattended -RenderOffscreen -nosplash -nosteam
"""
import os

import unreal

eal = unreal.EditorAssetLibrary
les = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
OUT = os.path.join(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_saved_dir()), "Audit")
os.makedirs(OUT, exist_ok=True)

GARAGE = (-400.0, 400.0, -300.0, 300.0)  # inner shell of the starting garage


def audit(level):
    if not les.load_level(level):
        unreal.log_error(f"WE_AUDIT cannot load {level}")
        return
    lines = []
    counts = {}
    for a in eas.get_all_level_actors():
        label = a.get_actor_label()
        cls = a.get_class().get_name()
        loc = a.get_actor_location()
        rot = a.get_actor_rotation()
        scale = a.get_actor_scale3d()
        for c in a.get_components_by_class(unreal.StaticMeshComponent):
            sm = c.static_mesh
            if not c.is_visible():
                continue
            mesh = sm.get_path_name() if sm else "None"
            issues = []
            if not sm:
                issues.append("NO_MESH")
            elif "/Engine/BasicShapes/" in mesh:
                issues.append("BASIC_SHAPE")
            if sm:
                for i in range(c.get_num_materials()):
                    m = c.get_material(i)
                    if not m or "WorldGridMaterial" in m.get_path_name() or "DefaultMaterial" in m.get_path_name():
                        issues.append(f"MATERIAL_{i}_MISSING")
                        break
                    if "M_WEPlaceholder" in m.get_path_name() or "MI_WEPlaceholder" in m.get_path_name():
                        issues.append("PLACEHOLDER_MATERIAL")
                        break
            ws = c.get_world_scale()
            if max(ws.x, ws.y, ws.z) > 0 and max(ws.x, ws.y, ws.z) / max(0.001, min(ws.x, ws.y, ws.z)) > 6.0 and "/Engine/BasicShapes/" not in mesh:
                issues.append(f"STRETCHED({ws.x:.2f},{ws.y:.2f},{ws.z:.2f})")
            org, ext, _r = unreal.SystemLibrary.get_component_bounds(c)
            lo = unreal.Vector(org.x - ext.x, org.y - ext.y, org.z - ext.z)
            hi = unreal.Vector(org.x + ext.x, org.y + ext.y, org.z + ext.z)
            if hi.z < -5.0 and sm:
                issues.append(f"BELOW_FLOOR(z={hi.z:.0f})")
            # Inside the garage footprint and crossing a wall plane by more than 3 cm (the walls themselves excluded).
            inside = GARAGE[0] - 40 < org.x < GARAGE[1] + 40 and GARAGE[2] - 40 < org.y < GARAGE[3] + 40
            is_shell = any(k in label for k in ("Wall", "Floor", "Ceiling", "Roof", "Garage", "Door", "Shutter", "Ground", "Beam", "Trim", "Skirting", "Gutter", "Sign"))
            if inside and sm and not is_shell and ext.z < 200:
                for plane, axis, lo_v, hi_v in ((GARAGE[0], "x", lo.x, hi.x), (GARAGE[1], "x", lo.x, hi.x), (GARAGE[2], "y", lo.y, hi.y), (GARAGE[3], "y", lo.y, hi.y)):
                    depth = min(hi_v - plane, plane - lo_v)
                    if lo_v < plane < hi_v and depth > 3.0:
                        issues.append(f"IN_WALL_{axis}{plane:.0f}({depth:.0f}cm)")
            if c.is_simulating_physics():
                issues.append("PHYSICS")
            if issues:
                for k in issues:
                    key = k.split("(")[0]
                    counts[key] = counts.get(key, 0) + 1
                lines.append(f"{label} [{cls}/{c.get_name()}] mesh={mesh.split('/')[-1]} at ({org.x:.0f},{org.y:.0f},{org.z:.0f}) ext ({ext.x:.0f},{ext.y:.0f},{ext.z:.0f}) rot {rot.yaw:.0f}/{rot.pitch:.0f}/{rot.roll:.0f}: {' '.join(issues)}")
    name = level.split("/")[-1]
    with open(os.path.join(OUT, f"{name}.txt"), "w", encoding="utf-8") as f:
        f.write("\n".join(sorted(lines)))
    unreal.log(f"WE_AUDIT {name}: {len(lines)} findings {counts}")


for lvl in ("/Game/WorkshopEmpire/Maps/L_Garage", "/Game/WorkshopEmpire/Maps/L_MainMenu"):
    audit(lvl)
unreal.SystemLibrary.quit_editor()
