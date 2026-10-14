#pragma once

#include "CoreMinimal.h"
#include "Engine/EngineTypes.h"

/** Categoria de log principal do jogo. */
DECLARE_LOG_CATEGORY_EXTERN(LogZombie, Log, All);

/**
 * Canal de colisão dos zumbis (ECC_GameTraceChannel1, "Zombie" no Config/DefaultEngine.ini). Com canal próprio o carro
 * deixa o zumbi passar por sobreposição (atropela em vez de bater num poste) e o jogador continua batendo no carro.
 */
constexpr ECollisionChannel ECC_ZMBZombie = ECC_GameTraceChannel1;
