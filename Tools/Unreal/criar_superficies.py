# Superfícies reais da cidade: asfalto, calçada e fachada de reboco com textura PBR do Poly Haven (CC0), em vez de cor lisa.
# Adaptado de Detonauto/Tools/Unreal/create_lookdev.py (build_surface_master): cor, normal e ARM projetados pelo mundo, com o
# tamanho real da textura em cm (Tile) e um tom (Tint), então os blocos esticados não esticam a textura.
#
# O que entra no git: só os materiais-pai /Game/Zombie/Materials/M_ZMBSuperficie (sem referência às texturas) e M_ZMBJanela (vidro com salas falsas, sem textura).
# O que fica fora (/Game/Externo/PolyHaven/Superficies, ignorado pelo git, ~30 MB de JPG 2k viram dezenas de MB de .uasset):
# as 12 texturas e as 3 instâncias. Sem elas, o jogo volta à cor lisa (UZMBGameData::SurfaceMaterial).
#
# Origem dos JPG (CC0): a pasta Sandbox/texturas_cc0 do Detonauto (variável ZMB_TEXTURAS para outra) ou polyhaven.com
# (asphalt_02, concrete_pavement, clay_plaster, brushed_concrete: diff, nor_dx e arm, 2k, JPG).
# Atalho deliberado (herdado do Detonauto): as normais das faces verticais ficam na base do tangente da malha, não da projeção.
# Uso: Tools\Unreal\criar_superficies.ps1 (com o editor fechado).
import os

import unreal

mel = unreal.MaterialEditingLibrary
tools = unreal.AssetToolsHelpers.get_asset_tools()

MASTER_DIR = "/Game/Zombie/Materials"
EXT_DIR = "/Game/Externo/PolyHaven/Superficies"
TEX_DIR = EXT_DIR + "/Textures"
MAT_DIR = EXT_DIR + "/Materials"
SOURCE = os.environ.get("ZMB_TEXTURAS", "C:/Users/Felip/source/repos/Detonauto/Detonauto/Sandbox/texturas_cc0")

# (nome no jogo, id do Poly Haven, lado real da textura em cm)
SETS = [
    ("Asfalto", "asphalt_02", 200.0),
    ("Calcada", "concrete_pavement", 180.0),
    ("Fachada", "clay_plaster", 200.0),
    ("Concreto", "brushed_concrete", 250.0),
]


def expr(m, cls, x, y):
    return mel.create_material_expression(m, cls, x, y)


def connect(a, a_out, b, b_in):
    if not mel.connect_material_expressions(a, a_out, b, b_in):
        raise RuntimeError("Ligação de material falhou: %s.%s -> %s.%s" % (a.get_name(), a_out, b.get_name(), b_in))


def mask(m, source, channels, x, y):
    c = expr(m, unreal.MaterialExpressionComponentMask, x, y)
    for ch in "rgba":
        c.set_editor_property(ch, ch in channels)
    connect(source, "", c, "")
    return c


def recreate(path, asset_class, factory):
    """Cria o asset ou reaproveita o pacote: material-pai perde os nós e é refeito; nada é apagado do disco."""
    if unreal.EditorAssetLibrary.does_asset_exist(path):
        asset = unreal.load_asset(path)
        if isinstance(asset, unreal.Material):
            mel.delete_all_material_expressions(asset)
        return asset
    folder, name = path.rsplit("/", 1)
    asset = tools.create_asset(name, folder, asset_class, factory)
    if asset is None:
        raise RuntimeError("Não consegui criar " + path)
    return asset


def importar_texturas():
    tasks, wanted = [], []
    for nome, ph, _ in SETS:
        for sufixo, tipo in (("diff", "D"), ("nor_dx", "N"), ("arm", "ARM")):
            caminho = os.path.join(SOURCE, ph, "%s_%s_2k.jpg" % (ph, sufixo))
            if not os.path.isfile(caminho):
                raise RuntimeError("Textura não encontrada: " + caminho)
            t = unreal.AssetImportTask()
            t.filename = caminho
            t.destination_path = TEX_DIR
            t.destination_name = "T_%s_%s" % (nome, tipo)
            t.automated = True
            t.replace_existing = True
            t.save = False
            tasks.append(t)
            wanted.append((TEX_DIR + "/T_%s_%s" % (nome, tipo), tipo))
    tools.import_asset_tasks(tasks)
    out = {}
    for caminho, tipo in wanted:
        tex = unreal.load_asset(caminho)
        if not isinstance(tex, unreal.Texture2D):
            raise RuntimeError("A importação não gerou " + caminho)
        if tipo == "N":
            tex.set_editor_property("compression_settings", unreal.TextureCompressionSettings.TC_NORMALMAP)
            tex.set_editor_property("srgb", False)
        elif tipo == "ARM":
            tex.set_editor_property("compression_settings", unreal.TextureCompressionSettings.TC_MASKS)
            tex.set_editor_property("srgb", False)
        unreal.EditorAssetLibrary.save_asset(caminho)
        out[caminho] = tex
    return out


def mestre():
    path = MASTER_DIR + "/M_ZMBSuperficie"
    m = recreate(path, unreal.Material, unreal.MaterialFactoryNew())
    m.set_editor_property("used_with_instanced_static_meshes", True)

    world = expr(m, unreal.MaterialExpressionWorldPosition, -1800, 0)
    normal = expr(m, unreal.MaterialExpressionVertexNormalWS, -1800, 300)
    absn = expr(m, unreal.MaterialExpressionAbs, -1600, 300)
    connect(normal, "", absn, "")
    nx, ny, nz = (mask(m, absn, c, -1400, 250 + i * 80) for i, c in enumerate("rgb"))
    xy, yz, xz = (mask(m, world, c, -1400, -150 + i * 80) for i, c in enumerate(("rg", "gb", "rb")))
    # parede: normal mais em X usa o plano YZ; mais em Y usa XZ. Chão: normal mais em Z que 0,5 usa o plano XY
    parede = expr(m, unreal.MaterialExpressionIf, -1150, 100)
    connect(nx, "", parede, "A")
    connect(ny, "", parede, "B")
    connect(yz, "", parede, "A > B")
    connect(yz, "", parede, "A == B")
    connect(xz, "", parede, "A < B")
    meio = expr(m, unreal.MaterialExpressionConstant, -1150, 350)
    meio.set_editor_property("r", 0.5)
    uv = expr(m, unreal.MaterialExpressionIf, -950, 0)
    connect(nz, "", uv, "A")
    connect(meio, "", uv, "B")
    connect(xy, "", uv, "A > B")
    connect(xy, "", uv, "A == B")
    connect(parede, "", uv, "A < B")
    tile = expr(m, unreal.MaterialExpressionScalarParameter, -950, 250)
    tile.set_editor_property("parameter_name", "Tile")
    tile.set_editor_property("default_value", 300.0)
    escala = expr(m, unreal.MaterialExpressionDivide, -750, 50)
    connect(uv, "", escala, "A")
    connect(tile, "", escala, "B")

    def amostra(nome, padrao, tipo, y):
        s = expr(m, unreal.MaterialExpressionTextureSampleParameter2D, -500, y)
        s.set_editor_property("parameter_name", nome)
        s.set_editor_property("texture", unreal.load_asset(padrao))
        s.set_editor_property("sampler_type", tipo)
        connect(escala, "", s, "UVs")
        return s

    st = unreal.MaterialSamplerType
    cor = amostra("Cor", "/Engine/EngineResources/WhiteSquareTexture", st.SAMPLERTYPE_COLOR, -300)
    nor = amostra("Normal", "/Engine/EngineMaterials/DefaultNormal", st.SAMPLERTYPE_NORMAL, 0)
    arm = amostra("ARM", "/Engine/EngineResources/WhiteSquareTexture", st.SAMPLERTYPE_MASKS, 300)
    tom = expr(m, unreal.MaterialExpressionVectorParameter, -500, -500)
    tom.set_editor_property("parameter_name", "Tint")
    tom.set_editor_property("default_value", unreal.LinearColor(1.0, 1.0, 1.0, 1.0))
    tingida = expr(m, unreal.MaterialExpressionMultiply, -200, -350)
    connect(cor, "RGB", tingida, "A")
    connect(tom, "", tingida, "B")
    mel.connect_material_property(tingida, "", unreal.MaterialProperty.MP_BASE_COLOR)
    mel.connect_material_property(nor, "RGB", unreal.MaterialProperty.MP_NORMAL)
    mel.connect_material_property(arm, "R", unreal.MaterialProperty.MP_AMBIENT_OCCLUSION)
    mel.connect_material_property(arm, "G", unreal.MaterialProperty.MP_ROUGHNESS)
    mel.connect_material_property(arm, "B", unreal.MaterialProperty.MP_METALLIC)
    mel.recompile_material(m)
    unreal.EditorAssetLibrary.save_asset(path)
    return m


def instancia(pai, nome, tile, tex):
    path = MAT_DIR + "/" + nome
    mi = recreate(path, unreal.MaterialInstanceConstant, unreal.MaterialInstanceConstantFactoryNew())
    mel.set_material_instance_parent(mi, pai)
    mel.set_material_instance_scalar_parameter_value(mi, "Tile", tile)
    for k, v in tex.items():
        mel.set_material_instance_texture_parameter_value(mi, k, v)
    mel.update_material_instance(mi)
    unreal.EditorAssetLibrary.save_asset(path)


# Interior mapping (adaptado de Detonauto/Tools/Unreal/create_lookdev.py): atrás do vidro, uma sala falsa por célula da grade do
# mundo (WP - Off) / Room, com chão, teto e paredes, acesa ou apagada por sorteio da célula; caixilho de alumínio a cada Pane cm.
INTERIOR_HLSL = """
float3 rs = max(Room, 1.0);
float3 n = normalize(N);
float3 p = (WP - n * 2.0 - Off) / rs;
float3 cell = floor(p);
float3 l = p - cell;
float3 d = -V / rs;
d = sign(d) * max(abs(d), 1e-5);
float3 t = (step(0.0, d) - l) / d;
float tm = min(min(t.x, t.y), t.z);
float r = frac(sin(dot(cell, float3(12.9898, 78.233, 37.719))) * 43758.5453);
float lit = r > 0.5 ? 1.0 : 0.12;
float3 wall = lerp(float3(0.42, 0.36, 0.28), float3(0.3, 0.33, 0.38), frac(r * 7.13));
float3 col;
if (tm == t.z)
{
    col = d.z < 0.0 ? float3(0.28, 0.19, 0.12) : float3(0.8, 0.79, 0.75);
}
else if ((tm == t.x && abs(n.x) > 0.5) || (tm == t.y && abs(n.y) > 0.5))
{
    col = wall * 0.75;
}
else
{
    col = wall;
}
float depth = saturate(tm * length(d * rs) / 700.0);
col *= lerp(1.0, 0.45, depth) * lit;
float u = abs(n.x) > 0.5 ? WP.y : WP.x;
float m = frac(u / max(Pane, 1.0));
float frame = (m < 0.025 || m > 0.975) ? 1.0 : 0.0;
return float4(col, frame);
"""


def mestre_janela():
    path = MASTER_DIR + "/M_ZMBJanela"
    m = recreate(path, unreal.Material, unreal.MaterialFactoryNew())
    m.set_editor_property("used_with_instanced_static_meshes", True)

    def param(cls, nome, valor, x, y):
        p = expr(m, cls, x, y)
        p.set_editor_property("parameter_name", nome)
        p.set_editor_property("default_value", valor)
        return p

    world = expr(m, unreal.MaterialExpressionWorldPosition, -1400, -200)
    cam = expr(m, unreal.MaterialExpressionCameraVectorWS, -1400, -100)
    normal = expr(m, unreal.MaterialExpressionVertexNormalWS, -1400, 0)
    # pé-direito dos andares do jogo: 320 cm, a partir do topo da calçada (15 cm)
    sala = param(unreal.MaterialExpressionVectorParameter, "RoomSizeCm", unreal.LinearColor(450.0, 450.0, 320.0, 0.0), -1400, 100)
    desloc = param(unreal.MaterialExpressionVectorParameter, "RoomOffsetCm", unreal.LinearColor(0.0, 0.0, 15.0, 0.0), -1400, 250)
    vao = param(unreal.MaterialExpressionScalarParameter, "PaneWidthCm", 150.0, -1400, 400)

    custom = expr(m, unreal.MaterialExpressionCustom, -1000, 0)
    custom.set_editor_property("code", INTERIOR_HLSL)
    custom.set_editor_property("description", "Salas falsas (interior mapping)")
    custom.set_editor_property("output_type", unreal.CustomMaterialOutputType.CMOT_FLOAT4)
    entradas = []
    for nome in ("WP", "V", "N", "Room", "Off", "Pane"):
        ci = unreal.CustomInput()
        ci.set_editor_property("input_name", nome)
        entradas.append(ci)
    custom.set_editor_property("inputs", entradas)
    for src, pino in ((world, "WP"), (cam, "V"), (normal, "N"), (sala, "Room"), (desloc, "Off"), (vao, "Pane")):
        connect(src, "", custom, pino)

    interior = mask(m, custom, "rgb", -750, -100)
    caixilho = mask(m, custom, "a", -750, 100)
    vidro = param(unreal.MaterialExpressionVectorParameter, "CorVidro", unreal.LinearColor(0.01, 0.012, 0.015, 1.0), -750, -400)
    cor_cx = param(unreal.MaterialExpressionVectorParameter, "CorCaixilho", unreal.LinearColor(0.09, 0.09, 0.095, 1.0), -750, -300)
    base = expr(m, unreal.MaterialExpressionLinearInterpolate, -450, -350)
    connect(vidro, "", base, "A")
    connect(cor_cx, "", base, "B")
    connect(caixilho, "", base, "Alpha")
    rug = expr(m, unreal.MaterialExpressionLinearInterpolate, -450, -200)
    rug.set_editor_property("const_a", 0.04)
    rug.set_editor_property("const_b", 0.45)
    connect(caixilho, "", rug, "Alpha")
    met = expr(m, unreal.MaterialExpressionLinearInterpolate, -450, -50)
    met.set_editor_property("const_a", 0.0)
    met.set_editor_property("const_b", 0.8)
    connect(caixilho, "", met, "Alpha")
    # sala acesa: emissivo atrás do vidro, mais fraco no ângulo rasante (onde o vidro só reflete); BrilhoInterior segue a hora (unidades do jogo: as janelas acesas do protótipo usavam 6 à noite; o Detonauto usa nits, 1500, com exposição física)
    fresnel = expr(m, unreal.MaterialExpressionFresnel, -750, 300)
    ve = expr(m, unreal.MaterialExpressionOneMinus, -600, 300)
    connect(fresnel, "", ve, "")
    sem_cx = expr(m, unreal.MaterialExpressionOneMinus, -600, 150)
    connect(caixilho, "", sem_cx, "")
    brilho = param(unreal.MaterialExpressionScalarParameter, "BrilhoInterior", 0.25, -600, 450)
    e1 = expr(m, unreal.MaterialExpressionMultiply, -400, 100)
    connect(interior, "", e1, "A")
    connect(sem_cx, "", e1, "B")
    e2 = expr(m, unreal.MaterialExpressionMultiply, -250, 200)
    connect(e1, "", e2, "A")
    connect(ve, "", e2, "B")
    e3 = expr(m, unreal.MaterialExpressionMultiply, -100, 250)
    connect(e2, "", e3, "A")
    connect(brilho, "", e3, "B")
    mel.connect_material_property(base, "", unreal.MaterialProperty.MP_BASE_COLOR)
    mel.connect_material_property(rug, "", unreal.MaterialProperty.MP_ROUGHNESS)
    mel.connect_material_property(met, "", unreal.MaterialProperty.MP_METALLIC)
    mel.connect_material_property(e3, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)
    mel.recompile_material(m)
    unreal.EditorAssetLibrary.save_asset(path)
    return m


def main():
    texturas = importar_texturas()
    pai = mestre()
    mestre_janela()
    for nome, _, tile in SETS:
        instancia(pai, "MI_" + nome, tile, {
            "Cor": texturas[TEX_DIR + "/T_%s_D" % nome],
            "Normal": texturas[TEX_DIR + "/T_%s_N" % nome],
            "ARM": texturas[TEX_DIR + "/T_%s_ARM" % nome],
        })
    unreal.log("criar_superficies OK: %d texturas, %d instâncias, janela" % (len(texturas), len(SETS)))


main()
