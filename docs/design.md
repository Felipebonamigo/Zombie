# AO VIVO — documento de design

Versão 0.1, 14 out 2026. Documento vivo: muda com cada playtest do Felipe.

**Título de trabalho:** AO VIVO (internacional: *LIVE OF THE DEAD*). Antes de anunciar, conferir se os dois nomes estão livres
na Steam, no INPI e no USPTO.

## Em uma frase

Um GTA de zumbis numa metrópole brasileira fictícia, em que você é um streamer transmitindo o fim do mundo **ao vivo**.
Quanto mais espetacular a sobrevivência, mais gente assiste. E o chat, de verdade, joga junto.

## Por que este jogo pode viralizar

A estratégia completa está em [`viralizar.md`](viralizar.md). Em resumo, o jogo é desenhado para produzir vídeo. Cada
pilar abaixo gera um tipo de clipe que se compartilha sozinho:

1. **Caos clipável:** bujões de gás em fila, reação em cadeia, zumbis em pedaços pela física e carro atropelando um bloco de
   Carnaval. O botão **Clipar** guarda o melhor lance dos últimos 20 segundos.
2. **O chat joga junto:** quem assiste digita `!zumbi` e um zumbi com o nome dele aparece para o streamer explodir. O chat
   também vota `!horda`, `!bujao` e `!caixa`. Streamer quer jogo que mexe o chat, e o chat chama mais gente.
3. **"Isso é real?":** a primeira pessoa é o celular preso no peito, com lente barata, grão e moldura de gravação. Foi esse
   visual de câmera corporal que viralizou o Unrecord e o Bodycam.
4. **Brasil no fim do mundo:** Carnaval, bujão de gás, trio elétrico, caixa d'água, orla e morro. É exótico para o mundo e
   íntimo para o maior público de jogos da América Latina.
5. **Junto é melhor:** co-op de 4 em que cada jogador tem a própria live, a "collab". Com 4 jogadores, saem 4 vídeos do
   mesmo caos.

## Referências: o que pegamos de cada uma

| Jogo | O que entra no AO VIVO |
|---|---|
| GTA V e VI | Cidade aberta e densa, carros para pegar na rua, caos sistêmico, câmera em terceira pessoa |
| Red Dead Redemption 2 | **Mira lenta** (o Dead Eye: Q desacelera o mundo e a mira continua rápida), ritmo e imersão, encontros na rua |
| Infestation (The War Z) | Sobrevivência numa cidade aberta, saque, zonas seguras, risco de perder o que carrega (modo difícil) |
| Night of the Dead | **Dia constrói, noite sobrevive:** base com barricadas e armadilhas, **Noite da Horda** a cada 3 noites |
| Left 4 Dead | **Diretor de IA** com tensão, pico e respiro; zumbis especiais; co-op de 4 |
| Dead Rising e Days Gone | Centenas de zumbis na tela, hordas que correm, armas improvisadas e humor |
| Unrecord e Bodycam | O realismo de câmera de celular que faz as pessoas perguntarem se é de verdade |
| Content Warning | O vídeo é o objetivo do jogo; quem joga já sai com o clipe pronto |

## Pilares

1. **Sobreviver de verdade:** munição contada, fôlego, vida que só volta até certo ponto, saque e, depois, comida, água,
   ferimentos e conserto.
2. **Caos espetacular:** explosões em cadeia, física em tudo, desmembramento e carros que voam.
3. **A live:** hype, espectadores, seguidores, doações que viram entrega por drone e o chat que vota.
4. **Dia e noite:** de dia se saqueia e se prepara; de noite a horda vem.
5. **Junto é melhor:** co-op de até 4.

## Porto Brasa

Metrópole litorânea fictícia, com um pouco de Rio, Salvador, Recife e Santos e nenhuma marca real. Tem a orla com a avenida
da praia, o centro de prédios altos e o morro (favela vertical com uma comunidade de sobreviventes organizada, nunca
retratada como cenário de crime). Também tem o porto, o sambódromo, o estádio e a ponte. A história começa no **sábado de
Carnaval**, quando o surto começa no meio dos blocos de rua.

O jogador é **Kaique "Brasa"**, motoboy e streamer pequeno (37 seguidores). Ele liga a live para mostrar o bloco e, sem
querer, transmite o começo do fim do mundo. Objetivo da campanha: virar **a maior live da história** antes de a cidade
cair, e descobrir quem está por trás do surto.

## O ciclo de jogo

- **Minuto a minuto:** explorar a pé ou de carro, encontrar a horda, resolver com criatividade (explosão, atropelo, tiro na
  cabeça na mira lenta) e ver o hype subir. Mais hype traz mais espectadores e doações, e as doações pagam drones.
- **Dia:** saque, pautas da live e reforço da base. As pautas já estão no protótipo: o patrocinador pede "explosão em
  cadeia 3 vezes em 2 minutos" e, se der certo, paga.
- **Noite:** defender a base ou fugir. A cada 3 noites vem a Noite da Horda.
- **Longo prazo:** seguidores liberam bairros, equipamentos, a base (o "estúdio") e sobreviventes que se juntam ao grupo.

## Câmera (pedido do Felipe)

- **Terceira pessoa de longe e do alto** é a padrão (9 m atrás, 3,8 m acima e 18° para baixo): dá para ver a horda chegando.
- **C** alterna entre longe e alta, média e perto (por cima do ombro). O botão direito puxa para a mira por cima do ombro.
- **V** liga a **primeira pessoa**, o "modo celular", com moldura de gravação, lente barata e grão. Serve para o clipe que
  parece real.
- Todas as distâncias ficam em `Data/Ajustes.csv` (`camera.*`).

## Zumbis

| Tipo | O que faz | Por que é divertido |
|---|---|---|
| Comum | Anda; à noite e na horda, corre | A massa da horda |
| Folião | Zumbi de bloco de Carnaval, com fantasia; vem em grupos de 2 a 4 | Visual único; atropelar o bloco inteiro |
| Corredor | Rápido e frágil (o Hunter do L4D) | Pressão e susto |
| **Bujão** | Carrega um bujão de gás nas costas: tiro no bujão ou morte = explosão | A estrela do jogo: fila de bujões vira reação em cadeia |
| Inchado | Lento, explode ao morrer e joga tudo longe | Kit de física: zumbis voando |
| Brutamontes | Ex-policial do choque com armadura | O "tanque" que pede plano ou explosivo |
| Rastejante | O zumbi que perdeu uma perna continua vindo, arrastando | Desmembramento com consequência |
| Gritador | A 25 m do jogador para e grita, chamando 5 zumbis correndo (o Screamer do Days Gone) | Prioridade de alvo: mate antes do grito |

Para depois:

- **Motoboy zumbi:** de moto.
- **Mergulhador:** sai do mar à noite.
- **Trio Elétrico:** chefe; o caminhão de som tomado, que atrai todas as hordas com a música.

## Armas, explosivos e veículos

- **Agora:** pistola, escopeta, fuzil, chute, granada e **bujão arremessável**. O lance-assinatura é jogar o bujão e
  acertar ele no ar. Também há bujões e barris pela rua e carros abandonados que explodem.
- **Depois:** facão, pé de cabra, extintor (que explode), molotov, arco e armadilhas de base.
- **Veículos:** carro de passeio (já dirigível). Depois vêm moto, Kombi, ônibus, caminhão de gás e o trio elétrico. O dano
  visual dos carros reaproveita o amassado do Detonauto.

## A live, como sistema (números em `regras-de-jogo.md`)

- **Hype (0 a 100):** sobe com os lances e cai sem ação. O combo multiplica. **Repetir o mesmo lance rende menos**, porque o
  chat cansa; isso empurra para a criatividade, que é o que vira clipe.
- **Espectadores** seguem o hype; com hype alto a live viraliza e passa do número de seguidores.
- **Seguidores** crescem com espectadores e hype; é a progressão de longo prazo.
- **Doações** chegam quando o hype passa de 50, 75 e 90. Cada R$ 50 paga uma **entrega por drone** (munição, granadas,
  bujões e vida).
- **Pautas do patrocinador:** desafios com prazo ("10 tiros na cabeça em 90 s") que pagam doação e dão hype. Elas empurram
  o jogador para o lance que vira clipe.
- **Chat:** sem Twitch, um chat simulado comenta tudo e também vota. Com Twitch (só o nome do canal, sem conta), o chat
  real manda zumbi com nome e vota a cada 30 s.
- **Clipar (B):** guarda a melhor janela de 20 s e tira um print com o HUD da live. Depois vira marcador na Gravação de
  Jogo da Steam e exportação vertical 9:16.

## Sobrevivência e base (próximas fases)

Comida e água sem microgerenciamento, ferimentos que sangram, conserto de arma e base com barricadas e armadilhas,
como no Night of the Dead. Haverá sobreviventes para resgatar, que viram moradores e dão missões, e uma zona segura com
troca. O modo difícil (Infestation) faz perder o que se carrega ao morrer.

## Modos

1. **História:** "A Maior Live do Mundo", campanha curta de 8 a 12 horas.
2. **Mundo livre:** sobrevivência sem fim.
3. **Co-op 4 (collab).**
4. **Horda sem fim:** arena para streamers, ótima para eventos de chat.
5. **Modo Streamer:** chat da Twitch ligado, sem música com direitos e filtro de palavras.

## Tom e conteúdo

- Violência só contra zumbis.
- O sangue tem opção "pouco" e "nenhum", para streamers em plataformas mais restritas. A classificação esperada é 18.
- Humor brasileiro no chat e nas situações, sem zombar de lugar ou grupo real. O morro é comunidade, não estereótipo.

## Escopo realista

GTA VI custou bilhões. O plano é chegar perto da **sensação** com uma pessoa e o Claude:

- Uma cidade **densa** em vez de gigante: um bairro no corte vertical e de 2 a 4 km² no acesso antecipado.
- Assets prontos de qualidade: City Sample, PCG, MetaHuman, o Game Animation Sample e o Mass para multidão.
- O ritmo vem do Diretor, não de conteúdo feito à mão.

Fases e marcos estão em [`roadmap.md`](roadmap.md).

## Monetização

Jogo pago (sugestão: R$ 59 ou US$ 19,99), com acesso antecipado e demo na Steam Next Fest. Sem pay-to-win. Mais tarde,
cosméticos da live (overlays, molduras, roupas) e cidades novas como DLC grande. Decisão final do Felipe.
