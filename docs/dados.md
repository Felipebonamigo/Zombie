# Tabelas de ajuste (`Data/`)

Tudo que se ajusta mora em CSV de texto, para editar no Excel, LibreOffice ou direto no GitHub. O jogo lê na hora de abrir
(`UZMBGameData`); no console do jogo, `zmb.dados.recarregar` lê de novo e vale para o que nascer depois. Os testes de deriva
(`Tools/Regras/testar.sh Deriva.`) carregam as mesmas tabelas e falham se algo estiver errado.

Regras comuns:

- UTF-8; a primeira linha é o cabeçalho.
- Número com ponto ou vírgula decimal. Texto com vírgula vai entre aspas.
- Colunas a mais (como `descricao`) são ignoradas pelo jogo e servem de anotação.

## `Ajustes.csv`: todos os números soltos

`chave,valor,unidade,descricao`. Uma linha por número do jogo. As chaves conhecidas estão em `ZR::TuningKeys()`
(`Rules/ZRData.cpp`, função `BindTuning`, com a faixa aceita de cada uma). Chave desconhecida ou repetida é erro; chave
que falta usa o padrão do código e aparece como aviso no log. Grupos:

| Prefixo | O que ajusta |
|---|---|
| `hype.` | Combo, variedade, queda, espectadores, seguidores e doações (`regras-de-jogo.md`, seção 5) |
| `diretor.` | Ritmo da horda, tetos de zumbis vivos, noite, horda e barulho (seção 6) |
| `cidade.` | Tamanho da cidade, quarteirões, praia e mar, praças, andares, recuo das fachadas (calçada e beco) e tamanho da célula (seção 2) |
| `chat.` | Rodada de votação, votos mínimos, espera do `!zumbi`, zumbis com nome ao mesmo tempo (seção 9) e o filtro de palavras do Modo Streamer (seção 16) |
| `pauta.` | Espera da primeira pauta, intervalo entre pautas e audiência mínima (seção 11) |
| `base.` | Sucata (começo, caixa, carro), desmanche, conserto, desmonte, alcance, grade, teto de peças, dano nas peças (zumbi, explosão, carro), barulho e a grade fina em que a horda contorna a base (`base.horda_*`) (seção 12) |
| `dia.` | Duração do dia, hora inicial, noite e Noite da Horda (seção 7) |
| `camera.` | As três distâncias da terceira pessoa, a mira, a primeira pessoa e o modo inicial |
| `jogador.` | Vida, velocidades, fôlego, chute, mira lenta (foco), arremesso, sobra de vida, cura sozinha e tempo para voltar à live |
| `resumo.` | Pico de espectadores que dá cada nota do resumo da live (C, B, A e S; seção 14) |
| `ferimento.` | Dano mínimo e chance de a mordida abrir ferimento, teto de ferimentos, sangria por segundo e duração (seção 17) |
| `saque.` | Alcance do E, distância mínima entre lugares de saque e quanto longe do começo eles ficam (seção 15) |
| `carro.` | Massa, suspensão, motor, freio, esterço, aderência e atropelo (unidades do Unreal) |
| `mundo.` | Cadáver, pedaços soltos, distância de sumir e de nascer, explosivos e carros nas ruas |

## `Zumbis.csv`: tipos de zumbi

`id,nome_pt,vida_cabeca,vida_tronco,vida_braco,vida_perna,dano_para_despedacar,velocidade_andar,velocidade_correr,`
`dano_ataque,intervalo_ataque_s,alcance_ataque_cm,explode,explosao_raio_interno_cm,explosao_raio_externo_cm,explosao_dano,`
`explosao_impulso,peso_dia,peso_noite,escala,cor`

- `explode` = 1: explode ao morrer (o Bujão também explode com tiro no bujão das costas).
- `peso_dia` e `peso_noite`: chance relativa de cada tipo nascer.
- `escala` vai de 0,3 a 3; `cor` é `#RRGGBB` da roupa.
- O código reconhece os ids `foliao` (vem em bloco, com fantasia), `bujao` (bujão nas costas), `inchado` e `brutamontes`
  para o visual, e `gritador` para o comportamento: a 25 m do jogador ele para, grita uma vez e chama 5 zumbis correndo em
  volta dele (`AZMBHordeDirector::CallHorde`).

## `Armas.csv`

`id,nome_pt,tecla,dano,tiros_por_s,automatica,pente,recarga_s,dispersao_graus,projeteis,alcance_m,impulso,barulho,municao_reserva`

- `tecla` (1 a 9) é única por arma.
- `projeteis` maior que 1 é escopeta (cada bago leva o `dano`).
- `barulho` chama zumbi pelo Diretor.

## `Explosivos.csv`

`id,nome_pt,vida,pavio_s,raio_interno_cm,raio_externo_cm,dano,impulso,barulho,cor`

O código usa os ids `bujao`, `barril`, `granada` e `carro`; se faltar algum, entra com os padrões. Em cadeia, o pavio fica
entre 25% e 60% do normal, para a sequência "pipocar".

## `Hype.csv`

`id,pontos,rotulo_pt,rotulo_en`

Uma linha por evento de `ZR::EHypeEvent`: `abate`, `tiro_na_cabeca`, `desmembrou`, `despedacou`, `abate_multiplo`,
`explosao_em_cadeia`, `atropelou`, `abate_mira_lenta`, `zumbi_do_chat`, `escapou_por_pouco`, `tiro_de_longe`, `apanhou`,
`calou_gritador` (matou o gritador antes do grito) e `abate_armadilha` (zumbi morto por armadilha da base).
O rótulo é o texto que salta na tela.

## `Pautas.csv`

`id,texto_pt,texto_en,evento,quantidade,prazo_s,recompensa_reais,hype_bonus,peso,so_de_noite`

- `evento` é um id de `Hype.csv`; `quantidade` vale pelo menos 1.
- `so_de_noite` = 1: a pauta só aparece à noite.
- O texto aparece no anúncio e na caixa da pauta no HUD.

## `Construcoes.csv`

`id,nome_pt,nome_en,tipo,tecla,custo_sucata,vida,largura_cm,profundidade_cm,altura_cm,dano_por_s,alvo,lentidao,desgaste,explosivo,cor`

- `tipo`: `barricada` (bloqueia) ou `armadilha` (o zumbi pisa).
- `tecla` (1 a 9) é única por peça e escolhe a peça no modo construção.
- `largura_cm` fica no eixo X sem girar; `profundidade_cm` no Y. A barricada até 1,2 m de altura dá para pular.
- Só armadilha: `dano_por_s` em cada zumbi em cima, no `alvo` `pernas` ou `corpo`; `lentidao` é a fração da velocidade em
  cima dela (1 = não segura); `desgaste` é o dano total que ela causa antes de gastar (0 = não gasta); `explosivo` é um id
  de `Explosivos.csv` e faz a armadilha explodir no primeiro zumbi. Armadilha sem nenhum dos três efeitos é erro.
- O código reconhece os ids `barricada`, `muro`, `estacas`, `arame` e as armadilhas com `explosivo` para o visual; id novo
  aparece como um bloco da cor da peça.
- Linha com erro sai inteira (não aparece no modo construção) e o erro vai para o log.

## `Sons.csv`

`id,arquivos,volume,variacao_tom,alcance_m,vozes_max,roubar,espera_s,laco,interface`

- `arquivos`: nomes sem extensão, separados por `;`, de `Audio/<nome>.wav` (só minúsculas, números e `_`: o nome vira o
  asset `SW_<nome>` no Unreal). Vazio = o som fica mudo.
- `variacao_tom`: o tom varia `1 ± isto`. `alcance_m`: além disso não toca. `vozes_max`: toques ao mesmo tempo.
  `roubar` = 1: com `vozes_max` cheio, o toque novo corta o mais antigo em vez de não tocar (tiros e explosão).
  `espera_s`: mínimo entre dois toques. `laco` = 1: toca sem fim, preso a algo (motor). `interface` = 1: sem posição.
- Os ids que o código toca estão em `ZR::RequiredSoundIds()`; o teste de deriva cobra uma linha para cada, e uma
  `tiro_<id>` para cada arma de `Armas.csv`.
- Som novo: skill `som-livre` (procurar, conferir a licença, tratar com `Tools/Audio/tratar_sons.py`, registrar).

## `FiltroPalavras.csv`

`termo,idioma,colado`

- Os termos do filtro do Modo Streamer (`regras-de-jogo.md`, seção 16): palavra inteira ou, com `*` no fim, começo de
  palavra. Acento, maiúscula e letra repetida não importam ("PORRA" repete "porra" e é erro). Termo com espaço é erro.
- `colado` = 1: pega também no meio do apelido ("xXporraXx"). Só para termo que não aparece dentro de nome comum: `cock`
  pegaria "Hancock", `viado` pegaria "Aviador". O teste de deriva tem uma lista de apelidos comuns que não podem cair.
- Prefixo curto pega palavra comum: `put*` pegaria "put" e `viad*` pegaria "viaduto". Nesses casos, liste as palavras
  inteiras. O teste de deriva acusa se o filtro pegar uma fala do chat simulado.
- `chat.filtro` = 0 em `Ajustes.csv` desliga o filtro.

## `Saque.csv`

`id,nome_pt,nome_en,onde,busca_s,rolagens,barulho,quantidade,renova_dias,largura_cm,profundidade_cm,altura_cm,cor`

- Os lugares de saque (`regras-de-jogo.md`, seção 15). `onde`: `calcada` (entre a rua e a fachada) ou `rua`.
- `busca_s`: segurando E. `rolagens`: sorteios na tabela de itens. `barulho`: vai para o Diretor ao terminar.
  `quantidade`: quantos na cidade (menos, se faltar espaço). `renova_dias`: 0 = revirado fica vazio.
- `largura_cm` é ao longo de para onde o lugar olha (a rua), `profundidade_cm` de lado; abaixo de 60 cm de altura dá para
  passar por cima. Id novo aparece como uma caixa da cor e do tamanho da linha.
- Linha com erro sai inteira e o erro vai para o log.

## `SaqueItens.csv`

`local,item,peso,min,max`

- `local`: um id de `Saque.csv`. `item`: `nada`, `municao` (pentes cheios de cada arma), `granada`, `bujao`, `curativo`
  (pontos de vida) ou `sucata`. A chance de cada linha é o `peso` dividido pela soma dos pesos do lugar.
- `min` e `max`: a quantidade sorteada (inteira). `min` maior que `max` é erro.

## `Marcos.csv`

`id,seguidores,texto_pt,texto_en,sucata,granadas,bujoes,caixa,hype_bonus`

- Os marcos de seguidores da live, em ordem **estritamente crescente** de `seguidores` (fora de ordem ou repetido é erro
  e a linha sai). Cada um paga uma vez (`regras-de-jogo.md`, seção 14).
- `texto_pt`/`texto_en`: o anúncio ("MIL SEGUIDORES"); o HUD mostra também como a próxima meta.
- `sucata`, `granadas`, `bujoes`: o presente vai direto para o jogador. `caixa` = 1: o chat manda uma caixa por drone.
  `hype_bonus`: soma no hype (até 100).

## `Creditos.txt`

O crédito que as licenças CC BY dos sons pedem. Vai junto no jogo empacotado (Zombie.Build.cs).

## `ChatFalso.csv`

`categoria,texto_pt,texto_en`

Falas do chat simulado. As categorias (`ZR::FakeChatCategories()`) são `inicio`, `abate`, `cabeca`, `explosao`, `cadeia`,
`atropelo`, `desmembrou`, `dano`, `quase_morte`, `calmo`, `doacao`, `horda`, `noite`, `mira_lenta`, `zumbi_do_chat`,
`barricada` (uma barricada caiu), `armadilha` (abate por armadilha), `marco` (meta de seguidores batida), `saque` (achou coisa boa revirando) e `sangrando` (abriu um ferimento), com
pelo menos 2 falas cada. Sem emoji: o HUD de protótipo não desenha emoji.

## Jogo empacotado

`Data/` fica na raiz do projeto e o jogo lê de `FPaths::ProjectDir()/Data`. No pacote, os `Data/*.csv` vão soltos, fora
do `.pak`, pelo `RuntimeDependencies` do `Zombie.Build.cs` (14/10/2026; falta conferir num pacote de verdade no PC). Sem
os arquivos, o jogo roda com os padrões do código e avisa no HUD.

## `Cidade_Malhas.csv`: malhas que trocam os blocos da cidade

`id,malha,escala,descricao`. O id é o que o código sabe trocar: `arvore`, `poste`, `caixa_dagua`, `ar_condicionado`, `lixeira`.
Várias linhas do mesmo id são variantes (sorteadas pela semente da cidade). `malha` é o caminho do asset e tem que começar
com `/Game/Externo/`: conteúdo da Epic (City Sample) fica só em `Content/Externo/`, fora do git. **Sem o asset no disco, o
jogo volta ao bloco do protótipo e avisa no log.** `escala` vai de 0,01 a 100. Linha com erro sai inteira e o teste de deriva
(`Deriva.CidadeMalhas...`) cobra.
