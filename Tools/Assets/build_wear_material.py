"""
Phase 21: M_WEWear, the overlay material that shows how worn a device is (drawn over its own materials):
dirt blotches (more on top faces), scratches (thin lines), yellowed plastic (tint) and damage (dark burnt/cracked spots),
all procedural in the object's own space (they move with it). Parameters: Dirt, Scratches, Yellowing, Damage (0-1), Seed.
FWEItemInstance::GetWear decides the amounts from condition, quality and the item's id; cleaning lowers the dirt.

  UnrealEditor-Cmd.exe <uproject> -ExecutePythonScript="<this file>" -unattended -RenderOffscreen -nosplash -nosteam
"""
import unreal

FOLDER = "/Game/WorkshopEmpire/Materials"
NAME = "M_WEWear"
tools = unreal.AssetToolsHelpers.get_asset_tools()
mel = unreal.MaterialEditingLibrary
eal = unreal.EditorAssetLibrary

CODE = r"""
struct FWear
{
    float H(float3 q) { return frac(sin(dot(q, float3(127.1, 311.7, 74.7))) * 43758.5453); }
    float N3(float3 x)
    {
        float3 i = floor(x);
        float3 f = frac(x);
        f = f * f * (3.0 - 2.0 * f);
        float a = H(i), b = H(i + float3(1, 0, 0)), c = H(i + float3(0, 1, 0)), d = H(i + float3(1, 1, 0));
        float e = H(i + float3(0, 0, 1)), g = H(i + float3(1, 0, 1)), k = H(i + float3(0, 1, 1)), l = H(i + float3(1, 1, 1));
        return lerp(lerp(lerp(a, b, f.x), lerp(c, d, f.x), f.y), lerp(lerp(e, g, f.x), lerp(k, l, f.x), f.y), f.z);
    }
    float Fbm(float3 x) { return N3(x) * 0.6 + N3(x * 2.1) * 0.3 + N3(x * 4.3) * 0.1; }
};
FWear W;
float3 P = Pos;
float up = saturate(Nrm.z * 0.5 + 0.5);
// Grime: an even film that darkens the surface, a bit uneven, more on top faces; heavy dirt adds small spots.
float soft = W.Fbm(P * 0.18 + Seed);
float grime = saturate(Dirt * (0.55 + 0.35 * soft + 0.2 * up));
float spotN = W.N3(P * 1.7 + Seed * 2.0);
float spots = saturate((spotN - (1.0 - Dirt * 0.35)) * 8.0) * step(0.45, Dirt);
float sa = W.N3(P * float3(2.6, 0.11, 2.6) + Seed * 3.0);
float sb = W.N3(P * float3(0.11, 2.6, 2.6) + Seed * 5.0);
float lines = saturate((max(sa, sb) - 0.86) * 10.0) * Scratches;
float dmgN = W.Fbm(P * 0.45 + Seed * 7.0);
float dmg = saturate((dmgN - (1.0 - Damage * 0.45)) * 7.0) * step(0.01, Damage);
float3 col = float3(0.62, 0.52, 0.26);
float a = Yellowing * 0.55;
col = lerp(col, float3(0.12, 0.095, 0.07), saturate(grime / max(a + grime, 0.001)));
a = max(a, grime * 0.5);
col = lerp(col, float3(0.08, 0.06, 0.04), spots);
a = max(a, spots * 0.7);
col = lerp(col, float3(0.8, 0.8, 0.78), lines);
a = max(a, lines * 0.6);
col = lerp(col, float3(0.03, 0.025, 0.02), dmg);
a = max(a, dmg * 0.9);return float4(col, a);
"""


def scalar(mat, name, value, x, y):
    s = mel.create_material_expression(mat, unreal.MaterialExpressionScalarParameter, x, y)
    s.set_editor_property("parameter_name", name)
    s.set_editor_property("default_value", value)
    return s


def main():
    path = f"{FOLDER}/{NAME}"
    if eal.does_asset_exist(path):
        eal.delete_asset(path)
    mat = tools.create_asset(NAME, FOLDER, unreal.Material, unreal.MaterialFactoryNew())
    mat.set_editor_property("blend_mode", unreal.BlendMode.BLEND_TRANSLUCENT)
    mat.set_editor_property("translucency_lighting_mode", unreal.TranslucencyLightingMode.TLM_VOLUMETRIC_NON_DIRECTIONAL)
    custom = mel.create_material_expression(mat, unreal.MaterialExpressionCustom, -300, 0)
    custom.set_editor_property("code", CODE)
    custom.set_editor_property("output_type", unreal.CustomMaterialOutputType.CMOT_FLOAT4)
    inputs = []
    for n in ("Pos", "Nrm", "Dirt", "Scratches", "Yellowing", "Damage", "Seed"):
        ci = unreal.CustomInput()
        ci.set_editor_property("input_name", n)
        inputs.append(ci)
    custom.set_editor_property("inputs", inputs)
    try:
        pos = mel.create_material_expression(mat, unreal.MaterialExpressionLocalPosition, -700, -200)
    except Exception:
        pos = mel.create_material_expression(mat, unreal.MaterialExpressionWorldPosition, -700, -200)
    nrm = mel.create_material_expression(mat, unreal.MaterialExpressionVertexNormalWS, -700, -100)
    mel.connect_material_expressions(pos, "", custom, "Pos")
    mel.connect_material_expressions(nrm, "", custom, "Nrm")
    for i, n in enumerate(("Dirt", "Scratches", "Yellowing", "Damage", "Seed")):
        mel.connect_material_expressions(scalar(mat, n, 0.0, -700, i * 90), "", custom, n)
    rgb = mel.create_material_expression(mat, unreal.MaterialExpressionComponentMask, 0, -60)
    rgb.set_editor_property("r", True)
    rgb.set_editor_property("g", True)
    rgb.set_editor_property("b", True)
    alpha = mel.create_material_expression(mat, unreal.MaterialExpressionComponentMask, 0, 80)
    alpha.set_editor_property("a", True)
    mel.connect_material_expressions(custom, "", rgb, "")
    mel.connect_material_expressions(custom, "", alpha, "")
    mel.connect_material_property(rgb, "", unreal.MaterialProperty.MP_BASE_COLOR)
    mel.connect_material_property(alpha, "", unreal.MaterialProperty.MP_OPACITY)
    rough = mel.create_material_expression(mat, unreal.MaterialExpressionConstant, 0, 200)
    rough.set_editor_property("r", 0.85)
    mel.connect_material_property(rough, "", unreal.MaterialProperty.MP_ROUGHNESS)
    mel.recompile_material(mat)
    eal.save_asset(path)
    unreal.log(f"WE_WEAR_MATERIAL {path}")
    unreal.SystemLibrary.quit_editor()


main()
