#include "Game/ZMBGameMode.h"
#include "Zombie.h"
#include "Game/ZMBWorldBuilder.h"
#include "Player/ZMBCharacter.h"
#include "Zombies/ZMBZombie.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Game/ZMBGameData.h"
#include "UI/ZMBHUD.h"
#include "Zombies/ZMBHordeDirector.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerStart.h"
#include "Misc/App.h"
#include "RHI.h"
#include "RenderTimer.h"
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

void AZMBGameMode::RunShotTest()
{
	const UZMBGameData* Data = UZMBGameData::Get(this);
	const ZR::FZombieType* Type = Data ? Data->FindZombie("comum") : nullptr;
	if (!Type)
	{
		UE_LOG(LogZombie, Error, TEXT("ZMBTesteTiro: sem o tipo comum"));
		return;
	}
	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	AZMBZombie* Zombie = GetWorld()->SpawnActorDeferred<AZMBZombie>(AZMBZombie::StaticClass(), FTransform(FRotator::ZeroRotator, Builder->GetStartLocation() + FVector(0.0f, -900.0f, 40.0f)), nullptr, nullptr,
		ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
	if (!Zombie)
	{
		return;
	}
	Zombie->InitZombie(*Type, FString(), false);
	Zombie->FinishSpawning(Zombie->GetActorTransform());
	Zombie->GetCharacterMovement()->DisableMovement();
	// linhas de visão de frente, no corpo em pé: nome, y, z locais, membro esperado (a ordem de ZR::ELimb)
	struct FProbe { const TCHAR* Name; float Y; float Z; int32 Expected; };
	const FProbe Probes[] = {
		{TEXT("cabeca"), 0.0f, 70.0f, 0}, {TEXT("tronco"), 0.0f, 24.0f, 1}, {TEXT("braco_esq"), -27.0f, 40.0f, 2},
		{TEXT("braco_dir"), 27.0f, 40.0f, 3}, {TEXT("perna_esq"), -11.0f, -50.0f, 4}, {TEXT("perna_dir"), 11.0f, -50.0f, 5},
	};
	int32 Right = 0;
	for (const FProbe& Probe : Probes)
	{
		const FVector From = Zombie->GetActorTransform().TransformPosition(FVector(300.0f, Probe.Y, Probe.Z));
		const FVector To = Zombie->GetActorTransform().TransformPosition(FVector(0.0f, Probe.Y, Probe.Z));
		FHitResult Hit;
		FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(ZMBShotTest), false);
		const bool bHit = GetWorld()->LineTraceSingleByChannel(Hit, From, To, ECC_Visibility, QueryParams);
		const bool bOnZombie = bHit && Hit.GetActor() == Zombie;
		const int32 Limb = bOnZombie ? Zombie->LimbForHit(Hit.GetComponent(), Hit.ImpactPoint) : -1;
		Right += Limb == Probe.Expected ? 1 : 0;
		UE_LOG(LogZombie, Display, TEXT("ZMBTesteTiro: %s acertou o zumbi=%d membro=%d esperado=%d"), Probe.Name, bOnZombie ? 1 : 0, Limb, Probe.Expected);
	}
	UE_LOG(LogZombie, Display, TEXT("ZMBTesteTiro: %d de %d pontos deram o membro certo"), Right, static_cast<int32>(UE_ARRAY_COUNT(Probes)));
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
			FVector Where = Builder->GetStartLocation();
			float Yaw = Yaws[(LookdevAngle - 1) & 3];
			if (LookdevAngle >= 5 && LookdevAngle <= 7)
			{
				Builder->GetLookdevView(LookdevAngle, Where, Yaw);
			}
			PlayerController->GetPawn()->SetActorLocation(Where);
			PlayerController->SetControlRotation(FRotator(0.0f, Yaw, 0.0f));
			bLookdevPlaced = true;
		}
	}
	if (!bShotTestDone && Builder && FParse::Param(FCommandLine::Get(), TEXT("ZMBTesteTiro")) && GetWorld()->GetTimeSeconds() > 5.0f)
	{
		bShotTestDone = true;
		RunShotTest();
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
		PerfGameMs += FPlatformTime::ToMilliseconds(GGameThreadTime);
		PerfRenderMs += FPlatformTime::ToMilliseconds(GRenderThreadTime);
		PerfGpuMs += FPlatformTime::ToMilliseconds(RHIGetGPUFrameCycles());
		++PerfFrames;
	}
	if (!bShotTaken && Elapsed >= ShotAt)
	{
		bShotTaken = true;
		if (PerfFrames > 0)
		{
			UE_LOG(LogZombie, Display, TEXT("ZMBPerf: %lld quadros, média %.2f ms (%.0f fps), pior %.2f ms"), PerfFrames, PerfSumMs / PerfFrames,
				1000.0 * PerfFrames / PerfSumMs, PerfWorstMs);
			UE_LOG(LogZombie, Display, TEXT("ZMBPerf: por fio, média: jogo %.2f ms, render %.2f ms, GPU %.2f ms; zumbis vivos no fim: %d"), PerfGameMs / PerfFrames,
				PerfRenderMs / PerfFrames, PerfGpuMs / PerfFrames, Director ? Director->AliveCount() : 0);
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
