# Plano visual: do protótipo de blocos ao "isso é real?"

Pedido do Felipe: realismo, qualidade gráfica e diversão. Visual realista é requisito. Todo efeito é aprovado por
**captura no jogo**, nunca só por teste.

## Onde estamos

O protótipo é feito de formas básicas: a cidade em blocos, os personagens em blocos e efeitos de esfera e luz. Ele **serve
para sentir o jogo** (ritmo, câmera, horda, explosão e a live), **não para julgar a aparência**. A base de renderização
já é a realista, herdada do Detonauto e medida lá a 60 fps:

- Lumen na luz indireta e nos reflexos;
- sombras virtuais;
- TSR;
- neblina volumétrica;
- céu com atmosfera física;
- luz do céu capturada em tempo real;
- sol e lua que giram com a hora.

## Passos

O roteiro de execução, passo a passo e com o que fica fora do git, está em `docs/realismo-roteiro.md`.

1. **Cena de aparência (`L_Lookdev`)**, criada por script Python no PC:
   - uma esquina de Porto Brasa no fim de tarde;
   - asfalto e calçada com texturas CC0 (Poly Haven);
   - um prédio do City Sample, um carro e um bujão;
   - quatro ângulos fixos de captura, antes e depois.
2. **Personagem e zumbi de verdade:**
   - Manny e Quinn com o Game Animation Sample;
   - um zumbi esquelético com o material de pele doente e sangue em camadas;
   - desmembramento por osso.
3. **Explosão de verdade:**
   - Niagara (bola de fogo, fumaça que sobe, faíscas, onda de choque);
   - decalque de queimado e luz que pisca.
   - O bujão ganha a malha real.
4. **Rua de verdade:** City Sample Buildings e Vehicles migrados só no que se usa (licença só para Unreal), mais PCG para
   lixo, carros abandonados, caixas d'água e varais.
5. **Noite:**
   - luz de sódio dos postes;
   - fogo de carros queimando como fonte de luz;
   - lanterna do celular.
6. **Modo celular:** lente mais convincente (distorção, rolagem do obturador nos movimentos rápidos e foco automático
   "caçando"), compressão de vídeo simulada e a interface do app de live.

## Como aprovar

- Captura no jogo: `Tools/Unreal/jogar.ps1 -Shot 8` (dia), `-Hour 21` (noite) e `-Cam 3` (modo celular). As imagens vão
  para `docs/img/` com antes e depois.
- Desempenho medido na mesma cena (skill `achar-gargalo`); nada entra se passar de 16,7 ms no perfil alvo.

## Fontes de arte (ordem do estúdio)

1. A aba "Modelos 3D" do doc do estúdio e o repositório `Felipebonamigo/acervo-3d`.
2. Conteúdo da Epic para Unreal (City Sample, Game Animation Sample, MetaHuman). Fica fora do repositório público e entra
   no projeto pelo PC.
3. Sketchfab, Poly Haven, Kenney, Quaternius e a galeria CC0 do Meshy, conferindo a licença.
4. Gerar no Meshy só em último caso, **perguntando ao Felipe antes**, com o custo em créditos (~30 por modelo com textura).

Todo asset novo ganha uma linha em `docs/licencas.csv` no mesmo dia (skill `modelo-3d`).
