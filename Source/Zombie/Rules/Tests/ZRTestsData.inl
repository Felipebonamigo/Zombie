// Testes: leitor de CSV, tabelas tipadas e deriva (as tabelas de verdade em Data/ carregam sem erro).
// Contrato: docs/regras-de-jogo.md, seção 10, e docs/dados.md.

#include <fstream>
#include <sstream>

#include "Rules/ZRCsv.h"
#include "Rules/ZRData.h"

namespace
{
	bool ReadDataFile(ZRTest::FContext& T, const char* Name, std::string& Out)
	{
		const std::string Path = T.DataDir + "/" + Name;
		std::ifstream File(Path, std::ios::binary);
		if (!File)
		{
			T.Fail(std::string("não abri ") + Path);
			return false;
		}
		std::ostringstream Buffer;
		Buffer << File.rdbuf();
		Out = Buffer.str();
		return true;
	}

	void ExpectNoErrors(ZRTest::FContext& T, const char* What, const std::vector<std::string>& Errors)
	{
		for (const std::string& Error : Errors)
		{
			T.Fail(std::string(What) + ": " + Error);
		}
	}
}

ZR_TEST(CsvAspasEQuebras, "Csv.AspasVirgulasEQuebrasDeLinha")
{
	ZR::FCsvTable Table;
	std::string Error;
	const std::string Text = "\xEF\xBB\xBF" "id,texto,valor\r\n"
		"a,\"oi, tudo bem\",1\r\n"
		"b,\"ele disse \"\"boom\"\"\",2\n"
		"c,\"duas\nlinhas\",3\n"
		"\n";
	ZR_EXPECT(ZR::ParseCsv(Text, Table, Error));
	ZR_EXPECT(Error.empty());
	ZR_EQ(static_cast<int>(Table.Header.size()), 3);
	ZR_EXPECT(Table.Header.size() == 3 && Table.Header[0] == "id");
	ZR_EQ(static_cast<int>(Table.Rows.size()), 3);
	if (Table.Rows.size() == 3)
	{
		ZR_EXPECT(Table.Rows[0][1] == "oi, tudo bem");
		ZR_EXPECT(Table.Rows[1][1] == "ele disse \"boom\"");
		ZR_EXPECT(Table.Rows[2][1] == "duas\nlinhas");
	}
	ZR_EQ(Table.Column("valor"), 2);
	ZR_EQ(Table.Column("nada"), -1);
}

ZR_TEST(CsvErros, "Csv.AspaAbertaELinhaTortaSaoErro")
{
	ZR::FCsvTable Table;
	std::string Error;
	ZR_EXPECT(!ZR::ParseCsv("a,b\n1,\"aberta\n", Table, Error));
	ZR_EXPECT(!Error.empty());
	Error.clear();
	ZR_EXPECT(!ZR::ParseCsv("a,b\n1,2,3\n", Table, Error));
	ZR_EXPECT(Error.find("linha 2") != std::string::npos);
	Error.clear();
	ZR_EXPECT(!ZR::ParseCsv("", Table, Error));
}

ZR_TEST(CsvNumeros, "Csv.NumerosComPontoEVirgulaDecimal")
{
	float Value = 0.0f;
	ZR_EXPECT(ZR::ParseFloat("3.5", Value));
	ZR_NEAR(Value, 3.5f, 1e-6);
	ZR_EXPECT(ZR::ParseFloat(" -2,25 ", Value));
	ZR_NEAR(Value, -2.25f, 1e-6);
	ZR_EXPECT(!ZR::ParseFloat("abc", Value));
	ZR_EXPECT(!ZR::ParseFloat("1.5x", Value));
	ZR_EXPECT(!ZR::ParseFloat("", Value));
	int Integer = 0;
	ZR_EXPECT(ZR::ParseInt("42", Integer));
	ZR_EQ(Integer, 42);
	ZR_EXPECT(!ZR::ParseInt("4.2", Integer));
	std::uint32_t Color = 0;
	ZR_EXPECT(ZR::ParseColor("#8a9a5b", Color));
	ZR_EQ(Color, 0x8A9A5Bu);
	ZR_EXPECT(!ZR::ParseColor("8a9a5b", Color));
	ZR_EXPECT(!ZR::ParseColor("#12345", Color));
}

ZR_TEST(DadosZumbisFaltandoColuna, "Dados.ZumbiSemColunaEhErro")
{
	std::vector<ZR::FZombieType> Zombies;
	std::vector<std::string> Errors;
	ZR::LoadZombies("id,nome_pt\ncomum,Comum\n", Zombies, Errors);
	ZR_EXPECT(!Errors.empty());
}

ZR_TEST(AjustesChaveDesconhecida, "Dados.AjusteDesconhecidoOuRepetidoEhErro")
{
	ZR::FTuning Tuning;
	std::vector<std::string> Errors;
	std::vector<std::string> Missing;
	ZR::LoadTuning("chave,valor,unidade,descricao\nhype.combo_janela_s,6,s,x\nnao.existe,1,,x\nhype.combo_janela_s,7,s,x\n",
		Tuning, Errors, Missing);
	ZR_EQ(static_cast<int>(Errors.size()), 2);
	ZR_NEAR(Tuning.Hype.ComboWindowSeconds, 6.0f, 1e-6);
	ZR_EXPECT(!Missing.empty());
}

ZR_TEST(AjustesValorRuim, "Dados.AjusteComValorRuimMantemOPadrao")
{
	ZR::FTuning Tuning;
	const float Default = Tuning.Hype.ComboWindowSeconds;
	std::vector<std::string> Errors;
	std::vector<std::string> Missing;
	ZR::LoadTuning("chave,valor,unidade,descricao\nhype.combo_janela_s,muito,s,x\n", Tuning, Errors, Missing);
	ZR_EQ(static_cast<int>(Errors.size()), 1);
	ZR_NEAR(Tuning.Hype.ComboWindowSeconds, Default, 1e-6);
}

ZR_TEST(DerivaAjustes, "Deriva.AjustesTemTodasAsChavesESoElas")
{
	std::string Text;
	if (!ReadDataFile(T, "Ajustes.csv", Text))
	{
		return;
	}
	ZR::FTuning Tuning;
	std::vector<std::string> Errors;
	std::vector<std::string> Missing;
	ZR::LoadTuning(Text, Tuning, Errors, Missing);
	ExpectNoErrors(T, "Ajustes.csv", Errors);
	for (const std::string& Key : Missing)
	{
		T.Fail("Ajustes.csv sem a chave " + Key);
	}
	ZR_EXPECT(Tuning.City.Width >= 8 && Tuning.City.Height >= 8);
	ZR_EXPECT(Tuning.Director.MaxAlive > 0 && Tuning.Director.MaxAliveNight >= Tuning.Director.MaxAlive);
	ZR_EXPECT(Tuning.Day.NightStartHour > Tuning.Day.NightEndHour);
	ZR_EXPECT(Tuning.Camera.FarDistanceCm > Tuning.Camera.NearDistanceCm);
}

ZR_TEST(DerivaZumbis, "Deriva.ZumbisCarregamEFazemSentido")
{
	std::string Text;
	if (!ReadDataFile(T, "Zumbis.csv", Text))
	{
		return;
	}
	std::vector<ZR::FZombieType> Zombies;
	std::vector<std::string> Errors;
	ZR::LoadZombies(Text, Zombies, Errors);
	ExpectNoErrors(T, "Zumbis.csv", Errors);
	ZR_EXPECT(Zombies.size() >= 5);
	ZR_EXPECT(ZR::FindById(Zombies, "comum") != nullptr);
	ZR_EXPECT(ZR::FindById(Zombies, "bujao") != nullptr);
	const ZR::FZombieType* Bomb = ZR::FindById(Zombies, "bujao");
	ZR_EXPECT(Bomb != nullptr && Bomb->bExplodes && Bomb->Blast.OuterRadiusCm > Bomb->Blast.InnerRadiusCm);
	float TotalWeight = 0.0f;
	for (const ZR::FZombieType& Zombie : Zombies)
	{
		TotalWeight += Zombie.SpawnWeight;
		ZR_EXPECT(Zombie.RunSpeed >= Zombie.WalkSpeed);
		ZR_EXPECT(Zombie.Scale > 0.3f && Zombie.Scale < 3.0f);
	}
	ZR_EXPECT(TotalWeight > 0.0f);
}

ZR_TEST(DerivaArmas, "Deriva.ArmasCarregam")
{
	std::string Text;
	if (!ReadDataFile(T, "Armas.csv", Text))
	{
		return;
	}
	std::vector<ZR::FWeaponType> Weapons;
	std::vector<std::string> Errors;
	ZR::LoadWeapons(Text, Weapons, Errors);
	ExpectNoErrors(T, "Armas.csv", Errors);
	ZR_EXPECT(Weapons.size() >= 3);
	for (const ZR::FWeaponType& Weapon : Weapons)
	{
		ZR_EXPECT(Weapon.Magazine > 0 && Weapon.ShotsPerSecond > 0.0f && Weapon.Pellets >= 1);
	}
}

ZR_TEST(DerivaExplosivos, "Deriva.ExplosivosCarregam")
{
	std::string Text;
	if (!ReadDataFile(T, "Explosivos.csv", Text))
	{
		return;
	}
	std::vector<ZR::FExplosiveType> Explosives;
	std::vector<std::string> Errors;
	ZR::LoadExplosives(Text, Explosives, Errors);
	ExpectNoErrors(T, "Explosivos.csv", Errors);
	ZR_EXPECT(ZR::FindById(Explosives, "bujao") != nullptr);
	ZR_EXPECT(ZR::FindById(Explosives, "granada") != nullptr);
}

ZR_TEST(DerivaHype, "Deriva.TodoEventoDeHypeTemLinha")
{
	std::string Text;
	if (!ReadDataFile(T, "Hype.csv", Text))
	{
		return;
	}
	std::vector<ZR::FHypeEventDef> Events;
	std::vector<std::string> Errors;
	ZR::LoadHypeEvents(Text, Events, Errors);
	ExpectNoErrors(T, "Hype.csv", Errors);
	ZR_EQ(static_cast<int>(Events.size()), static_cast<int>(ZR::EHypeEvent::Count));
}

ZR_TEST(DerivaChatFalso, "Deriva.ChatFalsoTemTodasAsCategorias")
{
	std::string Text;
	if (!ReadDataFile(T, "ChatFalso.csv", Text))
	{
		return;
	}
	std::vector<ZR::FChatLine> Lines;
	std::vector<std::string> Errors;
	ZR::LoadChatLines(Text, Lines, Errors);
	ExpectNoErrors(T, "ChatFalso.csv", Errors);
	for (const char* Category : ZR::FakeChatCategories())
	{
		int Count = 0;
		for (const ZR::FChatLine& Line : Lines)
		{
			Count += Line.Category == Category ? 1 : 0;
		}
		if (Count < 2)
		{
			T.Fail(std::string("ChatFalso.csv com menos de 2 falas na categoria ") + Category);
		}
	}
}
