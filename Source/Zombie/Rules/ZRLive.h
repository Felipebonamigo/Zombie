// Progressão da live dentro da sessão: os marcos de seguidores (100, 1.000, 10.000...) com o prêmio de cada um, e o resumo
// da live que aparece quando ela cai (o "card" que o jogador printa e posta). Regras puras: sem Unreal.
// Contrato: docs/regras-de-jogo.md, seção 14.
//
//  - Marco: paga uma vez só, na ordem; os que já estavam passados no começo da live não pagam; vários cruzados no mesmo
//    passo pagam todos.
//  - Resumo: pico de espectadores, seguidores ganhos, abates, tiros na cabeça, maior combo, maior cadeia, doações, pautas,
//    clipes e quedas da live, e a nota de S a D pelo pico de espectadores (limites em resumo.* de Data/Ajustes.csv).

#pragma once

#include <string>
#include <vector>

namespace ZR
{
	/** Uma linha de Data/Marcos.csv. */
	struct FMilestoneDef
	{
		std::string Id;
		float Followers = 100.0f;
		std::string TextPt;
		std::string TextEn;
		int Scrap = 0;
		int Grenades = 0;
		int Bombs = 0;
		/** Manda uma caixa por drone. */
		bool bAirdrop = false;
		float HypeBonus = 0.0f;
	};

	class FMilestoneTracker
	{
	public:
		/** Começo da live: os marcos que StartFollowers já passou ficam como pagos. */
		void Reset(const std::vector<FMilestoneDef>& Defs, float StartFollowers);

		/** Índices dos marcos cruzados agora, em ordem crescente de seguidores; cada um volta uma vez só. */
		std::vector<int> Update(const std::vector<FMilestoneDef>& Defs, float Followers);

		/** O próximo marco a alcançar (o de menos seguidores ainda não pago), ou -1. */
		int NextIndex(const std::vector<FMilestoneDef>& Defs) const;

	private:
		std::vector<bool> Paid;
	};

	struct FGradeConfig
	{
		/** Pico de espectadores para cada nota; abaixo de C é D. */
		float CViewers = 25.0f;
		float BViewers = 100.0f;
		float AViewers = 500.0f;
		float SViewers = 2000.0f;
	};

	/** O resumo da live. */
	struct FLiveStats
	{
		double LiveSeconds = 0.0;
		float StartFollowers = 0.0f;
		float Followers = 0.0f;
		float PeakViewers = 0.0f;
		int Kills = 0;
		int Headshots = 0;
		int BestCombo = 0;
		int BestChain = 0;
		double Donations = 0.0;
		int ChallengesDone = 0;
		int ChallengesFailed = 0;
		int Clips = 0;
		int Deaths = 0;

		/** Começa a live com estes seguidores. */
		void Reset(float InStartFollowers);
		/** A cada passo: guarda os picos e os seguidores atuais. */
		void Sample(double Seconds, float Viewers, float CurrentFollowers, int Combo);
		/** Seguidores ganhos na live (nunca negativo). */
		float FollowersGained() const;
	};

	/** 'S', 'A', 'B', 'C' ou 'D' pelo pico de espectadores. */
	char LiveGrade(const FGradeConfig& Config, const FLiveStats& Stats);
}
