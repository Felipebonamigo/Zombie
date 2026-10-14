"""
Cria os dois materiais-pai do protótipo, por script, sem abrir o editor:

    /Game/Zombie/Materials/M_ZMBSurface   cor lisa com aspereza e metal ajustáveis (Color, Roughness, Metallic)
    /Game/Zombie/Materials/M_ZMBEmissive  cor que brilha, sem luz (Color, Intensity): bola de fogo, janela acesa, rastro de tiro

O jogo (Game/ZMBGameData) procura os dois; sem eles, usa o BasicShapeMaterial da engine, que não tem aspereza nem brilho.

Rodar SEM janela, pelo commandlet de Python do Unreal (o mesmo jeito do create_dent_material.py do Detonauto):

    UnrealEditor-Cmd.exe Zombie.uproject -run=PythonScript -Script="<caminho>/criar_materiais.py" -unattended -nullrhi -nosplash -nosound

Existe um invólucro em PowerShell: Tools/Unreal/criar_materiais.ps1.

LIMITE DE SEGURANÇA: só grava em /Game/Zombie/Materials. Rodar de novo refaz os dois materiais no mesmo pacote.
"""

import sys

# O Python embutido da engine gravaria .pyc dentro da instalação do Unreal. Desliga ANTES de qualquer outro import.
sys.dont_write_bytecode = True

import unreal

DEST = "/Game/Zombie/Materials"


def recreate(name):
    path = f"{DEST}/{name}"
    if unreal.EditorAssetLibrary.does_asset_exist(path):
        unreal.EditorAssetLibrary.delete_asset(path)
    tools = unreal.AssetToolsHelpers.get_asset_tools()
    return tools.create_asset(name, DEST, unreal.Material, unreal.MaterialFactoryNew())


def vector_param(material, name, default, x, y):
    lib = unreal.MaterialEditingLibrary
    node = lib.create_material_expression(material, unreal.MaterialExpressionVectorParameter, x, y)
    node.set_editor_property("parameter_name", name)
    node.set_editor_property("default_value", default)
    return node


def scalar_param(material, name, default, x, y):
    lib = unreal.MaterialEditingLibrary
    node = lib.create_material_expression(material, unreal.MaterialExpressionScalarParameter, x, y)
    node.set_editor_property("parameter_name", name)
    node.set_editor_property("default_value", default)
    return node


def build_surface():
    lib = unreal.MaterialEditingLibrary
    material = recreate("M_ZMBSurface")
    color = vector_param(material, "Color", unreal.LinearColor(0.5, 0.5, 0.5, 1.0), -500, -100)
    roughness = scalar_param(material, "Roughness", 0.7, -500, 100)
    metallic = scalar_param(material, "Metallic", 0.0, -500, 250)
    lib.connect_material_property(color, "", unreal.MaterialProperty.MP_BASE_COLOR)
    lib.connect_material_property(roughness, "", unreal.MaterialProperty.MP_ROUGHNESS)
    lib.connect_material_property(metallic, "", unreal.MaterialProperty.MP_METALLIC)
    # usado em componentes instanciados (a cidade) e em malhas com Nanite no futuro
    material.set_editor_property("used_with_instanced_static_meshes", True)
    material.set_editor_property("used_with_nanite", True)
    lib.recompile_material(material)
    unreal.EditorAssetLibrary.save_loaded_asset(material)
    return material


def build_emissive():
    lib = unreal.MaterialEditingLibrary
    material = recreate("M_ZMBEmissive")
    material.set_editor_property("shading_model", unreal.MaterialShadingModel.MSM_UNLIT)
    color = vector_param(material, "Color", unreal.LinearColor(1.0, 0.45, 0.1, 1.0), -500, 0)
    intensity = scalar_param(material, "Intensity", 20.0, -500, 200)
    multiply = lib.create_material_expression(material, unreal.MaterialExpressionMultiply, -250, 100)
    lib.connect_material_expressions(color, "", multiply, "A")
    lib.connect_material_expressions(intensity, "", multiply, "B")
    lib.connect_material_property(multiply, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)
    material.set_editor_property("used_with_instanced_static_meshes", True)
    lib.recompile_material(material)
    unreal.EditorAssetLibrary.save_loaded_asset(material)
    return material


def main():
    if not DEST.startswith("/Game/Zombie/"):
        raise RuntimeError(f"destino recusado: {DEST}")
    unreal.EditorAssetLibrary.make_directory(DEST)
    surface = build_surface()
    emissive = build_emissive()
    unreal.log(f"criar_materiais: {surface.get_path_name()} e {emissive.get_path_name()} criados")


main()
