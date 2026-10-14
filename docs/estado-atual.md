# Estado atual do projeto

Documento de passagem. Uma sessão nova do Claude (nuvem ou PC) lê este arquivo e o `CLAUDE.md` e consegue continuar sem
o histórico das conversas. Atualizar a cada bloco de trabalho fechado.

Última revisão: 14 out 2026 (primeira sessão, na nuvem; blocos 1, 2 e 3).

## Como o trabalho está dividido

- **Nuvem:** design, contratos em `docs/`, regras puras com testes (compilam aqui), C++ do Unreal escrito e conferido sem
  compilar, dados e documentação.
- **PC do Felipe:** compila, roda os testes do Unreal, joga, captura, cria materiais e mapas por script e mede.
- **Felipe:** joga e decide a sensação, o visual e o escopo.

## O que existe

| Área | Onde | Estado |
|---|---|---|
| Design, estratégia viral, roteiro | `docs/design.md`, `docs/viralizar.md`, `docs/roadmap.md` | Versão 0.1, para o Felipe ler e decidir |
| Regras puras: cidade, fluxo, corpo, explosão, hype, Diretor, dia e noite, chat, clipes, pautas, base, CSV | `Source/Zombie/Rules/` | **90 testes passando** na nuvem (g++ 13 e clang++, `-Wall -Wextra -Wshadow -Wconversion -Werror`, ASan e UBSan) |
| Tabelas de ajuste | `Data/*.csv` | Carregam sem erro (testes `Deriva.*`) |
| Projeto Unreal 5.8 | `Zombie.uproject`, `Config/`, `Source/` | Escrito na nuvem; **ainda não compilado**. Conferidor de C++ sem erros. Uma revisão independente das assinaturas da API achou 1 erro de compilação (include do material dinâmico) e 1 defeito (o carro não viraria: o `APawn` segue o yaw do controle por padrão); os dois foram corrigidos |
| Jogo de protótipo | `Source/Zombie/{Game,Player,Zombies,Combat,Base,Vehicles,Stream,UI}` | Cidade gerada, jogador, 7 tipos de zumbi (com o gritador), explosões, carro, Diretor, live com pautas do patrocinador, base com barricadas e armadilhas (modo construção, X), Twitch e HUD (lista no README) |
| Branches | `main` e `claude/inspiring-einstein-idxmpu` | O `main` foi criado em 14/10 a partir do branch do Claude, a pedido do Felipe; os dois andam juntos |
| Scripts do PC | `Tools/Unreal/*.ps1`, `criar_materiais.py` | Copiados e adaptados do Detonauto; `jogar.ps1` e `criar_materiais` são novos e nunca rodaram |
| Rotinas do Claude | `.claude/skills/` | `verificar`, `revisar-sem-compilar`, `jogar-e-capturar`, `modelo-3d`, `triar-playtest` |

## Próximo passo (sessão do PC)

1. `Tools\Unreal\build.ps1 -Uproject Zombie.uproject`. O C++ nunca foi compilado: espere alguns erros e corrija um por um.
   Cada erro que o conferidor deveria ter pego vira regra nova nele (skill `revisar-sem-compilar`, passo 6).
2. `Tools\Unreal\test.ps1 -Uproject Zombie.uproject -Filter Zombie.Regras`: os mesmos 90 testes da nuvem.
3. `Tools\Unreal\criar_materiais.ps1`: materiais com aspereza e brilho; sem eles, o jogo usa o material básico da engine.
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

## Decisões pendentes do Felipe

Estão em `docs/roadmap.md`, "Decisões que são do Felipe": nome, repositório público ou privado, horas por semana, preço e
nível de gore.

## Pendências conhecidas

- Jogo empacotado: `Data/` ainda não é copiada no pacote (`DirectoriesToAlwaysStageAsNonUFS`); sem ela, valem os padrões do
  código. No editor e no `-game` não afeta.
- Base: o campo de fluxo da horda ignora as barricadas (perto, a 15 m, o zumbi vai direto e bate nelas; rua inteira fechada
  ainda não faz a horda contornar pelo quarteirão). O carro não quebra barricada (para nela). As armadilhas não têm som.
- Manchas de sangue e poeira de impacto não contam no orçamento de pedaços (`mundo.pedacos_max`). A mancha não tem Tick e
  só 30% dos acertos deixam uma; medir com horda grande.

## Armadilhas já descobertas

- **`APawn` segue o yaw do controle por padrão:** todo pawn com física própria (o carro) precisa desligar
  `bUseControllerRotationYaw`, senão a rotação é teleportada todo quadro.
- `FMath::FloorToInt(double)` devolve `int64` no UE5: para `int32`, use `FloorToInt32`.
- **Praça cercada de prédios ficava inalcançável** para a horda. O teste `Cidade.TodoChaoAndavelEhAlcancavel` pegou; a praça
  passou a ser o quarteirão inteiro.
- Herdadas do Detonauto (`docs/tecnico.md`): `bUseUnity = false`; sombra de nome é erro no Unreal; material com Nanite
  precisa da flag; teste verde não prova visual.
