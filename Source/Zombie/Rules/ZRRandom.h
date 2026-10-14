// Sorteio determinístico (SplitMix64). Regras puras: sem Unreal, testáveis na nuvem.
// Mesma semente, mesma sequência, em qualquer compilador: a cidade e o chat simulado dependem disso.

#pragma once

#include <cstdint>

namespace ZR
{
	class FRng
	{
	public:
		explicit FRng(std::uint64_t InSeed = 1) : State(InSeed) {}

		std::uint64_t Next()
		{
			std::uint64_t Z = (State += 0x9E3779B97F4A7C15ull);
			Z = (Z ^ (Z >> 30)) * 0xBF58476D1CE4E5B9ull;
			Z = (Z ^ (Z >> 27)) * 0x94D049BB133111EBull;
			return Z ^ (Z >> 31);
		}

		/** Número em [0, 1). */
		double Unit()
		{
			return static_cast<double>(Next() >> 11) * (1.0 / 9007199254740992.0);
		}

		/** Inteiro em [Lo, Hi], inclusivo nos dois lados. Lo > Hi devolve Lo. */
		int RangeInt(int Lo, int Hi)
		{
			if (Hi <= Lo)
			{
				return Lo;
			}
			const std::uint64_t Span = static_cast<std::uint64_t>(static_cast<std::int64_t>(Hi) - static_cast<std::int64_t>(Lo)) + 1u;
			return static_cast<int>(static_cast<std::int64_t>(Lo) + static_cast<std::int64_t>(Next() % Span));
		}

		double RangeDouble(double Lo, double Hi)
		{
			return Lo + (Hi - Lo) * Unit();
		}

		bool Chance(double Probability)
		{
			return Unit() < Probability;
		}

	private:
		std::uint64_t State;
	};
}
