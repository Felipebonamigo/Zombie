// Testes: Diretor da horda (ritmo à Left 4 Dead), dia e noite e momentos para clipe.
// Contrato: docs/regras-de-jogo.md, seções 6 a 8.

#include "Rules/ZRDirector.h"
#include "Rules/ZRDayNight.h"
#include "Rules/ZRHighlights.h"

namespace
{
	ZR::FDirectorConfig TestDirector()
	{
		ZR::FDirectorConfig Config;
		Config.PeakIntensity = 70.0f;
		Config.RelaxIntensity = 25.0f;
		Config.MaxBuildUpSeconds = 60.0f;
		Config.PeakSeconds = 20.0f;
		Config.RelaxSeconds = 15.0f;
		Config.IntensityDecayPerSecond = 2.0f;
		Config.DamageToIntensity = 1.0f;
		Config.BuildUpSpawnPerSecond = 0.5f;
		Config.PeakSpawnPerSecond = 2.0f;
		Config.RelaxSpawnPerSecond = 0.0f;
		Config.NightMultiplier = 2.0f;
		Config.HordeMultiplier = 3.0f;
		Config.MaxAlive = 1000;
		Config.MaxAliveNight = 1000;
		Config.NoiseToSpawn = 0.0f;
		return Config;
	}

	ZR::FDirectorInput Quiet(float Dt, int Alive = 0, bool bNight = false)
	{
		ZR::FDirectorInput Input;
		Input.Dt = Dt;
		Input.Alive = Alive;
		Input.bNight = bNight;
		return Input;
	}

	int RunDirector(const ZR::FDirectorConfig& Config, ZR::FDirectorState& State, float Seconds, bool bNight = false)
	{
		int Spawned = 0;
		for (int i = 0; i < static_cast<int>(Seconds * 10.0f + 0.5f); ++i)
		{
			Spawned += ZR::TickDirector(Config, State, Quiet(0.1f, 0, bNight)).SpawnNow;
		}
		return Spawned;
	}
}

ZR_TEST(DiretorAcumuladorExato, "Diretor.AcumuladorDaContaCerta")
{
	const ZR::FDirectorConfig Config = TestDirector();
	ZR::FDirectorState State;
	ZR_EQ(RunDirector(Config, State, 10.0f), 5);
	ZR_EQ(State.Phase, ZR::EDirectorPhase::BuildUp);
}

ZR_TEST(DiretorPicoPorIntensidade, "Diretor.IntensidadeAltaAbreOPico")
{
	const ZR::FDirectorConfig Config = TestDirector();
	ZR::FDirectorState State;
	ZR::FDirectorInput Hurt = Quiet(0.1f);
	Hurt.DamageTaken = 75.0f;
	const ZR::FDirectorOutput Out = ZR::TickDirector(Config, State, Hurt);
	ZR_EXPECT(Out.bPhaseChanged);
	ZR_EQ(State.Phase, ZR::EDirectorPhase::Peak);
	ZR_EQ(RunDirector(Config, State, 5.0f), 10);
}

ZR_TEST(DiretorPicoPorTempo, "Diretor.PicoChegaPeloTempoMaximo")
{
	const ZR::FDirectorConfig Config = TestDirector();
	ZR::FDirectorState State;
	RunDirector(Config, State, 59.5f);
	ZR_EQ(State.Phase, ZR::EDirectorPhase::BuildUp);
	RunDirector(Config, State, 1.0f);
	ZR_EQ(State.Phase, ZR::EDirectorPhase::Peak);
}

ZR_TEST(DiretorRelaxaEVolta, "Diretor.PicoRelaxaSemSpawnEVolta")
{
	const ZR::FDirectorConfig Config = TestDirector();
	ZR::FDirectorState State;
	State.Phase = ZR::EDirectorPhase::Peak;
	RunDirector(Config, State, 20.5f);
	ZR_EQ(State.Phase, ZR::EDirectorPhase::Relax);
	ZR_EQ(RunDirector(Config, State, 10.0f), 0);
	// intensidade alta segura o descanso até baixar de 25
	State.Intensity = 60.0f;
	State.PhaseTime = 14.0f;
	RunDirector(Config, State, 2.0f);
	ZR_EQ(State.Phase, ZR::EDirectorPhase::Relax);
	RunDirector(Config, State, 20.0f);
	ZR_EQ(State.Phase, ZR::EDirectorPhase::BuildUp);
}

ZR_TEST(DiretorNoiteEHorda, "Diretor.NoiteEHordaMultiplicam")
{
	const ZR::FDirectorConfig Config = TestDirector();
	ZR::FDirectorState Night;
	ZR_EQ(RunDirector(Config, Night, 10.0f, true), 10);
	ZR::FDirectorState Horde;
	ZR::StartHorde(Horde, 10.0f);
	// horda: ritmo do pico (2/s) x 3 = 6/s
	ZR_EQ(RunDirector(Config, Horde, 5.0f), 30);
	const ZR::FDirectorOutput Out = ZR::TickDirector(Config, Horde, Quiet(0.1f));
	ZR_EXPECT(Out.bHordeActive);
	RunDirector(Config, Horde, 6.0f);
	ZR_EXPECT(!ZR::TickDirector(Config, Horde, Quiet(0.1f)).bHordeActive);
}

ZR_TEST(DiretorRespeitaOTeto, "Diretor.NuncaPassaDoTetoDeVivos")
{
	ZR::FDirectorConfig Config = TestDirector();
	Config.MaxAlive = 12;
	Config.MaxAliveNight = 20;
	ZR::FDirectorState State;
	ZR::StartHorde(State, 100.0f);
	int Alive = 10;
	int MaxSeen = 0;
	for (int i = 0; i < 100; ++i)
	{
		Alive += ZR::TickDirector(Config, State, Quiet(0.1f, Alive)).SpawnNow;
		MaxSeen = Alive > MaxSeen ? Alive : MaxSeen;
	}
	ZR_EQ(MaxSeen, 12);
	for (int i = 0; i < 100; ++i)
	{
		Alive += ZR::TickDirector(Config, State, Quiet(0.1f, Alive, true)).SpawnNow;
	}
	ZR_EQ(Alive, 20);
}

ZR_TEST(DiretorBarulhoChamaMais, "Diretor.BarulhoChamaZumbiEPassa")
{
	ZR::FDirectorConfig Config = TestDirector();
	Config.BuildUpSpawnPerSecond = 0.0f;
	Config.NoiseToSpawn = 0.1f;
	Config.NoiseDecayPerSecond = 10.0f;
	ZR::FDirectorState State;
	ZR::FDirectorInput Loud = Quiet(0.1f);
	Loud.NoiseAdded = 50.0f;
	int Spawned = ZR::TickDirector(Config, State, Loud).SpawnNow;
	Spawned += RunDirector(Config, State, 10.0f);
	ZR_EXPECT(Spawned >= 10 && Spawned <= 14);
	ZR_NEAR(State.Noise, 0.0f, 1e-4);
}

ZR_TEST(DiaHoraAvancaEDaAVolta, "DiaNoite.HoraAvancaEDaAVolta")
{
	ZR::FDayConfig Day;
	Day.DayLengthSeconds = 1200.0f;
	Day.StartHour = 18.0f;
	ZR_NEAR(ZR::HourAt(Day, 0.0), 18.0, 1e-6);
	ZR_NEAR(ZR::HourAt(Day, 300.0), 0.0, 1e-6);
	ZR_NEAR(ZR::HourAt(Day, 350.0), 1.0, 1e-6);
	ZR_EQ(ZR::DayIndexAt(Day, 0.0), 0);
	ZR_EQ(ZR::DayIndexAt(Day, 300.0), 1);
}

ZR_TEST(DiaNoiteJanela, "DiaNoite.NoiteAtravessaAMeiaNoite")
{
	ZR::FDayConfig Day;
	Day.NightStartHour = 19.5f;
	Day.NightEndHour = 5.5f;
	ZR_EXPECT(ZR::IsNight(Day, 22.0));
	ZR_EXPECT(ZR::IsNight(Day, 2.0));
	ZR_EXPECT(!ZR::IsNight(Day, 12.0));
	ZR_EXPECT(!ZR::IsNight(Day, 19.0));
	ZR_EXPECT(ZR::IsNight(Day, 19.5));
	ZR_EXPECT(!ZR::IsNight(Day, 5.5));
}

ZR_TEST(DiaSolNoAltoAoMeioDia, "DiaNoite.SolNoAltoAoMeioDiaEAbaixoDeNoite")
{
	ZR_NEAR(ZR::SunElevationDegrees(12.0, 70.0f), 70.0, 1e-6);
	ZR_NEAR(ZR::SunElevationDegrees(6.0, 70.0f), 0.0, 1e-6);
	ZR_NEAR(ZR::SunElevationDegrees(18.0, 70.0f), 0.0, 1e-6);
	ZR_EXPECT(ZR::SunElevationDegrees(0.0, 70.0f) < -60.0);
}

ZR_TEST(DiaNoiteDaHorda, "DiaNoite.NoiteDaHordaACadaN")
{
	ZR::FDayConfig Day;
	Day.HordeEveryNights = 3;
	ZR_EXPECT(!ZR::IsHordeNight(Day, 0));
	ZR_EXPECT(!ZR::IsHordeNight(Day, 1));
	ZR_EXPECT(ZR::IsHordeNight(Day, 2));
	ZR_EXPECT(ZR::IsHordeNight(Day, 5));
	Day.HordeEveryNights = 0;
	ZR_EXPECT(!ZR::IsHordeNight(Day, 2));
}

ZR_TEST(ClipeMelhorJanela, "Clipe.MelhorJanelaPegaOPicoERotulo")
{
	ZR::FHighlightTracker Tracker(60.0);
	Tracker.Add(1.0, 2.0f, "abate");
	Tracker.Add(20.0, 5.0f, "tiro na cabeca");
	Tracker.Add(22.0, 30.0f, "explosao em cadeia");
	Tracker.Add(24.0, 6.0f, "despedacou");
	Tracker.Add(50.0, 8.0f, "atropelou");
	const ZR::FHighlightTracker::FWindow Best = Tracker.BestWindow(55.0, 10.0);
	ZR_EXPECT(Best.bValid);
	ZR_NEAR(Best.Score, 41.0f, 1e-4);
	ZR_EXPECT(Best.Start <= 20.0 && Best.End >= 24.0);
	ZR_EXPECT(Best.End - Best.Start <= 10.0 + 1e-9);
	ZR_EXPECT(Best.Label == "explosao em cadeia");
}

ZR_TEST(ClipeEsqueceOAntigo, "Clipe.EsqueceMomentosVelhos")
{
	ZR::FHighlightTracker Tracker(30.0);
	Tracker.Add(1.0, 100.0f, "velho");
	Tracker.Add(40.0, 3.0f, "novo");
	Tracker.Prune(45.0);
	ZR_EQ(static_cast<int>(Tracker.Count()), 1);
	const ZR::FHighlightTracker::FWindow Best = Tracker.BestWindow(45.0, 15.0);
	ZR_EXPECT(Best.Label == "novo");
	ZR::FHighlightTracker Empty(30.0);
	ZR_EXPECT(!Empty.BestWindow(10.0, 15.0).bValid);
}
