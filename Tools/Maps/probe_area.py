"""Dev probe: actors whose bounds overlap a floor box in L_Garage (x0, x1, y0, y1 below), with their collision state."""
import unreal

eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
les = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
BOX = (-400.0, 400.0, -300.0, 300.0)
les.load_level("/Game/WorkshopEmpire/Maps/L_Garage")
for a in eas.get_all_level_actors():
    org, ext = a.get_actor_bounds(False)
    if ext.x <= 0 or ext.z > 400 or org.z - ext.z > 170:
        continue
    if org.x + ext.x > BOX[0] and org.x - ext.x < BOX[1] and org.y + ext.y > BOX[2] and org.y - ext.y < BOX[3]:
        if not a.get_actor_label().startswith(('Deco_', 'N_', 'Item_')):
            continue
        coll = []
        for c in a.get_components_by_class(unreal.PrimitiveComponent):
            coll.append(str(c.get_collision_enabled()).split(".")[-1])
        unreal.log(f"WE_PROBE {a.get_actor_label()} {a.get_class().get_name()} org=({org.x:.0f},{org.y:.0f},{org.z:.0f}) ext=({ext.x:.0f},{ext.y:.0f},{ext.z:.0f}) coll={coll}")
unreal.SystemLibrary.quit_editor()
