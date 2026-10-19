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
            tier=0, staff=0, inventory_slots=0, tools_weightless=False):
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
    u.set_editor_property("inventory_extra_slots", inventory_slots)
    u.set_editor_property("tools_weightless", tools_weightless)
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
    # Phase 19: tools stop taking weight and there is room for them.
    upgrade("ToolBag", "Werkzeugtasche", "Für jeden eine Werkzeugtasche: +5 Inventarplätze, Werkzeuge wiegen nichts mehr.", 120, inventory_slots=5, tools_weightless=True),
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
    upgrade("OldVan", "Alter Transporter", "Firmenwagen vor dem Tor (20 Plätze, 600 kg): Verkaufsfahrten und Großeinkäufe – braucht einen Fahrer.", 2000,
            unlocks=True),
    upgrade("ModernVan", "Moderner Lieferwagen", "Ersetzt den alten Transporter: 30 Plätze, 1.200 kg, Touren 60 % schneller.", 4500,
            requires="OldVan"),
    # Phase 16: the big lots of the halls need a truck (tours 2.4x as fast as the old van, 40 places, 3 t).
    upgrade("Truck", "Lkw mit Ladekran", "Ersetzt den Lieferwagen: 40 Plätze, 3 t Nutzlast, Touren 2,4-mal so schnell wie mit dem alten Transporter. Für Großauflösungen und Fabrikauflösungen.", 12000,
            requires="CommercialHall"),
    # Phase 12: buildings - each lifts the company one tier (map actors tagged "WEUpgrade.<Id>" appear).
    upgrade("WorkshopExtension", "Werkstatt-Erweiterung", "Zweite Halle neben der Garage (8 × 7 m) mit eigenem Tor. Stufe \"Große Werkstatt\": +2 Mitarbeiter, Verpackungsanlage, erste Industrieaufträge, größere Abholtouren.", 5000,
            unlocks=True, power_kw=5.5, tier=2, staff=2),
    upgrade("CommercialHall", "Gewerbehalle", "14 × 14 m Halle auf dem Hof, 6 m hoch, eigener 22-kW-Anschluss. Stufe \"Gewerbehalle\": +4 Mitarbeiter, Schmelzofen, Hochregal, Barren-Aufträge, Großauflösungen.", 25000,
            requires="WorkshopExtension", unlocks=True, power_kw=22.0, tier=3, staff=4),
    upgrade("FactoryHall", "Fabrikhalle", "Große Fabrikhalle (14 × 16 m) mit Industrieanschluss (+44 kW). Stufe \"Fabrik\": +8 Mitarbeiter. Fabrikauflösungen, Edelmetall-Rückgewinnung. Mit 250.000 € Firmenwert wird daraus ein Industrieunternehmen.", 75000,
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

# --- Phase 16: milestones - long-term goals with a reward, up to a company worth a million ---------------
MK = unreal.WEMilestoneKind


def milestone(mid, title, desc, kind, target, euros):
    m = unreal.WEMilestoneDefinition()
    m.set_editor_property("id", mid)
    m.set_editor_property("title", unreal.Text(title))
    m.set_editor_property("description", unreal.Text(desc))
    m.set_editor_property("kind", kind)
    m.set_editor_property("target", int(target))
    reward = unreal.WEMoney()
    reward.set_editor_property("cents", int(euros * 100))
    m.set_editor_property("reward", reward)
    return m


EUR = 100  # money targets are in cents
MILESTONES = [
    milestone("Dis5", "Schrauber", "5 Geräte vollständig zerlegt.", MK.DEVICES_DISASSEMBLED, 5, 25),
    milestone("Dis50", "Ausschlachter", "50 Geräte vollständig zerlegt.", MK.DEVICES_DISASSEMBLED, 50, 100),
    milestone("Dis250", "Zerlegemeister", "250 Geräte vollständig zerlegt.", MK.DEVICES_DISASSEMBLED, 250, 300),
    milestone("Rep1", "Es lebt!", "Das erste Gerät repariert.", MK.DEVICES_REPAIRED, 1, 20),
    milestone("Rep25", "Reparaturprofi", "25 Geräte repariert.", MK.DEVICES_REPAIRED, 25, 150),
    milestone("Rep100", "Wiederbeleber", "100 Geräte repariert.", MK.DEVICES_REPAIRED, 100, 400),
    milestone("Rep500", "Legende der Werkbank", "500 Geräte repariert.", MK.DEVICES_REPAIRED, 500, 2000),
    milestone("Ord5", "Zuverlässig", "5 Kundenaufträge erledigt.", MK.ORDERS_COMPLETED, 5, 60),
    milestone("Ord25", "Stammkunden", "25 Kundenaufträge erledigt.", MK.ORDERS_COMPLETED, 25, 300),
    milestone("Ord100", "Bekannt in der Stadt", "100 Kundenaufträge erledigt.", MK.ORDERS_COMPLETED, 100, 1500),
    milestone("Ind5", "Industriepartner", "5 Industrieaufträge oder Lieferverträge erfüllt.", MK.INDUSTRIAL_ORDERS, 5, 1000),
    milestone("Ind25", "Systemlieferant", "25 Industrieaufträge oder Lieferverträge erfüllt.", MK.INDUSTRIAL_ORDERS, 25, 4000),
    milestone("Mac100", "Es läuft", "100 Maschinenvorgänge.", MK.MACHINE_CYCLES, 100, 100),
    milestone("Mac1000", "Taktgeber", "1.000 Maschinenvorgänge.", MK.MACHINE_CYCLES, 1000, 600),
    milestone("Mac10000", "Dauerbetrieb", "10.000 Maschinenvorgänge.", MK.MACHINE_CYCLES, 10000, 4000),
    milestone("Tour10", "Auf Achse", "10 Touren mit dem Firmenwagen.", MK.TOURS_COMPLETED, 10, 150),
    milestone("Tour100", "Fuhrpark-Chef", "100 Touren gefahren.", MK.TOURS_COMPLETED, 100, 1500),
    milestone("Rev1k", "Erste Tausend", "1.000 € Umsatz aus Verkäufen und Aufträgen.", MK.REVENUE, 1000 * EUR, 50),
    milestone("Rev10k", "Fünfstellig", "10.000 € Umsatz.", MK.REVENUE, 10000 * EUR, 300),
    milestone("Rev100k", "Sechsstellig", "100.000 € Umsatz.", MK.REVENUE, 100000 * EUR, 2000),
    milestone("Rev1m", "Umsatzmillionär", "1.000.000 € Umsatz.", MK.REVENUE, 1000000 * EUR, 15000),
    milestone("Tier2", "Raus aus der Garage", "Stufe Große Werkstatt erreicht.", MK.COMPANY_TIER, 2, 300),
    milestone("Tier3", "Gewerbetreibend", "Stufe Gewerbehalle erreicht.", MK.COMPANY_TIER, 3, 1500),
    milestone("Tier4", "Fabrikant", "Stufe Fabrik erreicht.", MK.COMPANY_TIER, 4, 4000),
    milestone("Tier5", "Industrieunternehmen", "Höchste Stufe erreicht.", MK.COMPANY_TIER, 5, 10000),
    milestone("Value500k", "Halbe Million", "Firmenwert 500.000 €.", MK.COMPANY_VALUE, 500000 * EUR, 15000),
    milestone("Value1m", "Werkstatt-Imperium", "Firmenwert 1.000.000 € - das Ziel eines Lebenswerks.", MK.COMPANY_VALUE, 1000000 * EUR, 0),
    milestone("Staff3", "Arbeitgeber", "3 Mitarbeiter beschäftigt.", MK.EMPLOYEES, 3, 150),
    milestone("Staff10", "Mittelstand", "10 Mitarbeiter beschäftigt.", MK.EMPLOYEES, 10, 1500),
    milestone("Rare1", "Glücksgriff", "Ein Sammlerstück verkauft oder an einen Sammler geliefert.", MK.RARE_FINDS, 1, 100),
    milestone("Rare4", "Sammlervitrine", "Jedes der 4 Sammlerstücke einmal verkauft oder geliefert.", MK.RARE_FINDS, 4, 1500),
]
mpath = f"{FOLDER}/DA_MilestoneCatalog"
if eal.does_asset_exist(mpath):
    mcat = eal.load_asset(mpath)
else:
    mfactory = unreal.DataAssetFactory()
    mfactory.set_editor_property("data_asset_class", unreal.WEMilestoneCatalog)
    mcat = tools.create_asset("DA_MilestoneCatalog", FOLDER, unreal.WEMilestoneCatalog, mfactory)
mcat.set_editor_property("milestones", MILESTONES)
eal.save_asset(mpath)

unreal.log(f"WE_PROGRESSION_BUILT {len(UPGRADES)} upgrades, {len(MILESTONES)} milestones")
unreal.SystemLibrary.quit_editor()
