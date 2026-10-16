# Superfícies reais da cidade: asfalto, calçada e fachada de reboco com textura PBR do Poly Haven (CC0), em vez de cor lisa.
# Adaptado de Detonauto/Tools/Unreal/create_lookdev.py (build_surface_master): cor, normal e ARM projetados pelo mundo, com o
# tamanho real da textura em cm (Tile) e um tom (Tint), então os blocos esticados não esticam a textura.
#
# O que entra no git: só o material-pai /Game/Zombie/Materials/M_ZMBSuperficie (sem referência às texturas).
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


def main():
    texturas = importar_texturas()
    pai = mestre()
    for nome, _, tile in SETS:
        instancia(pai, "MI_" + nome, tile, {
            "Cor": texturas[TEX_DIR + "/T_%s_D" % nome],
            "Normal": texturas[TEX_DIR + "/T_%s_N" % nome],
            "ARM": texturas[TEX_DIR + "/T_%s_ARM" % nome],
        })
    unreal.log("criar_superficies OK: %d texturas, %d instâncias" % (len(texturas), len(SETS)))


main()
