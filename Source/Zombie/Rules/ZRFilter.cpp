#include "Rules/ZRFilter.h"

#include <algorithm>
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

		bool StartsWith(const std::string& Text, const std::string& Prefix)
		{
			return Text.size() >= Prefix.size() && Text.compare(0, Prefix.size(), Prefix) == 0;
		}

		/** Termo curto ("cu") só vale para a palavra inteira; a partir disto, também colado no nome. */
		constexpr std::size_t MinGluedLetters = 4;
	}

	std::string NormalizeForFilter(const std::string& Utf8)
	{
		return Normalize(Utf8, 1);
	}

	void FWordFilter::Set(const std::vector<std::string>& InTerms)
	{
		Terms.clear();
		for (const std::string& Term : InTerms)
		{
			FTerm Entry;
			Entry.bPrefix = !Term.empty() && Term.back() == '*';
			const std::string Stem = Entry.bPrefix ? Term.substr(0, Term.size() - 1) : Term;
			Entry.Collapsed = Normalize(Stem, 1);
			Entry.Capped = Normalize(Stem, 2);
			Entry.bDouble = Entry.Capped != Entry.Collapsed;
			if (!Entry.Collapsed.empty())
			{
				Terms.push_back(Entry);
			}
		}
	}

	bool FWordFilter::MatchesWord(const std::string& Utf8Word) const
	{
		const std::string Collapsed = Normalize(Utf8Word, 1);
		if (Collapsed.empty())
		{
			return false;
		}
		const std::string Capped = Normalize(Utf8Word, 2);
		return std::any_of(Terms.begin(), Terms.end(), [&Collapsed, &Capped](const FTerm& Term)
		{
			if (!Term.bPrefix)
			{
				return Collapsed == Term.Collapsed;
			}
			return Term.bDouble ? StartsWith(Capped, Term.Capped) : StartsWith(Collapsed, Term.Collapsed);
		});
	}

	std::string FWordFilter::Mask(const std::string& Utf8) const
	{
		if (IsEmpty())
		{
			return Utf8;
		}
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
			// uma palavra: até o próximo espaço
			std::size_t End = At;
			while (End < Utf8.size() && !IsSpace(Utf8[End]))
			{
				++End;
			}
			const std::string Word = Utf8.substr(At, End - At);
			if (!MatchesWord(Word))
			{
				Out += Word;
				At = End;
				continue;
			}
			// pega: as letras viram "*", a pontuação em volta fica
			std::size_t Inner = 0;
			while (Inner < Word.size())
			{
				std::size_t Bytes = 1;
				const char Letter = FilterLetter(NextCodePoint(Word, Inner, Bytes));
				if (Letter != 0)
				{
					Out.push_back('*');
				}
				else
				{
					Out.append(Word, Inner, Bytes);
				}
				Inner += Bytes;
			}
			At = End;
		}
		return Out;
	}

	bool FWordFilter::IsOffensiveName(const std::string& Name) const
	{
		if (MatchesWord(Name))
		{
			return true;
		}
		const std::string Collapsed = Normalize(Name, 1);
		const std::string Capped = Normalize(Name, 2);
		return std::any_of(Terms.begin(), Terms.end(), [&Collapsed, &Capped](const FTerm& Term)
		{
			const std::string& Needle = Term.bDouble ? Term.Capped : Term.Collapsed;
			const std::string& Hay = Term.bDouble ? Capped : Collapsed;
			return Needle.size() >= MinGluedLetters && Hay.find(Needle) != std::string::npos;
		});
	}
}
