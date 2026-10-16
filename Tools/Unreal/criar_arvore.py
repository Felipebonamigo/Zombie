# Árvore folhuda para as praças: importa a island_tree_02 do Poly Haven (CC0), cria os materiais M_ZMBFolha (recortado pelo alfa,
# dupla face, luz atravessando a folha) e M_ZMBTronco (opaco), liga os dois e gera LODs sem Nanite.
# Decisões herdadas do Age of Earth (u1_vegetal.py e u2_arvore.py): folhas com Nanite e sombras virtuais travaram a GPU
# (DXGI_ERROR_DEVICE_HUNG), então a malha vira comum com 4 LODs; sem vento por vértice pelo mesmo motivo; as folhas em cartões
# finos colapsam abaixo de ~10 % dos triângulos.
# A malha vai para /Game/Externo/PolyHaven/ (ignorado pelo git: ~40 MB de glTF); só os dois materiais, que são nossos, entram no git.
# Origem do glTF: variável ZMB_ARVORE_GLTF (padrão: a cópia que o Age of Earth já baixou, no PC do Felipe) ou baixe de
# https://polyhaven.com/a/island_tree_02 (glTF 1k).
# Uso: Tools\Unreal\criar_arvore.ps1 (com o editor fechado).
import os

import unreal

mel = unreal.MaterialEditingLibrary
tools = unreal.AssetToolsHelpers.get_asset_tools()
reg = unreal.AssetRegistryHelpers.get_asset_registry()

PASTA_MATERIAIS = "/Game/Zombie/Materials"
DESTINO = "/Game/Externo/PolyHaven/island_tree_02"
BASE = DESTINO + "/island_tree_02"
GLTF = os.environ.get(
    "ZMB_ARVORE_GLTF",
    "C:/Users/Felip/source/repos/AgeOfEarth/unreal/AgeOfEarthUE/Content/Fontes/vegetacao/island_tree_02/island_tree_02.gltf",
)


def no(mat, cls, x, y):
    return mel.create_material_expression(mat, cls, x, y)


def param_tex(mat, nome, padrao, tipo, x, y):
    n = no(mat, unreal.MaterialExpressionTextureSampleParameter2D, x, y)
    n.set_editor_property("parameter_name", nome)
    n.set_editor_property("texture", unreal.load_asset(padrao))
    n.set_editor_property("sampler_type", tipo)
    return n


def mult(mat, a, b, x, y, ao="", bo=""):
    n = no(mat, unreal.MaterialExpressionMultiply, x, y)
    mel.connect_material_expressions(a, ao, n, "A")
    mel.connect_material_expressions(b, bo, n, "B")
    return n


def mestre(nome, folha):
    caminho = PASTA_MATERIAIS + "/" + nome
    if unreal.EditorAssetLibrary.does_asset_exist(caminho):
        unreal.EditorAssetLibrary.delete_asset(caminho)
    mat = tools.create_asset(nome, PASTA_MATERIAIS, unreal.Material, unreal.MaterialFactoryNew())
    mat.set_editor_property("used_with_instanced_static_meshes", True)
    mat.set_editor_property("two_sided", True)
    if folha:
        mat.set_editor_property("shading_model", unreal.MaterialShadingModel.MSM_TWO_SIDED_FOLIAGE)
        mat.set_editor_property("blend_mode", unreal.BlendMode.BLEND_MASKED)
        mat.set_editor_property("opacity_mask_clip_value", 0.45)
    st = unreal.MaterialSamplerType
    branco = "/Engine/EngineResources/WhiteSquareTexture"
    cor = param_tex(mat, "BaseColorTexture", branco, st.SAMPLERTYPE_COLOR, -900, 0)
    nor = param_tex(mat, "NormalTexture", "/Engine/EngineMaterials/DefaultNormal", st.SAMPLERTYPE_NORMAL, -900, 300)
    mr = param_tex(mat, "MetallicRoughnessTexture", branco, st.SAMPLERTYPE_LINEAR_COLOR, -900, 600)
    # folhas de foto ficam escuras à luz do jogo (manchas pretas de longe): clareia e puxa para o verde-oliva
    brilho = no(mat, unreal.MaterialExpressionVectorParameter, -700, -100)
    brilho.set_editor_property("parameter_name", "Brilho")
    brilho.set_editor_property("default_value", unreal.LinearColor(1.45, 1.5, 1.2, 1.0) if folha else unreal.LinearColor(1.0, 1.0, 1.0, 1.0))
    mel.connect_material_property(mult(mat, cor, brilho, -450, -50, "RGB", "RGB"), "", unreal.MaterialProperty.MP_BASE_COLOR)
    if folha:
        mel.connect_material_property(cor, "A", unreal.MaterialProperty.MP_OPACITY_MASK)
        meio = no(mat, unreal.MaterialExpressionConstant, -500, -150)
        meio.set_editor_property("r", 0.55)
        mel.connect_material_property(mult(mat, cor, meio, -300, -150, "RGB", ""), "", unreal.MaterialProperty.MP_SUBSURFACE_COLOR)
    mel.connect_material_property(nor, "RGB", unreal.MaterialProperty.MP_NORMAL)
    rug = no(mat, unreal.MaterialExpressionClamp, -500, 600)
    rug.set_editor_property("min_default", 0.45)
    rug.set_editor_property("max_default", 1.0)
    mel.connect_material_expressions(mr, "G", rug, "")
    mel.connect_material_property(rug, "", unreal.MaterialProperty.MP_ROUGHNESS)
    mel.recompile_material(mat)
    unreal.EditorAssetLibrary.save_loaded_asset(mat)
    return mat


def importar():
    if not os.path.exists(GLTF):
        raise RuntimeError("glTF não encontrado: %s (baixe de polyhaven.com/a/island_tree_02 e passe em ZMB_ARVORE_GLTF)" % GLTF)
    t = unreal.AssetImportTask()
    t.set_editor_property("filename", GLTF)
    t.set_editor_property("destination_path", DESTINO)
    t.set_editor_property("automated", True)
    t.set_editor_property("replace_existing", True)
    t.set_editor_property("save", True)
    tools.import_asset_tasks([t])
    reg.search_all_assets(True)


def ligar(folha, tronco):
    for a in reg.get_assets_by_path(BASE + "/Materials", recursive=False):
        mi = unreal.load_asset(str(a.package_name))
        eh_folha = "leaves" in str(a.asset_name)
        mi.set_editor_property("parent", folha if eh_folha else tronco)
        for tp in mi.get_editor_property("texture_parameter_values"):
            tex = tp.parameter_value
            if not tex:
                continue
            n = str(tp.parameter_info.name)
            if n == "NormalTexture":
                tex.set_editor_property("compression_settings", unreal.TextureCompressionSettings.TC_NORMALMAP)
                tex.set_editor_property("srgb", False)
            elif n == "MetallicRoughnessTexture":
                tex.set_editor_property("srgb", False)
            unreal.EditorAssetLibrary.save_loaded_asset(tex)
        unreal.EditorAssetLibrary.save_loaded_asset(mi)
        unreal.log("ARVORE %s -> %s" % (a.asset_name, "M_ZMBFolha" if eh_folha else "M_ZMBTronco"))


def lods():
    for a in reg.get_assets_by_path(BASE + "/StaticMeshes", recursive=False):
        m = unreal.load_asset(str(a.package_name))
        ns = m.get_editor_property("nanite_settings")
        ns.set_editor_property("enabled", False)
        m.set_editor_property("nanite_settings", ns)
        sub = unreal.get_editor_subsystem(unreal.StaticMeshEditorSubsystem)
        opc = unreal.StaticMeshReductionOptions()
        opc.set_editor_property("auto_compute_lod_screen_size", False)
        niveis = []
        for tela, frac in ((1.0, 0.22), (0.30, 0.09), (0.12, 0.045), (0.05, 0.022)):
            r = unreal.StaticMeshReductionSettings()
            r.set_editor_property("percent_triangles", frac)
            r.set_editor_property("screen_size", tela)
            niveis.append(r)
        opc.set_editor_property("reduction_settings", niveis)
        n = sub.set_lods(m, opc)
        unreal.EditorAssetLibrary.save_loaded_asset(m)
        unreal.log("ARVORE %s sem Nanite, %d LODs, vértices por LOD %s" % (a.asset_name, n, [sub.get_number_verts(m, i) for i in range(sub.get_lod_count(m))]))


def main():
    folha = mestre("M_ZMBFolha", True)
    tronco = mestre("M_ZMBTronco", False)
    importar()
    ligar(folha, tronco)
    lods()
    unreal.log("criar_arvore OK")


main()
