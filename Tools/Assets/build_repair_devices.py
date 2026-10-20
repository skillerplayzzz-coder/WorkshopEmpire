"""
Phase 20: models of the new repairable devices and their spare parts (real size in cm, origin at the bottom centre):
  SM_Gen_PortableRadio   cassette radio with handle, speaker grilles, dial and antenna
  SM_Gen_GameConsole     flat game console with disc slot, vents and a controller
  SM_Gen_Speaker         round speaker with magnet
  SM_Gen_DiscDrive       console disc drive (sheet metal case, tray, ribbon cable)
  SM_Gen_DrillBattery    slide-on 18 V battery pack
  SM_Gen_CarbonBrushes   bag with two carbon brushes and springs
(The television uses the CC0 model Television_01, see Tools/Assets/import_assets.py.)

  UnrealEditor-Cmd.exe <uproject> -ExecutePythonScript="<this file>" -unattended -RenderOffscreen -nosplash -nosteam
"""
import builtins
import os
import sys

import unreal

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
builtins.WE_MODELS_LIBRARY = True
import build_models as bm  # noqa: E402

Model = bm.Model


def portable_radio():
    m = Model("PortableRadio")
    m.box("PlasticGrey", (0, 0, 9.0), (36.0, 10.0, 18.0), bevel=1.2, segments=2)
    for x in (-11.0, 11.0):
        m.cyl("TrimBlack", (x, -5.05, 8.0), 5.2, 0.3, rot=(90, 0, 0), segs=28)          # speaker grille
        for r in (1.5, 3.0, 4.5):
            m.torus("SteelBrushed", (x, -5.25, 8.0), r, 0.12, rot=(90, 0, 0), segs=24, minor=6)
    m.box("ScreenDark", (0, -5.1, 13.5), (12.0, 0.3, 3.0), bevel=0.2)                   # dial window
    m.box("PlasticRed", (-2.0, -5.3, 13.5), (0.3, 0.2, 2.4))
    m.box("TrimBlack", (0, -5.1, 6.5), (7.0, 0.3, 4.2))                                 # cassette door
    for i in range(5):
        m.box("PlasticBlack", (-6.0 + i * 3.0, 0, 18.4), (2.2, 3.0, 0.8), bevel=0.2)   # keys
    m.cyl("SteelBrushed", (16.0, 3.0, 26.0), 0.25, 16.0, segs=8)                         # antenna
    m.box("PlasticBlack", (0, 2.0, 21.5), (26.0, 2.0, 1.6), bevel=0.4)                  # handle
    for x in (-12.0, 12.0):
        m.box("PlasticBlack", (x, 2.0, 19.5), (2.0, 2.0, 3.0))
    return m


def game_console():
    m = Model("GameConsole")
    m.box("PlasticBlack", (0, 0, 3.5), (30.0, 26.0, 7.0), bevel=1.0, segments=2)
    m.box("PlasticGrey", (0, 0, 7.05), (28.0, 24.0, 0.2))
    m.box("TrimBlack", (-4.0, -13.05, 4.2), (14.0, 0.3, 0.8))                           # disc slot
    m.box("LedGreen", (11.0, -13.1, 4.0), (0.8, 0.2, 0.8))
    for i in range(10):
        m.box("TrimBlack", (15.05, -9.0 + i * 2.0, 3.5), (0.3, 1.0, 4.5))               # vents
    # Controller beside it.
    m.box("PlasticGrey", (0, -22.0, 1.6), (12.0, 6.0, 3.2), bevel=1.2, segments=2)
    for x in (-4.5, 4.5):
        m.cyl("PlasticGrey", (x, -24.0, 1.6), 2.6, 3.2, segs=16)
        m.cyl("TrimBlack", (x * 0.4, -21.0, 3.4), 0.9, 0.6, segs=12)
    m.box("PlasticBlack", (0, -16.0, 1.0), (1.0, 6.0, 0.8))                              # cable
    return m


def speaker():
    m = Model("Speaker")
    m.cone("TrimBlack", (0, 0, 2.2), 6.5, 2.5, 3.0, segs=28)
    m.cyl("SteelBrushed", (0, 0, 0.4), 6.8, 0.8, segs=28)
    m.cyl("PlasticBlack", (0, 0, 4.2), 2.2, 1.6, segs=20)                                # magnet
    m.cyl("SteelBrushed", (0, 0, 3.6), 1.0, 0.4, segs=12)
    return m


def disc_drive():
    m = Model("DiscDrive")
    m.box("SteelBrushed", (0, 0, 2.0), (18.0, 15.0, 4.0), bevel=0.3)
    m.box("PlasticBlack", (0, -7.6, 2.0), (17.0, 0.4, 3.0))                              # front with tray
    m.box("PlasticGrey", (0, -6.0, 3.0), (12.0, 3.0, 0.4))
    m.box("Copper", (4.0, 7.9, 1.0), (5.0, 0.3, 0.8))                                    # ribbon cable
    m.box("PlasticWhite", (4.0, 10.0, 0.3), (4.5, 4.0, 0.15))
    return m


def drill_battery():
    m = Model("DrillBattery")
    m.box("PlasticBlack", (0, 0, 3.0), (11.0, 8.0, 6.0), bevel=0.8, segments=2)
    m.box("PlasticRed", (0, 0, 6.3), (9.0, 6.0, 0.8), bevel=0.3)                         # slide rails
    for x in (-1.0, 1.0):
        m.box("Gold", (x, 0, 6.8), (0.4, 3.0, 0.2))
    for i in range(3):
        m.box("LedGreen", (4.0, -4.05, 2.0 + i * 1.0), (0.6, 0.2, 0.5))
    return m


def carbon_brushes():
    m = Model("CarbonBrushes")
    m.box("Cardboard", (0, 0, 0.2), (7.0, 5.0, 0.4))
    for x in (-1.5, 1.5):
        m.box("TrimBlack", (x, 0, 0.9), (1.2, 0.8, 1.0))                                 # carbon block
        m.cyl("Copper", (x, 1.5, 0.7), 0.15, 2.0, rot=(90, 0, 0), segs=6)                # lead
        for i in range(4):
            m.torus("SteelBrushed", (x, -1.2 - i * 0.3, 0.9), 0.35, 0.06, rot=(90, 0, 0), segs=10, minor=4)
    return m


BUILDERS = {"PortableRadio": portable_radio, "GameConsole": game_console, "Speaker": speaker, "DiscDrive": disc_drive,
            "DrillBattery": drill_battery, "CarbonBrushes": carbon_brushes}


def main():
    built = 0
    for name, make in BUILDERS.items():
        try:
            if make().save():
                built += 1
        except Exception as e:  # keep going, report at the end
            unreal.log_error(f"WE_MODEL {name} failed: {e}")
    unreal.log(f"WE_REPAIR_MODELS {built}/{len(BUILDERS)}")
    unreal.SystemLibrary.quit_editor()


main()
