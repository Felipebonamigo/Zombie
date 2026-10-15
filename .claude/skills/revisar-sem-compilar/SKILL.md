---
name: revisar-sem-compilar
description: 'Confere o C++ do Unreal do Zombie (AO VIVO) antes do commit, porque a nuvem não compila: roda o conferidor (generated.h, GENERATED_BODY, UPROPERTY, *_API, .h × .cpp, includes, Build.cs e sombra de nome nos métodos), revisa à mão o que ele não pega (assinaturas da API) e diz ao PC o que compilar. Use ao mexer em .h ou .cpp de Source/Zombie fora de Rules/, antes de commitar C++, ou quando o Felipe disser "vai compilar?", "confere o C++" ou "o build quebrou".'
---
# Revisar o C++ sem compilar

A nuvem edita o C++ mas não tem o Unreal. Erro bobo só aparece quando o Felipe compila no PC, e isso custa uma volta
inteira. Esta rotina pega o erro mecânico antes do commit. Ela não substitui a compilação: a palavra final é do PC.

As regras puras (`Source/Zombie/Rules/`) **compilam na nuvem**: para elas vale `Tools/Regras/testar.sh` (skill `verificar`).

## Passos

1. Rode o conferidor na raiz do repositório (Python 3, sem dependência; o arquivo é o mesmo dos outros jogos em Unreal):
   ```bash
   python3 .claude/skills/revisar-sem-compilar/conferir_cpp.py                    # o que mudou desde o HEAD
   python3 .claude/skills/revisar-sem-compilar/conferir_cpp.py --base origin/main   # tudo o que o branch mudou
   python3 .claude/skills/revisar-sem-compilar/conferir_cpp.py --todos            # todo o C++
   ```
   Sai com 1 se achou ERRO. ERRO se conserta antes do commit; AVISO se lê e decide; NOTA lembra o que conferir à mão.
   Mexeu no conferidor? Rode o teste dele: `python3 .claude/skills/revisar-sem-compilar/teste_conferir.py`.
2. Revise à mão, nos trechos que mudou, a lista "O que o conferidor não pega".
3. Para mudança grande (classe nova, API do Unreal que o projeto ainda não usa), peça uma revisão independente a um
   subagente: ele procura erro de compilação com as assinaturas reais da API, usando o Detonauto
   (`Felipebonamigo/Detonauto`, que compila na 5.8) como referência de idioma.
4. Escreva no commit, ou na resposta ao Felipe, a linha "Para o PC:" que o conferidor imprime: ela diz se basta o Live
   Coding ou se é preciso fechar o editor e compilar.
5. Compile e teste no PC pela skill `verificar`.
6. Se o build do PC quebrar mesmo assim, o erro que escapou vira regra nova no conferidor, com o caso real no comentário.
   Copie a mudança para os outros jogos em Unreal (Detonauto, Age of Earth, TDFende): o arquivo é o mesmo.

## O que o conferidor não pega

- **Sombra de nome, fora do que o conferidor vê:** ele confere os métodos `Classe::Metodo` dos `.cpp` (parâmetro ou local
  com nome de membro da classe, C4458; local com nome de parâmetro, C4457; local escondendo outro local de fora, C4456),
  inclusive parâmetros e locais de lambda, mas não olha função solta nem método inline do `.h`, e das bases do Unreal só
  conhece os membros mais comuns (`MEMBROS_DA_BASE`: `Controller`, `Mesh`, `Canvas`, `bHidden`...). Nesses lugares,
  confira à mão: o Unreal trata sombra como **erro**.
- Assinatura igual no .h e no .cpp (`const`, referência, `override`); handler de `AddDynamic` com a assinatura exata do
  delegate e `UFUNCTION()`.
- `FMath::Min`, `Max`, `Clamp` e `RandRange` com tipos misturados (`int32` com `size_t`, `float` com `double`): o template
  não deduz. Converta antes.
- Macros do Unreal com nome comum: nunca use `PI` (use `UE_PI`), `check`, `verify`, `ensure`, `IN`, `OUT` ou `DELTA` como nome.
- `Super::` nos overrides (`BeginPlay`, `Tick`, `EndPlay`, `Deinitialize`...).
- Membro novo com valor inicial; ponteiro de UObject em membro só com `UPROPERTY` (ou `TWeakObjectPtr`).
- Texto que o jogador lê é `FText` (`LOCTEXT`); log em `LogZombie`.
- Tick ligado só quando precisa; sistema por quadro novo passa por medição.

## Armadilhas

- Rode sempre da raiz do repositório: o conferidor acha os módulos pelo `*.Build.cs`.
- Include de cabeçalho do motor fora da tabela do conferidor não é checado contra o `Build.cs`. Na dúvida, procure o módulo
  na página da classe na documentação da Epic ("Module") e confira o `Zombie.Build.cs`.
