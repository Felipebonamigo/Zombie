// Campo de fluxo da horda: um mapa de custo até o alvo (o jogador) sobre a grade da cidade. Cada zumbi só olha a
// célula onde está e anda para a vizinha mais barata; recalcular é um Dijkstra por grade inteira, barato para
// algumas centenas de células, e serve a quantos zumbis houver. Regras puras: sem Unreal.
// Contrato: docs/regras-de-jogo.md, seção 3.
//
// Custo 10 na ortogonal e 14 na diagonal; a diagonal só vale se as duas ortogonais do canto forem andáveis (não corta
// quina de prédio).

#pragma once

#include <cstdint>
#include <vector>

namespace ZR
{
	class FFlowField
	{
	public:
		/** Walkable: 1 andável, 0 bloqueado; Width x Height, linha a linha. Alvo bloqueado deixa tudo inalcançável. */
		void Build(const std::vector<std::uint8_t>& Walkable, int InWidth, int InHeight, int TargetX, int TargetY);

		/** Custo até o alvo, ou -1 (bloqueado, fora da grade ou sem caminho). */
		int CostAt(int X, int Y) const;

		/** A vizinha com menor custo. Falso no alvo, fora da grade ou sem caminho. */
		bool StepToward(int X, int Y, int& OutX, int& OutY) const;

		/** Vetor unitário da célula até a próxima (X para leste, Y para o sul). Falso como StepToward. */
		bool Direction(int X, int Y, double& OutDx, double& OutDy) const;

		/** Verdadeiro se o alvo era andável quando o campo foi feito. */
		bool IsValid() const { return bValid; }

		int GetTargetX() const { return TargetCellX; }
		int GetTargetY() const { return TargetCellY; }

	private:
		bool Walk(int X, int Y) const;

		int Width = 0;
		int Height = 0;
		int TargetCellX = -1;
		int TargetCellY = -1;
		bool bValid = false;
		std::vector<std::uint8_t> Open;
		std::vector<int> Cost;
	};
}
