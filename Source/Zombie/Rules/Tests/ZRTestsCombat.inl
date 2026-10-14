// Testes: corpo do zumbi (membros, desmembramento, explosão em pedaços) e hype da live.
// Contrato: docs/regras-de-jogo.md, seções 4 e 5.

#include "Rules/ZRBody.h"
#include "Rules/ZRHype.h"

namespace
{
	ZR::FBodyConfig TestBody()
	{
		ZR::FBodyConfig Body;
		Body.Health[static_cast<int>(ZR::ELimb::Head)] = 40.0f;
		Body.Health[static_cast<int>(ZR::ELimb::Torso)] = 150.0f;
		Body.Health[static_cast<int>(ZR::ELimb::ArmL)] = 50.0f;
		Body.Health[static_cast<int>(ZR::ELimb::ArmR)] = 50.0f;
		Body.Health[static_cast<int>(ZR::ELimb::LegL)] = 70.0f;
		Body.Health[static_cast<int>(ZR::ELimb::LegR)] = 70.0f;
		Body.TorsoTransfer = 0.25f;
		Body.GibDamage = 220.0f;
		return Body;
	}
}

ZR_TEST(CorpoTiroNaCabeca, "Corpo.TiroNaCabecaMata")
{
	const ZR::FBodyConfig Body = TestBody();
	ZR::FBodyState State;
	State.Reset(Body);
	const ZR::FHitOutcome Weak = ZR::ApplyHit(Body, State, ZR::ELimb::Head, 30.0f);
	ZR_EXPECT(!Weak.bKilled);
	const ZR::FHitOutcome Kill = ZR::ApplyHit(Body, State, ZR::ELimb::Head, 30.0f);
	ZR_EXPECT(Kill.bKilled);
	ZR_EXPECT(Kill.bHeadshotKill);
	ZR_EXPECT((Kill.SeveredMask & ZR::LimbBit(ZR::ELimb::Head)) != 0);
	ZR_EXPECT(State.bDead);
}

ZR_TEST(CorpoTroncoMata, "Corpo.TroncoZeradoMataSemSoltar")
{
	const ZR::FBodyConfig Body = TestBody();
	ZR::FBodyState State;
	State.Reset(Body);
	const ZR::FHitOutcome Hit = ZR::ApplyHit(Body, State, ZR::ELimb::Torso, 150.0f);
	ZR_EXPECT(Hit.bKilled);
	ZR_EXPECT(!Hit.bHeadshotKill);
	ZR_EQ(Hit.SeveredMask, 0);
}

ZR_TEST(CorpoPernaViraRastejante, "Corpo.PernaArrancadaViraRastejante")
{
	const ZR::FBodyConfig Body = TestBody();
	ZR::FBodyState State;
	State.Reset(Body);
	const ZR::FHitOutcome Hit = ZR::ApplyHit(Body, State, ZR::ELimb::LegL, 80.0f);
	ZR_EXPECT(!Hit.bKilled);
	ZR_EXPECT(Hit.bBecameCrawler);
	ZR_EXPECT(State.IsCrawler());
	// 25% do dano do membro vai para o tronco: 150 - 20 = 130
	ZR_NEAR(State.Health[static_cast<int>(ZR::ELimb::Torso)], 130.0f, 1e-4);
	// a segunda perna não "vira rastejante" de novo
	const ZR::FHitOutcome Second = ZR::ApplyHit(Body, State, ZR::ELimb::LegR, 80.0f);
	ZR_EXPECT(!Second.bBecameCrawler);
}

ZR_TEST(CorpoMembroSoltoIgnoraTiro, "Corpo.MembroJaArrancadoIgnoraTiro")
{
	const ZR::FBodyConfig Body = TestBody();
	ZR::FBodyState State;
	State.Reset(Body);
	ZR::ApplyHit(Body, State, ZR::ELimb::ArmL, 60.0f);
	const float TorsoBefore = State.Health[static_cast<int>(ZR::ELimb::Torso)];
	const ZR::FHitOutcome Again = ZR::ApplyHit(Body, State, ZR::ELimb::ArmL, 60.0f);
	ZR_EQ(Again.SeveredMask, 0);
	ZR_NEAR(State.Health[static_cast<int>(ZR::ELimb::Torso)], TorsoBefore, 1e-4);
	ZR_EQ(State.ArmsLeft(), 1);
}

ZR_TEST(CorpoMortoAindaDesmembraSemCredito, "Corpo.CadaverDesmembraSemContarAbate")
{
	const ZR::FBodyConfig Body = TestBody();
	ZR::FBodyState State;
	State.Reset(Body);
	ZR::ApplyHit(Body, State, ZR::ELimb::Torso, 200.0f);
	const ZR::FHitOutcome Hit = ZR::ApplyHit(Body, State, ZR::ELimb::ArmR, 60.0f);
	ZR_EXPECT(!Hit.bKilled);
	ZR_EXPECT((Hit.SeveredMask & ZR::LimbBit(ZR::ELimb::ArmR)) != 0);
}

ZR_TEST(CorpoExplosaoForteDespedaca, "Corpo.ExplosaoForteDespedaca")
{
	const ZR::FBodyConfig Body = TestBody();
	ZR::FBodyState State;
	State.Reset(Body);
	const ZR::FHitOutcome Blast = ZR::ApplyBlast(Body, State, 230.0f);
	ZR_EXPECT(Blast.bKilled);
	ZR_EXPECT(Blast.bGibbed);
	ZR_EQ(Blast.SeveredMask, ZR::AllLimbsMask);
	ZR_EXPECT(State.bGibbed);
	// depois de despedaçado, nada mais acontece
	const ZR::FHitOutcome After = ZR::ApplyBlast(Body, State, 500.0f);
	ZR_EXPECT(!After.bKilled && !After.bGibbed && After.SeveredMask == 0);
}

ZR_TEST(CorpoExplosaoFracaArrancaMembros, "Corpo.ExplosaoFracaArrancaOsMaisFracos")
{
	ZR::FBodyConfig Body = TestBody();
	for (int i = 0; i < ZR::LimbCount; ++i)
	{
		Body.BlastShare[i] = 1.0f;
	}
	ZR::FBodyState State;
	State.Reset(Body);
	// 60 de dano em todos: cabeça (40) e braços (50) saem; pernas (70) e tronco (150) ficam; a cabeça mata
	const ZR::FHitOutcome Blast = ZR::ApplyBlast(Body, State, 60.0f);
	ZR_EXPECT(Blast.bKilled);
	ZR_EXPECT(!Blast.bGibbed);
	const int Expected = ZR::LimbBit(ZR::ELimb::Head) | ZR::LimbBit(ZR::ELimb::ArmL) | ZR::LimbBit(ZR::ELimb::ArmR);
	ZR_EQ(Blast.SeveredMask, Expected);
}

ZR_TEST(ExplosaoQuedaComADistancia, "Explosao.DanoCaiLinearEntreOsRaios")
{
	ZR::FBlastConfig Blast;
	Blast.InnerRadiusCm = 200.0f;
	Blast.OuterRadiusCm = 600.0f;
	Blast.Damage = 400.0f;
	ZR_NEAR(ZR::BlastDamageAt(Blast, 0.0f), 400.0f, 1e-3);
	ZR_NEAR(ZR::BlastDamageAt(Blast, 200.0f), 400.0f, 1e-3);
	ZR_NEAR(ZR::BlastDamageAt(Blast, 400.0f), 200.0f, 1e-3);
	ZR_NEAR(ZR::BlastDamageAt(Blast, 600.0f), 0.0f, 1e-3);
	ZR_NEAR(ZR::BlastDamageAt(Blast, 900.0f), 0.0f, 1e-3);
	ZR_NEAR(ZR::BlastFalloff(Blast, 500.0f), 0.25f, 1e-4);
	// raios iguais: degrau
	Blast.OuterRadiusCm = 200.0f;
	ZR_NEAR(ZR::BlastDamageAt(Blast, 199.0f), 400.0f, 1e-3);
	ZR_NEAR(ZR::BlastDamageAt(Blast, 201.0f), 0.0f, 1e-3);
}

namespace
{
	ZR::FHypeConfig TestHype()
	{
		ZR::FHypeConfig Config;
		Config.ComboWindowSeconds = 4.0f;
		Config.ComboStep = 0.25f;
		Config.ComboMaxMultiplier = 2.0f;
		Config.VarietyFalloff = 0.5f;
		Config.VarietyMin = 0.2f;
		Config.DecayGraceSeconds = 5.0f;
		Config.DecayPerSecond = 4.0f;
		return Config;
	}

	std::vector<ZR::FDonation> TickFor(const ZR::FHypeConfig& Config, ZR::FHypeState& State, float Seconds, float Step = 0.1f)
	{
		std::vector<ZR::FDonation> All;
		for (float Elapsed = 0.0f; Elapsed < Seconds - 1e-4f; Elapsed += Step)
		{
			ZR::TickHype(Config, State, Step, All);
		}
		return All;
	}
}

ZR_TEST(HypeComboMultiplica, "Hype.ComboMultiplicaEPara")
{
	const ZR::FHypeConfig Config = TestHype();
	ZR::FHypeState State;
	State.Reset(Config);
	const ZR::FHypeGain First = ZR::AddHypeEvent(Config, State, 0, 2.0f);
	ZR_EQ(First.Combo, 1);
	ZR_NEAR(First.Multiplier, 1.0f, 1e-5);
	const ZR::FHypeGain Second = ZR::AddHypeEvent(Config, State, 1, 2.0f);
	ZR_EQ(Second.Combo, 2);
	ZR_NEAR(Second.Multiplier, 1.25f, 1e-5);
	ZR::AddHypeEvent(Config, State, 2, 2.0f);
	ZR::AddHypeEvent(Config, State, 3, 2.0f);
	ZR::AddHypeEvent(Config, State, 4, 2.0f);
	const ZR::FHypeGain Sixth = ZR::AddHypeEvent(Config, State, 5, 2.0f);
	ZR_NEAR(Sixth.Multiplier, 2.0f, 1e-5);
	// passada a janela, o combo zera
	TickFor(Config, State, 4.5f);
	ZR_EQ(State.Combo, 0);
	const ZR::FHypeGain Fresh = ZR::AddHypeEvent(Config, State, 0, 2.0f);
	ZR_EQ(Fresh.Combo, 1);
}

ZR_TEST(HypeVariedadeCansa, "Hype.RepetirOMesmoEventoRendeMenos")
{
	ZR::FHypeConfig Config = TestHype();
	Config.ComboMaxMultiplier = 1.0f;
	ZR::FHypeState State;
	State.Reset(Config);
	const ZR::FHypeGain A = ZR::AddHypeEvent(Config, State, 3, 4.0f);
	const ZR::FHypeGain B = ZR::AddHypeEvent(Config, State, 3, 4.0f);
	const ZR::FHypeGain C = ZR::AddHypeEvent(Config, State, 3, 4.0f);
	const ZR::FHypeGain D = ZR::AddHypeEvent(Config, State, 3, 4.0f);
	ZR_NEAR(A.Points, 4.0f, 1e-4);
	ZR_NEAR(B.Points, 2.0f, 1e-4);
	ZR_NEAR(C.Points, 1.0f, 1e-4);
	ZR_NEAR(D.Points, 0.8f, 1e-4); // mínimo 0,2
	const ZR::FHypeGain Other = ZR::AddHypeEvent(Config, State, 4, 4.0f);
	ZR_NEAR(Other.Variety, 1.0f, 1e-5);
}

ZR_TEST(HypeQuantidadeEMaximo, "Hype.QuantidadeSomaETetoEm100")
{
	ZR::FHypeConfig Config = TestHype();
	Config.ComboMaxMultiplier = 1.0f;
	ZR::FHypeState State;
	State.Reset(Config);
	const ZR::FHypeGain Gain = ZR::AddHypeEvent(Config, State, 0, 3.0f, 4);
	ZR_NEAR(Gain.Points, 12.0f, 1e-4);
	ZR_NEAR(State.Hype, 12.0f, 1e-4);
	ZR::AddHypeEvent(Config, State, 1, 500.0f);
	ZR_NEAR(State.Hype, 100.0f, 1e-4);
}

ZR_TEST(HypeCaiDepoisDaCarencia, "Hype.CaiSoDepoisDaCarencia")
{
	const ZR::FHypeConfig Config = TestHype();
	ZR::FHypeState State;
	State.Reset(Config);
	ZR::AddHypeEvent(Config, State, 0, 50.0f);
	TickFor(Config, State, 5.0f);
	ZR_NEAR(State.Hype, 50.0f, 1e-3);
	TickFor(Config, State, 2.0f);
	ZR_NEAR(State.Hype, 42.0f, 0.05);
	TickFor(Config, State, 60.0f);
	ZR_NEAR(State.Hype, 0.0f, 1e-4);
}

ZR_TEST(HypeEspectadoresSeguemOAlvo, "Hype.EspectadoresSeguemOAlvo")
{
	ZR::FHypeConfig Config = TestHype();
	Config.DecayGraceSeconds = 1000.0f;
	Config.FollowRatePerSecond = 0.0f;
	ZR::FHypeState State;
	State.Reset(Config);
	State.Followers = 1000.0f;
	ZR::AddHypeEvent(Config, State, 0, 100.0f);
	const float Target = ZR::TargetViewers(Config, 100.0f, 1000.0f);
	TickFor(Config, State, 30.0f);
	ZR_NEAR(State.Viewers, Target, Target * 0.01);
	// alvo cresce com o hype; no máximo passa dos seguidores (a live viraliza)
	ZR_EXPECT(ZR::TargetViewers(Config, 20.0f, 1000.0f) < ZR::TargetViewers(Config, 60.0f, 1000.0f));
	ZR_EXPECT(Target > 1000.0f);
	ZR_EXPECT(ZR::TargetViewers(Config, 0.0f, 1000.0f) > 0.0f);
}

ZR_TEST(HypeSeguidoresCrescemComTeto, "Hype.SeguidoresCrescemAteOTeto")
{
	ZR::FHypeConfig Config = TestHype();
	Config.DecayGraceSeconds = 1.0e6f;
	Config.FollowRatePerSecond = 0.05f;
	Config.FollowerCap = 5000.0f;
	ZR::FHypeState State;
	State.Reset(Config);
	ZR::AddHypeEvent(Config, State, 0, 80.0f);
	const float Before = State.Followers;
	TickFor(Config, State, 10.0f);
	ZR_EXPECT(State.Followers > Before);
	TickFor(Config, State, 3000.0f, 1.0f);
	ZR_EXPECT(State.Followers <= 5000.0f);
	ZR_EXPECT(State.Followers > 4500.0f);
}

ZR_TEST(HypeDoacaoUmaVezPorSubida, "Hype.DoacaoUmaVezPorSubidaERearma")
{
	ZR::FHypeConfig Config = TestHype();
	Config.DonationThresholds = {50.0f, 90.0f};
	Config.DonationRearm = 20.0f;
	ZR::FHypeState State;
	State.Reset(Config);
	State.Viewers = 1000.0f;
	State.Followers = 1000.0f;
	ZR::AddHypeEvent(Config, State, 0, 55.0f);
	std::vector<ZR::FDonation> Donations = TickFor(Config, State, 0.2f);
	ZR_EQ(static_cast<int>(Donations.size()), 1);
	if (!Donations.empty())
	{
		ZR_EQ(Donations[0].ThresholdIndex, 0);
		ZR_EXPECT(Donations[0].Amount >= 1.0f);
	}
	// continua acima: não doa de novo
	Donations = TickFor(Config, State, 2.0f);
	ZR_EQ(static_cast<int>(Donations.size()), 0);
	// cai abaixo de 50 - 20 = 30 e sobe de novo: doa outra vez
	State.Hype = 25.0f;
	TickFor(Config, State, 0.2f);
	ZR::AddHypeEvent(Config, State, 1, 30.0f);
	Donations = TickFor(Config, State, 0.2f);
	ZR_EQ(static_cast<int>(Donations.size()), 1);
	ZR_EXPECT(State.TotalDonations > 0.0);
}
