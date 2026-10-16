#include "Game/ZMBGameMode.h"
#include "Zombie.h"
#include "Game/ZMBWorldBuilder.h"
#include "Player/ZMBCharacter.h"
#include "UI/ZMBHUD.h"
#include "Zombies/ZMBHordeDirector.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerStart.h"
#include "Misc/App.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "UnrealClient.h"

AZMBGameMode::AZMBGameMode()
{
	PrimaryActorTick.bCanEverTick = true;
	DefaultPawnClass = AZMBCharacter::StaticClass();
	HUDClass = AZMBHUD::StaticClass();
}

AZMBGameMode* AZMBGameMode::Get(const UObject* WorldContext)
{
	const UWorld* World = WorldContext ? WorldContext->GetWorld() : nullptr;
	return World ? World->GetAuthGameMode<AZMBGameMode>() : nullptr;
}

void AZMBGameMode::InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage)
{
	Super::InitGame(MapName, Options, ErrorMessage);
	const TCHAR* CommandLine = FCommandLine::Get();
	int32 Seed = 2026;
	FParse::Value(CommandLine, TEXT("ZMBSeed="), Seed);
	FParse::Value(CommandLine, TEXT("ZMBShot="), ShotAt);
	FParse::Value(CommandLine, TEXT("ZMBLookdev="), LookdevAngle);

	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	Builder = GetWorld()->SpawnActor<AZMBWorldBuilder>(AZMBWorldBuilder::StaticClass(), FTransform::Identity, Params);
	if (Builder)
	{
		Builder->Build(static_cast<std::uint64_t>(Seed));
		float Hour = LookdevAngle > 0 ? 17.5f : -1.0f;
		FParse::Value(CommandLine, TEXT("ZMBHour="), Hour);
		if (Hour >= 0.0f)
		{
			Builder->SetHour(Hour);
		}
		Builder->SetFrozen(LookdevAngle > 0);
	}
	Director = GetWorld()->SpawnActor<AZMBHordeDirector>(AZMBHordeDirector::StaticClass(), FTransform::Identity, Params);
	// cena de aparência: rua vazia e relógio parado; sem Setup o Diretor não faz nada
	if (Director && LookdevAngle <= 0)
	{
		Director->Setup(Builder);
	}
	UE_LOG(LogZombie, Display, TEXT("AZMBGameMode: Porto Brasa montada no mapa '%s' (semente %d)"), *MapName, Seed);
}

AActor* AZMBGameMode::ChoosePlayerStart_Implementation(AController* Player)
{
	if (!Builder)
	{
		return Super::ChoosePlayerStart_Implementation(Player);
	}
	if (!Start)
	{
		FActorSpawnParameters Params;
		Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		Start = GetWorld()->SpawnActor<APlayerStart>(APlayerStart::StaticClass(), Builder->GetStartLocation(), Builder->GetStartRotation(), Params);
	}
	return Start;
}

void AZMBGameMode::StartPlay()
{
	Super::StartPlay();
	const TCHAR* CommandLine = FCommandLine::Get();
	if (FParse::Param(CommandLine, TEXT("ZMBHorde")) && Director)
	{
		Director->StartHorde(60.0f);
	}
}

void AZMBGameMode::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (LookdevAngle > 0 && !bLookdevPlaced && Builder)
	{
		APlayerController* PlayerController = GetWorld()->GetFirstPlayerController();
		if (PlayerController && PlayerController->GetPawn())
		{
			// quatro ângulos fixos a partir do começo: rua (norte), orla (oeste), mar (sul) e leste
			static const float Yaws[] = {-90.0f, 180.0f, 90.0f, 0.0f};
			PlayerController->GetPawn()->SetActorLocation(Builder->GetStartLocation());
			PlayerController->SetControlRotation(FRotator(0.0f, Yaws[(LookdevAngle - 1) & 3], 0.0f));
			bLookdevPlaced = true;
		}
	}
	if (ShotAt <= 0.0f)
	{
		return;
	}
	Elapsed += DeltaSeconds;
	// desempenho: média e pior quadro dos 3 s até a captura (o orçamento é 16,7 ms; rode com r.VSync 0 e t.MaxFPS 0)
	if (!bShotTaken && Elapsed >= 3.0f)
	{
		const double FrameMs = FApp::GetDeltaTime() * 1000.0;
		PerfSumMs += FrameMs;
		PerfWorstMs = FMath::Max(PerfWorstMs, FrameMs);
		++PerfFrames;
	}
	if (!bShotTaken && Elapsed >= ShotAt)
	{
		bShotTaken = true;
		if (PerfFrames > 0)
		{
			UE_LOG(LogZombie, Display, TEXT("ZMBPerf: %lld quadros, média %.2f ms (%.0f fps), pior %.2f ms"), PerfFrames, PerfSumMs / PerfFrames,
				1000.0 * PerfFrames / PerfSumMs, PerfWorstMs);
		}
		FScreenshotRequest::RequestScreenshot(TEXT("zmb-shot"), true, false);
		UE_LOG(LogZombie, Display, TEXT("ZMBShot: captura pedida aos %.1f s"), Elapsed);
	}
	else if (bShotTaken && Elapsed >= ShotAt + 1.5f)
	{
		if (APlayerController* PlayerController = GetWorld()->GetFirstPlayerController())
		{
			PlayerController->ConsoleCommand(TEXT("quit"));
		}
	}
}
