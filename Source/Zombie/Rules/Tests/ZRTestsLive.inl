// Testes: marcos de seguidores e o resumo da live. Contrato: docs/regras-de-jogo.md, seção 14.
// Usa ReadDataFile e ExpectNoErrors de ZRTestsData.inl.

#include "Rules/ZRLive.h"

namespace
{
	std::vector<ZR::FMilestoneDef> Marcos()
	{
		std::vector<ZR::FMilestoneDef> Defs(3);
		Defs[0].Id = "cinquenta";
		Defs[0].Followers = 50.0f;
		Defs[1].Id = "cem";
		Defs[1].Followers = 100.0f;
		Defs[2].Id = "duzentos_e_cinquenta";
		Defs[2].Followers = 250.0f;
		return Defs;
	}
}

ZR_TEST(MarcoUmaVez, "Marco.PagaUmaVezSoEmOrdem")
{
	const std::vector<ZR::FMilestoneDef> Defs = Marcos();
	ZR::FMilestoneTracker Tracker;
	Tracker.Reset(Defs, 37.0f);
	ZR_EXPECT(Tracker.Update(Defs, 45.0f).empty());
	ZR_EXPECT(Tracker.Update(Defs, 60.0f) == std::vector<int>({0}));
	ZR_EXPECT(Tracker.Update(Defs, 70.0f).empty());
	// dois de uma vez: os dois, em ordem
	ZR_EXPECT(Tracker.Update(Defs, 300.0f) == std::vector<int>({1, 2}));
	ZR_EXPECT(Tracker.Update(Defs, 300.0f).empty());
	// caiu e voltou: não paga de novo
	ZR_EXPECT(Tracker.Update(Defs, 40.0f).empty());
	ZR_EXPECT(Tracker.Update(Defs, 60.0f).empty());
}

ZR_TEST(MarcoComeco, "Marco.JaPassadoNoComecoNaoPagaEOProximo")
{
	const std::vector<ZR::FMilestoneDef> Defs = Marcos();
	ZR::FMilestoneTracker Tracker;
	Tracker.Reset(Defs, 120.0f);
	ZR_EQ(Tracker.NextIndex(Defs), 2);
	ZR_EXPECT(Tracker.Update(Defs, 130.0f).empty());
	ZR_EXPECT(Tracker.Update(Defs, 250.0f) == std::vector<int>({2}));
	ZR_EQ(Tracker.NextIndex(Defs), -1);
	Tracker.Reset(Defs, 0.0f);
	ZR_EQ(Tracker.NextIndex(Defs), 0);
}

ZR_TEST(ResumoPicos, "Resumo.PicosESeguidoresGanhos")
{
	ZR::FLiveStats Stats;
	Stats.Reset(37.0f);
	Stats.Sample(1.0, 10.0f, 40.0f, 2);
	Stats.Sample(2.0, 50.0f, 60.0f, 5);
	Stats.Sample(3.0, 20.0f, 70.0f, 1);
	ZR_NEAR(Stats.PeakViewers, 50.0f, 1e-4);
	ZR_EQ(Stats.BestCombo, 5);
	ZR_NEAR(Stats.Followers, 70.0f, 1e-4);
	ZR_NEAR(Stats.FollowersGained(), 33.0f, 1e-4);
	ZR_NEAR(Stats.LiveSeconds, 3.0, 1e-9);
	// seguidores caindo abaixo do começo não dão ganho negativo
	Stats.Sample(4.0, 0.0f, 30.0f, 0);
	ZR_NEAR(Stats.FollowersGained(), 0.0f, 1e-4);
	ZR_NEAR(Stats.PeakViewers, 50.0f, 1e-4);
	// recomeçar zera tudo
	Stats.Kills = 9;
	Stats.Reset(10.0f);
	ZR_EQ(Stats.Kills, 0);
	ZR_NEAR(Stats.PeakViewers, 0.0f, 1e-4);
	ZR_NEAR(Stats.StartFollowers, 10.0f, 1e-4);
}

ZR_TEST(ResumoNota, "Resumo.NotaPeloPicoDeEspectadores")
{
	const ZR::FGradeConfig Config;
	ZR::FLiveStats Stats;
	auto Nota = [&](float Peak)
	{
		Stats.PeakViewers = Peak;
		return ZR::LiveGrade(Config, Stats);
	};
	ZR_EQ(Nota(10.0f), 'D');
	ZR_EQ(Nota(25.0f), 'C');
	ZR_EQ(Nota(99.0f), 'C');
	ZR_EQ(Nota(100.0f), 'B');
	ZR_EQ(Nota(500.0f), 'A');
	ZR_EQ(Nota(2000.0f), 'S');
}

ZR_TEST(DadosMarcosErros, "Dados.MarcoForaDeOrdemOuRepetidoEhErro")
{
	const std::string Csv =
		"id,seguidores,texto_pt,texto_en,sucata,granadas,bujoes,caixa,hype_bonus\n"
		"cem,100,100 SEGUIDORES,100 FOLLOWERS,20,1,0,0,5\n"
		"cinquenta,50,50,50,10,0,0,0,0\n"
		"mil,1000,MIL,THOUSAND,60,2,2,1,15\n"
		"mil_de_novo,1000,X,X,0,0,0,0,0\n";
	std::vector<ZR::FMilestoneDef> Out;
	std::vector<std::string> Errors;
	ZR::LoadMilestones(Csv, Out, Errors);
	ZR_EQ(Out.size(), static_cast<std::size_t>(2));
	ZR_EQ(Errors.size(), static_cast<std::size_t>(2));
	if (Out.size() == 2)
	{
		ZR_EQ(Out[1].Id, std::string("mil"));
		ZR_EQ(Out[1].Scrap, 60);
		ZR_EQ(Out[1].Bombs, 2);
		ZR_EXPECT(Out[1].bAirdrop);
		ZR_NEAR(Out[1].HypeBonus, 15.0f, 1e-4);
	}
}

ZR_TEST(DerivaMarcos, "Deriva.MarcosCarregamEOPrimeiroEhAlcancavel")
{
	std::string Text;
	std::string TuningText;
	if (!ReadDataFile(T, "Marcos.csv", Text) || !ReadDataFile(T, "Ajustes.csv", TuningText))
	{
		return;
	}
	std::vector<ZR::FMilestoneDef> Defs;
	std::vector<std::string> Errors;
	ZR::LoadMilestones(Text, Defs, Errors);
	ZR::FTuning Tuning;
	std::vector<std::string> Missing;
	ZR::LoadTuning(TuningText, Tuning, Errors, Missing);
	ExpectNoErrors(T, "Marcos.csv", Errors);
	ZR_EXPECT(Defs.size() >= 5);
	// o primeiro marco fica acima dos seguidores do começo (senão nunca paga), e perto: chega nos primeiros minutos
	ZR_EXPECT(!Defs.empty() && Defs[0].Followers > Tuning.Hype.StartFollowers && Defs[0].Followers <= Tuning.Hype.StartFollowers * 3.0f);
	ZR_EXPECT(Tuning.Grade.CViewers < Tuning.Grade.BViewers && Tuning.Grade.BViewers < Tuning.Grade.AViewers
		&& Tuning.Grade.AViewers < Tuning.Grade.SViewers);
}
