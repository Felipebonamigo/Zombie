"""
Importa os sons de Audio/*.wav para /Game/Zombie/Sons/SW_<nome>, por script, sem abrir o editor. O jogo carrega por esse
caminho (Source/Zombie/Audio/ZMBAudioSubsystem.cpp); sem a importação, o jogo roda mudo e avisa no log.

Os sons com laco = 1 em Data/Sons.csv (o motor do carro) saem marcados para tocar em laço.

Rodar SEM janela, pelo commandlet de Python do Unreal:

    UnrealEditor-Cmd.exe Zombie.uproject -run=PythonScript -Script="<caminho>/importar_sons.py" -unattended -nullrhi -nosplash

Existe um invólucro em PowerShell: Tools/Unreal/importar_sons.ps1.

LIMITE DE SEGURANÇA: só grava em /Game/Zombie/Sons. Rodar de novo reimporta por cima (troca o som pelo .wav atual).
"""

import sys

# O Python embutido da engine gravaria .pyc dentro da instalação do Unreal. Desliga ANTES de qualquer outro import.
sys.dont_write_bytecode = True

import csv
import os

import unreal

DEST = "/Game/Zombie/Sons"


def project_path(*parts):
    root = unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir())
    return os.path.join(root, *parts)


def loop_files():
    """Nomes (sem extensão) dos arquivos dos sons em laço, pela coluna laco de Data/Sons.csv."""
    names = set()
    with open(project_path("Data", "Sons.csv"), encoding="utf-8-sig", newline="") as table:
        for row in csv.DictReader(table):
            if row.get("laco", "").strip() in ("1", "sim"):
                names.update(name.strip() for name in row.get("arquivos", "").split(";") if name.strip())
    return names


def main():
    audio = project_path("Audio")
    names = sorted(name[:-4] for name in os.listdir(audio) if name.lower().endswith(".wav"))
    tasks = []
    for name in names:
        task = unreal.AssetImportTask()
        task.set_editor_property("filename", os.path.join(audio, name + ".wav"))
        task.set_editor_property("destination_path", DEST)
        task.set_editor_property("destination_name", "SW_" + name)
        task.set_editor_property("replace_existing", True)
        task.set_editor_property("automated", True)
        task.set_editor_property("save", True)
        tasks.append(task)
    unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks(tasks)

    imported = sum(1 for task in tasks if task.get_editor_property("imported_object_paths"))
    loops = 0
    for name in loop_files():
        path = f"{DEST}/SW_{name}"
        wave = unreal.EditorAssetLibrary.load_asset(path)
        if wave:
            wave.set_editor_property("looping", True)
            unreal.EditorAssetLibrary.save_asset(path, only_if_is_dirty=False)
            loops += 1
    if imported != len(tasks):
        raise RuntimeError(f"importar_sons FALHOU: {imported} de {len(tasks)} .wav importados (veja o log acima)")
    # a linha que o importar_sons.ps1 procura: só sai com tudo importado
    unreal.log(f"importar_sons OK: {imported} sons importados em {DEST} ({loops} em laço)")


main()
