#include "Rules/ZRLive.h"

#include <algorithm>

namespace ZR
{
	void FMilestoneTracker::Reset(const std::vector<FMilestoneDef>& Defs, float StartFollowers)
	{
		Paid.assign(Defs.size(), false);
		for (std::size_t i = 0; i < Defs.size(); ++i)
		{
			Paid[i] = StartFollowers >= Defs[i].Followers;
		}
	}

	std::vector<int> FMilestoneTracker::Update(const std::vector<FMilestoneDef>& Defs, float Followers)
	{
		if (Paid.size() < Defs.size())
		{
			Paid.resize(Defs.size(), false);
		}
		std::vector<int> Crossed;
		for (std::size_t i = 0; i < Defs.size(); ++i)
		{
			if (!Paid[i] && Followers >= Defs[i].Followers)
			{
				Paid[i] = true;
				Crossed.push_back(static_cast<int>(i));
			}
		}
		// em ordem de seguidores, mesmo que a tabela não esteja
		std::sort(Crossed.begin(), Crossed.end(), [&Defs](int A, int B)
		{
			return Defs[static_cast<std::size_t>(A)].Followers < Defs[static_cast<std::size_t>(B)].Followers;
		});
		return Crossed;
	}

	int FMilestoneTracker::NextIndex(const std::vector<FMilestoneDef>& Defs) const
	{
		int Best = -1;
		for (std::size_t i = 0; i < Defs.size(); ++i)
		{
			const bool bPaid = i < Paid.size() && Paid[i];
			if (!bPaid && (Best < 0 || Defs[i].Followers < Defs[static_cast<std::size_t>(Best)].Followers))
			{
				Best = static_cast<int>(i);
			}
		}
		return Best;
	}

	void FLiveStats::Reset(float InStartFollowers)
	{
		*this = FLiveStats();
		StartFollowers = InStartFollowers;
		Followers = InStartFollowers;
	}

	void FLiveStats::Sample(double Seconds, float Viewers, float CurrentFollowers, int Combo)
	{
		LiveSeconds = Seconds;
		PeakViewers = std::max(PeakViewers, Viewers);
		Followers = CurrentFollowers;
		BestCombo = std::max(BestCombo, Combo);
	}

	float FLiveStats::FollowersGained() const
	{
		return std::max(0.0f, Followers - StartFollowers);
	}

	char LiveGrade(const FGradeConfig& Config, const FLiveStats& Stats)
	{
		const float Peak = Stats.PeakViewers;
		if (Peak >= Config.SViewers)
		{
			return 'S';
		}
		if (Peak >= Config.AViewers)
		{
			return 'A';
		}
		if (Peak >= Config.BViewers)
		{
			return 'B';
		}
		return Peak >= Config.CViewers ? 'C' : 'D';
	}
}
