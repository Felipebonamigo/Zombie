// Corpo do zumbi: vida por membro, desmembramento e explosão em pedaços. Regras puras: sem Unreal.
// Contrato: docs/regras-de-jogo.md, seção 4.
//
//  - Tiro tira vida do membro atingido; TorsoTransfer do dano vai também para o tronco (atirar nas pernas acaba matando).
//  - Membro com vida <= 0 sai do corpo (a cabeça "estoura"). Tiro em membro que já saiu não faz nada.
//  - Morre quando a cabeça sai ou o tronco zera. Sem uma perna vira rastejante. Cadáver ainda desmembra, sem contar abate.
//  - Explosão: dano >= GibDamage despedaça tudo; abaixo disso cada membro leva Dano x BlastShare do membro.

#pragma once

#include <cstdint>

namespace ZR
{
	enum class ELimb : std::uint8_t
	{
		Head = 0,
		Torso,
		ArmL,
		ArmR,
		LegL,
		LegR,
	};

	constexpr int LimbCount = 6;
	constexpr int AllLimbsMask = (1 << LimbCount) - 1;

	constexpr int LimbBit(ELimb Limb)
	{
		return 1 << static_cast<int>(Limb);
	}

	struct FBodyConfig
	{
		float Health[LimbCount] = {40.0f, 150.0f, 50.0f, 50.0f, 70.0f, 70.0f};
		/** Parte do dano de um tiro em braço, perna ou cabeça que também vai para o tronco. */
		float TorsoTransfer = 0.25f;
		/** Dano de explosão (já com a queda pela distância) que despedaça o corpo inteiro. */
		float GibDamage = 220.0f;
		/** Quanto do dano da explosão cada membro leva. */
		float BlastShare[LimbCount] = {0.6f, 1.0f, 0.8f, 0.8f, 0.8f, 0.8f};
	};

	struct FBodyState
	{
		float Health[LimbCount] = {0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f};
		bool Severed[LimbCount] = {false, false, false, false, false, false};
		bool bDead = false;
		bool bGibbed = false;

		void Reset(const FBodyConfig& Config);
		bool IsCrawler() const;
		int ArmsLeft() const;
	};

	struct FHitOutcome
	{
		/** Este golpe matou (cadáver atingido não conta). */
		bool bKilled = false;
		bool bGibbed = false;
		/** Morte porque a cabeça saiu com um tiro (explosão não conta como tiro na cabeça). */
		bool bHeadshotKill = false;
		/** Membros que saíram com este golpe (LimbBit). */
		int SeveredMask = 0;
		/** Perdeu a primeira perna com este golpe. */
		bool bBecameCrawler = false;
	};

	FHitOutcome ApplyHit(const FBodyConfig& Config, FBodyState& State, ELimb Limb, float Damage);
	FHitOutcome ApplyBlast(const FBodyConfig& Config, FBodyState& State, float Damage);

	struct FBlastConfig
	{
		/** Dentro deste raio o dano é cheio [cm]. */
		float InnerRadiusCm = 200.0f;
		/** Daqui para fora não há dano [cm]; entre os dois raios o dano cai em linha reta. */
		float OuterRadiusCm = 700.0f;
		float Damage = 400.0f;
		/** Impulso físico no centro (cai como o dano) [cm/s de mudança de velocidade, para corpos sem massa]. */
		float Impulse = 1800.0f;
	};

	/** Fração do dano na distância: 1 até o raio interno, 0 a partir do externo. */
	float BlastFalloff(const FBlastConfig& Blast, float DistanceCm);
	float BlastDamageAt(const FBlastConfig& Blast, float DistanceCm);
}
