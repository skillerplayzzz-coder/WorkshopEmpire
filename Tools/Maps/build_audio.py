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

Phase 15 adds machine loops, start/stop and cycle sounds, ambience and distance attenuation (SA_WE*).
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


# --- Phase 15: machine and ambience sounds --------------------------------------------------------

class OnePole:
    """Simple low-pass filter (coefficient from a cutoff frequency)."""
    def __init__(self, cutoff):
        self.a = math.exp(-2 * math.pi * cutoff / RATE)
        self.y = 0.0

    def __call__(self, x):
        self.y = (1 - self.a) * x + self.a * self.y
        return self.y


def loopable(samples, seconds_xfade=0.25):
    """Cross-fades the tail into the head so the sound loops without a click."""
    n = int(RATE * seconds_xfade)
    body = samples[:-n]
    tail = samples[-n:]
    for i in range(n):
        w = i / n
        body[i] = body[i] * w + tail[i] * (1 - w)
    return body


def noise():
    return rng.random() * 2 - 1


def shredder_loop():
    n = int(RATE * 3.25)
    lp, lp2 = OnePole(900), OnePole(220)
    out, crunch = [], 0.0
    for i in range(n):
        t = i / RATE
        motor = 0.5 * math.sin(2 * math.pi * 47 * t) + 0.25 * math.sin(2 * math.pi * 94 * t) + 0.12 * math.sin(2 * math.pi * 141 * t)
        if rng.random() < 0.0009:
            crunch = 1.0
        crunch *= 0.9993
        grind = lp(noise()) * (0.35 + 0.8 * crunch) * (0.75 + 0.25 * math.sin(2 * math.pi * 6.5 * t))
        rumble = lp2(noise()) * 0.8
        out.append(motor * 0.5 + grind + rumble)
    return loopable(out)


def press_loop():
    n = int(RATE * 3.25)
    hiss = OnePole(3000)
    out = []
    for i in range(n):
        t = i / RATE
        hum = 0.5 * math.sin(2 * math.pi * 118 * t) + 0.2 * math.sin(2 * math.pi * 236 * t)
        pump = 0.18 * math.sin(2 * math.pi * 410 * t) * (0.6 + 0.4 * math.sin(2 * math.pi * 3 * t))
        out.append(hum * 0.6 + pump + hiss(noise()) * 0.12)
    return loopable(out)


def motor_loop():
    n = int(RATE * 2.25)
    air = OnePole(1200)
    out = []
    for i in range(n):
        t = i / RATE
        hum = 0.45 * math.sin(2 * math.pi * 100 * t) + 0.2 * math.sin(2 * math.pi * 50 * t) + 0.08 * math.sin(2 * math.pi * 300 * t)
        whine = 0.07 * math.sin(2 * math.pi * 1240 * t)
        out.append(hum + whine + air(noise()) * 0.25)
    return loopable(out)


def furnace_loop():
    n = int(RATE * 4.25)
    roar, roar2 = OnePole(380), OnePole(90)
    out = []
    for i in range(n):
        t = i / RATE
        body = roar(noise()) * (0.9 + 0.2 * math.sin(2 * math.pi * 0.7 * t)) + roar2(noise()) * 1.4
        crackle = (noise() * math.exp(-((i % 1931) / 60.0))) * 0.25 if rng.random() < 0.3 else 0.0
        out.append(body + crackle)
    return loopable(out)


def sweep(f0, f1, seconds, decay_out):
    n = int(RATE * seconds)
    out, phase = [], 0.0
    lp = OnePole(800)
    for i in range(n):
        t = i / RATE
        f = f0 + (f1 - f0) * (t / seconds)
        phase += 2 * math.pi * f / RATE
        env = min(1.0, t * 8) if not decay_out else (1 - t / seconds)
        out.append((math.sin(phase) * 0.6 + math.sin(2 * phase) * 0.2 + lp(noise()) * 0.3) * env)
    return out


def machine_start():
    out = sweep(18, 100, 1.2, False)
    for i in range(int(RATE * 0.04)):  # relay click
        out[i] += noise() * math.exp(-i / 200) * 1.2
    return out


def machine_stop():
    return sweep(100, 15, 1.4, True)


def crunch():
    n = int(RATE * 0.9)
    lp = OnePole(1600)
    out = []
    for i in range(n):
        t = i / RATE
        grains = lp(noise()) * (1.0 if rng.random() < 0.6 else 0.3) * math.exp(-t * 4)
        ring = sum(a * math.sin(2 * math.pi * f * t) for f, a in ((870, 0.3), (1630, 0.2))) * math.exp(-t * 9)
        out.append(grains + ring)
    return out


def press_slam():
    n = int(RATE * 1.1)
    hiss = OnePole(4000)
    out = []
    for i in range(n):
        t = i / RATE
        thump = math.sin(2 * math.pi * 55 * t) * math.exp(-t * 9) * 1.4
        bang = sum(a * math.sin(2 * math.pi * f * t) for f, a in ((310, 0.5), (742, 0.35), (1290, 0.2))) * math.exp(-t * 7)
        release = hiss(noise()) * 0.35 * (math.exp(-(t - 0.35) * 5) if t > 0.35 else 0.0)
        out.append(thump + bang + noise() * math.exp(-t * 80) * 0.6 + release)
    return out


def pour_sizzle():
    n = int(RATE * 1.4)
    hiss = OnePole(5000)
    out = []
    for i in range(n):
        t = i / RATE
        clink = (math.sin(2 * math.pi * 2100 * t) * 0.4 + math.sin(2 * math.pi * 3300 * t) * 0.25) * math.exp(-t * 12)
        sizzle = hiss(noise()) * 0.5 * min(1.0, t * 6) * math.exp(-t * 1.6)
        out.append(clink + sizzle)
    return out


def strap():
    n = int(RATE * 0.7)
    out = []
    lp = OnePole(2500)
    for i in range(n):
        t = i / RATE
        zip_ = lp(noise()) * (0.8 if t < 0.45 else 0.0) * (0.5 + 0.5 * math.sin(2 * math.pi * 35 * t))
        clack = noise() * math.exp(-(t - 0.5) * 60) * 0.9 if t >= 0.5 else 0.0
        out.append(zip_ + clack)
    return out


def room_tone():
    n = int(RATE * 6.25)
    lp = OnePole(160)
    out = []
    for i in range(n):
        t = i / RATE
        out.append(lp(noise()) * 1.4 + 0.05 * math.sin(2 * math.pi * 50 * t) + 0.03 * math.sin(2 * math.pi * 100 * t))
    return loopable(out)


def outdoor():
    n = int(RATE * 10.25)
    wind = OnePole(500)
    out = [0.0] * n
    for i in range(n):
        t = i / RATE
        out[i] = wind(noise()) * (0.55 + 0.45 * math.sin(2 * math.pi * 0.11 * t) * math.sin(2 * math.pi * 0.037 * t + 1.0))
    # A few distant bird chirps.
    for start in (0.8, 1.05, 3.9, 6.2, 6.4, 6.55, 8.7):
        s0 = int(RATE * start)
        f0 = rng.uniform(2600, 3800)
        for i in range(int(RATE * 0.09)):
            t = i / RATE
            f = f0 + 1800 * t / 0.09
            if s0 + i < n:
                out[s0 + i] += math.sin(2 * math.pi * f * t) * math.sin(math.pi * t / 0.09) * 0.12
    return loopable(out)


def van_drive():
    """Diesel van pulling away: idle rumble, then revving up while it drives off (about 4 s)."""
    n = int(RATE * 4.2)
    lp = OnePole(400)
    out, phase = [], 0.0
    for i in range(n):
        t = i / RATE
        rev = min(1.0, max(0.0, (t - 0.4) / 2.2))
        f = 32 + 38 * rev
        phase += 2 * math.pi * f / RATE
        firing = (math.sin(phase) + 0.5 * math.sin(2 * phase) + 0.3 * math.sin(3 * phase)) * (0.7 + 0.3 * math.sin(phase * 0.5))
        env = min(1.0, t * 4) * (1.0 if t < 3.2 else max(0.0, 1 - (t - 3.2) / 1.0))
        out.append((firing * 0.6 + lp(noise()) * 0.5) * env)
    return out


def step():
    """Soft shoe on concrete: a short low thump with a little grit."""
    n = int(RATE * 0.16)
    lp, grit = OnePole(700), OnePole(3500)
    out = []
    for i in range(n):
        t = i / RATE
        out.append(lp(noise()) * math.exp(-t * 45) * 1.6 + grit(noise()) * math.exp(-t * 90) * 0.3 + math.sin(2 * math.pi * 90 * t) * math.exp(-t * 60) * 0.4)
    return out


SOUNDS = {
    "SFX_PlaceItem": thud(),
    "SFX_ScrewLoosen": ratchet(),
    "SFX_PartRemoved": clank(),
    "SFX_DisassemblyDone": chime(),
    "SFX_Denied": buzz(),
    # Phase 15
    "SFX_ShredderLoop": shredder_loop(),
    "SFX_PressLoop": press_loop(),
    "SFX_MotorLoop": motor_loop(),
    "SFX_FurnaceLoop": furnace_loop(),
    "SFX_MachineStart": machine_start(),
    "SFX_MachineStop": machine_stop(),
    "SFX_ShredderCrunch": crunch(),
    "SFX_PressSlam": press_slam(),
    "SFX_PourSizzle": pour_sizzle(),
    "SFX_Strap": strap(),
    "SFX_RoomTone": room_tone(),
    "SFX_Outdoor": outdoor(),
    "SFX_VanDrive": van_drive(),
    "SFX_Step": step(),
}
LOOPS = {"SFX_ShredderLoop", "SFX_PressLoop", "SFX_MotorLoop", "SFX_FurnaceLoop", "SFX_RoomTone", "SFX_Outdoor"}
# Volume per sound (synthesised files are all normalised).
VOLUME = {"SFX_ShredderLoop": 0.55, "SFX_PressLoop": 0.4, "SFX_MotorLoop": 0.3, "SFX_FurnaceLoop": 0.5, "SFX_MachineStart": 0.5,
          "SFX_MachineStop": 0.45, "SFX_RoomTone": 0.18, "SFX_Outdoor": 0.3, "SFX_ShredderCrunch": 0.7, "SFX_PressSlam": 0.8}

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
    wave = unreal.EditorAssetLibrary.load_asset(path)
    if not wave:
        continue
    wave.set_editor_property("looping", name in LOOPS)
    wave.set_editor_property("volume", VOLUME.get(name, 0.6))
    if name == "SFX_Outdoor":
        wave.set_editor_property("attenuation_settings", None)  # placed as an ambient actor with its own radius
    elif name == "SFX_RoomTone":
        wave.set_editor_property("attenuation_settings", att_ambience)
    elif name.startswith(("SFX_Van", "SFX_Shredder", "SFX_Press", "SFX_Motor", "SFX_Furnace", "SFX_Machine", "SFX_Pour", "SFX_Strap")):
        wave.set_editor_property("attenuation_settings", att_machine)
    else:
        wave.set_editor_property("attenuation_settings", att_small)
    unreal.EditorAssetLibrary.save_asset(path)

imported = [n for n in SOUNDS if unreal.EditorAssetLibrary.does_asset_exist(f"{DEST}/{n}")]
unreal.log(f"WE_AUDIO_BUILT {len(imported)}/{len(SOUNDS)}")
unreal.SystemLibrary.quit_editor()