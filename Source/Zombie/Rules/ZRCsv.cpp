#include "Rules/ZRCsv.h"

#include <cerrno>
#include <cstdlib>

namespace ZR
{
	namespace
	{
		std::string Trimmed(const std::string& Text)
		{
			const std::size_t First = Text.find_first_not_of(" \t\r\n");
			if (First == std::string::npos)
			{
				return std::string();
			}
			const std::size_t Last = Text.find_last_not_of(" \t\r\n");
			return Text.substr(First, Last - First + 1);
		}

		bool IsBlankRow(const std::vector<std::string>& Row)
		{
			for (const std::string& Field : Row)
			{
				if (!Trimmed(Field).empty())
				{
					return false;
				}
			}
			return true;
		}
	}

	int FCsvTable::Column(const std::string& Name) const
	{
		for (std::size_t i = 0; i < Header.size(); ++i)
		{
			if (Header[i] == Name)
			{
				return static_cast<int>(i);
			}
		}
		return -1;
	}

	bool ParseCsv(const std::string& Text, FCsvTable& Out, std::string& Error)
	{
		Out = FCsvTable();
		std::size_t Pos = 0;
		// BOM do UTF-8 (o Excel grava)
		if (Text.size() >= 3 && static_cast<unsigned char>(Text[0]) == 0xEF && static_cast<unsigned char>(Text[1]) == 0xBB
			&& static_cast<unsigned char>(Text[2]) == 0xBF)
		{
			Pos = 3;
		}

		std::vector<std::vector<std::string>> Records;
		std::vector<int> RecordLines;
		std::vector<std::string> Record;
		std::string Field;
		bool bInQuotes = false;
		bool bFieldStarted = false;
		int Line = 1;
		int RecordLine = 1;
		const std::size_t Size = Text.size();
		while (Pos < Size)
		{
			const char C = Text[Pos];
			if (bInQuotes)
			{
				if (C == '"')
				{
					if (Pos + 1 < Size && Text[Pos + 1] == '"')
					{
						Field.push_back('"');
						Pos += 2;
						continue;
					}
					bInQuotes = false;
					++Pos;
					continue;
				}
				if (C == '\n')
				{
					++Line;
				}
				Field.push_back(C);
				++Pos;
				continue;
			}
			if (C == '"' && !bFieldStarted)
			{
				bInQuotes = true;
				bFieldStarted = true;
				++Pos;
				continue;
			}
			if (C == ',')
			{
				Record.push_back(Field);
				Field.clear();
				bFieldStarted = false;
				++Pos;
				continue;
			}
			if (C == '\r' || C == '\n')
			{
				Record.push_back(Field);
				Field.clear();
				bFieldStarted = false;
				if (!IsBlankRow(Record))
				{
					Records.push_back(Record);
					RecordLines.push_back(RecordLine);
				}
				Record.clear();
				if (C == '\r' && Pos + 1 < Size && Text[Pos + 1] == '\n')
				{
					++Pos;
				}
				++Pos;
				++Line;
				RecordLine = Line;
				continue;
			}
			Field.push_back(C);
			bFieldStarted = true;
			++Pos;
		}
		if (bInQuotes)
		{
			Error = "aspas sem fechar a partir da linha " + std::to_string(RecordLine);
			return false;
		}
		if (bFieldStarted || !Record.empty())
		{
			Record.push_back(Field);
			if (!IsBlankRow(Record))
			{
				Records.push_back(Record);
				RecordLines.push_back(RecordLine);
			}
		}
		if (Records.empty())
		{
			Error = "arquivo vazio";
			return false;
		}
		for (const std::string& Name : Records[0])
		{
			Out.Header.push_back(Trimmed(Name));
		}
		for (std::size_t i = 1; i < Records.size(); ++i)
		{
			if (Records[i].size() != Out.Header.size())
			{
				Error = "linha " + std::to_string(RecordLines[i]) + ": " + std::to_string(Records[i].size()) + " colunas, o cabeçalho tem "
					+ std::to_string(Out.Header.size());
				return false;
			}
			Out.Rows.push_back(Records[i]);
			Out.RowLines.push_back(RecordLines[i]);
		}
		return true;
	}

	bool ParseFloat(const std::string& Text, float& Out)
	{
		std::string Clean = Trimmed(Text);
		if (Clean.empty())
		{
			return false;
		}
		for (char& C : Clean)
		{
			if (C == ',')
			{
				C = '.';
			}
		}
		char* End = nullptr;
		errno = 0;
		const double Value = std::strtod(Clean.c_str(), &End);
		if (End == Clean.c_str() || *End != '\0' || errno == ERANGE)
		{
			return false;
		}
		Out = static_cast<float>(Value);
		return true;
	}

	bool ParseInt(const std::string& Text, int& Out)
	{
		const std::string Clean = Trimmed(Text);
		if (Clean.empty())
		{
			return false;
		}
		char* End = nullptr;
		errno = 0;
		const long Value = std::strtol(Clean.c_str(), &End, 10);
		if (End == Clean.c_str() || *End != '\0' || errno == ERANGE || Value < -2147483647L || Value > 2147483647L)
		{
			return false;
		}
		Out = static_cast<int>(Value);
		return true;
	}

	bool ParseColor(const std::string& Text, std::uint32_t& Out)
	{
		const std::string Clean = Trimmed(Text);
		if (Clean.size() != 7 || Clean[0] != '#')
		{
			return false;
		}
		std::uint32_t Value = 0;
		for (std::size_t i = 1; i < 7; ++i)
		{
			const char C = Clean[i];
			std::uint32_t Digit = 0;
			if (C >= '0' && C <= '9')
			{
				Digit = static_cast<std::uint32_t>(C - '0');
			}
			else if (C >= 'a' && C <= 'f')
			{
				Digit = static_cast<std::uint32_t>(C - 'a' + 10);
			}
			else if (C >= 'A' && C <= 'F')
			{
				Digit = static_cast<std::uint32_t>(C - 'A' + 10);
			}
			else
			{
				return false;
			}
			Value = (Value << 4) | Digit;
		}
		Out = Value;
		return true;
	}
}
