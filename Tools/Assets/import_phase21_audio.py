"""
Phase 21: imports CC0 foley sounds from Kenney "RPG Audio" (kenney.nl, CC0; converted to mono WAV with ffmpeg into
SourceArt/Kenney/RPGAudio/Wav, see Docs/ASSET_SOURCES.md) into /Game/WorkshopEmpire/Audio/P21:
  SFX_K_handleCoins*    screws dropping into the screw bowl
  SFX_K_metalClick/Latch/Pot*  snapping in, tools, the bowl
  SFX_K_bookOpen/Flip*  cardboard box flaps
  SFX_K_cloth*          protective film, wiping
  SFX_K_drawKnife*, beltHandle*, handleSmallLeather, dropLeather  picking up / putting down tools

  UnrealEditor-Cmd.exe <uproject> -ExecutePythonScript="<this file>" -unattended -RenderOffscreen -nosplash -nosteam
"""
import os

import unreal

ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), "..", ".."))
SRC = os.path.join(ROOT, "SourceArt", "Kenney", "RPGAudio", "Wav")
DEST = "/Game/WorkshopEmpire/Audio/P21"
LOOPING = set()
tools = unreal.AssetToolsHelpers.get_asset_tools()

count = 0
for f in sorted(os.listdir(SRC)):
    if not f.endswith(".wav"):
        continue
    name = os.path.splitext(f)[0]
    task = unreal.AssetImportTask()
    task.set_editor_property("filename", os.path.join(SRC, f))
    task.set_editor_property("destination_path", DEST)
    task.set_editor_property("destination_name", name)
    task.set_editor_property("automated", True)
    task.set_editor_property("replace_existing", True)
    task.set_editor_property("save", True)
    tools.import_asset_tasks([task])
    wave = unreal.load_asset(f"{DEST}/{name}")
    if not wave:
        unreal.log_error(f"WE_AUDIO21 import failed: {f}")
        continue
    wave.set_editor_property("looping", name in LOOPING)
    unreal.EditorAssetLibrary.save_loaded_asset(wave)
    count += 1
unreal.log(f"WE_AUDIO21 imported {count}")
unreal.SystemLibrary.quit_editor()
