// Tabelas de ajuste do jogo (Data/*.csv) lidas para structs puras. Sem Unreal: o UZMBGameData lê os arquivos e chama
// estes carregadores; os testes de deriva carregam as mesmas tabelas na nuvem. Colunas e unidades: docs/dados.md.
//
// Regra: erro de dado nunca derruba o jogo. Linha ruim é pulada (ou o valor fica no padrão) e vira uma mensagem em
// Errors, que o jogo mostra no log e os testes transformam em falha.

#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "Rules/ZRBody.h"
#include "Rules/ZRChallenges.h"
#include "Rules/ZRChat.h"
#include "Rules/ZRCityPlan.h"
#include "Rules/ZRDayNight.h"
#include "Rules/ZRDirector.h"
#include "Rules/ZRHype.h"

namespace ZR
{
	struct FZombieType
	{
		std::string Id;
		std::string NamePt;
		FBodyConfig Body;
		/** Velocidade andando e correndo atrás do jogador [cm/s]. */
		float WalkSpeed = 150.0f;
		float RunSpeed = 300.0f;
		float AttackDamage = 10.0f;
		float AttackIntervalSeconds = 1.2f;
		float AttackRangeCm = 140.0f;
		/** Explode ao morrer (o Bujão, o Inchado). */
		bool bExplodes = false;
		FBlastConfig Blast;
		float SpawnWeight = 1.0f;
		float NightSpawnWeight = 1.0f;
		float Scale = 1.0f;
		std::uint32_t ColorRgb = 0x6B7A5A;
	};

	struct FWeaponType
	{
		std::string Id;
		std::string NamePt;
		/** Tecla numérica (1 a 9). */
		int Slot = 1;
		/** Dano por projétil. */
		float Damage = 30.0f;
		float ShotsPerSecond = 3.0f;
		bool bAutomatic = false;
		int Magazine = 12;
		float ReloadSeconds = 1.5f;
		float SpreadDeg = 1.0f;
		int Pellets = 1;
		float RangeM = 120.0f;
		/** Impulso por projétil num pedaço solto [cm/s de mudança de velocidade]. */
		float Impulse = 300.0f;
		/** Barulho por tiro para o Diretor (chama zumbi). */
		float Noise = 2.0f;
		/** Munição reserva no começo. */
		int StartReserve = 60;
	};

	struct FExplosiveType
	{
		std::string Id;
		std::string NamePt;
		float Health = 20.0f;
		/** Depois de atingido, quanto tempo chiando até explodir [s] (0 = na hora). */
		float FuseSeconds = 0.8f;
		FBlastConfig Blast;
		float Noise = 40.0f;
		std::uint32_t ColorRgb = 0xB03A2E;
	};

	/** Câmera: três distâncias de terceira pessoa (perto, média, longe e alta) e a primeira pessoa (modo celular). */
	struct FCameraConfig
	{
		float NearDistanceCm = 260.0f;
		float NearHeightCm = 70.0f;
		float NearSideCm = 55.0f;
		float NearPitchDeg = -4.0f;
		float MidDistanceCm = 520.0f;
		float MidHeightCm = 170.0f;
		float MidSideCm = 35.0f;
		float MidPitchDeg = -10.0f;
		float FarDistanceCm = 900.0f;
		float FarHeightCm = 380.0f;
		float FarSideCm = 0.0f;
		float FarPitchDeg = -18.0f;
		float AimDistanceCm = 170.0f;
		float AimSideCm = 70.0f;
		float AimFov = 55.0f;
		float Fov = 80.0f;
		float FirstPersonFov = 88.0f;
		float LagSpeed = 10.0f;
		/** 0 perto, 1 média, 2 longe e alta, 3 primeira pessoa. */
		int StartMode = 2;
	};

	struct FPlayerConfig
	{
		float Health = 100.0f;
		float WalkSpeed = 400.0f;
		float SprintSpeed = 680.0f;
		float Stamina = 100.0f;
		float StaminaDrainPerSecond = 16.0f;
		float StaminaRegenPerSecond = 12.0f;
		float KickDamage = 40.0f;
		float KickImpulse = 900.0f;
		float KickRangeCm = 190.0f;
		/** Mira lenta (Dead Eye): o mundo anda nesta fração da velocidade. */
		float SlowMoTimeDilation = 0.3f;
		float FocusMax = 100.0f;
		/** Gasto do foco por segundo REAL na mira lenta. */
		float FocusDrainPerSecond = 22.0f;
		float FocusPerHeadshot = 12.0f;
		float FocusRegenPerSecond = 2.0f;
		float ThrowSpeed = 1700.0f;
		int StartGrenades = 5;
		int StartBombs = 3;
		/** Abaixo desta vida, sobreviver a um ataque conta "escapou por pouco". */
		float CloseCallHealth = 20.0f;
		/** Vida recuperada por segundo depois de RegenDelaySeconds sem apanhar (só até RegenCap). */
		float RegenPerSecond = 3.0f;
		float RegenDelaySeconds = 6.0f;
		float RegenCap = 60.0f;
	};

	/** Carro de protótipo: suspensão por raios em quatro rodas, sem malha esquelética. */
	struct FCarConfig
	{
		float MassKg = 1300.0f;
		float SuspensionRestCm = 55.0f;
		/** Força da mola por cm comprimido, por roda [kg·cm/s²]. */
		float SpringPerCm = 16000.0f;
		/** Amortecimento por cm/s, por roda [kg/s]. */
		float DamperPerCmS = 2300.0f;
		/** Forças em unidade do Unreal [kg·cm/s²]. */
		float EngineForce = 900000.0f;
		float BrakeForce = 1600000.0f;
		float MaxSpeedKmh = 150.0f;
		float SteerDeg = 32.0f;
		float SideGrip = 4.5f;
		/** Dano no zumbi atropelado por km/h. */
		float RunOverDamagePerKmh = 6.0f;
		/** Abaixo disso o carro só empurra [km/h]. */
		float RunOverMinKmh = 18.0f;
	};

	struct FTuning
	{
		FHypeConfig Hype;
		FDirectorConfig Director;
		FCityConfig City;
		FVoteConfig Vote;
		FDayConfig Day;
		FCameraConfig Camera;
		FPlayerConfig Player;
		FCarConfig Car;
		FChallengeConfig Challenge;
		/** Zumbis com o nome de espectadores vivos ao mesmo tempo. */
		int MaxNamedZombies = 12;
		/** Pedaços soltos (membros e entulho) no mundo ao mesmo tempo. */
		int MaxGibs = 220;
		/** Segundos que um cadáver fica antes de sumir. */
		float CorpseSeconds = 12.0f;
		/** Distância em que um zumbi longe do jogador some (e volta para o Diretor) [cm]. */
		float DespawnDistanceCm = 9000.0f;
		/** Anel onde o Diretor faz zumbi nascer [cm]. */
		float SpawnMinDistanceCm = 2800.0f;
		float SpawnMaxDistanceCm = 6500.0f;
		/** Quantos explosivos (bujões e barris) espalhar pelas ruas. */
		int StreetExplosives = 70;
		int StreetCars = 40;
	};

	void LoadZombies(const std::string& Csv, std::vector<FZombieType>& Out, std::vector<std::string>& Errors);
	void LoadWeapons(const std::string& Csv, std::vector<FWeaponType>& Out, std::vector<std::string>& Errors);
	void LoadExplosives(const std::string& Csv, std::vector<FExplosiveType>& Out, std::vector<std::string>& Errors);
	/** Out sai com EHypeEvent::Count linhas, na ordem do enum (eventos faltando ficam com 0 ponto e geram erro). */
	void LoadHypeEvents(const std::string& Csv, std::vector<FHypeEventDef>& Out, std::vector<std::string>& Errors);
	void LoadChatLines(const std::string& Csv, std::vector<FChatLine>& Out, std::vector<std::string>& Errors);
	/** Data/Pautas.csv: o evento tem que ser um id de EHypeEvent e a quantidade, pelo menos 1. */
	void LoadChallenges(const std::string& Csv, std::vector<FChallengeDef>& Out, std::vector<std::string>& Errors);

	/**
	 * Data/Ajustes.csv (chave,valor,unidade,descricao). Chave desconhecida, repetida ou valor ruim viram Errors (o valor
	 * fica no padrão); chaves conhecidas que faltaram vão para Missing (o padrão do código vale).
	 */
	void LoadTuning(const std::string& Csv, FTuning& Out, std::vector<std::string>& Errors, std::vector<std::string>& Missing);

	/** Todas as chaves que LoadTuning conhece, na ordem em que são registradas. */
	std::vector<std::string> TuningKeys();

	template <typename T>
	const T* FindById(const std::vector<T>& Items, const std::string& Id)
	{
		for (const T& Item : Items)
		{
			if (Item.Id == Id)
			{
				return &Item;
			}
		}
		return nullptr;
	}
}
