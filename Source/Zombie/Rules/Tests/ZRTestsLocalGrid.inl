// Testes: campo de fluxo com custo extra (barricada cara, mas passável) e a grade fina em volta do jogador.
// Contrato: docs/regras-de-jogo.md, seções 3 e 12. Usa PlantaBase e Barricada de ZRTestsBase.inl.

#include "Rules/ZRFlowField.h"
#include "Rules/ZRLocalGrid.h"

namespace
{
	/** 7 x 5, tudo andável, com custo extra Cost na coluna 3, das linhas FirstRow a LastRow. */
	void GradeComMuro(int FirstRow, int LastRow, int Cost, std::vector<std::uint8_t>& Walkable, std::vector<int>& Extra)
	{
		Walkable.assign(35, 1);
		Extra.assign(35, 0);
		for (int Y = FirstRow; Y <= LastRow; ++Y)
		{
			Extra[static_cast<std::size_t>(Y * 7 + 3)] = Cost;
		}
	}

	/** Segue o campo de (X, Y) até o alvo; conta quantas células com custo extra pisou. Falso se não chegou. */
	bool Caminha(const ZR::FFlowField& Field, const std::vector<int>& Extra, int Width, int X, int Y, int& OutCostly)
	{
		OutCostly = 0;
		for (int Step = 0; Step < 200; ++Step)
		{
			if (X == Field.GetTargetX() && Y == Field.GetTargetY())
			{
				return true;
			}
			int NextX = 0;
			int NextY = 0;
			if (!Field.StepToward(X, Y, NextX, NextY))
			{
				return false;
			}
			X = NextX;
			Y = NextY;
			OutCostly += Extra[static_cast<std::size_t>(Y * Width + X)] > 0 ? 1 : 0;
		}
		return false;
	}
}

ZR_TEST(FluxoBarricadaDesvio, "Fluxo.BarricadaCaraFazAHordaDarAVolta")
{
	// muro nas linhas 0 a 3 da coluna 3; a linha 4 está aberta: o desvio (76) custa bem menos que atravessar (460)
	std::vector<std::uint8_t> Walkable;
	std::vector<int> Extra;
	GradeComMuro(0, 3, 400, Walkable, Extra);
	ZR::FFlowField Field;
	Field.Build(Walkable, Extra, 7, 5, 6, 2);
	ZR_EQ(Field.CostAt(0, 2), 76);
	int Costly = 0;
	ZR_EXPECT(Caminha(Field, Extra, 7, 0, 2, Costly));
	ZR_EQ(Costly, 0);
}

ZR_TEST(FluxoBarricadaSemDesvio, "Fluxo.SemDesvioAHordaVemBaterNaBarricada")
{
	// muro na coluna inteira: atravessar uma vez é o caminho (60 de andar + 400 da barricada)
	std::vector<std::uint8_t> Walkable;
	std::vector<int> Extra;
	GradeComMuro(0, 4, 400, Walkable, Extra);
	ZR::FFlowField Field;
	Field.Build(Walkable, Extra, 7, 5, 6, 2);
	ZR_EQ(Field.CostAt(0, 2), 460);
	int Costly = 0;
	ZR_EXPECT(Caminha(Field, Extra, 7, 0, 2, Costly));
	ZR_EQ(Costly, 1);
	// barricada barata: atravessar sai mais em conta que qualquer desvio
	GradeComMuro(0, 3, 5, Walkable, Extra);
	Field.Build(Walkable, Extra, 7, 5, 6, 2);
	ZR_EQ(Field.CostAt(0, 2), 65);
}

ZR_TEST(FluxoSemExtra, "Fluxo.CustoExtraZeroDaOMesmoCampo")
{
	const ZR::FCityPlan Plan = ZR::GenerateCity(ZR::FCityConfig());
	const std::vector<std::uint8_t> Mask = Plan.WalkableMask();
	int TargetX = -1;
	int TargetY = -1;
	for (int Y = 0; Y < Plan.Height && TargetX < 0; ++Y)
	{
		for (int X = 0; X < Plan.Width && TargetX < 0; ++X)
		{
			if (Plan.IsWalkable(X, Y))
			{
				TargetX = X;
				TargetY = Y;
			}
		}
	}
	ZR::FFlowField Plain;
	Plain.Build(Mask, Plan.Width, Plan.Height, TargetX, TargetY);
	ZR::FFlowField Zero;
	Zero.Build(Mask, std::vector<int>(Mask.size(), 0), Plan.Width, Plan.Height, TargetX, TargetY);
	ZR::FFlowField Empty;
	Empty.Build(Mask, std::vector<int>(), Plan.Width, Plan.Height, TargetX, TargetY);
	int Different = 0;
	for (int Y = 0; Y < Plan.Height; ++Y)
	{
		for (int X = 0; X < Plan.Width; ++X)
		{
			Different += (Plain.CostAt(X, Y) != Zero.CostAt(X, Y) || Plain.CostAt(X, Y) != Empty.CostAt(X, Y)) ? 1 : 0;
		}
	}
	ZR_EQ(Different, 0);
}

ZR_TEST(FluxoOlharAFrente, "Fluxo.OlharAlgunsPassosAFrente")
{
	std::vector<std::uint8_t> Walkable(35, 1);
	ZR::FFlowField Field;
	Field.Build(Walkable, 7, 5, 6, 2);
	int X = 0;
	int Y = 0;
	ZR_EXPECT(Field.LookAhead(0, 2, 3, X, Y));
	ZR_EQ(X, 3);
	ZR_EQ(Y, 2);
	// para no alvo, sem passar
	ZR_EXPECT(Field.LookAhead(4, 2, 5, X, Y));
	ZR_EQ(X, 6);
	ZR_EQ(Y, 2);
	// no alvo não há passo
	ZR_EXPECT(!Field.LookAhead(6, 2, 3, X, Y));
}

ZR_TEST(GradeFinaOrigem, "GradeFina.OrigemEncaixaNaResolucaoECentroDaCelula")
{
	ZR::FLocalGridConfig Config;
	Config.SizeCm = 2000.0f;
	Config.ResolutionCm = 100.0f;
	const ZR::FLocalGrid Grid = ZR::MakeLocalGrid(PlantaBase(), Celula, 523.0f, 1577.0f, Config, {});
	ZR_NEAR(Grid.OriginX, -500.0f, 1e-3);
	ZR_NEAR(Grid.OriginY, 500.0f, 1e-3);
	ZR_EQ(Grid.Width, 20);
	ZR_EQ(Grid.Height, 20);
	ZR_EQ(Grid.Walkable.size(), static_cast<std::size_t>(400));
	ZR_EQ(Grid.ExtraCost.size(), static_cast<std::size_t>(400));
	int X = 0;
	int Y = 0;
	ZR_EXPECT(Grid.ToCell(523.0f, 1577.0f, X, Y));
	ZR_EQ(X, 10);
	ZR_EQ(Y, 10);
	float CenterX = 0.0f;
	float CenterY = 0.0f;
	Grid.CellCenter(10, 10, CenterX, CenterY);
	ZR_NEAR(CenterX, 550.0f, 1e-3);
	ZR_NEAR(CenterY, 1550.0f, 1e-3);
	ZR_EXPECT(!Grid.ToCell(-501.0f, 1000.0f, X, Y));
	ZR_EXPECT(!Grid.ToCell(1500.0f, 1000.0f, X, Y));
}

ZR_TEST(GradeFinaCidade, "GradeFina.PredioMarEForaDaPlantaBloqueiam")
{
	ZR::FLocalGridConfig Config;
	Config.SizeCm = 2000.0f;
	Config.ResolutionCm = 100.0f;
	const ZR::FLocalGrid Grid = ZR::MakeLocalGrid(PlantaBase(), Celula, 500.0f, 1500.0f, Config, {});
	ZR_EQ(Grid.Walkable.size(), static_cast<std::size_t>(400));
	if (Grid.Walkable.size() != 400)
	{
		return;
	}
	auto WalkableAt = [&Grid](int X, int Y) { return Grid.Walkable[static_cast<std::size_t>(Y * Grid.Width + X)] != 0; };
	ZR_EXPECT(WalkableAt(10, 10));  // rua (0, 1)
	ZR_EXPECT(WalkableAt(10, 4));   // rua (0, 0)
	ZR_EXPECT(!WalkableAt(15, 0));  // prédio (1, 0)
	ZR_EXPECT(WalkableAt(15, 10));  // rua (1, 1)
	ZR_EXPECT(!WalkableAt(4, 10));  // x < 0: fora da planta
	ZR_EQ(Grid.CountCostly(), 0);
}

ZR_TEST(GradeFinaBarricada, "GradeFina.BarricadaCrescePeloRaioDoZumbiEFicaCara")
{
	ZR::FLocalGridConfig Config;
	Config.SizeCm = 2000.0f;
	Config.ResolutionCm = 100.0f;
	Config.InflateCm = 40.0f;
	Config.BarricadeCost = 400;
	// barricada de 400 x 40 em (500, 1500), crescida 40 cm: x de 260 a 740 e y de 1440 a 1560, ou as células 7 a 12 e 9 a 10
	const std::vector<ZR::FFootprint> Pieces = {ZR::MakeFootprint(Barricada(), 500.0f, 1500.0f, 0)};
	const ZR::FLocalGrid Grid = ZR::MakeLocalGrid(PlantaBase(), Celula, 500.0f, 1500.0f, Config, Pieces);
	ZR_EQ(Grid.ExtraCost.size(), static_cast<std::size_t>(400));
	if (Grid.ExtraCost.size() != 400 || Grid.Walkable.size() != 400)
	{
		return;
	}
	auto CostAt = [&Grid](int X, int Y) { return Grid.ExtraCost[static_cast<std::size_t>(Y * Grid.Width + X)]; };
	ZR_EQ(Grid.CountCostly(), 12);
	ZR_EQ(CostAt(7, 9), 400);
	ZR_EQ(CostAt(12, 10), 400);
	ZR_EQ(CostAt(6, 9), 0);
	ZR_EQ(CostAt(13, 9), 0);
	ZR_EQ(CostAt(7, 8), 0);
	ZR_EQ(CostAt(7, 11), 0);
	// a barricada é cara, não parede: a célula continua andável
	ZR_EXPECT(Grid.Walkable[static_cast<std::size_t>(9 * Grid.Width + 7)] != 0);
}
