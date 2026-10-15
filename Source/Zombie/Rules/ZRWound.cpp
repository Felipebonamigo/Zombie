#include "Rules/ZRWound.h"

#include <algorithm>

namespace ZR
{
	bool TryOpenWound(const FWoundConfig& Config, FWoundState& State, float Damage, double Roll01)
	{
		if (Damage < Config.MinDamage || Roll01 >= static_cast<double>(Config.Chance) || Config.MaxWounds <= 0)
		{
			return false;
		}
		if (State.Count() < Config.MaxWounds)
		{
			State.Left.push_back(Config.Seconds);
			return true;
		}
		// todos abertos: a mordida nova reabre o que ia fechar primeiro
		*std::min_element(State.Left.begin(), State.Left.end()) = Config.Seconds;
		return true;
	}

	float TickWounds(const FWoundConfig& Config, FWoundState& State, float Dt)
	{
		if (Dt <= 0.0f)
		{
			return 0.0f;
		}
		float Lost = 0.0f;
		for (float& Left : State.Left)
		{
			// o ferimento que fecha no meio do passo sangra só o que faltava
			Lost += std::min(Left, Dt) * Config.DamagePerSecond;
			Left -= Dt;
		}
		State.Left.erase(std::remove_if(State.Left.begin(), State.Left.end(), [](float Left) { return Left <= 0.0f; }), State.Left.end());
		return Lost;
	}

	void StopBleeding(FWoundState& State)
	{
		State.Left.clear();
	}
}
