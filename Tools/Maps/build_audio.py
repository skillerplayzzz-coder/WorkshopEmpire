"""
Synthesises the placeholder workshop sounds (no third-party audio, so no licence questions) and imports
them as Sound Waves into /Game/WorkshopEmpire/Audio:
  SFX_PlaceItem        dull thud when an item is put on the workbench
  SFX_ScrewLoosen      ratchet clicks while a step is running
  SFX_PartRemoved      metallic clank when a step finishes
  SFX_DisassemblyDone  two-tone chime when an item is fully disassembled
  SFX_Denied           short low buzz (cancelled step)

Run before build_garage_map.py (the workbench references these sounds):
  UnrealEditor-Cmd.exe <WorkshopEmpire.uproject> -ExecutePythonScript="<this file>" -unattended -RenderOffscreen -nosplash

Replaced by recorded sound design in Phase 15.
"""
import math
import os
import random
import struct
import wave

import unreal

RATE = 44100
OUT_DIR = os.path.join(unreal.Paths.project_saved_dir(), "GeneratedAudio")
DEST = "/Game/WorkshopEmpire/Audio"
rng = random.Random(7)


def write_wav(name, samples):
    os.makedirs(OUT_DIR, exist_ok=True)
    path = os.path.join(OUT_DIR, name + ".wav")
    # 3 ms fade-in / 15 ms fade-out so the sounds start and end at zero (no clicks or DC pops).
    fade_in, fade_out = int(RATE * 0.003), int(RATE * 0.015)
    samples = list(samples)
    for i in range(min(fade_in, len(samples))):
        samples[i] *= i / fade_in
    for i in range(min(fade_out, len(samples))):
        samples[-1 - i] *= i / fade_out
    peak = max(1e-6, max(abs(s) for s in samples))
    with wave.open(path, "wb") as w:
        w.setnchannels(1)
        w.setsampwidth(2)
        w.setframerate(RATE)
        w.writeframes(b"".join(struct.pack("<h", int(max(-1.0, min(1.0, s / peak * 0.85)) * 32767)) for s in samples))
    return path


def silence(seconds):
    return [0.0] * int(RATE * seconds)


def thud():
    n = int(RATE * 0.35)
    out = []
    for i in range(n):
        t = i / RATE
        body = math.sin(2 * math.pi * 85 * t) * math.exp(-t * 14)
        knock = (rng.random() * 2 - 1) * math.exp(-t * 60) * 0.5
        out.append(body + knock)
    return out


def ratchet():
    out = silence(1.1)
    for k in range(10):
        start = int(RATE * (0.02 + k * 0.1))
        for i in range(int(RATE * 0.03)):
            t = i / RATE
            click = ((rng.random() * 2 - 1) * 0.6 + math.sin(2 * math.pi * 3200 * t) * 0.4) * math.exp(-t * 180)
            if start + i < len(out):
                out[start + i] += click
    return out


def clank():
    n = int(RATE * 0.8)
    partials = [(523, 1.0), (1187, 0.6), (1873, 0.45), (2711, 0.3), (3990, 0.15)]
    out = []
    for i in range(n):
        t = i / RATE
        s = sum(a * math.sin(2 * math.pi * f * t) * math.exp(-t * (4 + f / 900)) for f, a in partials)
        s += (rng.random() * 2 - 1) * math.exp(-t * 90) * 0.8
        out.append(s)
    return out


def chime():
    out = []
    for f, length in ((660, 0.22), (990, 0.5)):
        for i in range(int(RATE * length)):
            t = i / RATE
            env = min(1.0, t * 200) * math.exp(-t * 5)
            out.append((math.sin(2 * math.pi * f * t) + 0.3 * math.sin(2 * math.pi * 2 * f * t)) * env)
    return out


def buzz():
    n = int(RATE * 0.3)
    out = []
    for i in range(n):
        t = i / RATE
        square = 1.0 if math.sin(2 * math.pi * 140 * t) > 0 else -1.0
        out.append(square * 0.5 * min(1.0, t * 100) * min(1.0, (0.3 - t) * 40))
    return out


SOUNDS = {
    "SFX_PlaceItem": thud(),
    "SFX_ScrewLoosen": ratchet(),
    "SFX_PartRemoved": clank(),
    "SFX_DisassemblyDone": chime(),
    "SFX_Denied": buzz(),
}

tasks = []
for name, samples in SOUNDS.items():
    task = unreal.AssetImportTask()
    task.set_editor_property("filename", write_wav(name, samples))
    task.set_editor_property("destination_path", DEST)
    task.set_editor_property("destination_name", name)
    task.set_editor_property("automated", True)
    task.set_editor_property("replace_existing", True)
    task.set_editor_property("save", True)
    tasks.append(task)
unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks(tasks)

imported = [n for n in SOUNDS if unreal.EditorAssetLibrary.does_asset_exist(f"{DEST}/{n}")]
unreal.log(f"WE_AUDIO_BUILT {len(imported)}/{len(SOUNDS)}")
unreal.SystemLibrary.quit_editor()
