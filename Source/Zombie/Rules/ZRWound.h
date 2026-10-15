// Ferimentos que sangram, sem microgerenciamento: mordida forte de zumbi pode abrir um ferimento; cada ferimento aberto
// tira vida aos poucos e trava a cura sozinha, e fecha sozinho depois de um tempo. Curativo (saque, caixa do drone)
// estanca todos. Regras puras: sem Unreal. Contrato: docs/regras-de-jogo.md, seção 17.

#pragma once

#include <vector>

namespace ZR
{
	struct FWoundConfig
	{
		/** Mordida com menos dano que isto não abre ferimento. */
		float MinDamage = 8.0f;
		/** Chance de a mordida forte abrir um ferimento (0 a 1). */
		float Chance = 0.3f;
		/** Ferimentos abertos ao mesmo tempo; com todos abertos, a mordida nova renova o que vai fechar primeiro. */
		int MaxWounds = 3;
		/** Vida perdida por segundo por ferimento. */
		float DamagePerSecond = 0.4f;
		/** Quanto tempo um ferimento sangra antes de fechar sozinho [s]. */
		float Seconds = 30.0f;
	};

	struct FWoundState
	{
		/** Segundos que faltam para cada ferimento aberto fechar. */
		std::vector<float> Left;

		int Count() const
		{
			return static_cast<int>(Left.size());
		}
	};

	/** A mordida de Damage abre ferimento? Roll01 é o sorteio em [0, 1) (a chance passa se Roll01 < Chance). */
	bool TryOpenWound(const FWoundConfig& Config, FWoundState& State, float Damage, double Roll01);

	/** Passa Dt segundos: devolve a vida perdida neles e fecha os ferimentos que venceram (o último pedaço conta só o que faltava). */
	float TickWounds(const FWoundConfig& Config, FWoundState& State, float Dt);

	/** Curativo: fecha todos. */
	void StopBleeding(FWoundState& State);
}
