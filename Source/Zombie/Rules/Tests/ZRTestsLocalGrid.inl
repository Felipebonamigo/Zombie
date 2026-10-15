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
	// barricada de 400 x 40 em (500, 1500), crescida 40 cm: x de 260 a 740 e y de 1440 a 1560. Conta a célula com o centro
	// dentro da faixa: colunas 8 a 11 (centros 350 a 650) e linhas 9 e 10 (centros 1450 e 1550). Duas células de espessura:
	// cada uma cobra metade, e atravessar custa o preço cheio
	const std::vector<ZR::FFootprint> Pieces = {ZR::MakeFootprint(Barricada(), 500.0f, 1500.0f, 0)};
	const ZR::FLocalGrid Grid = ZR::MakeLocalGrid(PlantaBase(), Celula, 500.0f, 1500.0f, Config, Pieces);
	ZR_EQ(Grid.ExtraCost.size(), static_cast<std::size_t>(400));
	if (Grid.ExtraCost.size() != 400 || Grid.Walkable.size() != 400)
	{
		return;
	}
	auto CostAt = [&Grid](int X, int Y) { return Grid.ExtraCost[static_cast<std::size_t>(Y * Grid.Width + X)]; };
	ZR_EQ(Grid.CountCostly(), 8);
	ZR_EQ(CostAt(8, 9), 200);
	ZR_EQ(CostAt(11, 10), 200);
	ZR_EQ(CostAt(7, 9), 0);
	ZR_EQ(CostAt(12, 9), 0);
	ZR_EQ(CostAt(8, 8), 0);
	ZR_EQ(CostAt(8, 11), 0);
	// a barricada é cara, não parede: a célula continua andável
	ZR_EXPECT(Grid.Walkable[static_cast<std::size_t>(9 * Grid.Width + 8)] != 0);
}

namespace
{
	/** 3 x 3 células de rua de 1000 cm: 30 m de chão aberto. */
	ZR::FCityPlan PlantaAberta()
	{
		ZR::FCityPlan Plan;
		Plan.Width = 3;
		Plan.Height = 3;
		Plan.LandHeight = 3;
		Plan.Cells.assign(9, ZR::ECell::Road);
		Plan.Floors.assign(9, 0);
		return Plan;
	}

	/** Custo de andar de (1550, 550) até (1550, 2550) na grade fina de 30 m com estas barricadas (a grade cobre a planta). */
	int CustoDeAtravessar(const std::vector<ZR::FFootprint>& Pieces)
	{
		ZR::FLocalGridConfig Config;
		Config.SizeCm = 3000.0f;
		Config.ResolutionCm = 100.0f;
		Config.InflateCm = 40.0f;
		Config.BarricadeCost = 400;
		const ZR::FLocalGrid Grid = ZR::MakeLocalGrid(PlantaAberta(), Celula, 1500.0f, 1500.0f, Config, Pieces);
		ZR::FFlowField Field;
		Field.Build(Grid.Walkable, Grid.ExtraCost, Grid.Width, Grid.Height, 15, 25);
		return Field.CostAt(15, 5);
	}

	/** Uma faixa de barricada de x = Left a x = Right, deitada em y. */
	ZR::FFootprint Faixa(float Left, float Right, float Y)
	{
		ZR::FFootprint Piece;
		Piece.X = (Left + Right) * 0.5f;
		Piece.Y = Y;
		Piece.HalfX = (Right - Left) * 0.5f;
		Piece.HalfY = 20.0f;
		return Piece;
	}
}

ZR_TEST(GradeFinaPrecoCheio, "GradeFina.AtravessarABarricadaCustaOPrecoCheio")
{
	// sem barricada: 20 passos retos
	ZR_EQ(CustoDeAtravessar({}), 200);
	// a faixa crescida (120 cm) pega duas linhas de centros (1450 e 1550) ou uma só (1450): o preço é o mesmo
	ZR_EQ(CustoDeAtravessar({Faixa(0.0f, 3000.0f, 1500.0f)}), 600);
	ZR_EQ(CustoDeAtravessar({Faixa(0.0f, 3000.0f, 1450.0f)}), 600);
}

ZR_TEST(GradeFinaVao, "GradeFina.VaoDeUmMetroEOitentaEhPassagem")
{
	// vão de 1,8 m entre duas barricadas, em dois encaixes diferentes: a horda passa pelo vão sem pagar
	ZR_EQ(CustoDeAtravessar({Faixa(0.0f, 1410.0f, 1500.0f), Faixa(1590.0f, 3000.0f, 1500.0f)}), 200);
	ZR_EQ(CustoDeAtravessar({Faixa(0.0f, 1460.0f, 1500.0f), Faixa(1640.0f, 3000.0f, 1500.0f)}), 200);
}

ZR_TEST(GradeFinaLinhaLivre, "GradeFina.LinhaLivreSoPorChaoSemBarricada")
{
	ZR::FLocalGridConfig Config;
	Config.SizeCm = 2000.0f;
	Config.ResolutionCm = 100.0f;
	const std::vector<ZR::FFootprint> Pieces = {ZR::MakeFootprint(Barricada(), 500.0f, 1500.0f, 0)};
	const ZR::FLocalGrid Grid = ZR::MakeLocalGrid(PlantaBase(), Celula, 500.0f, 1500.0f, Config, Pieces);
	ZR_EXPECT(Grid.ClearLine(50.0f, 1050.0f, 950.0f, 1050.0f));    // rua aberta
	ZR_EXPECT(Grid.ClearLine(950.0f, 1850.0f, 1350.0f, 1250.0f));  // diagonal pela rua
	ZR_EXPECT(!Grid.ClearLine(500.0f, 1200.0f, 500.0f, 1800.0f));  // atravessa a barricada
	ZR_EXPECT(!Grid.ClearLine(500.0f, 550.0f, 1200.0f, 550.0f));   // entra no prédio (1, 0)
	ZR_EXPECT(!Grid.ClearLine(500.0f, 1050.0f, 500.0f, 2600.0f));  // sai da grade
}

ZR_TEST(GradeFinaCalcada, "GradeFina.CalcadaDoPredioEhChao")
{
	ZR::FLocalGridConfig Config;
	Config.SizeCm = 2000.0f;
	Config.ResolutionCm = 100.0f;
	const ZR::FLocalGrid Grid = ZR::MakeLocalGrid(PlantaComRecuo(), Celula, 500.0f, 1500.0f, Config, {});
	ZR_EQ(Grid.Walkable.size(), static_cast<std::size_t>(400));
	if (Grid.Walkable.size() != 400)
	{
		return;
	}
	auto WalkableAt = [&Grid](int X, int Y) { return Grid.Walkable[static_cast<std::size_t>(Y * Grid.Width + X)] != 0; };
	ZR_EXPECT(WalkableAt(15, 0));   // centro (1050, 550): calçada do prédio (1, 0)
	ZR_EXPECT(!WalkableAt(19, 0));  // centro (1450, 550): dentro do prédio
}

