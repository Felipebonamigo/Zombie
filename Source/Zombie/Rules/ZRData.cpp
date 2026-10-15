#include "Rules/ZRData.h"

#include <algorithm>
#include <initializer_list>

#include "Rules/ZRCsv.h"

namespace ZR
{
	namespace
	{
		/** Lê colunas de uma tabela já analisada, acumulando erros com arquivo, linha e coluna. */
		class FRowReader
		{
		public:
			FRowReader(const FCsvTable& InTable, const char* InFile, std::vector<std::string>& InErrors)
				: Table(InTable), File(InFile), Errors(InErrors)
			{
			}

			bool Require(std::initializer_list<const char*> Columns)
			{
				bool bAll = true;
				for (const char* Column : Columns)
				{
					if (Table.Column(Column) < 0)
					{
						Errors.push_back(std::string(File) + ": falta a coluna '" + Column + "'");
						bAll = false;
					}
				}
				return bAll;
			}

			std::string Text(std::size_t Row, const char* Column) const
			{
				const int Index = Table.Column(Column);
				return Index < 0 ? std::string() : Table.Rows[Row][static_cast<std::size_t>(Index)];
			}

			float Float(std::size_t Row, const char* Column, float Min, float Max, float Fallback)
			{
				float Value = Fallback;
				const std::string Raw = Text(Row, Column);
				if (!ParseFloat(Raw, Value))
				{
					Report(Row, Column, "'" + Raw + "' não é número");
					return Fallback;
				}
				if (Value < Min || Value > Max)
				{
					Report(Row, Column, Raw + " fora da faixa [" + Short(Min) + ", " + Short(Max) + "]");
					return Fallback;
				}
				return Value;
			}

			int Int(std::size_t Row, const char* Column, int Min, int Max, int Fallback)
			{
				int Value = Fallback;
				const std::string Raw = Text(Row, Column);
				if (!ParseInt(Raw, Value) || Value < Min || Value > Max)
				{
					Report(Row, Column, "'" + Raw + "' não é inteiro entre " + std::to_string(Min) + " e " + std::to_string(Max));
					return Fallback;
				}
				return Value;
			}

			bool Bool(std::size_t Row, const char* Column)
			{
				const std::string Raw = Text(Row, Column);
				if (Raw == "1" || Raw == "sim")
				{
					return true;
				}
				if (Raw != "0" && Raw != "nao" && Raw != "não" && !Raw.empty())
				{
					Report(Row, Column, "'" + Raw + "' não é 0 ou 1");
				}
				return false;
			}

			std::uint32_t Color(std::size_t Row, const char* Column, std::uint32_t Fallback)
			{
				std::uint32_t Value = Fallback;
				const std::string Raw = Text(Row, Column);
				if (!ParseColor(Raw, Value))
				{
					Report(Row, Column, "'" + Raw + "' não é cor #RRGGBB");
					return Fallback;
				}
				return Value;
			}

			void Report(std::size_t Row, const char* Column, const std::string& Message)
			{
				Errors.push_back(std::string(File) + " linha " + std::to_string(Table.RowLines[Row]) + ", " + Column + ": " + Message);
			}

			/** Erro de linha inteira (id repetido, id vazio). */
			void ReportRow(std::size_t Row, const std::string& Message)
			{
				Errors.push_back(std::string(File) + " linha " + std::to_string(Table.RowLines[Row]) + ": " + Message);
			}

		private:
			static std::string Short(float Value)
			{
				std::string Text = std::to_string(Value);
				while (!Text.empty() && Text.back() == '0')
				{
					Text.pop_back();
				}
				if (!Text.empty() && Text.back() == '.')
				{
					Text.pop_back();
				}
				return Text;
			}

			const FCsvTable& Table;
			const char* File;
			std::vector<std::string>& Errors;
		};

		bool ParseOrReport(const std::string& Csv, const char* File, FCsvTable& Table, std::vector<std::string>& Errors)
		{
			std::string Error;
			if (!ParseCsv(Csv, Table, Error))
			{
				Errors.push_back(std::string(File) + ": " + Error);
				return false;
			}
			return true;
		}

		/** Id não vazio e sem repetir. */
		template <typename T>
		bool AcceptId(FRowReader& Reader, std::size_t Row, const std::string& Id, const std::vector<T>& Items)
		{
			if (Id.empty())
			{
				Reader.ReportRow(Row, "id vazio");
				return false;
			}
			if (FindById(Items, Id) != nullptr)
			{
				Reader.ReportRow(Row, "id '" + Id + "' repetido");
				return false;
			}
			return true;
		}

		constexpr float Huge = 1.0e9f;
	}

	void LoadZombies(const std::string& Csv, std::vector<FZombieType>& Out, std::vector<std::string>& Errors)
	{
		Out.clear();
		FCsvTable Table;
		if (!ParseOrReport(Csv, "Zumbis.csv", Table, Errors))
		{
			return;
		}
		FRowReader Reader(Table, "Zumbis.csv", Errors);
		if (!Reader.Require({"id", "nome_pt", "vida_cabeca", "vida_tronco", "vida_braco", "vida_perna", "dano_para_despedacar",
				"velocidade_andar", "velocidade_correr", "dano_ataque", "intervalo_ataque_s", "alcance_ataque_cm", "explode",
				"explosao_raio_interno_cm", "explosao_raio_externo_cm", "explosao_dano", "explosao_impulso", "peso_dia",
				"peso_noite", "escala", "cor"}))
		{
			return;
		}
		for (std::size_t Row = 0; Row < Table.Rows.size(); ++Row)
		{
			FZombieType Zombie;
			Zombie.Id = Reader.Text(Row, "id");
			if (!AcceptId(Reader, Row, Zombie.Id, Out))
			{
				continue;
			}
			Zombie.NamePt = Reader.Text(Row, "nome_pt");
			const float Arm = Reader.Float(Row, "vida_braco", 1.0f, Huge, 50.0f);
			const float Leg = Reader.Float(Row, "vida_perna", 1.0f, Huge, 70.0f);
			Zombie.Body.Health[static_cast<int>(ELimb::Head)] = Reader.Float(Row, "vida_cabeca", 1.0f, Huge, 40.0f);
			Zombie.Body.Health[static_cast<int>(ELimb::Torso)] = Reader.Float(Row, "vida_tronco", 1.0f, Huge, 150.0f);
			Zombie.Body.Health[static_cast<int>(ELimb::ArmL)] = Arm;
			Zombie.Body.Health[static_cast<int>(ELimb::ArmR)] = Arm;
			Zombie.Body.Health[static_cast<int>(ELimb::LegL)] = Leg;
			Zombie.Body.Health[static_cast<int>(ELimb::LegR)] = Leg;
			Zombie.Body.GibDamage = Reader.Float(Row, "dano_para_despedacar", 1.0f, Huge, 220.0f);
			Zombie.WalkSpeed = Reader.Float(Row, "velocidade_andar", 10.0f, 2000.0f, 150.0f);
			Zombie.RunSpeed = Reader.Float(Row, "velocidade_correr", 10.0f, 2000.0f, 300.0f);
			if (Zombie.RunSpeed < Zombie.WalkSpeed)
			{
				Reader.Report(Row, "velocidade_correr", "menor que velocidade_andar");
				Zombie.RunSpeed = Zombie.WalkSpeed;
			}
			Zombie.AttackDamage = Reader.Float(Row, "dano_ataque", 0.0f, 1000.0f, 10.0f);
			Zombie.AttackIntervalSeconds = Reader.Float(Row, "intervalo_ataque_s", 0.1f, 30.0f, 1.2f);
			Zombie.AttackRangeCm = Reader.Float(Row, "alcance_ataque_cm", 30.0f, 1000.0f, 140.0f);
			Zombie.bExplodes = Reader.Bool(Row, "explode");
			Zombie.Blast.InnerRadiusCm = Reader.Float(Row, "explosao_raio_interno_cm", 0.0f, 10000.0f, 200.0f);
			Zombie.Blast.OuterRadiusCm = Reader.Float(Row, "explosao_raio_externo_cm", 0.0f, 10000.0f, 600.0f);
			if (Zombie.Blast.OuterRadiusCm < Zombie.Blast.InnerRadiusCm)
			{
				Reader.Report(Row, "explosao_raio_externo_cm", "menor que o raio interno");
				Zombie.Blast.OuterRadiusCm = Zombie.Blast.InnerRadiusCm;
			}
			Zombie.Blast.Damage = Reader.Float(Row, "explosao_dano", 0.0f, Huge, 0.0f);
			Zombie.Blast.Impulse = Reader.Float(Row, "explosao_impulso", 0.0f, Huge, 0.0f);
			Zombie.SpawnWeight = Reader.Float(Row, "peso_dia", 0.0f, 1000.0f, 1.0f);
			Zombie.NightSpawnWeight = Reader.Float(Row, "peso_noite", 0.0f, 1000.0f, 1.0f);
			Zombie.Scale = Reader.Float(Row, "escala", 0.3f, 3.0f, 1.0f);
			Zombie.ColorRgb = Reader.Color(Row, "cor", 0x6B7A5A);
			Out.push_back(Zombie);
		}
	}

	void LoadWeapons(const std::string& Csv, std::vector<FWeaponType>& Out, std::vector<std::string>& Errors)
	{
		Out.clear();
		FCsvTable Table;
		if (!ParseOrReport(Csv, "Armas.csv", Table, Errors))
		{
			return;
		}
		FRowReader Reader(Table, "Armas.csv", Errors);
		if (!Reader.Require({"id", "nome_pt", "tecla", "dano", "tiros_por_s", "automatica", "pente", "recarga_s", "dispersao_graus",
				"projeteis", "alcance_m", "impulso", "barulho", "municao_reserva"}))
		{
			return;
		}
		for (std::size_t Row = 0; Row < Table.Rows.size(); ++Row)
		{
			FWeaponType Weapon;
			Weapon.Id = Reader.Text(Row, "id");
			if (!AcceptId(Reader, Row, Weapon.Id, Out))
			{
				continue;
			}
			Weapon.NamePt = Reader.Text(Row, "nome_pt");
			Weapon.Slot = Reader.Int(Row, "tecla", 1, 9, 1);
			Weapon.Damage = Reader.Float(Row, "dano", 0.0f, Huge, 30.0f);
			Weapon.ShotsPerSecond = Reader.Float(Row, "tiros_por_s", 0.1f, 60.0f, 3.0f);
			Weapon.bAutomatic = Reader.Bool(Row, "automatica");
			Weapon.Magazine = Reader.Int(Row, "pente", 1, 1000, 12);
			Weapon.ReloadSeconds = Reader.Float(Row, "recarga_s", 0.0f, 30.0f, 1.5f);
			Weapon.SpreadDeg = Reader.Float(Row, "dispersao_graus", 0.0f, 45.0f, 1.0f);
			Weapon.Pellets = Reader.Int(Row, "projeteis", 1, 64, 1);
			Weapon.RangeM = Reader.Float(Row, "alcance_m", 1.0f, 2000.0f, 120.0f);
			Weapon.Impulse = Reader.Float(Row, "impulso", 0.0f, Huge, 300.0f);
			Weapon.Noise = Reader.Float(Row, "barulho", 0.0f, 1000.0f, 2.0f);
			Weapon.StartReserve = Reader.Int(Row, "municao_reserva", 0, 100000, 60);
			for (const FWeaponType& Other : Out)
			{
				if (Other.Slot == Weapon.Slot)
				{
					Reader.Report(Row, "tecla", "tecla " + std::to_string(Weapon.Slot) + " já é de '" + Other.Id + "'");
				}
			}
			Out.push_back(Weapon);
		}
	}

	void LoadExplosives(const std::string& Csv, std::vector<FExplosiveType>& Out, std::vector<std::string>& Errors)
	{
		Out.clear();
		FCsvTable Table;
		if (!ParseOrReport(Csv, "Explosivos.csv", Table, Errors))
		{
			return;
		}
		FRowReader Reader(Table, "Explosivos.csv", Errors);
		if (!Reader.Require({"id", "nome_pt", "vida", "pavio_s", "raio_interno_cm", "raio_externo_cm", "dano", "impulso", "barulho", "cor"}))
		{
			return;
		}
		for (std::size_t Row = 0; Row < Table.Rows.size(); ++Row)
		{
			FExplosiveType Explosive;
			Explosive.Id = Reader.Text(Row, "id");
			if (!AcceptId(Reader, Row, Explosive.Id, Out))
			{
				continue;
			}
			Explosive.NamePt = Reader.Text(Row, "nome_pt");
			Explosive.Health = Reader.Float(Row, "vida", 0.1f, Huge, 20.0f);
			Explosive.FuseSeconds = Reader.Float(Row, "pavio_s", 0.0f, 30.0f, 0.8f);
			Explosive.Blast.InnerRadiusCm = Reader.Float(Row, "raio_interno_cm", 0.0f, 10000.0f, 200.0f);
			Explosive.Blast.OuterRadiusCm = Reader.Float(Row, "raio_externo_cm", 0.0f, 10000.0f, 700.0f);
			if (Explosive.Blast.OuterRadiusCm < Explosive.Blast.InnerRadiusCm)
			{
				Reader.Report(Row, "raio_externo_cm", "menor que o raio interno");
				Explosive.Blast.OuterRadiusCm = Explosive.Blast.InnerRadiusCm;
			}
			Explosive.Blast.Damage = Reader.Float(Row, "dano", 0.0f, Huge, 400.0f);
			Explosive.Blast.Impulse = Reader.Float(Row, "impulso", 0.0f, Huge, 1800.0f);
			Explosive.Noise = Reader.Float(Row, "barulho", 0.0f, 1000.0f, 40.0f);
			Explosive.ColorRgb = Reader.Color(Row, "cor", 0xB03A2E);
			Out.push_back(Explosive);
		}
	}

	void LoadHypeEvents(const std::string& Csv, std::vector<FHypeEventDef>& Out, std::vector<std::string>& Errors)
	{
		const std::size_t Count = static_cast<std::size_t>(EHypeEvent::Count);
		Out.assign(Count, FHypeEventDef());
		for (std::size_t i = 0; i < Count; ++i)
		{
			Out[i].Id = HypeEventId(static_cast<EHypeEvent>(i));
		}
		FCsvTable Table;
		if (!ParseOrReport(Csv, "Hype.csv", Table, Errors))
		{
			return;
		}
		FRowReader Reader(Table, "Hype.csv", Errors);
		if (!Reader.Require({"id", "pontos", "rotulo_pt", "rotulo_en"}))
		{
			return;
		}
		std::vector<bool> Seen(Count, false);
		for (std::size_t Row = 0; Row < Table.Rows.size(); ++Row)
		{
			const std::string Id = Reader.Text(Row, "id");
			std::size_t Index = Count;
			for (std::size_t i = 0; i < Count; ++i)
			{
				if (Out[i].Id == Id)
				{
					Index = i;
				}
			}
			if (Index == Count)
			{
				Reader.ReportRow(Row, "evento '" + Id + "' não existe no jogo (EHypeEvent)");
				continue;
			}
			if (Seen[Index])
			{
				Reader.ReportRow(Row, "evento '" + Id + "' repetido");
				continue;
			}
			Seen[Index] = true;
			Out[Index].Points = Reader.Float(Row, "pontos", 0.0f, 100.0f, 0.0f);
			Out[Index].LabelPt = Reader.Text(Row, "rotulo_pt");
			Out[Index].LabelEn = Reader.Text(Row, "rotulo_en");
		}
		for (std::size_t i = 0; i < Count; ++i)
		{
			if (!Seen[i])
			{
				Errors.push_back("Hype.csv: falta o evento '" + Out[i].Id + "'");
			}
		}
	}

	void LoadChatLines(const std::string& Csv, std::vector<FChatLine>& Out, std::vector<std::string>& Errors)
	{
		Out.clear();
		FCsvTable Table;
		if (!ParseOrReport(Csv, "ChatFalso.csv", Table, Errors))
		{
			return;
		}
		FRowReader Reader(Table, "ChatFalso.csv", Errors);
		if (!Reader.Require({"categoria", "texto_pt", "texto_en"}))
		{
			return;
		}
		const std::vector<const char*>& Categories = FakeChatCategories();
		for (std::size_t Row = 0; Row < Table.Rows.size(); ++Row)
		{
			FChatLine Line;
			Line.Category = Reader.Text(Row, "categoria");
			Line.TextPt = Reader.Text(Row, "texto_pt");
			Line.TextEn = Reader.Text(Row, "texto_en");
			const bool bKnown = std::any_of(Categories.begin(), Categories.end(),
				[&Line](const char* Category) { return Line.Category == Category; });
			if (!bKnown)
			{
				Reader.ReportRow(Row, "categoria '" + Line.Category + "' desconhecida");
				continue;
			}
			if (Line.TextPt.empty())
			{
				Reader.ReportRow(Row, "texto_pt vazio");
				continue;
			}
			Out.push_back(Line);
		}
	}

	void LoadChallenges(const std::string& Csv, std::vector<FChallengeDef>& Out, std::vector<std::string>& Errors)
	{
		Out.clear();
		FCsvTable Table;
		if (!ParseOrReport(Csv, "Pautas.csv", Table, Errors))
		{
			return;
		}
		FRowReader Reader(Table, "Pautas.csv", Errors);
		if (!Reader.Require({"id", "texto_pt", "texto_en", "evento", "quantidade", "prazo_s", "recompensa_reais", "hype_bonus", "peso",
				"so_de_noite"}))
		{
			return;
		}
		for (std::size_t Row = 0; Row < Table.Rows.size(); ++Row)
		{
			FChallengeDef Def;
			Def.Id = Reader.Text(Row, "id");
			if (!AcceptId(Reader, Row, Def.Id, Out))
			{
				continue;
			}
			const std::string Event = Reader.Text(Row, "evento");
			Def.EventIndex = -1;
			for (int i = 0; i < static_cast<int>(EHypeEvent::Count); ++i)
			{
				if (Event == HypeEventId(static_cast<EHypeEvent>(i)))
				{
					Def.EventIndex = i;
				}
			}
			if (Def.EventIndex < 0)
			{
				Reader.Report(Row, "evento", "'" + Event + "' não é um evento de Hype.csv");
				continue;
			}
			int Target = 0;
			if (!ParseInt(Reader.Text(Row, "quantidade"), Target) || Target < 1)
			{
				Reader.Report(Row, "quantidade", "tem que ser um inteiro de pelo menos 1");
				continue;
			}
			Def.Target = Target;
			Def.TextPt = Reader.Text(Row, "texto_pt");
			Def.TextEn = Reader.Text(Row, "texto_en");
			Def.TimeLimitSeconds = Reader.Float(Row, "prazo_s", 5.0f, 3600.0f, 60.0f);
			Def.Reward = Reader.Float(Row, "recompensa_reais", 0.0f, 100000.0f, 0.0f);
			Def.HypeBonus = Reader.Float(Row, "hype_bonus", 0.0f, 100.0f, 0.0f);
			Def.Weight = Reader.Float(Row, "peso", 0.0f, 1000.0f, 1.0f);
			Def.bNightOnly = Reader.Bool(Row, "so_de_noite");
			Out.push_back(Def);
		}
	}

	void LoadMilestones(const std::string& Csv, std::vector<FMilestoneDef>& Out, std::vector<std::string>& Errors)
	{
		Out.clear();
		FCsvTable Table;
		if (!ParseOrReport(Csv, "Marcos.csv", Table, Errors))
		{
			return;
		}
		FRowReader Reader(Table, "Marcos.csv", Errors);
		if (!Reader.Require({"id", "seguidores", "texto_pt", "texto_en", "sucata", "granadas", "bujoes", "caixa", "hype_bonus"}))
		{
			return;
		}
		for (std::size_t Row = 0; Row < Table.Rows.size(); ++Row)
		{
			FMilestoneDef Def;
			Def.Id = Reader.Text(Row, "id");
			if (!AcceptId(Reader, Row, Def.Id, Out))
			{
				continue;
			}
			const std::size_t ErrorsBefore = Errors.size();
			Def.Followers = Reader.Float(Row, "seguidores", 1.0f, 1.0e10f, 100.0f);
			if (Errors.size() != ErrorsBefore)
			{
				continue;
			}
			// em ordem crescente: a tabela se lê como a escada da live
			if (!Out.empty() && Def.Followers <= Out.back().Followers)
			{
				Reader.Report(Row, "seguidores", "tem que ser maior que o marco de cima (" + std::to_string(static_cast<long long>(Out.back().Followers)) + ")");
				continue;
			}
			Def.TextPt = Reader.Text(Row, "texto_pt");
			Def.TextEn = Reader.Text(Row, "texto_en");
			Def.Scrap = Reader.Int(Row, "sucata", 0, 100000, 0);
			Def.Grenades = Reader.Int(Row, "granadas", 0, 100, 0);
			Def.Bombs = Reader.Int(Row, "bujoes", 0, 100, 0);
			Def.bAirdrop = Reader.Bool(Row, "caixa");
			Def.HypeBonus = Reader.Float(Row, "hype_bonus", 0.0f, 100.0f, 0.0f);
			Out.push_back(Def);
		}
	}

	void LoadSounds(const std::string& Csv, std::vector<FSoundDef>& Out, std::vector<std::string>& Errors)
	{
		Out.clear();
		FCsvTable Table;
		if (!ParseOrReport(Csv, "Sons.csv", Table, Errors))
		{
			return;
		}
		FRowReader Reader(Table, "Sons.csv", Errors);
		if (!Reader.Require({"id", "arquivos", "volume", "variacao_tom", "alcance_m", "vozes_max", "roubar", "espera_s", "laco", "interface"}))
		{
			return;
		}
		for (std::size_t Row = 0; Row < Table.Rows.size(); ++Row)
		{
			FSoundDef Sound;
			Sound.Id = Reader.Text(Row, "id");
			if (!AcceptId(Reader, Row, Sound.Id, Out))
			{
				continue;
			}
			// "a;b;c": cada nome vira o asset SW_<nome> no Unreal, então só minúsculas, números e "_"
			const std::string Files = Reader.Text(Row, "arquivos");
			bool bBadName = false;
			std::size_t Start = 0;
			while (Start <= Files.size() && !Files.empty())
			{
				const std::size_t End = std::min(Files.find(';', Start), Files.size());
				std::string Name = Files.substr(Start, End - Start);
				while (!Name.empty() && Name.front() == ' ')
				{
					Name.erase(Name.begin());
				}
				while (!Name.empty() && Name.back() == ' ')
				{
					Name.pop_back();
				}
				const bool bValid = !Name.empty() && std::all_of(Name.begin(), Name.end(), [](char C)
				{
					return (C >= 'a' && C <= 'z') || (C >= '0' && C <= '9') || C == '_';
				});
				if (!bValid)
				{
					Reader.Report(Row, "arquivos", "'" + Name + "' não é um nome de arquivo válido (minúsculas, números e _)");
					bBadName = true;
					break;
				}
				Sound.Files.push_back(Name);
				Start = End + 1;
			}
			if (bBadName)
			{
				continue;
			}
			Sound.Volume = Reader.Float(Row, "volume", 0.0f, 4.0f, 1.0f);
			Sound.PitchJitter = Reader.Float(Row, "variacao_tom", 0.0f, 0.5f, 0.05f);
			Sound.RangeCm = Reader.Float(Row, "alcance_m", 1.0f, 2000.0f, 40.0f) * 100.0f;
			Sound.MaxVoices = Reader.Int(Row, "vozes_max", 1, 64, 4);
			Sound.bStealOldest = Reader.Bool(Row, "roubar");
			Sound.CooldownSeconds = Reader.Float(Row, "espera_s", 0.0f, 60.0f, 0.0f);
			Sound.bLoop = Reader.Bool(Row, "laco");
			Sound.b2D = Reader.Bool(Row, "interface");
			Out.push_back(Sound);
		}
	}

	void LoadFilterTerms(const std::string& Csv, std::vector<std::string>& Out, std::vector<std::string>& Errors)
	{
		Out.clear();
		FCsvTable Table;
		if (!ParseOrReport(Csv, "FiltroPalavras.csv", Table, Errors))
		{
			return;
		}
		FRowReader Reader(Table, "FiltroPalavras.csv", Errors);
		if (!Reader.Require({"termo"}))
		{
			return;
		}
		// repetido conta depois de normalizar ("PORRA" e "porra" são o mesmo termo)
		std::vector<std::string> Seen;
		for (std::size_t Row = 0; Row < Table.Rows.size(); ++Row)
		{
			const std::string Term = Reader.Text(Row, "termo");
			const bool bPrefix = !Term.empty() && Term.back() == '*';
			const std::string Stem = bPrefix ? Term.substr(0, Term.size() - 1) : Term;
			if (Stem.find('*') != std::string::npos)
			{
				Reader.Report(Row, "termo", "'" + Term + "': o * só vale no fim (começa com)");
				continue;
			}
			const std::string Key = NormalizeForFilter(Stem) + (bPrefix ? "*" : "");
			if (Key.empty() || Key == "*")
			{
				Reader.Report(Row, "termo", "'" + Term + "' não tem letra");
				continue;
			}
			if (std::find(Seen.begin(), Seen.end(), Key) != Seen.end())
			{
				Reader.Report(Row, "termo", "'" + Term + "' repete outro termo");
				continue;
			}
			Seen.push_back(Key);
			Out.push_back(Term);
		}
	}

	void LoadLootSpots(const std::string& Csv, std::vector<FLootSpotType>& Out, std::vector<std::string>& Errors)
	{
		Out.clear();
		FCsvTable Table;
		if (!ParseOrReport(Csv, "Saque.csv", Table, Errors))
		{
			return;
		}
		FRowReader Reader(Table, "Saque.csv", Errors);
		if (!Reader.Require({"id", "nome_pt", "nome_en", "onde", "busca_s", "rolagens", "barulho", "quantidade", "renova_dias",
				"largura_cm", "profundidade_cm", "altura_cm", "cor"}))
		{
			return;
		}
		for (std::size_t Row = 0; Row < Table.Rows.size(); ++Row)
		{
			const std::size_t ErrorsBefore = Errors.size();
			FLootSpotType Spot;
			Spot.Id = Reader.Text(Row, "id");
			if (!AcceptId(Reader, Row, Spot.Id, Out))
			{
				continue;
			}
			const std::string Where = Reader.Text(Row, "onde");
			if (Where == "rua")
			{
				Spot.Where = ELootWhere::Road;
			}
			else if (Where != "calcada")
			{
				Reader.Report(Row, "onde", "'" + Where + "' não é calcada ou rua");
				continue;
			}
			Spot.NamePt = Reader.Text(Row, "nome_pt");
			Spot.NameEn = Reader.Text(Row, "nome_en");
			Spot.SearchSeconds = Reader.Float(Row, "busca_s", 0.1f, 60.0f, 2.0f);
			Spot.Rolls = Reader.Int(Row, "rolagens", 1, 20, 1);
			Spot.Noise = Reader.Float(Row, "barulho", 0.0f, 100.0f, 0.0f);
			Spot.Count = Reader.Int(Row, "quantidade", 0, 1000, 0);
			Spot.RefillDays = Reader.Int(Row, "renova_dias", 0, 365, 0);
			Spot.WidthCm = Reader.Float(Row, "largura_cm", 10.0f, 2000.0f, 60.0f);
			Spot.DepthCm = Reader.Float(Row, "profundidade_cm", 10.0f, 2000.0f, 60.0f);
			Spot.HeightCm = Reader.Float(Row, "altura_cm", 5.0f, 1000.0f, 100.0f);
			Spot.ColorRgb = Reader.Color(Row, "cor", 0x556655);
			// linha com erro sai inteira: lugar pela metade na cidade confunde mais que lugar faltando
			if (Errors.size() != ErrorsBefore)
			{
				continue;
			}
			Out.push_back(Spot);
		}
	}

	void LoadLootItems(const std::string& Csv, const std::vector<FLootSpotType>& Spots, std::vector<FLootItemDef>& Out,
		std::vector<std::string>& Errors)
	{
		Out.clear();
		FCsvTable Table;
		if (!ParseOrReport(Csv, "SaqueItens.csv", Table, Errors))
		{
			return;
		}
		FRowReader Reader(Table, "SaqueItens.csv", Errors);
		if (!Reader.Require({"local", "item", "peso", "min", "max"}))
		{
			return;
		}
		for (std::size_t Row = 0; Row < Table.Rows.size(); ++Row)
		{
			const std::size_t ErrorsBefore = Errors.size();
			FLootItemDef Def;
			Def.Spot = Reader.Text(Row, "local");
			if (FindById(Spots, Def.Spot) == nullptr)
			{
				Reader.Report(Row, "local", "'" + Def.Spot + "' não é um id de Saque.csv");
				continue;
			}
			const std::string Item = Reader.Text(Row, "item");
			if (!ParseLootItem(Item, Def.Item))
			{
				Reader.Report(Row, "item", "'" + Item + "' não é nada, municao, granada, bujao, curativo ou sucata");
				continue;
			}
			Def.Weight = Reader.Float(Row, "peso", 0.0f, 1.0e6f, 1.0f);
			Def.Min = Reader.Int(Row, "min", 0, 100000, 1);
			Def.Max = Reader.Int(Row, "max", 0, 100000, 1);
			if (Errors.size() != ErrorsBefore)
			{
				continue;
			}
			if (Def.Min > Def.Max)
			{
				Reader.Report(Row, "min", "maior que max");
				continue;
			}
			Out.push_back(Def);
		}
	}

	void LoadBuildPieces(const std::string& Csv, const std::vector<FExplosiveType>& Explosives, std::vector<FBuildPieceType>& Out,
		std::vector<std::string>& Errors)
	{
		Out.clear();
		FCsvTable Table;
		if (!ParseOrReport(Csv, "Construcoes.csv", Table, Errors))
		{
			return;
		}
		FRowReader Reader(Table, "Construcoes.csv", Errors);
		if (!Reader.Require({"id", "nome_pt", "nome_en", "tipo", "tecla", "custo_sucata", "vida", "largura_cm", "profundidade_cm",
				"altura_cm", "dano_por_s", "alvo", "lentidao", "desgaste", "explosivo", "cor"}))
		{
			return;
		}
		for (std::size_t Row = 0; Row < Table.Rows.size(); ++Row)
		{
			// uma linha com erro sai inteira: peça pela metade no modo construção confunde mais que peça faltando
			const std::size_t ErrorsBefore = Errors.size();
			FBuildPieceType Piece;
			Piece.Id = Reader.Text(Row, "id");
			if (!AcceptId(Reader, Row, Piece.Id, Out))
			{
				continue;
			}
			const std::string Kind = Reader.Text(Row, "tipo");
			if (Kind == "barricada")
			{
				Piece.Kind = EPieceKind::Barricade;
			}
			else if (Kind == "armadilha")
			{
				Piece.Kind = EPieceKind::Trap;
			}
			else
			{
				Reader.Report(Row, "tipo", "'" + Kind + "' não é barricada ou armadilha");
				continue;
			}
			Piece.Slot = Reader.Int(Row, "tecla", 1, 9, 1);
			if (Errors.size() != ErrorsBefore)
			{
				continue;
			}
			bool bSlotTaken = false;
			for (const FBuildPieceType& Other : Out)
			{
				bSlotTaken = bSlotTaken || Other.Slot == Piece.Slot;
			}
			if (bSlotTaken)
			{
				Reader.Report(Row, "tecla", std::to_string(Piece.Slot) + " já é de outra peça");
				continue;
			}
			const std::string Target = Reader.Text(Row, "alvo");
			if (Target == "corpo")
			{
				Piece.Target = ETrapTarget::Body;
			}
			else if (Target != "pernas" && !Target.empty())
			{
				Reader.Report(Row, "alvo", "'" + Target + "' não é pernas ou corpo");
				continue;
			}
			Piece.ExplosiveId = Reader.Text(Row, "explosivo");
			if (Piece.IsExplosive() && FindById(Explosives, Piece.ExplosiveId) == nullptr)
			{
				Reader.Report(Row, "explosivo", "'" + Piece.ExplosiveId + "' não é um id de Explosivos.csv");
				continue;
			}
			Piece.NamePt = Reader.Text(Row, "nome_pt");
			Piece.NameEn = Reader.Text(Row, "nome_en");
			Piece.Cost = Reader.Int(Row, "custo_sucata", 0, 10000, 20);
			Piece.Health = Reader.Float(Row, "vida", 1.0f, 1.0e6f, 400.0f);
			Piece.WidthCm = Reader.Float(Row, "largura_cm", 10.0f, 2000.0f, 300.0f);
			Piece.DepthCm = Reader.Float(Row, "profundidade_cm", 10.0f, 2000.0f, 30.0f);
			Piece.HeightCm = Reader.Float(Row, "altura_cm", 1.0f, 1000.0f, 110.0f);
			Piece.DamagePerSecond = Reader.Float(Row, "dano_por_s", 0.0f, 1.0e5f, 0.0f);
			Piece.SlowFactor = Reader.Float(Row, "lentidao", 0.05f, 1.0f, 1.0f);
			Piece.Durability = Reader.Float(Row, "desgaste", 0.0f, 1.0e6f, 0.0f);
			Piece.ColorRgb = Reader.Color(Row, "cor", 0x7A5230);
			if (Errors.size() != ErrorsBefore)
			{
				continue;
			}
			if (Piece.Kind == EPieceKind::Trap && !Piece.IsExplosive() && Piece.DamagePerSecond <= 0.0f && Piece.SlowFactor >= 1.0f)
			{
				Reader.ReportRow(Row, "armadilha sem efeito: precisa de dano_por_s, lentidao abaixo de 1 ou explosivo");
				continue;
			}
			Out.push_back(Piece);
		}
	}

	namespace
	{
		struct FTuningKey
		{
			const char* Name;
			float* FloatValue;
			int* IntValue;
			double* DoubleValue;
			float Min;
			float Max;
		};

		std::vector<FTuningKey> BindTuning(FTuning& T)
		{
			std::vector<FTuningKey> Keys;
			auto F = [&Keys](const char* Name, float& Value, float Min, float Max) { Keys.push_back(FTuningKey{Name, &Value, nullptr, nullptr, Min, Max}); };
			auto I = [&Keys](const char* Name, int& Value, float Min, float Max) { Keys.push_back(FTuningKey{Name, nullptr, &Value, nullptr, Min, Max}); };
			auto D = [&Keys](const char* Name, double& Value, float Min, float Max) { Keys.push_back(FTuningKey{Name, nullptr, nullptr, &Value, Min, Max}); };

			F("hype.combo_janela_s", T.Hype.ComboWindowSeconds, 0.1f, 60.0f);
			F("hype.combo_passo", T.Hype.ComboStep, 0.0f, 10.0f);
			F("hype.combo_multiplicador_max", T.Hype.ComboMaxMultiplier, 1.0f, 50.0f);
			F("hype.variedade_queda", T.Hype.VarietyFalloff, 0.0f, 1.0f);
			F("hype.variedade_min", T.Hype.VarietyMin, 0.0f, 1.0f);
			F("hype.carencia_s", T.Hype.DecayGraceSeconds, 0.0f, 600.0f);
			F("hype.queda_por_s", T.Hype.DecayPerSecond, 0.0f, 100.0f);
			F("hype.espectadores_piso", T.Hype.ViewerFloor, 0.0f, 1.0f);
			F("hype.espectadores_expoente", T.Hype.ViewerExponent, 0.1f, 10.0f);
			F("hype.viral_inicio", T.Hype.ViralStartHype, 0.0f, 99.0f);
			F("hype.viral_bonus", T.Hype.ViralBoost, 0.0f, 100.0f);
			F("hype.espectadores_subida_s", T.Hype.ViewersRiseSeconds, 0.0f, 600.0f);
			F("hype.espectadores_descida_s", T.Hype.ViewersFallSeconds, 0.0f, 600.0f);
			F("hype.seguidores_taxa_por_s", T.Hype.FollowRatePerSecond, 0.0f, 10.0f);
			F("hype.seguidores_teto", T.Hype.FollowerCap, 1.0f, 1.0e10f);
			F("hype.seguidores_inicio", T.Hype.StartFollowers, 0.0f, 1.0e10f);
			F("hype.doacao_rearme", T.Hype.DonationRearm, 0.0f, 100.0f);
			F("hype.doacao_por_espectador", T.Hype.DonationPerViewer, 0.0f, 100.0f);

			F("diretor.intensidade_pico", T.Director.PeakIntensity, 0.0f, 100.0f);
			F("diretor.intensidade_descanso", T.Director.RelaxIntensity, 0.0f, 100.0f);
			F("diretor.tensao_max_s", T.Director.MaxBuildUpSeconds, 1.0f, 3600.0f);
			F("diretor.pico_s", T.Director.PeakSeconds, 1.0f, 3600.0f);
			F("diretor.descanso_s", T.Director.RelaxSeconds, 0.0f, 3600.0f);
			F("diretor.intensidade_queda_por_s", T.Director.IntensityDecayPerSecond, 0.0f, 100.0f);
			F("diretor.intensidade_por_dano", T.Director.DamageToIntensity, 0.0f, 100.0f);
			F("diretor.intensidade_por_abate", T.Director.KillNearToIntensity, 0.0f, 100.0f);
			F("diretor.intensidade_por_explosao", T.Director.ExplosionToIntensity, 0.0f, 100.0f);
			F("diretor.nascer_tensao_por_s", T.Director.BuildUpSpawnPerSecond, 0.0f, 100.0f);
			F("diretor.nascer_pico_por_s", T.Director.PeakSpawnPerSecond, 0.0f, 100.0f);
			F("diretor.nascer_descanso_por_s", T.Director.RelaxSpawnPerSecond, 0.0f, 100.0f);
			F("diretor.multiplicador_noite", T.Director.NightMultiplier, 0.0f, 100.0f);
			F("diretor.multiplicador_horda", T.Director.HordeMultiplier, 0.0f, 100.0f);
			I("diretor.vivos_max", T.Director.MaxAlive, 0.0f, 5000.0f);
			I("diretor.vivos_max_noite", T.Director.MaxAliveNight, 0.0f, 5000.0f);
			F("diretor.barulho_queda_por_s", T.Director.NoiseDecayPerSecond, 0.0f, 1000.0f);
			F("diretor.barulho_para_nascer", T.Director.NoiseToSpawn, 0.0f, 10.0f);

			I("cidade.largura_celulas", T.City.Width, 5.0f, 200.0f);
			I("cidade.altura_celulas", T.City.Height, 8.0f, 200.0f);
			I("cidade.quarteirao_celulas", T.City.BlockCells, 1.0f, 20.0f);
			I("cidade.praia_linhas", T.City.BeachRows, 0.0f, 20.0f);
			I("cidade.mar_linhas", T.City.SeaRows, 0.0f, 20.0f);
			D("cidade.chance_praca", T.City.PlazaChance, 0.0f, 1.0f);
			I("cidade.andares_min", T.City.MinFloors, 1.0f, 100.0f);
			I("cidade.andares_max", T.City.MaxFloors, 1.0f, 100.0f);
			F("cidade.celula_cm", T.City.CellSizeCm, 500.0f, 10000.0f);
			F("cidade.recuo_min_cm", T.City.MinInsetCm, 0.0f, 4000.0f);
			F("cidade.recuo_max_cm", T.City.MaxInsetCm, 0.0f, 4000.0f);

			F("chat.rodada_s", T.Vote.RoundSeconds, 1.0f, 3600.0f);
			I("chat.votos_min", T.Vote.MinVotes, 1.0f, 100000.0f);
			F("chat.zumbi_espera_s", T.Vote.ZombieCooldownSeconds, 0.0f, 3600.0f);
			I("chat.zumbis_com_nome_max", T.MaxNamedZombies, 0.0f, 500.0f);

			F("pauta.primeira_espera_s", T.Challenge.FirstDelaySeconds, 0.0f, 3600.0f);
			F("pauta.intervalo_s", T.Challenge.CooldownSeconds, 0.0f, 3600.0f);
			F("pauta.espectadores_min", T.Challenge.MinViewers, 0.0f, 1.0e9f);

			F("resumo.nota_c_espectadores", T.Grade.CViewers, 0.0f, 1.0e9f);
			F("resumo.nota_b_espectadores", T.Grade.BViewers, 0.0f, 1.0e9f);
			F("resumo.nota_a_espectadores", T.Grade.AViewers, 0.0f, 1.0e9f);
			F("resumo.nota_s_espectadores", T.Grade.SViewers, 0.0f, 1.0e9f);

			I("chat.filtro", T.ChatFilterOn, 0.0f, 1.0f);

			F("saque.alcance_cm", T.Loot.RangeCm, 20.0f, 2000.0f);
			F("saque.espacamento_cm", T.Loot.MinSpacingCm, 0.0f, 20000.0f);
			F("saque.longe_do_comeco_cm", T.Loot.AvoidStartCm, 0.0f, 100000.0f);

			I("base.sucata_inicio", T.Base.StartScrap, 0.0f, 100000.0f);
			I("base.sucata_por_caixa", T.Base.ScrapPerCrate, 0.0f, 100000.0f);
			I("base.sucata_por_carro", T.Base.ScrapPerWreck, 0.0f, 100000.0f);
			F("base.desmanchar_s", T.Base.DismantleSeconds, 0.1f, 60.0f);
			F("base.devolve_fracao", T.Base.RefundFraction, 0.0f, 1.0f);
			F("base.conserto_custo_fracao", T.Base.RepairCostFraction, 0.0f, 10.0f);
			F("base.alcance_cm", T.Base.BuildRangeCm, 100.0f, 5000.0f);
			F("base.grade_cm", T.Base.SnapCm, 0.0f, 1000.0f);
			I("base.pecas_max", T.Base.MaxPieces, 0.0f, 1000.0f);
			F("base.folga_jogador_cm", T.Base.PlayerClearanceCm, 0.0f, 500.0f);
			F("base.dano_zumbi_multiplicador", T.Base.ZombieDamageMultiplier, 0.0f, 100.0f);
			F("base.dano_explosao_multiplicador", T.Base.BlastDamageMultiplier, 0.0f, 100.0f);
			F("base.barulho_construir", T.Base.BuildNoise, 0.0f, 1000.0f);
			F("base.dano_carro_multiplicador", T.Base.VehicleDamageMultiplier, 0.0f, 100.0f);
			// a grade fina vai a no máximo 256 x 256 células (12800 / 50): o Dijkstra fica em milissegundos e o custo cabe num int
			F("base.horda_grade_cm", T.HordeGrid.SizeCm, 1000.0f, 12800.0f);
			F("base.horda_resolucao_cm", T.HordeGrid.ResolutionCm, 50.0f, 500.0f);
			F("base.horda_raio_zumbi_cm", T.HordeGrid.InflateCm, 0.0f, 200.0f);
			I("base.horda_custo_barricada", T.HordeGrid.BarricadeCost, 0.0f, 10000.0f);

			F("dia.duracao_s", T.Day.DayLengthSeconds, 10.0f, 86400.0f);
			F("dia.hora_inicio", T.Day.StartHour, 0.0f, 23.99f);
			F("dia.noite_comeca", T.Day.NightStartHour, 0.0f, 23.99f);
			F("dia.noite_termina", T.Day.NightEndHour, 0.0f, 23.99f);
			I("dia.noite_da_horda_a_cada", T.Day.HordeEveryNights, 0.0f, 100.0f);
			F("dia.sol_altura_max_graus", T.Day.MaxSunElevationDeg, 1.0f, 90.0f);

			F("camera.perto_distancia_cm", T.Camera.NearDistanceCm, 50.0f, 5000.0f);
			F("camera.perto_altura_cm", T.Camera.NearHeightCm, -500.0f, 5000.0f);
			F("camera.perto_lado_cm", T.Camera.NearSideCm, -500.0f, 500.0f);
			F("camera.perto_inclinacao_graus", T.Camera.NearPitchDeg, -89.0f, 30.0f);
			F("camera.media_distancia_cm", T.Camera.MidDistanceCm, 50.0f, 5000.0f);
			F("camera.media_altura_cm", T.Camera.MidHeightCm, -500.0f, 5000.0f);
			F("camera.media_lado_cm", T.Camera.MidSideCm, -500.0f, 500.0f);
			F("camera.media_inclinacao_graus", T.Camera.MidPitchDeg, -89.0f, 30.0f);
			F("camera.longe_distancia_cm", T.Camera.FarDistanceCm, 50.0f, 5000.0f);
			F("camera.longe_altura_cm", T.Camera.FarHeightCm, -500.0f, 5000.0f);
			F("camera.longe_lado_cm", T.Camera.FarSideCm, -500.0f, 500.0f);
			F("camera.longe_inclinacao_graus", T.Camera.FarPitchDeg, -89.0f, 30.0f);
			F("camera.mira_distancia_cm", T.Camera.AimDistanceCm, 50.0f, 5000.0f);
			F("camera.mira_lado_cm", T.Camera.AimSideCm, -500.0f, 500.0f);
			F("camera.mira_fov", T.Camera.AimFov, 10.0f, 170.0f);
			F("camera.fov", T.Camera.Fov, 10.0f, 170.0f);
			F("camera.primeira_pessoa_fov", T.Camera.FirstPersonFov, 10.0f, 170.0f);
			F("camera.atraso_velocidade", T.Camera.LagSpeed, 0.0f, 100.0f);
			I("camera.modo_inicial", T.Camera.StartMode, 0.0f, 3.0f);

			F("jogador.vida", T.Player.Health, 1.0f, 100000.0f);
			F("jogador.velocidade_andar", T.Player.WalkSpeed, 10.0f, 5000.0f);
			F("jogador.velocidade_correr", T.Player.SprintSpeed, 10.0f, 5000.0f);
			F("jogador.folego", T.Player.Stamina, 1.0f, 10000.0f);
			F("jogador.folego_gasto_por_s", T.Player.StaminaDrainPerSecond, 0.0f, 1000.0f);
			F("jogador.folego_volta_por_s", T.Player.StaminaRegenPerSecond, 0.0f, 1000.0f);
			F("jogador.chute_dano", T.Player.KickDamage, 0.0f, 10000.0f);
			F("jogador.chute_impulso", T.Player.KickImpulse, 0.0f, 100000.0f);
			F("jogador.chute_alcance_cm", T.Player.KickRangeCm, 10.0f, 1000.0f);
			F("jogador.mira_lenta_tempo", T.Player.SlowMoTimeDilation, 0.05f, 1.0f);
			F("jogador.foco_max", T.Player.FocusMax, 1.0f, 10000.0f);
			F("jogador.foco_gasto_por_s", T.Player.FocusDrainPerSecond, 0.0f, 1000.0f);
			F("jogador.foco_por_tiro_na_cabeca", T.Player.FocusPerHeadshot, 0.0f, 1000.0f);
			F("jogador.foco_volta_por_s", T.Player.FocusRegenPerSecond, 0.0f, 1000.0f);
			F("jogador.arremesso_velocidade", T.Player.ThrowSpeed, 100.0f, 10000.0f);
			I("jogador.granadas_inicio", T.Player.StartGrenades, 0.0f, 1000.0f);
			I("jogador.bujoes_inicio", T.Player.StartBombs, 0.0f, 1000.0f);
			F("jogador.escapou_por_pouco_vida", T.Player.CloseCallHealth, 0.0f, 100000.0f);
			F("jogador.cura_por_s", T.Player.RegenPerSecond, 0.0f, 1000.0f);
			F("jogador.cura_espera_s", T.Player.RegenDelaySeconds, 0.0f, 600.0f);
			F("jogador.cura_teto", T.Player.RegenCap, 0.0f, 100000.0f);
			F("jogador.renascer_s", T.Player.RespawnSeconds, 1.0f, 60.0f);

			F("carro.massa_kg", T.Car.MassKg, 100.0f, 50000.0f);
			F("carro.suspensao_cm", T.Car.SuspensionRestCm, 5.0f, 300.0f);
			F("carro.mola_por_cm", T.Car.SpringPerCm, 1.0f, 100000.0f);
			F("carro.amortecedor", T.Car.DamperPerCmS, 0.0f, 100000.0f);
			F("carro.motor_forca", T.Car.EngineForce, 0.0f, 1.0e9f);
			F("carro.freio_forca", T.Car.BrakeForce, 0.0f, 1.0e9f);
			F("carro.velocidade_max_kmh", T.Car.MaxSpeedKmh, 1.0f, 500.0f);
			F("carro.esterco_graus", T.Car.SteerDeg, 1.0f, 80.0f);
			F("carro.aderencia_lateral", T.Car.SideGrip, 0.0f, 100.0f);
			F("carro.atropelo_dano_por_kmh", T.Car.RunOverDamagePerKmh, 0.0f, 1000.0f);
			F("carro.atropelo_min_kmh", T.Car.RunOverMinKmh, 0.0f, 500.0f);

			F("mundo.cadaver_s", T.CorpseSeconds, 0.0f, 3600.0f);
			I("mundo.pedacos_max", T.MaxGibs, 0.0f, 10000.0f);
			F("mundo.sumir_distancia_cm", T.DespawnDistanceCm, 100.0f, 1.0e6f);
			F("mundo.nascer_min_cm", T.SpawnMinDistanceCm, 100.0f, 1.0e6f);
			F("mundo.nascer_max_cm", T.SpawnMaxDistanceCm, 100.0f, 1.0e6f);
			I("mundo.explosivos_na_rua", T.StreetExplosives, 0.0f, 10000.0f);
			I("mundo.carros_na_rua", T.StreetCars, 0.0f, 10000.0f);
			return Keys;
		}
	}

	std::vector<std::string> TuningKeys()
	{
		FTuning Scratch;
		std::vector<std::string> Names;
		for (const FTuningKey& Key : BindTuning(Scratch))
		{
			Names.push_back(Key.Name);
		}
		return Names;
	}

	void LoadTuning(const std::string& Csv, FTuning& Out, std::vector<std::string>& Errors, std::vector<std::string>& Missing)
	{
		std::vector<FTuningKey> Keys = BindTuning(Out);
		std::vector<bool> Seen(Keys.size(), false);
		FCsvTable Table;
		if (ParseOrReport(Csv, "Ajustes.csv", Table, Errors))
		{
			FRowReader Reader(Table, "Ajustes.csv", Errors);
			if (Reader.Require({"chave", "valor", "unidade", "descricao"}))
			{
				for (std::size_t Row = 0; Row < Table.Rows.size(); ++Row)
				{
					const std::string Name = Reader.Text(Row, "chave");
					std::size_t Index = Keys.size();
					for (std::size_t i = 0; i < Keys.size(); ++i)
					{
						if (Name == Keys[i].Name)
						{
							Index = i;
						}
					}
					if (Index == Keys.size())
					{
						Reader.ReportRow(Row, "chave '" + Name + "' desconhecida");
						continue;
					}
					if (Seen[Index])
					{
						Reader.ReportRow(Row, "chave '" + Name + "' repetida");
						continue;
					}
					Seen[Index] = true;
					FTuningKey& Key = Keys[Index];
					if (Key.FloatValue != nullptr)
					{
						*Key.FloatValue = Reader.Float(Row, "valor", Key.Min, Key.Max, *Key.FloatValue);
					}
					else if (Key.IntValue != nullptr)
					{
						*Key.IntValue = Reader.Int(Row, "valor", static_cast<int>(Key.Min), static_cast<int>(Key.Max), *Key.IntValue);
					}
					else if (Key.DoubleValue != nullptr)
					{
						const float Value = Reader.Float(Row, "valor", Key.Min, Key.Max, static_cast<float>(*Key.DoubleValue));
						*Key.DoubleValue = static_cast<double>(Value);
					}
				}
			}
		}
		for (std::size_t i = 0; i < Keys.size(); ++i)
		{
			if (!Seen[i])
			{
				Missing.push_back(Keys[i].Name);
			}
		}
	}
}
