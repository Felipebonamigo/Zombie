#include "Rules/ZRChallenges.h"

#include <algorithm>

namespace ZR
{
	namespace
	{
		constexpr float ChallengeRetrySeconds = 5.0f;
	}

	FChallengeTracker::FChallengeTracker(const FChallengeConfig& InConfig, std::uint64_t Seed)
		: Config(InConfig), Rng(Seed), Wait(InConfig.FirstDelaySeconds)
	{
	}

	void FChallengeTracker::Configure(const FChallengeConfig& InConfig)
	{
		Config = InConfig;
		if (Active < 0)
		{
			Wait = std::min(Wait, Config.FirstDelaySeconds);
		}
	}

	int FChallengeTracker::Pick(const std::vector<FChallengeDef>& Defs, bool bNight)
	{
		std::vector<int> Eligible;
		for (std::size_t i = 0; i < Defs.size(); ++i)
		{
			if (Defs[i].Weight > 0.0f && (!Defs[i].bNightOnly || bNight))
			{
				Eligible.push_back(static_cast<int>(i));
			}
		}
		// sem repetir a anterior quando há outra
		if (Eligible.size() > 1)
		{
			Eligible.erase(std::remove(Eligible.begin(), Eligible.end(), Last), Eligible.end());
		}
		if (Eligible.empty())
		{
			return -1;
		}
		double Total = 0.0;
		for (int Index : Eligible)
		{
			Total += Defs[static_cast<std::size_t>(Index)].Weight;
		}
		double Roll = Rng.Unit() * Total;
		for (int Index : Eligible)
		{
			Roll -= Defs[static_cast<std::size_t>(Index)].Weight;
			if (Roll < 0.0)
			{
				return Index;
			}
		}
		return Eligible.back();
	}

	FChallengeUpdate FChallengeTracker::Tick(const std::vector<FChallengeDef>& Defs, float DeltaSeconds, float Viewers, bool bNight)
	{
		FChallengeUpdate Update;
		const float Dt = std::max(0.0f, DeltaSeconds);
		if (Active >= 0)
		{
			Remaining -= Dt;
			if (Remaining <= 0.0f)
			{
				Update.Result = EChallengeResult::Failed;
				Update.Index = Active;
				Active = -1;
				Remaining = 0.0f;
				Wait = Config.CooldownSeconds;
			}
			return Update;
		}
		Wait -= Dt;
		if (Wait > 0.0f || Viewers < Config.MinViewers)
		{
			return Update;
		}
		const int Chosen = Pick(Defs, bNight);
		if (Chosen < 0)
		{
			Wait = ChallengeRetrySeconds;
			return Update;
		}
		Active = Chosen;
		Last = Chosen;
		Progress = 0;
		Remaining = Defs[static_cast<std::size_t>(Chosen)].TimeLimitSeconds;
		Update.Result = EChallengeResult::Started;
		Update.Index = Chosen;
		return Update;
	}

	FChallengeUpdate FChallengeTracker::OnEvent(const std::vector<FChallengeDef>& Defs, int EventIndex, int Count)
	{
		FChallengeUpdate Update;
		if (Active < 0 || static_cast<std::size_t>(Active) >= Defs.size())
		{
			return Update;
		}
		const FChallengeDef& Def = Defs[static_cast<std::size_t>(Active)];
		if (Def.EventIndex != EventIndex)
		{
			return Update;
		}
		Progress += std::max(1, Count);
		if (Progress < Def.Target)
		{
			return Update;
		}
		Update.Result = EChallengeResult::Completed;
		Update.Index = Active;
		Update.Reward = Def.Reward;
		Update.HypeBonus = Def.HypeBonus;
		Active = -1;
		Remaining = 0.0f;
		Wait = Config.CooldownSeconds;
		return Update;
	}
}
