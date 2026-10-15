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

	namespace
	{
		/** O prédio da célula (X, Y): o quadrado da célula encolhido pelo recuo. */
		void BuildingSquare(const FCityPlan& Plan, int X, int Y, float CellSizeCm, float& OutLeft, float& OutTop, float& OutRight,
			float& OutBottom)
		{
			const float Inset = std::clamp(Plan.InsetAt(X, Y), 0.0f, CellSizeCm * 0.5f);
			OutLeft = static_cast<float>(X) * CellSizeCm + Inset;
			OutTop = static_cast<float>(Y) * CellSizeCm + Inset;
			OutRight = static_cast<float>(X + 1) * CellSizeCm - Inset;
			OutBottom = static_cast<float>(Y + 1) * CellSizeCm - Inset;
		}
	}

	bool FCityPlan::IsWalkablePoint(float X, float Y, float CellSizeCm) const
	{
		if (X < 0.0f || Y < 0.0f || CellSizeCm <= 0.0f)
		{
			return false;
		}
		const int CellX = static_cast<int>(std::floor(X / CellSizeCm));
		const int CellY = static_cast<int>(std::floor(Y / CellSizeCm));
		if (At(CellX, CellY) != ECell::Building)
		{
			return IsWalkable(CellX, CellY);
		}
		// no prédio, só a faixa do recuo; a borda da fachada ainda é chão
		float Left = 0.0f;
		float Top = 0.0f;
		float Right = 0.0f;
		float Bottom = 0.0f;
		BuildingSquare(*this, CellX, CellY, CellSizeCm, Left, Top, Right, Bottom);
		return !(X > Left && X < Right && Y > Top && Y < Bottom);
	}

	bool FCityPlan::IsWalkableRect(float Left, float Top, float Right, float Bottom, float CellSizeCm) const
	{
		if (Left < 0.0f || Top < 0.0f || CellSizeCm <= 0.0f || Right < Left || Bottom < Top)
		{
			return false;
		}
		// toda célula que o retângulo cobre de verdade (encostar na borda da célula não conta)
		constexpr float Edge = 0.01f;
		const int FirstX = static_cast<int>(std::floor(Left / CellSizeCm));
		const int LastX = static_cast<int>(std::floor(std::max(Left, Right - Edge) / CellSizeCm));
		const int FirstY = static_cast<int>(std::floor(Top / CellSizeCm));
		const int LastY = static_cast<int>(std::floor(std::max(Top, Bottom - Edge) / CellSizeCm));
		for (int Y = FirstY; Y <= LastY; ++Y)
		{
			for (int X = FirstX; X <= LastX; ++X)
			{
				const ECell Cell = At(X, Y);
				if (Cell == ECell::Sea || !InBounds(X, Y))
				{
					return false;
				}
				if (Cell != ECell::Building)
				{
					continue;
				}
				float BuildingLeft = 0.0f;
				float BuildingTop = 0.0f;
				float BuildingRight = 0.0f;
				float BuildingBottom = 0.0f;
				BuildingSquare(*this, X, Y, CellSizeCm, BuildingLeft, BuildingTop, BuildingRight, BuildingBottom);
				if (Left < BuildingRight && Right > BuildingLeft && Top < BuildingBottom && Bottom > BuildingTop)
				{
					return false;
				}
			}
		}
		return true;
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

		// recuo de cada prédio num sorteio à parte: a mesma semente continua dando as mesmas ruas, praças e andares
		Plan.InsetCm.assign(Total, 0.0f);
		FRng InsetRng(Config.Seed ^ 0xA5A5F00DCAFEBEEFull);
		const double MaxInset = std::clamp(static_cast<double>(Config.MaxInsetCm), 0.0, static_cast<double>(Config.CellSizeCm) * 0.45);
		const double MinInset = std::clamp(static_cast<double>(Config.MinInsetCm), 0.0, MaxInset);
		for (std::size_t Cell = 0; Cell < Total; ++Cell)
		{
			if (Plan.Cells[Cell] == ECell::Building)
			{
				Plan.InsetCm[Cell] = static_cast<float>(InsetRng.RangeDouble(MinInset, MaxInset));
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
