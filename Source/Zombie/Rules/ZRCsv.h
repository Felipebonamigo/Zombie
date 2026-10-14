// Leitor de CSV das tabelas de Data/ (RFC 4180: aspas, vírgula e quebra de linha dentro de aspas, "" vira ").
// Regras puras: sem Unreal. Os números aceitam ponto ou vírgula decimal (planilha em português).

#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace ZR
{
	struct FCsvTable
	{
		std::vector<std::string> Header;
		std::vector<std::vector<std::string>> Rows;
		/** Número da linha no arquivo (1 = cabeçalho) de cada linha de Rows, para as mensagens de erro. */
		std::vector<int> RowLines;

		/** Índice da coluna, ou -1. */
		int Column(const std::string& Name) const;
	};

	/** Falso com Error preenchido se: arquivo vazio, aspas sem fechar ou linha com número de colunas diferente. Linhas vazias são puladas. */
	bool ParseCsv(const std::string& Text, FCsvTable& Out, std::string& Error);

	bool ParseFloat(const std::string& Text, float& Out);
	bool ParseInt(const std::string& Text, int& Out);
	/** "#RRGGBB" para 0xRRGGBB. */
	bool ParseColor(const std::string& Text, std::uint32_t& Out);
}
