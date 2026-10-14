// Testes: base do jogador (sucata, barricadas e armadilhas).
// Contrato: docs/regras-de-jogo.md, seção 12.

#include <algorithm>

#include "Rules/ZRBase.h"
#include "Rules/ZRData.h"

namespace
{
	/** 400 x 40 cm, 110 de altura, 20 de sucata, 400 de vida. */
	ZR::FBuildPieceType Barricada()
	{
		ZR::FBuildPieceType Type;
		Type.Id = "barricada";
		Type.Kind = ZR::EPieceKind::Barricade;
		Type.Cost = 20;
		Type.Health = 400.0f;
		Type.WidthCm = 400.0f;
		Type.DepthCm = 40.0f;
		Type.HeightCm = 110.0f;
		return Type;
	}

	/** 60 de dano por segundo nas pernas, segura em 60% e gasta depois de 100 de dano. */
	ZR::FBuildPieceType Estacas()
	{
		ZR::FBuildPieceType Type;
		Type.Id = "estacas";
		Type.Kind = ZR::EPieceKind::Trap;
		Type.Cost = 15;
		Type.Health = 150.0f;
		Type.WidthCm = 240.0f;
		Type.DepthCm = 90.0f;
		Type.HeightCm = 40.0f;
		Type.DamagePerSecond = 60.0f;
		Type.Target = ZR::ETrapTarget::Legs;
		Type.SlowFactor = 0.6f;
		Type.Durability = 100.0f;
		return Type;
	}

	/** 4 x 2 células de 1000 cm. Linha 0: rua, prédio, rua, praça. Linha 1: rua, rua, praia, mar. */
	ZR::FCityPlan PlantaBase()
	{
		ZR::FCityPlan Plan;
		Plan.Width = 4;
		Plan.Height = 2;
		Plan.LandHeight = 1;
		Plan.Cells = {ZR::ECell::Road, ZR::ECell::Building, ZR::ECell::Road, ZR::ECell::Plaza,
			ZR::ECell::Road, ZR::ECell::Road, ZR::ECell::Beach, ZR::ECell::Sea};
		Plan.Floors.assign(8, 0);
		Plan.Floors[1] = 4;
		return Plan;
	}

	constexpr float Celula = 1000.0f;

	/** Pedido que passa em tudo: barricada deitada no eixo X no meio da rua (0, 1), jogador 3 m ao sul, sucata de sobra. */
	ZR::FPlaceQuery PedidoBom(const ZR::FBuildPieceType& Type)
	{
		ZR::FPlaceQuery Query;
		Query.Piece = ZR::MakeFootprint(Type, 500.0f, 1500.0f, 0);
		Query.Cost = Type.Cost;
		Query.Scrap = 100;
		Query.PieceCount = 0;
		Query.PlayerX = 500.0f;
		Query.PlayerY = 1800.0f;
		return Query;
	}

	ZR::EPlaceResult Checa(const ZR::FPlaceQuery& Query, const std::vector<ZR::FFootprint>& Existing = {})
	{
		return ZR::CheckPlacement(PlantaBase(), Celula, ZR::FBaseConfig(), Query, Existing);
	}

	/** A barricada com o centro em (X, Y), girada Rot90 quartos de volta, com o jogador 3 m ao sul. */
	ZR::EPlaceResult ChecaEm(float X, float Y, int Rot90)
	{
		ZR::FPlaceQuery Query = PedidoBom(Barricada());
		Query.Piece = ZR::MakeFootprint(Barricada(), X, Y, Rot90);
		Query.PlayerX = X;
		Query.PlayerY = Y + 300.0f;
		return Checa(Query);
	}
}

ZR_TEST(BasePegadaGira, "Base.PegadaGiraDeNoventaEmNoventaGraus")
{
	const ZR::FBuildPieceType Type = Barricada();
	const ZR::FFootprint Reta = ZR::MakeFootprint(Type, 100.0f, 200.0f, 0);
	ZR_NEAR(Reta.X, 100.0f, 1e-4);
	ZR_NEAR(Reta.Y, 200.0f, 1e-4);
	ZR_NEAR(Reta.HalfX, 200.0f, 1e-4);
	ZR_NEAR(Reta.HalfY, 20.0f, 1e-4);
	const ZR::FFootprint Virada = ZR::MakeFootprint(Type, 0.0f, 0.0f, 1);
	ZR_NEAR(Virada.HalfX, 20.0f, 1e-4);
	ZR_NEAR(Virada.HalfY, 200.0f, 1e-4);
	ZR_NEAR(ZR::MakeFootprint(Type, 0.0f, 0.0f, 2).HalfX, 200.0f, 1e-4);
	ZR_NEAR(ZR::MakeFootprint(Type, 0.0f, 0.0f, -1).HalfX, 20.0f, 1e-4);
	ZR_NEAR(ZR::MakeFootprint(Type, 0.0f, 0.0f, 7).HalfX, 20.0f, 1e-4);
}

ZR_TEST(BaseEncostar, "Base.EncostarNaoEhSobrepor")
{
	const ZR::FBuildPieceType Type = Barricada();
	const ZR::FFootprint A = ZR::MakeFootprint(Type, 0.0f, 0.0f, 0);
	ZR_EXPECT(ZR::FootprintsOverlap(A, A));
	ZR_EXPECT(ZR::FootprintsOverlap(A, ZR::MakeFootprint(Type, 300.0f, 0.0f, 0)));
	// lado a lado, borda com borda: é assim que se faz um muro
	ZR_EXPECT(!ZR::FootprintsOverlap(A, ZR::MakeFootprint(Type, 400.0f, 0.0f, 0)));
	ZR_EXPECT(!ZR::FootprintsOverlap(A, ZR::MakeFootprint(Type, 0.0f, 40.0f, 0)));
	ZR_EXPECT(ZR::FootprintsOverlap(A, ZR::MakeFootprint(Type, 0.0f, 39.0f, 0)));
	// em L: a virada encosta na quina sem sobrepor; um pouco para dentro, sobrepõe
	ZR_EXPECT(!ZR::FootprintsOverlap(A, ZR::MakeFootprint(Type, 220.0f, 220.0f, 1)));
	ZR_EXPECT(ZR::FootprintsOverlap(A, ZR::MakeFootprint(Type, 210.0f, 100.0f, 1)));
}

ZR_TEST(BaseGrade, "Base.GradeArredondaParaOMaisPerto")
{
	ZR_NEAR(ZR::SnapToGrid(124.0f, 50.0f), 100.0f, 1e-4);
	ZR_NEAR(ZR::SnapToGrid(126.0f, 50.0f), 150.0f, 1e-4);
	ZR_NEAR(ZR::SnapToGrid(75.0f, 50.0f), 100.0f, 1e-4);
	ZR_NEAR(ZR::SnapToGrid(-26.0f, 50.0f), -50.0f, 1e-4);
	ZR_NEAR(ZR::SnapToGrid(1234.5f, 0.0f), 1234.5f, 1e-4);
	ZR_NEAR(ZR::SnapToGrid(1234.5f, -10.0f), 1234.5f, 1e-4);
}

ZR_TEST(BaseConstruirPede, "Base.ConstruirPedeSucataVagaEAlcance")
{
	const ZR::FBuildPieceType Type = Barricada();
	const ZR::FBaseConfig Config;
	ZR_EQ(Checa(PedidoBom(Type)), ZR::EPlaceResult::Ok);

	ZR::FPlaceQuery Pobre = PedidoBom(Type);
	Pobre.Scrap = 19;
	ZR_EQ(Checa(Pobre), ZR::EPlaceResult::NoScrap);
	Pobre.Scrap = 20;
	ZR_EQ(Checa(Pobre), ZR::EPlaceResult::Ok);

	ZR::FPlaceQuery Cheio = PedidoBom(Type);
	Cheio.PieceCount = Config.MaxPieces;
	ZR_EQ(Checa(Cheio), ZR::EPlaceResult::TooMany);
	Cheio.PieceCount = Config.MaxPieces - 1;
	ZR_EQ(Checa(Cheio), ZR::EPlaceResult::Ok);
	// o primeiro motivo é o que volta: sem sucata vem antes de sem vaga
	Cheio.PieceCount = Config.MaxPieces;
	Cheio.Scrap = 0;
	ZR_EQ(Checa(Cheio), ZR::EPlaceResult::NoScrap);

	ZR::FPlaceQuery Longe = PedidoBom(Type);
	Longe.PlayerY = 1500.0f + Config.BuildRangeCm + 1.0f;
	ZR_EQ(Checa(Longe), ZR::EPlaceResult::TooFar);
	Longe.PlayerY = 1500.0f + Config.BuildRangeCm;
	ZR_EQ(Checa(Longe), ZR::EPlaceResult::Ok);
}

ZR_TEST(BaseSoNaRua, "Base.SoConstroiEmChaoAndavel")
{
	ZR_EQ(ChecaEm(1500.0f, 500.0f, 0), ZR::EPlaceResult::OffStreet);  // prédio
	ZR_EQ(ChecaEm(950.0f, 500.0f, 0), ZR::EPlaceResult::OffStreet);   // metade na rua, metade no prédio
	ZR_EQ(ChecaEm(800.0f, 500.0f, 0), ZR::EPlaceResult::Ok);          // encostada na parede do prédio
	ZR_EQ(ChecaEm(3500.0f, 500.0f, 0), ZR::EPlaceResult::Ok);         // praça
	ZR_EQ(ChecaEm(2500.0f, 1500.0f, 0), ZR::EPlaceResult::Ok);        // praia
	ZR_EQ(ChecaEm(3500.0f, 1500.0f, 0), ZR::EPlaceResult::OffStreet); // mar
	ZR_EQ(ChecaEm(1000.0f, 1500.0f, 0), ZR::EPlaceResult::Ok);        // entre duas células de rua
	ZR_EQ(ChecaEm(500.0f, 990.0f, 1), ZR::EPlaceResult::Ok);          // virada, entre duas células de rua
	ZR_EQ(ChecaEm(150.0f, 500.0f, 0), ZR::EPlaceResult::OffStreet);   // sai da planta
}

ZR_TEST(BaseJogadorEPecas, "Base.NaoConstroiEmCimaDoJogadorNemDeOutraPeca")
{
	const ZR::FBuildPieceType Type = Barricada();
	// a peça vai de y = 1480 a 1520; a folga do jogador é 50 cm
	ZR::FPlaceQuery Query = PedidoBom(Type);
	Query.PlayerY = 1500.0f;
	ZR_EQ(Checa(Query), ZR::EPlaceResult::OnPlayer);
	Query.PlayerY = 1569.0f;
	ZR_EQ(Checa(Query), ZR::EPlaceResult::OnPlayer);
	Query.PlayerY = 1571.0f;
	ZR_EQ(Checa(Query), ZR::EPlaceResult::Ok);
	// na quina conta a distância até a quina
	Query.PlayerX = 700.0f + 30.0f;
	Query.PlayerY = 1520.0f + 30.0f;
	ZR_EQ(Checa(Query), ZR::EPlaceResult::OnPlayer);
	Query.PlayerX = 700.0f + 40.0f;
	Query.PlayerY = 1520.0f + 40.0f;
	ZR_EQ(Checa(Query), ZR::EPlaceResult::Ok);

	const ZR::FPlaceQuery Good = PedidoBom(Type);
	ZR_EQ(Checa(Good, {ZR::MakeFootprint(Type, 700.0f, 1500.0f, 0)}), ZR::EPlaceResult::Blocked);
	ZR_EQ(Checa(Good, {ZR::MakeFootprint(Type, 900.0f, 1500.0f, 0)}), ZR::EPlaceResult::Ok);
	ZR_EQ(Checa(Good, {ZR::MakeFootprint(Type, 2500.0f, 1500.0f, 0), ZR::MakeFootprint(Type, 500.0f, 1510.0f, 1)}),
		ZR::EPlaceResult::Blocked);
	// jogador em cima vem antes de peça em cima
	ZR::FPlaceQuery OnTop = PedidoBom(Type);
	OnTop.PlayerY = 1500.0f;
	ZR_EQ(Checa(OnTop, {ZR::MakeFootprint(Type, 700.0f, 1500.0f, 0)}), ZR::EPlaceResult::OnPlayer);
}

ZR_TEST(BaseBarricadaApanha, "Base.BarricadaApanhaMudaDeEstagioEQuebra")
{
	const ZR::FBuildPieceType Type = Barricada();
	ZR::FPieceState State;
	State.Reset(Type);
	ZR_NEAR(State.Health, 400.0f, 1e-4);
	ZR_NEAR(State.Wear, 0.0f, 1e-4);
	ZR_EXPECT(!State.bBroken);
	ZR_EQ(ZR::DamageStage(Type, State), 0);

	ZR_EXPECT(!ZR::DamagePiece(State, 100.0f));
	ZR_NEAR(State.Health, 300.0f, 1e-4);
	ZR_EQ(ZR::DamageStage(Type, State), 0);
	ZR_EXPECT(!ZR::DamagePiece(State, 50.0f));
	ZR_EQ(ZR::DamageStage(Type, State), 1);
	ZR_EXPECT(!ZR::DamagePiece(State, 120.0f));
	ZR_NEAR(State.Health, 130.0f, 1e-4);
	ZR_EQ(ZR::DamageStage(Type, State), 2);
	// dano negativo não cura
	ZR_EXPECT(!ZR::DamagePiece(State, -50.0f));
	ZR_NEAR(State.Health, 130.0f, 1e-4);

	ZR_EXPECT(ZR::DamagePiece(State, 500.0f));
	ZR_EXPECT(State.bBroken);
	ZR_NEAR(State.Health, 0.0f, 1e-4);
	ZR_EQ(ZR::DamageStage(Type, State), 3);
	// já quebrada não "quebra de novo"
	ZR_EXPECT(!ZR::DamagePiece(State, 10.0f));
}

ZR_TEST(BaseConserto, "Base.ConsertoCobraPelaVidaQueFaltaEConsertaParte")
{
	// preço 20, conserto do zero 60%: 12 de sucata para a vida inteira (400)
	ZR::FBuildPieceType Type = Barricada();
	const ZR::FBaseConfig Config;
	ZR::FPieceState State;
	State.Reset(Type);

	State.Health = 200.0f;
	ZR::FRepairResult Repair = ZR::RepairPiece(Type, State, 100, Config);
	ZR_NEAR(Repair.Health, 400.0f, 1e-3);
	ZR_EQ(Repair.ScrapSpent, 6);

	// 3,0 exato não pode virar 4 por erro de arredondamento
	State.Health = 300.0f;
	Repair = ZR::RepairPiece(Type, State, 100, Config);
	ZR_EQ(Repair.ScrapSpent, 3);

	// um arranhão custa 1
	State.Health = 390.0f;
	Repair = ZR::RepairPiece(Type, State, 100, Config);
	ZR_NEAR(Repair.Health, 400.0f, 1e-3);
	ZR_EQ(Repair.ScrapSpent, 1);

	// com pouca sucata conserta a parte que ela paga
	State.Health = 200.0f;
	Repair = ZR::RepairPiece(Type, State, 3, Config);
	ZR_NEAR(Repair.Health, 300.0f, 1e-2);
	ZR_EQ(Repair.ScrapSpent, 3);

	State.Health = 400.0f;
	Repair = ZR::RepairPiece(Type, State, 100, Config);
	ZR_EQ(Repair.ScrapSpent, 0);
	ZR_NEAR(Repair.Health, 400.0f, 1e-3);

	State.Health = 200.0f;
	Repair = ZR::RepairPiece(Type, State, 0, Config);
	ZR_EQ(Repair.ScrapSpent, 0);
	ZR_NEAR(Repair.Health, 200.0f, 1e-3);

	State.Health = 0.0f;
	State.bBroken = true;
	Repair = ZR::RepairPiece(Type, State, 100, Config);
	ZR_EQ(Repair.ScrapSpent, 0);
	ZR_NEAR(Repair.Health, 0.0f, 1e-3);

	// peça de graça conserta de graça
	Type.Cost = 0;
	State.bBroken = false;
	State.Health = 100.0f;
	Repair = ZR::RepairPiece(Type, State, 0, Config);
	ZR_NEAR(Repair.Health, 400.0f, 1e-3);
	ZR_EQ(Repair.ScrapSpent, 0);
}

ZR_TEST(BaseDesmontar, "Base.DesmontarDevolveMetadePelaVida")
{
	ZR::FBuildPieceType Type = Barricada();
	const ZR::FBaseConfig Config;
	ZR::FPieceState State;
	State.Reset(Type);
	ZR_EQ(ZR::DismantleRefund(Type, State, Config), 10);
	State.Health = 200.0f;
	ZR_EQ(ZR::DismantleRefund(Type, State, Config), 5);
	State.Health = 190.0f;
	ZR_EQ(ZR::DismantleRefund(Type, State, Config), 4);
	State.Health = 0.0f;
	State.bBroken = true;
	ZR_EQ(ZR::DismantleRefund(Type, State, Config), 0);
	Type.Cost = 15;
	State.Reset(Type);
	ZR_EQ(ZR::DismantleRefund(Type, State, Config), 7);
}

ZR_TEST(BaseEstacas, "Base.EstacasMachucamAsPernasSeguramEGastam")
{
	const ZR::FBuildPieceType Type = Estacas();
	ZR::FPieceState State;
	State.Reset(Type);
	ZR::FTrapContact Contact = ZR::TouchTrap(Type, State, 0.5f);
	ZR_NEAR(Contact.Damage, 30.0f, 1e-4);
	ZR_EQ(Contact.Target, ZR::ETrapTarget::Legs);
	ZR_NEAR(Contact.SpeedFactor, 0.6f, 1e-4);
	ZR_EXPECT(!Contact.bDetonate);
	ZR_EXPECT(!Contact.bBrokeNow);
	ZR_NEAR(State.Wear, 30.0f, 1e-4);

	// dois zumbis no mesmo passo gastam em dobro
	ZR::TouchTrap(Type, State, 0.5f);
	ZR::TouchTrap(Type, State, 0.5f);
	ZR_NEAR(State.Wear, 90.0f, 1e-4);
	// o último golpe só dá o que falta do desgaste
	Contact = ZR::TouchTrap(Type, State, 0.5f);
	ZR_NEAR(Contact.Damage, 10.0f, 1e-4);
	ZR_EXPECT(Contact.bBrokeNow);
	ZR_EXPECT(State.bBroken);
	// gasta: não machuca nem segura mais
	Contact = ZR::TouchTrap(Type, State, 0.5f);
	ZR_NEAR(Contact.Damage, 0.0f, 1e-4);
	ZR_NEAR(Contact.SpeedFactor, 1.0f, 1e-4);
	ZR_EXPECT(!Contact.bBrokeNow);
}

ZR_TEST(BaseArame, "Base.ArameSeguraEMachucaOCorpoSemGastar")
{
	ZR::FBuildPieceType Type = Estacas();
	Type.Id = "arame";
	Type.DamagePerSecond = 15.0f;
	Type.Target = ZR::ETrapTarget::Body;
	Type.SlowFactor = 0.3f;
	Type.Durability = 0.0f;
	ZR::FPieceState State;
	State.Reset(Type);
	float Total = 0.0f;
	for (int i = 0; i < 1000; ++i)
	{
		const ZR::FTrapContact Contact = ZR::TouchTrap(Type, State, 0.1f);
		ZR_EQ(Contact.Target, ZR::ETrapTarget::Body);
		ZR_NEAR(Contact.SpeedFactor, 0.3f, 1e-4);
		Total += Contact.Damage;
	}
	ZR_NEAR(Total, 1500.0f, 0.5f);
	ZR_EXPECT(!State.bBroken);
}

ZR_TEST(BaseBujaoArmado, "Base.BujaoArmadoExplodeUmaVezSo")
{
	ZR::FBuildPieceType Type;
	Type.Id = "bujao_armado";
	Type.Kind = ZR::EPieceKind::Trap;
	Type.Health = 30.0f;
	Type.ExplosiveId = "bujao";
	ZR_EXPECT(Type.IsExplosive());
	ZR::FPieceState State;
	State.Reset(Type);
	ZR::FTrapContact Contact = ZR::TouchTrap(Type, State, 0.1f);
	ZR_EXPECT(Contact.bDetonate);
	ZR_EXPECT(Contact.bBrokeNow);
	ZR_EXPECT(State.bBroken);
	Contact = ZR::TouchTrap(Type, State, 0.1f);
	ZR_EXPECT(!Contact.bDetonate);
	ZR_EXPECT(!Contact.bBrokeNow);
}

ZR_TEST(DadosConstrucoesErros, "Dados.ConstrucaoComErroEhApontada")
{
	std::vector<ZR::FExplosiveType> Explosives(1);
	Explosives[0].Id = "bujao";
	const std::string Csv =
		"id,nome_pt,nome_en,tipo,tecla,custo_sucata,vida,largura_cm,profundidade_cm,altura_cm,dano_por_s,alvo,lentidao,desgaste,explosivo,cor\n"
		"boa,Boa,Good,barricada,1,20,400,400,40,110,0,,1,0,,#7A5230\n"
		"tipo_ruim,X,X,torre,2,20,400,400,40,110,0,,1,0,,#7A5230\n"
		"tecla_repetida,X,X,barricada,1,20,400,400,40,110,0,,1,0,,#7A5230\n"
		"sem_efeito,X,X,armadilha,3,20,100,100,100,30,0,,1,0,,#7A5230\n"
		"explosivo_ruim,X,X,armadilha,4,20,30,80,80,90,0,,1,0,dinamite,#7A5230\n"
		"alvo_ruim,X,X,armadilha,5,20,100,100,100,30,10,cabeca,1,50,,#7A5230\n"
		"bujao,Bujão,Tank,armadilha,6,25,30,80,80,90,0,,1,0,bujao,#B03A2E\n";
	std::vector<ZR::FBuildPieceType> Pieces;
	std::vector<std::string> Errors;
	ZR::LoadBuildPieces(Csv, Explosives, Pieces, Errors);
	ZR_EQ(Pieces.size(), static_cast<std::size_t>(2));
	ZR_EQ(Errors.size(), static_cast<std::size_t>(5));
	auto Mentions = [&Errors](const char* Text)
	{
		for (const std::string& Error : Errors)
		{
			if (Error.find(Text) != std::string::npos)
			{
				return true;
			}
		}
		return false;
	};
	ZR_EXPECT(Mentions("torre"));
	ZR_EXPECT(Mentions("tecla"));
	ZR_EXPECT(Mentions("sem efeito"));
	ZR_EXPECT(Mentions("dinamite"));
	ZR_EXPECT(Mentions("cabeca"));
	if (Pieces.size() == 2)
	{
		ZR_EQ(Pieces[0].Id, std::string("boa"));
		ZR_EQ(Pieces[0].Kind, ZR::EPieceKind::Barricade);
		ZR_EQ(Pieces[0].Slot, 1);
		ZR_EQ(Pieces[1].Kind, ZR::EPieceKind::Trap);
		ZR_EQ(Pieces[1].ExplosiveId, std::string("bujao"));
		ZR_EQ(Pieces[1].ColorRgb, 0xB03A2Eu);
	}
}

ZR_TEST(DerivaConstrucoes, "Deriva.ConstrucoesCarregamEFazemSentido")
{
	std::string ExplosivesText;
	std::string PiecesText;
	std::string TuningText;
	if (!ReadDataFile(T, "Explosivos.csv", ExplosivesText) || !ReadDataFile(T, "Construcoes.csv", PiecesText)
		|| !ReadDataFile(T, "Ajustes.csv", TuningText))
	{
		return;
	}
	std::vector<std::string> Errors;
	std::vector<ZR::FExplosiveType> Explosives;
	ZR::LoadExplosives(ExplosivesText, Explosives, Errors);
	std::vector<ZR::FBuildPieceType> Pieces;
	ZR::LoadBuildPieces(PiecesText, Explosives, Pieces, Errors);
	ZR::FTuning Tuning;
	std::vector<std::string> Missing;
	ZR::LoadTuning(TuningText, Tuning, Errors, Missing);
	ExpectNoErrors(T, "Construcoes.csv", Errors);

	int CheapestBarricade = 1 << 30;
	bool bJumpable = false;
	int Traps = 0;
	for (const ZR::FBuildPieceType& Piece : Pieces)
	{
		if (Piece.Kind == ZR::EPieceKind::Barricade)
		{
			CheapestBarricade = std::min(CheapestBarricade, Piece.Cost);
			// o pulo do jogador passa de ~1,3 m: a barricada baixa dá para pular, os zumbis não pulam
			bJumpable = bJumpable || Piece.HeightCm <= 120.0f;
		}
		else
		{
			++Traps;
		}
	}
	ZR_EXPECT(Traps >= 2);
	ZR_EXPECT(bJumpable);
	// a sucata do começo paga pelo menos duas barricadas
	ZR_EXPECT(CheapestBarricade * 2 <= Tuning.Base.StartScrap);
	ZR_EXPECT(Tuning.Base.RefundFraction < 1.0f);
}
