// Som do jogo: a tabela Data/Sons.csv (variações, volume, alcance, vozes) e a escolha de cada toque. Regras puras: sem Unreal.
// O Unreal (Audio/ZMBAudio) carrega os arquivos importados de Audio/*.wav e toca o que o FSoundPicker mandar.
// Contrato: docs/regras-de-jogo.md, seção 13.
//
//  - Variação: sorteada entre os arquivos do som, sem repetir a anterior (o tiro não soa como metralhadora de um som só).
//  - Tom: 1 ± variacao_tom, sorteado a cada toque.
//  - Não toca: sem arquivo, longe demais (só som com posição), com vozes_max do mesmo som já tocando, ou antes de espera_s
//    desde o último toque dele (cem zumbis gemendo no mesmo quadro viram um coro, não um estouro); relógio que voltou
//    (fase recomeçada) libera a espera.

#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "Rules/ZRRandom.h"

namespace ZR
{
	struct FSoundDef
	{
		std::string Id;
		/** Nomes dos arquivos, sem extensão: Audio/<nome>.wav, importado como /Game/Zombie/Sons/SW_<nome>. */
		std::vector<std::string> Files;
		float Volume = 1.0f;
		/** O tom varia 1 ± isto. */
		float PitchJitter = 0.05f;
		/** Daqui para longe não toca [cm]; também é o fim da atenuação. */
		float RangeCm = 4000.0f;
		/** Toques deste som ao mesmo tempo. */
		int MaxVoices = 4;
		/** Espera mínima entre dois toques deste som [s]. */
		float CooldownSeconds = 0.0f;
		/** Em laço (motor do carro): o Unreal toca preso a um componente, sem fim. */
		bool bLoop = false;
		/** Interface (doação, pauta, clipe): sem posição nem distância. */
		bool b2D = false;
	};

	struct FSoundPick
	{
		bool bPlay = false;
		int File = -1;
		float Pitch = 1.0f;
		float Volume = 1.0f;
	};

	class FSoundPicker
	{
	public:
		explicit FSoundPicker(std::uint64_t Seed = 1);

		/**
		 * Decide um toque do som Defs[Index]: ActiveVoices é quantos dele tocam agora, DistanceCm é do ouvinte até a origem
		 * (ignorada em som de interface) e Now é o relógio do jogo [s].
		 */
		FSoundPick Pick(const std::vector<FSoundDef>& Defs, int Index, int ActiveVoices, float DistanceCm, double Now);

	private:
		FRng Rng;
		std::vector<int> LastFile;
		std::vector<double> LastTime;
	};

	/** Os ids que o código toca: a tabela precisa ter uma linha para cada (pode ficar sem arquivo, e aí é silêncio). */
	const std::vector<const char*>& RequiredSoundIds();

	struct FWavInfo
	{
		/** 1 = PCM. */
		int Format = 0;
		int Channels = 0;
		int SampleRate = 0;
		int BitsPerSample = 0;
		double Seconds = 0.0;
	};

	/** Lê o cabeçalho RIFF/WAVE (pula blocos desconhecidos, como LIST). Falso com o motivo em Error. */
	bool ParseWav(const std::string& Bytes, FWavInfo& Out, std::string& Error);
}
