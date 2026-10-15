#include "Rules/ZRLocalGrid.h"

#include <algorithm>
#include <cmath>

namespace ZR
{
	bool FLocalGrid::ToCell(float X, float Y, int& OutX, int& OutY) const
	{
		if (ResolutionCm <= 0.0f)
		{
			return false;
		}
		OutX = static_cast<int>(std::floor((X - OriginX) / ResolutionCm));
		OutY = static_cast<int>(std::floor((Y - OriginY) / ResolutionCm));
		return OutX >= 0 && OutY >= 0 && OutX < Width && OutY < Height;
	}

	void FLocalGrid::CellCenter(int X, int Y, float& OutX, float& OutY) const
	{
		OutX = OriginX + (static_cast<float>(X) + 0.5f) * ResolutionCm;
		OutY = OriginY + (static_cast<float>(Y) + 0.5f) * ResolutionCm;
	}

	int FLocalGrid::CountCostly() const
	{
		return static_cast<int>(std::count_if(ExtraCost.begin(), ExtraCost.end(), [](int Value) { return Value > 0; }));
	}

	FLocalGrid MakeLocalGrid(const FCityPlan& Plan, float CityCellCm, float CenterX, float CenterY, const FLocalGridConfig& Config,
		const std::vector<FFootprint>& Barricades)
	{
		FLocalGrid Grid;
		if (Config.ResolutionCm <= 0.0f || Config.SizeCm <= 0.0f || CityCellCm <= 0.0f)
		{
			return Grid;
		}
		const float Resolution = Config.ResolutionCm;
		Grid.ResolutionCm = Resolution;
		Grid.Width = std::max(1, static_cast<int>(std::lround(Config.SizeCm / Resolution)));
		Grid.Height = Grid.Width;
		// a origem encaixa na resolução: o jogador anda e as células ficam no lugar
		Grid.OriginX = std::floor((CenterX - Config.SizeCm * 0.5f) / Resolution) * Resolution;
		Grid.OriginY = std::floor((CenterY - Config.SizeCm * 0.5f) / Resolution) * Resolution;
		const std::size_t Total = static_cast<std::size_t>(Grid.Width) * static_cast<std::size_t>(Grid.Height);
		Grid.Walkable.assign(Total, 0);
		Grid.ExtraCost.assign(Total, 0);

		for (int Y = 0; Y < Grid.Height; ++Y)
		{
			for (int X = 0; X < Grid.Width; ++X)
			{
				float PlanX = 0.0f;
				float PlanY = 0.0f;
				Grid.CellCenter(X, Y, PlanX, PlanY);
				if (PlanX < 0.0f || PlanY < 0.0f)
				{
					continue;
				}
				const int CityX = static_cast<int>(std::floor(PlanX / CityCellCm));
				const int CityY = static_cast<int>(std::floor(PlanY / CityCellCm));
				Grid.Walkable[static_cast<std::size_t>(Y * Grid.Width + X)] = Plan.IsWalkable(CityX, CityY) ? 1 : 0;
			}
		}

		// cada barricada, crescida pelo raio do zumbi, marca as células que ela cobre (sobreposição de verdade, não encostar)
		const float Inflate = std::max(Config.InflateCm, 0.0f);
		for (const FFootprint& Piece : Barricades)
		{
			const float Left = Piece.X - Piece.HalfX - Inflate;
			const float Right = Piece.X + Piece.HalfX + Inflate;
			const float Top = Piece.Y - Piece.HalfY - Inflate;
			const float Bottom = Piece.Y + Piece.HalfY + Inflate;
			const int FirstX = std::max(0, static_cast<int>(std::floor((Left - Grid.OriginX) / Resolution)));
			const int LastX = std::min(Grid.Width - 1, static_cast<int>(std::ceil((Right - Grid.OriginX) / Resolution)) - 1);
			const int FirstY = std::max(0, static_cast<int>(std::floor((Top - Grid.OriginY) / Resolution)));
			const int LastY = std::min(Grid.Height - 1, static_cast<int>(std::ceil((Bottom - Grid.OriginY) / Resolution)) - 1);
			for (int Y = FirstY; Y <= LastY; ++Y)
			{
				for (int X = FirstX; X <= LastX; ++X)
				{
					const std::size_t Index = static_cast<std::size_t>(Y * Grid.Width + X);
					if (Grid.Walkable[Index] != 0)
					{
						Grid.ExtraCost[Index] = std::max(Grid.ExtraCost[Index], Config.BarricadeCost);
					}
				}
			}
		}
		return Grid;
	}
}
