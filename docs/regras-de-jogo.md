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
- `PickRoadSpots`: células de rua distintas, longe do início do jogador (distância de Chebyshev maior que o raio).
- Testes: `Cidade.*` (borda, ruas no período, praia e mar, andares na faixa e determinismo, centro mais alto, **todo chão
  andável alcançável**, pontos de objeto).

## 3. Campo de fluxo da horda (`ZRFlowField.h`)

- Dijkstra na grade a partir do alvo (o jogador): custo 10 na ortogonal e 14 na diagonal. A **diagonal não corta quina**:
  as duas ortogonais precisam estar livres.
- Cada zumbi anda para a vizinha de menor custo. Um único cálculo serve a todos os zumbis. O Unreal recalcula quando o
  jogador muda de célula ou a cada 3 s.
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
