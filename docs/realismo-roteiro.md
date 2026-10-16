# Roteiro do realismo (para a sessão local do PC)

Pedido do Felipe (16/10/2026, depois do primeiro playtest): o jogo está divertido de ver, **falta realismo e qualidade
gráfica**. Este roteiro leva o protótipo de blocos (`docs/plano-visual.md`) a uma cidade e a personagens de verdade, **sem
colocar conteúdo da Epic no git**: o repositório é público (`.gitignore` e `CLAUDE.md`).

Quem executa: a sessão local do PC (Unreal 5.8 aberto, RTX 4070 Ti). A nuvem escreve código e dados; só o PC baixa, migra e
captura. Princípio do estúdio (`CLAUDE.md`): divertido e bonito, **aprovado por captura olhada**, e 60 fps.

## Regra de ouro: o que entra no git e o que não entra

| Entra no git | Fica só no PC (fora do git) |
|---|---|
| C++, scripts de `Tools/`, `Data/*.csv`, `docs/` | Qualquer conteúdo da Epic: City Sample, Game Animation Sample, MetaHuman |
| Texturas CC0 (Poly Haven) e modelos CC0 ou CC BY, com linha em `docs/licencas.csv` | Conteúdo do Mixamo |
| Materiais e mapas **gerados por script Python** (`Tools/Unreal/`) | Plugins de upscaler (já barrados no `.gitignore`) |

- Todo conteúdo da Epic vai para **`Content/Externo/`**, que o `.gitignore` já ignora. Nada dele em `Content/Zombie/`.
- O que nós criamos **a partir** dele (um material nosso, um mapa) referencia o externo por caminho. Quem clonar sem o
  conteúdo da Epic abre o jogo com os blocos do protótipo no lugar (o código tem que tolerar asset ausente: `LoadObject`
  que falha vira o bloco e um aviso no log, nunca crash).
- Antes de qualquer `git add`: `git status` e conferir que nada de `Content/Externo/` aparece.

## Ordem (cada passo termina com captura olhada)

### Passo 0: a cena de aparência `L_Lookdev` (fazer primeiro, é barato)

Uma esquina de Porto Brasa no fim de tarde, quatro ângulos fixos, antes e depois. Serve de **régua** para todos os passos
seguintes. Script: `Tools/Unreal/criar_lookdev.py`, a escrever (o do Detonauto é o modelo). Capturas em
`docs/img/realismo/` com o nome `AAAA-MM-DD-<passo>-antes|depois-<angulo>.png`, olhadas antes de seguir.

### Passo 1: luz e céu (a mudança maior pelo menor custo)

Hoje: Lumen, sombras virtuais, TSR, neblina e céu físico já ligados. Falta fazer a imagem parecer **filmada**:

- hora dourada e noite de verdade: sol e lua com cor e intensidade pela hora (`AZMBWorldBuilder::UpdateSky`);
- exposição automática com curva de câmera de celular, `PostProcess` com bloom, vinheta, grão e aberração leve;
- neblina volumétrica por zona (orla úmida, centro com fumaça de queimadas);
- luz dos postes e dos carros acesa à noite (já há 64 luzes de poste).

Aceite: as quatro capturas do `L_Lookdev` ao fim de tarde e à noite, e `stat unit` dentro de 16,7 ms.

### Passo 2: a rua (City Sample)

1. Baixar o **City Sample** pelo Epic Games Launcher (Exemplos/Fab, grátis para Unreal). É enorme: baixe fora da pasta do
   projeto e **não** abra o mapa inteiro.
2. Migrar para `Content/Externo/CitySample/` **só o que se usa**: prédios (fachadas e lojas), calçada e meio-fio, postes,
   carros de rua, lixeiras e mobiliário urbano. Use *Migrate* do editor (traz as dependências). Nunca o pacote inteiro.
3. Trocar os blocos pelas malhas: `AZMBWorldBuilder::BuildBlocks` hoje cria cubos por prédio. A troca é por **tabela em
   texto** (`Data/Cidade_Malhas.csv`: tipo de prédio, malha, escala, andares), lida pelo `UZMBGameData`, com os blocos como
   reserva. Isso a nuvem escreve; o PC só preenche os caminhos dos assets.
4. Props espalhados (lixo, carros abandonados, varais, caixas d'água) com PCG, parâmetros em `Data/` quando possível.
5. Colisão simples nas fachadas (caixa), não a malha detalhada: a horda e o fluxo (`ZRCityPlan`) continuam lendo a planta.

Aceite: captura de dia e de noite da rua principal; 60 fps com a horda de 80 zumbis; a planta e o fluxo da horda intactos
(testes `Zombie.Regras` verdes).

### Passo 3: personagem e zumbis

1. **Game Animation Sample** (Fab, grátis): Motion Matching e os esqueletos Manny/Quinn. Para o jogador.
2. **MetaHuman** (para o Kaique "Brasa" e alguns zumbis de rosto), via Fab. Ver o custo de memória antes de usar em
   multidão: **a horda usa esqueleto simples e variações de material**, MetaHuman só em quem aparece de perto.
3. Zumbi: esqueleto com material de pele doente, sangue em camadas (decalque por parte do corpo), e o desmembramento hoje
   por membros de bloco (`ZRBody`) passa a **cortar o osso**. A regra pura (`ZRBody`) não muda; muda o visual de `AZMBZombie`.
4. Variação por instância (cor de pele, roupa de folião, fantasia de Carnaval) com *custom primitive data*, para a horda
   não ter dois zumbis iguais.

Aceite: capturas do jogador e de um zumbi de perto e de longe; 60 fps com a horda.

### Passo 4: explosão e efeitos (Niagara)

Bola de fogo, fumaça que sobe, faíscas, onda de choque, decalque de queimado e luz que pisca; o bujão ganha a malha real.
Hoje `AZMBFx` e `ZMBExplosion` desenham esferas e luz. A explosão em cadeia é o gancho viral: este passo vale o capricho.

Aceite: a fila de 10 bujões em captura de vídeo curto, e orçamento de partículas medido na horda (skill `achar-gargalo`).

### Passo 5: modo celular

Lente mais convincente (distorção, rolagem do obturador nos movimentos rápidos, foco "caçando"), compressão de vídeo
simulada e a interface do app de live. É o "isso é real?" do `docs/viralizar.md`.

## Desempenho (não negociável)

- Medir **antes e depois de cada passo** na mesma cena (`stat unit`, `stat gpu`), e registrar no `docs/estado-atual.md`.
- Upscaler (DLSS/FSR) só depois de medir o gargalo: skill `ligar-upscaler`. TSR já está ligado.
- Se um passo estourar 16,7 ms, **não segue**: reduz (LOD, distância de vista, instâncias) antes do próximo.

## Fechar cada passo

1. Capturas em `docs/img/realismo/`, olhadas, com o antes e o depois.
2. `docs/licencas.csv`: uma linha por pacote novo (a Epic entra como "conteúdo da Epic, só em jogo Unreal, fora do git").
3. `docs/estado-atual.md`: o que mudou, a medição e **uma linha sobre o que o passo mudou na diversão e no visual**.
4. Commit só do que é nosso (`git status` limpo de `Content/Externo/`), em português, com "Para o PC:" quando mexer em C++.

## O que a nuvem faz enquanto o PC trabalha

- Tabela `Data/Cidade_Malhas.csv` e o carregamento (com reserva em bloco) e o teste de deriva dela.
- Curvas de hora do dia, cor e intensidade da luz em `Data/` e em `ZRDayNight`, com teste.
- Variação de zumbi por instância (parâmetros e sorteio em regra pura).
- Os scripts Python de cena e de material de `Tools/Unreal/`.

Nada disso precisa do conteúdo da Epic, então dá para adiantar sem esperar o download.

## Pendências que são do Felipe

- Baixar o City Sample e os demais pelo Launcher/Fab (login da conta Epic): só ele.
- Se for gastar créditos do Meshy para algum prop: a sessão pergunta antes, com o custo (~30 créditos por modelo com textura).
- Confirmar o nome do jogo e o gore (muda o sangue do passo 3).
