---
name: jogar-e-capturar
description: Abre o Zombie (AO VIVO) para jogar ou tirar print sozinho (Tools/Unreal/jogar.ps1 com -Shot, -Hour, -Cam, -Horde, -Twitch, -Seed) e olha a imagem antes de afirmar qualquer coisa visual. Use para "jogar", "abrir o jogo", "tirar print", "mostra como ficou", "testa de noite", "testa o modo celular" ou aprovar um efeito visual.
---
# Jogar e capturar

Só no PC do Felipe; a nuvem não abre o jogo. Compile antes (skill `verificar`). O script abre o `UnrealEditor.exe -game`
com a DLL já compilada, no mapa vazio que vira Porto Brasa.

```powershell
.\Tools\Unreal\jogar.ps1                              # o Felipe joga (feche a janela para sair)
.\Tools\Unreal\jogar.ps1 -Shot 8                      # print aos 8 s, de dia, câmera longe e alta
.\Tools\Unreal\jogar.ps1 -Shot 8 -Hour 21             # de noite (postes e janelas acesos)
.\Tools\Unreal\jogar.ps1 -Shot 8 -Cam 3               # modo celular (primeira pessoa)
.\Tools\Unreal\jogar.ps1 -Shot 25 -Horde -Cam 1       # no meio de uma horda
.\Tools\Unreal\jogar.ps1 -Twitch nomedocanal          # com o chat de verdade
```
- `-Seed N` troca a cidade; `-Exec 'stat unit','stat fps'` manda comandos de console; `-ResX`/`-ResY` (1600x900).
- Deu certo: saída 0 e a linha `PRINT: <caminho>`. Abra a imagem com a ferramenta Read e descreva o que vê **antes** de
  dizer que algo ficou bom.
- Para comparar antes e depois, salve as capturas em `docs/img/<assunto>/` (vão no Git LFS).

## Ao olhar a captura

- Céu e luz coerentes com a hora (sol baixo e dourado às 17 h; azul-escuro com postes de sódio à noite).
- HUD da live completo: AO VIVO, espectadores, hype, chat à direita, vida, munição e relógio.
- Nenhum material cinza quadriculado (material faltando) e nenhum "Error" no log (o script lista).
- Desempenho: `-Exec 'stat unit'` e a skill `achar-gargalo` quando for aprovar sistema novo.
