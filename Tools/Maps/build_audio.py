"""
Builds the game's sound effects /Game/WorkshopEmpire/Audio/SFX_* from free recordings (no self-made sounds):
  SourceArt/Kenney/...     Kenney sound packs (CC0)
  SourceArt/Pixabay/...    Pixabay sound effects (Pixabay Content License: free, commercial use, no attribution needed)
Sources and licences are listed in Docs/ASSET_SOURCES.md.

Each source is imported, exported to WAV by the engine (so MP3/OGG need no extra tools), then cut to the part that is
used, turned to mono (3D sounds), faded in and out, loops cross-faded so they repeat without a click, normalised, and
imported under its game name. The C++ code only knows the SFX_* names.

Run before build_garage_map.py (the workbench references these sounds):
  UnrealEditor-Cmd.exe <WorkshopEmpire.uproject> -ExecutePythonScript="<this file>" -unattended -RenderOffscreen -nosplash
"""
import array
import math
import os
import wave

import unreal

PROJECT = unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir())
SOURCE = os.path.join(PROJECT, "SourceArt")
WORK = os.path.join(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_saved_dir()), "AudioWork")
DEST = "/Game/WorkshopEmpire/Audio"
TEMP = "/Game/WorkshopEmpire/Audio/_Source"
KENNEY_IMPACT = os.path.join("Kenney", "impact-sounds", "Audio")
KENNEY_UI = os.path.join("Kenney", "interface-sounds", "Audio")
PIXABAY = "Pixabay"

# name: (source file below SourceArt, start s ("auto" = steadiest part), length s (None = all), loop, peak level)
SOUNDS = {
    # Kenney (CC0)
    "SFX_PlaceItem":       (os.path.join(KENNEY_IMPACT, "impactWood_medium_002.ogg"), 0.0, None, False, 0.8),
    "SFX_PartRemoved":     (os.path.join(KENNEY_IMPACT, "impactMetal_light_002.ogg"), 0.0, None, False, 0.8),
    "SFX_DisassemblyDone": (os.path.join(KENNEY_UI, "confirmation_002.ogg"), 0.0, None, False, 0.7),
    "SFX_Denied":          (os.path.join(KENNEY_UI, "error_004.ogg"), 0.0, None, False, 0.6),
    "SFX_Step":            (os.path.join(KENNEY_IMPACT, "footstep_concrete_000.ogg"), 0.0, None, False, 0.6),
    # Pixabay
    "SFX_ScrewLoosen":     (os.path.join(PIXABAY, "ratchet-socket-wrench-tool.mp3"), 0.0, 1.2, False, 0.8),
    "SFX_ShredderLoop":    (os.path.join(PIXABAY, "wood-chipper-grinding-shredding.mp3"), "auto", 6.0, True, 0.9),
    "SFX_ShredderCrunch":  (os.path.join(PIXABAY, "heavy-metal-crush.mp3"), 0.0, 2.5, False, 0.9),
    "SFX_PressLoop":       (os.path.join(PIXABAY, "homemade-hydraulic-hoist.mp3"), "auto", 4.0, True, 0.8),
    "SFX_PressSlam":       (os.path.join(PIXABAY, "hydraulic-press.mp3"), 0.0, None, False, 0.9),
    "SFX_MotorLoop":       (os.path.join(PIXABAY, "electric-motor-whir.mp3"), "auto", 4.0, True, 0.7),
    "SFX_FurnaceLoop":     (os.path.join(PIXABAY, "fire-in-furnace.mp3"), "auto", 6.0, True, 0.8),
    "SFX_MachineStart":    (os.path.join(PIXABAY, "electric-motor-engine-start-stop.mp3"), "first-half", None, False, 0.7),
    "SFX_MachineStop":     (os.path.join(PIXABAY, "electric-motor-engine-start-stop.mp3"), "second-half", None, False, 0.7),
    "SFX_PourSizzle":      (os.path.join(PIXABAY, "boiling-metal.mp3"), "auto", 3.0, False, 0.8),
    "SFX_Strap":           (os.path.join(PIXABAY, "cable-tie.mp3"), 0.0, None, False, 0.8),
    "SFX_RoomTone":        (os.path.join(PIXABAY, "room-tone-office.mp3"), "auto", 10.0, True, 0.6),
    "SFX_Outdoor":         (os.path.join(PIXABAY, "suburban-spring-day-ambience.mp3"), "auto", 20.0, True, 0.7),
    "SFX_VanDrive":        (os.path.join(PIXABAY, "van-interior-driving.mp3"), "auto", 4.0, False, 0.8),
}
LOOPS = {n for n, s in SOUNDS.items() if s[3]}
STEREO = {"SFX_Outdoor"}  # not placed in 3D: keeps its width
# Playback volume per sound (the files are normalised).
VOLUME = {"SFX_ShredderLoop": 0.55, "SFX_PressLoop": 0.4, "SFX_MotorLoop": 0.3, "SFX_FurnaceLoop": 0.5, "SFX_MachineStart": 0.5,
          "SFX_MachineStop": 0.45, "SFX_RoomTone": 0.18, "SFX_Outdoor": 0.3, "SFX_ShredderCrunch": 0.7, "SFX_PressSlam": 0.8}


def import_file(path, folder, name):
    task = unreal.AssetImportTask()
    task.set_editor_property("filename", path)
    task.set_editor_property("destination_path", folder)
    task.set_editor_property("destination_name", name)
    task.set_editor_property("automated", True)
    task.set_editor_property("replace_existing", True)
    task.set_editor_property("save", True)
    unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])
    return unreal.EditorAssetLibrary.load_asset(f"{folder}/{name}")


def export_wav(asset, path):
    task = unreal.AssetExportTask()
    task.set_editor_property("object", asset)
    task.set_editor_property("filename", path)
    task.set_editor_property("automated", True)
    task.set_editor_property("replace_identical", True)
    task.set_editor_property("prompt", False)
    return unreal.Exporter.run_asset_export_task(task)


def read_wav(path):
    with wave.open(path, "rb") as w:
        channels, width, rate, frames = w.getnchannels(), w.getsampwidth(), w.getframerate(), w.getnframes()
        raw = w.readframes(frames)
    if width != 2:
        raise RuntimeError(f"{path}: {width * 8}-bit audio not supported")
    data = array.array("h")
    data.frombytes(raw)
    chans = [[data[i] / 32768.0 for i in range(c, len(data), channels)] for c in range(channels)]
    return chans, rate


def write_wav(path, chans, rate):
    data = array.array("h")
    n = len(chans[0])
    for i in range(n):
        for c in chans:
            data.append(int(max(-1.0, min(1.0, c[i])) * 32767))
    with wave.open(path, "wb") as w:
        w.setnchannels(len(chans))
        w.setsampwidth(2)
        w.setframerate(rate)
        w.writeframes(data.tobytes())


def rms(samples, start, length):
    seg = samples[start:start + length]
    return math.sqrt(sum(s * s for s in seg) / max(1, len(seg)))


def steadiest(mono, rate, seconds):
    """Start (samples) of the window whose loudness changes least, skipping the first and last second."""
    length = int(seconds * rate)
    block = max(1, rate // 10)
    if len(mono) <= length + 2 * rate:
        return max(0, (len(mono) - length) // 2)
    levels = [rms(mono, i, block) for i in range(0, len(mono) - block, block)]
    per_window = length // block
    best, best_score = rate, None
    for b in range(rate // block, len(levels) - per_window - rate // block):
        window = levels[b:b + per_window]
        mean = sum(window) / len(window)
        if mean < 0.02:
            continue  # near silence
        spread = math.sqrt(sum((x - mean) ** 2 for x in window) / len(window)) / mean
        if best_score is None or spread < best_score:
            best, best_score = b * block, spread
    return best


def split_halves(mono, rate):
    """Start/stop recording: the quietest moment in the middle third separates the two."""
    block = max(1, rate // 20)
    third = len(mono) // 3
    quiet = min(range(third, 2 * third - block, block), key=lambda i: rms(mono, i, block))
    return quiet


def process(name, chans, rate, start, length, loop, peak):
    mono = [sum(c[i] for c in chans) / len(chans) for i in range(len(chans[0]))]
    if start == "auto":
        s = steadiest(mono, rate, length)
    elif start in ("first-half", "second-half"):
        cut = split_halves(mono, rate)
        s, length = (0, cut / rate) if start == "first-half" else (cut, (len(mono) - cut) / rate)
    else:
        s = int(start * rate)
    e = len(mono) if length is None else min(len(mono), s + int(length * rate))
    out = [c[s:e] for c in chans] if name in STEREO and len(chans) > 1 else [mono[s:e]]
    n = len(out[0])
    if loop:
        # Cross-fade the end into the start: the loop point is inaudible.
        xf = min(int(0.25 * rate), n // 4)
        for c in out:
            for i in range(xf):
                a = i / xf
                c[i] = c[i] * a + c[n - xf + i] * (1.0 - a)
            del c[n - xf:]
    else:
        fade_in, fade_out = int(rate * 0.004), int(rate * 0.03)
        for c in out:
            for i in range(min(fade_in, len(c))):
                c[i] *= i / fade_in
            for i in range(min(fade_out, len(c))):
                c[-1 - i] *= i / fade_out
    top = max(1e-6, max(abs(x) for c in out for x in c))
    return [[x / top * peak for x in c] for c in out], len(out[0]) / rate


os.makedirs(WORK, exist_ok=True)
built, missing = [], []
cache = {}
for name, (rel, start, length, loop, peak) in SOUNDS.items():
    src = os.path.join(SOURCE, rel)
    if not os.path.exists(src):
        missing.append(rel)
        unreal.log_error(f"WE_AUDIO missing source for {name}: {src}")
        continue
    if src not in cache:
        key = "S_" + os.path.splitext(os.path.basename(src))[0].replace("-", "_")
        asset = import_file(src, TEMP, key)
        wav = os.path.join(WORK, key + ".wav")
        if not asset or not export_wav(asset, wav):
            unreal.log_error(f"WE_AUDIO could not decode {src}")
            continue
        cache[src] = read_wav(wav)
    chans, rate = cache[src]
    out, seconds = process(name, chans, rate, start, length, loop, peak)
    path = os.path.join(WORK, name + ".wav")
    write_wav(path, out, rate)
    if import_file(path, DEST, name):
        built.append(name)
        unreal.log(f"WE_AUDIO {name}: {rel} {seconds:.2f}s {'loop' if loop else 'one-shot'} {len(out)}ch")

# The decoded sources are only needed for the cut.
if unreal.EditorAssetLibrary.does_directory_exist(TEMP):
    unreal.EditorAssetLibrary.delete_directory(TEMP)

# 3D sound: louder near the source, fading with distance (machines carry further than small noises).
tools = unreal.AssetToolsHelpers.get_asset_tools()


def attenuation(name, inner, falloff):
    path = f"{DEST}/{name}"
    asset = unreal.EditorAssetLibrary.load_asset(path) if unreal.EditorAssetLibrary.does_asset_exist(path) else tools.create_asset(name, DEST, unreal.SoundAttenuation, unreal.SoundAttenuationFactory())
    settings = asset.get_editor_property("attenuation")
    settings.set_editor_property("attenuate", True)
    settings.set_editor_property("spatialize", True)
    settings.set_editor_property("attenuation_shape_extents", unreal.Vector(inner, 0, 0))
    settings.set_editor_property("falloff_distance", falloff)
    asset.set_editor_property("attenuation", settings)
    unreal.EditorAssetLibrary.save_asset(path)
    return asset


att_small = attenuation("SA_WESmall", 150.0, 1400.0)
att_machine = attenuation("SA_WEMachine", 250.0, 2800.0)
att_ambience = attenuation("SA_WEAmbience", 500.0, 1500.0)

for name in SOUNDS:
    path = f"{DEST}/{name}"
    if not unreal.EditorAssetLibrary.does_asset_exist(path):
        continue
    sound = unreal.EditorAssetLibrary.load_asset(path)
    sound.set_editor_property("looping", name in LOOPS)
    sound.set_editor_property("volume", VOLUME.get(name, 0.6))
    if name == "SFX_Outdoor":
        sound.set_editor_property("attenuation_settings", None)  # placed as an ambient actor with its own radius
    elif name == "SFX_RoomTone":
        sound.set_editor_property("attenuation_settings", att_ambience)
    elif name.startswith(("SFX_Van", "SFX_Shredder", "SFX_Press", "SFX_Motor", "SFX_Furnace", "SFX_Machine", "SFX_Pour", "SFX_Strap")):
        sound.set_editor_property("attenuation_settings", att_machine)
    else:
        sound.set_editor_property("attenuation_settings", att_small)
    unreal.EditorAssetLibrary.save_asset(path)

unreal.log(f"WE_AUDIO_BUILT {len(built)}/{len(SOUNDS)} missing={len(missing)}")
unreal.SystemLibrary.quit_editor()
