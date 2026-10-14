// A live: o coração do "AO VIVO". Recebe os lances do jogo (abates, explosões, sustos), soma o hype pelas regras puras
// (ZR::AddHypeEvent / ZR::TickHype), faz o chat reagir (chat simulado de Data/ChatFalso.csv ou o chat de verdade da Twitch),
// roda a votação do chat (!horda, !bujao, !caixa) e os zumbis com nome (!zumbi), transforma doações em entregas por drone
// e guarda os melhores momentos para o botão Clipar (ZR::FHighlightTracker).
//
// O chat simulado também vota: sem Twitch, os "espectadores" pedem horda, bujão e caixa de vez em quando, e mandam zumbi
// com nome. Assim o pilar "o chat joga junto" existe no jogo de uma pessoa só.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "Combat/ZMBTypes.h"
#include "Rules/ZRChallenges.h"
#include "Rules/ZRChat.h"
#include "Rules/ZRHighlights.h"
#include "Rules/ZRHype.h"
#include <memory>
#include "ZMBStreamSubsystem.generated.h"

struct FZMBTwitchMessage;

/** Uma linha do chat na tela. */
struct FZMBChatEntry
{
	FString Name;
	FString Text;
	FLinearColor NameColor = FLinearColor::White;
	/** Mensagem do sistema (doação, voto, anúncio): aparece destacada. */
	bool bSystem = false;
	float Age = 0.0f;
};

/** Texto que salta no meio da tela ("NA CABEÇA +6"). */
struct FZMBPopup
{
	FString Text;
	FLinearColor Color = FLinearColor::White;
	float Age = 0.0f;
};

UCLASS()
class ZOMBIE_API UZMBStreamSubsystem : public UTickableWorldSubsystem
{
	GENERATED_BODY()

public:
	virtual void OnWorldBeginPlay(UWorld& InWorld) override;
	virtual void Deinitialize() override;
	virtual void Tick(float DeltaTime) override;
	virtual TStatId GetStatId() const override;

	static UZMBStreamSubsystem* Get(const UObject* WorldContext);

	// --- lances do jogo ---
	void ReportHype(ZR::EHypeEvent Event, int32 Count, const FVector& Where);
	void ReportKill(const FZMBKillInfo& Kill);
	void ReportExplosion(int32 ExplosionKills, int32 Gibs, int32 ChainDepth, const FVector& Where);
	void ReportPlayerDamage(float Amount, float HealthAfter);
	void OnPlayerDied();
	void OnPlayerRespawned();

	/** O chat (simulado) comenta, com uma fala da categoria de Data/ChatFalso.csv. */
	void React(const char* Category);
	/** Faixa grande no meio da tela (Noite da Horda, entrega por drone...). */
	void Announce(const FText& Text);
	/** Aviso curto embaixo (clipe salvo, sucata ganha, peça consertada). */
	void ShowToast(const FString& Text);
	/** Botão Clipar: marca a melhor janela dos últimos 20 s e tira um print com o HUD da live. */
	void Clip();

	/** O Diretor pega aqui os pedidos do chat. */
	void ConsumeChatActions(TArray<FZMBChatAction>& Out);

	// --- leitura para o HUD ---
	float GetHype() const { return State.Hype; }
	float GetViewers() const { return State.Viewers; }
	float GetFollowers() const { return State.Followers; }
	int32 GetCombo() const { return State.Combo; }
	float GetComboMultiplier() const;
	double GetMoney() const { return Money; }
	const TArray<FZMBChatEntry>& GetChat() const { return Chat; }
	const TArray<FZMBPopup>& GetPopups() const { return Popups; }
	const FText& GetAnnouncement() const { return Announcement; }
	float GetAnnouncementAge() const { return AnnouncementAge; }
	const FString& GetToast() const { return Toast; }
	float GetToastAge() const { return ToastAge; }
	int32 GetVotes(ZR::EChatCommand Option) const { return Voting.VotesFor(Option); }
	float GetVoteTimeLeft() const { return Voting.RoundTimeLeft(); }
	bool IsTwitchConnected() const;
	FString GetTwitchChannel() const;
	double GetLiveSeconds() const { return LiveSeconds; }
	/** Pauta de patrocinador ativa: texto (vazio = nenhuma), progresso, alvo e segundos que faltam. */
	FString GetChallengeText() const;
	int32 GetChallengeProgress() const { return Challenges.ProgressCount(); }
	int32 GetChallengeTarget() const;
	float GetChallengeTimeLeft() const { return Challenges.TimeLeft(); }
	int32 GetKills() const { return Kills; }

private:
	void HandleTwitchMessage(const FZMBTwitchMessage& Message);
	void HandleCommand(const FString& User, const std::string& Text);
	void AddChat(const FString& Name, const FString& Text, const FLinearColor& Color, bool bSystem);
	void AddPopup(const FString& Text, const FLinearColor& Color);
	void TickFakeChat(float DeltaTime);
	void HandleChallenge(const ZR::FChallengeUpdate& Update);
	const ZR::FHypeEventDef* EventDef(ZR::EHypeEvent Event) const;

	ZR::FHypeConfig Config;
	ZR::FHypeState State;
	ZR::FChatVoting Voting;
	ZR::FChallengeTracker Challenges;
	ZR::FHighlightTracker Highlights = ZR::FHighlightTracker(60.0);
	std::unique_ptr<ZR::FFakeChat> FakeChat;

	TArray<FZMBChatEntry> Chat;
	TArray<FZMBPopup> Popups;
	TArray<FZMBChatAction> PendingActions;
	/** Reações do chat esperando um instante (gente demora para digitar). */
	TArray<TPair<float, FString>> PendingReactions;

	FText Announcement;
	float AnnouncementAge = 100.0f;
	FString Toast;
	float ToastAge = 100.0f;
	double Money = 0.0;
	double LiveSeconds = 0.0;
	float FakeChatTimer = 2.0f;
	float FakeVoteTimer = 12.0f;
	float FakeZombieTimer = 40.0f;
	int32 Kills = 0;
	int32 ClipCount = 0;
	bool bPortuguese = true;
	FDelegateHandle TwitchHandle;
};
