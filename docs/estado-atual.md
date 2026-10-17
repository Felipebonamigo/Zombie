# Estado atual do projeto

Documento de passagem. Uma sessão nova do Claude (nuvem ou PC) lê este arquivo e o `CLAUDE.md` e consegue continuar sem
o histórico das conversas. Atualizar a cada bloco de trabalho fechado.

Última revisão: 16 out 2026 (sessão do PC: realismo, passos 0, 1 e 2a; antes, blocos 1 a 9 na nuvem).

## Como o trabalho está dividido

- **Nuvem:** design, contratos em `docs/`, regras puras com testes (compilam aqui), C++ do Unreal escrito e conferido sem
  compilar, dados e documentação.
- **PC do Felipe:** compila, roda os testes do Unreal, joga, captura, cria materiais e mapas por script e mede.
- **Felipe:** joga e decide a sensação, o visual e o escopo.

## O que existe

| Área | Onde | Estado |
|---|---|---|
| Design, estratégia viral, roteiro | `docs/design.md`, `docs/viralizar.md`, `docs/roadmap.md` | Versão 0.1, para o Felipe ler e decidir |
| Regras puras: cidade, fluxo (com custo extra), grade fina da base, corpo, explosão, hype, Diretor, dia e noite, chat, clipes, pautas, base, som, marcos e resumo da live, saque, filtro de palavras, ferimentos, CSV | `Source/Zombie/Rules/` | **139 testes passando** na nuvem (g++ 13 e clang++, `-Wall -Wextra -Wshadow -Wconversion -Werror`, ASan e UBSan) |
| Tabelas de ajuste | `Data/*.csv` | Carregam sem erro (testes `Deriva.*`) |
| Projeto Unreal 5.8 | `Zombie.uproject`, `Config/`, `Source/` | Escrito na nuvem; **ainda não compilado**. Conferidor de C++ sem erros. Uma revisão independente das assinaturas da API achou 1 erro de compilação (include do material dinâmico) e 1 defeito (o carro não viraria: o `APawn` segue o yaw do controle por padrão); os dois foram corrigidos. A revisão do bloco 3 (base) não achou erro de compilação; achou 10 defeitos de comportamento (zumbi alto não via a barricada, mordida por cima dela, peça afundando na praça, mira do modo construção pegando o que estava atrás do jogador, entre outros), todos corrigidos. A do bloco 4 também não achou erro de compilação; achou 6 defeitos (a calçada não existia para as regras, o preço de atravessar a barricada dobrava conforme o encaixe, vão de 1,5 m invisível, zumbi andando em "L", carro parando na segunda barricada e derrubando muro de raspão), todos corrigidos. A do bloco 5 (som) também não achou erro de compilação; achou 3 defeitos (a distância medida da câmera, a ~10 m, deixava recarga, chute e mordida quase mudos; a rajada do fuzil e a cadeia de bujões perdiam toques pelo limite de vozes; o volume do motor ignorava a tabela) e 11 menores, todos corrigidos. Uma revisão de compilação do código todo (até o bloco 7) achou 1 erro provável (parâmetro `bHidden` de lambda escondendo `AActor::bHidden`), corrigido; desde então o conferidor pega sombra de nome (C4456, C4457 e C4458), inclusive em lambda, sem falso positivo no Detonauto (que compila). A revisão dos blocos 6 e 7 não achou erro de compilação; achou 3 defeitos (marco batido dirigindo perdia o prêmio, o anúncio do marco com caixa sumia, saque podia nascer em cima de carro ou bujão) e 7 menores, todos corrigidos. A dos blocos 8 e 9 também não achou erro de compilação; achou 1 defeito nos ferimentos (a explosão do zumbi-bujão abria ferimento) e 4 no filtro ("food" e apelidos como "Hancock" eram pegos, letra esticada e palavras coladas por pontuação escapavam), todos corrigidos |
| Jogo de protótipo | `Source/Zombie/{Game,Player,Zombies,Combat,Base,Vehicles,Stream,UI}` | Cidade gerada, jogador, 7 tipos de zumbi (com o gritador), explosões, carro, Diretor, live com pautas do patrocinador, base com barricadas e armadilhas (modo construção, X), som (25 sons, 65 arquivos), marcos de seguidores e resumo da live com nota, saque (lixeira, mochila, isopor e viatura), ferimentos que sangram, Twitch com filtro de palavras (Modo Streamer) e HUD (lista no README) |
| Branches | `main` e `claude/inspiring-einstein-idxmpu` | O `main` foi criado em 14/10 a partir do branch do Claude, a pedido do Felipe; os dois andam juntos |
| Scripts do PC | `Tools/Unreal/*.ps1`, `criar_materiais.py` | Copiados e adaptados do Detonauto; `jogar.ps1` e `criar_materiais` são novos e nunca rodaram |
| Rotinas do Claude | `.claude/skills/` | `verificar`, `revisar-sem-compilar`, `jogar-e-capturar`, `modelo-3d`, `triar-playtest` |

## Realismo (16 out 2026, sessão do PC; roteiro em `docs/realismo-roteiro.md`)

- **Compila e roda no PC.** `build.ps1` passou; `importar_sons.ps1` (65 sons) e `criar_materiais.ps1` rodaram (os dois
  devolvem código 1 por causa de erro de Python de plugin experimental da engine ao abrir, mas o log traz "OK" e os
  `.uasset` existem, em `Content/Zombie/`, no LFS).
- **Passo 0, a régua:** como a cidade é montada em código (não há `.umap`), o `L_Lookdev` virou a opção `-ZMBLookdev=1..4`
  do jogo (`jogar.ps1 -Lookdev N`): ângulo fixo (rua, orla, mar, leste), 17,5 h, relógio parado, sem horda, câmera de
  primeira pessoa. `Tools\Unreal\capturar_lookdev.ps1 -Passo X -Quando antes|depois [-Hour 21]` tira as quatro e guarda em
  `docs/img/realismo/`. Não há `criar_lookdev.py`: não há mapa para criar.
- **Passo 1, luz e céu:** hora dourada (o sol esquenta de 35° para baixo e a neblina ganha o halo do sol), lua mais forte
  (0,3 para 0,5), bloom menor (0,55 para 0,35), exposição de celular (adapta mais rápido), saturação sem o corte do
  protótipo, mar mais azul-esverdeado. **Faróis dos carros largados apagados** (só acendem com o jogador dirigindo): acesos,
  estouravam o chão em volta, de dia e de noite. Capturas `2026-10-16-1-luz-depois-*` contra `0-base-antes-*`, olhadas.
- **Medição** (`ZMBPerf` do `jogar.ps1`, sem vsync nem limite, 1600x900, RTX 4070 Ti, rua vazia): média de 3 a 6 ms
  (160 a 310 fps) antes e depois, bem dentro dos 16,7 ms. Pior quadro de 5 a 12 ms; uma vez em cada rodada aparecem
  engasgos de 70 ms a 3,7 s no primeiro uso de um ângulo novo (compilação de shader, não do passo). **Falta medir com a
  horda de 80** e em resolução maior; sem isso o orçamento do passo 1 não está provado para a hora do combate.
- **Diversão e visual:** o fim de tarde agora tem cara de filmado (luz laranja, sombras longas, céu quente) e a noite ficou
  legível e azulada; a diversão não mudou, só o clima.
- **Passo 2a, malhas do City Sample (16 out 2026):** o City Sample (UE 5.8) foi baixado pelo Launcher (`...\OneDrive\Documents\Unreal Projects\CitySample`, 96 GB;
  o OneDrive pode sincronizar isso: pausar ou mover). Migrado só o que o 2a usa (81 pacotes, 522 MB) por um projeto de
  staging fora do repositório (`C:\Users\Felip\UE\StagingCitySample`), renomeado lá para `/Game/Externo/CitySample/` (o editor
  atualiza as referências; 0 referências fora de Externo) e copiado para `Content/Externo/CitySample/`, ignorado pelo git.
  Nova tabela `Data/Cidade_Malhas.csv` (`docs/dados.md`) e `UZMBGameData::CityMeshes(id)`: **sem o asset, o jogo volta ao
  bloco do protótipo** e avisa no log, uma vez por caminho. Trocados por malha: **poste de rua** (visto de perto e de longe),
  **caixa d'água** e **ar-condicionado de telhado** (estes dois **não conferidos em captura**: nenhum ângulo de teste
  enxergou um telhado com eles). **Árvores:** as do City Sample são de inverno, sem folha (captura
  `2a-props-seed7-depois-praca`) e foram trocadas pela árvore do Poly Haven (item seguinte). A semente padrão 2026 não tem
  praça (0 praças; a 7 tem 36), então só a semente 7 mostra árvores.
- **Árvore folhuda (16 out 2026):** `island_tree_02` do Poly Haven (CC0), a mesma do Age of Earth, importada por
  `Tools\Unreal\criar_arvore.ps1` em `Content/Externo/PolyHaven/` (fora do git) com os materiais `M_ZMBFolha` e `M_ZMBTronco`
  (nossos, no git/LFS), sem Nanite e com 4 LODs. Entrou como `arvore` em `Data/Cidade_Malhas.csv` (escala 2). Captura
  `2a-arvore-depois-praca` (semente 7): folhagem à luz da hora dourada. Medição na praça: média 6,5 ms (antes ~4 ms, ~+2,4 ms
  pelas árvores), pior quadro 173 ms (shader no primeiro uso). Sem a malha no disco, volta ao tronco e copa de bloco.
- **Passo 2b, o que ficou (16 out 2026):** os prédios do City Sample **não servem**: são composições procedurais de Packed Level
  Instance (`Kit_Hero_Bldg`, `Kit_Ref_Bldg`, centenas de malhas cada, 40 a 90 m de lado) em arquitetura de Chicago, Nova York e
  São Francisco, contra as nossas células de 20 m. Em vez deles, **superfícies PBR**: asfalto, calçada e fachada de reboco
  com textura do Poly Haven (CC0; as mesmas do Detonauto) por `Tools\Unreal\criar_superficies.ps1`
  (`M_ZMBSuperficie` no git, texturas e instâncias em `Content/Externo/PolyHaven/Superficies`, fora do git), projetadas pelo mundo
  com `Tile` e `Tint`. `UZMBGameData::SurfaceMaterial` cai na cor lisa sem as texturas. Capturas `2b-superficies-depois-*`:
  o asfalto lê como asfalto de dia e de noite; as fachadas continuam sem relevo e **as janelas seguem faixas lisas**.
  Também: o laranja da hora dourada abrandou (sol de 0,62/0,32 para 0,74/0,5 em verde/azul) porque, com textura, o chão
  parecia areia. Medição: média de 4,1 a 4,9 ms na maior parte dos ângulos, mas **9,3 ms no leste e 8,2 ms na rua à noite** (sem
  causa confirmada; repetir). Próximo: janelas com vidro e interior falso (o `Detonauto` já tem o material), térreo com portão e toldo.
- **Passo 2c, janelas e térreo (17 out 2026):** as faixas de janela agora usam o vidro com salas falsas do Detonauto
  (`M_ZMBJanela`, interior mapping, no git, criado por `criar_superficies.ps1`): reflexo de vidro, caixilho a cada 1,5 m e uma
  sala por célula de 4,5 m, acesa ou apagada por sorteio. O brilho do interior segue a hora (`BrilhoInterior`: 0,1 de dia e
  0,3 à noite, em unidades do jogo; 250 e 1500 estouravam tudo, que é a unidade em nits do Detonauto). Térreo: portão de enrolar
  de metal (55% das fachadas) e toldo inclinado de lona em quatro cores (45%), sem colisão e com sorteio à parte, então a
  cidade não muda. Sem o material, volta ao vidro liso e às janelas acesas do protótipo. Capturas `2c-janelas-depois-*`: de
  dia as janelas leem como vidro com caixilho; à noite dá para ver sala acesa e apagada. **Toldos não conferidos** (nenhuma
  captura os pega de frente). A aberração cromática do pós-processo (0,2) fica forte nas janelas claras: reduzir.
  Medição: 4,5 a 5,9 ms, com um quadro de 19 ms (53 fps) que não repetiu; nesse momento o `OneDrive.Sync.Service` estava
  usando muita CPU (a pasta do City Sample, 96 GB, está dentro do OneDrive): **pausar a sincronização ou mover o projeto**.
- **Medição com horda e onde pesa (17 out 2026, RTX 4070 Ti, semente 2026, `-Horde`, câmera média, 40 s, sem vsync):**
  `ZMBPerf` agora separa os fios (jogo, render, GPU) e conta os zumbis vivos. Dia, 1920x1080, 80 zumbis: **jogo 7,7 ms**,
  render 3,9, GPU 3,9. **Noite, 1920x1080, 140 zumbis: jogo 12 a 16 ms** (quatro rodadas: 15,9, 12,7, 12,1 e 12,4), render 5 a 6,
  GPU 4. Ou seja, **o gargalo é a lógica (fio do jogo), não o visual**: a GPU está em ~4 ms. 4K não foi remedido depois que o
  `ZMBPerf` passou a separar os fios. Medição anterior sem separar (média de quadro): 4K de dia 68 ms e 4K à noite 11,5 ms,
  incoerentes entre si, **não confiáveis**. O alvo é um PC nível PS5, com CPU mais fraca que a deste PC: **12 a 16 ms de jogo aqui
  não dão folga**.
  Unreal Insights (`-trace=cpu,frame,bookmark -statnamedevents`; export por `UnrealInsights.exe -OpenTraceFile=... -ExecOnAnalysisCompleteCmd="TimingInsights.ExportTimerStatistics <csv> -threads=GameThread" -AutoQuit -NoUI`, que funciona):
  `Char PerformMovement` inclui ~8,5 ms por quadro (140 zumbis, uma vez cada), `Phys SetBodyTransform` ~1.365 chamadas por
  quadro (~3 ms) e `WaitForTasks` 3,5 ms por quadro. Os 1.365 vêm dos **6 membros de cada zumbi** (`QueryOnly`, bloqueiam só
  a Visibility para o tiro por membro), que são corpos físicos movidos junto com a cápsula a cada passo.
  Teste A/B (flags temporárias, já removidas): sem corpo físico nos membros, **jogo 9 a 10 ms (−3 ms)**; sem o desvio RVO, sem
  mudança (13,8, dentro do ruído de ±2 ms entre rodadas).
  **Decisão do Felipe pendente:** tirar o corpo físico dos membros e acertar a parte do corpo por uma só cápsula (`ZRBody` não muda;
  muda como o tiro descobre o membro: pela altura e pelo lado do ponto de impacto), −3 ms estimados, em troca de um tiro por
  parte menos exato (cabeça como faixa do cilindro, não esfera). Outros candidatos ainda não medidos: `ComputeFloorDist`
  (234 mil chamadas), o tick de cada zumbi (233 mil) e o componente `CharacterMesh0` vazio.
- Cena de aparência: ângulos 5 (poste), 6 (praça) e 7 (telhado) em `-ZMBLookdev` e `capturar_lookdev.ps1 -Angulos 5,6 -Seed 7`.
- Medição do 2a (rua vazia, 1600x900): média de 3,8 a 4,9 ms, pior quadro 7 a 9 ms (112 ms no primeiro ângulo, compilação
  de shader). Mesma faixa do passo 1, **sem horda ainda**.
- **Diversão e visual:** o poste agora tem braço e cabeça de verdade e a rua perde um pouco do ar de maquete; o resto
  (prédios, calçada, carros) segue em bloco. Os prédios do City Sample são arquitetura de arranha-céu americano e vêm
  montados por regras procedurais: o 2b depende de uma decisão do Felipe (volumes empilháveis ou fachadas brasileiras).
- Pendências vistas nas capturas: o mar mal aparece (a praia é larga e a água, uma faixa fina no horizonte); o céu do
  meio-dia ainda não foi olhado; neblina por zona (orla úmida, centro com fumaça) e luz de poste mais forte ficam para
  quando a cidade tiver as malhas do City Sample.

## Próximo passo (sessão do PC)

1. `Tools\Unreal\build.ps1 -Uproject Zombie.uproject`. O C++ nunca foi compilado: espere alguns erros e corrija um por um.
   Cada erro que o conferidor deveria ter pego vira regra nova nele (skill `revisar-sem-compilar`, passo 6).
2. `Tools\Unreal\test.ps1 -Uproject Zombie.uproject -Filter Zombie.Regras`: os mesmos 139 testes da nuvem.
3. `Tools\Unreal\criar_materiais.ps1`: materiais com aspereza e brilho; sem eles, o jogo usa o material básico da engine.
   `Tools\Unreal\importar_sons.ps1`: importa os 65 sons de `Audio/`; sem eles, o jogo roda mudo.
4. Capturas: `jogar.ps1 -Shot 8`, `-Shot 8 -Hour 21` e `-Shot 8 -Cam 3`, em `docs/img/prototipo/`.
5. O Felipe joga 15 minutos (`jogar.ps1`) e manda os problemas como Issues "[Playtest]".

## Pontos para olhar no primeiro playtest

- Câmera longe e alta (9 m, 3,8 m acima, 18° para baixo): ver a horda e mirar ao mesmo tempo funciona?
- Ritmo do Diretor: 0,6 zumbi/s na tensão e 2,5 no pico, à noite x2,5. Começa calmo demais ou pesado demais?
- Carro de protótipo: suspensão por raios com números calculados, não medidos. A mola e o amortecedor podem precisar de
  ajuste (`carro.*` em `Data/Ajustes.csv`).
- Explosão: a fila de 10 bujões a 25 m do começo tem que virar reação em cadeia.
- Hype: a primeira doação (hype 50) tem que chegar nos primeiros minutos de ação.
- Pautas: a primeira chega aos 45 s se houver 10 espectadores. Com 37 seguidores no começo, isso pede hype alto. Conferir se
  ela aparece cedo o bastante (`pauta.espectadores_min`).
- Gritador: o grito a 25 m chama 5 zumbis correndo. Ver se dá tempo de reagir e se o anúncio na tela não cansa.
- Base (X): a caixa de arame da peça aparece onde a mira aponta? Fechar uma rua de 20 m pede 5 barricadas (100 de sucata);
  a sucata do começo (60) e a do carro desmanchado (25) dão ritmo? O zumbi travado bate na barricada e quem chega de lado
  dá a volta? As estacas derrubam (perna arrancada vira rastejante)? Pular a barricada de madeira (1,1 m) funciona?
  Bujão armado no meio da horda vira cadeia?
- Som: os tiros soam fortes e cada disparo diferente? A explosão tem peso? Os gemidos viram um coro sem cansar? O motor
  do carro sobe com a velocidade? Na mira lenta (Q) tudo fica grave? Algum som alto ou baixo demais (`volume` em
  `Data/Sons.csv`)? A distância agora conta do personagem (não da câmera) e cai até -30 dB no fim do alcance: a horda a
  20 m soa perto o bastante? A rajada do fuzil soa contínua? O chiado da granada vai junto com ela e para no estouro?
- Ferimentos: o "SANGRANDO x2" aparece e se nota? 30% de chance por mordida forte e 0,4 de vida por segundo por 30 s
  pressionam a procurar curativo sem matar sozinho? Ou sangra demais junto com a horda (`ferimento.*`)?
- Filtro de palavras: com a Twitch ligada (`-ZMBTwitch=canal`), um palavrão no chat sai com `*` no HUD e o `!zumbi` de
  quem tem apelido ofensivo vira "espectador"? Alguma palavra comum sendo pega por engano?
- Saque: a dica "segure E: revirar a lixeira" aparece na hora certa? 2 s segurando E é bom, ou cansa? A lixeira rende
  demais ou de menos (`Data/SaqueItens.csv`)? O barulho da tampa (3) e do porta-malas (5) chama zumbis na medida? A
  caixa da viatura (4,6 m) atrapalha a rua? Dá para achar os lugares de longe (cor) e saber quais já foram revirados?
- Marcos e resumo: o primeiro marco (50 seguidores) chega cedo e o presente se nota? A linha "META" embaixo dos
  seguidores ajuda ou polui? Na tela "A LIVE CAIU", o resumo cabe e dá vontade de printar? 8 s é tempo bom para ler
  (`jogador.renascer_s`)? A nota (S a D pelo pico de espectadores, `resumo.*`) parece justa?
- Base com abertura: feche uma rua com barricadas, incluindo as duas calçadas (dá para construir nelas), e deixe um vão de
  2 m com estacas. A horda que vem de longe dá a volta e entra
  pelo vão? Sem vão, ela vem e bate? O zumbi segue o campo fino sem andar em zigue-zague? Carro a 60 km/h arromba a
  barricada de madeira e segue; no muro de sucata, para.

## Realismo (a próxima fase grande)

Primeiro playtest do Felipe (16/10/2026): compilou sem erro, câmera, horda, explosões e a live estão ok; **falta realismo e
qualidade gráfica**. O roteiro está em `docs/realismo-roteiro.md`: luz e céu, rua (City Sample), personagem e zumbis, explosão
em Niagara e modo celular, cada passo com captura e medição. Conteúdo da Epic fica em `Content/Externo/` (ignorado pelo git:
o repositório é público). A sessão local executa; a nuvem adianta as tabelas e o código que não dependem do download.

Sem som no primeiro playtest: faltava rodar `Tools\Unreal\importar_sons.ps1` (editor fechado).

## Decisões pendentes do Felipe

Estão em `docs/roadmap.md`, "Decisões que são do Felipe": nome, repositório público ou privado, horas por semana, preço e
nível de gore.

## Pendências conhecidas

- Jogo empacotado: `Data/*.csv` entra solta no pacote pelo `RuntimeDependencies` do `Zombie.Build.cs`; falta empacotar no
  PC e conferir que `Zombie/Data/` aparece ao lado do executável (o Detonauto tem a mesma pendência).
- Base: a horda só contorna a base dentro da grade fina (64 m em volta do jogador); de longe, segue a rua da cidade. Portas
  e janelas de prédio ainda não trancam.
- **Som escolhido sem ouvir:** os 65 arquivos (CC0 e CC BY, licença conferida página a página) foram escolhidos pela
  descrição e pela forma de onda. O Felipe precisa ouvir no jogo e apontar o que trocar. Falta uma batida de carro de
  verdade (hoje é metal batendo) e não há música. A melhor fonte de tiros, a Free Firearm Sound Library (CC0, 194 MB),
  fica para baixar no PC.
- Manchas de sangue e poeira de impacto não contam no orçamento de pedaços (`mundo.pedacos_max`). A mancha não tem Tick e
  só 30% dos acertos deixam uma; medir com horda grande.

## Armadilhas já descobertas

- **Consulta por tipo de objeto devolve o primeiro que achar:** para achar uma classe (a barricada), use a varredura
  múltipla e filtre; armadilha, membro de outro zumbi e a caixa do drone também são WorldDynamic.
- **A calçada fica dentro da célula do prédio:** a regra que só olha a célula de 20 m não a enxerga. O recuo de cada
  fachada mora na planta (`FCityPlan::InsetCm`); use `IsWalkablePoint` para chão fino.
- **Zumbi nasce escalado** (`escala` de `Zumbis.csv`, até 1,6): varredura "na altura do zumbi" tem que usar a cápsula
  dele, não uma altura fixa.
- **O chão não é plano:** a praça tem a laje da calçada (15 cm) e a grama. Peça colocada no mundo procura o chão com um
  traço para baixo.

- **`APawn` segue o yaw do controle por padrão:** todo pawn com física própria (o carro) precisa desligar
  `bUseControllerRotationYaw`, senão a rotação é teleportada todo quadro.
- `FMath::FloorToInt(double)` devolve `int64` no UE5: para `int32`, use `FloorToInt32`.
- **Praça cercada de prédios ficava inalcançável** para a horda. O teste `Cidade.TodoChaoAndavelEhAlcancavel` pegou; a praça
  passou a ser o quarteirão inteiro.
- Herdadas do Detonauto (`docs/tecnico.md`): `bUseUnity = false`; sombra de nome é erro no Unreal; material com Nanite
  precisa da flag; teste verde não prova visual.
