// Uma explosão de verdade no mundo: dano com queda pela distância (ZR::BlastDamageAt), zumbis despedaçados, explosivos
// vizinhos acesos (a reação em cadeia nasce daqui), empurrão em tudo que tem física, tremor de câmera, barulho para o
// Diretor e a conta da live (abate múltiplo e explosão em cadeia). Contrato: docs/regras-de-jogo.md, seções 4 e 5.

#pragma once

#include "CoreMinimal.h"
#include "Rules/ZRBody.h"

class AActor;
class AController;
class UWorld;

namespace ZMBExplosion
{
	struct FResult
	{
		int32 Kills = 0;
		int32 Gibs = 0;
	};

	/**
	 * Source é ignorado pela consulta (o próprio bujão ou zumbi que explodiu). ChainDepth: 0 para a primeira explosão; quem
	 * for aceso por esta recebe ChainDepth + 1. Noise vai para o Diretor.
	 */
	FResult Detonate(UWorld* World, const FVector& Origin, const ZR::FBlastConfig& Blast, AController* Instigator, AActor* Source,
		int32 ChainDepth, float Noise);
}
