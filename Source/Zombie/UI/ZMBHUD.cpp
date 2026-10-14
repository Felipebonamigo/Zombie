#include "UI/ZMBHUD.h"
#include "Zombie.h"
#include "Game/ZMBGameData.h"
#include "Game/ZMBGameMode.h"
#include "Game/ZMBWorldBuilder.h"
#include "Base/ZMBBuildPiece.h"
#include "Player/ZMBCharacter.h"
#include "Rules/ZRChat.h"
#include "Stream/ZMBStreamSubsystem.h"
#include "Vehicles/ZMBCar.h"
#include "Zombies/ZMBHordeDirector.h"
#include "Engine/Canvas.h"
#include "Engine/Engine.h"
#include "Engine/Font.h"

#define LOCTEXT_NAMESPACE "ZombieHUD"

namespace
{
	const FLinearColor LiveRed(0.85f, 0.04f, 0.04f);
	const FLinearColor Panel(0.0f, 0.0f, 0.0f, 0.38f);
	const FLinearColor Gold(1.0f, 0.78f, 0.15f);
	const FLinearColor Soft(0.82f, 0.82f, 0.85f);

	FString Thousands(float Value)
	{
		return FText::AsNumber(FMath::RoundToInt(Value)).ToString();
	}

	FString Shorten(const FString& Text, int32 MaxChars)
	{
		return Text.Len() <= MaxChars ? Text : Text.Left(MaxChars - 3) + TEXT("...");
	}
}

void AZMBHUD::DrawShadowedText(const FString& Text, const FLinearColor& Color, float X, float Y, UFont* Font, float TextScale)
{
	DrawText(Text, FLinearColor(0.0f, 0.0f, 0.0f, 0.75f * Color.A), X + 2.0f, Y + 2.0f, Font, TextScale);
	DrawText(Text, Color, X, Y, Font, TextScale);
}

void AZMBHUD::DrawBar(float X, float Y, float Width, float Height, float Fill, const FLinearColor& Color)
{
	DrawRect(Panel, X, Y, Width, Height);
	DrawRect(Color, X + 2.0f, Y + 2.0f, FMath::Max(0.0f, (Width - 4.0f) * FMath::Clamp(Fill, 0.0f, 1.0f)), Height - 4.0f);
}

void AZMBHUD::DrawHUD()
{
	Super::DrawHUD();
	if (!Canvas)
	{
		return;
	}
	Clock += GetWorld() ? GetWorld()->GetDeltaSeconds() : 0.016f;
	const float Scale = Canvas->ClipY / 1080.0f;
	const AZMBCharacter* Player = Cast<AZMBCharacter>(GetOwningPawn());
	if (Player && Player->IsFirstPerson() && !Player->IsDead())
	{
		DrawPhoneFrame(Scale);
	}
	DrawLiveBadge(Scale);
	DrawClock(Scale);
	DrawChat(Scale);
	DrawVote(Scale);
	DrawPopups(Scale);
	DrawPlayer(Scale);
	if (Player && !Player->IsDead())
	{
		DrawCrosshair(Scale);
		DrawBuild(Scale);
	}
	if (Player && Player->IsHelpVisible())
	{
		DrawHelp(Scale);
	}
}

void AZMBHUD::DrawLiveBadge(float Scale)
{
	const UZMBStreamSubsystem* Stream = UZMBStreamSubsystem::Get(this);
	if (!Stream)
	{
		return;
	}
	UFont* Medium = GEngine->GetMediumFont();
	UFont* Small = GEngine->GetSmallFont();
	const float X = 24.0f * Scale;
	float Y = 24.0f * Scale;

	// selo AO VIVO piscando devagar
	const float Pulse = 0.75f + 0.25f * FMath::Sin(Clock * 4.0f);
	DrawRect(LiveRed * Pulse, X, Y, 132.0f * Scale, 38.0f * Scale);
	DrawShadowedText(LOCTEXT("Live", "AO VIVO").ToString(), FLinearColor::White, X + 14.0f * Scale, Y + 6.0f * Scale, Medium, 1.3f * Scale);
	DrawShadowedText(FText::Format(LOCTEXT("Viewers", "{0} assistindo"), FText::FromString(Thousands(Stream->GetViewers()))).ToString(),
		FLinearColor::White, X + 148.0f * Scale, Y + 6.0f * Scale, Medium, 1.3f * Scale);
	Y += 48.0f * Scale;
	DrawShadowedText(FText::Format(LOCTEXT("Followers", "{0} seguidores"), FText::FromString(Thousands(Stream->GetFollowers()))).ToString(),
		Soft, X, Y, Small, 1.4f * Scale);
	Y += 30.0f * Scale;

	// hype: amarelo, laranja, vermelho
	const float Hype = Stream->GetHype() / 100.0f;
	const FLinearColor HypeColor = FMath::Lerp(FLinearColor(1.0f, 0.85f, 0.1f), FLinearColor(1.0f, 0.1f, 0.05f), Hype);
	DrawShadowedText(LOCTEXT("Hype", "HYPE").ToString(), FLinearColor::White, X, Y, Small, 1.3f * Scale);
	DrawBar(X + 60.0f * Scale, Y + 2.0f * Scale, 300.0f * Scale, 18.0f * Scale, Hype, HypeColor);
	if (Stream->GetCombo() > 1)
	{
		DrawShadowedText(FString::Printf(TEXT("COMBO %d  x%.2f"), Stream->GetCombo(), Stream->GetComboMultiplier()), Gold,
			X + 372.0f * Scale, Y - 2.0f * Scale, Medium, 1.1f * Scale);
	}
	Y += 30.0f * Scale;
	DrawShadowedText(FText::Format(LOCTEXT("Money", "Doações: R$ {0} de 50 para o próximo drone"), FText::AsNumber(FMath::FloorToInt(Stream->GetMoney()))).ToString(),
		Gold, X, Y, Small, 1.2f * Scale);
	Y += 24.0f * Scale;
	const FString Source = Stream->IsTwitchConnected()
		? FText::Format(LOCTEXT("TwitchOn", "Chat: twitch.tv/{0}"), FText::FromString(Stream->GetTwitchChannel())).ToString()
		: LOCTEXT("TwitchOff", "Chat simulado (console: zmb.twitch seucanal)").ToString();
	DrawShadowedText(Source, Soft, X, Y, Small, 1.0f * Scale);
	Y += 22.0f * Scale;
	DrawShadowedText(FText::Format(LOCTEXT("Kills", "Abates: {0}"), FText::AsNumber(Stream->GetKills())).ToString(), Soft, X, Y, Small, 1.0f * Scale);

	// pauta do patrocinador: o que fazer, quanto falta e o tempo
	const FString Challenge = Stream->GetChallengeText();
	if (!Challenge.IsEmpty())
	{
		Y += 34.0f * Scale;
		const int32 Seconds = FMath::Max(0, FMath::CeilToInt(Stream->GetChallengeTimeLeft()));
		const float Fill = static_cast<float>(Stream->GetChallengeProgress()) / static_cast<float>(FMath::Max(1, Stream->GetChallengeTarget()));
		DrawRect(Panel, X - 6.0f * Scale, Y - 6.0f * Scale, 470.0f * Scale, 62.0f * Scale);
		DrawShadowedText(FText::Format(LOCTEXT("Challenge", "PAUTA: {0}"), FText::FromString(Challenge)).ToString(), Gold, X, Y, Small, 1.15f * Scale);
		DrawBar(X, Y + 28.0f * Scale, 330.0f * Scale, 16.0f * Scale, Fill, Gold);
		DrawShadowedText(FString::Printf(TEXT("%d/%d   %d:%02d"), Stream->GetChallengeProgress(), Stream->GetChallengeTarget(), Seconds / 60, Seconds % 60),
			Seconds <= 10 ? LiveRed : FLinearColor::White, X + 342.0f * Scale, Y + 24.0f * Scale, Small, 1.1f * Scale);
		// o que vem embaixo (aviso de erro nos dados) fica abaixo da caixa
		Y += 40.0f * Scale;
	}

	if (const UZMBGameData* Data = UZMBGameData::Get(this))
	{
		if (Data->DataErrors().Num() > 0)
		{
			Y += 22.0f * Scale;
			DrawShadowedText(FText::Format(LOCTEXT("DataErrors", "{0} erro(s) em Data/*.csv (ver o log)"), FText::AsNumber(Data->DataErrors().Num())).ToString(),
				LiveRed, X, Y, Small, 1.0f * Scale);
		}
	}
}

void AZMBHUD::DrawClock(float Scale)
{
	const AZMBGameMode* GameMode = AZMBGameMode::Get(this);
	const AZMBWorldBuilder* Builder = GameMode ? GameMode->GetBuilder() : nullptr;
	if (!Builder)
	{
		return;
	}
	UFont* Medium = GEngine->GetMediumFont();
	UFont* Small = GEngine->GetSmallFont();
	const float Hour = Builder->GetHour();
	const int32 Hours = FMath::FloorToInt(Hour);
	const int32 Minutes = FMath::FloorToInt((Hour - Hours) * 60.0f);
	const bool bNight = Builder->IsNightNow();
	const FString Text = FString::Printf(TEXT("%02d:%02d  %s"), Hours, Minutes,
		*(bNight ? LOCTEXT("Night", "NOITE") : LOCTEXT("Day", "DIA")).ToString());
	float Width = 0.0f;
	float Height = 0.0f;
	GetTextSize(Text, Width, Height, Medium, 1.3f * Scale);
	const float X = (Canvas->ClipX - Width) * 0.5f;
	DrawShadowedText(Text, bNight ? FLinearColor(0.55f, 0.7f, 1.0f) : FLinearColor::White, X, 22.0f * Scale, Medium, 1.3f * Scale);
	if (const AZMBHordeDirector* Director = AZMBHordeDirector::Find(this))
	{
		const TCHAR* Phase = Director->GetPhase() == ZR::EDirectorPhase::Peak ? TEXT("pico")
			: (Director->GetPhase() == ZR::EDirectorPhase::Relax ? TEXT("respiro") : TEXT("tensão"));
		const FString Info = FString::Printf(TEXT("%s%s  ·  %d zumbis"), Director->IsHordeActive() ? TEXT("HORDA!  ") : TEXT(""), Phase, Director->AliveCount());
		GetTextSize(Info, Width, Height, Small, 1.1f * Scale);
		DrawShadowedText(Info, Director->IsHordeActive() ? LiveRed : Soft, (Canvas->ClipX - Width) * 0.5f, 58.0f * Scale, Small, 1.1f * Scale);
	}
}

void AZMBHUD::DrawChat(float Scale)
{
	const UZMBStreamSubsystem* Stream = UZMBStreamSubsystem::Get(this);
	if (!Stream || Stream->GetChat().Num() == 0)
	{
		return;
	}
	UFont* Small = GEngine->GetSmallFont();
	const float Width = 470.0f * Scale;
	const float Line = 27.0f * Scale;
	const float X = Canvas->ClipX - Width - 24.0f * Scale;
	const TArray<FZMBChatEntry>& Lines = Stream->GetChat();
	float Y = Canvas->ClipY * 0.30f;
	DrawRect(Panel, X - 10.0f * Scale, Y - 8.0f * Scale, Width + 20.0f * Scale, Lines.Num() * Line + 16.0f * Scale);
	for (const FZMBChatEntry& Entry : Lines)
	{
		const float Alpha = FMath::Clamp(1.0f - (Entry.Age - 24.0f) / 6.0f, 0.0f, 1.0f);
		if (Entry.bSystem)
		{
			DrawShadowedText(Shorten(Entry.Name.IsEmpty() ? Entry.Text : Entry.Name + TEXT(" ") + Entry.Text, 52), Gold * FLinearColor(1, 1, 1, Alpha), X, Y, Small, 1.15f * Scale);
		}
		else
		{
			const FString Name = Entry.Name + TEXT(":");
			float NameWidth = 0.0f;
			float NameHeight = 0.0f;
			GetTextSize(Name, NameWidth, NameHeight, Small, 1.15f * Scale);
			FLinearColor NameColor = Entry.NameColor;
			NameColor.A = Alpha;
			DrawShadowedText(Name, NameColor, X, Y, Small, 1.15f * Scale);
			DrawShadowedText(Shorten(Entry.Text, FMath::Max(8, 50 - Entry.Name.Len())), FLinearColor(1, 1, 1, Alpha), X + NameWidth + 8.0f * Scale, Y, Small, 1.15f * Scale);
		}
		Y += Line;
	}
}

void AZMBHUD::DrawVote(float Scale)
{
	const UZMBStreamSubsystem* Stream = UZMBStreamSubsystem::Get(this);
	if (!Stream)
	{
		return;
	}
	const int32 Horde = Stream->GetVotes(ZR::EChatCommand::Horde);
	const int32 Bomb = Stream->GetVotes(ZR::EChatCommand::Bomb);
	const int32 Box = Stream->GetVotes(ZR::EChatCommand::Airdrop);
	if (Horde + Bomb + Box == 0)
	{
		return;
	}
	UFont* Small = GEngine->GetSmallFont();
	const float X = Canvas->ClipX - 494.0f * Scale;
	const float Y = Canvas->ClipY * 0.30f - 64.0f * Scale;
	DrawShadowedText(FText::Format(LOCTEXT("Vote", "VOTAÇÃO DO CHAT ({0} s)"), FText::AsNumber(FMath::CeilToInt(Stream->GetVoteTimeLeft()))).ToString(),
		Gold, X, Y, Small, 1.2f * Scale);
	DrawShadowedText(FString::Printf(TEXT("!horda %d    !bujao %d    !caixa %d"), Horde, Bomb, Box), FLinearColor::White, X, Y + 26.0f * Scale, Small, 1.2f * Scale);
}

void AZMBHUD::DrawPopups(float Scale)
{
	const UZMBStreamSubsystem* Stream = UZMBStreamSubsystem::Get(this);
	if (!Stream)
	{
		return;
	}
	UFont* Large = GEngine->GetLargeFont();
	UFont* Medium = GEngine->GetMediumFont();
	const TArray<FZMBPopup>& Popups = Stream->GetPopups();
	for (int32 i = 0; i < Popups.Num(); ++i)
	{
		const FZMBPopup& Popup = Popups[Popups.Num() - 1 - i];
		const float Pop = Popup.Age < 0.12f ? 1.0f + (0.12f - Popup.Age) * 3.0f : 1.0f;
		FLinearColor Color = Popup.Color;
		Color.A = FMath::Clamp(1.0f - (Popup.Age - 1.1f) / 0.5f, 0.0f, 1.0f);
		const float TextScale = 1.1f * Scale * Pop;
		float Width = 0.0f;
		float Height = 0.0f;
		GetTextSize(Popup.Text, Width, Height, Medium, TextScale);
		DrawShadowedText(Popup.Text, Color, (Canvas->ClipX - Width) * 0.5f, Canvas->ClipY * 0.36f - i * 34.0f * Scale - Popup.Age * 20.0f * Scale, Medium, TextScale);
	}

	if (Stream->GetAnnouncementAge() < 3.5f)
	{
		const FString Text = Stream->GetAnnouncement().ToString();
		FLinearColor Color = FLinearColor::White;
		Color.A = FMath::Clamp(3.5f - Stream->GetAnnouncementAge(), 0.0f, 1.0f);
		float Width = 0.0f;
		float Height = 0.0f;
		GetTextSize(Text, Width, Height, Large, 2.2f * Scale);
		const float Y = Canvas->ClipY * 0.17f;
		DrawRect(FLinearColor(0.5f, 0.0f, 0.0f, 0.55f * Color.A), 0.0f, Y - 10.0f * Scale, Canvas->ClipX, Height + 20.0f * Scale);
		DrawShadowedText(Text, Color, (Canvas->ClipX - Width) * 0.5f, Y, Large, 2.2f * Scale);
	}
	if (Stream->GetToastAge() < 3.0f)
	{
		float Width = 0.0f;
		float Height = 0.0f;
		GetTextSize(Stream->GetToast(), Width, Height, Medium, 1.2f * Scale);
		DrawShadowedText(Stream->GetToast(), Gold, (Canvas->ClipX - Width) * 0.5f, Canvas->ClipY * 0.26f, Medium, 1.2f * Scale);
	}
}

void AZMBHUD::DrawCrosshair(float Scale)
{
	const AZMBCharacter* Player = Cast<AZMBCharacter>(GetOwningPawn());
	if (!Player || Player->IsDriving())
	{
		return;
	}
	const float CenterX = Canvas->ClipX * 0.5f;
	const float CenterY = Canvas->ClipY * 0.5f;
	const float Gap = (6.0f + Player->GetSpread01() * 40.0f) * Scale;
	const float Length = 10.0f * Scale;
	const FLinearColor Color = Player->IsSlowMo() ? Gold : FLinearColor(1.0f, 1.0f, 1.0f, 0.9f);
	DrawLine(CenterX - Gap - Length, CenterY, CenterX - Gap, CenterY, Color, 2.0f * Scale);
	DrawLine(CenterX + Gap, CenterY, CenterX + Gap + Length, CenterY, Color, 2.0f * Scale);
	DrawLine(CenterX, CenterY - Gap - Length, CenterX, CenterY - Gap, Color, 2.0f * Scale);
	DrawLine(CenterX, CenterY + Gap, CenterX, CenterY + Gap + Length, Color, 2.0f * Scale);
	if (Player->GetHitMarkerAge() < 0.15f)
	{
		const float Hit = 9.0f * Scale;
		const FLinearColor Red(1.0f, 0.15f, 0.1f);
		DrawLine(CenterX - Hit, CenterY - Hit, CenterX - Hit * 0.4f, CenterY - Hit * 0.4f, Red, 2.5f * Scale);
		DrawLine(CenterX + Hit, CenterY - Hit, CenterX + Hit * 0.4f, CenterY - Hit * 0.4f, Red, 2.5f * Scale);
		DrawLine(CenterX - Hit, CenterY + Hit, CenterX - Hit * 0.4f, CenterY + Hit * 0.4f, Red, 2.5f * Scale);
		DrawLine(CenterX + Hit, CenterY + Hit, CenterX + Hit * 0.4f, CenterY + Hit * 0.4f, Red, 2.5f * Scale);
	}
}

void AZMBHUD::DrawPlayer(float Scale)
{
	UFont* Large = GEngine->GetLargeFont();
	UFont* Medium = GEngine->GetMediumFont();
	UFont* Small = GEngine->GetSmallFont();
	const float Left = 24.0f * Scale;
	const float Bottom = Canvas->ClipY - 24.0f * Scale;

	if (const AZMBCar* Car = Cast<AZMBCar>(GetOwningPawn()))
	{
		const FString Speed = FString::Printf(TEXT("%d km/h"), FMath::RoundToInt(FMath::Abs(Car->GetSpeedKmh())));
		float Width = 0.0f;
		float Height = 0.0f;
		GetTextSize(Speed, Width, Height, Large, 2.0f * Scale);
		DrawShadowedText(Speed, FLinearColor::White, Canvas->ClipX - Width - 32.0f * Scale, Bottom - Height - 30.0f * Scale, Large, 2.0f * Scale);
		DrawShadowedText(LOCTEXT("CarHelp", "E sair   Espaço freio de mão   R desvirar").ToString(), Soft,
			Canvas->ClipX - 470.0f * Scale, Bottom - 22.0f * Scale, Small, 1.1f * Scale);
		return;
	}
	const AZMBCharacter* Player = Cast<AZMBCharacter>(GetOwningPawn());
	if (!Player)
	{
		return;
	}
	if (Player->IsDead())
	{
		const FString Dead = LOCTEXT("LiveDown", "A LIVE CAIU").ToString();
		float Width = 0.0f;
		float Height = 0.0f;
		GetTextSize(Dead, Width, Height, Large, 3.0f * Scale);
		DrawShadowedText(Dead, LiveRed, (Canvas->ClipX - Width) * 0.5f, Canvas->ClipY * 0.42f, Large, 3.0f * Scale);
		const FString Back = FText::Format(LOCTEXT("BackIn", "voltando em {0} s"), FText::AsNumber(FMath::CeilToInt(Player->GetRespawnSeconds()))).ToString();
		GetTextSize(Back, Width, Height, Medium, 1.4f * Scale);
		DrawShadowedText(Back, FLinearColor::White, (Canvas->ClipX - Width) * 0.5f, Canvas->ClipY * 0.52f, Medium, 1.4f * Scale);
		return;
	}

	// vida, fôlego e foco embaixo à esquerda
	const float BarWidth = 300.0f * Scale;
	const float BarHeight = 16.0f * Scale;
	float Y = Bottom - 3.0f * 34.0f * Scale;
	DrawShadowedText(LOCTEXT("Health", "VIDA").ToString(), FLinearColor::White, Left, Y - 2.0f * Scale, Small, 1.1f * Scale);
	DrawBar(Left + 96.0f * Scale, Y, BarWidth, BarHeight, Player->GetHealth() / FMath::Max(1.0f, Player->GetMaxHealth()), FLinearColor(0.85f, 0.08f, 0.06f));
	Y += 34.0f * Scale;
	DrawShadowedText(LOCTEXT("Stamina", "FÔLEGO").ToString(), FLinearColor::White, Left, Y - 2.0f * Scale, Small, 1.1f * Scale);
	DrawBar(Left + 96.0f * Scale, Y, BarWidth, BarHeight, Player->GetStamina01(), FLinearColor(0.85f, 0.85f, 0.8f));
	Y += 34.0f * Scale;
	DrawShadowedText(LOCTEXT("Focus", "FOCO (Q)").ToString(), FLinearColor::White, Left, Y - 2.0f * Scale, Small, 1.1f * Scale);
	DrawBar(Left + 96.0f * Scale, Y, BarWidth, BarHeight, Player->GetFocus01(), Player->IsSlowMo() ? Gold : FLinearColor(0.75f, 0.6f, 0.1f));

	// arma e munição embaixo à direita
	const FString Ammo = Player->IsReloading() ? LOCTEXT("Reloading", "recarregando...").ToString()
		: FString::Printf(TEXT("%d / %d"), Player->GetMagazine(), Player->GetReserve());
	float Width = 0.0f;
	float Height = 0.0f;
	GetTextSize(Ammo, Width, Height, Large, 1.8f * Scale);
	const float Right = Canvas->ClipX - 32.0f * Scale;
	DrawShadowedText(Ammo, Player->GetMagazine() == 0 ? LiveRed : FLinearColor::White, Right - Width, Bottom - Height - 34.0f * Scale, Large, 1.8f * Scale);
	const FString Weapon = Player->GetWeaponName();
	GetTextSize(Weapon, Width, Height, Medium, 1.2f * Scale);
	DrawShadowedText(Weapon, Soft, Right - Width, Bottom - 124.0f * Scale, Medium, 1.2f * Scale);
	const FString Throwables = FText::Format(LOCTEXT("Throwables", "G granadas {0}    H bujões {1}    X sucata {2}"), FText::AsNumber(Player->GetGrenades()),
		FText::AsNumber(Player->GetBombs()), FText::AsNumber(Player->GetScrap())).ToString();
	GetTextSize(Throwables, Width, Height, Small, 1.1f * Scale);
	DrawShadowedText(Throwables, Soft, Right - Width, Bottom - 22.0f * Scale, Small, 1.1f * Scale);
}

void AZMBHUD::DrawPhoneFrame(float Scale)
{
	const UZMBStreamSubsystem* Stream = UZMBStreamSubsystem::Get(this);
	UFont* Medium = GEngine->GetMediumFont();
	const float Margin = 40.0f * Scale;
	const float Arm = 70.0f * Scale;
	const float W = Canvas->ClipX;
	const float H = Canvas->ClipY;
	const FLinearColor Frame(1.0f, 1.0f, 1.0f, 0.8f);
	const float T = 3.0f * Scale;
	// cantos do enquadramento da câmera do celular
	DrawLine(Margin, Margin, Margin + Arm, Margin, Frame, T);
	DrawLine(Margin, Margin, Margin, Margin + Arm, Frame, T);
	DrawLine(W - Margin, Margin, W - Margin - Arm, Margin, Frame, T);
	DrawLine(W - Margin, Margin, W - Margin, Margin + Arm, Frame, T);
	DrawLine(Margin, H - Margin, Margin + Arm, H - Margin, Frame, T);
	DrawLine(Margin, H - Margin, Margin, H - Margin - Arm, Frame, T);
	DrawLine(W - Margin, H - Margin, W - Margin - Arm, H - Margin, Frame, T);
	DrawLine(W - Margin, H - Margin, W - Margin, H - Margin - Arm, Frame, T);
	// REC piscando, tempo de live e bateria
	const bool bBlink = FMath::Fmod(Clock, 1.0f) < 0.6f;
	const float RecX = W * 0.5f - 160.0f * Scale;
	if (bBlink)
	{
		DrawRect(LiveRed, RecX, H - Margin - 40.0f * Scale, 18.0f * Scale, 18.0f * Scale);
	}
	const int32 Seconds = Stream ? static_cast<int32>(Stream->GetLiveSeconds()) : 0;
	DrawShadowedText(FString::Printf(TEXT("REC  %02d:%02d:%02d   1080p60   4G"), Seconds / 3600, (Seconds / 60) % 60, Seconds % 60), FLinearColor::White,
		RecX + 28.0f * Scale, H - Margin - 46.0f * Scale, Medium, 1.1f * Scale);
	const int32 Battery = FMath::Clamp(87 - Seconds / 120, 3, 87);
	DrawShadowedText(FString::Printf(TEXT("%d%%"), Battery), Battery < 15 ? LiveRed : FLinearColor::White, W - Margin - 90.0f * Scale, Margin + 12.0f * Scale, Medium, 1.1f * Scale);
}

void AZMBHUD::DrawHelp(float Scale)
{
	UFont* Small = GEngine->GetSmallFont();
	const TCHAR* Lines[] = {
		TEXT("WASD andar   Shift correr   Espaço pular   Mouse olhar"),
		TEXT("Botão esq. atirar   Botão dir. mirar   R recarregar   1 2 3 armas"),
		TEXT("F chute   G granada   H joga bujão (atire nele no ar!)   Q mira lenta"),
		TEXT("E entrar/sair do carro   C distância da câmera   V primeira pessoa (celular)"),
		TEXT("X construir (clique constrói ou conserta, R gira, 1 a 5 peça, F desmonta)   segurar E desmancha carro"),
		TEXT("B clipar o melhor lance   N horda (teste)   T +3 horas (teste)   F1 esconde"),
	};
	const float LineHeight = 22.0f * Scale;
	const float Width = 860.0f * Scale;
	const float X = (Canvas->ClipX - Width) * 0.5f;
	const float Count = static_cast<float>(UE_ARRAY_COUNT(Lines));
	const float Y = Canvas->ClipY - 24.0f * Scale - Count * LineHeight - 16.0f * Scale;
	DrawRect(Panel, X - 12.0f * Scale, Y - 8.0f * Scale, Width + 24.0f * Scale, Count * LineHeight + 16.0f * Scale);
	for (int32 i = 0; i < static_cast<int32>(UE_ARRAY_COUNT(Lines)); ++i)
	{
		DrawShadowedText(Lines[i], Soft, X, Y + i * LineHeight, Small, 1.05f * Scale);
	}
}

void AZMBHUD::DrawBuild(float Scale)
{
	const AZMBCharacter* Player = Cast<AZMBCharacter>(GetOwningPawn());
	const UZMBGameData* Data = UZMBGameData::Get(this);
	if (!Player || !Data || Player->IsDriving())
	{
		return;
	}
	UFont* Medium = GEngine->GetMediumFont();
	UFont* Small = GEngine->GetSmallFont();
	const bool bPortuguese = UZMBGameData::IsPortuguese();
	float Width = 0.0f;
	float Height = 0.0f;
	auto Centered = [this, &Width, &Height](const FString& Text, const FLinearColor& Color, float Y, UFont* Font, float TextScale)
	{
		GetTextSize(Text, Width, Height, Font, TextScale);
		DrawShadowedText(Text, Color, (Canvas->ClipX - Width) * 0.5f, Y, Font, TextScale);
	};

	// desmanchando um carro (segurando E): barra no meio da tela; perto de um, a dica
	const float Dismantle = Player->GetDismantle01();
	if (Dismantle >= 0.0f)
	{
		const float BarWidth = 260.0f * Scale;
		Centered(LOCTEXT("Dismantling", "DESMANCHANDO O CARRO").ToString(), FLinearColor::White, Canvas->ClipY * 0.58f, Medium, 1.1f * Scale);
		DrawBar((Canvas->ClipX - BarWidth) * 0.5f, Canvas->ClipY * 0.58f + 30.0f * Scale, BarWidth, 14.0f * Scale, Dismantle, Gold);
	}
	else if (Player->IsWreckNearby() && !Player->IsBuildMode())
	{
		Centered(FText::Format(LOCTEXT("DismantleHint", "segure E: desmanchar o carro (+{0} sucata)"),
			FText::AsNumber(Data->Tuning().Base.ScrapPerWreck)).ToString(), Soft, Canvas->ClipY * 0.6f, Small, 1.1f * Scale);
	}

	if (!Player->IsBuildMode())
	{
		return;
	}
	const std::vector<ZR::FBuildPieceType>& Pieces = Data->BuildPieces();
	float Y = Canvas->ClipY * 0.64f;
	Centered(FText::Format(LOCTEXT("BuildTitle", "MODO CONSTRUÇÃO    SUCATA {0}"), FText::AsNumber(Player->GetScrap())).ToString(), Gold, Y, Medium, 1.2f * Scale);
	Y += 32.0f * Scale;

	// a fileira de peças: a escolhida em dourado, a que não dá para pagar apagada
	TArray<TPair<FString, FLinearColor>> Items;
	for (size_t i = 0; i < Pieces.size(); ++i)
	{
		const ZR::FBuildPieceType& Piece = Pieces[i];
		const FString Name = UZMBGameData::ToFString(bPortuguese || Piece.NameEn.empty() ? Piece.NamePt : Piece.NameEn);
		const bool bChosen = static_cast<int32>(i) == Player->GetBuildIndex();
		const FLinearColor Color = bChosen ? Gold : (Piece.Cost > Player->GetScrap() ? FLinearColor(0.5f, 0.5f, 0.5f) : Soft);
		Items.Add(TPair<FString, FLinearColor>(FString::Printf(TEXT("[%d] %s %d"), Piece.Slot, *Name, Piece.Cost), Color));
	}
	const float Gap = 22.0f * Scale;
	float Total = 0.0f;
	for (const TPair<FString, FLinearColor>& Item : Items)
	{
		GetTextSize(Item.Key, Width, Height, Small, 1.05f * Scale);
		Total += Width + Gap;
	}
	float X = (Canvas->ClipX - Total + Gap) * 0.5f;
	DrawRect(Panel, X - 10.0f * Scale, Y - 4.0f * Scale, Total - Gap + 20.0f * Scale, 26.0f * Scale);
	for (const TPair<FString, FLinearColor>& Item : Items)
	{
		GetTextSize(Item.Key, Width, Height, Small, 1.05f * Scale);
		DrawShadowedText(Item.Key, Item.Value, X, Y, Small, 1.05f * Scale);
		X += Width + Gap;
	}
	Y += 34.0f * Scale;

	// peça na mira: consertar ou desmontar; senão, o motivo de não poder construir (ou as teclas)
	if (const AZMBBuildPiece* Aimed = Player->GetAimedPiece())
	{
		const ZR::FBuildPieceType& Piece = Aimed->GetPieceType();
		const FString Name = UZMBGameData::ToFString(bPortuguese || Piece.NameEn.empty() ? Piece.NamePt : Piece.NameEn);
		const int32 Cost = Aimed->FullRepairCost();
		const FString Repair = Cost > 0 ? FText::Format(LOCTEXT("RepairCost", "clique conserta ({0})"), FText::AsNumber(Cost)).ToString()
			: LOCTEXT("IsIntact", "inteira").ToString();
		Centered(FText::Format(LOCTEXT("AimedPiece", "{0} {1}%    {2}    F desmonta (+{3})"), FText::FromString(Name),
			FText::AsNumber(FMath::RoundToInt(Aimed->GetHealth01() * 100.0f)), FText::FromString(Repair),
			FText::AsNumber(Aimed->RefundNow())).ToString(), FLinearColor::White, Y, Small, 1.1f * Scale);
		return;
	}
	FText Status;
	switch (Player->GetPlaceResult())
	{
	case ZR::EPlaceResult::NoScrap: Status = LOCTEXT("NoScrap", "SEM SUCATA: desmanche carros (E) ou espere a caixa do drone"); break;
	case ZR::EPlaceResult::TooMany: Status = LOCTEXT("TooMany", "LIMITE DE PEÇAS: desmonte alguma (F)"); break;
	case ZR::EPlaceResult::TooFar: Status = LOCTEXT("TooFar", "LONGE DEMAIS"); break;
	case ZR::EPlaceResult::OffStreet: Status = LOCTEXT("OffStreet", "SÓ NA RUA, NA PRAÇA OU NA PRAIA"); break;
	case ZR::EPlaceResult::OnPlayer: Status = LOCTEXT("OnPlayer", "SAIA DA FRENTE: a peça cai em cima de você"); break;
	case ZR::EPlaceResult::Blocked: Status = LOCTEXT("Blocked", "TEM ALGO NO CAMINHO (peça, poste, árvore, carro ou zumbi)"); break;
	case ZR::EPlaceResult::Ok:
	default: Status = LOCTEXT("BuildKeys", "clique constrói    R gira    1 a 5 ou roda troca    F desmonta    X sai"); break;
	}
	const bool bOk = Player->GetPlaceResult() == ZR::EPlaceResult::Ok;
	Centered(Status.ToString(), bOk ? Soft : FLinearColor(1.0f, 0.35f, 0.25f), Y, Small, 1.1f * Scale);
}

#undef LOCTEXT_NAMESPACE
