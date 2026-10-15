#!/usr/bin/env python3
"""Confere o C++ do Unreal sem compilar (a nuvem não tem o motor).

Pega os erros mecânicos que mais quebram o build no PC: .generated.h fora do lugar, GENERATED_BODY
faltando, ponteiro de UObject sem UPROPERTY (o coletor de lixo apaga), macro *_API errada, método
definido no .cpp sem declaração no .h, include do projeto que não existe, módulo do motor que falta
no *.Build.cs e sombra de nome nos métodos (C4456, C4457 e C4458, que o Unreal trata como erro).
Não substitui a compilação: no fim diz se o PC precisa fechar o editor ou se o Live Coding basta.
Teste do próprio conferidor: teste_conferir.py, na mesma pasta.

Uso (na raiz do repositório):
  python3 .claude/skills/revisar-sem-compilar/conferir_cpp.py            # o que mudou desde o HEAD
  python3 .claude/skills/revisar-sem-compilar/conferir_cpp.py --base origin/main
  python3 .claude/skills/revisar-sem-compilar/conferir_cpp.py --todos    # todo o C++ do projeto
Sai com 1 se achou ERRO, 0 se só avisos ou nada.
"""
import argparse
import os
import re
import subprocess
import sys

IGNORAR_PASTAS = {'Intermediate', 'Binaries', 'Saved', 'DerivedDataCache', '.git', 'node_modules', 'ThirdParty'}

# Cabeçalho do motor -> módulo que precisa estar no *.Build.cs. Só os mais comuns; o resto o PC acusa.
MODULO_DO_INCLUDE = [
    (r'^EnhancedInput|^InputAction\.h|^InputMappingContext\.h|^InputActionValue\.h', 'EnhancedInput'),
    (r'^Niagara', 'Niagara'),
    (r'^Blueprint/UserWidget\.h|^Components/(TextBlock|Image|Button|ProgressBar|CanvasPanel)', 'UMG'),
    (r'^Widgets/|^Framework/', 'Slate'),
    (r'^Styling/', 'SlateCore'),
    (r'^ChaosVehicle|^ChaosWheeledVehicle|^WheeledVehiclePawn\.h', 'ChaosVehicles'),
    (r'^GeometryCollection/', 'GeometryCollectionEngine'),
    (r'^Field/', 'FieldSystemEngine'),
    (r'^Dom/Json|^Serialization/Json', 'Json'),
    (r'^JsonObjectConverter\.h', 'JsonUtilities'),
    (r'^Http|^Interfaces/IHttp', 'HTTP'),
    (r'^AIController\.h|^BehaviorTree/|^Perception/', 'AIModule'),
    (r'^NavigationSystem\.h|^NavMesh/', 'NavigationSystem'),
    (r'^OnlineSubsystem', 'OnlineSubsystem'),
    (r'^Metasound', 'MetasoundEngine'),
    (r'^CommonActivatableWidget|^CommonButtonBase|^CommonUserWidget|^Input/CommonUIInputTypes', 'CommonUI'),
    (r'^Engine/DeveloperSettings\.h', 'DeveloperSettings'),
    (r'^MassEntity|^MassCommon|^MassProcessor', 'MassEntity'),
    (r'^PhysicsEngine/|^Physics/Experimental', 'PhysicsCore'),
    (r'^LevelSequence', 'LevelSequence'),
    (r'^MovieScene', 'MovieScene'),
    (r'^AudioMixer|^Sound/SoundSubmix', 'AudioMixer'),
    (r'^Kismet/GameplayStatics\.h|^Engine/|^GameFramework/|^Components/(StaticMesh|SkeletalMesh|Scene|Primitive|Box|Sphere|Capsule)', 'Engine'),
    (r'^UObject/', 'CoreUObject'),
]

# Módulos que o Engine já expõe como dependência pública: quem depende de Engine enxerga e linka.
VEM_COM_ENGINE = {'Core', 'CoreUObject', 'Slate', 'SlateCore', 'InputCore', 'Json', 'PhysicsCore', 'DeveloperSettings',
                  'ApplicationCore', 'RenderCore', 'RHI', 'NetCore', 'GameplayTags'}

REFLEXAO = re.compile(r'^\s*(UCLASS|USTRUCT|UINTERFACE|UENUM)\s*\(', re.M)


def sem_comentarios(texto):
    """Troca comentários e literais por espaços, mantendo as quebras de linha (os números de linha batem)."""
    saida, i, n = [], 0, len(texto)
    while i < n:
        c = texto[i]
        if texto.startswith('//', i):
            j = texto.find('\n', i)
            j = n if j < 0 else j
            saida.append(' ' * (j - i)); i = j
        elif texto.startswith('/*', i):
            j = texto.find('*/', i + 2)
            j = n if j < 0 else j + 2
            saida.append(re.sub(r'[^\n]', ' ', texto[i:j])); i = j
        elif c in '"\'':
            j = i + 1
            while j < n and texto[j] != c:
                if texto[j] == '\\':
                    j += 1
                if j < n and texto[j] == '\n':
                    break
                j += 1
            j = min(j + 1, n)
            saida.append(c + re.sub(r'[^\n]', ' ', texto[i + 1:j - 1]) + (c if j - 1 > i else '')); i = j
        else:
            saida.append(c); i += 1
    return ''.join(saida)


def linha_de(texto, pos):
    return texto.count('\n', 0, pos) + 1


def achar_modulos(raiz):
    """Cada *.Build.cs é um módulo: devolve {pasta_do_modulo: (nome, dependências)}."""
    modulos = {}
    for pasta, subpastas, arquivos in os.walk(raiz):
        subpastas[:] = [s for s in subpastas if s not in IGNORAR_PASTAS]
        for a in arquivos:
            if a.endswith('.Build.cs'):
                nome = a[:-len('.Build.cs')]
                cs = open(os.path.join(pasta, a), encoding='utf-8', errors='replace').read()
                deps = set(re.findall(r'"([A-Za-z0-9_]+)"', cs))
                modulos[os.path.abspath(pasta)] = (nome, deps)
    return modulos


def modulo_do_arquivo(caminho, modulos):
    p = os.path.abspath(caminho)
    melhor = None
    for pasta in modulos:
        if p.startswith(pasta + os.sep) and (melhor is None or len(pasta) > len(melhor)):
            melhor = pasta
    return melhor


def arquivos_mudados(base):
    def git(*args):
        r = subprocess.run(['git', *args], capture_output=True, text=True)
        return r.stdout.split('\n') if r.returncode == 0 else []
    nomes = set(git('diff', '--name-only', base)) | set(git('diff', '--name-only', '--cached', base))
    nomes |= set(git('ls-files', '--others', '--exclude-standard'))
    return sorted(n for n in nomes if n.endswith(('.h', '.cpp')) and os.path.exists(n))


def todos_os_arquivos(raiz):
    saida = []
    for pasta, subpastas, arquivos in os.walk(raiz):
        subpastas[:] = [s for s in subpastas if s not in IGNORAR_PASTAS]
        for a in arquivos:
            if a.endswith(('.h', '.cpp')):
                saida.append(os.path.relpath(os.path.join(pasta, a), raiz))
    return sorted(saida)


def corpo_de(texto_limpo, inicio):
    """Do '{' em diante, devolve (abre, fecha) do bloco balanceado."""
    abre = texto_limpo.find('{', inicio)
    if abre < 0:
        return None
    nivel = 0
    for j in range(abre, len(texto_limpo)):
        if texto_limpo[j] == '{':
            nivel += 1
        elif texto_limpo[j] == '}':
            nivel -= 1
            if nivel == 0:
                return abre, j
    return abre, len(texto_limpo)


def conferir_cabecalho(caminho, texto, limpo, modulo, achados):
    base = os.path.splitext(os.path.basename(caminho))[0]
    if '#pragma once' not in texto and not re.search(r'#ifndef\s+\w+', texto):
        achados.append(('AVISO', caminho, 1, 'cabeçalho sem #pragma once'))
    tipos = list(REFLEXAO.finditer(limpo))
    if not tipos:
        return
    includes = [(m.start(), m.group(1)) for m in re.finditer(r'^\s*#\s*include\s*[<"]([^>"]+)[>"]', texto, re.M)]
    gerado = [inc for inc in includes if inc[1].endswith('.generated.h')]
    if not gerado:
        achados.append(('ERRO', caminho, 1, f'tem {tipos[0].group(1)} mas falta #include "{base}.generated.h"'))
    else:
        pos, nome = gerado[-1]
        if nome != f'{base}.generated.h':
            achados.append(('ERRO', caminho, linha_de(texto, pos), f'inclui "{nome}", mas o arquivo é {base}.h: tem que ser "{base}.generated.h"'))
        if includes[-1][1] != nome:
            achados.append(('ERRO', caminho, linha_de(texto, includes[-1][0]), f'"{nome}" tem que ser o ÚLTIMO #include (depois dele vem "{includes[-1][1]}")'))
    nome_mod = modulo[0] if modulo else None
    for m in tipos:
        tipo = m.group(1)
        if tipo == 'UENUM':
            continue
        decl = re.compile(r'(class|struct)\s+(?:(\w+_API)\s+)?(\w+)').search(limpo, m.end())
        if not decl:
            continue
        api, nome = decl.group(2), decl.group(3)
        if api and nome_mod and api != f'{nome_mod.upper()}_API':
            achados.append(('ERRO', caminho, linha_de(limpo, decl.start()), f'{nome} usa {api}, mas o módulo é {nome_mod}: o certo é {nome_mod.upper()}_API'))
        bloco = corpo_de(limpo, decl.end())
        if not bloco:
            continue
        abre, fecha = bloco
        corpo = limpo[abre:fecha]
        if not re.search(r'GENERATED_(BODY|UCLASS_BODY|USTRUCT_BODY|IINTERFACE_BODY|UINTERFACE_BODY)\s*\(', corpo):
            achados.append(('ERRO', caminho, linha_de(limpo, abre), f'{nome} ({tipo}) sem GENERATED_BODY()'))
        conferir_membros(caminho, limpo, abre, fecha, nome, achados)
        if re.search(r'bCanEverTick\s*=\s*true', corpo):
            achados.append(('AVISO', caminho, linha_de(limpo, abre), f'{nome} liga o Tick no cabeçalho: confira se precisa (desempenho é requisito)'))
        for f in re.finditer(r'UFUNCTION\s*\(([^)]*)\)', corpo):
            esp = f.group(1)
            if re.search(r'\b(Server|Client|NetMulticast|BlueprintNativeEvent)\b', esp):
                nomef = re.search(r'(\w+)\s*\(', corpo[f.end():])
                if nomef:
                    achados.append(('NOTA', caminho, linha_de(limpo, abre + f.start()), f'{nome}::{nomef.group(1)} é {esp.strip()}: o .cpp define {nomef.group(1)}_Implementation' + (' e _Validate' if 'WithValidation' in esp else '')))


# Objetos que normalmente têm dono que os segura (ator, árvore de widgets): ponteiro cru para eles é aviso, não erro.
WIDGETS_COM_DONO = {'UWidget', 'UUserWidget', 'UTextBlock', 'URichTextBlock', 'UBorder', 'UImage', 'UButton', 'UCheckBox',
                    'UVerticalBox', 'UHorizontalBox', 'USizeBox', 'UCanvasPanel', 'UOverlay', 'UProgressBar', 'UScrollBox',
                    'UGridPanel', 'UUniformGridPanel', 'UWrapBox', 'UEditableText', 'UEditableTextBox', 'USpacer',
                    'UScaleBox', 'USlider', 'UComboBoxString', 'UWidgetSwitcher', 'UPanelWidget'}

MEMBRO_PONTEIRO = re.compile(r'^\s*(?:mutable\s+)?(?:class\s+)?((?:TObjectPtr\s*<\s*)?[UA][A-Z]\w*\s*>?\s*\*?)\s+\*?\s*(\w+)\s*(?:=[^;]*)?;\s*$')
MEMBRO_CONTAINER = re.compile(r'^\s*(T(?:Array|Map|Set)\s*<[^;]*\b(?:TObjectPtr\s*<\s*)?[UA][A-Z]\w*\s*>?\s*\*?[^;]*>)\s+(\w+)\s*(?:=[^;]*)?;\s*$')


def conferir_membros(caminho, limpo, abre, fecha, nome_classe, achados):
    """Membro do tipo U*/A* (ponteiro, TObjectPtr ou contêiner deles) direto no corpo da classe, sem UPROPERTY na frente."""
    nivel, inicio_linha = 0, abre + 1
    anterior = ''
    for j in range(abre + 1, fecha + 1):
        c = limpo[j] if j < len(limpo) else '\n'
        if c == '{':
            nivel += 1
        elif c == '}':
            nivel -= 1
        if c == '\n':
            linha = limpo[inicio_linha:j]
            if nivel == 0 and linha.strip():
                m = MEMBRO_PONTEIRO.match(linha) or MEMBRO_CONTAINER.match(linha)
                if m and ('*' in m.group(1) or 'TObjectPtr' in m.group(1)) and 'UPROPERTY' not in anterior and 'static' not in linha:
                    tipo = re.findall(r'[UA][A-Z]\w*', m.group(1))[-1]
                    if tipo.startswith('A') or tipo.endswith('Component') or tipo in WIDGETS_COM_DONO:
                        achados.append(('AVISO', caminho, linha_de(limpo, inicio_linha), f'{nome_classe}::{m.group(2)} aponta {tipo} sem UPROPERTY(): só fica vivo enquanto o dono (o ator ou a árvore de widgets) o segura, e o ponteiro não zera quando ele é destruído (use UPROPERTY() ou TWeakObjectPtr se ele pode sumir antes)'))
                    else:
                        achados.append(('ERRO', caminho, linha_de(limpo, inicio_linha), f'{nome_classe}::{m.group(2)} guarda {tipo} sem UPROPERTY(): o coletor de lixo pode apagá-lo (use UPROPERTY() ou TWeakObjectPtr)'))
                anterior = linha
            inicio_linha = j + 1


def declaracoes_do_cabecalho(texto_limpo):
    return set(re.findall(r'~?(\w+)\s*\(', texto_limpo))


def conferir_cpp(caminho, limpo, raiz_repo, achados):
    cab = os.path.splitext(caminho)[0] + '.h'
    candidatos = [cab]
    for pasta in ('Public', 'Private'):
        candidatos.append(cab.replace(os.sep + 'Private' + os.sep, os.sep + pasta + os.sep))
    cab = next((c for c in candidatos if os.path.exists(c)), None)
    if not cab:
        return
    decl = declaracoes_do_cabecalho(sem_comentarios(open(cab, encoding='utf-8', errors='replace').read()))
    for m in re.finditer(r'^[\w:<>,\s\*&]*?\b(\w+)::(~?\w+)\s*\(', limpo, re.M):
        classe, metodo = m.group(1), m.group(2)
        if metodo.endswith(('_Implementation', '_Validate')):
            base_metodo = metodo.rsplit('_', 1)[0]
            if base_metodo not in decl and metodo not in decl:
                achados.append(('ERRO', caminho, linha_de(limpo, m.start()), f'{classe}::{metodo} sem {base_metodo} declarado em {os.path.basename(cab)}'))
            continue
        if metodo.lstrip('~') == classe or metodo in ('StaticClass', 'GetPrivateStaticClass'):
            if metodo.lstrip('~') == classe and metodo.lstrip('~') not in decl and not metodo.startswith('~'):
                achados.append(('AVISO', caminho, linha_de(limpo, m.start()), f'construtor {classe}::{metodo} sem declaração em {os.path.basename(cab)}'))
            continue
        if metodo not in decl and re.search(rf'\b(class|struct)\s+(\w+_API\s+)?{classe}\b', open(cab, encoding='utf-8', errors='replace').read()):
            achados.append(('ERRO', caminho, linha_de(limpo, m.start()), f'{classe}::{metodo} definido aqui e não declarado em {os.path.basename(cab)}'))



# --- sombra de nome (C4456, C4457, C4458) ---------------------------------------------------------------------------
# O MSVC acusa variável local ou parâmetro com o nome de um membro da classe (C4458), local com o nome de um parâmetro
# (C4457) e local com o nome de outro local de um escopo de fora (C4456); o Unreal trata os três como erro. Caso real:
# um "Parts" local num método do AZMBCharacter, que tem o membro Parts (Zombie, 15/10/2026).

# Membros das bases do Unreal que viram nome de variável com facilidade (privados também contam: a busca de nome acha).
MEMBROS_DA_BASE = {
    'AActor': {'RootComponent', 'Tags', 'Instigator', 'Owner', 'Children', 'Layers', 'InputComponent',
               'CustomTimeDilation', 'InitialLifeSpan', 'PrimaryActorTick'},
    'APawn': {'Controller', 'PlayerState', 'BaseEyeHeight', 'LastHitBy', 'AutoPossessPlayer', 'AutoPossessAI',
              'AIControllerClass'},
    'ACharacter': {'Mesh', 'CharacterMovement', 'CapsuleComponent', 'JumpMaxCount', 'JumpCurrentCount',
                   'JumpKeyHoldTime', 'JumpMaxHoldTime', 'CrouchedEyeHeight'},
    'AHUD': {'Canvas', 'DebugCanvas', 'PlayerOwner', 'bShowHUD', 'bShowDebugInfo'},
    'AGameModeBase': {'GameState', 'GameSession', 'DefaultPawnClass', 'HUDClass', 'PlayerControllerClass',
                      'PlayerStateClass', 'GameStateClass', 'SpectatorClass', 'OptionsString'},
    'AController': {'PlayerState', 'Pawn', 'Character', 'ControlRotation', 'StartSpot'},
    'APlayerController': {'Player', 'PlayerCameraManager', 'MyHUD', 'PlayerInput'},
    'UActorComponent': {'PrimaryComponentTick', 'ComponentTags'},
    'USceneComponent': {'Mobility', 'ComponentVelocity', 'AttachParent', 'AttachChildren'},
}
CADEIA_DA_BASE = {
    'APawn': 'AActor', 'ACharacter': 'APawn', 'AHUD': 'AActor', 'AGameModeBase': 'AActor', 'AGameMode': 'AGameModeBase',
    'AController': 'AActor', 'APlayerController': 'AController', 'AAIController': 'AController', 'AWheeledVehiclePawn': 'APawn',
    'USceneComponent': 'UActorComponent', 'UPrimitiveComponent': 'USceneComponent',
}

PALAVRAS_NAO_TIPO = {'return', 'delete', 'new', 'throw', 'goto', 'case', 'else', 'do', 'using', 'typedef', 'break',
                     'continue', 'sizeof', 'default', 'public', 'private', 'protected', 'operator', 'co_return',
                     'co_yield', 'co_await', 'template', 'friend', 'namespace', 'static_assert', 'decltype'}

DECLARACAO = re.compile(
    r'\s*(?:(?:const|constexpr|static|unsigned|signed|volatile|mutable|typename|class|struct)\s+)*'
    r'(?P<tipo>[A-Za-z_][\w]*(?:::[A-Za-z_]\w*)*(?:\s*<[^;{}()]*>)?(?:::[A-Za-z_]\w*)*)'
    r'(?:\s+const)?\s*(?:[*&]+\s*(?:const\s+)?)*\s*(?<=[\s*&>])(?P<nome>[A-Za-z_]\w*)\s*(?P<depois>=(?!=)|;|:(?!:)|\(|\{|\[|,|\))')


def nomes_dos_parametros(lista):
    """Nomes dos parâmetros de 'const FVector& Where, float Scale = 1.0f' (os sem nome ficam de fora)."""
    nomes, nivel, atual = [], 0, ''
    for c in lista + ',':
        if c in '(<[{':
            nivel += 1
        elif c in ')>]}':
            nivel -= 1
        if c == ',' and nivel == 0:
            parte = atual.split('=')[0].strip()
            m = re.search(r'[\s*&]([A-Za-z_]\w*)\s*(\[[^\]]*\])?$', parte)
            if m and m.group(1) not in ('void', 'const') and len(parte.split()) >= 2 or (m and re.search(r'[*&]', parte)):
                nomes.append(m.group(1))
            atual = ''
        else:
            atual += c
    return nomes


def membros_das_classes(limpo_cab):
    """{classe: (base, {membros})} das classes e structs do cabeçalho (só as variáveis, não os métodos)."""
    saida = {}
    for decl in re.finditer(r'\b(class|struct)\s+(?:\w+_API\s+)?(\w+)\s*(?:final\s*)?(?::\s*(?:public|protected|private)\s+([\w:]+))?[^;{]*\{', limpo_cab):
        nome, base = decl.group(2), decl.group(3)
        bloco = corpo_de(limpo_cab, decl.end() - 1)
        if not bloco:
            continue
        abre, fecha = bloco
        membros, nivel, atual = set(), 0, ''
        for c in limpo_cab[abre + 1:fecha]:
            if c == '{':
                nivel += 1
                if nivel == 1:
                    atual = '{'  # corpo de método inline ou tipo aninhado: a declaração não é membro
            elif c == '}':
                nivel -= 1
                if nivel == 0:
                    atual = ''
                continue
            if nivel > 0:
                continue
            if c == ';':
                frase = re.sub(r'\b(public|protected|private)\s*:', ' ', atual)
                frase = re.sub(r'\b(U[A-Z]+|GENERATED_\w+)\s*\((?:[^()]|\([^()]*\))*\)', ' ', frase)
                antes = re.split(r'=(?!=)', frase, maxsplit=1)[0]
                if '{' not in frase and '(' not in antes and not re.search(r'\b(using|typedef|friend|enum|static_assert)\b', frase) \
                        and not re.match(r'^\s*(class|struct)\s+\w+\s*$', antes):
                    m = re.search(r'([A-Za-z_]\w*)\s*(?::\s*\d+\s*)?(?:\[[^\]]*\])?\s*$', antes)
                    if m and len(antes.split()) >= 2:
                        membros.add(m.group(1))
                atual = ''
            else:
                atual += c
        saida[nome] = (base, membros)
    return saida


def membros_herdados(base):
    nomes, vistos = {}, set()
    while base and base not in vistos:
        vistos.add(base)
        for n in MEMBROS_DA_BASE.get(base, ()):
            nomes.setdefault(n, base)
        base = CADEIA_DA_BASE.get(base)
    return nomes


def conferir_sombras(caminho, limpo, classes, achados):
    for m in re.finditer(r'^[\w:<>,\s\*&]*?\b(\w+)::(~?\w+)\s*\(', limpo, re.M):
        classe, metodo = m.group(1), m.group(2)
        if classe not in classes:
            continue
        base, membros = classes[classe]
        herdados = membros_herdados(base)
        # a lista de parâmetros, balanceada
        nivel, j = 0, m.end() - 1
        while j < len(limpo):
            if limpo[j] == '(':
                nivel += 1
            elif limpo[j] == ')':
                nivel -= 1
                if nivel == 0:
                    break
            j += 1
        resto = limpo[j + 1:j + 200]
        corpo_m = re.match(r'\s*(?:const\s*)?(?:override\s*)?(?:noexcept\s*)?(?::[^{;]*)?\{', resto)
        if not corpo_m:
            continue
        parametros = nomes_dos_parametros(limpo[m.end():j])
        linha_assinatura = linha_de(limpo, m.start(1))
        for p in parametros:
            if p in membros:
                achados.append(('ERRO', caminho, linha_assinatura, f'parâmetro {p} esconde o membro {classe}::{p} (C4457/C4458: troque o nome)'))
            elif p in herdados:
                achados.append(('ERRO', caminho, linha_assinatura, f'parâmetro {p} esconde o membro herdado de {herdados[p]} (C4458: troque o nome)'))
        bloco = corpo_de(limpo, j + 1 + corpo_m.end() - 1)
        if bloco:
            conferir_corpo(caminho, limpo, bloco[0], bloco[1], classe, membros, herdados, set(parametros), achados)


def conferir_corpo(caminho, limpo, abre, fecha, classe, membros, herdados, parametros, achados):
    """Anda pelo corpo com uma pilha de escopos; corpo de lambda é pulado (escopo à parte)."""
    escopos = [set()]
    pendentes = set()  # declarados no if/for/while/switch: valem no bloco que vem
    i = abre + 1
    inicio_frase = True
    while i < fecha:
        c = limpo[i]
        if c == '{':
            # lambda: ](...) [mutable] [-> tipo] {  — pula o corpo inteiro
            antes = limpo[max(abre, i - 300):i]
            if re.search(r'\]\s*(\([^()]*(?:\([^()]*\)[^()]*)*\))?\s*(mutable\s*)?(->\s*[\w:<>*&\s]+)?\s*$', antes):
                fim = corpo_de(limpo, i)
                i = (fim[1] if fim else fecha) + 1
                inicio_frase = False
                continue
            # struct ou class local: os campos dela não são variáveis locais; com nome de membro, só um aviso
            if re.search(r'\b(struct|class)\s+\w+\s*(?:final\s*)?(?::[^{;]*)?$', antes):
                fim = corpo_de(limpo, i)
                fim_i = fim[1] if fim else fecha
                for campo in re.finditer(r'([A-Za-z_]\w*)\s*(?:=[^;]*)?;', limpo[i + 1:fim_i]):
                    nome = campo.group(1)
                    if nome in membros or nome in herdados:
                        dono = f'{classe}::{nome}' if nome in membros else f'{herdados[nome]}::{nome}'
                        achados.append(('AVISO', caminho, linha_de(limpo, i + 1 + campo.start()), f'campo {nome} da struct local tem o nome de {dono}: o MSVC pode acusar C4458 (melhor trocar)'))
                i = fim_i + 1
                inicio_frase = False
                continue
            escopos.append(set(pendentes))
            pendentes = set()
            i += 1
            inicio_frase = True
            continue
        if c == '}':
            if len(escopos) > 1:
                escopos.pop()
            i += 1
            inicio_frase = True
            continue
        if c == ';':
            pendentes = set() if not re.search(r'\bfor\s*\([^)]*$', limpo[max(abre, i - 200):i]) else pendentes
            i += 1
            inicio_frase = True
            continue
        cond = re.match(r'(if|for|while|switch)\s*\(', limpo[i:i + 12]) if (i == 0 or not (limpo[i - 1].isalnum() or limpo[i - 1] == '_')) else None
        if cond:
            i += cond.end()
            inicio_frase = True
            # a declaração da condição vai para o bloco que vem
            d = DECLARACAO.match(limpo, i)
            if d and d.group('tipo') not in PALAVRAS_NAO_TIPO and d.group('depois') in ('=', ':', ';'):
                registrar(caminho, limpo, i, d.group('nome'), escopos, pendentes, classe, membros, herdados, parametros, achados, pendente=True)
                i = d.end('nome')
            continue
        if inicio_frase and not c.isspace():
            d = DECLARACAO.match(limpo, i)
            if d and d.group('tipo') not in PALAVRAS_NAO_TIPO and d.group('depois') in ('=', ';', '(', '{', '[', ':'):
                registrar(caminho, limpo, i, d.group('nome'), escopos, pendentes, classe, membros, herdados, parametros, achados)
                i = d.end('nome')
            inicio_frase = False
            continue
        i += 1


def registrar(caminho, limpo, pos, nome, escopos, pendentes, classe, membros, herdados, parametros, achados, pendente=False):
    linha = linha_de(limpo, pos)
    if nome in membros:
        achados.append(('ERRO', caminho, linha, f'{nome} esconde o membro {classe}::{nome} (C4458: troque o nome)'))
    elif nome in herdados:
        achados.append(('ERRO', caminho, linha, f'{nome} esconde o membro herdado de {herdados[nome]} (C4458: troque o nome)'))
    elif nome in parametros:
        achados.append(('ERRO', caminho, linha, f'{nome} esconde o parâmetro {nome} (C4457: troque o nome)'))
    elif any(nome in e for e in escopos[:-1]) or (pendente and nome in escopos[-1]):
        achados.append(('ERRO', caminho, linha, f'{nome} esconde outra declaração local de um escopo de fora (C4456: troque o nome)'))
    (pendentes if pendente else escopos[-1]).add(nome)

def conferir_includes(caminho, texto, modulo_pasta, modulo, achados):
    if not modulo_pasta:
        return
    nome_mod, deps = modulo
    subpastas = {d for d in os.listdir(modulo_pasta) if os.path.isdir(os.path.join(modulo_pasta, d))}
    for m in re.finditer(r'^\s*#\s*include\s*"([^"]+)"', texto, re.M):
        inc = m.group(1)
        if inc.endswith('.generated.h'):
            continue
        local = [os.path.join(os.path.dirname(caminho), inc), os.path.join(modulo_pasta, inc),
                 os.path.join(modulo_pasta, 'Public', inc), os.path.join(modulo_pasta, 'Private', inc)]
        primeiro = inc.split('/')[0]
        do_projeto = primeiro in subpastas or os.path.basename(inc).startswith(nome_mod[:2])
        if any(os.path.exists(c) for c in local):
            continue
        if do_projeto and '/' in inc and primeiro in subpastas:
            achados.append(('ERRO', caminho, linha_de(texto, m.start()), f'#include "{inc}" não existe em {os.path.relpath(modulo_pasta)}'))
            continue
        for padrao, mod in MODULO_DO_INCLUDE:
            if re.search(padrao, inc):
                if mod not in deps and mod != nome_mod and not ('Engine' in deps and mod in VEM_COM_ENGINE):
                    achados.append(('ERRO', caminho, linha_de(texto, m.start()), f'#include "{inc}" é do módulo {mod}, que não está no {nome_mod}.Build.cs'))
                break


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument('--base', default='HEAD', help='compara com esta referência (padrão HEAD)')
    ap.add_argument('--todos', action='store_true', help='confere todo o C++ do repositório')
    ap.add_argument('arquivos', nargs='*', help='arquivos específicos (.h/.cpp)')
    a = ap.parse_args()
    raiz = os.getcwd()
    modulos = achar_modulos(raiz)
    if not modulos:
        print('Nenhum *.Build.cs neste repositório: nada a conferir.')
        return 0
    arquivos = a.arquivos or (todos_os_arquivos(raiz) if a.todos else arquivos_mudados(a.base))
    if not arquivos:
        print(f'Nenhum .h ou .cpp mudou desde {a.base}.')
        return 0
    achados, mudou_cabecalho, mudou_reflexao = [], [], False
    for caminho in arquivos:
        texto = open(caminho, encoding='utf-8', errors='replace').read()
        limpo = sem_comentarios(texto)
        pasta = modulo_do_arquivo(caminho, modulos)
        modulo = modulos.get(pasta)
        if caminho.endswith('.h'):
            conferir_cabecalho(caminho, texto, limpo, modulo, achados)
            mudou_cabecalho.append(caminho)
            if REFLEXAO.search(limpo) or re.search(r'\bU(PROPERTY|FUNCTION)\s*\(', limpo):
                mudou_reflexao = True
        else:
            conferir_cpp(caminho, limpo, raiz, achados)
            cab = os.path.splitext(caminho)[0] + '.h'
            if os.path.exists(cab) and os.sep + 'Rules' + os.sep not in os.sep + caminho:
                classes = membros_das_classes(sem_comentarios(open(cab, encoding='utf-8', errors='replace').read()))
                conferir_sombras(caminho, limpo, classes, achados)
        conferir_includes(caminho, texto, pasta, modulo, achados)
    ordem = {'ERRO': 0, 'AVISO': 1, 'NOTA': 2}
    for nivel, caminho, linha, msg in sorted(achados, key=lambda x: (ordem[x[0]], x[1], x[2])):
        print(f'{nivel:5} {caminho}:{linha}  {msg}')
    erros = sum(1 for x in achados if x[0] == 'ERRO')
    print(f'\n{len(arquivos)} arquivo(s) conferido(s): {erros} erro(s), {sum(1 for x in achados if x[0] == "AVISO")} aviso(s).')
    if not a.todos:
        if mudou_reflexao:
            print('Para o PC: mudou cabeçalho com UCLASS/UPROPERTY/UFUNCTION. Feche o editor e compile pelo script do projeto (Live Coding não troca o layout da classe).')
        elif mudou_cabecalho:
            print('Para o PC: mudou cabeçalho sem reflexão. Compile pelo script do projeto; Live Coding pode não pegar.')
        else:
            print('Para o PC: só corpo de função em .cpp. Com o editor aberto, o Live Coding (Ctrl+Alt+F11) basta.')
    return 1 if erros else 0


if __name__ == '__main__':
    sys.exit(main())
