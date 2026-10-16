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


def tour_item(suffix, quantity, condition, quality):
    t = unreal.WETourItem()
    t.set_editor_property("item", item(suffix))
    t.set_editor_property("quantity", quantity)
    t.set_editor_property("condition", condition)
    t.set_editor_property("quality", quality)
    return t


def tour(tid, name, desc, kind, minutes, fuel, sell_factor=1.15, price=0.0, items=()):
    t = unreal.WETourDefinition()
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
    tour("DeliverOrders", "Auslieferung", "Fährt die Ware aus dem Laderaum zu den Kunden: jeder angenommene Auftrag, dessen Ware vollständig geladen ist, wird geliefert (Lieferverträge: eine Lieferung).",
         K.DELIVER, 4.0, 6.0),
])
eal.save_asset(tours_path)

unreal.log("WE_STAFF_BUILT 3 roles, 4 tours")
unreal.SystemLibrary.quit_editor()
