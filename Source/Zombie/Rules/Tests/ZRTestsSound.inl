// Testes: som (tabela Data/Sons.csv, escolha da variação, vozes, espera, distância e o cabeçalho dos .wav de Audio/).
// Contrato: docs/regras-de-jogo.md, seção 13. Usa ReadDataFile e ExpectNoErrors de ZRTestsData.inl.

#include <cstring>
#include <set>

#include "Rules/ZRSound.h"

namespace
{
	ZR::FSoundDef SomDe(const char* Id, int Files)
	{
		ZR::FSoundDef Def;
		Def.Id = Id;
		for (int i = 0; i < Files; ++i)
		{
			Def.Files.push_back(std::string(Id) + "_0" + std::to_string(i + 1));
		}
		Def.Volume = 0.8f;
		Def.PitchJitter = 0.1f;
		Def.RangeCm = 3000.0f;
		Def.MaxVoices = 3;
		return Def;
	}

	void Poe32(std::string& Out, std::uint32_t Value)
	{
		for (int i = 0; i < 4; ++i)
		{
			Out.push_back(static_cast<char>((Value >> (8 * i)) & 0xFF));
		}
	}

	void Poe16(std::string& Out, std::uint16_t Value)
	{
		Out.push_back(static_cast<char>(Value & 0xFF));
		Out.push_back(static_cast<char>((Value >> 8) & 0xFF));
	}

	/** Um .wav PCM com um bloco LIST antes dos dados (como o que o ffmpeg grava). */
	std::string WavDe(std::uint16_t Channels, std::uint32_t Rate, std::uint16_t Bits, std::uint32_t DataBytes)
	{
		std::string Body = "WAVE";
		Body += "fmt ";
		Poe32(Body, 16);
		Poe16(Body, 1);
		Poe16(Body, Channels);
		Poe32(Body, Rate);
		Poe32(Body, Rate * Channels * Bits / 8);
		Poe16(Body, static_cast<std::uint16_t>(Channels * Bits / 8));
		Poe16(Body, Bits);
		Body += "LIST";
		Poe32(Body, 4);
		Body += "INFO";
		Body += "data";
		Poe32(Body, DataBytes);
		Body += std::string(DataBytes, '\0');
		std::string Out = "RIFF";
		Poe32(Out, static_cast<std::uint32_t>(Body.size()));
		return Out + Body;
	}
}

ZR_TEST(SomVariacao, "Som.NaoRepeteAMesmaVariacaoSeguidaEUsaTodas")
{
	const std::vector<ZR::FSoundDef> Defs = {SomDe("tiro", 3)};
	ZR::FSoundPicker Picker(11);
	int Last = -1;
	bool bRepeated = false;
	std::set<int> Used;
	for (int i = 0; i < 300; ++i)
	{
		const ZR::FSoundPick Pick = Picker.Pick(Defs, 0, 0, 100.0f, i * 1.0);
		ZR_EXPECT(Pick.bPlay);
		bRepeated = bRepeated || Pick.File == Last;
		Last = Pick.File;
		Used.insert(Pick.File);
	}
	ZR_EXPECT(!bRepeated);
	ZR_EQ(Used.size(), static_cast<std::size_t>(3));
	// um arquivo só: sempre ele
	const std::vector<ZR::FSoundDef> One = {SomDe("ding", 1)};
	ZR_EQ(Picker.Pick(One, 0, 0, 100.0f, 0.0).File, 0);
	ZR_EQ(Picker.Pick(One, 0, 0, 100.0f, 1.0).File, 0);
}

ZR_TEST(SomTom, "Som.TomVariaDentroDaFaixaEVolumeDaTabela")
{
	const std::vector<ZR::FSoundDef> Defs = {SomDe("tiro", 2)};
	ZR::FSoundPicker Picker(5);
	float Low = 10.0f;
	float High = 0.0f;
	for (int i = 0; i < 200; ++i)
	{
		const ZR::FSoundPick Pick = Picker.Pick(Defs, 0, 0, 0.0f, i * 1.0);
		Low = std::min(Low, Pick.Pitch);
		High = std::max(High, Pick.Pitch);
		ZR_NEAR(Pick.Volume, 0.8f, 1e-5);
	}
	ZR_EXPECT(Low >= 0.9f && High <= 1.1f);
	ZR_EXPECT(High - Low > 0.1f);
}

ZR_TEST(SomVozesEspera, "Som.LimiteDeVozesEEsperaEntreToques")
{
	std::vector<ZR::FSoundDef> Defs = {SomDe("gemido", 4)};
	Defs[0].CooldownSeconds = 0.5f;
	ZR::FSoundPicker Picker(3);
	ZR_EXPECT(!Picker.Pick(Defs, 0, 3, 100.0f, 0.0).bPlay); // 3 vozes já tocando
	ZR_EXPECT(Picker.Pick(Defs, 0, 2, 100.0f, 0.0).bPlay);
	ZR_EXPECT(!Picker.Pick(Defs, 0, 0, 100.0f, 0.3).bPlay); // espera de 0,5 s
	ZR_EXPECT(Picker.Pick(Defs, 0, 0, 100.0f, 0.6).bPlay);
	// a espera é por som: outro som toca no mesmo instante
	Defs.push_back(SomDe("tiro", 2));
	ZR_EXPECT(Picker.Pick(Defs, 1, 0, 100.0f, 0.6).bPlay);
}

ZR_TEST(SomRoubaVoz, "Som.RajadaRoubaAVozMaisAntigaEmVezDeSairMuda")
{
	std::vector<ZR::FSoundDef> Defs = {SomDe("fuzil", 4), SomDe("gemido", 4)};
	Defs[0].bStealOldest = true;
	Defs[0].CooldownSeconds = 0.05f;
	ZR::FSoundPicker Picker(5);
	// vozes cheias: o tiro toca mesmo assim (o Unreal corta o mais antigo); o gemido não
	ZR_EXPECT(Picker.Pick(Defs, 0, 3, 100.0f, 0.0).bPlay);
	ZR_EXPECT(Picker.Pick(Defs, 0, 9, 100.0f, 0.1).bPlay);
	ZR_EXPECT(!Picker.Pick(Defs, 1, 3, 100.0f, 0.0).bPlay);
	// o resto das regras continua: espera e alcance
	ZR_EXPECT(!Picker.Pick(Defs, 0, 3, 100.0f, 0.12).bPlay);
	ZR_EXPECT(!Picker.Pick(Defs, 0, 0, 3001.0f, 1.0).bPlay);
}

ZR_TEST(SomDistancia, "Som.LongeDemaisOuSemArquivoNaoToca")
{
	std::vector<ZR::FSoundDef> Defs = {SomDe("tiro", 2), SomDe("ding", 1), SomDe("vazio", 0)};
	Defs[1].b2D = true;
	ZR::FSoundPicker Picker(9);
	ZR_EXPECT(Picker.Pick(Defs, 0, 0, 2999.0f, 0.0).bPlay);
	ZR_EXPECT(!Picker.Pick(Defs, 0, 0, 3001.0f, 1.0).bPlay);
	// interface não tem distância
	ZR_EXPECT(Picker.Pick(Defs, 1, 0, 1.0e7f, 0.0).bPlay);
	ZR_EXPECT(!Picker.Pick(Defs, 2, 0, 0.0f, 0.0).bPlay);
	// índice fora da tabela não toca nem quebra
	ZR_EXPECT(!Picker.Pick(Defs, 7, 0, 0.0f, 0.0).bPlay);
	ZR_EXPECT(!Picker.Pick(Defs, -1, 0, 0.0f, 0.0).bPlay);
}

ZR_TEST(SomWav, "Som.CabecalhoWavMonoPcm16")
{
	ZR::FWavInfo Info;
	std::string Error;
	ZR_EXPECT(ZR::ParseWav(WavDe(1, 48000, 16, 9600), Info, Error));
	ZR_EQ(Info.Format, 1);
	ZR_EQ(Info.Channels, 1);
	ZR_EQ(Info.SampleRate, 48000);
	ZR_EQ(Info.BitsPerSample, 16);
	ZR_NEAR(Info.Seconds, 0.1, 1e-9);
	ZR_EXPECT(ZR::ParseWav(WavDe(2, 44100, 24, 26460), Info, Error));
	ZR_EQ(Info.Channels, 2);
	ZR_NEAR(Info.Seconds, 0.1, 1e-9);

	ZR_EXPECT(!ZR::ParseWav("RIFX1234WAVE", Info, Error));
	ZR_EXPECT(!Error.empty());
	const std::string Good = WavDe(1, 48000, 16, 9600);
	ZR_EXPECT(!ZR::ParseWav(Good.substr(0, 30), Info, Error)); // cortado no meio do fmt
	std::string NoData = Good.substr(0, 12 + 8 + 16);           // só RIFF e fmt
	ZR_EXPECT(!ZR::ParseWav(NoData, Info, Error));
}

ZR_TEST(DadosSonsErros, "Dados.SomComErroEhApontado")
{
	const std::string Csv =
		"id,arquivos,volume,variacao_tom,alcance_m,vozes_max,roubar,espera_s,laco,interface\n"
		"tiro,tiro_01;tiro_02,0.9,0.05,120,6,1,0,0,0\n"
		"tiro,tiro_03,1,0,10,1,0,0,0,0\n"
		"ding,Ding Bom.wav,1,0,1,1,0,0,0,1\n"
		"vazio,,1,0,10,2,0,0,0,0\n"
		"alto,alto_01,9,0,10,2,0,0,0,0\n";
	std::vector<ZR::FSoundDef> Sounds;
	std::vector<std::string> Errors;
	ZR::LoadSounds(Csv, Sounds, Errors);
	ZR_EQ(Sounds.size(), static_cast<std::size_t>(3));
	ZR_EQ(Errors.size(), static_cast<std::size_t>(3));
	const ZR::FSoundDef* Tiro = ZR::FindById(Sounds, "tiro");
	ZR_EXPECT(Tiro != nullptr && Tiro->Files.size() == 2 && Tiro->Files[1] == "tiro_02");
	ZR_EXPECT(Tiro != nullptr && Tiro->RangeCm > 11999.0f && Tiro->RangeCm < 12001.0f);
	ZR_EXPECT(Tiro != nullptr && Tiro->bStealOldest);
	const ZR::FSoundDef* Vazio = ZR::FindById(Sounds, "vazio");
	ZR_EXPECT(Vazio != nullptr && Vazio->Files.empty() && !Vazio->bStealOldest);
	bool bMentionsName = false;
	for (const std::string& Error : Errors)
	{
		bMentionsName = bMentionsName || Error.find("Ding Bom.wav") != std::string::npos;
	}
	ZR_EXPECT(bMentionsName);
}

ZR_TEST(DerivaSons, "Deriva.SonsTemOsQueOCodigoTocaEOsArquivosSaoWavMono48k")
{
	std::string Text;
	if (!ReadDataFile(T, "Sons.csv", Text))
	{
		return;
	}
	std::vector<ZR::FSoundDef> Sounds;
	std::vector<std::string> Errors;
	ZR::LoadSounds(Text, Sounds, Errors);
	ExpectNoErrors(T, "Sons.csv", Errors);
	for (const char* Id : ZR::RequiredSoundIds())
	{
		if (ZR::FindById(Sounds, Id) == nullptr)
		{
			T.Fail(std::string("Sons.csv sem o som ") + Id);
		}
	}
	ZR_EXPECT(ZR::RequiredSoundIds().size() >= 20);
	// o tiro é "tiro_<id da arma>": arma nova em Armas.csv sem som sairia muda
	std::string WeaponsText;
	if (ReadDataFile(T, "Armas.csv", WeaponsText))
	{
		std::vector<ZR::FWeaponType> Weapons;
		std::vector<std::string> WeaponErrors;
		ZR::LoadWeapons(WeaponsText, Weapons, WeaponErrors);
		for (const ZR::FWeaponType& Weapon : Weapons)
		{
			if (ZR::FindById(Sounds, "tiro_" + Weapon.Id) == nullptr)
			{
				T.Fail("Sons.csv sem tiro_" + Weapon.Id + " (a arma " + Weapon.Id + " de Armas.csv atiraria muda)");
			}
		}
	}
	// cada arquivo: Audio/<nome>.wav, PCM 16 bits, mono, 48 kHz; som solto até 6 s, laço até 12 s
	ZRTest::FContext Audio;
	Audio.DataDir = T.DataDir + "/../Audio";
	int Files = 0;
	for (const ZR::FSoundDef& Sound : Sounds)
	{
		for (const std::string& File : Sound.Files)
		{
			std::string Bytes;
			if (!ReadDataFile(Audio, (File + ".wav").c_str(), Bytes))
			{
				T.Fail("falta Audio/" + File + ".wav (som " + Sound.Id + ")");
				continue;
			}
			++Files;
			ZR::FWavInfo Info;
			std::string Error;
			if (!ZR::ParseWav(Bytes, Info, Error))
			{
				T.Fail("Audio/" + File + ".wav: " + Error);
				continue;
			}
			if (Info.Format != 1 || Info.Channels != 1 || Info.SampleRate != 48000 || Info.BitsPerSample != 16)
			{
				T.Fail("Audio/" + File + ".wav não é PCM 16 bits mono 48 kHz");
			}
			if (Info.Seconds > (Sound.bLoop ? 12.0 : 6.0) || Info.Seconds < 0.05)
			{
				T.Fail("Audio/" + File + ".wav com duração fora da faixa");
			}
		}
	}
	ZR_EXPECT(Files >= 40);
}

ZR_TEST(DerivaLicencas, "Deriva.LicencasLegiveisECitamTodoSom")
{
	// nenhum asset entra sem uma linha em docs/licencas.csv (CLAUDE.md): a planilha tem que abrir certa e citar cada som
	ZRTest::FContext Docs;
	Docs.DataDir = T.DataDir + "/../docs";
	std::string Text;
	std::string Sounds;
	if (!ReadDataFile(Docs, "licencas.csv", Text) || !ReadDataFile(T, "Sons.csv", Sounds))
	{
		T.Fail("não abri docs/licencas.csv ou Data/Sons.csv");
		return;
	}
	ZR::FCsvTable Table;
	std::string Error;
	if (!ZR::ParseCsv(Text, Table, Error))
	{
		T.Fail("docs/licencas.csv: " + Error);
		return;
	}
	const int Ai = Table.Column("gerado_por_ia");
	const int What = Table.Column("arquivo_ou_pacote");
	ZR_EXPECT(Ai >= 0 && What >= 0 && Table.Column("licenca") >= 0);
	if (Ai < 0 || What < 0)
	{
		return;
	}
	std::string Cited;
	for (std::size_t Row = 0; Row < Table.Rows.size(); ++Row)
	{
		const std::vector<std::string>& Cells = Table.Rows[Row];
		if (Cells.size() != Table.Header.size())
		{
			T.Fail("docs/licencas.csv linha " + std::to_string(Table.RowLines[Row]) + ": " + std::to_string(Cells.size()) + " colunas em vez de "
				+ std::to_string(Table.Header.size()) + " (vírgula sem aspas?)");
			continue;
		}
		const std::string& Flag = Cells[static_cast<std::size_t>(Ai)];
		if (Flag != "sim" && Flag != "não")
		{
			T.Fail("docs/licencas.csv linha " + std::to_string(Table.RowLines[Row]) + ": gerado_por_ia '" + Flag + "' não é sim ou não");
		}
		Cited += Cells[static_cast<std::size_t>(What)] + "\n";
	}
	std::vector<ZR::FSoundDef> Defs;
	std::vector<std::string> Errors;
	ZR::LoadSounds(Sounds, Defs, Errors);
	for (const ZR::FSoundDef& Def : Defs)
	{
		for (const std::string& File : Def.Files)
		{
			if (Cited.find(File) == std::string::npos)
			{
				T.Fail("docs/licencas.csv não cita Audio/" + File + ".wav");
			}
		}
	}
}
