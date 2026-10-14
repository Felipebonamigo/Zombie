# Controles (protótipo)

Teclado e mouse, e controle (Xbox ou Steam Deck). Todos são criados em código (`Player/ZMBCharacter.cpp`,
`Vehicles/ZMBCar.cpp`); trocar uma tecla é trocar uma linha lá.

## A pé

| Ação | Teclado e mouse | Controle |
|---|---|---|
| Andar | W A S D | Analógico esquerdo |
| Olhar | Mouse | Analógico direito |
| Correr (gasta fôlego) | Shift (segurar) | Clique no analógico esquerdo |
| Pular | Espaço | A |
| Atirar | Botão esquerdo | Gatilho direito |
| Mirar por cima do ombro | Botão direito (segurar) | Gatilho esquerdo |
| Recarregar | R | X |
| Trocar de arma | 1 2 3 / roda do mouse | Direcional para a direita |
| Chute (empurra e derruba) | F | B |
| Granada | G | RB |
| Arremessar bujão (atire nele no ar!) | H | Direcional para a esquerda |
| Mira lenta (Dead Eye; gasta foco) | Q | LB |
| Entrar no carro | E (perto do carro) | Y |
| Distância da câmera (longe e alta, média, perto) | C | Direcional para cima |
| Primeira pessoa, "modo celular" | V | Direcional para baixo |
| Clipar o melhor lance (print com o HUD) | B | Botão View |
| Mostrar ou esconder a ajuda | F1 | Botão Menu |
| Teste: chamar a horda | N | — |
| Teste: avançar 3 horas | T | — |

## No carro

| Ação | Teclado | Controle |
|---|---|---|
| Acelerar | W | Gatilho direito |
| Frear e dar ré | S | Gatilho esquerdo |
| Esterçar | A D | Analógico esquerdo |
| Freio de mão (derrapa) | Espaço | A |
| Sair | E | Y |
| Desvirar | R | X |

## Console do jogo (tecla ')

| Comando | O que faz |
|---|---|
| `zmb.twitch nomedocanal` | Lê o chat de um canal da Twitch (sem conta) |
| `zmb.twitch.sair` | Para de ler a Twitch (volta o chat simulado) |
| `zmb.dados.recarregar` | Lê de novo `Data/*.csv` |
