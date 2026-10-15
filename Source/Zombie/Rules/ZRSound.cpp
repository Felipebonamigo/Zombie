#include "Rules/ZRSound.h"

#include <algorithm>

namespace ZR
{
	FSoundPicker::FSoundPicker(std::uint64_t Seed) : Rng(Seed) {}

	FSoundPick FSoundPicker::Pick(const std::vector<FSoundDef>& Defs, int Index, int ActiveVoices, float DistanceCm, double Now)
	{
		FSoundPick Out;
		if (Index < 0 || static_cast<std::size_t>(Index) >= Defs.size())
		{
			return Out;
		}
		const FSoundDef& Def = Defs[static_cast<std::size_t>(Index)];
		const std::size_t Slot = static_cast<std::size_t>(Index);
		if (LastFile.size() <= Slot)
		{
			LastFile.resize(Slot + 1, -1);
			LastTime.resize(Slot + 1, -1.0e9);
		}
		// relógio que voltou (fase recomeçada) libera a espera
		const double Since = Now - LastTime[Slot];
		if (Def.Files.empty() || ActiveVoices >= Def.MaxVoices || (!Def.b2D && DistanceCm > Def.RangeCm)
			|| (Since >= 0.0 && Since < static_cast<double>(Def.CooldownSeconds)))
		{
			return Out;
		}
		const int Count = static_cast<int>(Def.Files.size());
		int File = Rng.RangeInt(0, Count - 1);
		// sem repetir a anterior: sorteia entre as outras
		if (Count > 1 && File == LastFile[Slot])
		{
			File = (File + 1 + Rng.RangeInt(0, Count - 2)) % Count;
		}
		LastFile[Slot] = File;
		LastTime[Slot] = Now;
		Out.bPlay = true;
		Out.File = File;
		Out.Pitch = 1.0f + static_cast<float>(Rng.RangeDouble(-1.0, 1.0)) * Def.PitchJitter;
		Out.Volume = Def.Volume;
		return Out;
	}

	const std::vector<const char*>& RequiredSoundIds()
	{
		static const std::vector<const char*> Ids = {
			"tiro_pistola", "tiro_escopeta", "tiro_fuzil", "recarga", "arma_vazia", "explosao", "chiado", "zumbi_gemido",
			"zumbi_ataque", "zumbi_morte", "gritador_grito", "impacto_carne", "despedacar", "barricada_pancada", "barricada_quebra",
			"construir", "desmanche", "carro_motor", "carro_batida", "doacao", "pauta_ok", "pauta_nova", "clipe", "caixa", "chute",
		};
		return Ids;
	}

	namespace
	{
		std::uint32_t Read32(const std::string& Bytes, std::size_t At)
		{
			std::uint32_t Value = 0;
			for (std::size_t i = 0; i < 4; ++i)
			{
				Value |= static_cast<std::uint32_t>(static_cast<unsigned char>(Bytes[At + i])) << (8 * i);
			}
			return Value;
		}

		std::uint16_t Read16(const std::string& Bytes, std::size_t At)
		{
			return static_cast<std::uint16_t>(static_cast<unsigned char>(Bytes[At]) | (static_cast<unsigned char>(Bytes[At + 1]) << 8));
		}
	}

	bool ParseWav(const std::string& Bytes, FWavInfo& Out, std::string& Error)
	{
		Out = FWavInfo();
		if (Bytes.size() < 12 || Bytes.compare(0, 4, "RIFF") != 0 || Bytes.compare(8, 4, "WAVE") != 0)
		{
			Error = "não é RIFF/WAVE";
			return false;
		}
		bool bFormat = false;
		std::size_t At = 12;
		while (At + 8 <= Bytes.size())
		{
			const std::string Id = Bytes.substr(At, 4);
			const std::uint32_t Size = Read32(Bytes, At + 4);
			const std::size_t Body = At + 8;
			if (Id == "fmt ")
			{
				if (Size < 16 || Body + 16 > Bytes.size())
				{
					Error = "bloco fmt cortado";
					return false;
				}
				Out.Format = Read16(Bytes, Body);
				Out.Channels = Read16(Bytes, Body + 2);
				Out.SampleRate = static_cast<int>(Read32(Bytes, Body + 4));
				Out.BitsPerSample = Read16(Bytes, Body + 14);
				bFormat = true;
			}
			else if (Id == "data")
			{
				if (!bFormat)
				{
					Error = "dados antes do bloco fmt";
					return false;
				}
				const double BytesPerSecond = static_cast<double>(Out.SampleRate) * Out.Channels * (Out.BitsPerSample / 8);
				if (BytesPerSecond <= 0.0)
				{
					Error = "formato sem taxa, canais ou bits";
					return false;
				}
				// o tamanho declarado manda; se o arquivo acabar antes, conta só o que existe
				const std::size_t Available = Bytes.size() > Body ? Bytes.size() - Body : 0;
				Out.Seconds = static_cast<double>(std::min<std::size_t>(Size, Available)) / BytesPerSecond;
				return true;
			}
			// blocos têm tamanho par (preenchimento de 1 byte)
			At = Body + Size + (Size % 2);
		}
		Error = bFormat ? "sem bloco data" : "sem bloco fmt";
		return false;
	}
}
