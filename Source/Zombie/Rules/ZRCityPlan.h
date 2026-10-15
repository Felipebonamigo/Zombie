// Planta da cidade de Porto Brasa (protótipo): uma grade de células com ruas, prédios, praças, praia e mar.
// Regras puras: sem Unreal. O AZMBWorldBuilder transforma a planta em atores; o Diretor e os zumbis usam a mesma
// planta para o campo de fluxo (ZRFlowField.h). Contrato: docs/regras-de-jogo.md, seção 2.
//
// Coordenadas: X cresce para leste, Y para o sul. As linhas de terra vêm primeiro (Y = 0 .. LandHeight-1),
// depois BeachRows de praia e SeaRows de mar. Na terra, toda coluna e linha múltipla de (BlockCells + 1) é rua,
// e a borda também; o resto é prédio, ou praça: cada quarteirão inteiro vira praça com a chance PlazaChance.

#pragma once

#include <cstdint>
#include <vector>

namespace ZR
{
	enum class ECell : std::uint8_t
	{
		Road,
		Building,
		Plaza,
		Beach,
		Sea,
	};

	struct FCityConfig
	{
		int Width = 25;
		int Height = 30;
		/** Células de prédio entre duas ruas. */
		int BlockCells = 3;
		int BeachRows = 2;
		int SeaRows = 3;
		double PlazaChance = 0.08;
		int MinFloors = 2;
		int MaxFloors = 16;
		/** Recuo da fachada em cada prédio (a calçada e metade do beco entre dois prédios), sorteado entre os dois [cm]. */
		float MinInsetCm = 250.0f;
		float MaxInsetCm = 420.0f;
		/** Tamanho de uma célula no mundo [cm]. Só o Unreal usa; fica aqui para viajar junto da planta. */
		float CellSizeCm = 2000.0f;
		std::uint64_t Seed = 1;
	};

	struct FCityPlan
	{
		int Width = 0;
		int Height = 0;
		int LandHeight = 0;
		std::vector<ECell> Cells;
		/** Andares de cada célula: maior que zero só em prédio. */
		std::vector<int> Floors;
		/** Recuo da fachada de cada prédio [cm]; 0 nas outras células (e vazio numa planta montada à mão: prédio inteiro). */
		std::vector<float> InsetCm;

		bool InBounds(int X, int Y) const
		{
			return X >= 0 && Y >= 0 && X < Width && Y < Height;
		}

		/** Fora da planta conta como mar (intransponível). */
		ECell At(int X, int Y) const
		{
			return InBounds(X, Y) ? Cells[Index(X, Y)] : ECell::Sea;
		}

		int FloorsAt(int X, int Y) const
		{
			return InBounds(X, Y) ? Floors[Index(X, Y)] : 0;
		}

		bool IsWalkable(int X, int Y) const
		{
			const ECell Cell = At(X, Y);
			return Cell == ECell::Road || Cell == ECell::Plaza || Cell == ECell::Beach;
		}

		float InsetAt(int X, int Y) const
		{
			return InBounds(X, Y) && Index(X, Y) < InsetCm.size() ? InsetCm[Index(X, Y)] : 0.0f;
		}

		/**
		 * Chão num ponto do plano [cm, origem no canto da célula (0, 0)]: rua, praça e praia inteiras, e no prédio só a faixa
		 * do recuo (calçada e beco); a borda da fachada ainda é chão. Mar e fora da planta, não.
		 */
		bool IsWalkablePoint(float X, float Y, float CellSizeCm) const;

		/** O retângulo inteiro é chão: não toca mar nem fora da planta e não entra em prédio (encostar na fachada pode). */
		bool IsWalkableRect(float Left, float Top, float Right, float Bottom, float CellSizeCm) const;

		std::vector<std::uint8_t> WalkableMask() const;
		int CountOf(ECell Kind) const;

		std::size_t Index(int X, int Y) const
		{
			return static_cast<std::size_t>(Y) * static_cast<std::size_t>(Width) + static_cast<std::size_t>(X);
		}
	};

	FCityPlan GenerateCity(const FCityConfig& Config);

	/** Um ponto de objeto (explosivo, carro largado) numa célula de rua, com deslocamento dentro dela em [-0,5; 0,5]. */
	struct FPropSpot
	{
		int X = 0;
		int Y = 0;
		float OffsetX = 0.0f;
		float OffsetY = 0.0f;
	};

	/**
	 * Até Count células de rua diferentes, sorteadas com a semente, longe do início do jogador: a distância de Chebyshev
	 * até (AvoidX, AvoidY) passa de AvoidRadius. Devolve menos se não houver ruas suficientes.
	 */
	std::vector<FPropSpot> PickRoadSpots(const FCityPlan& Plan, int Count, std::uint64_t Seed, int AvoidX, int AvoidY, int AvoidRadius);
}
