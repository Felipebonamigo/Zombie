// Pautas da live: desafios de patrocinador com prazo ("3 explosões em cadeia em 2 minutos"). Cumprir paga doação (vira
// entrega por drone) e dá hype; estourar o prazo só passa a vez. Fecha o ciclo hype -> dinheiro -> drone e empurra o
// jogador para o lance que vira clipe. Regras puras: sem Unreal. Contrato: docs/regras-de-jogo.md, seção 11.
//
//  - A primeira pauta vem FirstDelaySeconds depois do começo; as seguintes, CooldownSeconds depois da anterior acabar
//    (cumprida ou não). Só começa com pelo menos MinViewers assistindo (patrocinador não paga live vazia).
//  - Sorteio pelo peso entre as elegíveis (as "só de noite" só à noite), sem repetir a anterior quando há outra.
//    Nenhuma elegível: tenta de novo em 5 s.
//  - Conta só o evento de hype da pauta (Count de cada lance); chegou ao alvo dentro do prazo, cumpriu.

#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "Rules/ZRRandom.h"

namespace ZR
{
	struct FChallengeDef
	{
		std::string Id;
		std::string TextPt;
		std::string TextEn;
		/** Índice do EHypeEvent que conta. */
		int EventIndex = 0;
		int Target = 1;
		float TimeLimitSeconds = 60.0f;
		/** Doação paga ao cumprir [R$]. */
		float Reward = 0.0f;
		/** Hype somado ao cumprir. */
		float HypeBonus = 0.0f;
		float Weight = 1.0f;
		bool bNightOnly = false;
	};

	struct FChallengeConfig
	{
		float FirstDelaySeconds = 45.0f;
		float CooldownSeconds = 40.0f;
		float MinViewers = 10.0f;
	};

	enum class EChallengeResult : std::uint8_t
	{
		None,
		Started,
		Completed,
		Failed,
	};

	struct FChallengeUpdate
	{
		EChallengeResult Result = EChallengeResult::None;
		int Index = -1;
		float Reward = 0.0f;
		float HypeBonus = 0.0f;
	};

	class FChallengeTracker
	{
	public:
		explicit FChallengeTracker(const FChallengeConfig& InConfig = FChallengeConfig(), std::uint64_t Seed = 1);

		void Configure(const FChallengeConfig& InConfig);

		/** Avança o relógio: pode começar uma pauta (Started) ou estourar o prazo da ativa (Failed). */
		FChallengeUpdate Tick(const std::vector<FChallengeDef>& Defs, float DeltaSeconds, float Viewers, bool bNight);

		/** Um lance do jogo (EHypeEvent e quantidade): pode cumprir a pauta ativa (Completed). */
		FChallengeUpdate OnEvent(const std::vector<FChallengeDef>& Defs, int EventIndex, int Count);

		/** Índice da pauta ativa em Defs, ou -1. */
		int ActiveIndex() const { return Active; }
		int ProgressCount() const { return Progress; }
		float TimeLeft() const { return Remaining; }
		float CooldownLeft() const { return Wait; }

	private:
		int Pick(const std::vector<FChallengeDef>& Defs, bool bNight);

		FChallengeConfig Config;
		FRng Rng;
		int Active = -1;
		int Progress = 0;
		float Remaining = 0.0f;
		float Wait = 0.0f;
		int Last = -1;
	};
}
