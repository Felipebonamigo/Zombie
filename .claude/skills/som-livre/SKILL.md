---
name: som-livre
description: 'Procura, confere a licença, trata, importa e registra efeito sonoro do AO VIVO (Zombie), na mesma linha da skill modelo-3d. Use antes de baixar, gravar ou gerar qualquer som, quando o Felipe pedir "põe um som de...", "troca o som do tiro", "falta áudio" ou "está mudo", e sempre que um som novo entrar no jogo.'
---
# Som livre: procurar, conferir, tratar, registrar

Regra do AO VIVO (CLAUDE.md): só CC0, CC BY com crédito (e OGA-BY, que é o CC BY do OpenGameArt) e o que o Felipe gravar.
**Música gerada por IA é proibida.** Efeito sonoro por IA só com licença comercial clara e o Felipe sabendo; marcado
em `gerado_por_ia` para a declaração da Steam (skill `declaracao-ia`).

## Como o som funciona no jogo

- `Audio/<id>_<nn>.wav`: o arquivo tratado (mono, 48 kHz, PCM 16 bits), no Git LFS.
- `Data/Sons.csv`: cada som (o id que o código toca) com os arquivos (variações, separadas por `;`), volume, variação de
  tom, alcance, vozes ao mesmo tempo, espera entre toques, laço e interface. Regras em `Rules/ZRSound.h`
  (`docs/regras-de-jogo.md`, seção 13). Os ids que o código toca estão em `ZR::RequiredSoundIds()`.
- No PC, `Tools\Unreal\importar_sons.ps1` (editor fechado) importa `Audio/*.wav` como `/Game/Zombie/Sons/SW_<nome>`; o
  jogo carrega por esse caminho (`Audio/ZMBAudioSubsystem`). Som não importado fica mudo e avisa no log.

## Onde procurar, nesta ordem

1. O que o jogo já tem: `Data/Sons.csv` e `docs/licencas.csv`.
2. **OpenGameArt** (busca avançada, tipo "Sound Effect", licença CC0): confira a licença **na página de cada item**, não a
   do pacote. Página com várias licenças: escolha a CC0 ou CC BY e anote qual.
3. **Kenney** (pacotes de áudio CC0).
4. **The Free Firearm Sound Library** (CC0, ~194 MB): a melhor fonte de tiros realistas; grande demais para a nuvem,
   baixe no PC.
5. **Freesound** (só CC0 ou CC BY) e **Sonniss GDC**: a nuvem não alcança, e o Freesound exige login, então quem baixa é o
   Felipe. A licença da Sonniss proíbe redistribuir o som solto: num repositório público, o `.wav` fica fora do git.

Recusa: NC, ND, SA, GPL, "uso pessoal", licença ausente ou ambígua, som de filme, jogo ou TV, e som de marca.

## Tratar

1. Escreva um manifesto JSON (lista de `{id, arquivo, titulo_da_pagina, autor, url_da_pagina, licenca, credito,
   trecho_s?, reserva?}`); cada trecho em `trecho_s` vira uma variação.
2. `python3 -I Tools/Audio/tratar_sons.py manifesto.json Audio`: corta o silêncio (nos impactos, a partir do estampido),
   converte, normaliza o pico em -1 dBFS e faz o fade.
3. **Olhe a forma de onda** de cada arquivo (ffmpeg `showwavespic`) e a imagem com a ferramenta Read: arquivo vazio,
   ataque atrasado (tiro com silêncio antes) ou corte no lugar errado aparecem ali. Na nuvem ninguém ouve: diga isso ao
   Felipe e peça para ele ouvir no PC.
4. Ajuste `Data/Sons.csv` e rode `Tools/Regras/testar.sh Deriva.Sons`: confere que todo id do código existe e que todo
   arquivo é WAV mono 48 kHz 16 bits de até 6 s (laço até 12 s).

## Registrar (no mesmo commit)

- Uma linha por página de origem em `docs/licencas.csv` (origem, url, licença, crédito, `gerado_por_ia`, usado_em, data).
- CC BY e OGA-BY: o texto de crédito em `Data/Creditos.txt`, que vai junto no jogo empacotado (avisa que os sons foram
  modificados).
- No PC: `Tools\Unreal\importar_sons.ps1` e, depois de ouvir no jogo, commit dos `.uasset` de `Content/Zombie/Sons`.
