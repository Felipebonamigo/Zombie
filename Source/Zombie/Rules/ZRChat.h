// Chat da live. Três partes, todas regras puras (sem Unreal); a conexão em si fica em Stream/ZMBTwitchChat no Unreal.
// Contrato: docs/regras-de-jogo.md, seção 9.
//
//  1. Leitura do chat da Twitch: o IRC da Twitch por WebSocket (wss://irc-ws.chat.twitch.tv) aceita conexão anônima só de
//     leitura (apelido justinfanNNNN), sem conta nem chave. Aqui: quebrar o pacote em linhas e ler cada linha IRC com tags.
//  2. Comandos do chat: !zumbi (nasce um zumbi com o nome de quem pediu, com espera por pessoa) e a votação de
//     !horda, !bujao e !caixa em rodadas: um voto por pessoa por rodada, vence o mais votado com o mínimo de votos, empate
//     vai para quem chegou primeiro àquela contagem.
//  3. Chat simulado: quando não há chat de verdade, falas de Data/ChatFalso.csv reagem ao que acontece no jogo.

#pragma once

#include <cstdint>
#include <string>
#include <utility>
#include <vector>

#include "Rules/ZRRandom.h"

namespace ZR
{
	struct FIrcMessage
	{
		std::vector<std::pair<std::string, std::string>> Tags;
		/** Prefixo sem os dois pontos (":nick!user@host"). */
		std::string Prefix;
		std::string Nick;
		std::string Command;
		/** Primeiro parâmetro que começa com '#'. */
		std::string Channel;
		/** Texto depois de " :" (o corpo da mensagem no PRIVMSG; o servidor no PING). */
		std::string Trailing;
		std::string DisplayName;
		std::string Color;
		bool bModerator = false;
		bool bSubscriber = false;
		bool bBroadcaster = false;
		int Bits = 0;
	};

	/** Quebra um pacote do WebSocket em linhas IRC (separadas por \r\n ou \n), sem as vazias. */
	std::vector<std::string> SplitIrcLines(const std::string& Chunk);

	/** Lê uma linha IRC. Falso se não houver comando. */
	bool ParseIrcLine(const std::string& Line, FIrcMessage& Out);

	/** Desfaz o escape dos valores de tag do IRCv3: \: ; \s espaço; \\ barra; \r e \n. */
	std::string UnescapeTagValue(const std::string& Value);

	/** Valor de uma tag, ou vazio. */
	std::string FindTag(const FIrcMessage& Message, const std::string& Key);

	/**
	 * Nome que aparece em cima do zumbi: o display-name se tiver só letras ASCII, dígitos e '_', senão o apelido; sem
	 * nenhum outro caractere, no máximo 25.
	 */
	std::string SafeViewerName(const FIrcMessage& Message);

	enum class EChatCommand : std::uint8_t
	{
		None,
		Zombie,
		Horde,
		Bomb,
		Airdrop,
	};

	/** Primeira palavra da mensagem, sem diferenciar maiúsculas (só ASCII), com os apelidos em inglês e com acento. */
	EChatCommand ParseChatCommand(const std::string& Text);

	struct FVoteConfig
	{
		float RoundSeconds = 30.0f;
		int MinVotes = 3;
		float ZombieCooldownSeconds = 60.0f;
	};

	class FChatVoting
	{
	public:
		explicit FChatVoting(const FVoteConfig& InConfig = FVoteConfig()) : Config(InConfig) {}

		void Configure(const FVoteConfig& InConfig) { Config = InConfig; }

		/** Só Horde, Bomb e Airdrop são votáveis. Falso se a pessoa já votou nesta rodada. */
		bool CastVote(const std::string& User, EChatCommand Option);

		/** !zumbi: verdadeiro se a pessoa pode pedir agora (e começa a espera dela). */
		bool RequestNamedZombie(const std::string& User);

		/** Avança o relógio; no fim da rodada devolve o vencedor (ou None) e começa outra. */
		EChatCommand Tick(float DeltaSeconds);

		int VotesFor(EChatCommand Option) const;
		float RoundTimeLeft() const { return Config.RoundSeconds - RoundTime; }

	private:
		static int Slot(EChatCommand Option);

		FVoteConfig Config;
		double Clock = 0.0;
		float RoundTime = 0.0f;
		int Votes[3] = {0, 0, 0};
		/** Quando cada opção chegou à contagem atual (desempate). */
		double ReachedAt[3] = {0.0, 0.0, 0.0};
		std::vector<std::string> VotedThisRound;
		std::vector<std::pair<std::string, double>> ZombieRequests;
	};

	struct FChatLine
	{
		std::string Category;
		std::string TextPt;
		std::string TextEn;
	};

	/** Categorias que o jogo pede ao chat simulado; Data/ChatFalso.csv precisa de pelo menos 2 falas em cada. */
	const std::vector<const char*>& FakeChatCategories();

	class FFakeChat
	{
	public:
		FFakeChat(std::vector<FChatLine> InLines, std::uint64_t Seed);

		/** Uma fala da categoria, sem repetir a anterior da mesma categoria quando há mais de uma. Nulo se não houver. */
		const FChatLine* Pick(const std::string& Category);

		/** Apelido de espectador inventado, só ASCII, até 25 caracteres. */
		std::string FakeUserName();

	private:
		std::vector<FChatLine> Lines;
		std::vector<std::pair<std::string, std::size_t>> LastPicked;
		FRng Rng;
	};
}
