"""
Creates/updates the item definition Data Assets (/Game/WorkshopEmpire/Data/Items), the disassembly
sequences (/Game/WorkshopEmpire/Data/Disassembly) and the repair procedures (/Game/WorkshopEmpire/Data/Repair).
Existing assets are updated in place.

Run after build_placeholder_material.py and before build_garage_map.py:
  UnrealEditor-Cmd.exe <WorkshopEmpire.uproject> -ExecutePythonScript="<this file>" -unattended -RenderOffscreen -nosplash

All numbers are first-pass placeholders; balancing happens in Phase 4/16.
"""
import unreal

ITEM_FOLDER = "/Game/WorkshopEmpire/Data/Items"
DIS_FOLDER = "/Game/WorkshopEmpire/Data/Disassembly"
REP_FOLDER = "/Game/WorkshopEmpire/Data/Repair"
CUBE = unreal.load_asset("/Engine/BasicShapes/Cube.Cube")
CYLINDER = unreal.load_asset("/Engine/BasicShapes/Cylinder.Cylinder")

C = unreal.WEItemCategory
M = unreal.WEMaterialType
T = unreal.WEToolType

tools = unreal.AssetToolsHelpers.get_asset_tools()
eal = unreal.EditorAssetLibrary


def data_asset(folder, name, cls):
    path = f"{folder}/{name}"
    if eal.does_asset_exist(path):
        return eal.load_asset(path), path
    factory = unreal.DataAssetFactory()
    factory.set_editor_property("data_asset_class", cls)
    return tools.create_asset(name, folder, cls, factory), path


# asset suffix, ItemId, name, description, category, kg, value (cents), stackable, max stack, fits inventory,
# repairable, tool type, tool speed, mesh, scale, tint, materials [(type, kg)]
ITEMS = [
    ("OldComputer", "OldComputer", "Alter Computer",
     "Ein ausgedienter Büro-PC. Das Gehäuse ist verbeult, aber vielleicht steckt noch etwas Brauchbares drin.",
     C.ELECTRONICS, 9.0, 7500, False, 1, False, True, T.NONE, 1.0, CUBE, (0.45, 0.20, 0.42), (0.72, 0.69, 0.60),
     [(M.STEEL, 5.0), (M.PLASTIC, 1.5), (M.ELECTRONICS, 2.0), (M.COPPER, 0.3), (M.PRECIOUS_METAL, 0.01)]),
    ("CrtMonitor", "CrtMonitor", "Röhrenmonitor",
     "Schwer, staubig und längst aus der Mode. Kupfer und Glas lassen sich vielleicht verwerten.",
     C.ELECTRONICS, 14.0, 3000, False, 1, False, True, T.NONE, 1.0, CUBE, (0.40, 0.40, 0.38), (0.15, 0.15, 0.16),
     [(M.GLASS, 8.0), (M.PLASTIC, 3.0), (M.COPPER, 1.0), (M.ELECTRONICS, 1.0)]),
    ("Screwdriver", "Screwdriver", "Schraubendreher",
     "Ein einfacher Kreuzschlitz-Schraubendreher. Der Griff ist abgenutzt.",
     C.TOOL, 0.2, 600, False, 1, True, False, T.SCREWDRIVER, 1.0, CYLINDER, (0.03, 0.03, 0.22), (0.75, 0.10, 0.08),
     [(M.STEEL, 0.15), (M.PLASTIC, 0.05)]),
    ("SideCutter", "SideCutter", "Seitenschneider",
     "Zum Abtrennen von Kabeln und Kabelbindern.",
     C.TOOL, 0.15, 900, False, 1, True, False, T.SIDE_CUTTER, 1.0, CUBE, (0.12, 0.05, 0.02), (0.85, 0.55, 0.10),
     [(M.STEEL, 0.12), (M.PLASTIC, 0.03)]),
    ("CordlessDrill", "CordlessDrill", "Akkuschrauber",
     "Schraubt deutlich schneller als jeder Handschraubendreher.",
     C.POWER_TOOL, 1.3, 4500, False, 1, True, False, T.SCREWDRIVER, 2.5, CUBE, (0.22, 0.07, 0.18), (0.15, 0.55, 0.20),
     [(M.STEEL, 0.4), (M.PLASTIC, 0.6), (M.ELECTRONICS, 0.3)]),
    ("Toolbox", "Toolbox", "Werkzeugkiste",
     "Eine Metallkiste für Handwerkzeug. Im Moment fast leer.",
     C.TOOL, 6.0, 2500, False, 1, False, False, T.NONE, 1.0, CUBE, (0.45, 0.22, 0.22), (0.10, 0.25, 0.65),
     [(M.STEEL, 6.0)]),
    ("ElectricMotor", "ElectricMotor", "Alter Elektromotor",
     "Ein schwerer Drehstrommotor. Zum Tragen braucht es später einen Hubwagen oder Hilfe.",
     C.MOTOR, 45.0, 6000, False, 1, False, True, T.NONE, 1.0, CYLINDER, (0.35, 0.35, 0.45), (0.25, 0.35, 0.28),
     [(M.IRON, 30.0), (M.COPPER, 8.0), (M.ALUMINIUM, 5.0)]),
    ("CableBox", "CableBox", "Karton mit Kabeln",
     "Ein Karton voller alter Kabel. Gutes Kupfer, wenn man es sortiert.",
     C.SCRAP_METAL, 3.0, 800, False, 1, False, False, T.NONE, 1.0, CUBE, (0.35, 0.30, 0.25), (0.55, 0.42, 0.26),
     [(M.COPPER, 1.6), (M.PLASTIC, 1.2)]),
    ("SheetMetalScrews", "SheetMetalScrews", "Blechschrauben",
     "Kleine verzinkte Schrauben. Braucht man ständig.",
     C.SPARE_PART, 0.005, 5, True, 200, True, False, T.NONE, 1.0, CUBE, (0.10, 0.07, 0.04), (0.70, 0.72, 0.75),
     [(M.STEEL, 0.005)]),
    ("CopperCable", "CopperCable", "Kupferkabel",
     "Ein Stück Kupferkabel, etwa ein Meter.",
     C.RAW_MATERIAL, 0.25, 120, True, 20, True, False, T.NONE, 1.0, CYLINDER, (0.05, 0.05, 0.30), (0.72, 0.38, 0.18),
     [(M.COPPER, 0.2), (M.PLASTIC, 0.05)]),
    ("RamModule", "RamModule", "Arbeitsspeicher (4 GB)",
     "Ein DDR3-Speicherriegel. Getestet ist er nicht.",
     C.ELECTRONICS, 0.03, 800, False, 1, True, True, T.NONE, 1.0, CUBE, (0.14, 0.04, 0.02), (0.10, 0.45, 0.20),
     [(M.ELECTRONICS, 0.025), (M.PRECIOUS_METAL, 0.0005)]),
    ("GraphicsCard", "GraphicsCard", "Grafikkarte (alt)",
     "Eine ältere Grafikkarte mit Lüfter. Für Bastler noch interessant.",
     C.ELECTRONICS, 0.6, 2500, False, 1, True, True, T.NONE, 1.0, CUBE, (0.25, 0.12, 0.04), (0.20, 0.20, 0.22),
     [(M.ELECTRONICS, 0.3), (M.ALUMINIUM, 0.2), (M.COPPER, 0.05), (M.PRECIOUS_METAL, 0.002)]),
    ("PowerSupply", "PowerSupply", "PC-Netzteil",
     "Ein ATX-Netzteil. Ob es noch läuft, zeigt erst ein Test.",
     C.SPARE_PART, 1.4, 1200, False, 1, True, True, T.NONE, 1.0, CUBE, (0.15, 0.14, 0.09), (0.35, 0.35, 0.38),
     [(M.STEEL, 0.7), (M.COPPER, 0.3), (M.ELECTRONICS, 0.4)]),
    ("Mainboard", "Mainboard", "Mainboard",
     "Die Hauptplatine des Computers mit Prozessor und Kühler.",
     C.ELECTRONICS, 0.8, 1500, False, 1, True, True, T.NONE, 1.0, CUBE, (0.30, 0.24, 0.03), (0.10, 0.35, 0.18),
     [(M.ELECTRONICS, 0.5), (M.COPPER, 0.1), (M.ALUMINIUM, 0.15), (M.PRECIOUS_METAL, 0.003)]),
    ("SteelScrap", "SteelScrap", "Stahlschrott",
     "Blechteile und Schrauben aus Stahl, je Einheit etwa ein Kilogramm.",
     C.RAW_MATERIAL, 1.0, 25, True, 50, True, False, T.NONE, 1.0, CUBE, (0.18, 0.18, 0.06), (0.45, 0.45, 0.48),
     [(M.STEEL, 1.0)]),
    ("PlasticParts", "PlasticParts", "Kunststoffteile",
     "Abdeckungen, Blenden und Gehäuseteile aus Kunststoff.",
     C.RAW_MATERIAL, 0.5, 10, True, 50, True, False, T.NONE, 1.0, CUBE, (0.16, 0.12, 0.06), (0.85, 0.85, 0.80),
     [(M.PLASTIC, 0.5)]),
    ("GlassCullet", "GlassCullet", "Glasbruch",
     "Scherben aus einer Bildröhre. Vorsicht, scharfkantig.",
     C.RAW_MATERIAL, 1.0, 5, True, 50, True, False, T.NONE, 1.0, CUBE, (0.15, 0.15, 0.07), (0.55, 0.75, 0.80),
     [(M.GLASS, 1.0)]),
    ("ElectronicScrap", "ElectronicScrap", "Elektronikschrott",
     "Platinen und Bauteile, aus denen sich später Edelmetalle gewinnen lassen.",
     C.RAW_MATERIAL, 0.25, 60, True, 50, True, False, T.NONE, 1.0, CUBE, (0.14, 0.10, 0.03), (0.15, 0.40, 0.25),
     [(M.ELECTRONICS, 0.24), (M.PRECIOUS_METAL, 0.001)]),
    # Phase 6: repair tools, spare parts and consumables.
    ("Multimeter", "Multimeter", "Multimeter",
     "Misst Spannung und Durchgang. Damit findet man heraus, welches Bauteil defekt ist.",
     C.TOOL, 0.3, 1200, False, 1, True, False, T.MULTIMETER, 1.0, CUBE, (0.10, 0.06, 0.03), (0.95, 0.75, 0.10),
     [(M.ELECTRONICS, 0.1), (M.PLASTIC, 0.2)]),
    ("SolderingIron", "SolderingIron", "Lötkolben",
     "Zum Aus- und Einlöten elektronischer Bauteile.",
     C.TOOL, 0.4, 1500, False, 1, True, False, T.SOLDERING_IRON, 1.0, CYLINDER, (0.03, 0.03, 0.25), (0.20, 0.20, 0.25),
     [(M.STEEL, 0.2), (M.PLASTIC, 0.1), (M.COPPER, 0.05)]),
    ("Capacitors", "Capacitors", "Kondensatoren-Set",
     "Ein Tütchen Elektrolytkondensatoren für Netzteil- und Monitorplatinen.",
     C.SPARE_PART, 0.05, 250, True, 10, True, False, T.NONE, 1.0, CUBE, (0.08, 0.06, 0.02), (0.15, 0.25, 0.60),
     [(M.ELECTRONICS, 0.04)]),
    ("CleaningKit", "CleaningKit", "Reinigungsset",
     "Pinsel, Druckluft, Isopropanol und Tücher. Macht Altgeräte wieder ansehnlich.",
     C.SPARE_PART, 0.4, 200, True, 10, True, False, T.NONE, 1.0, CUBE, (0.14, 0.08, 0.10), (0.30, 0.70, 0.80),
     [(M.PLASTIC, 0.3)]),
    # Phase 7: rare finds (collectors pay well) and sealed return pallets (contents: build_procurement.py).
    ("RareGpu", "RareGpu", "Grafikkarte (Sammlerstück)",
     "Eine legendäre Grafikkarte aus den frühen 2000ern. Kaum noch zu finden – Sammler zahlen viel.",
     C.COLLECTIBLE, 0.7, 18000, False, 1, True, False, T.NONE, 1.0, CUBE, (0.26, 0.12, 0.05), (0.75, 0.15, 0.15),
     [(M.ELECTRONICS, 0.4), (M.ALUMINIUM, 0.2), (M.PRECIOUS_METAL, 0.004)]),
    ("VintageRadio", "VintageRadio", "Röhrenradio (1958)",
     "Holzgehäuse, Stoffbespannung, magisches Auge. Bei Sammlern sehr gefragt.",
     C.COLLECTIBLE, 7.0, 12000, False, 1, False, False, T.NONE, 1.0, CUBE, (0.45, 0.22, 0.30), (0.45, 0.28, 0.15),
     [(M.GLASS, 1.0), (M.COPPER, 0.5), (M.ELECTRONICS, 0.8)]),
    ("GoldCpu", "GoldCpu", "Keramik-Prozessor mit Goldkontakten",
     "Ein uralter Prozessor mit vergoldeter Keramik. Für Sammler und das Edelmetall-Recycling interessant.",
     C.RARE_FIND, 0.05, 9000, False, 1, True, False, T.NONE, 1.0, CUBE, (0.06, 0.06, 0.01), (0.85, 0.70, 0.25),
     [(M.PRECIOUS_METAL, 0.01), (M.ELECTRONICS, 0.03)]),
    ("ServoMotor", "ServoMotor", "Industrie-Servomotor",
     "Präziser Servomotor aus einer Fertigungsanlage. Neu kostet so etwas ein kleines Vermögen.",
     C.MOTOR, 12.0, 22000, False, 1, False, False, T.NONE, 1.0, CYLINDER, (0.25, 0.25, 0.35), (0.15, 0.30, 0.55),
     [(M.COPPER, 3.0), (M.ALUMINIUM, 4.0), (M.ELECTRONICS, 1.0)]),
    ("ReturnPalletElectronics", "ReturnPalletElectronics", "Retourenpalette Elektronik",
     "Eingeschweißte Rückläufer aus dem Versandhandel. Was drin ist, weiß man erst nach dem Öffnen.",
     C.PRODUCT, 22.0, 2700, False, 1, False, False, T.NONE, 1.0, CUBE, (0.80, 0.60, 0.50), (0.70, 0.60, 0.40), []),
    ("ReturnPalletTools", "ReturnPalletTools", "Retourenpalette Werkzeug",
     "Zurückgeschickte Werkzeuge, gemischt verpackt. Manches neu, manches kaputt.",
     C.PRODUCT, 18.0, 2300, False, 1, False, False, T.NONE, 1.0, CUBE, (0.70, 0.55, 0.45), (0.55, 0.50, 0.35), []),
    # Phase 8: better tools and a carrying aid.
    ("BitRatchet", "BitRatchet", "Bit-Ratsche mit Bitsatz",
     "Ratsche mit Bits für jede Schraube – deutlich schneller als ein einfacher Schraubendreher.",
     C.TOOL, 0.5, 2800, False, 1, True, False, T.SCREWDRIVER, 1.6, CUBE, (0.18, 0.05, 0.04), (0.15, 0.15, 0.20),
     [(M.STEEL, 0.4), (M.PLASTIC, 0.1)]),
    ("ProSideCutter", "ProSideCutter", "Profi-Seitenschneider",
     "Gehärtete Schneiden, lange Griffe: Kabel sind im Nu ab.",
     C.TOOL, 0.25, 2200, False, 1, True, False, T.SIDE_CUTTER, 1.8, CUBE, (0.16, 0.06, 0.02), (0.80, 0.15, 0.15),
     [(M.STEEL, 0.2), (M.PLASTIC, 0.05)]),
    ("DigitalMultimeter", "DigitalMultimeter", "Digitalmultimeter",
     "Misst schnell und genau – Fehlersuche in der halben Zeit.",
     C.TOOL, 0.35, 3500, False, 1, True, False, T.MULTIMETER, 2.0, CUBE, (0.10, 0.06, 0.04), (0.95, 0.45, 0.05),
     [(M.ELECTRONICS, 0.15), (M.PLASTIC, 0.2)]),
    ("SolderingStation", "SolderingStation", "Lötstation",
     "Temperaturgeregelte Lötstation – Kondensatoren tauschen geht doppelt so schnell.",
     C.POWER_TOOL, 1.8, 6000, False, 1, True, False, T.SOLDERING_IRON, 2.0, CUBE, (0.20, 0.15, 0.10), (0.25, 0.25, 0.30),
     [(M.STEEL, 0.6), (M.ELECTRONICS, 0.5), (M.PLASTIC, 0.7)]),
    ("HandTruck", "HandTruck", "Sackkarre (klappbar)",
     "Zusammengeklappt im Inventar, ausgeklappt trägt man damit 30 kg mehr – auch den schweren Elektromotor.",
     C.TOOL, 4.0, 5500, False, 1, True, False, T.NONE, 1.0, CUBE, (0.12, 0.40, 0.08), (0.20, 0.25, 0.65),
     [(M.STEEL, 3.5), (M.PLASTIC, 0.5)]),
    # Phase 9: machine materials (shredded material, sorted fractions, pressed bales) and maintenance.
    ("ShredElectronics", "ShredElectronics", "Schreddergut Elektronik",
     "Zerkleinerte Geräte: Platinen, Gehäusereste, Kabel – gemischt. Erst sortiert wird es wertvoll.",
     C.RAW_MATERIAL, 1.0, 40, True, 50, True, False, T.NONE, 1.0, CUBE, (0.14, 0.14, 0.08), (0.30, 0.32, 0.28),
     [(M.ELECTRONICS, 0.25), (M.STEEL, 0.5), (M.PLASTIC, 0.2), (M.COPPER, 0.05)]),
    ("ShredMetal", "ShredMetal", "Schreddergut Metall",
     "Zerkleinerter Metallschrott, noch nicht sortiert.",
     C.RAW_MATERIAL, 1.0, 15, True, 50, True, False, T.NONE, 1.0, CUBE, (0.14, 0.14, 0.08), (0.40, 0.38, 0.36),
     [(M.STEEL, 0.85), (M.ALUMINIUM, 0.1), (M.COPPER, 0.05)]),
    ("CopperGranulate", "CopperGranulate", "Kupfergranulat",
     "Sortiertes, sauberes Kupfer in Körnern – sehr gefragt.",
     C.RAW_MATERIAL, 0.5, 300, True, 40, True, False, T.NONE, 1.0, CUBE, (0.10, 0.10, 0.05), (0.85, 0.45, 0.20),
     [(M.COPPER, 0.5)]),
    ("AluminiumScrap", "AluminiumScrap", "Aluminiumschrott",
     "Sortierte Aluminiumteile, je Einheit etwa ein Kilogramm.",
     C.RAW_MATERIAL, 1.0, 120, True, 50, True, False, T.NONE, 1.0, CUBE, (0.16, 0.16, 0.06), (0.80, 0.82, 0.86),
     [(M.ALUMINIUM, 1.0)]),
    ("SteelBale", "SteelBale", "Stahlballen",
     "20 kg gepresster Stahlschrott – Schrotthändler zahlen dafür mehr als für lose Teile.",
     C.RAW_MATERIAL, 20.0, 700, False, 1, False, False, T.NONE, 1.0, CUBE, (0.40, 0.40, 0.30), (0.40, 0.40, 0.44),
     [(M.STEEL, 20.0)]),
    ("AluBale", "AluBale", "Aluminiumballen",
     "10 kg gepresstes Aluminium.",
     C.RAW_MATERIAL, 10.0, 1680, False, 1, False, False, T.NONE, 1.0, CUBE, (0.35, 0.35, 0.25), (0.82, 0.84, 0.88),
     [(M.ALUMINIUM, 10.0)]),
    ("CopperBale", "CopperBale", "Kupferpaket",
     "10 kg gepresstes Kupfergranulat – richtig was wert.",
     C.RAW_MATERIAL, 10.0, 7500, False, 1, False, False, T.NONE, 1.0, CUBE, (0.30, 0.30, 0.20), (0.80, 0.42, 0.18),
     [(M.COPPER, 10.0)]),
    ("MaintenanceKit", "MaintenanceKit", "Wartungsset",
     "Öl, Fett, Filter, Ersatzmesser: bringt eine Maschine wieder auf 100 %.",
     C.SPARE_PART, 1.5, 800, True, 5, True, False, T.NONE, 1.0, CUBE, (0.20, 0.12, 0.10), (0.85, 0.65, 0.10),
     [(M.STEEL, 1.0), (M.PLASTIC, 0.5)]),
    # Phase 10: products of the assembly machine (Fertigungsautomat).
    ("CopperWireSpool", "CopperWireSpool", "Kupferdrahtspule",
     "Zwei Kilo blanker Kupferdraht auf einer Spule – aus eigenem Granulat gewickelt.",
     C.PRODUCT, 2.0, 1500, True, 10, True, False, T.NONE, 1.0, CYLINDER, (0.22, 0.22, 0.14), (0.88, 0.48, 0.22),
     [(M.COPPER, 2.0)]),
    ("ExtensionCable", "ExtensionCable", "Verlängerungskabel (5 m)",
     "Fertig konfektioniertes Verlängerungskabel mit Stecker und Kupplung.",
     C.PRODUCT, 1.0, 900, True, 10, True, False, T.NONE, 1.0, CUBE, (0.20, 0.20, 0.08), (0.90, 0.90, 0.88),
     [(M.COPPER, 0.5), (M.PLASTIC, 0.5)]),
    ("SteelBracketSet", "SteelBracketSet", "Stahlwinkel-Set",
     "Zehn gestanzte Montagewinkel aus Recyclingstahl, verzinkt.",
     C.PRODUCT, 1.5, 220, True, 20, True, False, T.NONE, 1.0, CUBE, (0.16, 0.12, 0.06), (0.62, 0.64, 0.68),
     [(M.STEEL, 1.5)]),
    # Phase 11: too heavy for hands and hand truck - forklift job.
    ("IndustrialMotorLarge", "IndustrialMotorLarge", "Großer Industriemotor",
     "180 kg Drehstrommotor aus einer Fabrikhalle. Nur mit dem Gabelstapler zu bewegen; im Schredder wird viel Metall daraus.",
     C.MOTOR, 180.0, 9000, False, 1, False, False, T.NONE, 1.0, CYLINDER, (0.75, 0.75, 0.80), (0.25, 0.32, 0.38),
     [(M.STEEL, 140.0), (M.COPPER, 30.0), (M.ALUMINIUM, 10.0)]),
]

CARRY_BONUS = {"HandTruck": 30.0}

RARITY = {"RareGpu": unreal.WEItemRarity.RARE, "VintageRadio": unreal.WEItemRarity.RARE,
          "GoldCpu": unreal.WEItemRarity.VERY_RARE, "ServoMotor": unreal.WEItemRarity.RARE}

# Broken complex devices are worth much less than working ones, so that repairing pays off (Phase 6).
BROKEN_VALUE = {"OldComputer": 0.42, "CrtMonitor": 0.5}

items = {}
for (suffix, item_id, name, desc, cat, kg, cents, stack, max_stack, fits, repair, tool, speed, mesh, scale, tint, mats) in ITEMS:
    da, path = data_asset(ITEM_FOLDER, f"DA_Item_{suffix}", unreal.WEItemDefinition)
    da.set_editor_property("item_id", item_id)
    da.set_editor_property("display_name", unreal.Text(name))
    da.set_editor_property("description", unreal.Text(desc))
    da.set_editor_property("category", cat)
    da.set_editor_property("weight_kg", kg)
    money = unreal.WEMoney()
    money.set_editor_property("cents", cents)
    da.set_editor_property("base_value", money)
    da.set_editor_property("stackable", stack)
    da.set_editor_property("max_stack_size", max_stack)
    da.set_editor_property("fits_in_inventory", fits)
    da.set_editor_property("repairable", repair)
    da.set_editor_property("tool_type", tool)
    da.set_editor_property("tool_speed_multiplier", speed)
    da.set_editor_property("world_mesh", mesh)
    da.set_editor_property("world_scale", unreal.Vector(*scale))
    da.set_editor_property("tint", unreal.LinearColor(*tint, 1.0))
    amounts = []
    for (mtype, mkg) in mats:
        a = unreal.WEMaterialAmount()
        a.set_editor_property("material", mtype)
        a.set_editor_property("kilograms", mkg)
        amounts.append(a)
    da.set_editor_property("materials", amounts)
    da.set_editor_property("broken_value_multiplier", BROKEN_VALUE.get(suffix, 1.0))
    da.set_editor_property("rarity", RARITY.get(suffix, unreal.WEItemRarity.COMMON))
    da.set_editor_property("carry_bonus_kg", CARRY_BONUS.get(suffix, 0.0))
    items[suffix] = (da, path)


def out(item, lo, hi=None, chance=1.0, inherit=False):
    o = unreal.WEDisassemblyOutput()
    o.set_editor_property("item", items[item][0])
    o.set_editor_property("min_quantity", lo)
    o.set_editor_property("max_quantity", hi if hi is not None else lo)
    o.set_editor_property("chance", chance)
    o.set_editor_property("inherit_condition", inherit)
    return o


def step(text, tool, seconds, outputs):
    s = unreal.WEDisassemblyStep()
    s.set_editor_property("action_text", unreal.Text(text))
    s.set_editor_property("required_tool", tool)
    s.set_editor_property("base_seconds", seconds)
    s.set_editor_property("outputs", outputs)
    return s


# Disassembly sequences (§9.2). Raw materials do not inherit condition so they stack.
SEQUENCES = {
    "OldComputer": [
        step("Gehäuseschrauben lösen", T.SCREWDRIVER, 3.0, [out("SheetMetalScrews", 4, 6)]),
        step("Seitenabdeckung abnehmen", T.NONE, 1.5, [out("SteelScrap", 1)]),
        step("Grafikkarte ausbauen", T.SCREWDRIVER, 2.5, [out("GraphicsCard", 1, chance=0.85, inherit=True)]),
        step("Arbeitsspeicher entfernen", T.NONE, 1.5, [out("RamModule", 1, 2, inherit=True)]),
        step("Netzteil ausbauen", T.SCREWDRIVER, 3.0, [out("PowerSupply", 1, inherit=True)]),
        step("Mainboard ausbauen", T.SCREWDRIVER, 3.5, [out("Mainboard", 1, inherit=True), out("SheetMetalScrews", 2, 4)]),
        step("Kabel abtrennen und Gehäuse verwerten", T.SIDE_CUTTER, 3.0,
             [out("SteelScrap", 3, 4), out("PlasticParts", 1, 2), out("CopperCable", 1, 2)]),
    ],
    "CrtMonitor": [
        step("Rückwand abschrauben", T.SCREWDRIVER, 3.0, [out("SheetMetalScrews", 3, 5), out("PlasticParts", 2, 3)]),
        step("Platine ausbauen", T.SCREWDRIVER, 2.5, [out("ElectronicScrap", 2, 4)]),
        step("Ablenkspule abtrennen", T.SIDE_CUTTER, 2.5, [out("CopperCable", 2, 3)]),
        step("Bildröhre zerlegen", T.NONE, 4.0, [out("GlassCullet", 6, 8), out("SteelScrap", 1)]),
    ],
    "CableBox": [
        step("Kabel sortieren", T.NONE, 3.0, [out("CopperCable", 4, 6), out("PlasticParts", 1, 2)]),
    ],
}

for suffix, steps in SEQUENCES.items():
    dis, dis_path = data_asset(DIS_FOLDER, f"DIS_{suffix}", unreal.WEDisassemblyDefinition)
    dis.set_editor_property("steps", steps)
    eal.save_asset(dis_path)
    items[suffix][0].set_editor_property("disassembly", dis)


def fault(name, remove, install, part, tool, remove_s, install_s, weight, returns_part=True):
    f = unreal.WERepairFault()
    f.set_editor_property("display_name", unreal.Text(name))
    f.set_editor_property("remove_text", unreal.Text(remove))
    f.set_editor_property("install_text", unreal.Text(install))
    f.set_editor_property("part", items[part][0])
    f.set_editor_property("tool", tool)
    f.set_editor_property("remove_seconds", remove_s)
    f.set_editor_property("install_seconds", install_s)
    f.set_editor_property("weight", weight)
    f.set_editor_property("returns_broken_part", returns_part)
    return f


# Repair procedures (§9.5): diagnose (multimeter) -> exchange broken parts -> function test -> refurbish.
REPAIRS = {
    "OldComputer": dict(faults=[
        fault("Netzteil defekt", "Defektes Netzteil ausbauen", "Ersatz-Netzteil einbauen", "PowerSupply", T.SCREWDRIVER, 3.0, 3.0, 3.0),
        fault("Arbeitsspeicher defekt", "Defekten Arbeitsspeicher ziehen", "Ersatz-Arbeitsspeicher einsetzen", "RamModule", T.NONE, 1.5, 1.5, 3.0),
        fault("Grafikkarte defekt", "Defekte Grafikkarte ausbauen", "Ersatz-Grafikkarte einbauen", "GraphicsCard", T.SCREWDRIVER, 2.5, 2.5, 1.0),
        fault("Mainboard defekt", "Defektes Mainboard ausbauen", "Ersatz-Mainboard einbauen", "Mainboard", T.SCREWDRIVER, 4.0, 4.0, 1.0),
    ], diagnose_s=4.0, test_s=3.0),
    "CrtMonitor": dict(faults=[
        fault("Kondensatoren aufgebläht", "Alte Kondensatoren auslöten", "Neue Kondensatoren einlöten", "Capacitors", T.SOLDERING_IRON, 4.0, 4.0, 3.0, False),
        fault("Netzkabel gebrochen", "Kaputtes Netzkabel abtrennen", "Neues Kabel anschließen", "CopperCable", T.SIDE_CUTTER, 2.0, 2.0, 2.0, False),
    ], diagnose_s=3.0, test_s=2.0),
}

for suffix, spec in REPAIRS.items():
    rep, rep_path = data_asset(REP_FOLDER, f"REP_{suffix}", unreal.WERepairDefinition)
    rep.set_editor_property("faults", spec["faults"])
    rep.set_editor_property("diagnose_tool", T.MULTIMETER)
    rep.set_editor_property("diagnose_seconds", spec["diagnose_s"])
    rep.set_editor_property("test_tool", T.NONE)
    rep.set_editor_property("test_seconds", spec["test_s"])
    rep.set_editor_property("refurbish_material", items["CleaningKit"][0])
    rep.set_editor_property("repair_quality_gain", 15)
    rep.set_editor_property("refurbish_quality_gain", 30)
    eal.save_asset(rep_path)
    items[suffix][0].set_editor_property("repair", rep)

for suffix, (da, path) in items.items():
    eal.save_asset(path)

unreal.log(f"WE_ITEMS_BUILT {len(items)} items, {len(SEQUENCES)} disassembly sequences, {len(REPAIRS)} repair procedures")
unreal.SystemLibrary.quit_editor()
