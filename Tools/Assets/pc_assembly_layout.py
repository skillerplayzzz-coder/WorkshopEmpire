"""
Phase 18a: layout of the old office PC (pure data, no unreal import), shared by
  Tools/Assets/build_pc_assembly.py   (models of the case and of every part, in device space)
  Tools/Maps/build_item_definitions.py (UWEAssemblyDefinition: fasteners, prerequisites, pull directions)

Device space (cm): x = width (left side panel at -X, mainboard tray at +X), y = depth (front at -Y),
z = height, origin at the bottom centre (the feet stand on z = 0, the case body starts at z = FEET).
Part i is disassembly step i of DIS_OldComputer (same order as PARTS below).
"""

FEET = 0.8  # the body stands on rubber feet; every z below is measured from the body's bottom

# Case body: 19 x 44 x 42 cm (classic beige mid tower).
W, D, H = 19.0, 44.0, 42.0
BOARD_X = 8.0  # mainboard plane (component side faces -X)


def p(x, y, z):
    """Case-body coordinates -> device space."""
    return (x, y, z + FEET)


# Fastener: (kind, location, direction, order, side)
#   kind: Screw | Thumbscrew | Clip | Plug | Lever
#   direction: screws come out along it, plugs are pulled along it, clips/levers open towards it
#   side: optional X axis of the fastener model (a lever's arm, the side a clip tips away from)
def screw(loc, d, order=1):
    return ("Screw", p(*loc), d, order, (0, 0, 0))


def plug(loc, d, order=0):
    return ("Plug", p(*loc), d, order, (0, 0, 0))


def clip(loc, d, side, order=0):
    return ("Clip", p(*loc), d, order, side)


def lever(loc, d, side, order=0):
    return ("Lever", p(*loc), d, order, side)


MX = (-1, 0, 0)   # out of the mainboard / out of the open side
PY = (0, 1, 0)    # towards the back
NY = (0, -1, 0)   # towards the front

RAM_Y = (3.6, 4.8)

# id, name, mesh suffix (SM_Gen_PCA_<suffix>), fasteners, requires, pull direction, pull distance, flags
PARTS = [
    dict(id="SidePanel", name="Seitenteil", fasteners=[
        screw((-8.4, 22.35, 7.0), PY, 0), screw((-8.4, 22.35, 35.0), PY, 0)],
        requires=[], pull=MX, dist=9.0, service=True, beside=True),
    dict(id="GraphicsCard", name="Grafikkarte", fasteners=[
        plug((-3.6, 8.0, 14.55), MX, 0),
        screw((-4.4, 20.8, 14.0), MX, 1),
        clip((7.0, 7.4, 14.6), MX, (0, 1, 0), 2)],
        requires=["SidePanel"], pull=MX, dist=7.0, alts=["GraphicsCardPro", "RareGpu"]),
    # Quality update: every stick on its own (two latches each); the second stick is part 12 (appended, saves keep their bits).
    dict(id="Ram", name="RAM-Riegel 1", fasteners=[
        clip((7.3, RAM_Y[0], 31.9), MX, (0, 0, -1)), clip((7.3, RAM_Y[0], 17.4), MX, (0, 0, 1))],
        requires=["SidePanel"], pull=MX, dist=4.5, alts=["RamModule8"], ram=0),
    # The cooler goes back on with fresh thermal paste (consumed on installation).
    dict(id="CpuCooler", name="CPU-Kühler", fasteners=[
        plug((7.3, 7.2, 31.6), MX, 0),
        clip((6.4, 12.0, 29.6), MX, (0, 0, -1), 1), clip((6.4, 12.0, 22.4), MX, (0, 0, 1), 1)],
        requires=["SidePanel"], pull=MX, dist=8.0, install="ThermalPaste"),
    dict(id="Cpu", name="Prozessor", fasteners=[
        lever((7.2, 15.4, 23.2), MX, (0, 0, 1), 0)],
        requires=["SidePanel", "CpuCooler"], pull=MX, dist=4.0, alts=["ProcessorFast"]),
    dict(id="HardDrive", name="Festplatte", fasteners=[
        plug((-1.5, -6.0, 9.3), PY, 0), plug((2.8, -6.0, 9.3), PY, 0),
        screw((-6.65, -18.0, 9.3), MX, 1), screw((-6.65, -10.0, 9.3), MX, 1)],
        requires=["SidePanel"], pull=PY, dist=6.0),
    dict(id="Ssd", name="SSD", fasteners=[
        plug((-1.0, -10.3, 13.35), PY, 0),
        screw((-6.65, -18.5, 13.3), MX, 1), screw((-6.65, -10.8, 13.3), MX, 1)],
        requires=["SidePanel"], pull=PY, dist=6.0, alts=["SsdFast"]),
    dict(id="CaseFan", name="Gehäuselüfter", fasteners=[
        plug((7.3, 17.0, 18.6), MX, 0),
        screw((-6.6, 22.0, 23.4), PY, 1), screw((0.6, 22.0, 23.4), PY, 1),
        screw((-6.6, 22.0, 30.6), PY, 1), screw((0.6, 22.0, 30.6), PY, 1)],
        requires=["SidePanel"], pull=NY, dist=4.0, optional=True),
    dict(id="PowerSupply", name="Netzteil", fasteners=[
        plug((6.5, -2.0, 27.5), MX, 0), plug((6.9, 16.4, 32.0), MX, 0),
        screw((-7.25, 22.0, 33.15), PY, 1), screw((7.25, 22.0, 33.15), PY, 1),
        screw((-7.25, 22.0, 41.25), PY, 1), screw((7.25, 22.0, 41.25), PY, 1)],
        requires=["SidePanel"], pull=NY, dist=5.0),
    dict(id="Mainboard", name="Mainboard", fasteners=[
        screw((7.75, y, z), MX, 0) for y in (-2.6, 9.0, 20.2) for z in (3.3, 31.8)],
        requires=["SidePanel", "GraphicsCard", "Ram", "Ram2", "CpuCooler", "Cpu", "PowerSupply", "CaseFan"], pull=MX, dist=3.0),
    dict(id="CdDrive", name="CD-Laufwerk", fasteners=[
        plug((-2.5, -1.3, 37.5), PY, 0), plug((4.5, -1.3, 37.5), PY, 0),
        screw((-7.85, -16.0, 37.5), MX, 1), screw((-7.85, -8.0, 37.5), MX, 1)],
        requires=["SidePanel"], pull=NY, dist=12.0, optional=True),
    dict(id="Case", name="Gehäuse", fasteners=[],
        requires=["SidePanel", "GraphicsCard", "Ram", "Ram2", "CpuCooler", "Cpu", "HardDrive", "Ssd", "CaseFan", "PowerSupply",
                  "Mainboard", "CdDrive"], pull=(0, 0, 1), dist=2.0, shell=True),
    dict(id="Ram2", name="RAM-Riegel 2", fasteners=[
        clip((7.3, RAM_Y[1], 31.9), MX, (0, 0, -1)), clip((7.3, RAM_Y[1], 17.4), MX, (0, 0, 1))],
        requires=["SidePanel"], pull=MX, dist=4.5, alts=["RamModule8"], ram=1),
]

# Lying on its right side (+X down), open side up: pitch -90 turns -X into +Z; yaw 180 turns the back to the player.
BENCH_ROTATION = dict(roll=0.0, pitch=-90.0, yaw=180.0)  # back (screws, ports) towards the player
