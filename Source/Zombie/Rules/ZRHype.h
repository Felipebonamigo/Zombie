// Hype da live: o coração do "AO VIVO". Cada coisa espetacular que o jogador faz vira pontos de hype; o hype puxa os
// espectadores; espectadores com hype viram seguidores; e passar de certos níveis de hype dispara doações (dinheiro
// para os pedidos de entrega por drone). Regras puras: sem Unreal. Contrato: docs/regras-de-jogo.md, seção 5.
//
//  - Combo: eventos dentro de ComboWindowSeconds do anterior somam combo; multiplicador 1 + ComboStep x (combo - 1),
//    com teto ComboMaxMultiplier.
//  - Variedade: repetir o MESMO evento seguido rende VarietyFalloff^repetições (mínimo VarietyMin): o chat cansa.
//  - Queda: passados DecayGraceSeconds sem evento, o hype cai DecayPerSecond.
//  - Espectadores seguem o alvo TargetViewers com tempos de subida e descida diferentes.
//  - Seguidores crescem com espectadores x hype, num logístico com teto FollowerCap.
//  - Doação: cada limiar de DonationThresholds doa uma vez por subida; rearma quando o hype cai DonationRearm abaixo dele.

#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace ZR
{
	/** Eventos que rendem hype. A ordem é a das linhas esperadas em Data/Hype.csv (coluna id). */
	enum class EHypeEvent : std::uint8_t
	{
		Kill,             // abate
		Headshot,         // tiro_na_cabeca
		Dismember,        // desmembrou
		Gib,              // despedacou
		MultiKill,        // abate_multiplo (por zumbi além do primeiro na mesma explosão)
		ChainExplosion,   // explosao_em_cadeia (por elo)
		VehicleKill,      // atropelou
		SlowMoKill,       // abate_mira_lenta
		NamedZombieKill,  // zumbi_do_chat
		CloseCall,        // escapou_por_pouco
		LongShot,         // tiro_de_longe
		TookHit,          // apanhou
		Count
	};

	/** Id de cada evento na tabela, na ordem do enum. */
	const char* HypeEventId(EHypeEvent Event);

	struct FHypeEventDef
	{
		std::string Id;
		float Points = 0.0f;
		std::string LabelPt;
		std::string LabelEn;
	};

	struct FHypeConfig
	{
		float ComboWindowSeconds = 4.0f;
		float ComboStep = 0.25f;
		float ComboMaxMultiplier = 4.0f;
		float VarietyFalloff = 0.75f;
		float VarietyMin = 0.3f;
		float DecayGraceSeconds = 5.0f;
		float DecayPerSecond = 4.0f;
		/** Fração dos seguidores assistindo com hype zero. */
		float ViewerFloor = 0.05f;
		float ViewerExponent = 1.5f;
		/** Acima deste hype (0 a 100) a live viraliza: espectadores passam dos seguidores. */
		float ViralStartHype = 80.0f;
		/** Com hype 100, espectadores = seguidores x (1 + ViralBoost). */
		float ViralBoost = 2.0f;
		float ViewersRiseSeconds = 3.0f;
		float ViewersFallSeconds = 12.0f;
		float FollowRatePerSecond = 0.01f;
		float FollowerCap = 10000000.0f;
		float StartFollowers = 37.0f;
		std::vector<float> DonationThresholds = {50.0f, 75.0f, 90.0f};
		float DonationRearm = 20.0f;
		/** Reais por espectador no limiar 1; o limiar n doa n vezes isso. */
		float DonationPerViewer = 0.02f;
	};

	struct FHypeState
	{
		float Hype = 0.0f;
		float Viewers = 0.0f;
		float Followers = 0.0f;
		int Combo = 0;
		float ComboTimer = 0.0f;
		float SinceLastEvent = 1.0e9f;
		int LastEvent = -1;
		int RepeatCount = 0;
		std::vector<bool> ThresholdArmed;
		double TotalDonations = 0.0;

		void Reset(const FHypeConfig& Config);
	};

	struct FHypeGain
	{
		float Points = 0.0f;
		float Multiplier = 1.0f;
		float Variety = 1.0f;
		int Combo = 0;
	};

	struct FDonation
	{
		float Amount = 0.0f;
		int ThresholdIndex = 0;
	};

	/** Soma um evento. EventIndex identifica o tipo para a variedade (normalmente o EHypeEvent); Count multiplica a base. */
	FHypeGain AddHypeEvent(const FHypeConfig& Config, FHypeState& State, int EventIndex, float BasePoints, int Count = 1);

	/** Avança o tempo; as doações deste passo entram no fim de OutDonations. */
	void TickHype(const FHypeConfig& Config, FHypeState& State, float DeltaSeconds, std::vector<FDonation>& OutDonations);

	float TargetViewers(const FHypeConfig& Config, float Hype, float Followers);
}
