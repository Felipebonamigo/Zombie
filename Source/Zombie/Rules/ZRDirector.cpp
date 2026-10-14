#include "Rules/ZRDirector.h"

#include <algorithm>
#include <cmath>

namespace ZR
{
	namespace
	{
		void EnterPhase(FDirectorState& State, EDirectorPhase Phase, FDirectorOutput& Out)
		{
			State.Phase = Phase;
			State.PhaseTime = 0.0f;
			Out.bPhaseChanged = true;
		}
	}

	FDirectorOutput TickDirector(const FDirectorConfig& Config, FDirectorState& State, const FDirectorInput& Input)
	{
		FDirectorOutput Out;
		const float Dt = std::max(0.0f, Input.Dt);

		State.Intensity += Input.DamageTaken * Config.DamageToIntensity
			+ static_cast<float>(Input.KillsNear) * Config.KillNearToIntensity
			+ static_cast<float>(Input.Explosions) * Config.ExplosionToIntensity;
		State.Intensity = std::clamp(State.Intensity - Config.IntensityDecayPerSecond * Dt, 0.0f, 100.0f);
		State.Noise += std::max(0.0f, Input.NoiseAdded);
		State.PhaseTime += Dt;

		switch (State.Phase)
		{
		case EDirectorPhase::BuildUp:
			if (State.Intensity >= Config.PeakIntensity || State.PhaseTime >= Config.MaxBuildUpSeconds)
			{
				EnterPhase(State, EDirectorPhase::Peak, Out);
			}
			break;
		case EDirectorPhase::Peak:
			if (State.PhaseTime >= Config.PeakSeconds)
			{
				EnterPhase(State, EDirectorPhase::Relax, Out);
			}
			break;
		case EDirectorPhase::Relax:
			if (State.PhaseTime >= Config.RelaxSeconds && State.Intensity <= Config.RelaxIntensity)
			{
				EnterPhase(State, EDirectorPhase::BuildUp, Out);
			}
			break;
		}

		Out.bHordeActive = State.HordeSecondsLeft > 0.0f;
		float Rate = 0.0f;
		if (Out.bHordeActive)
		{
			Rate = Config.PeakSpawnPerSecond * Config.HordeMultiplier;
		}
		else if (State.Phase == EDirectorPhase::Peak)
		{
			Rate = Config.PeakSpawnPerSecond;
		}
		else if (State.Phase == EDirectorPhase::BuildUp)
		{
			Rate = Config.BuildUpSpawnPerSecond;
		}
		else
		{
			Rate = Config.RelaxSpawnPerSecond;
		}
		if (Input.bNight)
		{
			Rate *= Config.NightMultiplier;
		}
		Rate += State.Noise * Config.NoiseToSpawn;

		State.SpawnAccumulator += static_cast<double>(Rate) * static_cast<double>(Dt);
		// a folga de 1e-6 absorve o erro de somar passos em float (10 x 0,1 s tem que dar 1 zumbi, não 0)
		const int Whole = static_cast<int>(std::floor(State.SpawnAccumulator + 1e-6));
		State.SpawnAccumulator = std::max(0.0, State.SpawnAccumulator - Whole);
		const int Cap = Input.bNight ? Config.MaxAliveNight : Config.MaxAlive;
		const int Room = std::max(0, Cap - Input.Alive);
		Out.SpawnNow = std::min(Whole, Room);

		State.Noise = std::max(0.0f, State.Noise - Config.NoiseDecayPerSecond * Dt);
		State.HordeSecondsLeft = std::max(0.0f, State.HordeSecondsLeft - Dt);
		return Out;
	}

	void StartHorde(FDirectorState& State, float Seconds)
	{
		State.HordeSecondsLeft = std::max(State.HordeSecondsLeft, Seconds);
	}
}
