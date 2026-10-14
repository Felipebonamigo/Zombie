#!/usr/bin/env bash
# Compila e roda os testes das regras puras (Source/Zombie/Rules) sem o Unreal.
# Uso (na raiz do repositório):
#   Tools/Regras/testar.sh              # g++ e clang++, avisos como erro, depois os testes
#   Tools/Regras/testar.sh Hype.        # só os testes cujo nome começa com "Hype."
#   ZR_SANITIZE=1 Tools/Regras/testar.sh   # também com AddressSanitizer e UBSan
# Sai 0 se compilou nos dois compiladores e todos os testes passaram.
set -euo pipefail
cd "$(dirname "$0")/../.."

FILTRO="${1:-}"
SAIDA="${TMPDIR:-/tmp}/zombie-regras"
mkdir -p "$SAIDA"

# As regras não podem incluir nada do Unreal: é isso que as deixa testáveis aqui.
if grep -RnE '#include\s+"(CoreMinimal|Engine/|GameFramework/|UObject/|Misc/|Containers/)' Source/Zombie/Rules --include='*.h' --include='*.cpp' --include='*.inl' | grep -v 'Rules/Tests/ZRRulesTests.cpp'; then
	echo "ERRO: arquivo de Source/Zombie/Rules inclui cabeçalho do Unreal (lista acima)."
	exit 1
fi

FONTES=(Source/Zombie/Rules/*.cpp Tools/Regras/executor.cpp)
AVISOS=(-std=c++20 -Wall -Wextra -Wshadow -Wconversion -Wpedantic -Werror -ISource/Zombie)

compilar() {
	local cxx="$1"; shift
	if ! command -v "$cxx" >/dev/null 2>&1; then
		echo "($cxx não instalado; pulando)"
		return 0
	fi
	echo "== $cxx"
	"$cxx" "${AVISOS[@]}" "$@" "${FONTES[@]}" -o "$SAIDA/executor-$cxx"
}

compilar g++ -O1
compilar clang++ -O1
EXEC="$SAIDA/executor-g++"
[ -x "$EXEC" ] || EXEC="$SAIDA/executor-clang++"

if [ "${ZR_SANITIZE:-0}" = "1" ]; then
	echo "== g++ com sanitizers"
	g++ "${AVISOS[@]}" -O1 -g -fsanitize=address,undefined -fno-sanitize-recover=undefined "${FONTES[@]}" -o "$SAIDA/executor-san"
	EXEC="$SAIDA/executor-san"
fi

"$EXEC" Data "$FILTRO"
