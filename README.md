# AO VIVO (Zombie)

**Um GTA de zumbis no Brasil, transmitido ao vivo por você.**

Porto Brasa, sábado de Carnaval. O surto começa no meio do bloco, e você, streamer com 37 seguidores, está com a live
ligada. Sobreviva à cidade aberta, de carro ou a pé: um bujão de gás bem colocado vira reação em cadeia, e o chat manda
hordas e zumbis com o próprio nome. Quanto mais espetacular, mais gente assiste. Quanto mais gente assiste, mais doações,
e as doações viram munição entregue por drone.

Unreal Engine 5.8, C++. Referências: GTA, Red Dead Redemption 2, Infestation, Night of the Dead e Left 4 Dead.

- Design: [`docs/design.md`](docs/design.md)
- Como viralizar: [`docs/viralizar.md`](docs/viralizar.md)
- Roteiro: [`docs/roadmap.md`](docs/roadmap.md)
- Estado atual: [`docs/estado-atual.md`](docs/estado-atual.md)
- Controles: [`docs/controles.md`](docs/controles.md)

## O que já tem (protótipo de outubro de 2026)

Tudo em formas básicas da engine, para sentir o jogo; o visual realista vem por fases ([`docs/plano-visual.md`](docs/plano-visual.md)).

- **Cidade gerada:** ruas, prédios com janelas acesas à noite, caixas d'água, praças, postes, praia e mar.
- **Céu com dia e noite:** atmosfera física, sol e lua, neblina volumétrica e Lumen.
- **Câmera:** terceira pessoa **de longe e do alto** (padrão), média e perto; mira por cima do ombro; **primeira pessoa no
  "modo celular"**.
- **Armas:** pistola, escopeta e fuzil, mais chute, granada, **bujão arremessável** e **mira lenta**, o Dead Eye.
- **7 tipos de zumbi:** comum, folião de Carnaval, corredor, **bujão** (explode), inchado, brutamontes e **gritador**
  (chama a horda).
  - Têm desmembramento por membro (a cabeça estoura, sem perna vira rastejante) e explodem em pedaços.
- **Explosões em cadeia:** bujões, barris, carros abandonados e os zumbis-bujão.
- **Carro dirigível,** que atropela.
- **Base à Night of the Dead:** sucata (caixa do drone e carro desmanchado) e **modo construção** (X) com barricada de
  madeira, muro de sucata, estacas, arame farpado e **bujão armado**. A horda contorna a base e entra pela abertura (onde
  ficam as armadilhas); sem abertura, bate na barricada até quebrar. Carro rápido arromba. Dá para consertar e desmontar.
- **Diretor da horda** no estilo Left 4 Dead (tensão, pico e respiro), noite com mais zumbis e **Noite da Horda** a cada 3
  noites.
- **A live:**
  - hype, combo, espectadores, seguidores e doações que pagam drones de suprimento;
  - **pautas do patrocinador:** desafios com prazo que pagam ("explosão em cadeia 3 vezes em 2 min");
  - chat simulado que comenta e vota;
  - **chat de verdade da Twitch** (`!zumbi`, `!horda`, `!bujao`, `!caixa`);
  - botão **Clipar**.

## Como jogar (PC com Unreal 5.8)

```powershell
.\Tools\Unreal\build.ps1 -Uproject Zombie.uproject      # compila (editor fechado)
.\Tools\Unreal\jogar.ps1                                # abre o jogo
.\Tools\Unreal\jogar.ps1 -Twitch seucanal               # com o chat da sua live na Twitch
```

Ou abra `Zombie.uproject` no editor e aperte Play: o mapa vazio padrão vira Porto Brasa sozinho.

## Testes

```bash
Tools/Regras/testar.sh                 # regras puras (qualquer máquina com g++ ou clang++)
```
```powershell
.\Tools\Unreal\test.ps1 -Uproject Zombie.uproject -Filter Zombie    # no Unreal
```

## Estrutura

| Pasta | O quê |
|---|---|
| `Source/Zombie/Rules/` | Regras puras em C++ sem Unreal (testadas em qualquer lugar) |
| `Source/Zombie/{Game,Player,Zombies,Combat,Base,Vehicles,Stream,UI}/` | O jogo no Unreal |
| `Data/` | Tabelas de ajuste em CSV ([`docs/dados.md`](docs/dados.md)) |
| `Config/` | Renderização (Lumen, sombras virtuais, TSR), entrada e colisão |
| `Tools/` | Testes das regras, scripts do PC e scripts Python do editor |
| `docs/` | Design, regras, técnica, visual, roteiro e licenças |
