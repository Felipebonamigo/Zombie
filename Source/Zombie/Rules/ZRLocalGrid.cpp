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

	bool FLocalGrid::ClearLine(float FromX, float FromY, float ToX, float ToY) const
	{
		if (ResolutionCm <= 0.0f)
		{
			return false;
		}
		// amostras a cada quarto de célula: pega toda célula que a reta atravessa (só pode escapar uma quina raspada)
		const float Dx = ToX - FromX;
		const float Dy = ToY - FromY;
		const float Length = std::sqrt(Dx * Dx + Dy * Dy);
		const int Samples = static_cast<int>(std::ceil(Length / (ResolutionCm * 0.25f))) + 1;
		for (int i = 0; i <= Samples; ++i)
		{
			const float T = static_cast<float>(i) / static_cast<float>(Samples);
			int X = 0;
			int Y = 0;
			if (!ToCell(FromX + Dx * T, FromY + Dy * T, X, Y))
			{
				return false;
			}
			const std::size_t Index = static_cast<std::size_t>(Y * Width + X);
			if (Walkable[Index] == 0 || ExtraCost[Index] > 0)
			{
				return false;
			}
		}
		return true;
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
				// pelo ponto: a calçada e o beco dentro da célula do prédio também são chão
				Grid.Walkable[static_cast<std::size_t>(Y * Grid.Width + X)] = Plan.IsWalkablePoint(PlanX, PlanY, CityCellCm) ? 1 : 0;
			}
		}

		// cada barricada, crescida pelo raio do zumbi, marca as células com o centro dentro da faixa (assim um vão de 1,8 m
		// sempre deixa uma célula livre). O preço se divide pela espessura em células: atravessar custa BarricadeCost, com a
		// peça encaixada em qualquer ponto da grade de 50 cm
		const float Inflate = std::max(Config.InflateCm, 0.0f);
		for (const FFootprint& Piece : Barricades)
		{
			const float Left = Piece.X - Piece.HalfX - Inflate;
			const float Right = Piece.X + Piece.HalfX + Inflate;
			const float Top = Piece.Y - Piece.HalfY - Inflate;
			const float Bottom = Piece.Y + Piece.HalfY + Inflate;
			// as células com o centro estritamente dentro de (Lo, Hi): a primeira e a última
			const auto Span = [Resolution](float Lo, float Hi, float Origin, int Count, int& OutFirst, int& OutLast)
			{
				OutFirst = std::max(0, static_cast<int>(std::floor((Lo - Origin) / Resolution - 0.5f)) + 1);
				OutLast = std::min(Count - 1, static_cast<int>(std::ceil((Hi - Origin) / Resolution - 0.5f)) - 1);
			};
			int FirstX = 0;
			int LastX = -1;
			int FirstY = 0;
			int LastY = -1;
			Span(Left, Right, Grid.OriginX, Grid.Width, FirstX, LastX);
			Span(Top, Bottom, Grid.OriginY, Grid.Height, FirstY, LastY);
			// a espessura é contada no eixo fino da peça, sem cortar na borda da grade (a peça na borda não fica mais cara)
			const bool bThinX = Piece.HalfX <= Piece.HalfY;
			const float ThinLo = (bThinX ? Left - Grid.OriginX : Top - Grid.OriginY) / Resolution;
			const float ThinHi = (bThinX ? Right - Grid.OriginX : Bottom - Grid.OriginY) / Resolution;
			const int ThinFirst = static_cast<int>(std::floor(ThinLo - 0.5f)) + 1;
			const int ThinLast = static_cast<int>(std::ceil(ThinHi - 0.5f)) - 1;
			const int Thickness = std::max(1, ThinLast - ThinFirst + 1);
			const int CellCost = (std::max(Config.BarricadeCost, 0) + Thickness - 1) / Thickness;
			for (int Y = FirstY; Y <= LastY; ++Y)
			{
				for (int X = FirstX; X <= LastX; ++X)
				{
					const std::size_t Index = static_cast<std::size_t>(Y * Grid.Width + X);
					if (Grid.Walkable[Index] != 0)
					{
						Grid.ExtraCost[Index] = std::max(Grid.ExtraCost[Index], CellCost);
					}
				}
			}
		}
		return Grid;
	}
}
