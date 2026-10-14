#include "Rules/ZRDayNight.h"

#include <cmath>

namespace ZR
{
	namespace
	{
		double TotalHours(const FDayConfig& Config, double Seconds)
		{
			const double Length = Config.DayLengthSeconds > 0.0f ? static_cast<double>(Config.DayLengthSeconds) : 1.0;
			return static_cast<double>(Config.StartHour) + 24.0 * Seconds / Length;
		}
	}

	double HourAt(const FDayConfig& Config, double Seconds)
	{
		const double Hour = std::fmod(TotalHours(Config, Seconds), 24.0);
		return Hour < 0.0 ? Hour + 24.0 : Hour;
	}

	int DayIndexAt(const FDayConfig& Config, double Seconds)
	{
		return static_cast<int>(std::floor(TotalHours(Config, Seconds) / 24.0 + 1e-9));
	}

	bool IsNight(const FDayConfig& Config, double Hour)
	{
		const double Start = Config.NightStartHour;
		const double End = Config.NightEndHour;
		if (Start > End)
		{
			return Hour >= Start || Hour < End;
		}
		return Hour >= Start && Hour < End;
	}

	double SunElevationDegrees(double Hour, float MaxElevationDeg)
	{
		const double Pi = 3.14159265358979323846;
		return static_cast<double>(MaxElevationDeg) * std::sin(Pi * (Hour - 6.0) / 12.0);
	}

	bool IsHordeNight(const FDayConfig& Config, int NightIndex)
	{
		return Config.HordeEveryNights > 0 && NightIndex >= 0 && (NightIndex + 1) % Config.HordeEveryNights == 0;
	}
}
