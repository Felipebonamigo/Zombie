#include "Rules/ZRLoot.h"

#include <algorithm>
#include <cmath>

namespace ZR
{
	namespace
	{
		struct FLootItemName
		{
			ELootItem Item;
			const char* Id;
		};

		constexpr FLootItemName LootItemNames[] = {
			{ELootItem::Nothing, "nada"},
			{ELootItem::Ammo, "municao"},
			{ELootItem::Grenade, "granada"},
			{ELootItem::Bomb, "bujao"},
			{ELootItem::Health, "curativo"},
			{ELootItem::Scrap, "sucata"},
		};

		/** Um lugar possível: a célula e, na calçada, o lado da rua (0 leste, 1 sul, 2 oeste, 3 norte). */
		struct FCandidate
		{
			int X = 0;
			int Y = 0;
			int Side = 0;
		};

		constexpr int SideDx[4] = {1, 0, -1, 0};
		constexpr int SideDy[4] = {0, 1, 0, -1};
	}

	const char* LootItemId(ELootItem Item)
	{
		for (const FLootItemName& Name : LootItemNames)
		{
			if (Name.Item == Item)
			{
				return Name.Id;
			}
		}
		return "nada";
	}

	bool ParseLootItem(const std::string& Id, ELootItem& Out)
	{
		for (const FLootItemName& Name : LootItemNames)
		{
			if (Id == Name.Id)
			{
				Out = Name.Item;
				return true;
			}
		}
		return false;
	}

	FLootResult RollLoot(const std::vector<FLootItemDef>& Items, const FLootSpotType& Type, FRng& Rng)
	{
		FLootResult Out;
		double Total = 0.0;
		for (const FLootItemDef& Def : Items)
		{
			Total += Def.Spot == Type.Id && Def.Weight > 0.0f ? static_cast<double>(Def.Weight) : 0.0;
		}
		if (Total <= 0.0)
		{
			return Out;
		}
		for (int Roll = 0; Roll < Type.Rolls; ++Roll)
		{
			// a roleta: cada linha ocupa uma fatia do tamanho do seu peso
			double Pick = Rng.Unit() * Total;
			const FLootItemDef* Chosen = nullptr;
			for (const FLootItemDef& Def : Items)
			{
				if (Def.Spot != Type.Id || Def.Weight <= 0.0f)
				{
					continue;
				}
				Chosen = &Def;
				Pick -= static_cast<double>(Def.Weight);
				if (Pick < 0.0)
				{
					break;
				}
			}
			if (!Chosen)
			{
				continue;
			}
			const int Amount = Rng.RangeInt(Chosen->Min, Chosen->Max);
			switch (Chosen->Item)
			{
			case ELootItem::Ammo:
				Out.AmmoPacks += Amount;
				break;
			case ELootItem::Grenade:
				Out.Grenades += Amount;
				break;
			case ELootItem::Bomb:
				Out.Bombs += Amount;
				break;
			case ELootItem::Health:
				Out.Health += static_cast<float>(Amount);
				break;
			case ELootItem::Scrap:
				Out.Scrap += Amount;
				break;
			case ELootItem::Nothing:
				break;
			}
		}
		return Out;
	}

	bool CanSearch(const FLootSpotType& Type, const FLootSpotState& State, int Day)
	{
		return State.SearchedDay < 0 || (Type.RefillDays > 0 && Day >= State.SearchedDay + Type.RefillDays);
	}

	std::vector<FLootPlace> PlaceLoot(const FCityPlan& Plan, float CellSizeCm, const std::vector<FLootSpotType>& Types,
		const FLootConfig& Config, std::uint64_t Seed, float StartX, float StartY)
	{
		std::vector<FLootPlace> Out;
		if (CellSizeCm <= 0.0f)
		{
			return Out;
		}
		FRng Rng(Seed);
		const float Spacing2 = Config.MinSpacingCm * Config.MinSpacingCm;
		const float Avoid2 = Config.AvoidStartCm * Config.AvoidStartCm;
		for (std::size_t TypeIndex = 0; TypeIndex < Types.size(); ++TypeIndex)
		{
			const FLootSpotType& Type = Types[TypeIndex];
			std::vector<FCandidate> Candidates;
			for (int Y = 0; Y < Plan.Height; ++Y)
			{
				for (int X = 0; X < Plan.Width; ++X)
				{
					const ECell Cell = Plan.At(X, Y);
					if (Type.Where == ELootWhere::Road && Cell == ECell::Road)
					{
						Candidates.push_back({X, Y, 0});
					}
					else if (Type.Where == ELootWhere::Sidewalk && Cell == ECell::Building)
					{
						for (int Side = 0; Side < 4; ++Side)
						{
							if (Plan.At(X + SideDx[Side], Y + SideDy[Side]) == ECell::Road)
							{
								Candidates.push_back({X, Y, Side});
							}
						}
					}
				}
			}
			int Placed = 0;
			for (std::size_t i = 0; i < Candidates.size() && Placed < Type.Count; ++i)
			{
				// Fisher-Yates conforme anda: a ordem dos candidatos é sorteada pela semente
				const int Swap = Rng.RangeInt(static_cast<int>(i), static_cast<int>(Candidates.size()) - 1);
				std::swap(Candidates[i], Candidates[static_cast<std::size_t>(Swap)]);
				const FCandidate& Here = Candidates[i];
				FLootPlace Place;
				Place.Type = static_cast<int>(TypeIndex);
				const float CellLeft = static_cast<float>(Here.X) * CellSizeCm;
				const float CellTop = static_cast<float>(Here.Y) * CellSizeCm;
				const float Along = static_cast<float>(Rng.RangeDouble(0.15, 0.85)) * CellSizeCm;
				int Facing = Here.Side;
				if (Type.Where == ELootWhere::Sidewalk)
				{
					// na faixa entre a rua e a fachada, de frente para a rua; o objeto inteiro cabe nela
					const float Inset = std::clamp(Plan.InsetAt(Here.X, Here.Y), 0.0f, CellSizeCm * 0.5f);
					const float Half = Type.WidthCm * 0.5f + 10.0f;
					if (Inset < Half * 2.0f)
					{
						continue;
					}
					const float In = std::clamp(static_cast<float>(Rng.RangeDouble(0.35, 0.65)) * Inset, Half, Inset - Half);
					switch (Here.Side)
					{
					case 0:
						Place.X = CellLeft + CellSizeCm - In;
						Place.Y = CellTop + Along;
						break;
					case 1:
						Place.X = CellLeft + Along;
						Place.Y = CellTop + CellSizeCm - In;
						break;
					case 2:
						Place.X = CellLeft + In;
						Place.Y = CellTop + Along;
						break;
					default:
						Place.X = CellLeft + Along;
						Place.Y = CellTop + In;
						break;
					}
				}
				else
				{
					// estacionado na rua, no sentido dela, para um dos lados
					const bool bEastWest = Plan.At(Here.X - 1, Here.Y) == ECell::Road || Plan.At(Here.X + 1, Here.Y) == ECell::Road;
					const bool bNorthSouth = Plan.At(Here.X, Here.Y - 1) == ECell::Road || Plan.At(Here.X, Here.Y + 1) == ECell::Road;
					const bool bAlongX = bEastWest && (!bNorthSouth || Rng.Unit() < 0.5);
					const float Across = static_cast<float>(Rng.RangeDouble(0.15, 0.3)) * CellSizeCm * (Rng.Unit() < 0.5 ? -1.0f : 1.0f);
					const float Center = CellSizeCm * 0.5f;
					Place.X = CellLeft + (bAlongX ? Along : Center + Across);
					Place.Y = CellTop + (bAlongX ? Center + Across : Along);
					Facing = (bAlongX ? 0 : 1) + (Rng.Unit() < 0.5 ? 2 : 0);
				}
				Place.Yaw = static_cast<float>(Facing) * 90.0f;
				// largura ao longo de onde ele olha, profundidade de lado
				const bool bFacingX = Facing % 2 == 0;
				const float HalfX = (bFacingX ? Type.WidthCm : Type.DepthCm) * 0.5f;
				const float HalfY = (bFacingX ? Type.DepthCm : Type.WidthCm) * 0.5f;
				if (!Plan.IsWalkablePoint(Place.X, Place.Y, CellSizeCm)
					|| !Plan.IsWalkableRect(Place.X - HalfX, Place.Y - HalfY, Place.X + HalfX, Place.Y + HalfY, CellSizeCm))
				{
					continue;
				}
				const float ToStartX = Place.X - StartX;
				const float ToStartY = Place.Y - StartY;
				if (ToStartX * ToStartX + ToStartY * ToStartY < Avoid2)
				{
					continue;
				}
				const bool bCrowded = std::any_of(Out.begin(), Out.end(), [&Place, Spacing2](const FLootPlace& Other)
				{
					const float Dx = Other.X - Place.X;
					const float Dy = Other.Y - Place.Y;
					return Dx * Dx + Dy * Dy < Spacing2;
				});
				if (bCrowded)
				{
					continue;
				}
				Out.push_back(Place);
				++Placed;
			}
		}
		return Out;
	}
}
