"""
Phase 19: models of the better PC parts (loose items, real size in cm, lying flat):
  SM_Gen_GraphicsCardPro   mid-range card, black shroud, two fans, green accent
  SM_Gen_RamModule8        memory module with a red aluminium heat spreader
  SM_Gen_ProcessorFast     processor with a metal heat spreader, blue substrate
  SM_Gen_SsdFast           2.5" SSD, dark blue case with a label

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


def gpu_pro():
    m = Model("GraphicsCardPro")
    m.box("PCB", (0, 0, 0.1), (24.0, 11.0, 0.2))
    m.box("PlasticBlack", (0.5, 0, 1.6), (23.0, 10.6, 2.8), bevel=0.4)          # shroud
    for x in (-5.5, 5.5):
        m.cyl("TrimBlack", (x, 0, 3.05), 4.3, 0.2, segs=28)                     # fan opening
        m.cyl("PlasticBlack", (x, 0, 3.0), 1.2, 0.6, segs=16)                   # hub
        for i in range(9):
            m.box("PlasticGrey", (x, 0, 3.0), (3.6, 0.7, 0.15), rot=(0, 0, i * 20.0))
    m.box("LedGreen", (0.5, -5.35, 1.6), (21.0, 0.15, 0.5))                      # light strip
    m.box("SteelBrushed", (-12.3, 0, 1.4), (0.3, 12.0, 2.8))                     # slot bracket
    m.box("Gold", (2.0, 5.6, 0.1), (8.0, 0.4, 0.15))                            # contacts
    m.box("PlasticBlack", (9.5, 4.8, 2.9), (2.2, 1.0, 1.0))                      # power plug
    return m


def ram8():
    m = Model("RamModule8")
    m.box("PCB", (0, 0, 0.06), (13.3, 3.1, 0.12))
    m.box("PlasticRed", (0, -0.2, 0.32), (13.0, 2.6, 0.3), bevel=0.05)            # heat spreader
    for i in range(6):
        m.box("Aluminium", (-5.0 + i * 2.0, -1.2, 0.5), (1.0, 0.6, 0.12))
    for i in range(30):
        m.box("Gold", (-6.2 + i * 0.42, 1.4, 0.13), (0.26, 0.3, 0.02))
    return m


def cpu_fast():
    m = Model("ProcessorFast")
    m.box("MetalPaintedBlue", (0, 0, 0.1), (4.0, 4.0, 0.2), bevel=0.05)          # substrate
    m.box("SteelBrushed", (0, 0, 0.35), (3.0, 3.0, 0.3), bevel=0.1)              # heat spreader
    m.box("Gold", (-1.6, -1.6, 0.21), (0.4, 0.4, 0.02))                           # corner mark
    return m


def ssd_fast():
    m = Model("SsdFast")
    m.box("ContainerBlue", (0, 0, 0.35), (10.0, 7.0, 0.7), bevel=0.1)
    m.box("PlasticWhite", (0.5, 0, 0.71), (6.0, 4.5, 0.02))                       # label
    m.box("PlasticBlack", (-4.9, 1.5, 0.35), (0.3, 3.0, 0.4))                     # SATA connector
    return m


built = 0
for make in (gpu_pro, ram8, cpu_fast, ssd_fast):
    try:
        built += 1 if make().save(normalize=False) else 0
    except Exception as e:  # keep going with the others
        unreal.log_error(f"WE_UPGRADES {make.__name__} failed: {e}")
unreal.log(f"WE_UPGRADES_BUILT {built}/4")
unreal.SystemLibrary.quit_editor()
