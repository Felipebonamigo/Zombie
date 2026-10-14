#include "Game/ZMBGameMode.h"
#include "Zombie.h"
#include "Game/ZMBWorldBuilder.h"
#include "Player/ZMBCharacter.h"
#include "UI/ZMBHUD.h"
#include "Zombies/ZMBHordeDirector.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerStart.h"
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

	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	Builder = GetWorld()->SpawnActor<AZMBWorldBuilder>(AZMBWorldBuilder::StaticClass(), FTransform::Identity, Params);
	if (Builder)
	{
		Builder->Build(static_cast<std::uint64_t>(Seed));
		float Hour = -1.0f;
		if (FParse::Value(CommandLine, TEXT("ZMBHour="), Hour) && Hour >= 0.0f)
		{
			Builder->SetHour(Hour);
		}
	}
	Director = GetWorld()->SpawnActor<AZMBHordeDirector>(AZMBHordeDirector::StaticClass(), FTransform::Identity, Params);
	if (Director)
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
	if (ShotAt <= 0.0f)
	{
		return;
	}
	Elapsed += DeltaSeconds;
	if (!bShotTaken && Elapsed >= ShotAt)
	{
		bShotTaken = true;
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
