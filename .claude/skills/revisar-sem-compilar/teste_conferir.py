#!/usr/bin/env python3
"""Teste do conferidor: monta um módulo de mentira numa pasta temporária, roda o conferir_cpp.py nele e confere os
achados. Cada caso tem a linha marcada no próprio C++ com // ESPERA: <trecho da mensagem> (ou // LIMPO: não pode acusar).

Uso (na raiz do repositório):  python3 .claude/skills/revisar-sem-compilar/teste_conferir.py
Sai com 0 se tudo bate, 1 se não.
"""
import os
import re
import subprocess
import sys
import tempfile

AQUI = os.path.dirname(os.path.abspath(__file__))

BUILD_CS = '''using UnrealBuildTool;
public class Teste : ModuleRules
{
	public Teste(ReadOnlyTargetRules Target) : base(Target)
	{
		PublicDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine" });
	}
}
'''

CABECALHO = '''#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "TSTHero.generated.h"

class UStaticMeshComponent;

UCLASS()
class TESTE_API ATSTHero : public ACharacter
{
	GENERATED_BODY()

public:
	ATSTHero();
	void Kick(float Damage, int32 Count);
	void Loot(const TArray<int32>& Parts);
	void Scan();
	void Ok(float Speed);
	int32 GetCount() const { return Count; }
	/** Um membro com inicializador que chama função: não é método. */
	float Mixed = FMath::Max(1.0f, 2.0f);

private:
	UPROPERTY(Transient)
	TArray<TObjectPtr<UStaticMeshComponent>> Parts;

	float Health = 100.0f;
	int32 Count = 0;
	TWeakObjectPtr<AActor> Target;
	uint8 bFlag : 1;
};
'''

FONTE = '''#include "TSTHero.h"
#include "Components/StaticMeshComponent.h"

ATSTHero::ATSTHero()
{
}

void ATSTHero::Kick(float Damage, int32 Count) // ESPERA: parâmetro Count esconde o membro ATSTHero::Count
{
	const float Health = Damage; // ESPERA: Health esconde o membro ATSTHero::Health
	float Damage2 = Damage; // LIMPO
	if (Damage2 > 0.0f)
	{
		float Damage = 1.0f; // ESPERA: Damage esconde o parâmetro
		(void)Damage;
	}
	(void)Health;
}

void ATSTHero::Loot(const TArray<int32>& Parts) // ESPERA: parâmetro Parts esconde o membro ATSTHero::Parts
{
	const UStaticMeshComponent* Mesh = nullptr; // ESPERA: Mesh esconde o membro herdado de ACharacter
	(void)Mesh;
}

void ATSTHero::Scan()
{
	const int32 Total = 3; // LIMPO
	for (int32 i = 0; i < Total; ++i) // LIMPO
	{
		const int32 Step = i * 2; // LIMPO
		(void)Step;
	}
	for (int32 i = 0; i < Total; ++i) // LIMPO
	{
		const int32 Step = i; // LIMPO
		(void)Step;
	}
	if (const AActor* Other = Target.Get()) // LIMPO
	{
		(void)Other;
	}
	if (const AActor* Other = Target.Get()) // LIMPO
	{
		const int32 Total = 4; // ESPERA: Total esconde outra declaração local
		(void)Other;
		(void)Total;
	}
	for (const auto& Part : Parts) // LIMPO
	{
		(void)Part;
	}
	auto Sum = [this](int32 Value) -> int32 // LIMPO
	{
		const int32 Doubled = Value * 2; // LIMPO
		return Doubled;
	};
	(void)Sum;
	auto Spawn = [this](const FVector& Where, bool bHidden) // ESPERA: parâmetro bHidden da lambda esconde o membro herdado de AActor
	{
		const int32 Count = 1; // ESPERA: Count esconde o membro ATSTHero::Count
		(void)Where;
		(void)bHidden;
		(void)Count;
	};
	(void)Spawn;
	FActorSpawnParameters Params; // LIMPO
	(void)Params;
	// caso real do Age of Earth: range-for com "->" no contêiner, fora e dentro de lambda de uma linha
	const ATSTHero* Self = this; // LIMPO
	int32 Sum2 = 0; for (const TPair<int32, int32>& K : Self->GetPairs()) Sum2 += K.Key; // LIMPO
	auto CountB = [&](int32 Key) { int32 N = 0; for (const TPair<int32, int32>& K : Self->GetPairs()) if (K.Key == Key) ++N; return N; }; // LIMPO
	(void)CountB;
	return;
}

void ATSTHero::Ok(float Speed) // LIMPO
{
	struct FSpec // LIMPO
	{
		UStaticMesh* Mesh; // ESPERA: campo Mesh da struct local tem o nome de ACharacter::Mesh
		float Scale; // LIMPO
	};
	const FSpec Specs[] = {{nullptr, 1.0f}}; // LIMPO
	(void)Specs;
	const float Twice = Speed * 2.0f; // LIMPO
	Health -= Twice;
	Count += 1;
}
'''


def esperados(fonte):
    """{linha: trecho esperado} e o conjunto de linhas que não podem ser acusadas."""
    espera, limpo = {}, set()
    for n, linha in enumerate(fonte.split('\n'), start=1):
        m = re.search(r'// ESPERA: (.+)$', linha)
        if m:
            espera[n] = m.group(1).strip()
        elif '// LIMPO' in linha:
            limpo.add(n)
    return espera, limpo


def main():
    with tempfile.TemporaryDirectory() as raiz:
        modulo = os.path.join(raiz, 'Source', 'Teste')
        os.makedirs(modulo)
        open(os.path.join(modulo, 'Teste.Build.cs'), 'w', encoding='utf-8').write(BUILD_CS)
        open(os.path.join(modulo, 'TSTHero.h'), 'w', encoding='utf-8').write(CABECALHO)
        open(os.path.join(modulo, 'TSTHero.cpp'), 'w', encoding='utf-8').write(FONTE)
        r = subprocess.run([sys.executable, os.path.join(AQUI, 'conferir_cpp.py'), '--todos'], cwd=raiz, capture_output=True, text=True)
    saida = r.stdout
    achados = {}
    for linha in saida.split('\n'):
        m = re.match(r'^(ERRO|AVISO|NOTA)\s+(\S+):(\d+)\s+(.*)$', linha)
        if m and m.group(2).endswith('TSTHero.cpp'):
            achados.setdefault(int(m.group(3)), []).append(m.group(4))
    espera, limpo = esperados(FONTE)
    falhas = []
    for n, trecho in sorted(espera.items()):
        if not any(trecho in msg for msg in achados.get(n, [])):
            falhas.append(f'linha {n}: esperava "{trecho}", veio {achados.get(n, [])}')
    for n in sorted(limpo):
        if n in achados:
            falhas.append(f'linha {n}: não devia acusar, veio {achados[n]}')
    if falhas:
        print('FALHOU:\n  ' + '\n  '.join(falhas))
        print('\nSaída do conferidor:\n' + saida)
        return 1
    print(f'OK: {len(espera)} sombras acusadas e {len(limpo)} linhas limpas sem acusação.')
    return 0


if __name__ == '__main__':
    sys.exit(main())
