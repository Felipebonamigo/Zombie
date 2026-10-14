// Testes: chat da Twitch (IRC por WebSocket, só leitura), comandos e votação do chat, chat simulado.
// Contrato: docs/regras-de-jogo.md, seção 9.

#include "Rules/ZRChat.h"

ZR_TEST(ChatQuebraLinhas, "Chat.QuebraOPacoteEmLinhas")
{
	const std::vector<std::string> Lines = ZR::SplitIrcLines("PING :tmi.twitch.tv\r\n:a!a@a.tmi.twitch.tv PRIVMSG #c :oi\r\n\r\n");
	ZR_EQ(static_cast<int>(Lines.size()), 2);
	if (Lines.size() == 2)
	{
		ZR_EXPECT(Lines[0] == "PING :tmi.twitch.tv");
		ZR_EXPECT(Lines[1] == ":a!a@a.tmi.twitch.tv PRIVMSG #c :oi");
	}
}

ZR_TEST(ChatPing, "Chat.ReconhecePing")
{
	ZR::FIrcMessage Message;
	ZR_EXPECT(ZR::ParseIrcLine("PING :tmi.twitch.tv", Message));
	ZR_EXPECT(Message.Command == "PING");
	ZR_EXPECT(Message.Trailing == "tmi.twitch.tv");
}

ZR_TEST(ChatMensagemSimples, "Chat.MensagemSemTags")
{
	ZR::FIrcMessage Message;
	ZR_EXPECT(ZR::ParseIrcLine(":felipe!felipe@felipe.tmi.twitch.tv PRIVMSG #brasa :!zumbi agora", Message));
	ZR_EXPECT(Message.Command == "PRIVMSG");
	ZR_EXPECT(Message.Nick == "felipe");
	ZR_EXPECT(Message.Channel == "#brasa");
	ZR_EXPECT(Message.Trailing == "!zumbi agora");
	ZR_EXPECT(Message.DisplayName.empty());
}

ZR_TEST(ChatMensagemComTags, "Chat.MensagemComTagsEEscapes")
{
	ZR::FIrcMessage Message;
	const std::string Line =
		"@badge-info=;badges=moderator/1,subscriber/12;color=#FF4500;display-name=Zeca_Bala;mod=1;subscriber=1;bits=100;"
		"user-type=mod;msg-id=x\\sy :zeca_bala!zeca_bala@zeca_bala.tmi.twitch.tv PRIVMSG #brasa :manda a horda: agora";
	ZR_EXPECT(ZR::ParseIrcLine(Line, Message));
	ZR_EXPECT(Message.DisplayName == "Zeca_Bala");
	ZR_EXPECT(Message.Color == "#FF4500");
	ZR_EXPECT(Message.bModerator);
	ZR_EXPECT(Message.bSubscriber);
	ZR_EXPECT(!Message.bBroadcaster);
	ZR_EQ(Message.Bits, 100);
	ZR_EXPECT(Message.Trailing == "manda a horda: agora");
	ZR_EXPECT(ZR::FindTag(Message, "msg-id") == "x y");
	ZR_EXPECT(ZR::UnescapeTagValue("a\\:b\\\\c\\sd\\re\\nf") == "a;b\\c d\re\nf");
}

ZR_TEST(ChatBroadcasterPeloBadge, "Chat.DonoDoCanalPeloBadge")
{
	ZR::FIrcMessage Message;
	ZR_EXPECT(ZR::ParseIrcLine("@badges=broadcaster/1;display-name=Brasa :brasa!brasa@brasa.tmi.twitch.tv PRIVMSG #brasa :teste", Message));
	ZR_EXPECT(Message.bBroadcaster);
}

ZR_TEST(ChatLinhaInvalida, "Chat.LinhaInvalidaNaoQuebra")
{
	ZR::FIrcMessage Message;
	ZR_EXPECT(!ZR::ParseIrcLine("", Message));
	ZR_EXPECT(!ZR::ParseIrcLine("@so-tags", Message));
	ZR_EXPECT(!ZR::ParseIrcLine(":so-prefixo", Message));
	ZR_EXPECT(ZR::ParseIrcLine(":tmi.twitch.tv 001 justinfan123 :Welcome, GLHF!", Message));
	ZR_EXPECT(Message.Command == "001");
}

ZR_TEST(ChatNomeSeguro, "Chat.NomeDoZumbiSoComCaracteresSeguros")
{
	ZR::FIrcMessage Message;
	Message.Nick = "zeca_bala";
	Message.DisplayName = "Zeca_Bala";
	ZR_EXPECT(ZR::SafeViewerName(Message) == "Zeca_Bala");
	Message.DisplayName = "ゼカ";
	ZR_EXPECT(ZR::SafeViewerName(Message) == "zeca_bala");
	Message.DisplayName = "";
	Message.Nick = "a<b>c";
	ZR_EXPECT(ZR::SafeViewerName(Message) == "abc");
	Message.Nick = std::string(40, 'x');
	ZR_EQ(static_cast<int>(ZR::SafeViewerName(Message).size()), 25);
}

ZR_TEST(ChatComandos, "Chat.ComandosEApelidos")
{
	ZR_EQ(ZR::ParseChatCommand("!zumbi"), ZR::EChatCommand::Zombie);
	ZR_EQ(ZR::ParseChatCommand("!ZOMBIE por favor"), ZR::EChatCommand::Zombie);
	ZR_EQ(ZR::ParseChatCommand("!horda"), ZR::EChatCommand::Horde);
	ZR_EQ(ZR::ParseChatCommand("!horde"), ZR::EChatCommand::Horde);
	ZR_EQ(ZR::ParseChatCommand("!bujao"), ZR::EChatCommand::Bomb);
	ZR_EQ(ZR::ParseChatCommand("!bujão"), ZR::EChatCommand::Bomb);
	ZR_EQ(ZR::ParseChatCommand("!caixa"), ZR::EChatCommand::Airdrop);
	ZR_EQ(ZR::ParseChatCommand("!airdrop"), ZR::EChatCommand::Airdrop);
	ZR_EQ(ZR::ParseChatCommand("  !horda"), ZR::EChatCommand::Horde);
	ZR_EQ(ZR::ParseChatCommand("horda"), ZR::EChatCommand::None);
	ZR_EQ(ZR::ParseChatCommand("!hordas"), ZR::EChatCommand::None);
	ZR_EQ(ZR::ParseChatCommand(""), ZR::EChatCommand::None);
}

namespace
{
	ZR::FVoteConfig TestVotes()
	{
		ZR::FVoteConfig Config;
		Config.RoundSeconds = 30.0f;
		Config.MinVotes = 3;
		Config.ZombieCooldownSeconds = 60.0f;
		return Config;
	}

	ZR::EChatCommand AdvanceRound(ZR::FChatVoting& Voting, float Seconds)
	{
		ZR::EChatCommand Winner = ZR::EChatCommand::None;
		for (int i = 0; i < static_cast<int>(Seconds * 10.0f + 0.5f); ++i)
		{
			const ZR::EChatCommand Result = Voting.Tick(0.1f);
			if (Result != ZR::EChatCommand::None)
			{
				Winner = Result;
			}
		}
		return Winner;
	}
}

ZR_TEST(VotoUmPorPessoa, "Voto.UmVotoPorPessoaPorRodada")
{
	ZR::FChatVoting Voting(TestVotes());
	ZR_EXPECT(Voting.CastVote("ana", ZR::EChatCommand::Horde));
	ZR_EXPECT(!Voting.CastVote("ana", ZR::EChatCommand::Bomb));
	ZR_EXPECT(!Voting.CastVote("bia", ZR::EChatCommand::Zombie));
	ZR_EXPECT(!Voting.CastVote("bia", ZR::EChatCommand::None));
	ZR_EQ(Voting.VotesFor(ZR::EChatCommand::Horde), 1);
	ZR_EQ(Voting.VotesFor(ZR::EChatCommand::Bomb), 0);
}

ZR_TEST(VotoVencedorNoFim, "Voto.VenceOMaisVotadoNoFimDaRodada")
{
	ZR::FChatVoting Voting(TestVotes());
	Voting.CastVote("a", ZR::EChatCommand::Bomb);
	Voting.CastVote("b", ZR::EChatCommand::Horde);
	Voting.CastVote("c", ZR::EChatCommand::Horde);
	Voting.CastVote("d", ZR::EChatCommand::Horde);
	ZR_EQ(AdvanceRound(Voting, 29.0f), ZR::EChatCommand::None);
	ZR_EQ(AdvanceRound(Voting, 1.5f), ZR::EChatCommand::Horde);
	// rodada nova: tudo zerado e quem votou pode votar de novo
	ZR_EQ(Voting.VotesFor(ZR::EChatCommand::Horde), 0);
	ZR_EXPECT(Voting.CastVote("a", ZR::EChatCommand::Airdrop));
}

ZR_TEST(VotoMinimo, "Voto.SemOMinimoNinguemVence")
{
	ZR::FChatVoting Voting(TestVotes());
	Voting.CastVote("a", ZR::EChatCommand::Bomb);
	Voting.CastVote("b", ZR::EChatCommand::Bomb);
	ZR_EQ(AdvanceRound(Voting, 31.0f), ZR::EChatCommand::None);
}

ZR_TEST(VotoEmpate, "Voto.EmpateVaiParaQuemChegouPrimeiro")
{
	ZR::FChatVoting Voting(TestVotes());
	Voting.CastVote("a", ZR::EChatCommand::Airdrop);
	Voting.CastVote("b", ZR::EChatCommand::Bomb);
	Voting.CastVote("c", ZR::EChatCommand::Bomb);
	Voting.CastVote("d", ZR::EChatCommand::Airdrop);
	Voting.CastVote("e", ZR::EChatCommand::Bomb);
	Voting.CastVote("f", ZR::EChatCommand::Airdrop);
	// os dois com 3; a bomba chegou a 3 primeiro
	ZR_EQ(AdvanceRound(Voting, 31.0f), ZR::EChatCommand::Bomb);
}

ZR_TEST(VotoZumbiComNomeTemEspera, "Voto.ZumbiComNomeTemEsperaPorPessoa")
{
	ZR::FChatVoting Voting(TestVotes());
	ZR_EXPECT(Voting.RequestNamedZombie("ana"));
	ZR_EXPECT(!Voting.RequestNamedZombie("ana"));
	ZR_EXPECT(Voting.RequestNamedZombie("bia"));
	AdvanceRound(Voting, 59.0f);
	ZR_EXPECT(!Voting.RequestNamedZombie("ana"));
	AdvanceRound(Voting, 1.5f);
	ZR_EXPECT(Voting.RequestNamedZombie("ana"));
}

ZR_TEST(ChatFalsoNaoRepete, "ChatFalso.EscolheDaCategoriaSemRepetirSeguido")
{
	std::vector<ZR::FChatLine> Lines;
	Lines.push_back(ZR::FChatLine{"abate", "boa!", "nice!"});
	Lines.push_back(ZR::FChatLine{"abate", "kkkkk", "lol"});
	Lines.push_back(ZR::FChatLine{"explosao", "BOOM", "BOOM"});
	ZR::FFakeChat Chat(Lines, 5);
	std::string Previous;
	bool bNoRepeat = true;
	bool bCategory = true;
	for (int i = 0; i < 50; ++i)
	{
		const ZR::FChatLine* Line = Chat.Pick("abate");
		bCategory = bCategory && Line != nullptr && Line->Category == "abate";
		if (Line != nullptr)
		{
			bNoRepeat = bNoRepeat && Line->TextPt != Previous;
			Previous = Line->TextPt;
		}
	}
	ZR_EXPECT(bNoRepeat);
	ZR_EXPECT(bCategory);
	ZR_EXPECT(Chat.Pick("nao-existe") == nullptr);
	const ZR::FChatLine* Single = Chat.Pick("explosao");
	ZR_EXPECT(Single != nullptr && Single->TextPt == "BOOM");
	const std::string Name = Chat.FakeUserName();
	ZR_EXPECT(!Name.empty() && Name.size() <= 25);
}
