#include "Stream/ZMBTwitchChat.h"
#include "Zombie.h"
#include "Rules/ZRChat.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "HAL/IConsoleManager.h"
#include "IWebSocket.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Modules/ModuleManager.h"
#include "WebSocketsModule.h"

namespace
{
	const TCHAR* TwitchUrl = TEXT("wss://irc-ws.chat.twitch.tv:443");

	UZMBTwitchChat* TwitchOf(UWorld* World)
	{
		UGameInstance* GameInstance = World ? World->GetGameInstance() : nullptr;
		return GameInstance ? GameInstance->GetSubsystem<UZMBTwitchChat>() : nullptr;
	}

	void ConnectCommand(const TArray<FString>& Args, UWorld* World)
	{
		UZMBTwitchChat* Twitch = TwitchOf(World);
		if (Twitch && Args.Num() > 0)
		{
			Twitch->Connect(Args[0]);
		}
	}

	void DisconnectCommand(const TArray<FString>& Args, UWorld* World)
	{
		if (UZMBTwitchChat* Twitch = TwitchOf(World))
		{
			Twitch->Disconnect();
		}
	}

	FAutoConsoleCommandWithWorldAndArgs GTwitchConnect(TEXT("zmb.twitch"),
		TEXT("Lê o chat de um canal da Twitch (sem conta): zmb.twitch nomedocanal"),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&ConnectCommand));

	FAutoConsoleCommandWithWorldAndArgs GTwitchDisconnect(TEXT("zmb.twitch.sair"),
		TEXT("Para de ler o chat da Twitch."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&DisconnectCommand));

	FLinearColor ParseTwitchColor(const FString& Hex)
	{
		if (Hex.Len() == 7 && Hex[0] == TEXT('#'))
		{
			return FLinearColor(FColor::FromHex(Hex));
		}
		return FLinearColor(0.6f, 0.45f, 1.0f);
	}
}

void UZMBTwitchChat::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	FString FromCommandLine;
	if (FParse::Value(FCommandLine::Get(), TEXT("ZMBTwitch="), FromCommandLine) && !FromCommandLine.IsEmpty())
	{
		Connect(FromCommandLine);
	}
}

void UZMBTwitchChat::Deinitialize()
{
	Disconnect();
	Super::Deinitialize();
}

void UZMBTwitchChat::Connect(const FString& InChannel)
{
	Disconnect();
	Channel = InChannel.TrimStartAndEnd().ToLower();
	Channel.RemoveFromStart(TEXT("#"));
	if (Channel.IsEmpty())
	{
		return;
	}
	OpenSocket();
}

void UZMBTwitchChat::Disconnect()
{
	Channel.Reset();
	bJoined = false;
	RetryIn = -1.0f;
	if (Socket)
	{
		Socket->OnConnected().RemoveAll(this);
		Socket->OnConnectionError().RemoveAll(this);
		Socket->OnClosed().RemoveAll(this);
		Socket->OnMessage().RemoveAll(this);
		if (Socket->IsConnected())
		{
			Socket->Close();
		}
		Socket.Reset();
	}
}

void UZMBTwitchChat::OpenSocket()
{
	FWebSocketsModule& WebSockets = FModuleManager::LoadModuleChecked<FWebSocketsModule>(TEXT("WebSockets"));
	Socket = WebSockets.CreateWebSocket(TwitchUrl);
	Socket->OnConnected().AddUObject(this, &UZMBTwitchChat::HandleConnected);
	Socket->OnConnectionError().AddUObject(this, &UZMBTwitchChat::HandleError);
	Socket->OnClosed().AddUObject(this, &UZMBTwitchChat::HandleClosed);
	Socket->OnMessage().AddUObject(this, &UZMBTwitchChat::HandleMessage);
	UE_LOG(LogZombie, Display, TEXT("Twitch: conectando ao chat de #%s"), *Channel);
	Socket->Connect();
}

void UZMBTwitchChat::HandleConnected()
{
	// leitura anônima: tags pedem o display-name e a cor; o PASS é ignorado pela Twitch nesse modo
	Socket->Send(TEXT("CAP REQ :twitch.tv/tags"));
	Socket->Send(TEXT("PASS SCHMOOPIIE"));
	Socket->Send(FString::Printf(TEXT("NICK justinfan%d"), FMath::RandRange(10000, 99999)));
	Socket->Send(FString::Printf(TEXT("JOIN #%s"), *Channel));
	bJoined = true;
	UE_LOG(LogZombie, Display, TEXT("Twitch: lendo o chat de #%s"), *Channel);
}

void UZMBTwitchChat::HandleError(const FString& Error)
{
	UE_LOG(LogZombie, Warning, TEXT("Twitch: erro de conexão (%s); tento de novo em 5 s"), *Error);
	bJoined = false;
	RetryIn = 5.0f;
}

void UZMBTwitchChat::HandleClosed(int32 StatusCode, const FString& Reason, bool bWasClean)
{
	UE_LOG(LogZombie, Display, TEXT("Twitch: conexão fechada (%d %s)"), StatusCode, *Reason);
	bJoined = false;
	if (!Channel.IsEmpty())
	{
		RetryIn = 5.0f;
	}
}

void UZMBTwitchChat::Update(float DeltaSeconds)
{
	if (RetryIn < 0.0f || Channel.IsEmpty())
	{
		return;
	}
	RetryIn -= DeltaSeconds;
	if (RetryIn <= 0.0f)
	{
		RetryIn = -1.0f;
		const FString Again = Channel;
		Disconnect();
		Channel = Again;
		OpenSocket();
	}
}

void UZMBTwitchChat::HandleMessage(const FString& Text)
{
	const std::string Chunk(TCHAR_TO_UTF8(*Text));
	for (const std::string& Line : ZR::SplitIrcLines(Chunk))
	{
		ZR::FIrcMessage Message;
		if (!ZR::ParseIrcLine(Line, Message))
		{
			continue;
		}
		if (Message.Command == "PING")
		{
			Socket->Send(FString(TEXT("PONG :")) + FString(UTF8_TO_TCHAR(Message.Trailing.c_str())));
			continue;
		}
		if (Message.Command != "PRIVMSG")
		{
			continue;
		}
		FZMBTwitchMessage Out;
		Out.User = FString(UTF8_TO_TCHAR(ZR::SafeViewerName(Message).c_str()));
		Out.Text = FString(UTF8_TO_TCHAR(Message.Trailing.c_str()));
		Out.Color = ParseTwitchColor(FString(UTF8_TO_TCHAR(Message.Color.c_str())));
		Out.bModerator = Message.bModerator;
		if (!Out.User.IsEmpty())
		{
			OnMessage.Broadcast(Out);
		}
	}
}
