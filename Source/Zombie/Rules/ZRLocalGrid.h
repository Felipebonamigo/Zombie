// Grade fina em volta do jogador: o pedaço da cidade numa resolução de 1 m, com as barricadas da base marcadas como "caro,
// mas passa". O Diretor faz um segundo campo de fluxo (ZRFlowField.h) nela, com o custo extra, e os zumbis por perto o
// seguem: se o desvio por uma abertura custa menos que quebrar a barricada, a horda dá a volta e entra pela abertura (onde
// ficam as armadilhas); se não, vem e bate. A grade da cidade (20 m por célula) é grossa demais para isso.
// Regras puras: sem Unreal. Contrato: docs/regras-de-jogo.md, seção 12.
//
// Coordenadas no plano da cidade [cm], como em ZRBase.h. A origem da grade encaixa na resolução, para as células não
// "andarem" quando o jogador anda.

#pragma once

#include <cstdint>
#include <vector>

#include "Rules/ZRBase.h"
#include "Rules/ZRCityPlan.h"

namespace ZR
{
	struct FLocalGridConfig
	{
		/** Lado da grade, quadrada e centrada no jogador [cm]. */
		float SizeCm = 6400.0f;
		/** Lado de uma célula da grade [cm]. */
		float ResolutionCm = 100.0f;
		/** A barricada cresce isto em volta (o raio do zumbi), para o caminho não raspar nela [cm]. */
		float InflateCm = 40.0f;
		/** Custo extra de entrar numa célula de barricada (10 = um passo de uma célula): o desvio que a horda aceita. */
		int BarricadeCost = 400;
	};

	struct FLocalGrid
	{
		/** Canto da célula (0, 0) no plano da cidade [cm]. */
		float OriginX = 0.0f;
		float OriginY = 0.0f;
		float ResolutionCm = 100.0f;
		int Width = 0;
		int Height = 0;
		/** 1 onde a cidade é andável (rua, praça, praia) e dentro da planta. */
		std::vector<std::uint8_t> Walkable;
		/** Custo extra de cada célula (barricada). */
		std::vector<int> ExtraCost;

		/** A célula de um ponto do plano. Falso fora da grade. */
		bool ToCell(float X, float Y, int& OutX, int& OutY) const;
		/** O centro de uma célula, no plano. */
		void CellCenter(int X, int Y, float& OutX, float& OutY) const;
		/** Quantas células têm custo extra. */
		int CountCostly() const;
	};

	FLocalGrid MakeLocalGrid(const FCityPlan& Plan, float CityCellCm, float CenterX, float CenterY, const FLocalGridConfig& Config,
		const std::vector<FFootprint>& Barricades);
}
