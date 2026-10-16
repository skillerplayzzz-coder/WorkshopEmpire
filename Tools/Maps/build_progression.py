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


def upgrade(uid, name, desc, euros, requires=None, bench_slots=0, inventory_kg=0.0, work_speed=1.0, light=1.0, unlocks=False, power_kw=0.0,
            tier=0, staff=0):
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
    u.set_editor_property("power_capacity_kw", power_kw)
    u.set_editor_property("company_tier", tier)
    u.set_editor_property("staff_slots", staff)
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
    # Phase 9: more power for machines (11 kW house connection -> 22 kW).
    upgrade("HeavyCurrent", "Starkstromanschluss", "Zweiter Drehstromanschluss: +11 kW für Maschinen – Schredder und Sortiermaschine laufen gleichzeitig.", 300,
            power_kw=11.0),
    # Phase 11: logistics. The van and the forklift stand in the map and appear once bought.
    upgrade("Forklift", "Gabelstapler", "Gebrauchter Elektro-Gabelstapler: hebt bis 1.500 kg – für Industriemotoren, Ballen und Paletten.", 900,
            unlocks=True),
    upgrade("OldVan", "Alter Transporter", "Firmenwagen vor dem Tor (20 Plätze, 600 kg): Verkaufsfahrten und Großeinkäufe – braucht einen Fahrer.", 2400,
            unlocks=True),
    upgrade("ModernVan", "Moderner Lieferwagen", "Ersetzt den alten Transporter: 30 Plätze, 1.200 kg, Touren 60 % schneller.", 4500,
            requires="OldVan"),
    # Phase 12: buildings - each lifts the company one tier (map actors tagged "WEUpgrade.<Id>" appear).
    upgrade("WorkshopExtension", "Werkstatt-Erweiterung", "Zweite Halle neben der Garage (8 × 7 m) mit eigenem Tor. Stufe \"Große Werkstatt\": +2 Mitarbeiter, Verpackungsanlage, erste Industrieaufträge.", 6000,
            unlocks=True, power_kw=5.5, tier=2, staff=2),
    upgrade("CommercialHall", "Gewerbehalle", "14 × 14 m Halle auf dem Hof, 6 m hoch, eigener 22-kW-Anschluss. Stufe \"Gewerbehalle\": +4 Mitarbeiter, Schmelzofen, Hochregal, Barren-Aufträge.", 25000,
            requires="WorkshopExtension", unlocks=True, power_kw=22.0, tier=3, staff=4),
    upgrade("FactoryHall", "Fabrikhalle", "Große Fabrikhalle (14 × 16 m) mit Industrieanschluss (+44 kW). Stufe \"Fabrik\": +8 Mitarbeiter. Mit 250.000 € Firmenwert wird daraus ein Industrieunternehmen.", 90000,
            requires="CommercialHall", unlocks=True, power_kw=44.0, tier=4, staff=8),
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
