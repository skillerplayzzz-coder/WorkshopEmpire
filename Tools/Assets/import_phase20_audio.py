"""
Phase 20: imports the CC0 sounds and music from OpenGameArt (converted to mono WAV with ffmpeg into
SourceArt/OpenGameArt/Wav, see Docs/ASSET_SOURCES.md) into /Game/WorkshopEmpire/Audio/P20:
  SFX_Radio1..6         workshop radio music (Holizna, "Funk (Collection)", CC0)
  SFX_RainLoop          rain loop (Ylmir, "Rain (loopable)", CC0)
  SFX_Thunder           thunder (WuxiaScrub, "Rain + Long Thunder", CC0)
  SFX_cat_*             purring and meowing (Kerzoven, "Cat Purr & Meow", CC0)

  UnrealEditor-Cmd.exe <uproject> -ExecutePythonScript="<this file>" -unattended -RenderOffscreen -nosplash -nosteam
"""
import os

import unreal

ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), "..", ".."))
SRC = os.path.join(ROOT, "SourceArt", "OpenGameArt", "Wav")
DEST = "/Game/WorkshopEmpire/Audio/P20"
LOOPING = {"SFX_RainLoop", "SFX_cat_purrsleepy_loop"}
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
        unreal.log_error(f"WE_AUDIO20 import failed: {f}")
        continue
    wave.set_editor_property("looping", name in LOOPING)
    unreal.EditorAssetLibrary.save_loaded_asset(wave)
    count += 1
unreal.log(f"WE_AUDIO20 imported {count}")
unreal.SystemLibrary.quit_editor()
