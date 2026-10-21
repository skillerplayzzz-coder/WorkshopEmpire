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


def furniture(fid, name, desc, euros, prop_type, size, tint, bench=False, storage=None, speed=1.0, machine=None, production_storage=False,
              requires=None):
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
    f.set_editor_property("production_storage", production_storage)
    f.set_editor_property("required_upgrade", requires or "None")
    if machine:
        f.set_editor_property("machine", unreal.load_asset(f"/Game/WorkshopEmpire/Data/Machines/{machine}"))
        f.set_editor_property("has_storage", True)
    return f


ITEMS = [
    furniture("Workbench", "Werkbank", "Eine zweite Werkbank: zwei Geräte gleichzeitig bearbeiten.", 90, P.WORKBENCH,
              (80, 200, 90), (0.45, 0.30, 0.18), bench=True, storage=("Werkbank", 24, 100.0)),
    furniture("LargeWorkbench", "Große Werkbank", "Massive Arbeitsplatte, mehr Ablage: Arbeiten 20 % schneller.", 240, P.WORKBENCH,
              (100, 280, 95), (0.32, 0.22, 0.14), bench=True, storage=("Große Werkbank", 36, 160.0), speed=1.2),
    furniture("HeavyShelf", "Schwerlastregal", "Robustes Stahlregal für Teile und Material.", 60, P.SHELF,
              (50, 180, 200), (0.30, 0.32, 0.35), storage=("Schwerlastregal", 15, 150.0)),
    furniture("LargeShelf", "Großes Lagerregal", "Hohes, breites Regal für viel Lagerplatz.", 140, P.SHELF,
              (60, 240, 240), (0.22, 0.30, 0.40), storage=("Großes Lagerregal", 25, 300.0)),
    furniture("ToolWall", "Werkzeugwand", "Lochwand mit Haken: Werkzeug hängt sichtbar an seinem Platz und ist an der Werkbank in der Nähe griffbereit ([R] aufhängen, [Q] nehmen).", 40, P.SHELF,
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
    # Phase 10: automation. Conveyors run along +X: start at the back, end at the front.
    furniture("Conveyor", "Förderband", "1 m Förderband: nimmt Produkte aus der Maschine dahinter und gibt sie vorn ab. 0,15 kW.", 25, P.CONVEYOR,
              (100, 50, 45), (0.18, 0.19, 0.21)),
    furniture("ConveyorLong", "Förderband (2 m)", "2 m Förderband.", 45, P.CONVEYOR,
              (200, 50, 45), (0.18, 0.19, 0.21)),
    furniture("MaterialStore", "Materiallager", "Großer Gitterbehälter für Produktionslinien: nimmt Material vom Band an und gibt es auf ein Band ab ([T] wählt was).", 150, P.CRATE,
              (100, 120, 80), (0.30, 0.45, 0.25), storage=("Materiallager", 30, 400.0), production_storage=True),
    furniture("Assembler", "Fertigungsautomat", "Stellt aus sortiertem Material Produkte her: Kupferdrahtspulen, Verlängerungskabel, Stahlwinkel. 6 kW.", 600, P.MACHINE,
              (120, 160, 160), (0.25, 0.30, 0.55), machine="MD_Assembler"),
    # Phase 12: expansion (need the buildings).
    furniture("Packer", "Verpackungsanlage", "Palettiert Stahlwinkel-Sets und Verlängerungskabel für den Großhandel. 3 kW.", 1400, P.MACHINE,
              (160, 200, 180), (0.40, 0.55, 0.30), machine="MD_Packer", requires="WorkshopExtension"),
    furniture("Furnace", "Schmelzofen", "Schmilzt Aluminiumschrott und Kupfergranulat zu Barren. 15 kW – braucht den Anschluss der Gewerbehalle.", 2800, P.MACHINE,
              (200, 200, 260), (0.70, 0.22, 0.08), machine="MD_Furnace", requires="CommercialHall"),
    furniture("HighRack", "Hochregal", "Hohes Palettenregal: 40 Plätze, 1.000 kg, nimmt sperrige Paletten nicht (dafür das Materiallager).", 700, P.SHELF,
              (120, 400, 450), (0.25, 0.35, 0.55), storage=("Hochregal", 40, 1000.0), requires="CommercialHall"),
    furniture("BigMaterialStore", "Großes Materiallager", "Gitterboxlager für die Halle: 60 Plätze, 1.500 kg, auch Paletten und Ballen.", 900, P.CRATE,
              (200, 240, 100), (0.30, 0.45, 0.25), storage=("Großes Materiallager", 60, 1500.0), production_storage=True, requires="CommercialHall"),
    # Phase 16: endgame machine - needs the factory hall (industrial power connection).
    furniture("Refinery", "Edelmetall-Rückgewinnung", "Schließt Platinenschrott auf: 20 Elektronikschrott → Edelmetallkonzentrat und Kupfergranulat. 12 kW – nur in der Fabrikhalle.", 9000, P.MACHINE,
              (180, 220, 230), (0.55, 0.50, 0.20), machine="MD_Refinery", requires="FactoryHall"),
    # Phase 17.5: decoration - makes the workshop yours (no storage; bought and placed like furniture, saved and synced the same way).
    furniture("DecoPlant", "Deko: Pflanzkübel", "Ein bisschen Grün zwischen Öl und Metall.", 25, P.CRATE, (45, 95, 45), (0.3, 0.4, 0.25)),
    furniture("DecoChalkboard", "Deko: Kreidetafel", "Für Aufträge, Notizen und Sprüche.", 35, P.CRATE, (45, 80, 150), (0.2, 0.2, 0.2)),
    furniture("DecoTV", "Deko: Alter Fernseher", "Läuft immer, keiner schaut hin.", 45, P.CRATE, (50, 60, 50), (0.2, 0.2, 0.2)),
    furniture("DecoChair", "Deko: Stuhl", "Für die Pause zwischendurch.", 20, P.CRATE, (55, 50, 85), (0.2, 0.3, 0.5)),
    furniture("DecoBarrelStove", "Deko: Fassofen", "Ein alter Ölfass-Ofen für kalte Wintertage.", 60, P.CRATE, (65, 65, 90), (0.3, 0.25, 0.2)),
    furniture("DecoWetFloor", "Deko: Warnschild", "Vorsicht, Rutschgefahr!", 10, P.CRATE, (35, 35, 65), (0.8, 0.7, 0.1)),
    furniture("DecoExtinguisher", "Deko: Feuerlöscher", "Sicherheit zuerst.", 30, P.CRATE, (25, 25, 60), (0.7, 0.1, 0.1)),
    furniture("DecoCompressor", "Deko: Kompressor", "Ein alter Kompressor, der in jede Werkstatt gehört.", 120, P.CRATE, (60, 110, 80), (0.3, 0.35, 0.3)),
    furniture("DecoDrillPress", "Deko: Standbohrmaschine", "Schwer, alt, schön.", 180, P.CRATE, (60, 60, 170), (0.3, 0.35, 0.3)),
    furniture("DecoCrates", "Deko: Getränkekisten", "Gestapelte Kisten mit leeren Flaschen.", 15, P.CRATE, (40, 60, 60), (0.6, 0.1, 0.1)),
    furniture("DecoHangingLamp", "Deko: Industrielampe", "Eine Hängelampe über dem Arbeitsplatz.", 40, P.CRATE, (40, 40, 60), (0.2, 0.2, 0.2)),
    furniture("DecoCoffee", "Deko: Kaffeewagen", "Ohne Kaffee läuft nichts – für die größere Werkstatt.", 150, P.CRATE, (60, 120, 100), (0.5, 0.3, 0.2), requires="WorkshopExtension"),
    furniture("DecoCar", "Deko: Abgedecktes Auto", "Ein Projektwagen unter der Plane – für die Halle.", 400, P.CRATE, (180, 440, 145), (0.4, 0.4, 0.4), requires="CommercialHall"),]

path = f"{FOLDER}/{NAME}"
if eal.does_asset_exist(path):
    catalog = eal.load_asset(path)
else:
    factory = unreal.DataAssetFactory()
    factory.set_editor_property("data_asset_class", unreal.WEFurnitureCatalog)
    catalog = tools.create_asset(NAME, FOLDER, unreal.WEFurnitureCatalog, factory)
# --- Phase 15: modelled looks (CC0 models from Poly Haven, imported by Tools/Assets/import_assets.py) ---
MODELS = "/Game/WorkshopEmpire/Art/Models"


def model(name):
    path = f"{MODELS}/SM_{name}"
    return unreal.load_asset(path) if unreal.EditorAssetLibrary.does_asset_exist(path) else None


def deco(name, loc, rot=(0.0, 0.0, 0.0), scale=1.0):
    return (model(name), unreal.Transform(unreal.Vector(*loc), unreal.Rotator(roll=rot[2], pitch=rot[0], yaw=rot[1]), unreal.Vector(scale, scale, scale)))


VISUALS = {
    # id: (model, yaw, tile copies along the width, decorations)
    "HeavyShelf": ("worn_metal_rack", 90.0, True, []),
    "LargeShelf": ("steel_frame_shelves_02", 90.0, True, []),
    "HighRack": ("steel_frame_shelves_01", 90.0, True, []),
    "Crate": ("plastic_crate_01", 90.0, True, []),
    "Table": ("tool_cart", 90.0, False, []),
    # Workbenches keep their wooden box shape (items lie on the top); a toolbox on the lower shelf.
    "Workbench": (None, 0.0, False, [deco("metal_toolbox", (5, -55, 12), (0, 90, 0)), deco("oil_tin", (5, 55, 12))]),
    "LargeWorkbench": (None, 0.0, False, [deco("metal_toolbox", (5, -80, 12), (0, 90, 0)), deco("metal_tool_chest", (5, 70, 12), (0, 90, 0))]),
    # Tool wall (Phase 21): AWEToolWall draws the pegboard and hangs the tools that are really in its storage.
    "ToolWall": (None, 0.0, False, []),
    # Machines keep their painted-metal bodies; a switch box on the side.
    "Shredder": (None, 0.0, False, [deco("power_box_01", (0, -60, 110), (0, -90, 0), 0.7)]),
    "Sorter": (None, 0.0, False, [deco("power_box_01", (0, -95, 105), (0, -90, 0), 0.7)]),
    "Press": (None, 0.0, False, [deco("power_box_01", (0, -55, 115), (0, -90, 0), 0.7)]),
    "Assembler": (None, 0.0, False, [deco("power_box_01", (0, -85, 110), (0, -90, 0), 0.7)]),
    "Packer": (None, 0.0, False, [deco("utility_box_01", (0, -105, 0), (0, -90, 0), 0.8)]),
    "Furnace": (None, 0.0, False, [deco("utility_box_01", (0, -105, 0), (0, -90, 0), 0.9), deco("propane_tank", (-70, -110, 0))]),
    "Refinery": (None, 0.0, False, [deco("power_box_01", (0, -115, 120), (0, -90, 0), 0.8), deco("Barrel_01", (-60, -125, 0))]),
    # Phase 17.5 decoration models.
    "DecoPlant": ("planter_box_01", 90.0, False, []),
    "DecoChalkboard": ("standing_chalkboard_01", 90.0, False, []),
    "DecoTV": ("Television_01", 90.0, False, []),
    "DecoChair": ("SchoolChair_01", 90.0, False, []),
    "DecoBarrelStove": ("barrel_stove", 0.0, False, []),
    "DecoWetFloor": ("WetFloorSign_01", 90.0, False, []),
    "DecoExtinguisher": ("korean_fire_extinguisher_01", 0.0, False, []),
    "DecoCompressor": ("old_military_compressor", 90.0, False, []),
    "DecoDrillPress": ("old_drill_press", 90.0, False, []),
    "DecoCrates": ("plastic_crate_02", 90.0, False, []),
    "DecoHangingLamp": ("caged_hanging_light", 0.0, False, []),
    "DecoCoffee": ("CoffeeCart_01", 90.0, False, []),
    "DecoCar": ("covered_car", 0.0, False, []),
}
for f in ITEMS:
    fid = str(f.get_editor_property("id"))
    if fid not in VISUALS:
        continue
    mesh_name, yaw, tile, decos = VISUALS[fid]
    f.set_editor_property("visual_mesh", model(mesh_name) if mesh_name else None)
    f.set_editor_property("visual_yaw", yaw)
    f.set_editor_property("tile_visual", tile)
    decos = [d for d in decos if d[0]]
    f.set_editor_property("deco_meshes", [d[0] for d in decos])
    f.set_editor_property("deco_transforms", [d[1] for d in decos])

catalog.set_editor_property("items", ITEMS)
eal.save_asset(path)

unreal.log(f"WE_WORKSHOP_BUILT {len(ITEMS)} furniture")
unreal.SystemLibrary.quit_editor()
