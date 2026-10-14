// HUD da live, desenhado no Canvas (sem UMG, que é binário): selo AO VIVO, espectadores e seguidores, barra de hype com o
// combo, o chat à direita, a votação do chat, os lances que saltam no meio, mira com marcador de acerto, vida, fôlego,
// foco da mira lenta, arma e munição, relógio e noite, e no "modo celular" (primeira pessoa) a moldura do app gravando.
// Provisório: o HUD final vem em CommonUI (menus por controle, Steam Deck), com o mesmo conteúdo.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "ZMBHUD.generated.h"

class UFont;

UCLASS()
class ZOMBIE_API AZMBHUD : public AHUD
{
	GENERATED_BODY()

public:
	virtual void DrawHUD() override;

private:
	void DrawLiveBadge(float Scale);
	void DrawChat(float Scale);
	void DrawVote(float Scale);
	void DrawPopups(float Scale);
	void DrawPlayer(float Scale);
	void DrawCrosshair(float Scale);
	void DrawClock(float Scale);
	void DrawPhoneFrame(float Scale);
	void DrawHelp(float Scale);
	void DrawShadowedText(const FString& Text, const FLinearColor& Color, float X, float Y, UFont* Font, float TextScale);
	void DrawBar(float X, float Y, float Width, float Height, float Fill, const FLinearColor& Color);

	float Clock = 0.0f;
};
