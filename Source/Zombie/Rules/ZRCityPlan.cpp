#include "Rules/ZRCityPlan.h"

#include <algorithm>
#include <cmath>
#include <cstdlib>

#include "Rules/ZRRandom.h"

namespace ZR
{
	std::vector<std::uint8_t> FCityPlan::WalkableMask() const
	{
		std::vector<std::uint8_t> Mask(Cells.size(), 0);
		for (int Y = 0; Y < Height; ++Y)
		{
			for (int X = 0; X < Width; ++X)
			{
				Mask[Index(X, Y)] = IsWalkable(X, Y) ? 1 : 0;
			}
		}
		return Mask;
	}

	int FCityPlan::CountOf(ECell Kind) const
	{
		return static_cast<int>(std::count(Cells.begin(), Cells.end(), Kind));
	}

	FCityPlan GenerateCity(const FCityConfig& Config)
	{
		FCityPlan Plan;
		Plan.Width = std::max(3, Config.Width);
		Plan.Height = std::max(3, Config.Height);
		const int Coast = std::max(0, Config.BeachRows) + std::max(0, Config.SeaRows);
		Plan.LandHeight = std::max(3, Plan.Height - Coast);
		Plan.Height = Plan.LandHeight + Coast;
		const std::size_t Total = static_cast<std::size_t>(Plan.Width) * static_cast<std::size_t>(Plan.Height);
		Plan.Cells.assign(Total, ECell::Road);
		Plan.Floors.assign(Total, 0);

		FRng Rng(Config.Seed);
		const int Period = std::max(1, Config.BlockCells) + 1;
		// praça é o quarteirão inteiro: uma célula de praça cercada de prédios ficaria inalcançável
		const int BlocksX = Plan.Width / Period + 1;
		const int BlocksY = Plan.LandHeight / Period + 1;
		std::vector<bool> PlazaBlock(static_cast<std::size_t>(BlocksX * BlocksY), false);
		for (std::size_t Block = 0; Block < PlazaBlock.size(); ++Block)
		{
			PlazaBlock[Block] = Rng.Chance(Config.PlazaChance);
		}
		const double CenterX = (Plan.Width - 1) * 0.5;
		const double CenterY = (Plan.LandHeight - 1) * 0.5;
		const int MinFloors = std::max(1, Config.MinFloors);
		const int MaxFloors = std::max(MinFloors, Config.MaxFloors);

		for (int Y = 0; Y < Plan.Height; ++Y)
		{
			for (int X = 0; X < Plan.Width; ++X)
			{
				const std::size_t Cell = Plan.Index(X, Y);
				if (Y >= Plan.LandHeight)
				{
					Plan.Cells[Cell] = (Y < Plan.LandHeight + std::max(0, Config.BeachRows)) ? ECell::Beach : ECell::Sea;
					continue;
				}
				const bool bRoad = X % Period == 0 || Y % Period == 0 || X == Plan.Width - 1 || Y == Plan.LandHeight - 1;
				if (bRoad)
				{
					continue;
				}
				// um sorteio por célula, mesmo na praça: a altura dos outros prédios não muda com a praça
				const double Jitter = Rng.RangeDouble(0.6, 1.0);
				if (PlazaBlock[static_cast<std::size_t>((Y / Period) * BlocksX + X / Period)])
				{
					Plan.Cells[Cell] = ECell::Plaza;
					continue;
				}
				// mais alto no centro: t = 1 no centro, 0 na borda (e além dela, nos cantos)
				const double Dx = CenterX > 0.0 ? (X - CenterX) / CenterX : 0.0;
				const double Dy = CenterY > 0.0 ? (Y - CenterY) / CenterY : 0.0;
				const double Closeness = std::max(0.0, 1.0 - std::sqrt(Dx * Dx + Dy * Dy));
				const double Extra = (MaxFloors - MinFloors) * std::pow(Closeness, 1.5) * Jitter;
				Plan.Cells[Cell] = ECell::Building;
				Plan.Floors[Cell] = std::clamp(MinFloors + static_cast<int>(std::lround(Extra)), MinFloors, MaxFloors);
			}
		}
		return Plan;
	}

	std::vector<FPropSpot> PickRoadSpots(const FCityPlan& Plan, int Count, std::uint64_t Seed, int AvoidX, int AvoidY, int AvoidRadius)
	{
		std::vector<FPropSpot> Candidates;
		for (int Y = 0; Y < Plan.Height; ++Y)
		{
			for (int X = 0; X < Plan.Width; ++X)
			{
				const bool bNearStart = std::abs(X - AvoidX) <= AvoidRadius && std::abs(Y - AvoidY) <= AvoidRadius;
				if (Plan.At(X, Y) == ECell::Road && !bNearStart)
				{
					FPropSpot Spot;
					Spot.X = X;
					Spot.Y = Y;
					Candidates.push_back(Spot);
				}
			}
		}
		// Fisher-Yates parcial: as primeiras Count posições viram a escolha
		FRng Rng(Seed);
		const int Wanted = std::min(std::max(0, Count), static_cast<int>(Candidates.size()));
		for (int i = 0; i < Wanted; ++i)
		{
			const int Pick = Rng.RangeInt(i, static_cast<int>(Candidates.size()) - 1);
			std::swap(Candidates[static_cast<std::size_t>(i)], Candidates[static_cast<std::size_t>(Pick)]);
			Candidates[static_cast<std::size_t>(i)].OffsetX = static_cast<float>(Rng.RangeDouble(-0.4, 0.4));
			Candidates[static_cast<std::size_t>(i)].OffsetY = static_cast<float>(Rng.RangeDouble(-0.4, 0.4));
		}
		Candidates.resize(static_cast<std::size_t>(Wanted));
		return Candidates;
	}
}
