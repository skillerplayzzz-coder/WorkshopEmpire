"""
Downloads the free (CC0) source assets of Phase 15 into SourceArt/ (not in Git; Docs/ASSET_SOURCES.md
lists sources and licences). Plain Python 3, no extra packages:
  python Tools/Assets/download_assets.py

Poly Haven: models as glTF (1k textures for small things, 2k for big ones) and PBR textures (2k jpg:
colour, normal, ARM). Kenney: two sound packs (zip, extracted). Files that already exist are skipped.
"""
import json
import os
import sys
import urllib.request
import zipfile

ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), "..", ".."))
OUT = os.path.join(ROOT, "SourceArt")
UA = {"User-Agent": "WorkshopEmpire-AssetDownload/1.0 (game development, CC0 assets)"}

# Model id -> texture resolution. 1k for hand-held items, 2k for furniture and large props.
MODELS = {
    # Tools (items)
    "screwdriver": "1k", "pliers": "1k", "Drill_01": "1k", "retro_multimeter": "1k", "combination_wrench": "1k",
    "cross_pein_hammer": "1k", "adjustable_wrench": "1k", "ratchet_wrench": "1k", "measuring_tape_01": "1k",
    # Devices / items
    "Television_01": "2k", "classic_laptop": "1k", "hand_truck": "2k", "old_tyre": "1k", "propane_tank": "1k",
    "metal_jerrycan": "1k", "oil_tin": "1k", "lubricant_spray": "1k",
    # Furniture
    "steel_frame_shelves_01": "2k", "steel_frame_shelves_02": "2k", "worn_metal_rack": "2k", "metal_tool_chest": "2k",
    "metal_toolbox": "1k", "tool_cart": "2k", "bench_vice_01": "1k", "plastic_crate_01": "1k", "wooden_crate_01": "1k",
    "industrial_storage_cart": "2k",
    # Workshop machines and installations (decoration)
    "old_drill_press": "2k", "old_military_compressor": "2k", "portable_welding_cart": "2k", "portable_generator": "2k",
    "power_box_01": "1k", "utility_box_01": "1k", "modular_electric_cables": "1k", "modular_industrial_pipes_01": "1k",
    "modular_airduct_rectangular_01": "1k",
    # Lights
    "hanging_industrial_lamp": "1k", "mounted_fluorescent_lights": "1k", "industrial_wall_lamp": "1k",
    "security_light": "1k", "caged_hanging_light": "1k",
    # Environment
    "rollershutter_door": "2k", "Barrel_01": "1k", "barrel_03": "1k", "cardboard_box_01": "1k", "cement_bag": "1k",
    "korean_fire_extinguisher_01": "1k", "metal_trash_can": "1k", "plastic_broom": "1k", "ladder_sectioned_01": "1k",
    "metal_stool_01": "1k", "spray_paint_bottles": "1k", "rusted_wheel_rim_01": "1k",
}

TEXTURES = [
    "garage_floor", "concrete_floor_worn_001", "asphalt_02", "concrete_wall_008", "beige_wall_001", "factory_wall",
    "corrugated_iron_02", "brick_wall_005", "concrete_block_wall", "metal_plate", "rusty_painted_metal",
    "blue_metal_plate", "plywood", "oriented_strand_board",
]
TEXTURE_MAPS = {"Diffuse": "diff", "nor_gl": "nor_gl", "arm": "arm"}

KENNEY = {
    "impact-sounds": "https://kenney.nl/media/pages/assets/impact-sounds/87b4ddecda-1677589768/kenney_impact-sounds.zip",
    "interface-sounds": "https://kenney.nl/media/pages/assets/interface-sounds/fa43c1dd4d-1677589452/kenney_interface-sounds.zip",
}


def fetch_json(url):
    with urllib.request.urlopen(urllib.request.Request(url, headers=UA), timeout=60) as r:
        return json.loads(r.read().decode("utf-8"))


def download(url, path):
    if os.path.exists(path) and os.path.getsize(path) > 0:
        return 0
    os.makedirs(os.path.dirname(path), exist_ok=True)
    with urllib.request.urlopen(urllib.request.Request(url, headers=UA), timeout=180) as r:
        data = r.read()
    with open(path + ".part", "wb") as f:
        f.write(data)
    os.replace(path + ".part", path)
    return len(data)


def model(asset_id, res):
    files = fetch_json(f"https://api.polyhaven.com/files/{asset_id}")
    gltf = files["gltf"][res]["gltf"]
    folder = os.path.join(OUT, "PolyHaven", "models", asset_id)
    total = download(gltf["url"], os.path.join(folder, f"{asset_id}.gltf"))
    for rel, info in gltf.get("include", {}).items():
        total += download(info["url"], os.path.join(folder, rel.replace("/", os.sep)))
    return total


def texture(asset_id):
    files = fetch_json(f"https://api.polyhaven.com/files/{asset_id}")
    folder = os.path.join(OUT, "PolyHaven", "textures", asset_id)
    total = 0
    for key, short in TEXTURE_MAPS.items():
        entry = files.get(key, {}).get("2k", {})
        info = entry.get("jpg") or entry.get("png")
        if not info:
            print(f"  {asset_id}: no {key}")
            continue
        ext = "jpg" if entry.get("jpg") else "png"
        total += download(info["url"], os.path.join(folder, f"{asset_id}_{short}_2k.{ext}"))
    return total


def main():
    grand = 0
    for asset_id, res in MODELS.items():
        try:
            n = model(asset_id, res)
            grand += n
            print(f"model   {asset_id:34s} {n / 1e6:6.1f} MB")
        except Exception as e:  # keep going; the import script skips missing models
            print(f"model   {asset_id:34s} FAILED: {e}")
    for asset_id in TEXTURES:
        try:
            n = texture(asset_id)
            grand += n
            print(f"texture {asset_id:34s} {n / 1e6:6.1f} MB")
        except Exception as e:
            print(f"texture {asset_id:34s} FAILED: {e}")
    for name, url in KENNEY.items():
        try:
            path = os.path.join(OUT, "Kenney", f"kenney_{name}.zip")
            n = download(url, path)
            grand += n
            with zipfile.ZipFile(path) as z:
                z.extractall(os.path.join(OUT, "Kenney", name))
            print(f"sounds  {name:34s} {n / 1e6:6.1f} MB")
        except Exception as e:
            print(f"sounds  {name:34s} FAILED: {e}")
    print(f"WE_DOWNLOAD_DONE {grand / 1e6:.1f} MB")


if __name__ == "__main__":
    sys.exit(main())
