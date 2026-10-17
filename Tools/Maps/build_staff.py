"""
Creates/updates the staff and tour data (/Game/WorkshopEmpire/Data/Staff, Phase 11):
  DA_StaffCatalog - roles (wage per minute at skill 1, work clothes colour) and applicant names
  DA_TourCatalog  - van tours: direct sales and bulk pickups

Run after build_item_definitions.py:
  UnrealEditor-Cmd.exe <WorkshopEmpire.uproject> -ExecutePythonScript="<this file>" -unattended -RenderOffscreen -nosplash

Numbers are first-pass balancing; the tests Economy.BalanceStaff and Economy.BalanceTours measure them.
"""
import unreal

FOLDER = "/Game/WorkshopEmpire/Data/Staff"
ITEMS = "/Game/WorkshopEmpire/Data/Items"
R = unreal.WEStaffRole
Q = unreal.WEItemCondition
eal = unreal.EditorAssetLibrary
tools = unreal.AssetToolsHelpers.get_asset_tools()


def data_asset(name, cls):
    path = f"{FOLDER}/{name}"
    if eal.does_asset_exist(path):
        return eal.load_asset(path), path
    factory = unreal.DataAssetFactory()
    factory.set_editor_property("data_asset_class", cls)
    return tools.create_asset(name, FOLDER, cls, factory), path


def money(euros):
    m = unreal.WEMoney()
    m.set_editor_property("cents", int(round(euros * 100)))
    return m


def role(r, name, desc, cents_per_min, tint):
    d = unreal.WEStaffRoleDefinition()
    d.set_editor_property("role", r)
    d.set_editor_property("display_name", unreal.Text(name))
    d.set_editor_property("description", unreal.Text(desc))
    d.set_editor_property("wage_cents_per_minute", cents_per_min)
    d.set_editor_property("tint", unreal.LinearColor(*tint, 1.0))
    return d


staff, staff_path = data_asset("DA_StaffCatalog", unreal.WEStaffCatalog)
staff.set_editor_property("roles", [
    role(R.WAREHOUSE_WORKER, "Lagerarbeiter", "Trägt Material von einem Lager oder einer Maschine zum nächsten Ziel.", 60, (0.20, 0.38, 0.70)),
    role(R.MACHINE_OPERATOR, "Maschinenbediener", "Hält eine Maschine am Laufen: einschalten, warten, Produkte ins Lager. Die Maschine läuft 15 % schneller.", 80, (0.70, 0.45, 0.12)),
    role(R.DRIVER, "Fahrer", "Fährt den Firmenwagen: Verkaufsfahrten und Großeinkäufe.", 70, (0.20, 0.55, 0.30)),
])
staff.set_editor_property("first_names", ["Anna", "Ben", "Carla", "Deniz", "Emil", "Fatma", "Georg", "Hanna", "Ivan", "Jana", "Kemal", "Lea",
                                          "Marek", "Nina", "Oskar", "Petra", "Rafael", "Sophie", "Tim", "Uwe", "Vera", "Yusuf"])
staff.set_editor_property("last_names", ["Bauer", "Schmidt", "Yilmaz", "Novak", "Krüger", "Wagner", "Hoffmann", "Becker", "Kowalczyk", "Richter",
                                         "Klein", "Wolf", "Neumann", "Schulz", "Popescu", "Lange", "Fischer", "Meyer"])
staff.set_editor_property("max_employees", 6)
eal.save_asset(staff_path)


def item(suffix):
    return unreal.load_asset(f"{ITEMS}/DA_Item_{suffix}")


def tour_item(suffix, quantity, condition, quality, chance=1.0):
    t = unreal.WETourItem()
    t.set_editor_property("chance", chance)
    t.set_editor_property("item", item(suffix))
    t.set_editor_property("quantity", quantity)
    t.set_editor_property("condition", condition)
    t.set_editor_property("quality", quality)
    return t


def tour(tid, name, desc, kind, minutes, fuel, sell_factor=1.15, price=0.0, items=(), min_tier=1):
    t = unreal.WETourDefinition()
    t.set_editor_property("min_tier", min_tier)
    t.set_editor_property("id", tid)
    t.set_editor_property("display_name", unreal.Text(name))
    t.set_editor_property("description", unreal.Text(desc))
    t.set_editor_property("kind", kind)
    t.set_editor_property("minutes", minutes)
    t.set_editor_property("fuel_cost", money(fuel))
    t.set_editor_property("sell_factor", sell_factor)
    t.set_editor_property("pickup_price", money(price))
    t.set_editor_property("pickup_items", list(items))
    return t


K = unreal.WETourKind
tours, tours_path = data_asset("DA_TourCatalog", unreal.WETourCatalog)
tours.set_editor_property("tours", [
    tour("SellRun", "Verkaufsfahrt", "Lädt alles aus dem Laderaum direkt beim Händler ab, der am meisten zahlt: 15 % mehr als über den Marktplatz.",
         K.SELL, 4.0, 6.0, sell_factor=1.15),
    tour("PickupScrapLot", "Großeinkauf Altgeräte", "Holt einen Posten Altgeräte vom Wertstoffhof: 4 Computer, 3 Röhrenmonitore, 4 Kabelkartons.",
         K.PICKUP, 5.0, 8.0, price=55.0,
         items=[tour_item("OldComputer", 4, Q.DEFECTIVE, 35), tour_item("CrtMonitor", 3, Q.DEFECTIVE, 35), tour_item("CableBox", 4, Q.USED, 40)]),
    tour("PickupMotors", "Abholung Industriemotoren", "Zwei ausgemusterte Industriemotoren (je 180 kg) aus einer Fabrikauflösung – ohne Gabelstapler nicht zu bewegen.",
         K.PICKUP, 5.0, 8.0, price=50.0,
         items=[tour_item("IndustrialMotorLarge", 2, Q.DEFECTIVE, 30)]),
    # Phase 12: bring goods for accepted orders and supply contracts.
    # Phase 16: bigger companies get bigger lots (supply grows with the tier; Saved/Balance/ProgressionCurve.md).
    tour("PickupEWasteContainer", "Elektroschrott-Container", "Ein Container vom Wertstoffhof: 12 Computer, 8 Röhrenmonitore, 10 Kabelkartons, 2 Elektromotoren – manchmal ist etwas Seltenes dabei.",
         K.PICKUP, 6.0, 10.0, price=120.0, min_tier=2,
         items=[tour_item("OldComputer", 12, Q.DEFECTIVE, 30), tour_item("CrtMonitor", 8, Q.DEFECTIVE, 30), tour_item("CableBox", 10, Q.USED, 35),
                tour_item("ElectricMotor", 2, Q.DEFECTIVE, 30), tour_item("RareGpu", 1, Q.DAMAGED, 40, chance=0.04), tour_item("VintageRadio", 1, Q.DAMAGED, 40, chance=0.03)]),
    tour("PickupCableLot", "Kabel- und Motorenschrott vom Netzbetreiber", "Ein Lkw voll Altkabel und ausgebauter Motoren: 30 Kabelkartons, 4 Elektromotoren, ein großer Industriemotor.",
         K.PICKUP, 6.0, 12.0, price=160.0, min_tier=3,
         items=[tour_item("CableBox", 30, Q.USED, 35), tour_item("ElectricMotor", 4, Q.DEFECTIVE, 35), tour_item("IndustrialMotorLarge", 1, Q.DEFECTIVE, 30),
                tour_item("ServoMotor", 1, Q.USED, 50, chance=0.05)]),
    tour("PickupOfficeClearance", "Großauflösung Bürogebäude", "Ein ganzes Bürogebäude wird geräumt: 25 Computer, 12 Röhrenmonitore, 10 Kabelkartons, 2 Elektromotoren aus der Haustechnik – Sammlerstücke nicht ausgeschlossen.",
         K.PICKUP, 7.0, 12.0, price=200.0, min_tier=3,
         items=[tour_item("OldComputer", 25, Q.DEFECTIVE, 35), tour_item("CrtMonitor", 12, Q.DEFECTIVE, 35), tour_item("CableBox", 10, Q.USED, 40),
                tour_item("ElectricMotor", 2, Q.DEFECTIVE, 35),
                tour_item("GoldCpu", 1, Q.USED, 50, chance=0.06), tour_item("RareGpu", 1, Q.DAMAGED, 45, chance=0.05)]),
    tour("PickupFactoryClearance", "Fabrikauflösung", "Eine stillgelegte Fabrik: 4 große Industriemotoren, 8 Elektromotoren, 30 Kabelkartons, 20 Computer aus der Verwaltung, oft Servomotoren.",
         K.PICKUP, 8.0, 20.0, price=350.0, min_tier=4,
         items=[tour_item("IndustrialMotorLarge", 4, Q.DEFECTIVE, 35), tour_item("ElectricMotor", 8, Q.DEFECTIVE, 35), tour_item("CableBox", 30, Q.USED, 40),
                tour_item("OldComputer", 20, Q.DEFECTIVE, 35), tour_item("ServoMotor", 1, Q.USED, 55, chance=0.25), tour_item("GoldCpu", 1, Q.USED, 50, chance=0.08)]),
    tour("DeliverOrders", "Auslieferung", "Fährt die Ware aus dem Laderaum zu den Kunden: jeder angenommene Auftrag, dessen Ware vollständig geladen ist, wird geliefert (Lieferverträge: eine Lieferung).",
         K.DELIVER, 4.0, 6.0),
])
eal.save_asset(tours_path)

unreal.log(f"WE_STAFF_BUILT 3 roles, {len(tours.get_editor_property('tours'))} tours")
unreal.SystemLibrary.quit_editor()
