"""
Phase 17.5: master material for the procedural models (devices, vans, the workshop PC, containers) and its
instances. Unlike M_WESurface (projected in world space for walls), M_WEProp samples its textures with the mesh UVs,
so the texture moves with the object.
  /Game/WorkshopEmpire/Materials/M_WEProp, M_WEPropGlass, Props/MI_Prop_<Name>

Parameters: BaseColorTex (UV), TexStrength (0 = plain Tint, 1 = texture * Tint), Tint, UVScale, Roughness, Metallic,
ARMTex + ARMStrength (texture roughness/AO), NormalTex + NormalStrength, Emissive.
Run after import_assets.py:
  UnrealEditor-Cmd.exe <uproject> -ExecutePythonScript="<this file>" -unattended -RenderOffscreen -nosplash -nosteam
"""
import unreal

FOLDER = "/Game/WorkshopEmpire/Materials"
PROPS = FOLDER + "/Props"
TEX = "/Game/WorkshopEmpire/Art/Textures"
tools = unreal.AssetToolsHelpers.get_asset_tools()
mel = unreal.MaterialEditingLibrary
eal = unreal.EditorAssetLibrary
WHITE_LINEAR = FOLDER + "/T_WEWhiteLinear"


def new_material(name):
    path = f"{FOLDER}/{name}"
    if eal.does_asset_exist(path):
        eal.delete_asset(path)
    return tools.create_asset(name, FOLDER, unreal.Material, unreal.MaterialFactoryNew()), path


def build_prop():
    mat, path = new_material("M_WEProp")

    def scalar(name, value, x, y):
        s = mel.create_material_expression(mat, unreal.MaterialExpressionScalarParameter, x, y)
        s.set_editor_property("parameter_name", name)
        s.set_editor_property("default_value", value)
        return s

    def vector(name, value, x, y):
        v = mel.create_material_expression(mat, unreal.MaterialExpressionVectorParameter, x, y)
        v.set_editor_property("parameter_name", name)
        v.set_editor_property("default_value", value)
        return v

    def tex(name, default, x, y, sampler):
        t = mel.create_material_expression(mat, unreal.MaterialExpressionTextureSampleParameter2D, x, y)
        t.set_editor_property("parameter_name", name)
        t.set_editor_property("texture", unreal.load_asset(default))
        t.set_editor_property("sampler_type", sampler)
        return t

    def node(cls, x, y):
        return mel.create_material_expression(mat, cls, x, y)

    def link(a, a_out, b, b_in):
        mel.connect_material_expressions(a, a_out, b, b_in)

    uv = node(unreal.MaterialExpressionTextureCoordinate, -1500, 0)
    uv_scaled = node(unreal.MaterialExpressionMultiply, -1350, 0)
    link(uv, "", uv_scaled, "A")
    link(scalar("UVScale", 1.0, -1500, 100), "", uv_scaled, "B")

    base = tex("BaseColorTex", "/Engine/EngineResources/WhiteSquareTexture", -1150, -400, unreal.MaterialSamplerType.SAMPLERTYPE_COLOR)
    link(uv_scaled, "", base, "UVs")
    lerp_tex = node(unreal.MaterialExpressionLinearInterpolate, -850, -400)
    lerp_tex.set_editor_property("const_a", 1.0)
    link(base, "RGB", lerp_tex, "B")
    link(scalar("TexStrength", 0.0, -1000, -250), "", lerp_tex, "Alpha")
    tinted = node(unreal.MaterialExpressionMultiply, -650, -350)
    link(lerp_tex, "", tinted, "A")
    link(vector("Tint", unreal.LinearColor(0.7, 0.7, 0.7, 1.0), -850, -200), "", tinted, "B")
    mel.connect_material_property(tinted, "", unreal.MaterialProperty.MP_BASE_COLOR)

    arm = tex("ARMTex", WHITE_LINEAR, -1150, 200, unreal.MaterialSamplerType.SAMPLERTYPE_MASKS)
    link(uv_scaled, "", arm, "UVs")
    arm_strength = scalar("ARMStrength", 0.0, -1000, 380)
    rough = node(unreal.MaterialExpressionLinearInterpolate, -700, 200)
    link(scalar("Roughness", 0.55, -900, 150), "", rough, "A")
    link(arm, "G", rough, "B")
    link(arm_strength, "", rough, "Alpha")
    mel.connect_material_property(rough, "", unreal.MaterialProperty.MP_ROUGHNESS)
    ao = node(unreal.MaterialExpressionLinearInterpolate, -700, 330)
    ao.set_editor_property("const_a", 1.0)
    link(arm, "R", ao, "B")
    link(arm_strength, "", ao, "Alpha")
    mel.connect_material_property(ao, "", unreal.MaterialProperty.MP_AMBIENT_OCCLUSION)
    mel.connect_material_property(scalar("Metallic", 0.0, -700, 450), "", unreal.MaterialProperty.MP_METALLIC)

    nor = tex("NormalTex", "/Engine/EngineMaterials/FlatNormal", -1150, 600, unreal.MaterialSamplerType.SAMPLERTYPE_NORMAL)
    link(uv_scaled, "", nor, "UVs")
    flat = node(unreal.MaterialExpressionConstant3Vector, -900, 750)
    flat.set_editor_property("constant", unreal.LinearColor(0, 0, 1, 1))
    nlerp = node(unreal.MaterialExpressionLinearInterpolate, -700, 650)
    link(flat, "", nlerp, "A")
    link(nor, "RGB", nlerp, "B")
    link(scalar("NormalStrength", 0.0, -900, 850), "", nlerp, "Alpha")
    mel.connect_material_property(nlerp, "", unreal.MaterialProperty.MP_NORMAL)

    mel.connect_material_property(vector("Emissive", unreal.LinearColor(0, 0, 0, 1), -700, 950), "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)
    mel.recompile_material(mat)
    eal.save_asset(path)
    return mat


def build_glass():
    mat, path = new_material("M_WEPropGlass")
    mat.set_editor_property("blend_mode", unreal.BlendMode.BLEND_TRANSLUCENT)
    mat.set_editor_property("translucency_lighting_mode", unreal.TranslucencyLightingMode.TLM_SURFACE_PER_PIXEL_LIGHTING)
    tint = mel.create_material_expression(mat, unreal.MaterialExpressionVectorParameter, -500, -100)
    tint.set_editor_property("parameter_name", "Tint")
    tint.set_editor_property("default_value", unreal.LinearColor(0.02, 0.025, 0.03, 1))
    mel.connect_material_property(tint, "", unreal.MaterialProperty.MP_BASE_COLOR)
    op = mel.create_material_expression(mat, unreal.MaterialExpressionScalarParameter, -500, 50)
    op.set_editor_property("parameter_name", "Opacity")
    op.set_editor_property("default_value", 0.55)
    mel.connect_material_property(op, "", unreal.MaterialProperty.MP_OPACITY)
    rough = mel.create_material_expression(mat, unreal.MaterialExpressionConstant, -500, 150)
    rough.set_editor_property("r", 0.04)
    mel.connect_material_property(rough, "", unreal.MaterialProperty.MP_ROUGHNESS)
    spec = mel.create_material_expression(mat, unreal.MaterialExpressionConstant, -500, 250)
    spec.set_editor_property("r", 1.0)
    mel.connect_material_property(spec, "", unreal.MaterialProperty.MP_SPECULAR)
    mel.recompile_material(mat)
    eal.save_asset(path)
    return mat


def tex_path(tid, kind):
    p = f"{TEX}/{tid}/T_{tid}_{kind}"
    return p if eal.does_asset_exist(p) else None


prop = build_prop()
glass = build_glass()

# name: (tint rgb, roughness, metallic, texture id or None, tex strength, uv scale, emissive rgb or None)
INSTANCES = {
    "PlasticBeige": ((0.62, 0.58, 0.47), 0.55, 0.0, None, 0.0, 1.0, None),
    "PlasticGrey": ((0.20, 0.21, 0.22), 0.55, 0.0, None, 0.0, 1.0, None),
    "PlasticBlack": ((0.025, 0.025, 0.028), 0.5, 0.0, None, 0.0, 1.0, None),
    "PlasticWhite": ((0.75, 0.75, 0.73), 0.45, 0.0, None, 0.0, 1.0, None),
    "PlasticRed": ((0.55, 0.05, 0.03), 0.45, 0.0, None, 0.0, 1.0, None),
    "PlasticYellow": ((0.75, 0.48, 0.03), 0.45, 0.0, None, 0.0, 1.0, None),
    "MetalPainted": ((0.30, 0.34, 0.33), 0.6, 0.35, "rusty_painted_metal", 0.3, 1.5, None),
    "MetalPaintedBlue": ((0.10, 0.20, 0.36), 0.55, 0.35, "rusty_painted_metal", 0.25, 1.5, None),
    "MetalPaintedGreen": ((0.13, 0.26, 0.17), 0.55, 0.35, "rusty_painted_metal", 0.3, 1.5, None),
    "MetalPaintedGrey": ((0.35, 0.36, 0.37), 0.55, 0.35, "metal_plate_02", 0.35, 1.0, None),
    "SteelBrushed": ((0.62, 0.63, 0.64), 0.42, 1.0, "metal_plate_02", 0.45, 1.0, None),
    "Chrome": ((0.85, 0.86, 0.88), 0.12, 1.0, None, 0.0, 1.0, None),
    "Aluminium": ((0.80, 0.81, 0.82), 0.38, 1.0, None, 0.0, 1.0, None),
    "Copper": ((0.93, 0.52, 0.32), 0.32, 1.0, None, 0.0, 1.0, None),
    "Gold": ((1.0, 0.76, 0.33), 0.22, 1.0, None, 0.0, 1.0, None),
    "PCB": ((0.03, 0.22, 0.09), 0.35, 0.0, None, 0.0, 1.0, None),
    "Chip": ((0.03, 0.03, 0.035), 0.4, 0.0, None, 0.0, 1.0, None),
    "Rubber": ((0.018, 0.018, 0.02), 0.9, 0.0, None, 0.0, 1.0, None),
    "Rust": ((0.9, 0.85, 0.8), 0.8, 0.4, "rusty_painted_metal", 1.0, 1.2, None),
    "RustHeavy": ((0.85, 0.8, 0.75), 0.85, 0.3, "green_metal_rust", 1.0, 1.0, None),
    "Wood": ((0.95, 0.9, 0.85), 0.7, 0.0, "plywood", 1.0, 1.0, None),
    "Cardboard": ((0.50, 0.36, 0.21), 0.9, 0.0, None, 0.0, 1.0, None),
    "ScreenDark": ((0.012, 0.014, 0.016), 0.08, 0.0, None, 0.0, 1.0, None),
    "LedGreen": ((0.0, 0.1, 0.0), 0.3, 0.0, None, 0.0, 1.0, (0.2, 3.0, 0.3)),
    "LedRed": ((0.1, 0.0, 0.0), 0.3, 0.0, None, 0.0, 1.0, (3.0, 0.15, 0.1)),
    "LedAmber": ((0.1, 0.06, 0.0), 0.3, 0.0, None, 0.0, 1.0, (3.0, 1.4, 0.1)),
    "Headlight": ((0.8, 0.8, 0.8), 0.05, 0.0, None, 0.0, 1.0, (0.6, 0.6, 0.55)),
    "Taillight": ((0.35, 0.01, 0.01), 0.1, 0.0, None, 0.0, 1.0, (0.4, 0.0, 0.0)),
    "Indicator": ((0.6, 0.3, 0.0), 0.1, 0.0, None, 0.0, 1.0, (0.3, 0.12, 0.0)),
    "VanOld": ((0.66, 0.64, 0.58), 0.5, 0.05, "rusty_painted_metal", 0.12, 0.25, None),  # faded white paint, a little grime
    "VanNew": ((0.62, 0.64, 0.66), 0.25, 0.1, None, 0.0, 1.0, None),
    "VanNewAccent": ((0.06, 0.20, 0.42), 0.25, 0.15, None, 0.0, 1.0, None),
    "TrimBlack": ((0.03, 0.03, 0.035), 0.6, 0.0, None, 0.0, 1.0, None),
    "TireRubber": ((0.014, 0.014, 0.015), 0.88, 0.0, "rubber_tiles", 0.04, 3.0, None),  # (quality update: the texture made tyres grey)
    "Container": ((0.55, 0.14, 0.08), 0.6, 0.4, "rusty_corrugated_iron", 0.55, 0.5, None),
    "ContainerBlue": ((0.10, 0.22, 0.40), 0.6, 0.4, "rusty_corrugated_iron", 0.45, 0.5, None),
    "DumpsterGreen": ((0.10, 0.25, 0.14), 0.6, 0.4, "rusty_painted_metal", 0.6, 0.8, None),
    "Concrete": ((0.75, 0.74, 0.72), 0.85, 0.0, "concrete_wall_008", 1.0, 0.5, None),
    "OsDesktop": ((0.01, 0.02, 0.04), 0.1, 0.0, None, 0.0, 1.0, (0.05, 0.12, 0.25)),
    # Quality update: parked cars (build_cars.py) - car paint (clear coat look: low roughness, a little metal flake),
    # lights switched off, and a faded, grimy paint for old workshop vehicles.
    # (dielectric clear coat: little metal, else the sky reflection washes the colour out in the sun)
    "CarPaintRed": ((0.09, 0.007, 0.006), 0.32, 0.05, None, 0.0, 1.0, None),
    "CarPaintBlue": ((0.008, 0.02, 0.055), 0.32, 0.05, None, 0.0, 1.0, None),
    "CarPaintGreen": ((0.01, 0.032, 0.018), 0.34, 0.05, None, 0.0, 1.0, None),
    "CarPaintBeige": ((0.2, 0.165, 0.11), 0.36, 0.05, None, 0.0, 1.0, None),
    "CarPaintWhite": ((0.42, 0.42, 0.40), 0.32, 0.0, None, 0.0, 1.0, None),
    "CarPaintBlack": ((0.008, 0.008, 0.009), 0.28, 0.05, None, 0.0, 1.0, None),
    "CarPaintSilver": ((0.30, 0.31, 0.32), 0.3, 0.6, None, 0.0, 1.0, None),
    "CarPaintBrown": ((0.07, 0.035, 0.018), 0.36, 0.05, None, 0.0, 1.0, None),
    "CarPaintMaroon": ((0.04, 0.004, 0.007), 0.32, 0.05, None, 0.0, 1.0, None),
    "CarPaintFaded": ((0.22, 0.24, 0.20), 0.6, 0.05, "rusty_painted_metal", 0.22, 0.35, None),
    "CarInterior": ((0.03, 0.028, 0.026), 0.8, 0.0, None, 0.0, 1.0, None),
    "HeadlightOff": ((0.75, 0.76, 0.75), 0.04, 0.2, None, 0.0, 1.0, None),
    "TaillightOff": ((0.28, 0.01, 0.01), 0.08, 0.0, None, 0.0, 1.0, None),
    "IndicatorOff": ((0.55, 0.28, 0.02), 0.08, 0.0, None, 0.0, 1.0, None),
    "FluorescentTube": ((0.9, 0.9, 0.88), 0.3, 0.0, None, 0.0, 1.0, (6.0, 5.6, 5.0)),
    # Quality overhaul: industrial machine paints (a little wear) and a glowing furnace slit.
    "MachineYellow": ((0.32, 0.19, 0.01), 0.55, 0.05, "rusty_painted_metal", 0.12, 0.6, None),
    "MachineTeal": ((0.02, 0.11, 0.11), 0.55, 0.05, "rusty_painted_metal", 0.12, 0.6, None),
    "MachineRed": ((0.2, 0.025, 0.015), 0.55, 0.05, "rusty_painted_metal", 0.12, 0.6, None),
    "MachineBlue": ((0.025, 0.06, 0.17), 0.55, 0.05, "rusty_painted_metal", 0.12, 0.6, None),
    "MachineGreen": ((0.05, 0.12, 0.06), 0.55, 0.05, "rusty_painted_metal", 0.12, 0.6, None),
    "MachineOrange": ((0.3, 0.08, 0.01), 0.55, 0.05, "rusty_painted_metal", 0.12, 0.6, None),
    "MachineGrey": ((0.07, 0.075, 0.08), 0.55, 0.05, "rusty_painted_metal", 0.15, 0.6, None),
    "Refractory": ((0.42, 0.36, 0.30), 0.85, 0.0, "concrete_wall_008", 0.6, 0.5, None),
    "FurnaceGlow": ((0.3, 0.08, 0.0), 0.6, 0.0, None, 0.0, 1.0, (8.0, 2.4, 0.3)),
    # Traffic signal lenses: dark tinted glass when off, bright enough for daylight when on.
    "SignalRedOff": ((0.05, 0.004, 0.004), 0.1, 0.0, None, 0.0, 1.0, None),
    "SignalAmberOff": ((0.07, 0.035, 0.002), 0.1, 0.0, None, 0.0, 1.0, None),
    "SignalGreenOff": ((0.004, 0.035, 0.018), 0.1, 0.0, None, 0.0, 1.0, None),
    "SignalRed": ((0.4, 0.02, 0.01), 0.1, 0.0, None, 0.0, 1.0, (12.0, 0.4, 0.1)),
    "SignalGreen": ((0.02, 0.4, 0.2), 0.1, 0.0, None, 0.0, 1.0, (0.3, 10.0, 3.5)),
}
count = 0
for name, (tint, rough, metal, tid, strength, uvs, emissive) in INSTANCES.items():
    path = f"{PROPS}/MI_Prop_{name}"
    mi = eal.load_asset(path) if eal.does_asset_exist(path) else tools.create_asset(f"MI_Prop_{name}", PROPS, unreal.MaterialInstanceConstant, unreal.MaterialInstanceConstantFactoryNew())
    mi.set_editor_property("parent", prop)
    mel.set_material_instance_vector_parameter_value(mi, "Tint", unreal.LinearColor(*tint, 1.0))
    mel.set_material_instance_scalar_parameter_value(mi, "Roughness", rough)
    mel.set_material_instance_scalar_parameter_value(mi, "Metallic", metal)
    mel.set_material_instance_scalar_parameter_value(mi, "UVScale", uvs)
    if tid and tex_path(tid, "diff"):
        mel.set_material_instance_texture_parameter_value(mi, "BaseColorTex", unreal.load_asset(tex_path(tid, "diff")))
        mel.set_material_instance_scalar_parameter_value(mi, "TexStrength", strength)
        if tex_path(tid, "arm"):
            mel.set_material_instance_texture_parameter_value(mi, "ARMTex", unreal.load_asset(tex_path(tid, "arm")))
            mel.set_material_instance_scalar_parameter_value(mi, "ARMStrength", min(1.0, strength))
        if tex_path(tid, "nor"):
            mel.set_material_instance_texture_parameter_value(mi, "NormalTex", unreal.load_asset(tex_path(tid, "nor")))
            mel.set_material_instance_scalar_parameter_value(mi, "NormalStrength", min(1.0, strength + 0.2))
    if emissive:
        mel.set_material_instance_vector_parameter_value(mi, "Emissive", unreal.LinearColor(*emissive, 1.0))
    eal.save_asset(path)
    count += 1
for name, (tint, opacity) in {"Glass": ((0.02, 0.025, 0.03), 0.45), "GlassClear": ((0.3, 0.35, 0.38), 0.18), "CrtGlass": ((0.02, 0.03, 0.03), 0.85),
                               "CarGlass": ((0.012, 0.016, 0.02), 0.72)}.items():
    path = f"{PROPS}/MI_Prop_{name}"
    mi = eal.load_asset(path) if eal.does_asset_exist(path) else tools.create_asset(f"MI_Prop_{name}", PROPS, unreal.MaterialInstanceConstant, unreal.MaterialInstanceConstantFactoryNew())
    mi.set_editor_property("parent", glass)
    mel.set_material_instance_vector_parameter_value(mi, "Tint", unreal.LinearColor(*tint, 1.0))
    mel.set_material_instance_scalar_parameter_value(mi, "Opacity", opacity)
    eal.save_asset(path)
    count += 1
unreal.log(f"WE_PROP_MATERIALS_BUILT {count}")
unreal.SystemLibrary.quit_editor()
