#include "Rules/ZRChat.h"

#include <algorithm>
#include <cstdlib>

namespace ZR
{
	namespace
	{
		bool IsSafeNameChar(char C)
		{
			return (C >= 'a' && C <= 'z') || (C >= 'A' && C <= 'Z') || (C >= '0' && C <= '9') || C == '_';
		}

		std::string LowerAscii(std::string Text)
		{
			for (char& C : Text)
			{
				if (C >= 'A' && C <= 'Z')
				{
					C = static_cast<char>(C - 'A' + 'a');
				}
			}
			return Text;
		}

		bool HasBadge(const std::string& Badges, const std::string& Name)
		{
			std::size_t Start = 0;
			while (Start <= Badges.size())
			{
				const std::size_t Comma = Badges.find(',', Start);
				const std::string Badge = Badges.substr(Start, Comma == std::string::npos ? std::string::npos : Comma - Start);
				if (Badge.compare(0, Name.size() + 1, Name + "/") == 0)
				{
					return true;
				}
				if (Comma == std::string::npos)
				{
					break;
				}
				Start = Comma + 1;
			}
			return false;
		}
	}

	std::vector<std::string> SplitIrcLines(const std::string& Chunk)
	{
		std::vector<std::string> Lines;
		std::string Current;
		for (char C : Chunk)
		{
			if (C == '\n')
			{
				if (!Current.empty() && Current.back() == '\r')
				{
					Current.pop_back();
				}
				if (!Current.empty())
				{
					Lines.push_back(Current);
				}
				Current.clear();
			}
			else
			{
				Current.push_back(C);
			}
		}
		if (!Current.empty() && Current.back() == '\r')
		{
			Current.pop_back();
		}
		if (!Current.empty())
		{
			Lines.push_back(Current);
		}
		return Lines;
	}

	std::string UnescapeTagValue(const std::string& Value)
	{
		std::string Out;
		Out.reserve(Value.size());
		for (std::size_t i = 0; i < Value.size(); ++i)
		{
			if (Value[i] != '\\' || i + 1 >= Value.size())
			{
				if (Value[i] != '\\')
				{
					Out.push_back(Value[i]);
				}
				continue;
			}
			const char Next = Value[++i];
			switch (Next)
			{
			case ':': Out.push_back(';'); break;
			case 's': Out.push_back(' '); break;
			case 'r': Out.push_back('\r'); break;
			case 'n': Out.push_back('\n'); break;
			default: Out.push_back(Next); break;
			}
		}
		return Out;
	}

	bool ParseIrcLine(const std::string& Line, FIrcMessage& Out)
	{
		Out = FIrcMessage();
		std::size_t Pos = 0;
		const std::size_t Size = Line.size();

		if (Pos < Size && Line[Pos] == '@')
		{
			const std::size_t End = Line.find(' ', Pos);
			if (End == std::string::npos)
			{
				return false;
			}
			const std::string TagText = Line.substr(Pos + 1, End - Pos - 1);
			std::size_t TagStart = 0;
			while (TagStart <= TagText.size())
			{
				const std::size_t Semicolon = TagText.find(';', TagStart);
				const std::string Tag = TagText.substr(TagStart, Semicolon == std::string::npos ? std::string::npos : Semicolon - TagStart);
				if (!Tag.empty())
				{
					const std::size_t Equals = Tag.find('=');
					const std::string Key = Tag.substr(0, Equals);
					const std::string Value = Equals == std::string::npos ? std::string() : UnescapeTagValue(Tag.substr(Equals + 1));
					Out.Tags.emplace_back(Key, Value);
				}
				if (Semicolon == std::string::npos)
				{
					break;
				}
				TagStart = Semicolon + 1;
			}
			Pos = End + 1;
		}
		while (Pos < Size && Line[Pos] == ' ')
		{
			++Pos;
		}
		if (Pos < Size && Line[Pos] == ':')
		{
			const std::size_t End = Line.find(' ', Pos);
			if (End == std::string::npos)
			{
				return false;
			}
			Out.Prefix = Line.substr(Pos + 1, End - Pos - 1);
			const std::size_t Bang = Out.Prefix.find('!');
			Out.Nick = Out.Prefix.substr(0, Bang);
			Pos = End + 1;
		}
		while (Pos < Size && Line[Pos] == ' ')
		{
			++Pos;
		}
		const std::size_t CommandEnd = std::min(Line.find(' ', Pos), Size);
		Out.Command = Line.substr(Pos, CommandEnd - Pos);
		if (Out.Command.empty())
		{
			return false;
		}
		Pos = CommandEnd;

		while (Pos < Size)
		{
			while (Pos < Size && Line[Pos] == ' ')
			{
				++Pos;
			}
			if (Pos >= Size)
			{
				break;
			}
			if (Line[Pos] == ':')
			{
				Out.Trailing = Line.substr(Pos + 1);
				break;
			}
			const std::size_t End = std::min(Line.find(' ', Pos), Size);
			const std::string Param = Line.substr(Pos, End - Pos);
			if (Out.Channel.empty() && !Param.empty() && Param[0] == '#')
			{
				Out.Channel = Param;
			}
			Pos = End;
		}

		Out.DisplayName = FindTag(Out, "display-name");
		Out.Color = FindTag(Out, "color");
		const std::string Badges = FindTag(Out, "badges");
		Out.bModerator = FindTag(Out, "mod") == "1" || HasBadge(Badges, "moderator");
		Out.bSubscriber = FindTag(Out, "subscriber") == "1" || HasBadge(Badges, "subscriber");
		Out.bBroadcaster = HasBadge(Badges, "broadcaster");
		Out.Bits = std::atoi(FindTag(Out, "bits").c_str());
		return true;
	}

	std::string FindTag(const FIrcMessage& Message, const std::string& Key)
	{
		for (const std::pair<std::string, std::string>& Tag : Message.Tags)
		{
			if (Tag.first == Key)
			{
				return Tag.second;
			}
		}
		return std::string();
	}

	std::string SafeViewerName(const FIrcMessage& Message)
	{
		const bool bDisplayOk = !Message.DisplayName.empty()
			&& std::all_of(Message.DisplayName.begin(), Message.DisplayName.end(), IsSafeNameChar);
		const std::string& Source = bDisplayOk ? Message.DisplayName : Message.Nick;
		std::string Name;
		for (char C : Source)
		{
			if (IsSafeNameChar(C) && Name.size() < 25)
			{
				Name.push_back(C);
			}
		}
		return Name;
	}

	EChatCommand ParseChatCommand(const std::string& Text)
	{
		std::size_t Start = Text.find_first_not_of(" \t");
		if (Start == std::string::npos || Text[Start] != '!')
		{
			return EChatCommand::None;
		}
		const std::size_t End = Text.find_first_of(" \t", Start);
		const std::string Word = LowerAscii(Text.substr(Start, End == std::string::npos ? std::string::npos : End - Start));
		if (Word == "!zumbi" || Word == "!zombie")
		{
			return EChatCommand::Zombie;
		}
		if (Word == "!horda" || Word == "!horde")
		{
			return EChatCommand::Horde;
		}
		// "!bujão" em UTF-8: ã = C3 A3 (só as letras ASCII passam para minúscula)
		if (Word == "!bujao" || Word == "!buj\xC3\xA3o" || Word == "!bomb" || Word == "!bomba")
		{
			return EChatCommand::Bomb;
		}
		if (Word == "!caixa" || Word == "!airdrop" || Word == "!drop")
		{
			return EChatCommand::Airdrop;
		}
		return EChatCommand::None;
	}

	int FChatVoting::Slot(EChatCommand Option)
	{
		switch (Option)
		{
		case EChatCommand::Horde: return 0;
		case EChatCommand::Bomb: return 1;
		case EChatCommand::Airdrop: return 2;
		default: return -1;
		}
	}

	bool FChatVoting::CastVote(const std::string& User, EChatCommand Option)
	{
		const int Index = Slot(Option);
		if (Index < 0 || User.empty())
		{
			return false;
		}
		if (std::find(VotedThisRound.begin(), VotedThisRound.end(), User) != VotedThisRound.end())
		{
			return false;
		}
		VotedThisRound.push_back(User);
		++Votes[Index];
		// relógio + um pouco por voto: votos no mesmo quadro ainda ficam em ordem de chegada
		ReachedAt[Index] = Clock + 1e-6 * static_cast<double>(VotedThisRound.size());
		return true;
	}

	bool FChatVoting::RequestNamedZombie(const std::string& User)
	{
		if (User.empty())
		{
			return false;
		}
		for (std::pair<std::string, double>& Request : ZombieRequests)
		{
			if (Request.first == User)
			{
				if (Clock - Request.second < static_cast<double>(Config.ZombieCooldownSeconds))
				{
					return false;
				}
				Request.second = Clock;
				return true;
			}
		}
		ZombieRequests.emplace_back(User, Clock);
		return true;
	}

	EChatCommand FChatVoting::Tick(float DeltaSeconds)
	{
		const float Dt = std::max(0.0f, DeltaSeconds);
		Clock += static_cast<double>(Dt);
		RoundTime += Dt;
		// esquece pedidos de zumbi que já passaram da espera (a lista não cresce sem fim numa live longa)
		ZombieRequests.erase(std::remove_if(ZombieRequests.begin(), ZombieRequests.end(),
			[this](const std::pair<std::string, double>& Request)
			{
				return Clock - Request.second >= static_cast<double>(Config.ZombieCooldownSeconds);
			}), ZombieRequests.end());
		if (RoundTime < Config.RoundSeconds)
		{
			return EChatCommand::None;
		}

		static const EChatCommand Options[3] = {EChatCommand::Horde, EChatCommand::Bomb, EChatCommand::Airdrop};
		EChatCommand Winner = EChatCommand::None;
		int BestVotes = std::max(1, Config.MinVotes) - 1;
		double BestReached = 0.0;
		for (int i = 0; i < 3; ++i)
		{
			if (Votes[i] > BestVotes || (Votes[i] == BestVotes && Winner != EChatCommand::None && ReachedAt[i] < BestReached))
			{
				Winner = Options[i];
				BestVotes = Votes[i];
				BestReached = ReachedAt[i];
			}
		}
		RoundTime = 0.0f;
		for (int i = 0; i < 3; ++i)
		{
			Votes[i] = 0;
			ReachedAt[i] = 0.0;
		}
		VotedThisRound.clear();
		return Winner;
	}

	int FChatVoting::VotesFor(EChatCommand Option) const
	{
		const int Index = Slot(Option);
		return Index < 0 ? 0 : Votes[Index];
	}

	const std::vector<const char*>& FakeChatCategories()
	{
		static const std::vector<const char*> Categories = {
			"abate", "cabeca", "explosao", "cadeia", "atropelo", "desmembrou", "dano", "quase_morte", "calmo",
			"doacao", "horda", "noite", "mira_lenta", "zumbi_do_chat", "inicio", "barricada", "armadilha", "marco", "saque",
		};
		return Categories;
	}

	FFakeChat::FFakeChat(std::vector<FChatLine> InLines, std::uint64_t Seed) : Lines(std::move(InLines)), Rng(Seed) {}

	const FChatLine* FFakeChat::Pick(const std::string& Category)
	{
		std::vector<std::size_t> Matches;
		for (std::size_t i = 0; i < Lines.size(); ++i)
		{
			if (Lines[i].Category == Category)
			{
				Matches.push_back(i);
			}
		}
		if (Matches.empty())
		{
			return nullptr;
		}
		std::size_t* Last = nullptr;
		for (std::pair<std::string, std::size_t>& Entry : LastPicked)
		{
			if (Entry.first == Category)
			{
				Last = &Entry.second;
			}
		}
		std::size_t Choice = Matches[static_cast<std::size_t>(Rng.RangeInt(0, static_cast<int>(Matches.size()) - 1))];
		if (Last != nullptr && Matches.size() > 1 && Choice == *Last)
		{
			// troca pela próxima da mesma categoria (em volta)
			const std::size_t Position = static_cast<std::size_t>(std::find(Matches.begin(), Matches.end(), Choice) - Matches.begin());
			Choice = Matches[(Position + 1) % Matches.size()];
		}
		if (Last != nullptr)
		{
			*Last = Choice;
		}
		else
		{
			LastPicked.emplace_back(Category, Choice);
		}
		return &Lines[Choice];
	}

	std::string FFakeChat::FakeUserName()
	{
		static const char* const Starts[] = {
			"ze", "bia", "kaka", "tio", "mano", "lele", "dudu", "nanda", "gui", "rafa", "juju", "biel", "vovo", "neto", "lu",
		};
		static const char* const Ends[] = {
			"_gamer", "_zumbi", "brasa", "_tv", "_br", "lives", "_rj", "_sp", "plays", "_007", "_mito", "_noob", "_fps",
		};
		const int StartCount = static_cast<int>(sizeof(Starts) / sizeof(Starts[0]));
		const int EndCount = static_cast<int>(sizeof(Ends) / sizeof(Ends[0]));
		std::string Name = Starts[Rng.RangeInt(0, StartCount - 1)];
		Name += Ends[Rng.RangeInt(0, EndCount - 1)];
		if (Rng.Chance(0.5))
		{
			Name += std::to_string(Rng.RangeInt(1, 99));
		}
		return Name.substr(0, 25);
	}
}
