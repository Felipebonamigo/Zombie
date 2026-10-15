#include "Stream/ZMBStreamSubsystem.h"
#include "Zombie.h"
#include "Audio/ZMBAudioSubsystem.h"
#include "Game/ZMBGameData.h"
#include "Player/ZMBCharacter.h"
#include "Stream/ZMBTwitchChat.h"
#include "Zombies/ZMBHordeDirector.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/DateTime.h"
#include "UnrealClient.h"

#define LOCTEXT_NAMESPACE "ZombieStream"

namespace
{
	constexpr int32 MaxChatLines = 9;
	constexpr float ChatLineSeconds = 30.0f;
	constexpr float PopupSeconds = 1.6f;
	constexpr double AirdropPrice = 50.0;
	const FLinearColor Gold(1.0f, 0.78f, 0.15f);
	const FLinearColor Hot(1.0f, 0.35f, 0.1f);

	FLinearColor NameColor(const FString& Name)
	{
		return FLinearColor::MakeFromHSV8(static_cast<uint8>(GetTypeHash(Name) & 0xFF), 170, 255);
	}
}

UZMBStreamSubsystem* UZMBStreamSubsystem::Get(const UObject* WorldContext)
{
	const UWorld* World = WorldContext ? WorldContext->GetWorld() : nullptr;
	return World ? World->GetSubsystem<UZMBStreamSubsystem>() : nullptr;
}

TStatId UZMBStreamSubsystem::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(UZMBStreamSubsystem, STATGROUP_Tickables);
}

void UZMBStreamSubsystem::OnWorldBeginPlay(UWorld& InWorld)
{
	Super::OnWorldBeginPlay(InWorld);
	bPortuguese = UZMBGameData::IsPortuguese();
	if (const UZMBGameData* Data = UZMBGameData::Get(&InWorld))
	{
		Config = Data->Tuning().Hype;
		Voting.Configure(Data->Tuning().Vote);
		Challenges = ZR::FChallengeTracker(Data->Tuning().Challenge, static_cast<std::uint64_t>(FDateTime::Now().GetTicks()) ^ 0xC0FFEEull);
		FakeChat = std::make_unique<ZR::FFakeChat>(Data->ChatLines(), static_cast<std::uint64_t>(FDateTime::Now().GetTicks()));
	}
	State.Reset(Config);
	Stats.Reset(State.Followers);
	if (const UZMBGameData* Data = UZMBGameData::Get(&InWorld))
	{
		Milestones.Reset(Data->Milestones(), State.Followers);
	}
	if (UGameInstance* GameInstance = InWorld.GetGameInstance())
	{
		if (UZMBTwitchChat* Twitch = GameInstance->GetSubsystem<UZMBTwitchChat>())
		{
			TwitchHandle = Twitch->OnMessage.AddUObject(this, &UZMBStreamSubsystem::HandleTwitchMessage);
		}
	}
	AddChat(FString(), LOCTEXT("LiveStart", "Você está AO VIVO em Porto Brasa. Sobreviva e dê um show.").ToString(), Gold, true);
	React("inicio");
}

void UZMBStreamSubsystem::Deinitialize()
{
	const UWorld* World = GetWorld();
	if (UGameInstance* GameInstance = World ? World->GetGameInstance() : nullptr)
	{
		if (UZMBTwitchChat* Twitch = GameInstance->GetSubsystem<UZMBTwitchChat>())
		{
			Twitch->OnMessage.Remove(TwitchHandle);
		}
	}
	FakeChat.reset();
	Super::Deinitialize();
}

bool UZMBStreamSubsystem::IsTwitchConnected() const
{
	const UWorld* World = GetWorld();
	const UGameInstance* GameInstance = World ? World->GetGameInstance() : nullptr;
	const UZMBTwitchChat* Twitch = GameInstance ? GameInstance->GetSubsystem<UZMBTwitchChat>() : nullptr;
	return Twitch && Twitch->IsConnected();
}

FString UZMBStreamSubsystem::GetTwitchChannel() const
{
	const UWorld* World = GetWorld();
	const UGameInstance* GameInstance = World ? World->GetGameInstance() : nullptr;
	const UZMBTwitchChat* Twitch = GameInstance ? GameInstance->GetSubsystem<UZMBTwitchChat>() : nullptr;
	return Twitch ? Twitch->GetChannel() : FString();
}

float UZMBStreamSubsystem::GetComboMultiplier() const
{
	if (State.Combo <= 0)
	{
		return 1.0f;
	}
	return FMath::Min(1.0f + Config.ComboStep * static_cast<float>(State.Combo - 1), FMath::Max(1.0f, Config.ComboMaxMultiplier));
}

const ZR::FHypeEventDef* UZMBStreamSubsystem::EventDef(ZR::EHypeEvent Event) const
{
	const UZMBGameData* Data = UZMBGameData::Get(this);
	const size_t Index = static_cast<size_t>(Event);
	if (!Data || Index >= Data->HypeEvents().size())
	{
		return nullptr;
	}
	return &Data->HypeEvents()[Index];
}

void UZMBStreamSubsystem::AddChat(const FString& Name, const FString& Text, const FLinearColor& Color, bool bSystem)
{
	FZMBChatEntry Entry;
	Entry.Name = Name;
	Entry.Text = Text;
	Entry.NameColor = Color;
	Entry.bSystem = bSystem;
	Chat.Add(Entry);
	while (Chat.Num() > MaxChatLines)
	{
		Chat.RemoveAt(0);
	}
}

void UZMBStreamSubsystem::AddPopup(const FString& Text, const FLinearColor& Color)
{
	FZMBPopup Popup;
	Popup.Text = Text;
	Popup.Color = Color;
	Popups.Add(Popup);
	while (Popups.Num() > 5)
	{
		Popups.RemoveAt(0);
	}
}

void UZMBStreamSubsystem::ReportHype(ZR::EHypeEvent Event, int32 Count, const FVector& Where)
{
	const ZR::FHypeEventDef* Def = EventDef(Event);
	if (!Def || Count <= 0)
	{
		return;
	}
	const ZR::FHypeGain Gain = ZR::AddHypeEvent(Config, State, static_cast<int>(Event), Def->Points, Count);
	FString Label = UZMBGameData::ToFString(bPortuguese ? Def->LabelPt : Def->LabelEn);
	if (Label.IsEmpty())
	{
		Label = UZMBGameData::ToFString(Def->Id);
	}
	Highlights.Add(LiveSeconds, Gain.Points, std::string(TCHAR_TO_UTF8(*Label)));
	if (const UZMBGameData* Data = UZMBGameData::Get(this))
	{
		HandleChallenge(Challenges.OnEvent(Data->Challenges(), static_cast<int>(Event), Count));
	}
	if (Gain.Points >= 0.5f)
	{
		const FString Times = Count > 1 ? FString::Printf(TEXT(" x%d"), Count) : FString();
		AddPopup(FString::Printf(TEXT("%s%s  +%.0f"), *Label, *Times, Gain.Points), Gain.Points >= 8.0f ? Hot : (Gain.Points >= 4.0f ? Gold : FLinearColor::White));
	}
}

void UZMBStreamSubsystem::ReportKill(const FZMBKillInfo& Kill)
{
	++Kills;
	ReportHype(ZR::EHypeEvent::Kill, 1, Kill.Location);
	++Stats.Kills;
	if (Kill.bHeadshot)
	{
		++Stats.Headshots;
		ReportHype(ZR::EHypeEvent::Headshot, 1, Kill.Location);
	}
	if (Kill.bGibbed)
	{
		ReportHype(ZR::EHypeEvent::Gib, 1, Kill.Location);
	}
	else if (Kill.SeveredLimbs > 0)
	{
		ReportHype(ZR::EHypeEvent::Dismember, Kill.SeveredLimbs, Kill.Location);
	}
	if (Kill.Cause == EZMBKillCause::Vehicle)
	{
		ReportHype(ZR::EHypeEvent::VehicleKill, 1, Kill.Location);
	}
	if (Kill.Cause == EZMBKillCause::Trap)
	{
		ReportHype(ZR::EHypeEvent::TrapKill, 1, Kill.Location);
	}
	if (Kill.bSlowMo)
	{
		ReportHype(ZR::EHypeEvent::SlowMoKill, 1, Kill.Location);
	}
	if (Kill.Cause == EZMBKillCause::Bullet && Kill.DistanceCm > 4000.0f)
	{
		ReportHype(ZR::EHypeEvent::LongShot, 1, Kill.Location);
	}
	if (Kill.bNamed)
	{
		ReportHype(ZR::EHypeEvent::NamedZombieKill, 1, Kill.Location);
		AddChat(FString(), FText::Format(LOCTEXT("NamedKill", "O zumbi de {0} já era. F no chat."), FText::FromString(Kill.ViewerName)).ToString(), Gold, true);
		React("zumbi_do_chat");
		return;
	}
	if (Kill.Cause == EZMBKillCause::Vehicle)
	{
		React("atropelo");
	}
	else if (Kill.Cause == EZMBKillCause::Trap)
	{
		React("armadilha");
	}
	else if (Kill.bHeadshot)
	{
		React("cabeca");
	}
	else if (Kill.SeveredLimbs > 0)
	{
		React("desmembrou");
	}
	else if (FMath::FRand() < 0.35f)
	{
		React("abate");
	}
}

void UZMBStreamSubsystem::ReportExplosion(int32 ExplosionKills, int32 Gibs, int32 ChainDepth, const FVector& Where)
{
	if (ExplosionKills >= 2)
	{
		ReportHype(ZR::EHypeEvent::MultiKill, ExplosionKills - 1, Where);
	}
	// a cadeia conta as explosões em sequência: a primeira é a profundidade 0
	Stats.BestChain = FMath::Max(Stats.BestChain, ChainDepth + 1);
	if (ChainDepth >= 1)
	{
		ReportHype(ZR::EHypeEvent::ChainExplosion, 1, Where);
		React("cadeia");
	}
	else
	{
		React("explosao");
	}
}

void UZMBStreamSubsystem::ReportPlayerDamage(float Amount, float HealthAfter)
{
	ReportHype(ZR::EHypeEvent::TookHit, 1, FVector::ZeroVector);
	if (HealthAfter < 25.0f)
	{
		React("quase_morte");
	}
	else if (FMath::FRand() < 0.3f)
	{
		React("dano");
	}
}

void UZMBStreamSubsystem::OnPlayerDied()
{
	++Stats.Deaths;
	State.Hype = 0.0f;
	Announce(LOCTEXT("Died", "VOCÊ MORREU. A LIVE CAIU..."));
	AddChat(FString(), LOCTEXT("DiedChat", "Conexão perdida. Os espectadores estão saindo.").ToString(), Hot, true);
}

char UZMBStreamSubsystem::GetLiveGrade() const
{
	const UZMBGameData* Data = UZMBGameData::Get(this);
	return ZR::LiveGrade(Data ? Data->Tuning().Grade : ZR::FGradeConfig(), Stats);
}

bool UZMBStreamSubsystem::GetNextMilestone(FString& OutText, float& OutFollowers) const
{
	const UZMBGameData* Data = UZMBGameData::Get(this);
	const int32 Index = Data ? Milestones.NextIndex(Data->Milestones()) : -1;
	if (Index < 0)
	{
		return false;
	}
	const ZR::FMilestoneDef& Def = Data->Milestones()[static_cast<size_t>(Index)];
	OutText = UZMBGameData::ToFString(bPortuguese || Def.TextEn.empty() ? Def.TextPt : Def.TextEn);
	OutFollowers = Def.Followers;
	return true;
}

void UZMBStreamSubsystem::HandleMilestone(int32 Index)
{
	const UZMBGameData* Data = UZMBGameData::Get(this);
	if (!Data || Index < 0 || Index >= static_cast<int32>(Data->Milestones().size()))
	{
		return;
	}
	const ZR::FMilestoneDef& Def = Data->Milestones()[static_cast<size_t>(Index)];
	const FText Text = FText::FromString(UZMBGameData::ToFString(bPortuguese || Def.TextEn.empty() ? Def.TextPt : Def.TextEn));
	Announce(FText::Format(LOCTEXT("Milestone", "{0}!"), Text));
	AddChat(FString(), FText::Format(LOCTEXT("MilestoneChat", "Meta batida: {0}. O chat mandou presente!"), Text).ToString(), Gold, true);
	React("marco");
	UZMBAudioSubsystem::Play2D(this, "pauta_ok");
	State.Hype = FMath::Min(100.0f, State.Hype + Def.HypeBonus);
	Highlights.Add(LiveSeconds, Def.HypeBonus, std::string(TCHAR_TO_UTF8(*Text.ToString())));
	if (AZMBCharacter* Player = Cast<AZMBCharacter>(UGameplayStatics::GetPlayerCharacter(this, 0)))
	{
		Player->AddScrap(Def.Scrap);
		Player->AddThrowables(Def.Grenades, Def.Bombs);
	}
	if (Def.bAirdrop)
	{
		FZMBChatAction Action;
		Action.Kind = EZMBChatActionKind::Airdrop;
		PendingActions.Add(Action);
	}
}

void UZMBStreamSubsystem::OnPlayerRespawned()
{
	Announce(LOCTEXT("Back", "DE VOLTA AO VIVO"));
}

void UZMBStreamSubsystem::React(const char* Category)
{
	if (PendingReactions.Num() >= 4 || IsTwitchConnected())
	{
		return;
	}
	PendingReactions.Add(TPair<float, FString>(FMath::FRandRange(0.3f, 1.4f), FString(UTF8_TO_TCHAR(Category))));
}

void UZMBStreamSubsystem::Announce(const FText& Text)
{
	Announcement = Text;
	AnnouncementAge = 0.0f;
}

void UZMBStreamSubsystem::ShowToast(const FString& Text)
{
	Toast = Text;
	ToastAge = 0.0f;
}

void UZMBStreamSubsystem::Clip()
{
	const ZR::FHighlightTracker::FWindow Window = Highlights.BestWindow(LiveSeconds, 20.0);
	++ClipCount;
	++Stats.Clips;
	UZMBAudioSubsystem::Play2D(this, "clipe");
	if (Window.bValid)
	{
		Toast = FText::Format(LOCTEXT("ClipSaved", "CLIPE {0} SALVO: {1} (nota {2})"), FText::AsNumber(ClipCount),
			FText::FromString(UZMBGameData::ToFString(Window.Label)), FText::AsNumber(FMath::RoundToInt(Window.Score))).ToString();
	}
	else
	{
		Toast = FText::Format(LOCTEXT("ClipEmpty", "CLIPE {0} SALVO"), FText::AsNumber(ClipCount)).ToString();
	}
	ToastAge = 0.0f;
	// o print sai com o HUD da live (Saved/Screenshots); a janela do lance fica no log para a Gravação de Jogo da Steam
	FScreenshotRequest::RequestScreenshot(FString::Printf(TEXT("ZMB_Clipe_%03d"), ClipCount), true, true);
	UE_LOG(LogZombie, Display, TEXT("Clipe %d: %.1f s a %.1f s da live, nota %.0f, '%s'"), ClipCount, Window.Start, Window.End, Window.Score,
		*UZMBGameData::ToFString(Window.Label));
}

FString UZMBStreamSubsystem::GetChallengeText() const
{
	const UZMBGameData* Data = UZMBGameData::Get(this);
	const int32 Index = Challenges.ActiveIndex();
	if (!Data || Index < 0 || Index >= static_cast<int32>(Data->Challenges().size()))
	{
		return FString();
	}
	const ZR::FChallengeDef& Def = Data->Challenges()[static_cast<size_t>(Index)];
	return UZMBGameData::ToFString(bPortuguese || Def.TextEn.empty() ? Def.TextPt : Def.TextEn);
}

int32 UZMBStreamSubsystem::GetChallengeTarget() const
{
	const UZMBGameData* Data = UZMBGameData::Get(this);
	const int32 Index = Challenges.ActiveIndex();
	if (!Data || Index < 0 || Index >= static_cast<int32>(Data->Challenges().size()))
	{
		return 0;
	}
	return Data->Challenges()[static_cast<size_t>(Index)].Target;
}

void UZMBStreamSubsystem::HandleChallenge(const ZR::FChallengeUpdate& Update)
{
	const UZMBGameData* Data = UZMBGameData::Get(this);
	if (!Data || Update.Result == ZR::EChallengeResult::None || Update.Index < 0 || Update.Index >= static_cast<int32>(Data->Challenges().size()))
	{
		return;
	}
	const ZR::FChallengeDef& Def = Data->Challenges()[static_cast<size_t>(Update.Index)];
	const FText Text = FText::FromString(UZMBGameData::ToFString(bPortuguese || Def.TextEn.empty() ? Def.TextPt : Def.TextEn));
	switch (Update.Result)
	{
	case ZR::EChallengeResult::Started:
		Announce(FText::Format(LOCTEXT("ChallengeStart", "PATROCÍNIO: {0} EM {1} S"), Text, FText::AsNumber(FMath::RoundToInt(Def.TimeLimitSeconds))));
		UZMBAudioSubsystem::Play2D(this, "pauta_nova");
		AddChat(FString(), FText::Format(LOCTEXT("ChallengeChat", "Pauta do patrocinador: {0} (paga R$ {1})"), Text, FText::AsNumber(FMath::RoundToInt(Def.Reward))).ToString(), Gold, true);
		break;
	case ZR::EChallengeResult::Completed:
		Money += Update.Reward;
		++Stats.ChallengesDone;
		State.Hype = FMath::Min(100.0f, State.Hype + Update.HypeBonus);
		UZMBAudioSubsystem::Play2D(this, "pauta_ok");
		Highlights.Add(LiveSeconds, Update.HypeBonus, std::string(TCHAR_TO_UTF8(*Text.ToString())));
		AddPopup(FText::Format(LOCTEXT("ChallengeDone", "PAUTA CUMPRIDA  +R$ {0}"), FText::AsNumber(FMath::RoundToInt(Update.Reward))).ToString(), Gold);
		AddChat(FString(), FText::Format(LOCTEXT("ChallengePaid", "O patrocinador pagou R$ {0}!"), FText::AsNumber(FMath::RoundToInt(Update.Reward))).ToString(), Gold, true);
		React("doacao");
		break;
	case ZR::EChallengeResult::Failed:
		++Stats.ChallengesFailed;
		AddChat(FString(), LOCTEXT("ChallengeFailed", "O patrocinador desistiu da pauta. Fica para a próxima.").ToString(), Hot, true);
		React("calmo");
		break;
	default:
		break;
	}
}

void UZMBStreamSubsystem::ConsumeChatActions(TArray<FZMBChatAction>& Out)
{
	Out = MoveTemp(PendingActions);
	PendingActions.Reset();
}

void UZMBStreamSubsystem::HandleTwitchMessage(const FZMBTwitchMessage& Message)
{
	const std::string Raw(TCHAR_TO_UTF8(*Message.Text));
	// Modo Streamer: o que aparece na tela passa pelo filtro; o comando é lido do texto original
	const UZMBGameData* Data = UZMBGameData::Get(this);
	const bool bFilter = Data && Data->Tuning().ChatFilterOn != 0;
	const FString Shown = bFilter ? UZMBGameData::ToFString(Data->WordFilter().Mask(Raw)) : Message.Text;
	AddChat(CleanViewerName(Message.User), Shown, Message.Color, false);
	HandleCommand(Message.User, Raw);
}

FString UZMBStreamSubsystem::CleanViewerName(const FString& User) const
{
	const UZMBGameData* Data = UZMBGameData::Get(this);
	if (Data && Data->Tuning().ChatFilterOn != 0 && Data->WordFilter().IsOffensiveName(std::string(TCHAR_TO_UTF8(*User))))
	{
		return LOCTEXT("HiddenViewer", "espectador").ToString();
	}
	return User;
}

void UZMBStreamSubsystem::HandleCommand(const FString& User, const std::string& Text)
{
	const ZR::EChatCommand Command = ZR::ParseChatCommand(Text);
	const std::string Name(TCHAR_TO_UTF8(*User));
	if (Command == ZR::EChatCommand::Zombie)
	{
		if (Voting.RequestNamedZombie(Name))
		{
			FZMBChatAction Action;
			Action.Kind = EZMBChatActionKind::NamedZombie;
			Action.ViewerName = CleanViewerName(User);
			PendingActions.Add(Action);
		}
	}
	else if (Command != ZR::EChatCommand::None)
	{
		Voting.CastVote(Name, Command);
	}
}

void UZMBStreamSubsystem::TickFakeChat(float DeltaTime)
{
	if (!FakeChat)
	{
		return;
	}
	FakeChatTimer -= DeltaTime;
	if (FakeChatTimer <= 0.0f)
	{
		// mais gente assistindo, chat mais rápido
		FakeChatTimer = FMath::Clamp(6.0f / (1.0f + State.Viewers / 150.0f), 0.35f, 6.0f) * FMath::FRandRange(0.6f, 1.4f);
		if (State.Hype < 15.0f && State.Viewers >= 1.0f)
		{
			React("calmo");
		}
		else if (State.Viewers >= 1.0f && FMath::FRand() < 0.5f)
		{
			React("abate");
		}
	}

	// o chat simulado também vota: com hype alto pede horda; baixo, pede caixa
	FakeVoteTimer -= DeltaTime;
	if (FakeVoteTimer <= 0.0f)
	{
		FakeVoteTimer = FMath::FRandRange(8.0f, 15.0f);
		const int32 Voters = FMath::Clamp(FMath::FloorToInt(State.Viewers / 40.0f), 0, 8);
		for (int32 i = 0; i < Voters; ++i)
		{
			const float Roll = FMath::FRand();
			const float HordeChance = 0.25f + State.Hype / 200.0f;
			const ZR::EChatCommand Option = Roll < HordeChance ? ZR::EChatCommand::Horde
				: (Roll < HordeChance + 0.3f ? ZR::EChatCommand::Bomb : ZR::EChatCommand::Airdrop);
			const std::string Voter = FakeChat->FakeUserName();
			if (Voting.CastVote(Voter, Option))
			{
				const TCHAR* Word = Option == ZR::EChatCommand::Horde ? TEXT("!horda") : (Option == ZR::EChatCommand::Bomb ? TEXT("!bujao") : TEXT("!caixa"));
				const FString VoterName = UZMBGameData::ToFString(Voter);
				AddChat(VoterName, Word, NameColor(VoterName), false);
			}
		}
	}

	// e manda zumbi com o próprio nome
	FakeZombieTimer -= DeltaTime;
	if (FakeZombieTimer <= 0.0f)
	{
		FakeZombieTimer = FMath::FRandRange(30.0f, 60.0f);
		if (State.Viewers >= 20.0f)
		{
			const std::string Viewer = FakeChat->FakeUserName();
			const FString ViewerName = UZMBGameData::ToFString(Viewer);
			AddChat(ViewerName, TEXT("!zumbi"), NameColor(ViewerName), false);
			HandleCommand(ViewerName, "!zumbi");
		}
	}
}

void UZMBStreamSubsystem::Tick(float DeltaTime)
{
	LiveSeconds += DeltaTime;

	std::vector<ZR::FDonation> Donations;
	ZR::TickHype(Config, State, DeltaTime, Donations);
	Stats.Sample(LiveSeconds, State.Viewers, State.Followers, State.Combo);
	if (const UZMBGameData* Data = UZMBGameData::Get(this))
	{
		for (const int Crossed : Milestones.Update(Data->Milestones(), State.Followers))
		{
			HandleMilestone(Crossed);
		}
	}
	for (const ZR::FDonation& Donation : Donations)
	{
		Money += Donation.Amount;
		Stats.Donations += Donation.Amount;
		UZMBAudioSubsystem::Play2D(this, "doacao");
		const FString Donor = FakeChat ? UZMBGameData::ToFString(FakeChat->FakeUserName()) : FString(TEXT("anonimo"));
		const ZR::FChatLine* Line = FakeChat ? FakeChat->Pick("doacao") : nullptr;
		const FString Words = Line ? UZMBGameData::ToFString(bPortuguese ? Line->TextPt : Line->TextEn) : FString();
		AddChat(Donor, FText::Format(LOCTEXT("Donation", "doou R$ {0}: {1}"), FText::AsNumber(FMath::RoundToInt(Donation.Amount)), FText::FromString(Words)).ToString(), Gold, true);
		AddPopup(FText::Format(LOCTEXT("DonationPopup", "DOAÇÃO R$ {0}"), FText::AsNumber(FMath::RoundToInt(Donation.Amount))).ToString(), Gold);
	}
	while (Money >= AirdropPrice)
	{
		Money -= AirdropPrice;
		FZMBChatAction Action;
		Action.Kind = EZMBChatActionKind::Airdrop;
		PendingActions.Add(Action);
		AddChat(FString(), LOCTEXT("Sponsor", "As doações pagaram uma entrega por drone!").ToString(), Gold, true);
	}

	if (const UZMBGameData* Data = UZMBGameData::Get(this))
	{
		const AZMBHordeDirector* Director = AZMBHordeDirector::Find(this);
		HandleChallenge(Challenges.Tick(Data->Challenges(), DeltaTime, State.Viewers, Director && Director->IsNight()));
	}

	const ZR::EChatCommand Winner = Voting.Tick(DeltaTime);
	if (Winner != ZR::EChatCommand::None)
	{
		FZMBChatAction Action;
		Action.Kind = Winner == ZR::EChatCommand::Horde ? EZMBChatActionKind::Horde
			: (Winner == ZR::EChatCommand::Bomb ? EZMBChatActionKind::BombSquad : EZMBChatActionKind::Airdrop);
		PendingActions.Add(Action);
		AddChat(FString(), LOCTEXT("VoteWon", "O chat votou e decidiu!").ToString(), Hot, true);
	}

	if (UGameInstance* GameInstance = GetWorld() ? GetWorld()->GetGameInstance() : nullptr)
	{
		if (UZMBTwitchChat* Twitch = GameInstance->GetSubsystem<UZMBTwitchChat>())
		{
			Twitch->Update(DeltaTime);
		}
	}
	if (!IsTwitchConnected())
	{
		TickFakeChat(DeltaTime);
	}

	for (int32 i = PendingReactions.Num() - 1; i >= 0; --i)
	{
		PendingReactions[i].Key -= DeltaTime;
		if (PendingReactions[i].Key > 0.0f)
		{
			continue;
		}
		const FString Category = PendingReactions[i].Value;
		PendingReactions.RemoveAt(i);
		const ZR::FChatLine* Line = FakeChat ? FakeChat->Pick(std::string(TCHAR_TO_UTF8(*Category))) : nullptr;
		if (Line)
		{
			const FString Viewer = UZMBGameData::ToFString(FakeChat->FakeUserName());
			AddChat(Viewer, UZMBGameData::ToFString(bPortuguese ? Line->TextPt : Line->TextEn), NameColor(Viewer), false);
		}
	}

	for (int32 i = Chat.Num() - 1; i >= 0; --i)
	{
		Chat[i].Age += DeltaTime;
		if (Chat[i].Age > ChatLineSeconds)
		{
			Chat.RemoveAt(i);
		}
	}
	for (int32 i = Popups.Num() - 1; i >= 0; --i)
	{
		Popups[i].Age += DeltaTime;
		if (Popups[i].Age > PopupSeconds)
		{
			Popups.RemoveAt(i);
		}
	}
	AnnouncementAge += DeltaTime;
	ToastAge += DeltaTime;
	Highlights.Prune(LiveSeconds);
}

#undef LOCTEXT_NAMESPACE
