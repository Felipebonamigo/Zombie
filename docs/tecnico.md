# Arquitetura técnica

Versão 0.1, 14 out 2026. Base: Unreal Engine 5.8, C++, um módulo (`Zombie`), Windows/Steam, 60 fps num PC nível PS5
(o PC do Felipe tem uma RTX 4070 Ti).

## Princípios (os mesmos do estúdio)

1. **Lógica em C++; Blueprint só como cola.** Blueprint é binário e o Claude não o lê nem edita com segurança.
2. **Regras fora do motor.** O que dá para testar sem física fica em `Source/Zombie/Rules/`, C++ puro sem Unreal. Compila e
   é testado **na nuvem** (`Tools/Regras/testar.sh`, g++ e clang++ com avisos como erro, mais sanitizers) e no PC como
   Automation (`Zombie.Regras.*`), com os mesmos casos. É a diferença em relação ao Detonauto, cujas regras usam tipos do
   Unreal e só são testadas no PC.
3. **Dados em texto** (`Data/*.csv`, ver `dados.md`). Ajustar não exige recompilar.
4. **Desempenho medido antes de aprovado.** Todo sistema por quadro tem orçamento (abaixo).
5. **Visual aprovado por captura** no jogo (`-ZMBShot`), não por teste verde.

## Camadas

```
 UI (ZMBHUD)            Stream (live, Twitch)
        \                 /
   Player   Zombies   Vehicles   Combat
        \      |        |        /
            Game (dados, mundo, modo)
                    |
              Rules (C++ puro, ZR::)
```

- `Rules/` nunca inclui nada do Unreal: o `testar.sh` falha se incluir.
- A `UI` só lê. Os outros módulos falam com a live por `UZMBStreamSubsystem::Report*` e com o Diretor por
  `AZMBHordeDirector::Report*`.

| Pasta | Classe | Papel |
|---|---|---|
| `Rules/` | `ZR::` | Hype, corpo, explosão, Diretor, fluxo, cidade, dia e noite, chat, clipes, base, som, marcos, saque, CSV e tabelas |
| `Game/` | `UZMBGameData` | Lê `Data/*.csv` e guarda os materiais de cor compartilhados |
| `Game/` | `AZMBWorldBuilder` | Monta a cidade de protótipo (com os lugares de saque), o céu, o sol e a lua, a neblina e o pós-processo |
| `Game/` | `AZMBGameMode` | Liga tudo num mapa vazio; opções de linha de comando para captura |
| `Player/` | `AZMBCharacter` | Câmeras, armas, chute, arremesso, mira lenta, carro e clipe |
| `Zombies/` | `AZMBZombie`, `AZMBHordeDirector` | Corpo por membros; onde e quando nascem; fluxo; pedidos do chat |
| `Combat/` | `AZMBExplosive`, `ZMBExplosion`, `AZMBFx`, `AZMBPickup` | Explosivos, explosão, efeitos provisórios, caixa do drone |
| `Loot/` | `AZMBLootSpot` | Lugar de saque (lixeira, mochila, isopor, viatura): revirar com E, escurece vazio |
| `Base/` | `AZMBBuildPiece` | Barricada e armadilha construídas: colisão, golpes, conserto, desmonte e o bujão armado |
| `Audio/` | `UZMBAudioSubsystem` | Toca os sons de `Data/Sons.csv` (variação, vozes, alcance); motor em laço |
| `Vehicles/` | `AZMBCar` | Carro de suspensão por raios (protótipo) |
| `Stream/` | `UZMBStreamSubsystem`, `UZMBTwitchChat` | A live e o chat da Twitch por WebSocket |
| `UI/` | `AZMBHUD` | HUD da live no Canvas |

## Nuvem e PC

- **Sessão da nuvem:** design, contratos em `docs/`, regras puras com testes, C++ do Unreal conferido sem compilar (skill
  `revisar-sem-compilar`), dados e documentação.
- **Sessão do PC:** compila (`Tools/Unreal/build.ps1`), roda os testes (`test.ps1 -Filter Zombie`), joga e captura
  (`jogar.ps1`), cria materiais e mapas por script Python e mede o desempenho.
- **Felipe:** joga, decide a sensação, o visual e o escopo.

## Orçamentos de desempenho (protótipo, a medir no PC)

| Sistema | Orçamento | Como |
|---|---|---|
| Zumbis com corpo completo (ator) | até 140 vivos à noite | `diretor.vivos_max_noite`; ACharacter sem controlador e sem malha esquelética |
| Pedaços com física (sangue e entulho) | até 220 | `mundo.pedacos_max`; os novos não nascem acima disso |
| Luzes pontuais | 64 postes (só à noite) + efeitos curtos | Sem sombra |
| Cidade | ~25 x 30 células; componentes instanciados (uma chamada por cor) | `cidade.*` |
| Vozes de som | por som, `vozes_max` de `Data/Sons.csv` (6 gemidos, 8 tiros de fuzil...) | A escolha (`ZR::FSoundPicker`) corta antes de tocar; atenuação por som criada em código |
| Peças da base | até 40 | `base.pecas_max`; armadilha olha quem está em cima 10 vezes por segundo; zumbi procura barricada na frente 4 vezes por segundo (varredura curta); com barricada perto, a grade fina (64 x 64 células) é refeita a cada 0,5 s e quando o jogador muda de célula (um Dijkstra de ~4 mil células) |

Meta de quadro: 16,7 ms (60 fps) no perfil alvo. A primeira medição fica para a sessão do PC (skill `achar-gargalo`).

## Caminho até o jogo final

| Sistema | Protótipo de hoje | Final |
|---|---|---|
| Personagens | Corpo de blocos | Manny e Quinn do Unreal, depois MetaHuman; locomoção do **Game Animation Sample** (Motion Matching), grátis no Fab |
| Zumbis | Blocos, um por membro | Malhas esqueléticas com **ossos que se soltam** (esconde o osso e solta a peça cortada), Niagara de sangue; animações de zumbi (Mesh2Motion CC0, captura própria ou pacotes do Fab) |
| Horda grande | Até 140 atores | **Mass Entity** (o mesmo da multidão do City Sample) para centenas ou milhares longe; perto, vira ator completo |
| Cidade | Grade de blocos gerada | **World Partition** + City Sample Buildings + **PCG** para espalhar quarteirões, lixo e carros |
| Explosões | Formas básicas e luz | Niagara (fogo, fumaça e faíscas), decalques e **Chaos Destruction** em vidros, muros e carros |
| Base | Peças de blocos (barricada, muro, estacas, arame, bujão armado) | Peças modulares do acervo-3d (skill `modelo-3d`), barricada que lasca com Chaos Destruction, portas e janelas de prédio para trancar |
| Carro | Caixa com suspensão por raios | **Chaos Vehicles**, com o que o Detonauto já aprendeu (presets e amassado) |
| Som | Gravações livres (CC0 e CC BY) em WAV, atenuação simples, mira lenta pelo tom global | **MetaSounds** (variação e camadas por arma), submixes com reverb por lugar (rua, beco, prédio), oclusão, a Free Firearm Sound Library e gravações próprias; música de banda independente com acordo escrito (nunca por IA) |
| Clipe | Print + janela no log | **Steam Timeline** (marcadores na Gravação de Jogo da Steam) e exportação 9:16 |
| Replay | — | Sistema de replay do Unreal (DemoNetDriver) para o replay de 10 s do clipe |
| Chat | Twitch anônimo (só leitura) | Mais YouTube Live e TikTok LIVE; extensão da Twitch |
| Multijogador | Um jogador | **Co-op de 4** com servidor de quem hospeda (listen server) e Online Subsystem Steam |
| Interface | Canvas | **CommonUI** (controle e Steam Deck), textos em String Table (skill `traduzir`) |

O co-op mexe em tudo: quem tem autoridade sobre o dano, a física e o Diretor. A decisão (**arquitetural**, aprovada pelo
Felipe antes de qualquer código) entra antes da fase 6 do roteiro. Até lá, o código evita supor um jogador só onde é
barato (por exemplo, o dano passa pelo `ApplyDamage` do Unreal).

## Armadilhas já conhecidas (herdadas do Detonauto)

- `bUseUnity = false`: helpers com nomes curtos colidiam no agrupamento unity.
- Material em malha Nanite precisa da flag de uso com Nanite, senão a engine troca pelo material padrão.
- Teste verde não prova efeito visível: todo efeito visual precisa de captura no jogo.
- No Unreal, aviso de sombra de nome (variável local com o nome de um membro) é **erro**: o conferidor não pega; revise à
  mão.
