// Chat de verdade da Twitch, só leitura e sem conta: o IRC da Twitch por WebSocket aceita o apelido anônimo
// "justinfanNNNNN". Basta o nome do canal:
//   - linha de comando:  -ZMBTwitch=nomedocanal
//   - console do jogo:   zmb.twitch nomedocanal   (zmb.twitch.sair desconecta)
// Cada mensagem vira um FZMBTwitchMessage (nome seguro para pôr em cima do zumbi, texto, cor); a live
// (UZMBStreamSubsystem) mostra no chat e trata os comandos. A leitura das linhas IRC é regra pura (Rules/ZRChat.h).
// Cai a conexão: tenta de novo a cada 5 s enquanto houver canal.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "ZMBTwitchChat.generated.h"

class IWebSocket;

struct FZMBTwitchMessage
{
	FString User;
	FString Text;
	FLinearColor Color = FLinearColor::White;
	bool bModerator = false;
};

DECLARE_MULTICAST_DELEGATE_OneParam(FZMBOnTwitchMessage, const FZMBTwitchMessage&);

UCLASS()
class ZOMBIE_API UZMBTwitchChat : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	void Connect(const FString& InChannel);
	void Disconnect();

	/** Chamado todo quadro pela live: reconexão. */
	void Update(float DeltaSeconds);

	bool IsConnected() const { return bJoined; }
	const FString& GetChannel() const { return Channel; }

	FZMBOnTwitchMessage OnMessage;

private:
	void OpenSocket();
	void HandleConnected();
	void HandleError(const FString& Error);
	void HandleClosed(int32 StatusCode, const FString& Reason, bool bWasClean);
	void HandleMessage(const FString& Text);

	TSharedPtr<IWebSocket> Socket;
	FString Channel;
	bool bJoined = false;
	float RetryIn = -1.0f;
};
