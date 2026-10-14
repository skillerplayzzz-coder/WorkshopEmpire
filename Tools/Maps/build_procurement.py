"""
Creates/updates the loot tables (/Game/WorkshopEmpire/Data/Loot) for Phase 7 procurement:
  LT_ReturnsElectronics, LT_ReturnsTools  (contents of the return pallets; set as Contents on the pallet items)
  LT_ScrapPile                            (what appears on the scrap heap behind the garage)

Run after build_item_definitions.py and before build_economy.py:
  UnrealEditor-Cmd.exe <WorkshopEmpire.uproject> -ExecutePythonScript="<this file>" -unattended -RenderOffscreen -nosplash

Weights are first-pass balancing; the test Economy.BalanceReturnPallets measures the expected value.
Rare finds are rare on purpose: they add excitement, progress must not depend on them (spec §11).
"""
import unreal

FOLDER = "/Game/WorkshopEmpire/Data/Loot"
ITEMS = "/Game/WorkshopEmpire/Data/Items"
Q = unreal.WEItemCondition
eal = unreal.EditorAssetLibrary
tools = unreal.AssetToolsHelpers.get_asset_tools()


def item(suffix):
    return unreal.load_asset(f"{ITEMS}/DA_Item_{suffix}")


def data_asset(name, cls):
    path = f"{FOLDER}/{name}"
    if eal.does_asset_exist(path):
        return eal.load_asset(path), path
    factory = unreal.DataAssetFactory()
    factory.set_editor_property("data_asset_class", cls)
    return tools.create_asset(name, FOLDER, cls, factory), path


def entry(suffix, weight, best, worst, qmin, qmax, lo=1, hi=1):
    e = unreal.WELootEntry()
    e.set_editor_property("item", item(suffix))
    e.set_editor_property("weight", weight)
    e.set_editor_property("min_quantity", lo)
    e.set_editor_property("max_quantity", hi)
    e.set_editor_property("best_condition", best)
    e.set_editor_property("worst_condition", worst)
    e.set_editor_property("min_quality", qmin)
    e.set_editor_property("max_quality", qmax)
    return e


TABLES = {
    "LT_ReturnsElectronics": (3, 5, [
        entry("OldComputer", 3.0, Q.USED, Q.DEFECTIVE, 30, 70),
        entry("CrtMonitor", 2.0, Q.USED, Q.DEFECTIVE, 30, 65),
        entry("RamModule", 3.0, Q.USED, Q.DAMAGED, 35, 75, 1, 2),
        entry("PowerSupply", 3.0, Q.USED, Q.DEFECTIVE, 30, 70),
        entry("GraphicsCard", 2.0, Q.USED, Q.DEFECTIVE, 30, 70),
        entry("Mainboard", 1.5, Q.USED, Q.DEFECTIVE, 30, 70),
        entry("CopperCable", 2.0, Q.USED, Q.USED, 40, 70, 3, 6),
        entry("Capacitors", 1.0, Q.NEW, Q.USED, 60, 90, 1, 3),
        entry("RareGpu", 0.15, Q.USED, Q.DAMAGED, 40, 80),
        entry("GoldCpu", 0.10, Q.USED, Q.DAMAGED, 40, 80),
    ]),
    "LT_ReturnsTools": (3, 4, [
        entry("Screwdriver", 3.0, Q.NEW, Q.DAMAGED, 40, 85),
        entry("SideCutter", 3.0, Q.NEW, Q.DAMAGED, 40, 85),
        entry("CordlessDrill", 2.0, Q.VERY_GOOD, Q.DEFECTIVE, 35, 80),
        entry("Multimeter", 2.0, Q.VERY_GOOD, Q.DEFECTIVE, 35, 80),
        entry("SolderingIron", 1.5, Q.VERY_GOOD, Q.DEFECTIVE, 35, 80),
        entry("Toolbox", 1.0, Q.USED, Q.DAMAGED, 40, 70),
        entry("CleaningKit", 2.0, Q.NEW, Q.NEW, 70, 90, 1, 3),
        entry("ServoMotor", 0.15, Q.USED, Q.DAMAGED, 40, 75),
    ]),
    # The scrap heap rolls single entries (RollOne); MinRolls/MaxRolls are not used there.
    "LT_ScrapPile": (1, 1, [
        entry("CableBox", 4.0, Q.SCRAP, Q.SCRAP, 10, 30),
        entry("SteelScrap", 4.0, Q.SCRAP, Q.SCRAP, 10, 30, 2, 5),
        entry("OldComputer", 2.0, Q.DEFECTIVE, Q.SCRAP, 10, 35),
        entry("CrtMonitor", 1.5, Q.DEFECTIVE, Q.SCRAP, 10, 35),
        entry("PlasticParts", 2.0, Q.SCRAP, Q.SCRAP, 10, 30, 2, 5),
        entry("ElectronicScrap", 2.0, Q.SCRAP, Q.SCRAP, 10, 30, 1, 3),
        entry("VintageRadio", 0.08, Q.DAMAGED, Q.DEFECTIVE, 20, 45),
        entry("RareGpu", 0.04, Q.DEFECTIVE, Q.DEFECTIVE, 20, 40),
    ]),
}

for name, (lo, hi, entries) in TABLES.items():
    table, path = data_asset(name, unreal.WELootTable)
    table.set_editor_property("entries", entries)
    table.set_editor_property("min_rolls", lo)
    table.set_editor_property("max_rolls", hi)
    eal.save_asset(path)

for pallet, table in (("ReturnPalletElectronics", "LT_ReturnsElectronics"), ("ReturnPalletTools", "LT_ReturnsTools")):
    da = item(pallet)
    da.set_editor_property("contents", unreal.load_asset(f"{FOLDER}/{table}"))
    eal.save_asset(f"{ITEMS}/DA_Item_{pallet}")

unreal.log(f"WE_PROCUREMENT_BUILT {len(TABLES)} loot tables")
unreal.SystemLibrary.quit_editor()
