// Testes: ferimentos que sangram. Contrato: docs/regras-de-jogo.md, seção 17.

#include "Rules/ZRWound.h"

ZR_TEST(FerimentoAbre, "Ferimento.MordidaForteComSorteAbre")
{
	ZR::FWoundConfig Config;
	Config.MinDamage = 8.0f;
	Config.Chance = 0.35f;
	Config.Seconds = 45.0f;
	ZR::FWoundState State;
	// fraca não abre, nem com sorte
	ZR_EXPECT(!ZR::TryOpenWound(Config, State, 7.9f, 0.0));
	// forte abre se o sorteio passar da chance
	ZR_EXPECT(!ZR::TryOpenWound(Config, State, 20.0f, 0.36));
	ZR_EXPECT(ZR::TryOpenWound(Config, State, 20.0f, 0.34));
	ZR_EQ(State.Count(), 1);
	ZR_EXPECT(State.Count() == 1 && State.Left[0] > Config.Seconds - 1e-3f && State.Left[0] < Config.Seconds + 1e-3f);
}

ZR_TEST(FerimentoTeto, "Ferimento.NoTetoAMordidaRenovaOQueFechaPrimeiro")
{
	ZR::FWoundConfig Config;
	Config.MaxWounds = 2;
	Config.Seconds = 10.0f;
	ZR::FWoundState State;
	ZR_EXPECT(ZR::TryOpenWound(Config, State, 20.0f, 0.0));
	ZR::TickWounds(Config, State, 4.0f);
	ZR_EXPECT(ZR::TryOpenWound(Config, State, 20.0f, 0.0));
	ZR::TickWounds(Config, State, 1.0f);
	// os dois abertos (5 s e 9 s): a terceira não abre outro, renova o de 5 s
	ZR_EXPECT(ZR::TryOpenWound(Config, State, 20.0f, 0.0));
	ZR_EQ(State.Count(), 2);
	float Shortest = 1.0e9f;
	for (const float Left : State.Left)
	{
		Shortest = Left < Shortest ? Left : Shortest;
	}
	ZR_NEAR(Shortest, 9.0f, 1e-5);
}

ZR_TEST(FerimentoSangra, "Ferimento.SangraPorFerimentoEFechaSozinho")
{
	ZR::FWoundConfig Config;
	Config.DamagePerSecond = 0.5f;
	Config.Seconds = 3.0f;
	ZR::FWoundState State;
	ZR::TryOpenWound(Config, State, 20.0f, 0.0);
	ZR::TryOpenWound(Config, State, 20.0f, 0.0);
	// dois ferimentos: 1 de vida por segundo
	ZR_NEAR(ZR::TickWounds(Config, State, 2.0f), 2.0f, 1e-5);
	ZR_EQ(State.Count(), 2);
	// falta 1 s em cada: um passo de 2 s só conta 1 s de cada e fecha os dois
	ZR_NEAR(ZR::TickWounds(Config, State, 2.0f), 1.0f, 1e-5);
	ZR_EQ(State.Count(), 0);
	ZR_NEAR(ZR::TickWounds(Config, State, 5.0f), 0.0f, 1e-9);
	// tempo negativo não cura nem abre nada
	ZR::TryOpenWound(Config, State, 20.0f, 0.0);
	ZR_NEAR(ZR::TickWounds(Config, State, -1.0f), 0.0f, 1e-9);
	ZR_EQ(State.Count(), 1);
}

ZR_TEST(FerimentoCurativo, "Ferimento.CurativoEstancaTodos")
{
	const ZR::FWoundConfig Config;
	ZR::FWoundState State;
	ZR::TryOpenWound(Config, State, 20.0f, 0.0);
	ZR::TryOpenWound(Config, State, 20.0f, 0.0);
	ZR::StopBleeding(State);
	ZR_EQ(State.Count(), 0);
	ZR_NEAR(ZR::TickWounds(Config, State, 10.0f), 0.0f, 1e-9);
}
