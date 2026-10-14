// Tipos pequenos que o combate, a live e o HUD trocam entre si. Sem UCLASS: só structs e enums de C++.

#pragma once

#include "CoreMinimal.h"

class AController;

/** O que matou o zumbi (decide os eventos de hype). */
enum class EZMBKillCause : uint8
{
	Bullet,
	Blast,
	Vehicle,
	Kick,
	/** Armadilha da base (estacas, arame; o bujão armado conta como explosão). */
	Trap,
};

/** Quem e como atingiu: viaja do tiro, do chute, do carro ou da explosão até a morte do zumbi. */
struct FZMBHitContext
{
	TWeakObjectPtr<AController> Instigator;
	EZMBKillCause Cause = EZMBKillCause::Bullet;
	/** O jogador estava na mira lenta. */
	bool bSlowMo = false;
	/** Distância do jogador até o ponto atingido [cm]. */
	float DistanceCm = 0.0f;
	/** Profundidade da explosão em cadeia (0 = a primeira). */
	int32 ChainDepth = 0;
};

/** Uma morte de zumbi, do jeito que a live conta. */
struct FZMBKillInfo
{
	EZMBKillCause Cause = EZMBKillCause::Bullet;
	bool bHeadshot = false;
	bool bGibbed = false;
	bool bSlowMo = false;
	bool bNamed = false;
	int32 SeveredLimbs = 0;
	float DistanceCm = 0.0f;
	FVector Location = FVector::ZeroVector;
	FString ViewerName;
};

/** Pedido que o chat (de verdade ou simulado) faz ao Diretor. */
enum class EZMBChatActionKind : uint8
{
	NamedZombie,
	Horde,
	BombSquad,
	Airdrop,
};

struct FZMBChatAction
{
	EZMBChatActionKind Kind = EZMBChatActionKind::Horde;
	FString ViewerName;
};
