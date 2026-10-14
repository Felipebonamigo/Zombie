---
name: triar-playtest
description: Ler, organizar e resolver os problemas de playtest do Zombie (AO VIVO) que chegam como Issues "[Playtest]" no GitHub. Use quando o Felipe disser que jogou, mandar uma lista de problemas ou pedir para ver as Issues.
---
# Triar o playtest

Os problemas chegam como Issues "[Playtest] ..." pelo modelo `.github/ISSUE_TEMPLATE/playtest.yml`. Se o Felipe mandar a
lista pelo chat, abra uma Issue por item com esse modelo antes de começar.

1. Liste as Issues abertas com "[Playtest]" no título (`Felipebonamigo/Zombie`).
2. Ordene pela gravidade do formulário: trava, quebra a partida, atrapalha, incômodo, sugestão. Sugestão vira item do
   `docs/roadmap.md`, não correção.
3. Reproduza antes de mexer (skill `jogar-e-capturar`, no PC). Sem reproduzir, comente o que tentou e pergunte o que falta.
4. Corrija com teste: se a regra é pura (`Rules/`), o defeito vira teste visto falhar antes da correção.
5. Verifique com a skill `verificar`.
6. Commit pequeno em português citando a Issue: `Corrige <o quê> (fecha #N)`.
7. Comente na Issue o commit e o que mudou, numa frase, com o rodapé do Claude Code. Se depende de o Felipe jogar de novo,
   deixe aberta.
8. Resuma para o Felipe: o que fechou, o que ficou aberto e por quê.
