"""
Creates/updates the furniture catalog /Game/WorkshopEmpire/Data/Workshop/DA_FurnitureCatalog (Phase 8):
everything the company can buy and place in build mode ([B]).

Run after build_item_definitions.py and before build_garage_map.py:
  UnrealEditor-Cmd.exe <WorkshopEmpire.uproject> -ExecutePythonScript="<this file>" -unattended -RenderOffscreen -nosplash

Prices are first-pass balancing (an hour of play buys a second bench and a big shelf).
Sizes in cm: X depth, Y width, Z height; origin = floor centre, front faces +X.
"""
import unreal

FOLDER = "/Game/WorkshopEmpire/Data/Workshop"
NAME = "DA_FurnitureCatalog"
P = unreal.WEPlaceholderPropType
eal = unreal.EditorAssetLibrary
tools = unreal.AssetToolsHelpers.get_asset_tools()


def furniture(fid, name, desc, euros, prop_type, size, tint, bench=False, storage=None, speed=1.0, machine=None):
    f = unreal.WEFurnitureDefinition()
    f.set_editor_property("id", fid)
    f.set_editor_property("display_name", unreal.Text(name))
    f.set_editor_property("description", unreal.Text(desc))
    cost = unreal.WEMoney()
    cost.set_editor_property("cents", int(euros * 100))
    f.set_editor_property("cost", cost)
    f.set_editor_property("workbench", bench)
    f.set_editor_property("prop_type", prop_type)
    f.set_editor_property("size", unreal.Vector(*size))
    f.set_editor_property("tint", unreal.LinearColor(*tint, 1.0))
    f.set_editor_property("has_storage", storage is not None)
    if storage:
        label, slots, kg = storage
        f.set_editor_property("storage_name", unreal.Text(label))
        f.set_editor_property("storage_slots", slots)
        f.set_editor_property("storage_max_weight_kg", kg)
    f.set_editor_property("work_speed_multiplier", speed)
    if machine:
        f.set_editor_property("machine", unreal.load_asset(f"/Game/WorkshopEmpire/Data/Machines/{machine}"))
        f.set_editor_property("has_storage", True)
    return f


ITEMS = [
    furniture("Workbench", "Werkbank", "Eine zweite Werkbank: zwei Geräte gleichzeitig bearbeiten.", 90, P.WORKBENCH,
              (80, 200, 90), (0.45, 0.30, 0.18), bench=True, storage=("Werkbank", 12, 60.0)),
    furniture("LargeWorkbench", "Große Werkbank", "Massive Arbeitsplatte, mehr Ablage: Arbeiten 20 % schneller.", 240, P.WORKBENCH,
              (100, 280, 95), (0.32, 0.22, 0.14), bench=True, storage=("Große Werkbank", 20, 100.0), speed=1.2),
    furniture("HeavyShelf", "Schwerlastregal", "Robustes Stahlregal für Teile und Material.", 60, P.SHELF,
              (50, 180, 200), (0.30, 0.32, 0.35), storage=("Schwerlastregal", 15, 150.0)),
    furniture("LargeShelf", "Großes Lagerregal", "Hohes, breites Regal für viel Lagerplatz.", 140, P.SHELF,
              (60, 240, 240), (0.22, 0.30, 0.40), storage=("Großes Lagerregal", 25, 300.0)),
    furniture("ToolWall", "Werkzeugwand", "Flache Wand mit Haken und Ablagen – Werkzeug immer griffbereit.", 40, P.SHELF,
              (20, 160, 180), (0.25, 0.38, 0.30), storage=("Werkzeugwand", 10, 30.0)),
    furniture("Crate", "Lagerkiste", "Einfache Holzkiste für Kleinkram.", 15, P.CRATE,
              (60, 80, 50), (0.62, 0.45, 0.25), storage=("Lagerkiste", 8, 60.0)),
    furniture("Table", "Ablagetisch", "Tisch zum Abstellen großer Geräte.", 30, P.WORKBENCH,
              (70, 150, 85), (0.50, 0.42, 0.32)),
    # Phase 9: machines (definitions: build_machines.py).
    furniture("Shredder", "Schredder", "Zerkleinert Geräte und Schrott zu Schreddergut. 7,5 kW.", 450, P.MACHINE,
              (110, 110, 150), (0.75, 0.55, 0.10), machine="MD_Shredder"),
    furniture("Sorter", "Sortiermaschine", "Trennt Schreddergut in Kupfer, Aluminium, Stahl, Kunststoff und Elektronikschrott. 4 kW.", 380, P.MACHINE,
              (100, 180, 140), (0.15, 0.45, 0.45), machine="MD_Sorter"),
    furniture("Press", "Metallpresse", "Presst lose Metallteile zu Ballen, die mehr wert sind. 5,5 kW.", 320, P.MACHINE,
              (120, 100, 170), (0.50, 0.14, 0.12), machine="MD_Press"),
]

path = f"{FOLDER}/{NAME}"
if eal.does_asset_exist(path):
    catalog = eal.load_asset(path)
else:
    factory = unreal.DataAssetFactory()
    factory.set_editor_property("data_asset_class", unreal.WEFurnitureCatalog)
    catalog = tools.create_asset(NAME, FOLDER, unreal.WEFurnitureCatalog, factory)
catalog.set_editor_property("items", ITEMS)
eal.save_asset(path)

unreal.log(f"WE_WORKSHOP_BUILT {len(ITEMS)} furniture")
unreal.SystemLibrary.quit_editor()
