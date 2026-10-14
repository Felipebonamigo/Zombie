---
name: verificar
description: O que rodar no Zombie (AO VIVO) antes de dizer que uma mudança está pronta, na nuvem (testes das regras puras com g++/clang++, conferidor de C++) e no PC (build.ps1, test.ps1 -Filter Zombie, captura). Use ao terminar qualquer mudança, antes do commit, ou quando perguntarem "está pronto?", "os testes passam?", "compila?".
---
# Verificar

Nada está pronto sem rodar o comando no mesmo turno e ler a saída. "Deveria funcionar" não conta.

## Na nuvem (sempre)

1. Regras puras: compilam com g++ e clang++ (avisos como erro) e rodam todos os testes, incluindo os de deriva das
   tabelas de `Data/`:
   ```bash
   Tools/Regras/testar.sh              # tudo
   Tools/Regras/testar.sh Hype.        # só um grupo (prefixo do nome)
   ZR_SANITIZE=1 Tools/Regras/testar.sh
   ```
   Deu certo: `RESUMO: N testes, N aprovados, 0 reprovados` e saída 0. Regra nova: o teste é escrito antes e visto falhar
   pelo motivo certo.
2. C++ do Unreal: skill `revisar-sem-compilar` (o conferidor sai com 0).
3. Mexeu em `Data/*.csv`: o passo 1 já carrega as tabelas de verdade (`Deriva.*`).

## No PC (o que só o Unreal prova)

Feche o editor antes de compilar (ele segura a DLL).
```powershell
.\Tools\Unreal\build.ps1 -Uproject Zombie.uproject
.\Tools\Unreal\test.ps1 -Uproject Zombie.uproject -Filter Zombie
.\Tools\Unreal\test.ps1 -Uproject Zombie.uproject -Filter Zombie.Regras.Hype
```
- `BUILD OK` e saída 0; nos testes, `RESUMO: N aprovados, 0 reprovados, 0 não rodaram`.
- Mexeu em algo visível: skill `jogar-e-capturar` e olhar a imagem.
- Os testes `Zombie.Regras.*` são os mesmos casos da nuvem: se passam lá e falham aqui, a diferença é de compilador ou de
  caminho de `Data/` (`FPaths::ProjectDir()`).

## Quando falha

- Build: leia a primeira mensagem de erro (as outras costumam ser consequência); se o erro escapou do conferidor, ele vira
  regra nova lá (skill `revisar-sem-compilar`, passo 6).
- `Nenhum teste encontrado`: o filtro tem que começar com `Zombie`.
- Teste verde não prova efeito visível.
