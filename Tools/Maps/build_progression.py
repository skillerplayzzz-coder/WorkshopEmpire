"""
Creates/updates the workshop upgrade catalog /Game/WorkshopEmpire/Data/Progression/DA_UpgradeCatalog.

Run after build_economy.py and before build_garage_map.py:
  UnrealEditor-Cmd.exe <WorkshopEmpire.uproject> -ExecutePythonScript="<this file>" -unattended -RenderOffscreen -nosplash

Prices are first-pass balancing against ~6 EUR profit per disassembled PC (Phase 4 measurement).
"""
import unreal

FOLDER = "/Game/WorkshopEmpire/Data/Progression"
NAME = "DA_UpgradeCatalog"
eal = unreal.EditorAssetLibrary
tools = unreal.AssetToolsHelpers.get_asset_tools()


def upgrade(uid, name, desc, euros, requires=None, bench_slots=0, inventory_kg=0.0, work_speed=1.0, light=1.0, unlocks=False):
    u = unreal.WEUpgradeDefinition()
    u.set_editor_property("id", uid)
    u.set_editor_property("display_name", unreal.Text(name))
    u.set_editor_property("description", unreal.Text(desc))
    cost = unreal.WEMoney()
    cost.set_editor_property("cents", int(euros * 100))
    u.set_editor_property("cost", cost)
    u.set_editor_property("required_upgrade", requires or "None")
    u.set_editor_property("bench_extra_slots", bench_slots)
    u.set_editor_property("inventory_extra_kg", inventory_kg)
    u.set_editor_property("work_speed_multiplier", work_speed)
    u.set_editor_property("light_multiplier", light)
    u.set_editor_property("unlocks_actors", unlocks)
    return u


UPGRADES = [
    upgrade("LedLights", "LED-Werkstattbeleuchtung", "Hellere Lampen – man sieht endlich, was man schraubt.", 30, light=2.0),
    upgrade("BenchCabinet", "Werkbank-Unterschrank", "+6 Plätze im Werkbanklager.", 35, bench_slots=6),
    upgrade("SecondShelf", "Zweites Regal", "Ein weiteres Schwerlastregal mit 15 Plätzen neben dem Tor.", 60, unlocks=True),
    upgrade("ToolBelt", "Werkzeuggürtel und Tragegurt", "+10 kg Tragkraft im Inventar für alle Mitarbeiter.", 70, inventory_kg=10.0),
    upgrade("WorkMat", "Arbeitsmatte und Magnetschale", "Schrauben gehen nicht mehr verloren: Zerlegen 25 % schneller.", 80,
            requires="BenchCabinet", work_speed=1.25),
    upgrade("ProBench", "Profi-Werkbankausstattung", "Druckluft, Ablagen und Licht: Zerlegen nochmals 25 % schneller, +6 Plätze.", 220,
            requires="WorkMat", bench_slots=6, work_speed=1.25),
]

path = f"{FOLDER}/{NAME}"
if eal.does_asset_exist(path):
    catalog = eal.load_asset(path)
else:
    factory = unreal.DataAssetFactory()
    factory.set_editor_property("data_asset_class", unreal.WEUpgradeCatalog)
    catalog = tools.create_asset(NAME, FOLDER, unreal.WEUpgradeCatalog, factory)
catalog.set_editor_property("upgrades", UPGRADES)
eal.save_asset(path)

unreal.log(f"WE_PROGRESSION_BUILT {len(UPGRADES)} upgrades")
unreal.SystemLibrary.quit_editor()
