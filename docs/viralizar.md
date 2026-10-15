# Como o AO VIVO viraliza

Versão 0.1, 14 out 2026.

**A aposta: um jogo que fabrica vídeo.** Jogos que estouraram em 2023–2026 tinham momentos que as pessoas gravam sem
pensar. Exemplos: Lethal Company, Content Warning, R.E.P.O., Peak, o trailer do Unrecord e o Schedule I. O AO VIVO junta os
três motores mais fortes desse tipo de sucesso:

- caos físico clipável;
- interação com o chat de quem transmite;
- realismo de câmera de celular.

Por cima de tudo, uma ambientação que ninguém fez: o Brasil no Carnaval do fim do mundo.

## 1. Gancho de 3 segundos

Todo vídeo curto (TikTok, Reels, Shorts) começa pelo mesmo tipo de cena:

- uma **fila de bujões explodindo em cadeia** e zumbis voando em pedaços;
- **um zumbi com o nome de um espectador** sendo atropelado;
- um clipe no **modo celular** que parece filmagem real ("isso é um jogo?").

O jogo é desenhado para que esses lances aconteçam sozinhos. O primeiro minuto já tem um caminhão de gás descarregado no
meio da rua, a 25 m de onde o jogador nasce (`AZMBWorldBuilder::SpawnProps`).

## 2. Recursos do jogo que geram conteúdo

| Recurso | Estado | Por que espalha |
|---|---|---|
| **Clipar (B)** | Protótipo: marca a melhor janela de 20 s e tira um print com o HUD da live | Quem joga já sai com o lance pronto |
| Marcadores na Gravação de Jogo da Steam (Steam Timeline) | Planejado | A Steam grava e exporta o clipe, sem programa extra |
| Exportação vertical 9:16 com o HUD da live | Planejado | Formato nativo de TikTok, Reels e Shorts |
| **Modo celular** (primeira pessoa com lente e grão) | Protótipo | O "isso é real?" do Unrecord e do Bodycam |
| HUD de live (AO VIVO, espectadores, chat) | Protótipo | O vídeo já parece uma live viral |
| Física e desmembramento | Protótipo (blocos); final com ossos e Niagara | Cada partida é diferente |

## 3. Primeiro para streamers

Streamer grande jogando = milhões vendo. O AO VIVO é feito para a live deles render mais.

- **Chat da Twitch sem configurar nada:** basta o nome do canal (`-ZMBTwitch=canal` ou `zmb.twitch canal` no console).
  A leitura é anônima, sem conta nem chave (`Stream/ZMBTwitchChat`).
- **`!zumbi`:** nasce um zumbi com o nome de quem digitou. Todo espectador quer ver o "seu" zumbi explodir, e isso enche o
  chat.
- **Votação a cada 30 s** (`!horda`, `!bujao`, `!caixa`): o chat sente que manda na live.
- **Modo Streamer:** sem música com direitos autorais, com filtro de palavras (no protótipo desde o bloco 8: texto,
  apelido e nome do `!zumbi` passam pelo filtro) e sangue ajustável (depende da decisão sobre o gore).
- Depois: YouTube Live e TikTok LIVE, Twitch Drops (cosméticos) e uma extensão da Twitch com placar do chat.

## 4. O Brasil como vantagem

- O Brasil tem um dos maiores públicos de jogos e de lives do mundo. Um jogo **brasileiro, sobre o Brasil, em português**
  ganha atenção da imprensa e dos criadores daqui de graça.
- Para fora, o Carnaval com zumbis, o trio elétrico e o bujão são imagens que ninguém viu num jogo realista.
- **Lançar (ou abrir a demo) no Carnaval** coloca o jogo junto do assunto do momento no país.

## 5. A história do desenvolvimento também viraliza

"Um desenvolvedor brasileiro sozinho, com IA, fazendo um GTA de zumbi" é uma história que as pessoas assistem. Plano:

- devlog curto semanal (TikTok, Shorts e Reels) com o antes e depois e as explosões novas;
- a página da Steam no ar cedo, para juntar listas de desejos;
- convites para playtest no Discord.

Transparência sobre IA: declarar na Steam o conteúdo gerado por IA (skill `declaracao-ia`) e mostrar que a direção, o
design e o jogo são do Felipe.

## 6. Calendário sugerido

| Quando | O quê |
|---|---|
| Agora | Protótipo jogável (este repositório); primeiros vídeos de bastidor |
| +2 a 3 meses | Visual realista num quarteirão (lookdev) e trailer-teaser de 30 s; página da Steam e Discord |
| +4 a 6 meses | Corte vertical: um bairro, horda em massa e a live completa; playtest fechado com streamers pequenos |
| Steam Next Fest seguinte | Demo pública com o chat da Twitch; campanha com streamers BR médios |
| Carnaval | Acesso antecipado ou grande atualização temática |

## 7. Como medir

Listas de desejos (meta: 7 mil para aparecer em "Populares em breve"), taxa de clique da cápsula, número de clipes salvos
por sessão (telemetria opcional), quantos streamers ligaram o chat e as visualizações do devlog.

## 8. Riscos e cuidados

- **Marcas reais:** nenhuma em fachada, veículo ou interface, como no Detonauto.
- **Retrato de lugares e grupos:** o morro é comunidade, não estereótipo; texto sobre o mundo real passa pela skill
  `conferir-fato`.
- **Twitch:** a leitura anônima do IRC é permitida. Comandos que **escrevem** no chat exigiriam conta e aprovação, e ficam
  fora.
- **Nomes de espectadores na tela:** só letras, números e `_`, no máximo 25 caracteres (`ZR::SafeViewerName`); na versão
  final, com filtro de palavras.
- **Música:** nada gerado por IA nem com direitos de terceiros; a live tem que poder ser transmitida sem bloqueio.
- **"Feito com IA":** declarar e manter a qualidade alta. O público perdoa a ferramenta, não o resultado ruim.
