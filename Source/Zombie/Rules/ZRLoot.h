// Saque: os lugares da cidade que dá para revirar (lixeira, mochila de folião, isopor de ambulante, viatura largada), o que
// cada um rende e onde ficam. Regras puras: sem Unreal. Contrato: docs/regras-de-jogo.md, seção 15.
//
//  - Revirar: segurar E perto do lugar por busca_s; o barulho chama a horda. Rende rolagens sorteios na tabela
//    Data/SaqueItens.csv do lugar, pelo peso de cada linha (a linha "nada" também conta); a quantidade sai entre min e max.
//  - Um lugar revirado fica vazio; com renova_dias > 0, volta a render depois desses dias.
//  - Onde: "calcada" na faixa entre a rua e a fachada (de frente para a rua); "rua" numa célula de rua. Nada perto do
//    começo do jogador, nada colado noutro lugar, sempre em chão (IsWalkablePoint).

#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "Rules/ZRCityPlan.h"
#include "Rules/ZRRandom.h"

namespace ZR
{
	enum class ELootWhere : std::uint8_t
	{
		Sidewalk,
		Road,
	};

	enum class ELootItem : std::uint8_t
	{
		Nothing,
		/** Pentes: um pente cheio de cada arma por unidade. */
		Ammo,
		Grenade,
		Bomb,
		/** Vida (curativo, água, energético). */
		Health,
		Scrap,
	};

	/** Os ids de Data/SaqueItens.csv: nada, municao, granada, bujao, curativo, sucata. */
	const char* LootItemId(ELootItem Item);
	bool ParseLootItem(const std::string& Id, ELootItem& Out);

	/** Uma linha de Data/Saque.csv. */
	struct FLootSpotType
	{
		std::string Id;
		std::string NamePt;
		std::string NameEn;
		ELootWhere Where = ELootWhere::Sidewalk;
		/** Segurando E [s]. */
		float SearchSeconds = 2.0f;
		/** Sorteios na tabela de itens do lugar. */
		int Rolls = 1;
		/** Barulho ao terminar (o mesmo do Diretor: chama zumbis). */
		float Noise = 0.0f;
		/** Quantos espalhar pela cidade. */
		int Count = 0;
		/** 0 = revirado fica vazio; N = volta a render N dias depois. */
		int RefillDays = 0;
		float WidthCm = 60.0f;
		float DepthCm = 60.0f;
		float HeightCm = 100.0f;
		std::uint32_t ColorRgb = 0x556655;
	};

	/** Uma linha de Data/SaqueItens.csv. */
	struct FLootItemDef
	{
		std::string Spot;
		ELootItem Item = ELootItem::Nothing;
		float Weight = 1.0f;
		int Min = 1;
		int Max = 1;
	};

	struct FLootResult
	{
		int AmmoPacks = 0;
		int Grenades = 0;
		int Bombs = 0;
		float Health = 0.0f;
		int Scrap = 0;

		bool IsEmpty() const
		{
			return AmmoPacks <= 0 && Grenades <= 0 && Bombs <= 0 && Health <= 0.0f && Scrap <= 0;
		}
	};

	/** Sorteia o que Type rende: Type.Rolls sorteios entre as linhas de Items com Spot == Type.Id. */
	FLootResult RollLoot(const std::vector<FLootItemDef>& Items, const FLootSpotType& Type, FRng& Rng);

	struct FLootSpotState
	{
		/** Dia em que foi revirado; negativo = nunca. */
		int SearchedDay = -1;
	};

	bool CanSearch(const FLootSpotType& Type, const FLootSpotState& State, int Day);

	struct FLootConfig
	{
		/** Até onde o E alcança, da borda do lugar [cm]. */
		float RangeCm = 160.0f;
		/** Distância mínima entre dois lugares [cm]. */
		float MinSpacingCm = 800.0f;
		/** Nada a menos disso do começo do jogador [cm]. */
		float AvoidStartCm = 600.0f;
	};

	/** Um lugar posto na cidade: X e Y no plano [cm, origem no canto da célula (0, 0)]; Yaw em graus (de frente para a rua). */
	struct FLootPlace
	{
		int Type = 0;
		float X = 0.0f;
		float Y = 0.0f;
		float Yaw = 0.0f;
	};

	/** Espalha Types[i].Count lugares de cada tipo (menos, se faltar espaço), sempre igual para a mesma semente. */
	std::vector<FLootPlace> PlaceLoot(const FCityPlan& Plan, float CellSizeCm, const std::vector<FLootSpotType>& Types,
		const FLootConfig& Config, std::uint64_t Seed, float StartX, float StartY);
}
