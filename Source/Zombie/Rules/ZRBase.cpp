#include "Rules/ZRBase.h"

#include <algorithm>
#include <cmath>

namespace ZR
{
	namespace
	{
		/** Encostar não é sobrepor: a sobreposição precisa passar disso [cm]. */
		constexpr float TouchSlackCm = 0.01f;
		/** Folga do arredondamento da sucata (12 x 0,25 em float dá 3,0000001, que não pode virar 4). */
		constexpr float ScrapRoundSlack = 1.0e-3f;

		/** Distância do ponto até o retângulo (0 dentro dele). */
		float DistanceToFootprint(const FFootprint& Piece, float X, float Y)
		{
			const float Dx = std::max(std::fabs(X - Piece.X) - Piece.HalfX, 0.0f);
			const float Dy = std::max(std::fabs(Y - Piece.Y) - Piece.HalfY, 0.0f);
			return std::sqrt(Dx * Dx + Dy * Dy);
		}
	}

	FFootprint MakeFootprint(const FBuildPieceType& Type, float X, float Y, int Rot90)
	{
		FFootprint Out;
		Out.X = X;
		Out.Y = Y;
		const bool bTurned = (Rot90 % 2) != 0;
		Out.HalfX = (bTurned ? Type.DepthCm : Type.WidthCm) * 0.5f;
		Out.HalfY = (bTurned ? Type.WidthCm : Type.DepthCm) * 0.5f;
		return Out;
	}

	bool FootprintsOverlap(const FFootprint& A, const FFootprint& B)
	{
		return std::fabs(A.X - B.X) < A.HalfX + B.HalfX - TouchSlackCm && std::fabs(A.Y - B.Y) < A.HalfY + B.HalfY - TouchSlackCm;
	}

	float SnapToGrid(float Value, float Step)
	{
		if (Step <= 0.0f)
		{
			return Value;
		}
		return std::round(Value / Step) * Step;
	}

	EPlaceResult CheckPlacement(const FCityPlan& Plan, float CellSizeCm, const FBaseConfig& Config, const FPlaceQuery& Query,
		const std::vector<FFootprint>& Existing)
	{
		if (Query.Scrap < Query.Cost)
		{
			return EPlaceResult::NoScrap;
		}
		if (Query.PieceCount >= Config.MaxPieces)
		{
			return EPlaceResult::TooMany;
		}
		const FFootprint& Piece = Query.Piece;
		const float ToPlayerX = Piece.X - Query.PlayerX;
		const float ToPlayerY = Piece.Y - Query.PlayerY;
		if (ToPlayerX * ToPlayerX + ToPlayerY * ToPlayerY > Config.BuildRangeCm * Config.BuildRangeCm)
		{
			return EPlaceResult::TooFar;
		}
		if (!Plan.IsWalkableRect(Piece.X - Piece.HalfX, Piece.Y - Piece.HalfY, Piece.X + Piece.HalfX, Piece.Y + Piece.HalfY, CellSizeCm))
		{
			return EPlaceResult::OffStreet;
		}
		if (DistanceToFootprint(Piece, Query.PlayerX, Query.PlayerY) < Config.PlayerClearanceCm)
		{
			return EPlaceResult::OnPlayer;
		}
		for (const FFootprint& Other : Existing)
		{
			if (FootprintsOverlap(Piece, Other))
			{
				return EPlaceResult::Blocked;
			}
		}
		return EPlaceResult::Ok;
	}

	void FPieceState::Reset(const FBuildPieceType& Type)
	{
		Health = Type.Health;
		Wear = 0.0f;
		bBroken = false;
	}

	bool DamagePiece(FPieceState& State, float Amount)
	{
		if (State.bBroken || Amount <= 0.0f)
		{
			return false;
		}
		State.Health -= Amount;
		if (State.Health > 0.0f)
		{
			return false;
		}
		State.Health = 0.0f;
		State.bBroken = true;
		return true;
	}

	float PieceCondition(const FBuildPieceType& Type, const FPieceState& State)
	{
		if (State.bBroken || Type.Health <= 0.0f)
		{
			return 0.0f;
		}
		float Condition = std::clamp(State.Health / Type.Health, 0.0f, 1.0f);
		if (Type.Durability > 0.0f)
		{
			Condition = std::min(Condition, std::clamp(1.0f - State.Wear / Type.Durability, 0.0f, 1.0f));
		}
		return Condition;
	}

	int DamageStage(const FBuildPieceType& Type, const FPieceState& State)
	{
		if (State.bBroken)
		{
			return 3;
		}
		const float Fraction = Type.Health > 0.0f ? State.Health / Type.Health : 0.0f;
		if (Fraction > 2.0f / 3.0f)
		{
			return 0;
		}
		return Fraction > 1.0f / 3.0f ? 1 : 2;
	}

	FRepairResult RepairPiece(const FBuildPieceType& Type, const FPieceState& State, int Scrap, const FBaseConfig& Config)
	{
		FRepairResult Out;
		Out.Health = State.Health;
		const float Missing = Type.Health - State.Health;
		if (State.bBroken || Missing <= 0.0f || Type.Health <= 0.0f)
		{
			return Out;
		}
		const float FullPrice = static_cast<float>(Type.Cost) * std::max(Config.RepairCostFraction, 0.0f);
		if (FullPrice <= 0.0f)
		{
			Out.Health = Type.Health;
			return Out;
		}
		const int Needed = static_cast<int>(std::ceil(FullPrice * Missing / Type.Health - ScrapRoundSlack));
		if (Scrap >= Needed)
		{
			Out.Health = Type.Health;
			Out.ScrapSpent = std::max(Needed, 0);
			return Out;
		}
		if (Scrap <= 0)
		{
			return Out;
		}
		Out.ScrapSpent = Scrap;
		Out.Health = std::min(Type.Health, State.Health + static_cast<float>(Scrap) / FullPrice * Type.Health);
		return Out;
	}

	int DismantleRefund(const FBuildPieceType& Type, const FPieceState& State, const FBaseConfig& Config)
	{
		const float Refund = static_cast<float>(Type.Cost) * std::max(Config.RefundFraction, 0.0f) * PieceCondition(Type, State);
		return static_cast<int>(std::floor(Refund + ScrapRoundSlack));
	}

	FTrapContact TouchTrap(const FBuildPieceType& Type, FPieceState& State, float Dt)
	{
		FTrapContact Out;
		Out.Target = Type.Target;
		if (State.bBroken)
		{
			return Out;
		}
		if (Type.IsExplosive())
		{
			Out.bDetonate = true;
			Out.bBrokeNow = true;
			State.bBroken = true;
			return Out;
		}
		Out.SpeedFactor = Type.SlowFactor;
		float Damage = Type.DamagePerSecond * std::max(Dt, 0.0f);
		if (Type.Durability > 0.0f)
		{
			Damage = std::min(Damage, Type.Durability - State.Wear);
		}
		Out.Damage = std::max(Damage, 0.0f);
		State.Wear += Out.Damage;
		if (Type.Durability > 0.0f && State.Wear >= Type.Durability)
		{
			State.bBroken = true;
			Out.bBrokeNow = true;
		}
		return Out;
	}
}
