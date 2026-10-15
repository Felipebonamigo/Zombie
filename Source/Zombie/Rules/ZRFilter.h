// Filtro de palavras do Modo Streamer: o que vem do chat da Twitch (texto, apelido e o nome do !zumbi) passa por aqui
// antes de aparecer na tela. Regras puras: sem Unreal. Contrato: docs/regras-de-jogo.md, seção 16.
//
//  - A lista (Data/FiltroPalavras.csv) é de termos; "fod*" pega tudo que começa com "fod".
//  - Compara a palavra inteira depois de normalizar: minúsculas, sem acento, 0/1/3/4/5/7/@/$ viram o/i/e/a/s/t/a/s,
//    sem pontuação no meio ("p.o.r.r.a") e sem letra repetida ("porrrra"). Palavra inocente que contém um termo
//    ("computador", "cuidado") não é pega. Prefixo e termo colado com letra dobrada comparam com a repetição limitada a 2
//    (sem isso "pass*" encolheria para "pas" e pegaria "pasta").
//  - Trocar: só as letras da palavra pega viram "*"; espaços, pontuação e o resto da frase ficam iguais.
//  - Nome de espectador: pego se for um termo ou se tiver, colado, um termo de 4 letras ou mais ("xXporraXx").

#pragma once

#include <string>
#include <vector>

namespace ZR
{
	/** O texto do jeito que o filtro compara (só letras a-z, normalizadas). */
	std::string NormalizeForFilter(const std::string& Utf8);

	/** Uma linha de Data/FiltroPalavras.csv. */
	struct FFilterTerm
	{
		std::string Text;
		/** Pega também colado no meio do apelido ("xXporraXx"); só para termo que não aparece dentro de nome comum. */
		bool bGlued = false;
	};

	class FWordFilter
	{
	public:
		void Set(const std::vector<FFilterTerm>& InTerms);
		/** Termos sem a marca de colado. */
		void Set(const std::vector<std::string>& InTerms);
		bool IsEmpty() const { return Terms.empty(); }

		/** Troca por "*" as letras de cada palavra proibida; o resto do texto fica igual (UTF-8). */
		std::string Mask(const std::string& Utf8) const;

		/** Nome de espectador (só letras, números e _) que não pode aparecer em cima do zumbi. */
		bool IsOffensiveName(const std::string& Name) const;

	private:
		struct FTerm
		{
			/** As letras do termo, uma por sequência ("porra": p o r a)... */
			std::vector<char> Letters;
			/** ...e quantas de cada (1 ou 2: "porra" pede o r dobrado). */
			std::vector<int> Counts;
			bool bPrefix = false;
			bool bGlued = false;
		};

		/** Algum termo bate com o texto: na palavra inteira (prefixo vale), ou em qualquer ponto dela (bAnywhere). */
		bool MatchesAt(const std::string& Utf8, bool bAnywhere, bool bOnlyGlued) const;

		std::vector<FTerm> Terms;
	};
}
