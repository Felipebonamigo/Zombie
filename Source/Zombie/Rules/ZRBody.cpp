#include "Rules/ZRBody.h"

namespace ZR
{
	namespace
	{
		bool IsLeg(int Limb)
		{
			return Limb == static_cast<int>(ELimb::LegL) || Limb == static_cast<int>(ELimb::LegR);
		}

		/** Aplica dano a um membro ainda preso; marca o que saiu e se morreu. Não mexe em bKilled (quem chama decide). */
		void DamageLimb(FBodyState& State, int Limb, float Damage, FHitOutcome& Out)
		{
			if (State.Severed[Limb] || Damage <= 0.0f)
			{
				return;
			}
			State.Health[Limb] -= Damage;
			if (State.Health[Limb] > 0.0f || Limb == static_cast<int>(ELimb::Torso))
			{
				return;
			}
			const bool bWasCrawler = State.IsCrawler();
			State.Severed[Limb] = true;
			Out.SeveredMask |= 1 << Limb;
			if (IsLeg(Limb) && !bWasCrawler)
			{
				Out.bBecameCrawler = true;
			}
		}

		bool ShouldDie(const FBodyState& State)
		{
			return State.Severed[static_cast<int>(ELimb::Head)] || State.Health[static_cast<int>(ELimb::Torso)] <= 0.0f;
		}
	}

	void FBodyState::Reset(const FBodyConfig& Config)
	{
		for (int i = 0; i < LimbCount; ++i)
		{
			Health[i] = Config.Health[i];
			Severed[i] = false;
		}
		bDead = false;
		bGibbed = false;
	}

	bool FBodyState::IsCrawler() const
	{
		return Severed[static_cast<int>(ELimb::LegL)] || Severed[static_cast<int>(ELimb::LegR)];
	}

	int FBodyState::ArmsLeft() const
	{
		return (Severed[static_cast<int>(ELimb::ArmL)] ? 0 : 1) + (Severed[static_cast<int>(ELimb::ArmR)] ? 0 : 1);
	}

	ELimb LimbAtPoint(float /*X*/, float Y, float Z)
	{
		constexpr float HeadMinZ = 56.0f;
		constexpr float TorsoMinZ = -6.0f;
		constexpr float ArmMinZ = 30.0f;
		constexpr float ArmMaxZ = 50.0f;
		constexpr float ArmMinSide = 21.0f;
		if (Z > HeadMinZ)
		{
			return ELimb::Head;
		}
		if (Z < TorsoMinZ)
		{
			return Y < 0.0f ? ELimb::LegL : ELimb::LegR;
		}
		const float Side = Y < 0.0f ? -Y : Y;
		if (Z >= ArmMinZ && Z <= ArmMaxZ && Side >= ArmMinSide)
		{
			return Y < 0.0f ? ELimb::ArmL : ELimb::ArmR;
		}
		return ELimb::Torso;
	}

	FHitOutcome ApplyHit(const FBodyConfig& Config, FBodyState& State, ELimb Limb, float Damage)
	{
		FHitOutcome Out;
		const int Index = static_cast<int>(Limb);
		if (State.bGibbed || State.Severed[Index] || Damage <= 0.0f)
		{
			return Out;
		}
		DamageLimb(State, Index, Damage, Out);
		if (Limb != ELimb::Torso)
		{
			DamageLimb(State, static_cast<int>(ELimb::Torso), Damage * Config.TorsoTransfer, Out);
		}
		if (!State.bDead && ShouldDie(State))
		{
			State.bDead = true;
			Out.bKilled = true;
			Out.bHeadshotKill = (Out.SeveredMask & LimbBit(ELimb::Head)) != 0;
		}
		return Out;
	}

	FHitOutcome ApplyBlast(const FBodyConfig& Config, FBodyState& State, float Damage)
	{
		FHitOutcome Out;
		if (State.bGibbed || Damage <= 0.0f)
		{
			return Out;
		}
		if (Damage >= Config.GibDamage)
		{
			for (int i = 0; i < LimbCount; ++i)
			{
				State.Severed[i] = true;
				State.Health[i] = 0.0f;
			}
			Out.SeveredMask = AllLimbsMask;
			Out.bGibbed = true;
			State.bGibbed = true;
			Out.bKilled = !State.bDead;
			State.bDead = true;
			return Out;
		}
		for (int i = 0; i < LimbCount; ++i)
		{
			DamageLimb(State, i, Damage * Config.BlastShare[i], Out);
		}
		if (!State.bDead && ShouldDie(State))
		{
			State.bDead = true;
			Out.bKilled = true;
		}
		return Out;
	}

	float BlastFalloff(const FBlastConfig& Blast, float DistanceCm)
	{
		if (DistanceCm <= Blast.InnerRadiusCm)
		{
			return 1.0f;
		}
		if (DistanceCm >= Blast.OuterRadiusCm)
		{
			return 0.0f;
		}
		return (Blast.OuterRadiusCm - DistanceCm) / (Blast.OuterRadiusCm - Blast.InnerRadiusCm);
	}

	float BlastDamageAt(const FBlastConfig& Blast, float DistanceCm)
	{
		return Blast.Damage * BlastFalloff(Blast, DistanceCm);
	}
}
