"""
Creates/updates the item definition Data Assets in /Game/WorkshopEmpire/Data/Items (one per item type).
Existing assets are updated in place (references from maps stay valid).

Run after build_placeholder_material.py and before build_garage_map.py:
  UnrealEditor-Cmd.exe <WorkshopEmpire.uproject> -ExecutePythonScript="<this file>" -unattended -RenderOffscreen -nosplash

Values are first-pass placeholders; balancing happens in Phase 4/16.
"""
import unreal

FOLDER = "/Game/WorkshopEmpire/Data/Items"
CUBE = unreal.load_asset("/Engine/BasicShapes/Cube.Cube")
CYLINDER = unreal.load_asset("/Engine/BasicShapes/Cylinder.Cylinder")

C = unreal.WEItemCategory
M = unreal.WEMaterialType
R = unreal.WEItemRarity

tools = unreal.AssetToolsHelpers.get_asset_tools()
eal = unreal.EditorAssetLibrary

# asset name, ItemId, name, description, category, kg, value (cents), stackable, max stack, fits inventory,
# repairable, disassemblable, mesh, scale, tint, materials [(type, kg)]
ITEMS = [
    ("DA_Item_OldComputer", "OldComputer", "Alter Computer",
     "Ein ausgedienter Büro-PC. Das Gehäuse ist verbeult, aber vielleicht steckt noch etwas Brauchbares drin.",
     C.ELECTRONICS, 9.0, 4000, False, 1, False, True, True, CUBE, (0.45, 0.20, 0.42), (0.72, 0.69, 0.60),
     [(M.STEEL, 5.0), (M.PLASTIC, 1.5), (M.ELECTRONICS, 2.0), (M.COPPER, 0.3), (M.PRECIOUS_METAL, 0.01)]),
    ("DA_Item_CrtMonitor", "CrtMonitor", "Röhrenmonitor",
     "Schwer, staubig und längst aus der Mode. Kupfer und Glas lassen sich vielleicht verwerten.",
     C.ELECTRONICS, 14.0, 1500, False, 1, False, True, True, CUBE, (0.40, 0.40, 0.38), (0.15, 0.15, 0.16),
     [(M.GLASS, 8.0), (M.PLASTIC, 3.0), (M.COPPER, 1.0), (M.ELECTRONICS, 1.0)]),
    ("DA_Item_Screwdriver", "Screwdriver", "Schraubendreher",
     "Ein einfacher Kreuzschlitz-Schraubendreher. Der Griff ist abgenutzt.",
     C.TOOL, 0.2, 600, False, 1, True, False, False, CYLINDER, (0.03, 0.03, 0.22), (0.75, 0.10, 0.08),
     [(M.STEEL, 0.15), (M.PLASTIC, 0.05)]),
    ("DA_Item_Toolbox", "Toolbox", "Werkzeugkiste",
     "Eine Metallkiste für Handwerkzeug. Im Moment fast leer.",
     C.TOOL, 6.0, 2500, False, 1, False, False, False, CUBE, (0.45, 0.22, 0.22), (0.10, 0.25, 0.65),
     [(M.STEEL, 6.0)]),
    ("DA_Item_ElectricMotor", "ElectricMotor", "Alter Elektromotor",
     "Ein schwerer Drehstrommotor. Zum Tragen braucht es später einen Hubwagen oder Hilfe.",
     C.MOTOR, 45.0, 6000, False, 1, False, True, True, CYLINDER, (0.35, 0.35, 0.45), (0.25, 0.35, 0.28),
     [(M.IRON, 30.0), (M.COPPER, 8.0), (M.ALUMINIUM, 5.0)]),
    ("DA_Item_CableBox", "CableBox", "Karton mit Kabeln",
     "Ein Karton voller alter Kabel. Gutes Kupfer, wenn man es abisoliert.",
     C.SCRAP_METAL, 3.0, 800, False, 1, False, False, True, CUBE, (0.35, 0.30, 0.25), (0.55, 0.42, 0.26),
     [(M.COPPER, 1.6), (M.PLASTIC, 1.2)]),
    ("DA_Item_SheetMetalScrews", "SheetMetalScrews", "Blechschrauben",
     "Kleine verzinkte Schrauben. Braucht man ständig.",
     C.SPARE_PART, 0.005, 5, True, 200, True, False, False, CUBE, (0.10, 0.07, 0.04), (0.70, 0.72, 0.75),
     [(M.STEEL, 0.005)]),
    ("DA_Item_CopperCable", "CopperCable", "Kupferkabel",
     "Ein abisoliertes Stück Kupferkabel, etwa ein Meter.",
     C.RAW_MATERIAL, 0.25, 120, True, 20, True, False, False, CYLINDER, (0.05, 0.05, 0.30), (0.72, 0.38, 0.18),
     [(M.COPPER, 0.25)]),
    ("DA_Item_RamModule", "RamModule", "Arbeitsspeicher (4 GB)",
     "Ein DDR3-Speicherriegel. Getestet ist er nicht.",
     C.ELECTRONICS, 0.03, 800, False, 1, True, False, False, CUBE, (0.14, 0.04, 0.02), (0.10, 0.45, 0.20),
     [(M.ELECTRONICS, 0.025), (M.PRECIOUS_METAL, 0.0005)]),
]

factory = unreal.DataAssetFactory()
factory.set_editor_property("data_asset_class", unreal.WEItemDefinition)

for (asset, item_id, name, desc, cat, kg, cents, stack, max_stack, fits, repair, disasm, mesh, scale, tint, mats) in ITEMS:
    path = f"{FOLDER}/{asset}"
    da = eal.load_asset(path) if eal.does_asset_exist(path) else tools.create_asset(asset, FOLDER, unreal.WEItemDefinition, factory)
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
    da.set_editor_property("disassemblable", disasm)
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
    eal.save_asset(path)

unreal.log(f"WE_ITEMS_BUILT {len(ITEMS)}")
unreal.SystemLibrary.quit_editor()
