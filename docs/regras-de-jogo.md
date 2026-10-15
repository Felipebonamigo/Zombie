# Regras de jogo (contrato)

As regras puras moram em `Source/Zombie/Rules/` (namespace `ZR`). São C++ sem Unreal: compilam e são testadas na nuvem
com `Tools/Regras/testar.sh` (g++ e clang++, avisos como erro) e, no PC, como testes de Automation `Zombie.Regras.*`.
Cada seção diz onde está a regra, os números e os testes que a seguram. **Trocar uma decisão aqui = trocar o teste.**

Os números padrão estão no código e em `Data/Ajustes.csv` (que vence). Tabelas e colunas: [`dados.md`](dados.md).

## 1. Sorteio (`ZRRandom.h`)

SplitMix64. Mesma semente, mesma sequência em qualquer compilador. `RangeInt(Lo, Hi)` é inclusivo nos dois lados.
Testes: `Sorteio.*`.

## 2. Planta da cidade (`ZRCityPlan.h`)

- Grade de `Width x Height` células de 20 m (`cidade.celula_cm`). As linhas de terra vêm primeiro (Y = 0 a `LandHeight - 1`),
  depois `BeachRows` de praia e `SeaRows` de mar, ao sul.
- Na terra, toda coluna e linha múltipla de `BlockCells + 1` é rua, e a borda também.
- O resto é prédio. **Praça é o quarteirão inteiro** (chance `PlazaChance` por quarteirão), porque uma célula de praça
  cercada de prédios ficaria inalcançável. O teste de conectividade achou isso.
- Andares: `Min + (Max - Min) x proximidade^1,5 x sorteio(0,6 a 1,0)`, com proximidade 1 no centro e 0 na borda.
- **Recuo da fachada** de cada prédio, sorteado entre `cidade.recuo_min_cm` e `cidade.recuo_max_cm` (2,5 a 4,2 m) num sorteio
  à parte (a mesma semente dá as mesmas ruas). O prédio é o quadrado da célula encolhido pelo recuo; a faixa em volta é a
  calçada e metade do beco. **Calçada e beco são chão** para andar e construir (`IsWalkablePoint`, `IsWalkableRect`); a
  borda da fachada também. O campo de fluxo da cidade continua por célula inteira.
- `PickRoadSpots`: células de rua distintas, longe do início do jogador (distância de Chebyshev maior que o raio).
- Testes: `Cidade.*` (borda, ruas no período, praia e mar, andares na faixa e determinismo, centro mais alto, **todo chão
  andável alcançável**, pontos de objeto).

## 3. Campo de fluxo da horda (`ZRFlowField.h`)

- Dijkstra na grade a partir do alvo (o jogador): custo 10 na ortogonal e 14 na diagonal. A **diagonal não corta quina**:
  as duas ortogonais precisam estar livres.
- Cada zumbi anda para a vizinha que sai mais barato: o custo restante dela mais o passo mais o custo extra de entrar
  nela. Um único cálculo serve a todos os zumbis. O Unreal recalcula quando o jogador muda de célula ou a cada 3 s.
- **Custo extra** (opcional, por célula): entrar na célula custa também o extra dela. É como a barricada vira "cara, mas
  passa" (seção 12).
- Alvo bloqueado deixa o campo inválido; o zumbi vai direto no jogador.
- Testes: `Fluxo.*`.

## 4. Corpo, desmembramento e explosão (`ZRBody.h`)

| Membro | Vida (comum) | Ao zerar |
|---|---|---|
| Cabeça | 40 | sai e **mata** (tiro na cabeça) |
| Tronco | 150 | **mata**, não sai |
| Braço (cada) | 50 | sai; sem os dois, a mordida cai à metade |
| Perna (cada) | 70 | sai; a primeira perna faz virar **rastejante** (30% da velocidade) |

- Tiro em braço, perna ou cabeça manda 25% do dano também para o tronco (`TorsoTransfer`).
- Tiro em membro que já saiu não faz nada no corpo (o pedaço solto só é empurrado). Cadáver ainda desmembra, sem contar abate.
- Explosão: dano maior ou igual a `GibDamage` (220 no comum) **despedaça** o corpo todo. Abaixo disso, cada membro leva
  `dano x BlastShare` (cabeça 0,6; o resto 0,8; tronco 1,0).
- Queda pela distância: dano cheio até o raio interno, linear até zero no externo. Raios iguais viram degrau.
- Testes: `Corpo.*` e `Explosao.*`.

## 5. Hype da live (`ZRHype.h`)

| Regra | Padrão |
|---|---|
| Combo: lances a até 4 s um do outro; multiplicador `1 + 0,25 x (combo - 1)` | teto x4 |
| Variedade: o mesmo lance seguido rende `0,75^repetições` | mínimo 0,3 |
| Queda: depois de 5 s sem lance, cai 4 pontos/s (só a parte depois da carência) | |
| Espectadores alvo: `seguidores x (0,05 + 0,95 x h^1,5) x (1 + 2 x viral)`, com viral = 0 até hype 80 e 1 no hype 100 | |
| Espectadores seguem o alvo: sobem em 3 s, descem em 12 s (exponencial) | |
| Seguidores: `+ espectadores x 0,01 x h x (1 - seguidores / teto) por segundo` (logístico) | começa em 37; teto 10 milhões |
| Doação: limiares 50, 75 e 90; cada um doa **uma vez por subida** e rearma quando o hype cai 20 abaixo dele | R$ = espectadores x 0,02 x n |

Pontos por lance ficam em `Data/Hype.csv`. A ordem e os ids são os do enum `ZR::EHypeEvent`; um teste de deriva exige
todos. Cada R$ 50 doados pagam uma entrega por drone (no Unreal: `UZMBStreamSubsystem`).
Testes: `Hype.*` e `Deriva.TodoEventoDeHypeTemLinha`.

## 6. Diretor da horda (`ZRDirector.h`)

O ciclo do Left 4 Dead: **tensão → pico → respiro**.

- Intensidade (0 a 100) = dano levado x 0,6 + abates por perto x 1,5 + explosões por perto x 4; cai 3/s.
- Tensão vira pico com intensidade 70 ou após 90 s. O pico dura 25 s e vira respiro. O respiro dura no mínimo 20 s e só
  acaba com intensidade até 25.
- Nascimentos por segundo: tensão 0,6; pico 2,5; respiro 0,05.
  - À noite: x2,5.
  - Na horda: ritmo do pico x4.
  - Barulho: + barulho x 0,02.
- O barulho cai 10/s.
- Acumulador: nasce a parte inteira; o que passa do teto de vivos (80 de dia, 140 à noite) **é descartado**, não guardado.
- Testes: `Diretor.*`.

## 7. Dia e noite (`ZRDayNight.h`)

- Um dia de 24 h dura 24 min reais (`dia.duracao_s`) e começa às 17 h, na luz dourada.
- A noite vai das 19h30 às 5h30 e atravessa a meia-noite.
- Altura do sol: `72° x sen(pi x (hora - 6) / 12)`; zero às 6 h e às 18 h.
- **Noite da Horda:** a noite de índice n (0, 1, 2...) é da horda quando `(n + 1) % 3 == 0`, ou seja, a 3ª, 6ª, 9ª...
  Começa uma horda de 90 s.
- Testes: `DiaNoite.*`.

## 8. Clipes (`ZRHighlights.h`)

Cada lance entra com a nota (os pontos de hype) e o rótulo. `BestWindow(agora, 20 s)` acha a janela de até 20 s com a
maior soma de notas; o rótulo é o do maior lance dela. Momentos com mais de 60 s são esquecidos.
Testes: `Clipe.*`.

## 9. Chat (`ZRChat.h`)

- **IRC da Twitch:**
  - quebra o pacote em linhas (`\r\n` ou `\n`);
  - lê tags IRCv3 com escapes (`\:` vira `;`, `\s` vira espaço, `\\`, `\r` e `\n`), prefixo, comando, canal e texto;
  - `PING` é respondido com `PONG`.
- **Nome seguro:** usa o display-name se tiver só `[A-Za-z0-9_]`, senão o apelido. Só esses caracteres entram, no máximo 25.
- **Comandos** (a primeira palavra, sem diferenciar maiúsculas): `!zumbi` ou `!zombie`; `!horda` ou `!horde`; `!bujao`,
  `!bujão`, `!bomb` ou `!bomba`; `!caixa`, `!airdrop` ou `!drop`.
- **Votação:** rodadas de 30 s; um voto por pessoa por rodada; vence o mais votado com no mínimo 3 votos. No empate vence
  quem chegou primeiro àquela contagem.
- **`!zumbi`:** espera de 60 s por pessoa; no máximo 12 zumbis com nome vivos (`chat.zumbis_com_nome_max`).
- **Chat simulado:** escolhe uma fala da categoria sem repetir a anterior. As categorias estão em
  `ZR::FakeChatCategories()` e são exigidas por teste de deriva.
- Testes: `Chat.*`, `Voto.*` e `ChatFalso.*`.

## 10. Tabelas (`ZRCsv.h`, `ZRData.h`)

CSV no padrão RFC 4180: aspas, vírgula e quebra de linha dentro de aspas, BOM do Excel. Número com ponto ou vírgula
decimal; cor `#RRGGBB`.

**Erro de dado nunca derruba o jogo.** Chave desconhecida, repetida ou valor fora da faixa vira erro no log e no HUD, e
vale o padrão do código.

Os testes de deriva (`Deriva.*`) carregam as tabelas de verdade de `Data/` e falham com qualquer erro ou chave faltando.
Testes: `Csv.*`, `Dados.*` e `Deriva.*`.

## 11. Pautas da live (`ZRChallenges.h`)

Desafios de patrocinador com prazo, em `Data/Pautas.csv` (por exemplo "explosão em cadeia 3 vezes em 120 s, paga R$ 60").

- **Quando vêm:** a primeira chega 45 s depois do começo; as seguintes, 40 s depois de a anterior acabar, cumprida ou não.
  Só com pelo menos 10 espectadores.
- **Qual vem:** sorteio pelo peso entre as elegíveis (as "só de noite" só à noite), sem repetir a anterior quando há outra.
  Sem nenhuma elegível, tenta de novo em 5 s.
- **Progresso:** conta só o evento de hype da pauta, somando a quantidade de cada lance (um abate múltiplo de 4 conta 3).
- **Cumpriu dentro do prazo:** a doação da pauta entra no dinheiro (R$ 50 pagam um drone), o hype ganha o bônus e o lance
  vira momento de clipe.
- **Estourou o prazo:** só passa a vez.

Testes: `Pauta.*`, `Dados.PautaComEventoDesconhecidoEhErro` e `Deriva.PautasCarregam`.

## 12. Base: sucata, barricadas e armadilhas (`ZRBase.h`)

O "dia constrói, noite sobrevive" do Night of the Dead. Peças em `Data/Construcoes.csv`; números soltos em `base.*` de
`Data/Ajustes.csv`.

- **Sucata:** 60 no começo, 40 em cada caixa do drone e 25 por carro desmanchado (segurar E ao lado dele por 2,5 s; tomar
  dano, sair andando ou o carro pegar fogo cancela).
- **Onde dá para construir** (o primeiro motivo que falha é o que aparece): ter a sucata; ter vaga (até 40 peças); o centro
  a até 7 m do jogador; a pegada inteira no chão: rua, praça, praia, calçada ou beco, sem entrar no prédio (encostar na
  fachada pode; a quina do prédio dentro da peça não pode); a pelo menos 50 cm do
  jogador; sem sobrepor outra peça (encostar pode, para fazer muro). A peça encaixa numa grade de 50 cm e gira de 90 em
  90 graus. No jogo, poste, árvore, carro, bujão de rua ou zumbi dentro da pegada também impedem (o Unreal confere; a
  regra pura não vê o mundo).
- **Barricada:** segura zumbi e jogador. A de madeira tem 1,1 m e o jogador pula; o zumbi não pula. O zumbi colado nela
  na direção do jogador não morde por cima: quase parado (abaixo de 30% da velocidade dele), para e bate (o ataque dele
  vezes `base.dano_zumbi_multiplicador`) até ela cair; quem chega de lado escorrega e dá a volta. Explosão tira o dano dela vezes `base.dano_explosao_multiplicador`.
  Carro acima de `carro.atropelo_min_kmh` (contando só a velocidade contra a peça: raspar de lado não derruba) tira
  `km/h x carro.atropelo_dano_por_kmh x base.dano_carro_multiplicador`; se ela cai, o carro passa com três quartos da
  velocidade (de frente, a madeira cai acima de ~35 km/h e o muro acima de ~100). Cada peça leva uma batida por vez;
  duas lado a lado levam uma cada no mesmo choque.
  Estágios para o visual: inteira (acima de 2/3 da vida), avariada (acima de 1/3), quase caindo, quebrada.
- **A horda contorna a base:** com barricada a até 48 m do jogador, o Diretor faz uma grade fina de 64 x 64 m (células de
  1 m) em volta dele. Prédio, mar e fora da planta bloqueiam (a calçada e o beco são chão). A barricada cresce 40 cm (o
  raio do zumbi) e marca as células com o centro dentro dela; o preço se divide pela espessura em células, então
  **atravessar uma barricada custa 400** (o mesmo que andar 40 m), encaixada onde for. Um vão de 1,8 m ou mais sempre
  deixa passagem. O zumbi dentro da grade vai direto quando a reta até o jogador não passa por prédio nem barricada;
  senão, segue o campo mirando dois passos à frente: se a abertura da base fica a menos de 40 m de desvio, ele dá a volta
  e entra por ela (onde ficam as armadilhas); se não, vem e bate. Fora da grade, segue a rua da cidade; sem barricada
  perto, nada muda. A grade é refeita quando uma barricada muda, a cada 0,5 s ou quando o jogador anda 2 m (no máximo
  uma vez a cada 0,15 s). Números em `base.horda_*`.
- **Armadilha:** o jogador e a bala passam. Cada zumbi em cima leva `dano_por_s` (nas pernas ou no corpo) e anda na fração
  `lentidao` da velocidade. A armadilha gasta com o dano que causa (`desgaste`; 0 = não gasta). O abate conta para a live
  (`abate_armadilha`). O bujão armado explode no primeiro zumbi que pisa, no tiro ou na explosão vizinha, com pavio curto
  (entra na cadeia).
- **Consertar:** custa `preço x base.conserto_custo_fracao x vida que falta / vida cheia`, arredondado para cima; com pouca
  sucata, gasta toda e conserta a parte proporcional. Peça quebrada não conserta.
- **Desmontar:** devolve `preço x base.devolve_fracao x estado`, arredondado para baixo. O estado é o pior entre a vida
  e o que sobra do desgaste (armadilha gasta vale menos); é também o que o HUD mostra.
- **Barulho:** construir uma peça faz barulho (`base.barulho_construir`) e desmanchar carro, o dobro: chama zumbi pelo
  Diretor.

Testes: `Base.*`, `GradeFina.*`, `Fluxo.BarricadaCaraFazAHordaDarAVolta`, `Fluxo.SemDesvioAHordaVemBaterNaBarricada`,
`Dados.ConstrucaoComErroEhApontada` e `Deriva.ConstrucoesCarregamEFazemSentido`.

## 13. Som (`ZRSound.h`)

Cada som do jogo é uma linha de `Data/Sons.csv` com um ou mais arquivos (as variações) de `Audio/`. A cada toque:

- **Variação:** sorteada entre os arquivos, sem repetir a anterior (um som só, sempre ele).
- **Tom:** `1 ± variacao_tom`, sorteado.
- **Não toca** quando: o som não tem arquivo; o ouvinte está além do `alcance_m` (som de interface não tem distância);
  `vozes_max` toques dele já tocam; ou não passou `espera_s` desde o último toque dele. Relógio que voltou (fase
  recomeçada) libera a espera.
- **Rouba a voz** (`roubar` = 1: os tiros e a explosão): com `vozes_max` cheio, o toque novo toca e o Unreal corta o mais
  antigo dele, que já está na cauda. A rajada do fuzil (9 tiros/s, arquivos de ~1,3 s) não perde tiro e a cadeia de
  bujões não perde a última explosão.
- **O ouvinte** é o personagem (ou o carro que ele dirige) para a distância e a atenuação, e a câmera para a direção. A
  câmera fica a ~10 m do personagem: medindo dela, a recarga, o chute e a mordida no jogador sumiriam, e a mistura mudaria
  ao trocar de câmera.
- O Unreal toca com a atenuação do alcance (volume cheio até 5% dele, queda natural até -30 dB no fim dele), preso a nada
  (som solto), preso a um objeto (o chiado do pavio anda com a granada e com o zumbi-bujão e para na explosão), em laço
  preso ao carro (o motor: o volume da tabela vezes 0,6 a 1,0 pelo acelerador) ou sem posição (interface).
- Mira lenta: o som do mundo desce para 0,65 do tom enquanto ela dura.

Quem toca o quê: tiro (`tiro_<id da arma>`), clique sem munição (um a cada 0,3 s com o gatilho apertado), recarga, chute,
impacto no corpo (bala e chute), explosão (volume pelo raio), chiado do pavio, gemido do zumbi (a cada 4 a 11 s, sorteado
por zumbi), ataque, morte, membro arrancado, grito do gritador, pancada e quebra da barricada, martelada de construir e
consertar, desmonte e desmanche, motor e batida do carro, a caixa do drone, e na live a doação, a pauta nova, a pauta
cumprida, o marco de seguidores e o clipe.

Testes: `Som.*`, `Dados.SomComErroEhApontado` e `Deriva.SonsTemOsQueOCodigoTocaEOsArquivosSaoWavMono48k` (todo id do
código tem linha, toda arma de `Armas.csv` tem o seu `tiro_<id>`; todo arquivo é WAV PCM 16 bits mono 48 kHz, até 6 s, ou
12 s em laço).

## 14. Marcos de seguidores e resumo da live (`ZRLive.h`)

**Marcos** (`Data/Marcos.csv`): 50, 100, 250, 500, 1.000, 5.000, 10.000, 100 mil e 1 milhão de seguidores. Cada um tem
um prêmio do chat: sucata, granadas, bujões, uma caixa por drone e hype.

- Paga **uma vez só**, na ordem dos seguidores. Vários cruzados no mesmo passo pagam todos, do menor para o maior.
- Os que já estavam passados no começo da live não pagam (com 37 seguidores no começo, o primeiro é o de 50).
- Ao bater: anúncio no meio da tela, fala do chat (`marco` em `ChatFalso.csv`), som de pauta cumprida, o hype do marco e
  um lance para o clipe. O HUD mostra embaixo dos seguidores a próxima meta e quanto falta.

**Resumo da live:** a live inteira (não zera quando o jogador cai e volta) guarda tempo ao vivo, pico de espectadores,
seguidores ganhos, abates e tiros na cabeça, maior combo, maior cadeia de explosões (uma explosão sozinha é 1), doações
(R$), pautas cumpridas e perdidas, clipes e quedas. Quando a live cai, a tela "A LIVE CAIU" mostra o resumo e a **nota**:

| Pico de espectadores | Nota |
|---|---|
| `resumo.nota_s_espectadores` (2.000) ou mais | S |
| `resumo.nota_a_espectadores` (500) | A |
| `resumo.nota_b_espectadores` (100) | B |
| `resumo.nota_c_espectadores` (25) | C |
| abaixo | D |

O jogador volta depois de `jogador.renascer_s` (8 s), o tempo de ler o resumo. A tela é pensada para print: é o "card"
que o jogador posta.

Testes: `Marco.*`, `Resumo.*`, `Dados.MarcoForaDeOrdemOuRepetidoEhErro` e `Deriva.MarcosCarregamEOPrimeiroEhAlcancavel`
(a tabela carrega e o primeiro marco fica acima dos seguidores do começo).
