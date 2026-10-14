// Base do jogador (à Night of the Dead): sucata, peças de construção (barricadas e armadilhas), onde dá para construir,
// dano, conserto, desmonte e o efeito das armadilhas nos zumbis. Regras puras: sem Unreal.
// Contrato: docs/regras-de-jogo.md, seção 12.
//
//  - Coordenadas no plano da cidade [cm], com a origem no canto da célula (0, 0): X para leste, Y para o sul. A célula de um
//    ponto é (floor(X / CellSizeCm), floor(Y / CellSizeCm)), como em AZMBWorldBuilder::WorldToCell.
//  - A peça gira de 90 em 90 graus: a pegada é sempre um retângulo alinhado aos eixos.
//  - Construir pede, nesta ordem: sucata, vaga (menos de MaxPieces peças), alcance, a pegada inteira em chão andável, folga do
//    jogador e nenhuma sobreposição com outra peça (encostar pode, para fazer muro). O primeiro motivo que falha é o que volta.

#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "Rules/ZRCityPlan.h"

namespace ZR
{
	enum class EPieceKind : std::uint8_t
	{
		Barricade,
		Trap,
	};

	enum class ETrapTarget : std::uint8_t
	{
		Legs,
		Body,
	};

	/** Uma linha de Data/Construcoes.csv. */
	struct FBuildPieceType
	{
		std::string Id;
		std::string NamePt;
		std::string NameEn;
		EPieceKind Kind = EPieceKind::Barricade;
		/** Tecla (1 a 9) no modo construção. */
		int Slot = 1;
		/** Preço em sucata. */
		int Cost = 20;
		/** Vida contra golpes de zumbi e explosões. */
		float Health = 400.0f;
		/** Medidas sem girar: largura no eixo X, profundidade no Y [cm]. */
		float WidthCm = 300.0f;
		float DepthCm = 30.0f;
		float HeightCm = 110.0f;
		// --- só armadilha ---
		/** Dano por segundo em cada zumbi em cima. */
		float DamagePerSecond = 0.0f;
		ETrapTarget Target = ETrapTarget::Legs;
		/** Velocidade do zumbi em cima, em fração da normal (1 = não segura). */
		float SlowFactor = 1.0f;
		/** Dano total que a armadilha causa antes de gastar (0 = não gasta com o uso). */
		float Durability = 0.0f;
		/** Id de Explosivos.csv: a armadilha explode no primeiro zumbi que pisa (uso único). Vazio = não explode. */
		std::string ExplosiveId;
		std::uint32_t ColorRgb = 0x7A5230;

		bool IsExplosive() const { return !ExplosiveId.empty(); }
	};

	struct FBaseConfig
	{
		int StartScrap = 60;
		int ScrapPerCrate = 40;
		int ScrapPerWreck = 25;
		/** Segurando E ao lado do carro batido para desmanchar [s]. */
		float DismantleSeconds = 2.5f;
		/** Desmontar uma peça devolve esta fração do preço, pela vida que ela ainda tem. */
		float RefundFraction = 0.5f;
		/** Consertar uma peça do zero custa esta fração do preço dela. */
		float RepairCostFraction = 0.6f;
		/** Distância máxima do jogador até o centro da peça [cm]. */
		float BuildRangeCm = 700.0f;
		/** Grade em que a peça encaixa [cm]. */
		float SnapCm = 50.0f;
		int MaxPieces = 40;
		/** Folga mínima entre a pegada da peça e o centro do jogador [cm]. */
		float PlayerClearanceCm = 50.0f;
		/** Multiplica o ataque do zumbi quando ele bate na peça. */
		float ZombieDamageMultiplier = 1.0f;
		/** Multiplica o dano de explosão na peça. */
		float BlastDamageMultiplier = 0.5f;
		/** Barulho de construir uma peça (a martelada chama zumbi). */
		float BuildNoise = 3.0f;
	};

	/** Pegada de uma peça no plano [cm]: centro e meias medidas, já girada. */
	struct FFootprint
	{
		float X = 0.0f;
		float Y = 0.0f;
		float HalfX = 0.0f;
		float HalfY = 0.0f;
	};

	/** Rot90 em quartos de volta (qualquer inteiro, negativo também): ímpar troca largura e profundidade. */
	FFootprint MakeFootprint(const FBuildPieceType& Type, float X, float Y, int Rot90);

	/** Sobreposição de verdade: encostar borda com borda não conta. */
	bool FootprintsOverlap(const FFootprint& A, const FFootprint& B);

	/** O múltiplo de Step mais perto (Step <= 0 devolve o próprio valor). */
	float SnapToGrid(float Value, float Step);

	enum class EPlaceResult : std::uint8_t
	{
		Ok,
		NoScrap,
		TooMany,
		TooFar,
		OffStreet,
		OnPlayer,
		Blocked,
	};

	struct FPlaceQuery
	{
		FFootprint Piece;
		int Cost = 0;
		int Scrap = 0;
		/** Peças já construídas. */
		int PieceCount = 0;
		float PlayerX = 0.0f;
		float PlayerY = 0.0f;
	};

	EPlaceResult CheckPlacement(const FCityPlan& Plan, float CellSizeCm, const FBaseConfig& Config, const FPlaceQuery& Query,
		const std::vector<FFootprint>& Existing);

	/** Estado de uma peça construída. */
	struct FPieceState
	{
		float Health = 0.0f;
		/** Dano que a armadilha já causou (gasto). */
		float Wear = 0.0f;
		bool bBroken = false;

		void Reset(const FBuildPieceType& Type);
	};

	/** Golpe ou explosão na peça. Verdadeiro se quebrou com este golpe. */
	bool DamagePiece(FPieceState& State, float Amount);

	/** 0 inteira (acima de 2/3 da vida), 1 avariada (acima de 1/3), 2 quase caindo, 3 quebrada. */
	int DamageStage(const FBuildPieceType& Type, const FPieceState& State);

	struct FRepairResult
	{
		/** Vida depois do conserto. */
		float Health = 0.0f;
		int ScrapSpent = 0;
	};

	/**
	 * Conserta o que a sucata pagar. Preço do conserto completo: Cost x RepairCostFraction x (vida que falta / vida cheia),
	 * arredondado para cima. Com menos sucata que isso, gasta toda e conserta a parte proporcional. Peça quebrada não conserta.
	 */
	FRepairResult RepairPiece(const FBuildPieceType& Type, const FPieceState& State, int Scrap, const FBaseConfig& Config);

	/** Sucata devolvida ao desmontar: Cost x RefundFraction x (vida / vida cheia), arredondado para baixo. Quebrada: 0. */
	int DismantleRefund(const FBuildPieceType& Type, const FPieceState& State, const FBaseConfig& Config);

	struct FTrapContact
	{
		/** Dano neste zumbi neste passo. */
		float Damage = 0.0f;
		ETrapTarget Target = ETrapTarget::Legs;
		/** Velocidade do zumbi neste passo, em fração da normal. */
		float SpeedFactor = 1.0f;
		/** Armadilha explosiva: explode agora. */
		bool bDetonate = false;
		/** Gastou (ou explodiu) neste passo. */
		bool bBrokeNow = false;
	};

	/**
	 * Um zumbi em cima da armadilha por Dt segundos (chamar uma vez por zumbi). O dano para no que falta do desgaste;
	 * armadilha quebrada não faz nada.
	 */
	FTrapContact TouchTrap(const FBuildPieceType& Type, FPieceState& State, float Dt);
}
