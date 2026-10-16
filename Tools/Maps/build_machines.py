"""
Creates/updates the machine definitions (/Game/WorkshopEmpire/Data/Machines, Phase 9):
  MD_Shredder  - devices and scrap -> shredded material (amount by weight)
  MD_Sorter    - shredded material -> copper, aluminium, steel, plastic, electronic scrap
  MD_Press     - loose metal -> bales (worth more at the scrap dealer)
  MD_Assembler - sorted material -> products (Phase 10)

Run after build_item_definitions.py and before build_workshop.py (the furniture catalog links them):
  UnrealEditor-Cmd.exe <WorkshopEmpire.uproject> -ExecutePythonScript="<this file>" -unattended -RenderOffscreen -nosplash

Numbers are first-pass balancing; the test Economy.BalanceRecyclingChain measures them.
"""
import unreal

FOLDER = "/Game/WorkshopEmpire/Data/Machines"
ITEMS = "/Game/WorkshopEmpire/Data/Items"
C = unreal.WEItemCategory
eal = unreal.EditorAssetLibrary
tools = unreal.AssetToolsHelpers.get_asset_tools()


def item(suffix):
    return unreal.load_asset(f"{ITEMS}/DA_Item_{suffix}")


def data_asset(name):
    path = f"{FOLDER}/{name}"
    if eal.does_asset_exist(path):
        return eal.load_asset(path), path
    factory = unreal.DataAssetFactory()
    factory.set_editor_property("data_asset_class", unreal.WEMachineDefinition)
    return tools.create_asset(name, FOLDER, unreal.WEMachineDefinition, factory), path


def out(suffix, lo=1, hi=None, chance=1.0, per_kg=0.0, new=False):
    """new=True: a product made new (condition Neu, quality 70); otherwise material (Gebraucht, 50)."""
    o = unreal.WEMachineOutput()
    o.set_editor_property("condition", unreal.WEItemCondition.NEW if new else unreal.WEItemCondition.USED)
    o.set_editor_property("quality", 70 if new else 50)
    o.set_editor_property("item", item(suffix))
    o.set_editor_property("min_quantity", lo)
    o.set_editor_property("max_quantity", hi if hi is not None else lo)
    o.set_editor_property("chance", chance)
    o.set_editor_property("per_input_kg", per_kg)
    return o


def recipe(name, outputs, input_item=None, quantity=1, categories=(), base_s=4.0, per_kg_s=0.0, second=None, second_qty=1):
    r = unreal.WEMachineRecipe()
    r.set_editor_property("display_name", unreal.Text(name))
    if input_item:
        r.set_editor_property("input_item", item(input_item))
    if second:
        r.set_editor_property("second_input_item", item(second))
        r.set_editor_property("second_input_quantity", second_qty)
    r.set_editor_property("input_quantity", quantity)
    r.set_editor_property("input_categories", list(categories))
    r.set_editor_property("outputs", outputs)
    r.set_editor_property("base_seconds", base_s)
    r.set_editor_property("seconds_per_kg", per_kg_s)
    return r


MACHINES = {
    "MD_Shredder": dict(
        name="Schredder", power=7.5, idle=0.4, bulky=True, in_slots=6, in_kg=120.0, out_slots=10, out_kg=250.0, wear=2.0,
        recipes=[
            # Specific inputs first: cables are worth it for their copper.
            recipe("Kabel schreddern", [out("CopperGranulate", 3, 4), out("PlasticParts", 2, 3)],
                   input_item="CableBox", base_s=3.0, per_kg_s=1.0),
            recipe("Elektronik schreddern", [out("ShredElectronics", per_kg=0.9)],
                   categories=(C.ELECTRONICS, C.HOUSEHOLD_APPLIANCE, C.POWER_TOOL), base_s=3.0, per_kg_s=1.2),
            recipe("Metallschrott schreddern", [out("ShredMetal", per_kg=0.95)],
                   categories=(C.SCRAP_METAL, C.MOTOR, C.TOOL, C.SPARE_PART), base_s=3.0, per_kg_s=1.0),
        ]),
    "MD_Sorter": dict(
        name="Sortiermaschine", power=4.0, idle=0.3, bulky=False, in_slots=8, in_kg=150.0, out_slots=12, out_kg=200.0, wear=2.0,
        recipes=[
            recipe("Schreddergut Elektronik sortieren", [
                out("ElectronicScrap", 3, 5), out("SteelScrap", 2, 3), out("PlasticParts", 1, 2),
                out("CopperGranulate", 1, 1, chance=0.5), out("AluminiumScrap", 1, 1, chance=0.25)],
                input_item="ShredElectronics", quantity=5, base_s=6.0),
            recipe("Schreddergut Metall sortieren", [
                out("SteelScrap", 3, 4), out("AluminiumScrap", 1, 1, chance=0.6), out("CopperGranulate", 1, 1, chance=0.4),
                out("PlasticParts", 1, 1, chance=0.5)],
                input_item="ShredMetal", quantity=5, base_s=5.0),
        ]),
    "MD_Press": dict(
        name="Metallpresse", power=5.5, idle=0.3, bulky=False, in_slots=8, in_kg=200.0, out_slots=4, out_kg=100.0, wear=3.0,
        recipes=[
            recipe("Stahlballen pressen", [out("SteelBale")], input_item="SteelScrap", quantity=20, base_s=8.0),
            recipe("Aluminiumballen pressen", [out("AluBale")], input_item="AluminiumScrap", quantity=10, base_s=8.0),
            recipe("Kupferpaket pressen", [out("CopperBale")], input_item="CopperGranulate", quantity=20, base_s=8.0),
        ]),
    # Phase 10: makes products from sorted material. Several recipes share an input: choose with [T].
    "MD_Assembler": dict(
        name="Fertigungsautomat", power=6.0, idle=0.4, bulky=False, in_slots=8, in_kg=150.0, out_slots=8, out_kg=120.0, wear=2.0,
        recipes=[
            recipe("Kupferdrahtspule wickeln", [out("CopperWireSpool", new=True)], input_item="CopperGranulate", quantity=4, base_s=10.0),
            recipe("Verlängerungskabel fertigen", [out("ExtensionCable", new=True)], input_item="CopperGranulate", quantity=2,
                   second="PlasticParts", second_qty=3, base_s=12.0),
            recipe("Stahlwinkel-Set stanzen", [out("SteelBracketSet", new=True)], input_item="SteelScrap", quantity=6, base_s=8.0),
        ]),
}

kit = item("MaintenanceKit")
for asset_name, spec in MACHINES.items():
    md, path = data_asset(asset_name)
    md.set_editor_property("display_name", unreal.Text(spec["name"]))
    md.set_editor_property("power_kw", spec["power"])
    md.set_editor_property("idle_power_kw", spec["idle"])
    md.set_editor_property("accepts_bulky", spec["bulky"])
    md.set_editor_property("input_slots", spec["in_slots"])
    md.set_editor_property("input_max_weight_kg", spec["in_kg"])
    md.set_editor_property("output_slots", spec["out_slots"])
    md.set_editor_property("output_max_weight_kg", spec["out_kg"])
    md.set_editor_property("wear_per_cycle", spec["wear"])
    md.set_editor_property("worn_threshold", 30.0)
    md.set_editor_property("maintenance_item", kit)
    md.set_editor_property("recipes", spec["recipes"])
    eal.save_asset(path)

unreal.log(f"WE_MACHINES_BUILT {len(MACHINES)} machines")
unreal.SystemLibrary.quit_editor()
