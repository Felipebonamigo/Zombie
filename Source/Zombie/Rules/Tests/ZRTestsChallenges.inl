// Testes: pautas da live (desafios de patrocinador com prazo e recompensa).
// Contrato: docs/regras-de-jogo.md, seção 11.

#include "Rules/ZRChallenges.h"
#include "Rules/ZRData.h"

namespace
{
	ZR::FChallengeDef Pauta(const char* Id, int EventIndex, int Target, float Limit, bool bNightOnly = false)
	{
		ZR::FChallengeDef Def;
		Def.Id = Id;
		Def.TextPt = Id;
		Def.EventIndex = EventIndex;
		Def.Target = Target;
		Def.TimeLimitSeconds = Limit;
		Def.Reward = 40.0f;
		Def.HypeBonus = 15.0f;
		Def.bNightOnly = bNightOnly;
		return Def;
	}

	ZR::FChallengeConfig PautaConfig()
	{
		ZR::FChallengeConfig Config;
		Config.FirstDelaySeconds = 10.0f;
		Config.CooldownSeconds = 30.0f;
		Config.MinViewers = 5.0f;
		return Config;
	}

	/** Avança Seconds em passos de 0,1 s e devolve a última mudança que não foi "nada". */
	ZR::FChallengeUpdate TickPautas(ZR::FChallengeTracker& Tracker, const std::vector<ZR::FChallengeDef>& Defs, float Seconds,
		float Viewers, bool bNight = false)
	{
		ZR::FChallengeUpdate Last;
		for (int i = 0; i < static_cast<int>(Seconds * 10.0f + 0.5f); ++i)
		{
			const ZR::FChallengeUpdate Update = Tracker.Tick(Defs, 0.1f, Viewers, bNight);
			if (Update.Result != ZR::EChallengeResult::None)
			{
				Last = Update;
			}
		}
		return Last;
	}
}

ZR_TEST(PautaComecaDepoisDaEspera, "Pauta.ComecaDepoisDaPrimeiraEsperaComPublico")
{
	const std::vector<ZR::FChallengeDef> Defs = {Pauta("a", 3, 3, 60.0f), Pauta("b", 4, 2, 60.0f)};
	ZR::FChallengeTracker Tracker(PautaConfig(), 7);
	ZR_EQ(TickPautas(Tracker, Defs, 9.8f, 10.0f).Result, ZR::EChallengeResult::None);
	ZR_EQ(Tracker.ActiveIndex(), -1);
	const ZR::FChallengeUpdate Started = TickPautas(Tracker, Defs, 0.4f, 10.0f);
	ZR_EQ(Started.Result, ZR::EChallengeResult::Started);
	ZR_EXPECT(Tracker.ActiveIndex() == 0 || Tracker.ActiveIndex() == 1);
	ZR_EQ(Started.Index, Tracker.ActiveIndex());
	ZR_EQ(Tracker.ProgressCount(), 0);
	ZR_NEAR(Tracker.TimeLeft(), 60.0f, 0.5f);
}

ZR_TEST(PautaSemPublicoNaoComeca, "Pauta.SemOMinimoDeEspectadoresNaoComeca")
{
	const std::vector<ZR::FChallengeDef> Defs = {Pauta("a", 3, 3, 60.0f)};
	ZR::FChallengeTracker Tracker(PautaConfig(), 7);
	ZR_EQ(TickPautas(Tracker, Defs, 100.0f, 2.0f).Result, ZR::EChallengeResult::None);
	ZR_EQ(TickPautas(Tracker, Defs, 0.1f, 6.0f).Result, ZR::EChallengeResult::Started);
}

ZR_TEST(PautaSoOEventoCertoConta, "Pauta.SoOEventoDaPautaContaECompletaComRecompensa")
{
	const std::vector<ZR::FChallengeDef> Defs = {Pauta("a", 3, 3, 60.0f)};
	ZR::FChallengeTracker Tracker(PautaConfig(), 7);
	TickPautas(Tracker, Defs, 10.5f, 10.0f);
	ZR_EQ(Tracker.ActiveIndex(), 0);
	ZR_EQ(Tracker.OnEvent(Defs, 2, 5).Result, ZR::EChallengeResult::None);
	ZR_EQ(Tracker.ProgressCount(), 0);
	ZR_EQ(Tracker.OnEvent(Defs, 3, 1).Result, ZR::EChallengeResult::None);
	ZR_EQ(Tracker.ProgressCount(), 1);
	const ZR::FChallengeUpdate Done = Tracker.OnEvent(Defs, 3, 2);
	ZR_EQ(Done.Result, ZR::EChallengeResult::Completed);
	ZR_EQ(Done.Index, 0);
	ZR_NEAR(Done.Reward, 40.0f, 1e-5);
	ZR_NEAR(Done.HypeBonus, 15.0f, 1e-5);
	ZR_EQ(Tracker.ActiveIndex(), -1);
	ZR_NEAR(Tracker.CooldownLeft(), 30.0f, 1e-4);
	// sem pauta ativa, evento não faz nada
	ZR_EQ(Tracker.OnEvent(Defs, 3, 9).Result, ZR::EChallengeResult::None);
}

ZR_TEST(PautaPrazoEstoura, "Pauta.PrazoEstouradoFalhaEEsperaOIntervalo")
{
	const std::vector<ZR::FChallengeDef> Defs = {Pauta("a", 3, 3, 20.0f)};
	ZR::FChallengeTracker Tracker(PautaConfig(), 7);
	TickPautas(Tracker, Defs, 10.5f, 10.0f);
	Tracker.OnEvent(Defs, 3, 2);
	const ZR::FChallengeUpdate Failed = TickPautas(Tracker, Defs, 21.0f, 10.0f);
	ZR_EQ(Failed.Result, ZR::EChallengeResult::Failed);
	ZR_EQ(Failed.Index, 0);
	ZR_EQ(Tracker.ActiveIndex(), -1);
	ZR_EXPECT(Tracker.CooldownLeft() > 28.0f);
	// a próxima só depois do intervalo
	ZR_EQ(TickPautas(Tracker, Defs, 25.0f, 10.0f).Result, ZR::EChallengeResult::None);
	ZR_EQ(TickPautas(Tracker, Defs, 6.0f, 10.0f).Result, ZR::EChallengeResult::Started);
}

ZR_TEST(PautaNaoRepeteSeguida, "Pauta.NaoRepeteAMesmaSeguida")
{
	const std::vector<ZR::FChallengeDef> Defs = {Pauta("a", 3, 1, 60.0f), Pauta("b", 4, 1, 60.0f)};
	ZR::FChallengeConfig Config = PautaConfig();
	Config.CooldownSeconds = 1.0f;
	ZR::FChallengeTracker Tracker(Config, 11);
	TickPautas(Tracker, Defs, 10.5f, 10.0f);
	int Previous = Tracker.ActiveIndex();
	bool bAlternates = Previous >= 0;
	for (int Round = 0; Round < 12 && bAlternates; ++Round)
	{
		const int Event = Defs[static_cast<std::size_t>(Previous)].EventIndex;
		bAlternates = Tracker.OnEvent(Defs, Event, 1).Result == ZR::EChallengeResult::Completed;
		TickPautas(Tracker, Defs, 1.5f, 10.0f);
		const int Next = Tracker.ActiveIndex();
		bAlternates = bAlternates && Next >= 0 && Next != Previous;
		Previous = Next;
	}
	ZR_EXPECT(bAlternates);
}

ZR_TEST(PautaSoDeNoite, "Pauta.PautaDaNoiteSoComecaDeNoite")
{
	const std::vector<ZR::FChallengeDef> Defs = {Pauta("noite", 3, 3, 60.0f, true)};
	ZR::FChallengeTracker Tracker(PautaConfig(), 7);
	ZR_EQ(TickPautas(Tracker, Defs, 40.0f, 10.0f, false).Result, ZR::EChallengeResult::None);
	ZR_EQ(TickPautas(Tracker, Defs, 6.0f, 10.0f, true).Result, ZR::EChallengeResult::Started);
}

ZR_TEST(PautaEventoDesconhecido, "Dados.PautaComEventoDesconhecidoEhErro")
{
	std::vector<ZR::FChallengeDef> Defs;
	std::vector<std::string> Errors;
	ZR::LoadChallenges("id,texto_pt,texto_en,evento,quantidade,prazo_s,recompensa_reais,hype_bonus,peso,so_de_noite\n"
		"x,X,X,nao_existe,3,60,10,5,1,0\n"
		"y,Y,Y,atropelou,0,60,10,5,1,0\n"
		"z,Z,Z,atropelou,4,60,10,5,1,0\n",
		Defs, Errors);
	ZR_EQ(static_cast<int>(Errors.size()), 2);
	ZR_EQ(static_cast<int>(Defs.size()), 1);
	ZR_EXPECT(!Defs.empty() && Defs[0].EventIndex == static_cast<int>(ZR::EHypeEvent::VehicleKill));
}

ZR_TEST(DerivaPautas, "Deriva.PautasCarregam")
{
	std::string Text;
	if (!ReadDataFile(T, "Pautas.csv", Text))
	{
		return;
	}
	std::vector<ZR::FChallengeDef> Defs;
	std::vector<std::string> Errors;
	ZR::LoadChallenges(Text, Defs, Errors);
	ExpectNoErrors(T, "Pautas.csv", Errors);
	ZR_EXPECT(Defs.size() >= 6);
	bool bSomeDay = false;
	for (const ZR::FChallengeDef& Def : Defs)
	{
		ZR_EXPECT(Def.Target >= 1 && Def.TimeLimitSeconds >= 10.0f && Def.Reward > 0.0f);
		bSomeDay = bSomeDay || !Def.bNightOnly;
	}
	ZR_EXPECT(bSomeDay);
}
