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
| `cidade.` | Tamanho da cidade, quarteirões, praia e mar, praças, andares e tamanho da célula (seção 2) |
| `chat.` | Rodada de votação, votos mínimos, espera do `!zumbi` e zumbis com nome ao mesmo tempo (seção 9) |
| `dia.` | Duração do dia, hora inicial, noite e Noite da Horda (seção 7) |
| `camera.` | As três distâncias da terceira pessoa, a mira, a primeira pessoa e o modo inicial |
| `jogador.` | Vida, velocidades, fôlego, chute, mira lenta (foco), arremesso, sobra de vida e cura sozinha |
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
  para o visual.

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
`explosao_em_cadeia`, `atropelou`, `abate_mira_lenta`, `zumbi_do_chat`, `escapou_por_pouco`, `tiro_de_longe` e `apanhou`.
O rótulo é o texto que salta na tela.

## `ChatFalso.csv`

`categoria,texto_pt,texto_en`

Falas do chat simulado. As categorias (`ZR::FakeChatCategories()`) são `inicio`, `abate`, `cabeca`, `explosao`, `cadeia`,
`atropelo`, `desmembrou`, `dano`, `quase_morte`, `calmo`, `doacao`, `horda`, `noite`, `mira_lenta` e `zumbi_do_chat`, com
pelo menos 2 falas cada. Sem emoji: o HUD de protótipo não desenha emoji.

## Jogo empacotado

`Data/` fica na raiz do projeto. Para o jogo empacotado, a pasta ainda precisa entrar em
`DirectoriesToAlwaysStageAsNonUFS` (pendente, como no Detonauto). Sem os arquivos, o jogo roda com os padrões do código e
avisa no HUD.
