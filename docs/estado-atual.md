# Estado atual do projeto

Documento de passagem. Uma sessão nova do Claude (nuvem ou PC) lê este arquivo e o `CLAUDE.md` e consegue continuar sem
o histórico das conversas. Atualizar a cada bloco de trabalho fechado.

Última revisão: 14 out 2026 (primeira sessão, na nuvem).

## Como o trabalho está dividido

- **Nuvem:** design, contratos em `docs/`, regras puras com testes (compilam aqui), C++ do Unreal escrito e conferido sem
  compilar, dados e documentação.
- **PC do Felipe:** compila, roda os testes do Unreal, joga, captura, cria materiais e mapas por script e mede.
- **Felipe:** joga e decide a sensação, o visual e o escopo.

## O que existe

| Área | Onde | Estado |
|---|---|---|
| Design, estratégia viral, roteiro | `docs/design.md`, `docs/viralizar.md`, `docs/roadmap.md` | Versão 0.1, para o Felipe ler e decidir |
| Regras puras: cidade, fluxo, corpo, explosão, hype, Diretor, dia e noite, chat, clipes, CSV | `Source/Zombie/Rules/` | **68 testes passando** na nuvem (g++ 13 e clang++, `-Wall -Wextra -Wshadow -Wconversion -Werror`, ASan e UBSan) |
| Tabelas de ajuste | `Data/*.csv` | Carregam sem erro (testes `Deriva.*`) |
| Projeto Unreal 5.8 | `Zombie.uproject`, `Config/`, `Source/` | Escrito na nuvem; **ainda não compilado**. Conferidor de C++ sem erros; revisão independente feita (ver abaixo) |
| Jogo de protótipo | `Source/Zombie/{Game,Player,Zombies,Combat,Vehicles,Stream,UI}` | Cidade gerada, jogador, zumbis, explosões, carro, Diretor, live, Twitch e HUD (lista no README) |
| Scripts do PC | `Tools/Unreal/*.ps1`, `criar_materiais.py` | Copiados e adaptados do Detonauto; `jogar.ps1` e `criar_materiais` são novos e nunca rodaram |
| Rotinas do Claude | `.claude/skills/` | `verificar`, `revisar-sem-compilar`, `jogar-e-capturar`, `modelo-3d`, `triar-playtest` |

## Próximo passo (sessão do PC)

1. `Tools\Unreal\build.ps1 -Uproject Zombie.uproject`. O C++ nunca foi compilado: espere alguns erros e corrija um por um.
   Cada erro que o conferidor deveria ter pego vira regra nova nele (skill `revisar-sem-compilar`, passo 6).
2. `Tools\Unreal\test.ps1 -Uproject Zombie.uproject -Filter Zombie.Regras`: os mesmos 68 testes da nuvem.
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

## Decisões pendentes do Felipe

Estão em `docs/roadmap.md`, "Decisões que são do Felipe": nome, repositório público ou privado, horas por semana, preço e
nível de gore.

## Armadilhas já descobertas

- **Praça cercada de prédios ficava inalcançável** para a horda. O teste `Cidade.TodoChaoAndavelEhAlcancavel` pegou; a praça
  passou a ser o quarteirão inteiro.
- Herdadas do Detonauto (`docs/tecnico.md`): `bUseUnity = false`; sombra de nome é erro no Unreal; material com Nanite
  precisa da flag; teste verde não prova visual.
