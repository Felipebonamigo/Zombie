// Modo de jogo do protótipo: num mapa vazio (o /Engine/Maps/Entry do Config/DefaultEngine.ini) monta Porto Brasa
// (AZMBWorldBuilder), põe o Diretor da horda (AZMBHordeDirector), o jogador na orla e o HUD da live.
//
// Linha de comando (também para as capturas automáticas da sessão do PC; skill jogar-e-capturar):
//   -ZMBSeed=N         semente da cidade (padrão 2026)
//   -ZMBHour=H         começa nesta hora (ex.: 21 para já começar de noite)
//   -ZMBCam=0..3       câmera inicial (0 perto, 1 média, 2 longe e alta, 3 primeira pessoa)
//   -ZMBHorde          começa com uma horda
//   -ZMBShot=S         tira um print aos S segundos e fecha o jogo (zmb-shot.png em Saved/Screenshots)
//   -ZMBLookdev=1..4   cena de aparência: ângulo fixo (1 rua, 2 orla, 3 mar, 4 leste), 17,5 h, relógio parado, sem horda
//   -ZMBTwitch=canal   lê o chat de um canal da Twitch (Stream/ZMBTwitchChat)

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "ZMBGameMode.generated.h"

class AZMBWorldBuilder;
class AZMBHordeDirector;
class APlayerStart;

UCLASS()
class ZOMBIE_API AZMBGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	AZMBGameMode();

	static AZMBGameMode* Get(const UObject* WorldContext);

	virtual void InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage) override;
	virtual AActor* ChoosePlayerStart_Implementation(AController* Player) override;
	virtual void StartPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	AZMBWorldBuilder* GetBuilder() const { return Builder; }
	AZMBHordeDirector* GetDirector() const { return Director; }

	/** Rodando por script (-ZMBShot): sem a ajuda na tela. */
	bool IsAutomatedRun() const { return ShotAt > 0.0f; }

private:
	UPROPERTY(Transient)
	TObjectPtr<AZMBWorldBuilder> Builder;

	UPROPERTY(Transient)
	TObjectPtr<AZMBHordeDirector> Director;

	UPROPERTY(Transient)
	TObjectPtr<APlayerStart> Start;

	double PerfSumMs = 0.0;
	double PerfWorstMs = 0.0;
	int64 PerfFrames = 0;
	int32 LookdevAngle = 0;
	bool bLookdevPlaced = false;
	float ShotAt = -1.0f;
	float Elapsed = 0.0f;
	bool bShotTaken = false;
};
