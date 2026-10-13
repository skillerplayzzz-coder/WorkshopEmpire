"""
Creates/updates the economy Data Assets in /Game/WorkshopEmpire/Data/Economy:
  DA_Trader_Scrapyard, DA_Trader_UsedGoods, DA_Trader_ToolShop, DA_Trader_SpareParts  (what traders buy and sell)
  DA_ContractCatalog                                            (customer order templates)

Run after build_item_definitions.py and before build_garage_map.py:
  UnrealEditor-Cmd.exe <WorkshopEmpire.uproject> -ExecutePythonScript="<this file>" -unattended -RenderOffscreen -nosplash

Prices derive from item base values (see build_item_definitions.py), condition, quality and market demand.
All numbers are first-pass balancing (Phase 16 tunes them with playtests).
"""
import unreal

FOLDER = "/Game/WorkshopEmpire/Data/Economy"
ITEMS = "/Game/WorkshopEmpire/Data/Items"
C = unreal.WEItemCategory
Q = unreal.WEItemCondition

tools = unreal.AssetToolsHelpers.get_asset_tools()
eal = unreal.EditorAssetLibrary


def item(suffix):
    return unreal.load_asset(f"{ITEMS}/DA_Item_{suffix}")


def data_asset(name, cls):
    path = f"{FOLDER}/{name}"
    if eal.does_asset_exist(path):
        return eal.load_asset(path), path
    factory = unreal.DataAssetFactory()
    factory.set_editor_property("data_asset_class", cls)
    return tools.create_asset(name, FOLDER, cls, factory), path


def rate(category, multiplier):
    r = unreal.WETraderCategoryRate()
    r.set_editor_property("category", category)
    r.set_editor_property("multiplier", multiplier)
    return r


def stock(suffix, quantity, condition, quality, price=1.0):
    s = unreal.WETraderStockEntry()
    s.set_editor_property("item", item(suffix))
    s.set_editor_property("quantity", quantity)
    s.set_editor_property("condition", condition)
    s.set_editor_property("quality", quality)
    s.set_editor_property("price_multiplier", price)
    return s


TRADERS = [
    ("DA_Trader_Scrapyard", "Schrotthandel Kowalski", 15.0,
     [rate(C.RAW_MATERIAL, 1.1), rate(C.SCRAP_METAL, 1.2), rate(C.SPARE_PART, 0.6), rate(C.MOTOR, 0.8)],
     [stock("CableBox", 3, Q.SCRAP, 20, 0.8), stock("ElectricMotor", 1, Q.DEFECTIVE, 30, 0.9)]),
    ("DA_Trader_UsedGoods", "Gebrauchtbörse 24", 10.0,
     [rate(C.ELECTRONICS, 1.0), rate(C.SPARE_PART, 1.0), rate(C.HOUSEHOLD_APPLIANCE, 1.0),
      rate(C.TOOL, 0.8), rate(C.POWER_TOOL, 0.8), rate(C.COLLECTIBLE, 1.2), rate(C.RARE_FIND, 1.2)],
     [stock("OldComputer", 4, Q.DEFECTIVE, 35), stock("CrtMonitor", 2, Q.DAMAGED, 40), stock("RamModule", 3, Q.USED, 55)]),
    ("DA_Trader_ToolShop", "Werkzeug-Versand Hammerl", 30.0,
     [rate(C.TOOL, 0.7), rate(C.POWER_TOOL, 0.7)],
     [stock("Screwdriver", 5, Q.NEW, 80), stock("SideCutter", 3, Q.NEW, 80), stock("CordlessDrill", 2, Q.NEW, 85),
      stock("Multimeter", 3, Q.NEW, 80), stock("SolderingIron", 2, Q.NEW, 80)]),
    # Phase 6: spare parts for repairs (buying is convenient; parts from another device are cheaper).
    ("DA_Trader_SpareParts", "Ersatzteil-Express", 20.0,
     [rate(C.SPARE_PART, 0.9), rate(C.ELECTRONICS, 0.9)],
     [stock("PowerSupply", 3, Q.USED, 55), stock("RamModule", 4, Q.USED, 55), stock("GraphicsCard", 1, Q.USED, 50), stock("Mainboard", 1, Q.USED, 50),
      stock("Capacitors", 10, Q.NEW, 80), stock("CleaningKit", 10, Q.NEW, 80)]),
]

for name, display, restock, buys, offers in TRADERS:
    da, path = data_asset(name, unreal.WETraderDefinition)
    da.set_editor_property("display_name", unreal.Text(display))
    da.set_editor_property("buys", buys)
    da.set_editor_property("stock", offers)
    da.set_editor_property("restock_minutes", restock)
    eal.save_asset(path)


def template(title, suffix, lo, hi, min_condition, factor, minutes, repair_from=None):
    t = unreal.WEContractTemplate()
    t.set_editor_property("title", unreal.Text(title))
    if repair_from is not None:
        # Repair order: the customer's device arrives in condition repair_from, must come back min_condition.
        t.set_editor_property("kind", unreal.WEContractKind.REPAIR)
        t.set_editor_property("customer_condition", repair_from)
    t.set_editor_property("item", item(suffix))
    t.set_editor_property("min_quantity", lo)
    t.set_editor_property("max_quantity", hi)
    t.set_editor_property("min_condition", min_condition)
    t.set_editor_property("reward_factor", factor)
    t.set_editor_property("duration_minutes", minutes)
    return t


catalog, catalog_path = data_asset("DA_ContractCatalog", unreal.WEContractCatalog)
catalog.set_editor_property("templates", [
    template("Bastler sucht Arbeitsspeicher", "RamModule", 1, 2, Q.DAMAGED, 1.6, 20.0),
    template("Schüler braucht eine Grafikkarte", "GraphicsCard", 1, 1, Q.DAMAGED, 1.5, 25.0),
    template("Elektriker sucht Kupferkabel", "CopperCable", 4, 8, Q.USED, 1.5, 20.0),
    template("Nachbarwerkstatt braucht Schrauben", "SheetMetalScrews", 20, 40, Q.USED, 1.6, 15.0),
    template("PC-Notdienst sucht ein Netzteil", "PowerSupply", 1, 1, Q.DAMAGED, 1.5, 25.0),
    template("Recyclinghof nimmt Stahlschrott", "SteelScrap", 5, 10, Q.SCRAP, 1.3, 30.0),
    template("Retro-Sammler sucht ein Mainboard", "Mainboard", 1, 1, Q.DEFECTIVE, 1.7, 30.0),
    # Repair orders (Phase 6): reward = value of the repaired device x factor (labour fee).
    template("Kundin bringt ihren PC: startet nicht mehr", "OldComputer", 1, 1, Q.USED, 0.45, 30.0, repair_from=Q.DEFECTIVE),
    template("Büro-PC zur Reparatur", "OldComputer", 1, 1, Q.USED, 0.45, 30.0, repair_from=Q.DAMAGED),
    template("Röhrenmonitor flackert – bitte reparieren", "CrtMonitor", 1, 1, Q.USED, 0.6, 30.0, repair_from=Q.DAMAGED),
])
eal.save_asset(catalog_path)

unreal.log(f"WE_ECONOMY_BUILT {len(TRADERS)} traders, catalog")
unreal.SystemLibrary.quit_editor()
