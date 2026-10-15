// Testes: saque (o que cada lugar rende, quando volta a render e onde os lugares ficam na cidade).
// Contrato: docs/regras-de-jogo.md, seção 15. Usa ReadDataFile e ExpectNoErrors de ZRTestsData.inl.

#include <cmath>

#include "Rules/ZRLoot.h"

namespace
{
	ZR::FLootSpotType Lugar(const char* Id, ZR::ELootWhere Where, int Count)
	{
		ZR::FLootSpotType Type;
		Type.Id = Id;
		Type.Where = Where;
		Type.Count = Count;
		return Type;
	}

	ZR::FLootItemDef Item(const char* Spot, ZR::ELootItem What, float Weight, int Min, int Max)
	{
		ZR::FLootItemDef Def;
		Def.Spot = Spot;
		Def.Item = What;
		Def.Weight = Weight;
		Def.Min = Min;
		Def.Max = Max;
		return Def;
	}

	/** A célula vizinha para onde Yaw aponta (0 = leste, 90 = sul, 180 = oeste, 270 = norte). */
	void Vizinha(float Yaw, int& Dx, int& Dy)
	{
		const int Quarter = static_cast<int>(std::lround(Yaw / 90.0f)) % 4;
		Dx = Quarter == 0 ? 1 : Quarter == 2 ? -1 : 0;
		Dy = Quarter == 1 ? 1 : Quarter == 3 ? -1 : 0;
	}
}

ZR_TEST(SaquePesos, "Saque.SorteioSegueOsPesos")
{
	const ZR::FLootSpotType Lixeira = Lugar("lixeira", ZR::ELootWhere::Sidewalk, 1);
	const std::vector<ZR::FLootItemDef> Items = {
		Item("lixeira", ZR::ELootItem::Scrap, 3.0f, 5, 5),
		Item("lixeira", ZR::ELootItem::Nothing, 1.0f, 0, 0),
		// de outro lugar: nunca sai da lixeira
		Item("mochila", ZR::ELootItem::Grenade, 100.0f, 1, 1),
	};
	ZR::FRng Rng(7);
	int WithScrap = 0;
	int Grenades = 0;
	const int Rounds = 4000;
	for (int i = 0; i < Rounds; ++i)
	{
		const ZR::FLootResult Result = ZR::RollLoot(Items, Lixeira, Rng);
		WithScrap += Result.Scrap == 5 ? 1 : 0;
		Grenades += Result.Grenades;
		ZR_EXPECT(Result.Scrap == 0 || Result.Scrap == 5);
	}
	const double Share = static_cast<double>(WithScrap) / Rounds;
	ZR_NEAR(Share, 0.75, 0.03);
	ZR_EQ(Grenades, 0);
}

ZR_TEST(SaqueQuantidade, "Saque.QuantidadeEntreMinEMaxERolagensSomam")
{
	ZR::FLootSpotType Viatura = Lugar("viatura", ZR::ELootWhere::Road, 1);
	Viatura.Rolls = 2;
	const std::vector<ZR::FLootItemDef> Items = {Item("viatura", ZR::ELootItem::Ammo, 1.0f, 1, 3)};
	ZR::FRng Rng(11);
	bool bSawLowest = false;
	bool bSawHighest = false;
	for (int i = 0; i < 500; ++i)
	{
		const ZR::FLootResult Result = ZR::RollLoot(Items, Viatura, Rng);
		ZR_EXPECT(Result.AmmoPacks >= 2 && Result.AmmoPacks <= 6);
		bSawLowest = bSawLowest || Result.AmmoPacks == 2;
		bSawHighest = bSawHighest || Result.AmmoPacks == 6;
	}
	ZR_EXPECT(bSawLowest && bSawHighest);

	// vida, granada e bujão caem cada um no seu campo
	ZR::FLootSpotType Isopor = Lugar("isopor", ZR::ELootWhere::Sidewalk, 1);
	Isopor.Rolls = 3;
	const std::vector<ZR::FLootItemDef> Gelado = {Item("isopor", ZR::ELootItem::Health, 1.0f, 10, 20)};
	const ZR::FLootResult Drinks = ZR::RollLoot(Gelado, Isopor, Rng);
	ZR_EXPECT(Drinks.Health >= 30.0f && Drinks.Health <= 60.0f);
	ZR_EQ(Drinks.Scrap, 0);
	const std::vector<ZR::FLootItemDef> Bomba = {Item("isopor", ZR::ELootItem::Bomb, 1.0f, 1, 1)};
	ZR_EQ(ZR::RollLoot(Bomba, Isopor, Rng).Bombs, 3);

	// lugar sem nenhuma linha na tabela: vazio
	ZR_EXPECT(ZR::RollLoot(Items, Lugar("orelhao", ZR::ELootWhere::Sidewalk, 1), Rng).IsEmpty());
}

ZR_TEST(SaqueRenova, "Saque.RevistadoSoVoltaDepoisDosDias")
{
	ZR::FLootSpotType Mochila = Lugar("mochila", ZR::ELootWhere::Road, 1);
	ZR::FLootSpotState State;
	ZR_EXPECT(ZR::CanSearch(Mochila, State, 0));
	State.SearchedDay = 0;
	ZR_EXPECT(!ZR::CanSearch(Mochila, State, 0));
	ZR_EXPECT(!ZR::CanSearch(Mochila, State, 9)); // renova_dias 0: nunca mais

	ZR::FLootSpotType Lixeira = Lugar("lixeira", ZR::ELootWhere::Sidewalk, 1);
	Lixeira.RefillDays = 2;
	State.SearchedDay = 1;
	ZR_EXPECT(!ZR::CanSearch(Lixeira, State, 1));
	ZR_EXPECT(!ZR::CanSearch(Lixeira, State, 2));
	ZR_EXPECT(ZR::CanSearch(Lixeira, State, 3));
}

ZR_TEST(SaqueIds, "Saque.ItensPorNome")
{
	const ZR::ELootItem All[] = {ZR::ELootItem::Nothing, ZR::ELootItem::Ammo, ZR::ELootItem::Grenade, ZR::ELootItem::Bomb,
		ZR::ELootItem::Health, ZR::ELootItem::Scrap};
	for (const ZR::ELootItem What : All)
	{
		ZR::ELootItem Back = ZR::ELootItem::Nothing;
		ZR_EXPECT(ZR::ParseLootItem(ZR::LootItemId(What), Back) && Back == What);
	}
	ZR_EXPECT(std::string(ZR::LootItemId(ZR::ELootItem::Health)) == "curativo");
	ZR::ELootItem Unused = ZR::ELootItem::Nothing;
	ZR_EXPECT(!ZR::ParseLootItem("laser", Unused));
}

ZR_TEST(SaqueLugares, "Saque.LugaresNaCalcadaENaRuaEspacadosELongeDoComeco")
{
	ZR::FCityConfig City;
	City.Seed = 5;
	const ZR::FCityPlan Plan = ZR::GenerateCity(City);
	const float Cell = City.CellSizeCm;
	std::vector<ZR::FLootSpotType> Types = {Lugar("lixeira", ZR::ELootWhere::Sidewalk, 30), Lugar("viatura", ZR::ELootWhere::Road, 10)};
	Types[0].WidthCm = 70.0f;
	Types[0].DepthCm = 60.0f;
	Types[1].WidthCm = 460.0f;
	Types[1].DepthCm = 190.0f;
	const ZR::FLootConfig Config;
	// o começo: o meio de uma rua no meio da cidade
	int StartCellX = Plan.Width / 2;
	const int StartCellY = Plan.LandHeight / 2;
	while (Plan.At(StartCellX, StartCellY) != ZR::ECell::Road && StartCellX < Plan.Width)
	{
		++StartCellX;
	}
	const float StartX = (static_cast<float>(StartCellX) + 0.5f) * Cell;
	const float StartY = (static_cast<float>(StartCellY) + 0.5f) * Cell;

	const std::vector<ZR::FLootPlace> Places = ZR::PlaceLoot(Plan, Cell, Types, Config, 99, StartX, StartY);
	int Counts[2] = {0, 0};
	for (std::size_t i = 0; i < Places.size(); ++i)
	{
		const ZR::FLootPlace& Place = Places[i];
		ZR_EXPECT(Place.Type == 0 || Place.Type == 1);
		if (Place.Type != 0 && Place.Type != 1)
		{
			continue;
		}
		++Counts[Place.Type];
		ZR_EXPECT(Plan.IsWalkablePoint(Place.X, Place.Y, Cell));
		ZR_EXPECT(std::fmod(Place.Yaw, 90.0f) == 0.0f && Place.Yaw >= 0.0f && Place.Yaw < 360.0f);
		const int CellX = static_cast<int>(std::floor(Place.X / Cell));
		const int CellY = static_cast<int>(std::floor(Place.Y / Cell));
		if (Place.Type == 0)
		{
			// na calçada: célula de prédio, de frente para a rua, e o lixo inteiro cabe no chão
			ZR_EXPECT(Plan.At(CellX, CellY) == ZR::ECell::Building);
			int Dx = 0;
			int Dy = 0;
			Vizinha(Place.Yaw, Dx, Dy);
			ZR_EXPECT(Plan.At(CellX + Dx, CellY + Dy) == ZR::ECell::Road);
			ZR_EXPECT(Plan.IsWalkableRect(Place.X - 35.0f, Place.Y - 35.0f, Place.X + 35.0f, Place.Y + 35.0f, Cell));
		}
		else
		{
			ZR_EXPECT(Plan.At(CellX, CellY) == ZR::ECell::Road);
		}
		ZR_EXPECT(std::hypot(Place.X - StartX, Place.Y - StartY) >= Config.AvoidStartCm);
		for (std::size_t j = i + 1; j < Places.size(); ++j)
		{
			ZR_EXPECT(std::hypot(Place.X - Places[j].X, Place.Y - Places[j].Y) >= Config.MinSpacingCm);
		}
	}
	ZR_EQ(Counts[0], 30);
	ZR_EQ(Counts[1], 10);

	// mesma semente, mesma cidade; outra semente, outros lugares
	const std::vector<ZR::FLootPlace> Again = ZR::PlaceLoot(Plan, Cell, Types, Config, 99, StartX, StartY);
	ZR_EQ(Again.size(), Places.size());
	bool bSame = Again.size() == Places.size();
	for (std::size_t i = 0; bSame && i < Places.size(); ++i)
	{
		bSame = Again[i].X == Places[i].X && Again[i].Y == Places[i].Y && Again[i].Type == Places[i].Type;
	}
	ZR_EXPECT(bSame);
	const std::vector<ZR::FLootPlace> Other = ZR::PlaceLoot(Plan, Cell, Types, Config, 100, StartX, StartY);
	bool bDifferent = Other.size() != Places.size();
	for (std::size_t i = 0; !bDifferent && i < Places.size(); ++i)
	{
		bDifferent = Other[i].X != Places[i].X || Other[i].Y != Places[i].Y;
	}
	ZR_EXPECT(bDifferent);
}

ZR_TEST(DadosSaqueErros, "Dados.SaqueComErroEhApontado")
{
	const std::string SpotsCsv =
		"id,nome_pt,nome_en,onde,busca_s,rolagens,barulho,quantidade,renova_dias,largura_cm,profundidade_cm,altura_cm,cor\n"
		"lixeira,Lixeira,Trash bin,calcada,2,1,3,40,2,70,60,110,#2F4A2F\n"
		"viatura,Viatura,Police car,rua,3.5,2,5,6,0,460,190,150,#DDE3EA\n"
		"lixeira,Lixeira 2,Bin,calcada,2,1,3,4,0,70,60,110,#2F4A2F\n"
		"telhado,Telhado,Roof,telhado,2,1,0,4,0,70,60,110,#000000\n"
		"lenta,Lenta,Slow,calcada,-1,1,0,4,0,70,60,110,#000000\n";
	std::vector<ZR::FLootSpotType> Spots;
	std::vector<std::string> Errors;
	ZR::LoadLootSpots(SpotsCsv, Spots, Errors);
	ZR_EQ(Spots.size(), static_cast<std::size_t>(2));
	ZR_EQ(Errors.size(), static_cast<std::size_t>(3));
	const ZR::FLootSpotType* Lixeira = ZR::FindById(Spots, "lixeira");
	ZR_EXPECT(Lixeira != nullptr && Lixeira->Where == ZR::ELootWhere::Sidewalk && Lixeira->Count == 40 && Lixeira->RefillDays == 2);
	ZR_EXPECT(Lixeira != nullptr && std::fabs(Lixeira->SearchSeconds - 2.0f) < 1e-6f && std::fabs(Lixeira->Noise - 3.0f) < 1e-6f);
	ZR_EXPECT(Lixeira != nullptr && std::fabs(Lixeira->WidthCm - 70.0f) < 1e-6f && Lixeira->ColorRgb == 0x2F4A2Fu);
	const ZR::FLootSpotType* Viatura = ZR::FindById(Spots, "viatura");
	ZR_EXPECT(Viatura != nullptr && Viatura->Where == ZR::ELootWhere::Road && Viatura->Rolls == 2);

	const std::string ItemsCsv =
		"local,item,peso,min,max\n"
		"lixeira,sucata,45,5,12\n"
		"lixeira,nada,40,0,0\n"
		"viatura,municao,60,1,2\n"
		"telhado,sucata,1,1,1\n"
		"lixeira,laser,1,1,1\n"
		"lixeira,curativo,-1,1,1\n"
		"lixeira,granada,1,3,1\n";
	std::vector<ZR::FLootItemDef> Items;
	Errors.clear();
	ZR::LoadLootItems(ItemsCsv, Spots, Items, Errors);
	ZR_EQ(Items.size(), static_cast<std::size_t>(3));
	ZR_EQ(Errors.size(), static_cast<std::size_t>(4));
	ZR_EXPECT(Items.size() == 3 && Items[0].Spot == "lixeira" && Items[0].Item == ZR::ELootItem::Scrap && Items[0].Max == 12);
	ZR_EXPECT(Items.size() == 3 && Items[2].Item == ZR::ELootItem::Ammo && std::fabs(Items[2].Weight - 60.0f) < 1e-6f);
}

ZR_TEST(DerivaSaque, "Deriva.SaqueCarregaETodoLugarRendeAlgo")
{
	std::string SpotsText;
	std::string ItemsText;
	std::string TuningText;
	if (!ReadDataFile(T, "Saque.csv", SpotsText) || !ReadDataFile(T, "SaqueItens.csv", ItemsText)
		|| !ReadDataFile(T, "Ajustes.csv", TuningText))
	{
		return;
	}
	std::vector<ZR::FLootSpotType> Spots;
	std::vector<ZR::FLootItemDef> Items;
	std::vector<std::string> Errors;
	ZR::LoadLootSpots(SpotsText, Spots, Errors);
	ExpectNoErrors(T, "Saque.csv", Errors);
	Errors.clear();
	ZR::LoadLootItems(ItemsText, Spots, Items, Errors);
	ExpectNoErrors(T, "SaqueItens.csv", Errors);
	ZR_EXPECT(Spots.size() >= 3);
	int Wanted = 0;
	for (const ZR::FLootSpotType& Spot : Spots)
	{
		Wanted += Spot.Count;
		float Useful = 0.0f;
		for (const ZR::FLootItemDef& Def : Items)
		{
			Useful += Def.Spot == Spot.Id && Def.Item != ZR::ELootItem::Nothing && Def.Max > 0 ? Def.Weight : 0.0f;
		}
		if (Useful <= 0.0f)
		{
			T.Fail("o lugar " + Spot.Id + " não rende nada em SaqueItens.csv");
		}
	}
	// a cidade das tabelas tem lugar para quase todos
	ZR::FTuning Tuning;
	std::vector<std::string> Missing;
	Errors.clear();
	ZR::LoadTuning(TuningText, Tuning, Errors, Missing);
	const ZR::FCityPlan Plan = ZR::GenerateCity(Tuning.City);
	const std::vector<ZR::FLootPlace> Places = ZR::PlaceLoot(Plan, Tuning.City.CellSizeCm, Spots, Tuning.Loot, 1,
		Tuning.City.CellSizeCm * 0.5f, Tuning.City.CellSizeCm * 0.5f);
	ZR_EXPECT(Wanted > 0);
	ZR_EXPECT(static_cast<int>(Places.size()) * 10 >= Wanted * 9);
}
