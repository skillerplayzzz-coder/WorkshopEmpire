"""
Phase 17.5: imports the PNGs drawn by Tools/UI/make_icons.py into /Game/WorkshopEmpire/UI as UI textures
(no mip maps, UI compression, UI texture group). Existing textures are re-imported (overwritten).
  UnrealEditor-Cmd.exe <uproject> -ExecutePythonScript="<this file>" -unattended -nosplash
"""
import os
import unreal

ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), "..", ".."))
SRC = os.path.join(ROOT, "SourceArt", "UI")
DEST = "/Game/WorkshopEmpire/UI"
tools = unreal.AssetToolsHelpers.get_asset_tools()


def import_png(path, dest, tiling=False):
    name = os.path.splitext(os.path.basename(path))[0]
    task = unreal.AssetImportTask()
    task.set_editor_property("filename", path)
    task.set_editor_property("destination_path", dest)
    task.set_editor_property("destination_name", name)
    task.set_editor_property("automated", True)
    task.set_editor_property("replace_existing", True)
    task.set_editor_property("save", False)
    tools.import_asset_tasks([task])
    tex = unreal.load_asset(f"{dest}/{name}")
    if not tex:
        unreal.log_error(f"WE_UI import failed: {path}")
        return 0
    tex.set_editor_property("compression_settings", unreal.TextureCompressionSettings.TC_EDITOR_ICON)
    tex.set_editor_property("mip_gen_settings", unreal.TextureMipGenSettings.TMGS_NO_MIPMAPS)
    tex.set_editor_property("lod_group", unreal.TextureGroup.TEXTUREGROUP_UI)
    tex.set_editor_property("srgb", True)
    if tiling:
        tex.set_editor_property("address_x", unreal.TextureAddress.TA_WRAP)
        tex.set_editor_property("address_y", unreal.TextureAddress.TA_WRAP)
    unreal.EditorAssetLibrary.save_loaded_asset(tex)
    return 1


count = 0
icons = os.path.join(SRC, "Icons")
for f in sorted(os.listdir(icons)):
    if f.endswith(".png"):
        count += import_png(os.path.join(icons, f), f"{DEST}/Icons")
for f, tiling in (("T_HazardStripe.png", True), ("T_FerritLogo.png", False), ("T_FerritWallpaper.png", False), ("T_PanelShadow.png", False)):
    count += import_png(os.path.join(SRC, f), DEST, tiling)
# Item icons rendered by the game (-WEBakeIcons) land in Saved/ItemIcons.
baked = os.path.join(ROOT, "Saved", "ItemIcons")
if os.path.isdir(baked):
    for f in sorted(os.listdir(baked)):
        if f.endswith(".png"):
            count += import_png(os.path.join(baked, f), f"{DEST}/ItemIcons")
unreal.log(f"WE_UI_IMPORTED {count}")
