// Momentos para clipe: guarda os lances recentes com uma nota e acha a melhor janela de N segundos. O botão "Clipar"
// e a marcação automática usam isto; quando o Steamworks entrar, a janela vira um evento na linha do tempo da
// Gravação de Jogo da Steam. Regras puras: sem Unreal. Contrato: docs/regras-de-jogo.md, seção 8.

#pragma once

#include <cstddef>
#include <deque>
#include <string>

namespace ZR
{
	class FHighlightTracker
	{
	public:
		explicit FHighlightTracker(double InKeepSeconds = 60.0) : KeepSeconds(InKeepSeconds) {}

		void Add(double Time, float Score, const std::string& Label);

		/** Esquece momentos mais velhos que KeepSeconds antes de Now. */
		void Prune(double Now);

		struct FWindow
		{
			bool bValid = false;
			/** Hora do primeiro e do último momento da janela. */
			double Start = 0.0;
			double End = 0.0;
			float Score = 0.0f;
			/** Rótulo do momento de maior nota da janela. */
			std::string Label;
		};

		/** A janela de no máximo WindowSeconds com a maior soma de notas entre os momentos guardados até Now. */
		FWindow BestWindow(double Now, double WindowSeconds) const;

		std::size_t Count() const { return Moments.size(); }

	private:
		struct FMoment
		{
			double Time = 0.0;
			float Score = 0.0f;
			std::string Label;
		};

		double KeepSeconds;
		std::deque<FMoment> Moments;
	};
}
