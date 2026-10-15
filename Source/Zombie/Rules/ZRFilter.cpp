#include "Rules/ZRFilter.h"

#include <algorithm>
#include <cctype>
#include <cstdint>

namespace ZR
{
	namespace
	{
		/** Próximo caractere UTF-8 a partir de At: o código e quantos bytes ele ocupa (byte inválido conta como 1). */
		std::uint32_t NextCodePoint(const std::string& Text, std::size_t At, std::size_t& Bytes)
		{
			const unsigned char Lead = static_cast<unsigned char>(Text[At]);
			Bytes = Lead < 0x80 ? 1 : (Lead >> 5) == 0x6 ? 2 : (Lead >> 4) == 0xE ? 3 : (Lead >> 3) == 0x1E ? 4 : 1;
			if (At + Bytes > Text.size())
			{
				Bytes = 1;
			}
			if (Bytes == 1)
			{
				return Lead;
			}
			std::uint32_t Code = Lead & (0xFFu >> (Bytes + 1));
			for (std::size_t i = 1; i < Bytes; ++i)
			{
				Code = (Code << 6) | (static_cast<unsigned char>(Text[At + i]) & 0x3Fu);
			}
			return Code;
		}

		/** A letra que o caractere vale para o filtro (a-z), ou 0 se ele não conta (pontuação, espaço, emoji). */
		char FilterLetter(std::uint32_t Code)
		{
			if (Code >= 'a' && Code <= 'z')
			{
				return static_cast<char>(Code);
			}
			if (Code >= 'A' && Code <= 'Z')
			{
				return static_cast<char>(Code - 'A' + 'a');
			}
			switch (Code)
			{
			case '0':
				return 'o';
			case '1':
				return 'i';
			case '3':
				return 'e';
			case '4':
			case '@':
				return 'a';
			case '5':
			case '$':
				return 's';
			case '7':
				return 't';
			default:
				break;
			}
			// Latin-1 (U+00C0 a U+00FF): as letras acentuadas do português e do espanhol
			if (Code >= 0xC0 && Code <= 0xFF && Code != 0xD7 && Code != 0xF7)
			{
				static const char Latin1[] = "aaaaaaaceeeeiiiidnooooo-ouuuuyts" "aaaaaaaceeeeiiiidnooooo-ouuuuyty";
				return Latin1[Code - 0xC0];
			}
			return 0;
		}

		bool IsSpace(char C)
		{
			return C == ' ' || C == '\t' || C == '\n' || C == '\r';
		}

		/** As letras do filtro, com a mesma letra seguida no máximo MaxRun vezes ("porrrra": 1 dá "pora", 2 dá "porra"). */
		std::string Normalize(const std::string& Utf8, std::size_t MaxRun)
		{
			std::string Out;
			std::size_t Run = 0;
			std::size_t At = 0;
			while (At < Utf8.size())
			{
				std::size_t Bytes = 1;
				const char Letter = FilterLetter(NextCodePoint(Utf8, At, Bytes));
				At += Bytes;
				if (Letter == 0)
				{
					continue;
				}
				Run = !Out.empty() && Out.back() == Letter ? Run + 1 : 1;
				if (Run <= MaxRun)
				{
					Out.push_back(Letter);
				}
			}
			return Out;
		}

		/** Uma sequência da mesma letra: "porrra" é p1 o1 r3 a1. */
		struct FRun
		{
			char Letter = 0;
			int Count = 0;
		};

		/** As sequências de letras do texto (o que não é letra para o filtro some, então "p.o.r.r.a" é p o r2 a). */
		std::vector<FRun> RunsOf(const std::string& Utf8)
		{
			std::vector<FRun> Runs;
			std::size_t At = 0;
			while (At < Utf8.size())
			{
				std::size_t Bytes = 1;
				const char Letter = FilterLetter(NextCodePoint(Utf8, At, Bytes));
				At += Bytes;
				if (Letter == 0)
				{
					continue;
				}
				if (!Runs.empty() && Runs.back().Letter == Letter)
				{
					++Runs.back().Count;
				}
				else
				{
					Runs.push_back({Letter, 1});
				}
			}
			return Runs;
		}

		/**
		 * Letra dobrada no termo pede dobrada (ou mais) no texto: "night" não é "nigg*". Letra simples no termo aceita 1 ou
		 * 3 ou mais, nunca 2: dobrar é a grafia normal de muita palavra ("food" não é "fod*"), esticar é "foooda".
		 */
		bool RunMatches(int WordCount, int TermCount)
		{
			return TermCount >= 2 ? WordCount >= 2 : (WordCount == 1 || WordCount >= 3);
		}

		/** Nome de espectador em pedaços: "_", o que não é letra nem dígito e a troca de minúscula para maiúscula. */
		std::vector<std::string> NamePieces(const std::string& Name)
		{
			std::vector<std::string> Pieces(1);
			for (std::size_t i = 0; i < Name.size(); ++i)
			{
				const unsigned char C = static_cast<unsigned char>(Name[i]);
				const bool bWordChar = std::isalnum(C) != 0;
				const bool bCamel = i > 0 && std::isupper(C) != 0 && std::islower(static_cast<unsigned char>(Name[i - 1])) != 0;
				if (!bWordChar || bCamel)
				{
					if (!Pieces.back().empty())
					{
						Pieces.emplace_back();
					}
					if (!bWordChar)
					{
						continue;
					}
				}
				Pieces.back().push_back(Name[i]);
			}
			return Pieces;
		}
	}

	std::string NormalizeForFilter(const std::string& Utf8)
	{
		return Normalize(Utf8, 1);
	}

	void FWordFilter::Set(const std::vector<FFilterTerm>& InTerms)
	{
		Terms.clear();
		for (const FFilterTerm& Source : InTerms)
		{
			FTerm Entry;
			Entry.bPrefix = !Source.Text.empty() && Source.Text.back() == '*';
			Entry.bGlued = Source.bGlued;
			const std::string Stem = Entry.bPrefix ? Source.Text.substr(0, Source.Text.size() - 1) : Source.Text;
			for (const FRun& Run : RunsOf(Stem))
			{
				// o termo guarda até 2: "porrra" na lista vale como "porra"
				Entry.Letters.push_back(Run.Letter);
				Entry.Counts.push_back(Run.Count >= 2 ? 2 : 1);
			}
			if (!Entry.Letters.empty())
			{
				Terms.push_back(Entry);
			}
		}
	}

	void FWordFilter::Set(const std::vector<std::string>& InTerms)
	{
		std::vector<FFilterTerm> Plain;
		for (const std::string& Text : InTerms)
		{
			FFilterTerm Term;
			Term.Text = Text;
			Plain.push_back(Term);
		}
		Set(Plain);
	}

	bool FWordFilter::MatchesAt(const std::string& Utf8, bool bAnywhere, bool bOnlyGlued) const
	{
		const std::vector<FRun> Word = RunsOf(Utf8);
		if (Word.empty())
		{
			return false;
		}
		for (const FTerm& Term : Terms)
		{
			if (bOnlyGlued && !Term.bGlued)
			{
				continue;
			}
			const std::size_t Length = Term.Letters.size();
			const std::size_t LastStart = Word.size() < Length ? 0 : Word.size() - Length;
			for (std::size_t Start = 0; Word.size() >= Length && Start <= (bAnywhere ? LastStart : 0); ++Start)
			{
				// palavra inteira: o termo exato tem que acabar junto com ela; prefixo e colado, não
				if (!bAnywhere && !Term.bPrefix && Length != Word.size())
				{
					break;
				}
				bool bMatch = true;
				for (std::size_t i = 0; i < Length && bMatch; ++i)
				{
					bMatch = Word[Start + i].Letter == Term.Letters[i] && RunMatches(Word[Start + i].Count, Term.Counts[i]);
				}
				if (bMatch)
				{
					return true;
				}
			}
		}
		return false;
	}

	std::string FWordFilter::Mask(const std::string& Utf8) const
	{
		if (IsEmpty())
		{
			return Utf8;
		}
		// troca por "*" as letras de Text[From, To)
		auto Star = [](const std::string& Text, std::size_t From, std::size_t To, std::string& Out)
		{
			std::size_t At = From;
			while (At < To)
			{
				std::size_t Bytes = 1;
				const char Letter = FilterLetter(NextCodePoint(Text, At, Bytes));
				if (Letter != 0)
				{
					Out.push_back('*');
				}
				else
				{
					Out.append(Text, At, Bytes);
				}
				At += Bytes;
			}
		};
		std::string Out;
		Out.reserve(Utf8.size());
		std::size_t At = 0;
		while (At < Utf8.size())
		{
			if (IsSpace(Utf8[At]))
			{
				Out.push_back(Utf8[At++]);
				continue;
			}
			std::size_t End = At;
			while (End < Utf8.size() && !IsSpace(Utf8[End]))
			{
				++End;
			}
			// menção: o @ do começo é do chat, não um "a"
			while (At < End && Utf8[At] == '@')
			{
				Out.push_back(Utf8[At++]);
			}
			const std::string Word = Utf8.substr(At, End - At);
			if (MatchesAt(Word, false, false))
			{
				Star(Word, 0, Word.size(), Out);
				At = End;
				continue;
			}
			// a palavra inteira não é termo: os pedaços entre pontuação podem ser ("porra,caralho", "filho-da-puta")
			std::size_t Piece = 0;
			while (Piece < Word.size())
			{
				std::size_t Bytes = 1;
				if (FilterLetter(NextCodePoint(Word, Piece, Bytes)) == 0)
				{
					Out.append(Word, Piece, Bytes);
					Piece += Bytes;
					continue;
				}
				std::size_t PieceEnd = Piece;
				while (PieceEnd < Word.size())
				{
					std::size_t Next = 1;
					if (FilterLetter(NextCodePoint(Word, PieceEnd, Next)) == 0)
					{
						break;
					}
					PieceEnd += Next;
				}
				const std::string Part = Word.substr(Piece, PieceEnd - Piece);
				if (MatchesAt(Part, false, false))
				{
					Star(Word, Piece, PieceEnd, Out);
				}
				else
				{
					Out += Part;
				}
				Piece = PieceEnd;
			}
			At = End;
		}
		return Out;
	}

	bool FWordFilter::IsOffensiveName(const std::string& Name) const
	{
		if (MatchesAt(Name, false, false))
		{
			return true;
		}
		for (const std::string& Piece : NamePieces(Name))
		{
			if (MatchesAt(Piece, false, false))
			{
				return true;
			}
		}
		// colado no meio do nome ("xXporraXx"): só os termos marcados, que não aparecem dentro de nome comum
		return MatchesAt(Name, true, true);
	}
}
