#include "Rules/ZRHype.h"

#include <algorithm>
#include <cmath>

namespace ZR
{
	const char* HypeEventId(EHypeEvent Event)
	{
		switch (Event)
		{
		case EHypeEvent::Kill: return "abate";
		case EHypeEvent::Headshot: return "tiro_na_cabeca";
		case EHypeEvent::Dismember: return "desmembrou";
		case EHypeEvent::Gib: return "despedacou";
		case EHypeEvent::MultiKill: return "abate_multiplo";
		case EHypeEvent::ChainExplosion: return "explosao_em_cadeia";
		case EHypeEvent::VehicleKill: return "atropelou";
		case EHypeEvent::SlowMoKill: return "abate_mira_lenta";
		case EHypeEvent::NamedZombieKill: return "zumbi_do_chat";
		case EHypeEvent::CloseCall: return "escapou_por_pouco";
		case EHypeEvent::LongShot: return "tiro_de_longe";
		case EHypeEvent::TookHit: return "apanhou";
		case EHypeEvent::Silenced: return "calou_gritador";
		case EHypeEvent::TrapKill: return "abate_armadilha";
		case EHypeEvent::Count: break;
		}
		return "";
	}

	void FHypeState::Reset(const FHypeConfig& Config)
	{
		Hype = 0.0f;
		Viewers = 0.0f;
		Followers = Config.StartFollowers;
		Combo = 0;
		ComboTimer = 0.0f;
		SinceLastEvent = 1.0e9f;
		LastEvent = -1;
		RepeatCount = 0;
		ThresholdArmed.assign(Config.DonationThresholds.size(), true);
		TotalDonations = 0.0;
	}

	FHypeGain AddHypeEvent(const FHypeConfig& Config, FHypeState& State, int EventIndex, float BasePoints, int Count)
	{
		FHypeGain Gain;
		State.Combo = State.ComboTimer > 0.0f ? State.Combo + 1 : 1;
		State.ComboTimer = Config.ComboWindowSeconds;
		State.RepeatCount = (EventIndex == State.LastEvent) ? State.RepeatCount + 1 : 0;
		State.LastEvent = EventIndex;
		State.SinceLastEvent = 0.0f;

		Gain.Combo = State.Combo;
		Gain.Multiplier = std::min(1.0f + Config.ComboStep * static_cast<float>(State.Combo - 1), std::max(1.0f, Config.ComboMaxMultiplier));
		Gain.Variety = std::max(std::pow(Config.VarietyFalloff, static_cast<float>(State.RepeatCount)), Config.VarietyMin);
		Gain.Points = BasePoints * static_cast<float>(std::max(1, Count)) * Gain.Multiplier * Gain.Variety;
		State.Hype = std::clamp(State.Hype + Gain.Points, 0.0f, 100.0f);
		return Gain;
	}

	float TargetViewers(const FHypeConfig& Config, float Hype, float Followers)
	{
		const float Level = std::clamp(Hype / 100.0f, 0.0f, 1.0f);
		const float Watching = Config.ViewerFloor + (1.0f - Config.ViewerFloor) * std::pow(Level, Config.ViewerExponent);
		const float ViralStart = std::clamp(Config.ViralStartHype / 100.0f, 0.0f, 0.999f);
		const float Viral = std::max(0.0f, (Level - ViralStart) / (1.0f - ViralStart));
		return std::max(0.0f, Followers) * Watching * (1.0f + Config.ViralBoost * Viral);
	}

	void TickHype(const FHypeConfig& Config, FHypeState& State, float DeltaSeconds, std::vector<FDonation>& OutDonations)
	{
		if (DeltaSeconds <= 0.0f)
		{
			return;
		}
		State.ComboTimer -= DeltaSeconds;
		if (State.ComboTimer <= 0.0f)
		{
			State.ComboTimer = 0.0f;
			State.Combo = 0;
		}

		// queda: só a parte deste passo que passou da carência
		const float Before = std::max(0.0f, State.SinceLastEvent - Config.DecayGraceSeconds);
		State.SinceLastEvent += DeltaSeconds;
		const float After = std::max(0.0f, State.SinceLastEvent - Config.DecayGraceSeconds);
		State.Hype = std::max(0.0f, State.Hype - Config.DecayPerSecond * (After - Before));

		const float Target = TargetViewers(Config, State.Hype, State.Followers);
		const float Seconds = Target > State.Viewers ? Config.ViewersRiseSeconds : Config.ViewersFallSeconds;
		const float Blend = Seconds > 0.0f ? 1.0f - std::exp(-DeltaSeconds / Seconds) : 1.0f;
		State.Viewers += (Target - State.Viewers) * Blend;

		const float Room = Config.FollowerCap > 0.0f ? std::max(0.0f, 1.0f - State.Followers / Config.FollowerCap) : 1.0f;
		State.Followers += State.Viewers * Config.FollowRatePerSecond * (State.Hype / 100.0f) * Room * DeltaSeconds;
		if (Config.FollowerCap > 0.0f)
		{
			State.Followers = std::min(State.Followers, Config.FollowerCap);
		}

		if (State.ThresholdArmed.size() != Config.DonationThresholds.size())
		{
			State.ThresholdArmed.assign(Config.DonationThresholds.size(), true);
		}
		for (std::size_t i = 0; i < Config.DonationThresholds.size(); ++i)
		{
			const float Threshold = Config.DonationThresholds[i];
			if (State.ThresholdArmed[i] && State.Hype >= Threshold)
			{
				FDonation Donation;
				Donation.ThresholdIndex = static_cast<int>(i);
				Donation.Amount = std::max(1.0f, std::round(State.Viewers * Config.DonationPerViewer * static_cast<float>(i + 1)));
				State.TotalDonations += Donation.Amount;
				OutDonations.push_back(Donation);
				State.ThresholdArmed[i] = false;
			}
			else if (!State.ThresholdArmed[i] && State.Hype < Threshold - Config.DonationRearm)
			{
				State.ThresholdArmed[i] = true;
			}
		}
	}
}
