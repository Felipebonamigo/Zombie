# Roteiro

Versão 0.1, 14 out 2026. Cada fase termina num **portão**: o que precisa estar verdadeiro para seguir. As datas dependem
das horas semanais do Felipe (pendência do estúdio) e do Detonauto, que começa em tempo integral em 02/11/2026.

## Fase 0 — Protótipo jogável (agora)

- [x] Design, estratégia viral e contrato das regras (`design.md`, `viralizar.md`, `regras-de-jogo.md`)
- [x] Regras puras com 68 testes passando na nuvem (76 com o bloco 2, 91 com o bloco 3, 105 com o bloco 4, 112 com o bloco 5, 118 com o bloco 6, 119 com a revisão do som, 126 com o bloco 7, 132 com o bloco 8) (g++ e clang++, avisos como erro,
  sanitizers)
- [x] Projeto Unreal 5.8 em C++ escrito:
  - cidade gerada, jogador com 4 câmeras, 3 armas, granada, bujão, chute e mira lenta;
  - 6 tipos de zumbi com desmembramento;
  - explosões em cadeia;
  - carro dirigível;
  - Diretor de horda com dia e noite e Noite da Horda;
  - a live (hype, chat simulado, votação, doações, drones, clipe) e o chat da Twitch.
- [x] Bloco 2: pautas do patrocinador (desafios com prazo e recompensa) e o zumbi gritador
- [x] Bloco 3, adiantado da fase 4: a base do Night of the Dead. Sucata (caixa do drone e carro desmanchado), modo
  construção (X) com barricada de madeira, muro de sucata, estacas, arame farpado e bujão armado; o zumbi travado bate na
  barricada até quebrar; conserto e desmonte
- [x] Bloco 5, adiantado da fase 1: som. 25 sons (65 arquivos CC0 e CC BY) com variação, tom, vozes e alcance pela
  tabela `Data/Sons.csv`; motor do carro em laço; mira lenta grave; importação por script no PC
- [x] Bloco 4: a horda contorna a base (campo fino de 1 m em volta do jogador, barricada cara mas passável) e entra pela
  abertura; o carro rápido arromba barricada; `Data/` vai no jogo empacotado
- [x] Bloco 6: marcos de seguidores (`Data/Marcos.csv`, com o presente do chat) e o resumo da live com nota de S a D
  na tela "A LIVE CAIU"
- [x] Bloco 7, adiantado da fase 4: saque. Lixeira, mochila de folião, isopor do ambulante e viatura largada espalhados
  pela cidade (`Data/Saque.csv` e `SaqueItens.csv`); segurar E revira, faz barulho e rende munição, granada, vida ou sucata
- [x] Bloco 8, adiantado da fase 5: filtro de palavras do Modo Streamer (`Data/FiltroPalavras.csv`, `chat.filtro`) no
  texto, no apelido e no nome do `!zumbi` vindos da Twitch
- [ ] **Compilar no PC** (skill `verificar`) e corrigir o que o compilador apontar
- [ ] `Zombie.Regras` verde no Unreal (os mesmos testes da nuvem; 132 hoje)
- [ ] Capturas de dia, de noite e no modo celular (`jogar.ps1`) em `docs/img/prototipo/`
- [ ] **Primeiro playtest do Felipe** (15 min) e lista de problemas como Issues "[Playtest]"

**Portão:** o Felipe joga 15 minutos e quer jogar de novo. A câmera de longe é boa, explodir a fila de bujões é
satisfatório e a live dá vontade de subir o hype.

## Fase 1 — Sensação (2 a 3 meses)

- Personagem real com Motion Matching (Game Animation Sample); armas com animação, recuo e som.
- Zumbi esquelético: andar, correr, rastejar, morder e cair; desmembramento por osso; sangue em Niagara.
- Explosão em Niagara; som das armas e explosões em MetaSounds, sobre os sons do bloco 5 (skill `som-livre`).
- Mira lenta com som abafado e rastro de bala.
- **Portão:** capturas aprovadas na cena de aparência e 60 fps no perfil alvo com 80 zumbis.

## Fase 2 — Um bairro de verdade (2 a 3 meses)

- Mapa com World Partition: a orla e um pedaço do centro (~0,5 km²), feitos com City Sample + PCG.
- Interiores simples para saquear (loja, farmácia, posto de gasolina).
- Trailer-teaser de 30 s; página da Steam; Discord.
- **Portão:** um minuto de vídeo "isso é real?" gravado no modo celular.

## Fase 3 — Horda em massa (1 a 2 meses)

- Mass Entity para os zumbis distantes (meta: 500 visíveis), com troca para ator completo perto.
- Noite da Horda de verdade; o Gritador.
- **Portão:** 500 zumbis na tela a 60 fps no perfil alvo.

## Fase 4 — Sobrevivência e base (2 a 3 meses)

Saque, comida e água, ferimentos, sobreviventes resgatados e progressão por seguidores. Já adiantados no protótipo: o
saque na rua (bloco 7; falta o de dentro dos prédios), os marcos de seguidores (bloco 6), a
base com barricadas e armadilhas (blocos 3 e 4: a horda contorna a base e entra pela abertura; o carro arromba
barricada) e as pautas da live (bloco 2). Falta na base: portas e janelas de prédio para trancar e peças de verdade
(acervo-3d).

## Fase 5 — Live 2.0 (1 mês, em paralelo)

Marcadores da Steam Timeline, exportação vertical 9:16 do clipe, replay de 10 s, YouTube Live e TikTok LIVE, e o Modo
Streamer.

## Fase 6 — Co-op de 4 (2 a 3 meses)

Decisão de arquitetura antes (autoridade do dano, da física e do Diretor). Inclui listen server, Steam e a collab com uma
live por jogador.

## Fase 7 — Demo e acesso antecipado

Demo na Steam Next Fest, campanha com streamers e acesso antecipado (de preferência no Carnaval).

## Decisões que são do Felipe

1. **Nome:** AO VIVO / LIVE OF THE DEAD, ou outro.
2. **Repositório público ou privado:** se for público, nada da Epic nem do Mixamo no git, como no acervo.
3. **Horas por semana** para este jogo, ao lado do Detonauto.
4. **Preço e modelo:** jogo pago, acesso antecipado.
5. **Classificação e gore:** quanto sangue e se haverá opção "sem sangue".
