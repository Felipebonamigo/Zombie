#include "Rules/ZRHighlights.h"

namespace ZR
{
	void FHighlightTracker::Add(double Time, float Score, const std::string& Label)
	{
		FMoment Moment;
		Moment.Time = Time;
		Moment.Score = Score;
		Moment.Label = Label;
		Moments.push_back(Moment);
	}

	void FHighlightTracker::Prune(double Now)
	{
		while (!Moments.empty() && Moments.front().Time < Now - KeepSeconds)
		{
			Moments.pop_front();
		}
	}

	FHighlightTracker::FWindow FHighlightTracker::BestWindow(double Now, double WindowSeconds) const
	{
		FWindow Best;
		// janela que começa em cada momento; poucos momentos (dezenas), então o laço duplo basta
		for (std::size_t First = 0; First < Moments.size(); ++First)
		{
			const FMoment& Start = Moments[First];
			if (Start.Time < Now - KeepSeconds || Start.Time > Now)
			{
				continue;
			}
			FWindow Window;
			Window.bValid = true;
			Window.Start = Start.Time;
			Window.End = Start.Time;
			float TopScore = -1.0f;
			for (std::size_t i = First; i < Moments.size() && Moments[i].Time <= Start.Time + WindowSeconds && Moments[i].Time <= Now; ++i)
			{
				Window.Score += Moments[i].Score;
				Window.End = Moments[i].Time;
				if (Moments[i].Score > TopScore)
				{
					TopScore = Moments[i].Score;
					Window.Label = Moments[i].Label;
				}
			}
			if (!Best.bValid || Window.Score > Best.Score)
			{
				Best = Window;
			}
		}
		return Best;
	}
}
