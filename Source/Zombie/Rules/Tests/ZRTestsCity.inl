// Testes: sorteio determinístico, planta da cidade e campo de fluxo da horda.
// Contrato: docs/regras-de-jogo.md, seções 1 a 3.

#include <cstdlib>

#include "Rules/ZRRandom.h"
#include "Rules/ZRCityPlan.h"
#include "Rules/ZRFlowField.h"

namespace
{
	ZR::FCityConfig SmallCity(std::uint64_t Seed)
	{
		ZR::FCityConfig Config;
		Config.Width = 17;
		Config.Height = 21;
		Config.BlockCells = 3;
		Config.BeachRows = 2;
		Config.SeaRows = 3;
		Config.Seed = Seed;
		return Config;
	}
}

ZR_TEST(RandomRepeteComAMesmaSemente, "Sorteio.MesmaSementeMesmaSequencia")
{
	ZR::FRng A(42);
	ZR::FRng B(42);
	ZR::FRng C(43);
	bool bSame = true;
	bool bDiffers = false;
	for (int i = 0; i < 100; ++i)
	{
		const std::uint64_t ValueA = A.Next();
		bSame = bSame && (ValueA == B.Next());
		bDiffers = bDiffers || (ValueA != C.Next());
	}
	ZR_EXPECT(bSame);
	ZR_EXPECT(bDiffers);
}

ZR_TEST(RandomFaixasInclusivas, "Sorteio.FaixaInteiraInclusivaECobreTudo")
{
	ZR::FRng Rng(7);
	bool Seen[5] = {false, false, false, false, false};
	bool bInside = true;
	for (int i = 0; i < 2000; ++i)
	{
		const int Value = Rng.RangeInt(3, 7);
		bInside = bInside && Value >= 3 && Value <= 7;
		if (Value >= 3 && Value <= 7)
		{
			Seen[Value - 3] = true;
		}
		const double Unit = Rng.Unit();
		bInside = bInside && Unit >= 0.0 && Unit < 1.0;
	}
	ZR_EXPECT(bInside);
	ZR_EXPECT(Seen[0] && Seen[1] && Seen[2] && Seen[3] && Seen[4]);
	ZR_EQ(Rng.RangeInt(5, 5), 5);
}

ZR_TEST(CidadeBordaEhRua, "Cidade.BordaDaTerraEhRua")
{
	const ZR::FCityPlan Plan = ZR::GenerateCity(SmallCity(1));
	const int LandHeight = Plan.LandHeight;
	bool bBorder = true;
	for (int X = 0; X < Plan.Width; ++X)
	{
		bBorder = bBorder && Plan.At(X, 0) == ZR::ECell::Road && Plan.At(X, LandHeight - 1) == ZR::ECell::Road;
	}
	for (int Y = 0; Y < LandHeight; ++Y)
	{
		bBorder = bBorder && Plan.At(0, Y) == ZR::ECell::Road && Plan.At(Plan.Width - 1, Y) == ZR::ECell::Road;
	}
	ZR_EXPECT(bBorder);
	ZR_EQ(LandHeight, 21 - 2 - 3);
}

ZR_TEST(CidadeRuasNoPeriodoDoQuarteirao, "Cidade.RuaACadaQuarteirao")
{
	const ZR::FCityPlan Plan = ZR::GenerateCity(SmallCity(2));
	bool bGrid = true;
	for (int Y = 0; Y < Plan.LandHeight; ++Y)
	{
		for (int X = 0; X < Plan.Width; ++X)
		{
			if (X % 4 == 0 || Y % 4 == 0)
			{
				bGrid = bGrid && Plan.At(X, Y) == ZR::ECell::Road;
			}
		}
	}
	ZR_EXPECT(bGrid);
	ZR_EXPECT(Plan.CountOf(ZR::ECell::Building) > 0);
}

ZR_TEST(CidadePraiaEMar, "Cidade.PraiaEMarNoSul")
{
	const ZR::FCityPlan Plan = ZR::GenerateCity(SmallCity(3));
	bool bOk = true;
	for (int X = 0; X < Plan.Width; ++X)
	{
		bOk = bOk && Plan.At(X, Plan.LandHeight) == ZR::ECell::Beach && Plan.At(X, Plan.LandHeight + 1) == ZR::ECell::Beach;
		bOk = bOk && Plan.At(X, Plan.Height - 1) == ZR::ECell::Sea && Plan.At(X, Plan.Height - 3) == ZR::ECell::Sea;
	}
	ZR_EXPECT(bOk);
	ZR_EXPECT(Plan.IsWalkable(0, Plan.LandHeight));
	ZR_EXPECT(!Plan.IsWalkable(0, Plan.Height - 1));
	ZR_EXPECT(!Plan.InBounds(-1, 0));
	ZR_EQ(Plan.At(-1, 0), ZR::ECell::Sea);
}

ZR_TEST(CidadeAndaresNaFaixaEDeterministica, "Cidade.AndaresNaFaixaEMesmaSemente")
{
	const ZR::FCityConfig Config = SmallCity(4);
	const ZR::FCityPlan A = ZR::GenerateCity(Config);
	const ZR::FCityPlan B = ZR::GenerateCity(Config);
	bool bRange = true;
	for (int Y = 0; Y < A.Height; ++Y)
	{
		for (int X = 0; X < A.Width; ++X)
		{
			const int Floors = A.FloorsAt(X, Y);
			if (A.At(X, Y) == ZR::ECell::Building)
			{
				bRange = bRange && Floors >= Config.MinFloors && Floors <= Config.MaxFloors;
			}
			else
			{
				bRange = bRange && Floors == 0;
			}
		}
	}
	ZR_EXPECT(bRange);
	ZR_EXPECT(A.Cells == B.Cells);
	ZR_EXPECT(A.Floors == B.Floors);
	const ZR::FCityPlan Other = ZR::GenerateCity(SmallCity(5));
	ZR_EXPECT(Other.Floors != A.Floors);
}

ZR_TEST(CidadeCentroMaisAlto, "Cidade.CentroMaisAltoQueABorda")
{
	ZR::FCityConfig Config;
	Config.Width = 33;
	Config.Height = 38;
	Config.PlazaChance = 0.0;
	Config.Seed = 11;
	const ZR::FCityPlan Plan = ZR::GenerateCity(Config);
	double CenterSum = 0.0;
	int CenterCount = 0;
	double EdgeSum = 0.0;
	int EdgeCount = 0;
	const double CenterX = (Plan.Width - 1) * 0.5;
	const double CenterY = (Plan.LandHeight - 1) * 0.5;
	for (int Y = 0; Y < Plan.LandHeight; ++Y)
	{
		for (int X = 0; X < Plan.Width; ++X)
		{
			if (Plan.At(X, Y) != ZR::ECell::Building)
			{
				continue;
			}
			const double Dx = (X - CenterX) / CenterX;
			const double Dy = (Y - CenterY) / CenterY;
			const double Distance = std::sqrt(Dx * Dx + Dy * Dy);
			if (Distance < 0.35)
			{
				CenterSum += Plan.FloorsAt(X, Y);
				++CenterCount;
			}
			else if (Distance > 0.8)
			{
				EdgeSum += Plan.FloorsAt(X, Y);
				++EdgeCount;
			}
		}
	}
	ZR_EXPECT(CenterCount > 0 && EdgeCount > 0);
	if (CenterCount > 0 && EdgeCount > 0)
	{
		ZR_EXPECT(CenterSum / CenterCount > EdgeSum / EdgeCount + 2.0);
	}
}

ZR_TEST(CidadeTudoLigado, "Cidade.TodoChaoAndavelEhAlcancavel")
{
	for (std::uint64_t Seed = 1; Seed <= 5; ++Seed)
	{
		ZR::FCityConfig Config = SmallCity(Seed);
		Config.PlazaChance = 0.3;
		const ZR::FCityPlan Plan = ZR::GenerateCity(Config);
		ZR::FFlowField Flow;
		Flow.Build(Plan.WalkableMask(), Plan.Width, Plan.Height, 0, 0);
		bool bAllReached = true;
		for (int Y = 0; Y < Plan.Height; ++Y)
		{
			for (int X = 0; X < Plan.Width; ++X)
			{
				if (Plan.IsWalkable(X, Y))
				{
					bAllReached = bAllReached && Flow.CostAt(X, Y) >= 0;
				}
			}
		}
		ZR_EXPECT(bAllReached);
	}
}

ZR_TEST(CidadePontosDeObjetoNaRua, "Cidade.PontosDeObjetoSoNaRuaESemRepetir")
{
	const ZR::FCityPlan Plan = ZR::GenerateCity(SmallCity(9));
	const std::vector<ZR::FPropSpot> Spots = ZR::PickRoadSpots(Plan, 30, 99, 0, 0, 2);
	ZR_EQ(static_cast<int>(Spots.size()), 30);
	bool bOnRoad = true;
	bool bDistinct = true;
	bool bOffsets = true;
	bool bAwayFromStart = true;
	for (std::size_t i = 0; i < Spots.size(); ++i)
	{
		bOnRoad = bOnRoad && Plan.At(Spots[i].X, Spots[i].Y) == ZR::ECell::Road;
		bOffsets = bOffsets && std::fabs(Spots[i].OffsetX) <= 0.5f && std::fabs(Spots[i].OffsetY) <= 0.5f;
		bAwayFromStart = bAwayFromStart && (std::abs(Spots[i].X) > 2 || std::abs(Spots[i].Y) > 2);
		for (std::size_t j = i + 1; j < Spots.size(); ++j)
		{
			bDistinct = bDistinct && !(Spots[i].X == Spots[j].X && Spots[i].Y == Spots[j].Y);
		}
	}
	ZR_EXPECT(bOnRoad);
	ZR_EXPECT(bDistinct);
	ZR_EXPECT(bOffsets);
	ZR_EXPECT(bAwayFromStart);
	const std::vector<ZR::FPropSpot> Again = ZR::PickRoadSpots(Plan, 30, 99, 0, 0, 2);
	ZR_EXPECT(Again.size() == Spots.size() && Again[5].X == Spots[5].X && Again[5].Y == Spots[5].Y);
}

namespace
{
	// 5 x 5, parede vertical em X = 2 de Y = 0 a Y = 3 (passagem só em Y = 4)
	std::vector<std::uint8_t> WallGrid()
	{
		std::vector<std::uint8_t> Grid(25, 1);
		for (int Y = 0; Y <= 3; ++Y)
		{
			Grid[static_cast<std::size_t>(Y * 5 + 2)] = 0;
		}
		return Grid;
	}
}

ZR_TEST(FluxoCustoEmLinhaReta, "Fluxo.CustoOrtogonal10Diagonal14")
{
	std::vector<std::uint8_t> Open(25, 1);
	ZR::FFlowField Flow;
	Flow.Build(Open, 5, 5, 0, 0);
	ZR_EQ(Flow.CostAt(0, 0), 0);
	ZR_EQ(Flow.CostAt(4, 0), 40);
	ZR_EQ(Flow.CostAt(2, 2), 28);
	ZR_EQ(Flow.CostAt(4, 2), 48);
	ZR_EQ(Flow.CostAt(9, 9), -1);
}

ZR_TEST(FluxoContornaParede, "Fluxo.ContornaAParede")
{
	ZR::FFlowField Flow;
	Flow.Build(WallGrid(), 5, 5, 0, 0);
	ZR_EQ(Flow.CostAt(2, 0), -1);
	// de (4,0) o caminho desce até Y = 4 para passar a parede
	int X = 4;
	int Y = 0;
	int Steps = 0;
	bool bPassedBottom = false;
	while (Steps < 30)
	{
		int NextX = 0;
		int NextY = 0;
		if (!Flow.StepToward(X, Y, NextX, NextY))
		{
			break;
		}
		X = NextX;
		Y = NextY;
		bPassedBottom = bPassedBottom || (X == 2 && Y == 4);
		++Steps;
	}
	ZR_EXPECT(X == 0 && Y == 0);
	ZR_EXPECT(bPassedBottom);
}

ZR_TEST(FluxoNaoCortaQuina, "Fluxo.DiagonalNaoCortaQuina")
{
	// 3 x 3 com bloqueio em (1,0): de (2,1) para (0,0) a diagonal (1,0)->... não existe; e de (1,1) a diagonal até (0,0) passa
	// raspando a quina (1,0) bloqueada, então é proibida
	std::vector<std::uint8_t> Grid(9, 1);
	Grid[1] = 0;
	ZR::FFlowField Flow;
	Flow.Build(Grid, 3, 3, 0, 0);
	ZR_EQ(Flow.CostAt(1, 1), 20);
	int NextX = 0;
	int NextY = 0;
	ZR_EXPECT(Flow.StepToward(1, 1, NextX, NextY));
	ZR_EXPECT(NextX == 0 && NextY == 1);
}

ZR_TEST(FluxoDirecaoUnitaria, "Fluxo.DirecaoUnitariaEAlvoSemPasso")
{
	std::vector<std::uint8_t> Open(25, 1);
	ZR::FFlowField Flow;
	Flow.Build(Open, 5, 5, 2, 2);
	double Dx = 0.0;
	double Dy = 0.0;
	ZR_EXPECT(Flow.Direction(4, 4, Dx, Dy));
	ZR_NEAR(Dx * Dx + Dy * Dy, 1.0, 1e-9);
	ZR_EXPECT(Dx < 0.0 && Dy < 0.0);
	ZR_EXPECT(!Flow.Direction(2, 2, Dx, Dy));
	int NextX = 0;
	int NextY = 0;
	ZR_EXPECT(!Flow.StepToward(2, 2, NextX, NextY));
	ZR_EXPECT(!Flow.StepToward(-1, 0, NextX, NextY));
}

ZR_TEST(FluxoAlvoBloqueado, "Fluxo.AlvoBloqueadoDeixaTudoInalcancavel")
{
	ZR::FFlowField Flow;
	Flow.Build(WallGrid(), 5, 5, 2, 0);
	ZR_EQ(Flow.CostAt(0, 0), -1);
	ZR_EXPECT(!Flow.IsValid());
}
