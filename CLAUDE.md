# AO VIVO (repositório Zombie) — instruções para o Claude

Mundo aberto de zumbis em terceira pessoa (um "GTA de zumbis"), em Unreal Engine 5.8 e C++, numa metrópole brasileira
fictícia (Porto Brasa) durante o Carnaval. O jogador é um streamer que transmite o fim do mundo ao vivo; o chat joga
junto. Referências do Felipe: GTA, Red Dead Redemption 2, Infestation, Night of the Dead e Left 4 Dead.

Responda em português do Brasil, inclusive nos commits, nos comentários e nos documentos. Nomes de código (classes,
funções, variáveis) ficam em inglês, como é o padrão do Unreal.

## Ler primeiro

1. `docs/estado-atual.md`: o que existe, o que foi verificado e qual é o próximo passo. Toda sessão começa por ele.
2. `docs/design.md` (o jogo), `docs/viralizar.md` (a estratégia) e `docs/roadmap.md` (fases e portões).
3. `docs/regras-de-jogo.md` (o contrato das regras puras) e `docs/tecnico.md` (a arquitetura).
4. Estúdio, com os jogos do Felipe, as ferramentas e as pendências em comum:
   https://claude.ai/artifact/72KXW2Tve5gB65BHD1CDp6 (documento do Claude Docs, lido pelo conector).

## Comandos

- Na nuvem:
  - `Tools/Regras/testar.sh [filtro]` compila as regras puras com g++ e clang++ (avisos como erro) e roda os testes.
    `ZR_SANITIZE=1` liga ASan e UBSan.
  - `python3 .claude/skills/revisar-sem-compilar/conferir_cpp.py` confere o C++ do Unreal sem compilar.
- No PC (PowerShell, na raiz):
  - `Tools\Unreal\build.ps1 -Uproject Zombie.uproject` compila.
  - `Tools\Unreal\test.ps1 -Uproject Zombie.uproject -Filter Zombie` roda os testes de Automation.
  - `Tools\Unreal\jogar.ps1 [-Shot 8] [-Hour 21] [-Cam 3] [-Horde] [-Twitch canal]` joga ou tira print.
  - `Tools\Unreal\criar_materiais.ps1` cria os materiais do protótipo.

## Rotinas em `.claude/skills/`

- `verificar`: o que rodar antes de dizer que está pronto, na nuvem e no PC.
- `revisar-sem-compilar`: confere o C++ antes do commit; o conferidor é o mesmo dos outros jogos em Unreal.
- `jogar-e-capturar`: abre o jogo, tira print e olha a imagem antes de afirmar algo visual.
- `modelo-3d`: procurar antes, registrar depois.
- `triar-playtest`: Issues "[Playtest]"; o commit que corrige diz "fecha #N".

As rotinas dos outros jogos (`achar-gargalo`, `ler-crash`, `publicar-na-steam`, `declaracao-ia`, `conferir-fato`,
`som-livre`, `traduzir`, `ligar-unreal-mcp`) servem aqui do mesmo jeito. Copie de `Felipebonamigo/Detonauto` e adapte os
caminhos quando a fase pedir. Unreal MCP deste jogo: **porta 8040**.

## Regras de trabalho

1. **Regras puras fora do motor.** O que dá para testar sem física (hype, corpo, Diretor, cidade, chat, tabelas) vai para
   `Source/Zombie/Rules/`, em C++ puro (namespace `ZR`, sem nenhum include do Unreal; o `testar.sh` recusa). Regra nova
   ganha teste em `Rules/Tests/ZRTests*.inl`, **visto falhar antes da correção**. Os mesmos casos rodam no Unreal como
   `Zombie.Regras.*`.
2. **Lógica em C++, Blueprint só como cola.** Nada de .umap ou Blueprint que o Claude precise ler: mapa, material e cena
   saem de scripts Python em `Tools/Unreal/`.
3. **Dados em texto:** todo número de ajuste vai para `Data/*.csv` (`docs/dados.md`). Chave nova de `Ajustes.csv` entra
   em `BindTuning` (`Rules/ZRData.cpp`); o teste de deriva cobra.
4. **A nuvem não compila o Unreal.** Antes de commitar C++ do Unreal, rode a skill `revisar-sem-compilar` e escreva no
   commit a linha "Para o PC:". Nunca diga que compila sem a saída do `build.ps1` no PC.
5. **Visual realista é requisito, aprovado por captura** (`jogar.ps1 -Shot`), nunca só por teste verde
   (`docs/plano-visual.md`).
6. **Desempenho é requisito:** 60 fps num PC nível PS5. Sistema novo de horda, efeito ou física passa por medição antes de
   ser aprovado.
7. Prefixos: `AZMB`, `UZMB` e `FZMB` no Unreal; `ZR::` nas regras puras. Log em `LogZombie`. Texto que o jogador lê é `FText`
   (`LOCTEXT`); os testes se chamam `Zombie.<Grupo>.<Caso>` sem acento.
8. **Tarefa "architectural"** (design escrito e aprovado pelo Felipe antes de qualquer código): multijogador e co-op,
   formato de save, mudança na arquitetura das camadas, troca de motor ou de sistema central (Mass, World Partition) e
   qualquer integração que **escreva** em serviço externo (Twitch, YouTube, Steam). O resto é "bounded".

## Licenças, marcas e IA

- Nenhum asset entra sem uma linha em `docs/licencas.csv` (skill `modelo-3d`).
  - **Aceito:** CC0, CC BY com crédito, conteúdo da Epic só em jogo Unreal, MIT ou Apache com aviso.
  - **Recusado:** NC, ND, "uso pessoal" e origem não comprovada.
- Modelos 3D em comum aos jogos do estúdio: antes de baixar ou gerar, procure na aba "Modelos 3D" do doc do estúdio e no
  repositório `Felipebonamigo/acervo-3d`. Modelo novo entra nos dois no mesmo dia.
- **Meshy só em último caso e sempre perguntando ao Felipe antes**, com o custo (~30 créditos por modelo com textura).
  Aceitar EULA, logar em conta ou comprar: só o Felipe autoriza.
- **Repositório possivelmente público:** nada da Epic nem do Mixamo no git até o Felipe decidir. City Sample, Game
  Animation Sample e MetaHuman entram no projeto pelo PC, fora do git, ou com o repositório privado.
- **Nenhuma marca real** em fachada, veículo, roupa ou interface. Texto sobre lugares e grupos reais passa pela skill
  `conferir-fato`; o morro de Porto Brasa é comunidade, não estereótipo.
- Conteúdo gerado por IA que o jogador vê (falas do chat simulado, modelos do Meshy) é marcado em `gerado_por_ia` e entra
  na declaração da Steam. **Música gerada por IA é proibida.**
- Twitch: só **leitura** anônima do chat. Escrever no chat ou usar conta exige aprovação do Felipe.

## Git

- Commits pequenos, em português, no imperativo ("Adiciona o zumbi gritador").
- Branch de trabalho: o indicado pela sessão. O `.gitattributes` manda binários para o Git LFS.
- Fechou um bloco de trabalho: atualize `docs/estado-atual.md`, faça o commit e o push.
