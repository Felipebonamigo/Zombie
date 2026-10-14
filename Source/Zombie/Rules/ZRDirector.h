// Diretor da horda, no espírito do AI Director do Left 4 Dead: decide QUANTOS zumbis nascem agora, num ciclo de
// tensão (BuildUp), pico (Peak) e respiro (Relax). A noite multiplica, a Noite da Horda (ou o chat) multiplica mais, e o
// barulho do jogador chama zumbi extra. Onde e qual tipo nascem fica com o AZMBHordeDirector no Unreal.
// Regras puras: sem Unreal. Contrato: docs/regras-de-jogo.md, seção 6.
//
// Passo de TickDirector, nesta ordem:
//  1. Intensidade += dano levado, abates por perto e explosões (com os pesos da config); depois cai IntensityDecay x dt.
//  2. Barulho += NoiseAdded.
//  3. Troca de fase: BuildUp -> Peak (intensidade >= PeakIntensity ou tempo >= MaxBuildUpSeconds); Peak -> Relax
//     (tempo >= PeakSeconds); Relax -> BuildUp (tempo >= RelaxSeconds e intensidade <= RelaxIntensity).
//  4. Ritmo = ritmo da fase (horda ativa: ritmo do pico x HordeMultiplier) x NightMultiplier à noite
//     + Barulho x NoiseToSpawn. Acumula ritmo x dt; nasce a parte inteira, sem passar do teto de vivos (o que passa
//     do teto é descartado, não fica guardado).
//  5. Barulho cai NoiseDecay x dt; o tempo de horda corre.

#pragma once

#include <cstdint>

namespace ZR
{
	enum class EDirectorPhase : std::uint8_t
	{
		BuildUp,
		Peak,
		Relax,
	};

	struct FDirectorConfig
	{
		float PeakIntensity = 70.0f;
		float RelaxIntensity = 25.0f;
		float MaxBuildUpSeconds = 90.0f;
		float PeakSeconds = 25.0f;
		float RelaxSeconds = 20.0f;
		float IntensityDecayPerSecond = 3.0f;
		/** Intensidade por ponto de vida perdido pelo jogador. */
		float DamageToIntensity = 0.6f;
		float KillNearToIntensity = 1.5f;
		float ExplosionToIntensity = 4.0f;
		float BuildUpSpawnPerSecond = 0.5f;
		float PeakSpawnPerSecond = 2.5f;
		float RelaxSpawnPerSecond = 0.05f;
		float NightMultiplier = 2.5f;
		float HordeMultiplier = 4.0f;
		int MaxAlive = 80;
		int MaxAliveNight = 140;
		float NoiseDecayPerSecond = 10.0f;
		/** Zumbis extras por segundo, por ponto de barulho. */
		float NoiseToSpawn = 0.02f;
	};

	struct FDirectorState
	{
		EDirectorPhase Phase = EDirectorPhase::BuildUp;
		float PhaseTime = 0.0f;
		float Intensity = 0.0f;
		float Noise = 0.0f;
		double SpawnAccumulator = 0.0;
		float HordeSecondsLeft = 0.0f;
	};

	struct FDirectorInput
	{
		float Dt = 0.0f;
		int Alive = 0;
		bool bNight = false;
		float DamageTaken = 0.0f;
		int KillsNear = 0;
		int Explosions = 0;
		float NoiseAdded = 0.0f;
	};

	struct FDirectorOutput
	{
		int SpawnNow = 0;
		bool bPhaseChanged = false;
		bool bHordeActive = false;
	};

	FDirectorOutput TickDirector(const FDirectorConfig& Config, FDirectorState& State, const FDirectorInput& Input);

	/** Começa (ou estende) uma horda de Seconds segundos: Noite da Horda ou pedido do chat. */
	void StartHorde(FDirectorState& State, float Seconds);
}
