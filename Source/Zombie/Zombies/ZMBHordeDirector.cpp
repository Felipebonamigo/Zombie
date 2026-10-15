#include "Zombies/ZMBHordeDirector.h"
#include "Zombie.h"
#include "Base/ZMBBuildPiece.h"
#include "Combat/ZMBPickup.h"
#include "Combat/ZMBTypes.h"
#include "Game/ZMBGameData.h"
#include "Game/ZMBGameMode.h"
#include "Game/ZMBWorldBuilder.h"
#include "Stream/ZMBStreamSubsystem.h"
#include "Zombies/ZMBZombie.h"
#include "Camera/PlayerCameraManager.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/Pawn.h"
#include "Kismet/GameplayStatics.h"

#define LOCTEXT_NAMESPACE "ZombieDirector"

AZMBHordeDirector::AZMBHordeDirector()
{
	PrimaryActorTick.bCanEverTick = true;
	SetRootComponent(CreateDefaultSubobject<USceneComponent>(TEXT("Root")));
}

AZMBHordeDirector* AZMBHordeDirector::Find(const UObject* WorldContext)
{
	const AZMBGameMode* GameMode = AZMBGameMode::Get(WorldContext);
	return GameMode ? GameMode->GetDirector() : nullptr;
}

void AZMBHordeDirector::Setup(AZMBWorldBuilder* InBuilder)
{
	Builder = InBuilder;
}

int32 AZMBHordeDirector::AliveCount() const
{
	int32 Count = 0;
	for (const TWeakObjectPtr<AZMBZombie>& Zombie : Alive)
	{
		Count += (Zombie.IsValid() && !Zombie->IsDead()) ? 1 : 0;
	}
	return Count;
}

int32 AZMBHordeDirector::NamedAliveCount() const
{
	int32 Count = 0;
	for (const TWeakObjectPtr<AZMBZombie>& Zombie : Alive)
	{
		Count += (Zombie.IsValid() && !Zombie->IsDead() && Zombie->IsNamed()) ? 1 : 0;
	}
	return Count;
}

void AZMBHordeDirector::ReportDamageTaken(float Amount)
{
	PendingDamage += FMath::Max(0.0f, Amount);
}

void AZMBHordeDirector::ReportExplosion(const FVector& Where, float Noise)
{
	const APawn* Player = UGameplayStatics::GetPlayerPawn(this, 0);
	if (Player && FVector::Dist(Player->GetActorLocation(), Where) < 4000.0f)
	{
		++PendingExplosions;
	}
	PendingNoise += Noise;
}

void AZMBHordeDirector::ReportNoise(float Noise)
{
	PendingNoise += Noise;
}

void AZMBHordeDirector::ReportZombieDied(AZMBZombie* Zombie, bool bPlayerKill)
{
	const APawn* Player = UGameplayStatics::GetPlayerPawn(this, 0);
	if (Zombie && Player && FVector::Dist(Player->GetActorLocation(), Zombie->GetActorLocation()) < 2500.0f)
	{
		++PendingKillsNear;
	}
}

void AZMBHordeDirector::StartHorde(float Seconds)
{
	ZR::StartHorde(State, Seconds);
}

void AZMBHordeDirector::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	const UZMBGameData* Data = UZMBGameData::Get(this);
	if (!Builder || !Data)
	{
		return;
	}
	const ZR::FTuning& Tuning = Data->Tuning();
	Cleanup();
	UpdateFlow();
	FlowAge += DeltaSeconds;
	UpdateLocalFlow(DeltaSeconds);

	const bool bNightNow = Builder->IsNightNow();
	if (bNightNow && !bNight)
	{
		const bool bHordeNight = ZR::IsHordeNight(Tuning.Day, NightsSeen);
		++NightsSeen;
		if (UZMBStreamSubsystem* Stream = UZMBStreamSubsystem::Get(this))
		{
			Stream->Announce(bHordeNight ? LOCTEXT("HordeNight", "NOITE DA HORDA") : LOCTEXT("NightFalls", "A NOITE CHEGOU"));
			Stream->React("noite");
		}
		if (bHordeNight)
		{
			StartHorde(90.0f);
		}
	}
	bNight = bNightNow;

	ConsumeChatActions();

	ZR::FDirectorInput Input;
	Input.Dt = DeltaSeconds;
	Input.Alive = AliveCount();
	Input.bNight = bNight;
	Input.DamageTaken = PendingDamage;
	Input.KillsNear = PendingKillsNear;
	Input.Explosions = PendingExplosions;
	Input.NoiseAdded = PendingNoise;
	PendingDamage = 0.0f;
	PendingKillsNear = 0;
	PendingExplosions = 0;
	PendingNoise = 0.0f;
	const ZR::FDirectorOutput Out = ZR::TickDirector(Tuning.Director, State, Input);

	int32 ToSpawn = Out.SpawnNow;
	const int32 Cap = bNight ? Tuning.Director.MaxAliveNight : Tuning.Director.MaxAlive;
	while (ToSpawn > 0)
	{
		FVector Location;
		const ZR::FZombieType* Type = PickType(bNight);
		if (!Type || !PickSpawnLocation(Location, Tuning.SpawnMinDistanceCm, Tuning.SpawnMaxDistanceCm, true))
		{
			break;
		}
		// folião vem em bloco de Carnaval: até 4 juntos, dentro do mesmo orçamento
		const int32 Group = Type->Id == "foliao" ? FMath::Min(ToSpawn, FMath::RandRange(2, 4)) : 1;
		for (int32 i = 0; i < Group && AliveCount() < Cap; ++i)
		{
			const FVector Offset(FMath::FRandRange(-250.0f, 250.0f), FMath::FRandRange(-250.0f, 250.0f), 0.0f);
			SpawnZombie(*Type, Location + Offset, FString(), Out.bHordeActive);
		}
		ToSpawn -= Group;
	}
}

void AZMBHordeDirector::Cleanup()
{
	const APawn* Player = UGameplayStatics::GetPlayerPawn(this, 0);
	const UZMBGameData* Data = UZMBGameData::Get(this);
	const float Despawn = Data ? Data->Tuning().DespawnDistanceCm : 9000.0f;
	for (int32 i = Alive.Num() - 1; i >= 0; --i)
	{
		AZMBZombie* Zombie = Alive[i].Get();
		if (!Zombie || Zombie->IsDead())
		{
			Alive.RemoveAtSwap(i);
			continue;
		}
		const bool bTooFar = Player && !Zombie->IsNamed()
			&& FVector::Dist2D(Zombie->GetActorLocation(), Player->GetActorLocation()) > Despawn;
		const bool bFellOut = Zombie->GetActorLocation().Z < -3000.0f;
		if (bTooFar || bFellOut)
		{
			Zombie->Destroy();
			Alive.RemoveAtSwap(i);
		}
	}
}

void AZMBHordeDirector::UpdateFlow()
{
	const APawn* Player = UGameplayStatics::GetPlayerPawn(this, 0);
	if (!Player)
	{
		return;
	}
	const ZR::FCityPlan& Plan = Builder->GetPlan();
	int32 CellX = 0;
	int32 CellY = 0;
	if (!Builder->WorldToCell(Player->GetActorLocation(), CellX, CellY))
	{
		return;
	}
	// jogador num beco (célula de prédio): o alvo vira a rua andável mais perto
	if (!Plan.IsWalkable(CellX, CellY))
	{
		bool bFound = false;
		for (int32 Radius = 1; Radius <= 2 && !bFound; ++Radius)
		{
			for (int32 Dy = -Radius; Dy <= Radius && !bFound; ++Dy)
			{
				for (int32 Dx = -Radius; Dx <= Radius && !bFound; ++Dx)
				{
					if (Plan.IsWalkable(CellX + Dx, CellY + Dy))
					{
						CellX += Dx;
						CellY += Dy;
						bFound = true;
					}
				}
			}
		}
	}
	if (CellX == FlowTargetX && CellY == FlowTargetY && FlowAge < 3.0f)
	{
		return;
	}
	FlowTargetX = CellX;
	FlowTargetY = CellY;
	FlowAge = 0.0f;
	Flow.Build(Plan.WalkableMask(), Plan.Width, Plan.Height, CellX, CellY);
}

void AZMBHordeDirector::UpdateLocalFlow(float DeltaSeconds)
{
	LocalAge += DeltaSeconds;
	const APawn* Player = UGameplayStatics::GetPlayerPawn(this, 0);
	const UZMBGameData* Data = UZMBGameData::Get(this);
	if (!Player || !Builder || !Data)
	{
		bLocalActive = false;
		return;
	}
	const FVector Origin = Builder->GetOrigin();
	const FVector PlayerLocal = Player->GetActorLocation() - Origin;
	const float PlayerX = static_cast<float>(PlayerLocal.X);
	const float PlayerY = static_cast<float>(PlayerLocal.Y);
	int CellX = -1;
	int CellY = -1;
	// refaz na hora quando uma barricada muda; senão a cada meio segundo ou quando o jogador anda duas células (de carro,
	// isso seria todo quadro), e nunca mais que uma vez a cada 0,15 s
	const bool bMoved = bLocalActive
		&& (!LocalGrid.ToCell(PlayerX, PlayerY, CellX, CellY) || FMath::Abs(CellX - LocalPlayerX) >= 2 || FMath::Abs(CellY - LocalPlayerY) >= 2);
	if (LocalAge < 0.15f || (!bLocalDirty && !bMoved && LocalAge < 0.5f))
	{
		return;
	}
	LocalAge = 0.0f;
	bLocalDirty = false;
	const ZR::FLocalGridConfig& Config = Data->Tuning().HordeGrid;
	const float Reach = Config.SizeCm * 0.75f;
	std::vector<ZR::FFootprint> Barricades;
	for (TActorIterator<AZMBBuildPiece> It(GetWorld()); It; ++It)
	{
		if (It->IsBarricade() && !It->IsGone() && FVector::Dist2D(It->GetActorLocation(), Player->GetActorLocation()) < Reach)
		{
			Barricades.push_back(It->Footprint(Origin));
		}
	}
	// sem barricada perto, a grade fina não muda nada: os zumbis seguem como antes (direto, perto do jogador)
	if (Barricades.empty())
	{
		bLocalActive = false;
		return;
	}
	LocalGrid = ZR::MakeLocalGrid(Builder->GetPlan(), Builder->GetCellSize(), PlayerX, PlayerY, Config, Barricades);
	if (!LocalGrid.ToCell(PlayerX, PlayerY, CellX, CellY))
	{
		bLocalActive = false;
		return;
	}
	LocalPlayerX = CellX;
	LocalPlayerY = CellY;
	// jogador fora do chão (encostado num canto que a grade marca como prédio): o alvo vira a célula andável mais perto,
	// pela distância de verdade, até um pouco mais que o maior recuo de fachada
	int TargetX = CellX;
	int TargetY = CellY;
	const auto WalkableAt = [this](int X, int Y)
	{
		return X >= 0 && Y >= 0 && X < LocalGrid.Width && Y < LocalGrid.Height
			&& LocalGrid.Walkable[static_cast<size_t>(Y * LocalGrid.Width + X)] != 0;
	};
	if (!WalkableAt(CellX, CellY))
	{
		const int SearchCells = FMath::CeilToInt32(450.0f / FMath::Max(LocalGrid.ResolutionCm, 1.0f)) + 1;
		int BestDistance = TNumericLimits<int>::Max();
		for (int Dy = -SearchCells; Dy <= SearchCells; ++Dy)
		{
			for (int Dx = -SearchCells; Dx <= SearchCells; ++Dx)
			{
				const int Distance = Dx * Dx + Dy * Dy;
				if (Distance < BestDistance && WalkableAt(CellX + Dx, CellY + Dy))
				{
					BestDistance = Distance;
					TargetX = CellX + Dx;
					TargetY = CellY + Dy;
				}
			}
		}
	}
	LocalFlow.Build(LocalGrid.Walkable, LocalGrid.ExtraCost, LocalGrid.Width, LocalGrid.Height, TargetX, TargetY);
	bLocalActive = LocalFlow.IsValid();
}

bool AZMBHordeDirector::FlowDirection(const FVector& From, FVector& OutDirection, bool bAllowCity) const
{
	if (!Builder)
	{
		return false;
	}
	if (bLocalActive)
	{
		const FVector Local = From - Builder->GetOrigin();
		// nada no caminho até o jogador (sem prédio nem barricada na reta): direto, sem o zigue-zague das 8 direções
		const APawn* Player = UGameplayStatics::GetPlayerPawn(this, 0);
		if (Player)
		{
			const FVector PlayerLocal = Player->GetActorLocation() - Builder->GetOrigin();
			if (LocalGrid.ClearLine(static_cast<float>(Local.X), static_cast<float>(Local.Y), static_cast<float>(PlayerLocal.X),
					static_cast<float>(PlayerLocal.Y)))
			{
				FVector Straight = Player->GetActorLocation() - From;
				Straight.Z = 0.0f;
				OutDirection = Straight.GetSafeNormal();
				if (!OutDirection.IsNearlyZero())
				{
					return true;
				}
			}
		}
		int LocalX = 0;
		int LocalY = 0;
		int AheadX = 0;
		int AheadY = 0;
		// dois passos à frente: o caminho de 8 direções fica menos serrilhado; no alvo não há passo (vai direto)
		if (LocalGrid.ToCell(static_cast<float>(Local.X), static_cast<float>(Local.Y), LocalX, LocalY)
			&& LocalFlow.LookAhead(LocalX, LocalY, 2, AheadX, AheadY))
		{
			float AimX = 0.0f;
			float AimY = 0.0f;
			LocalGrid.CellCenter(AheadX, AheadY, AimX, AimY);
			FVector Delta = Builder->GetOrigin() + FVector(AimX, AimY, 0.0f) - From;
			Delta.Z = 0.0f;
			OutDirection = Delta.GetSafeNormal();
			if (!OutDirection.IsNearlyZero())
			{
				return true;
			}
		}
	}
	if (!bAllowCity || !Flow.IsValid())
	{
		return false;
	}
	int32 CellX = 0;
	int32 CellY = 0;
	int NextX = 0;
	int NextY = 0;
	if (!Builder->WorldToCell(From, CellX, CellY) || !Flow.StepToward(CellX, CellY, NextX, NextY))
	{
		return false;
	}
	FVector Delta = Builder->CellCenter(NextX, NextY) - From;
	Delta.Z = 0.0f;
	OutDirection = Delta.GetSafeNormal();
	return !OutDirection.IsNearlyZero();
}

bool AZMBHordeDirector::PickSpawnLocation(FVector& OutLocation, float MinCm, float MaxCm, bool bPreferHidden)
{
	const APawn* Player = UGameplayStatics::GetPlayerPawn(this, 0);
	if (!Player || !Builder)
	{
		return false;
	}
	const ZR::FCityPlan& Plan = Builder->GetPlan();
	const FVector Center = Player->GetActorLocation();
	FVector ViewForward = Player->GetActorForwardVector();
	if (const APlayerCameraManager* Camera = UGameplayStatics::GetPlayerCameraManager(this, 0))
	{
		ViewForward = Camera->GetCameraRotation().Vector();
	}
	ViewForward.Z = 0.0f;
	ViewForward.Normalize();

	for (int32 Try = 0; Try < 48; ++Try)
	{
		const float Angle = FMath::FRandRange(0.0f, 2.0f * UE_PI);
		const float Distance = FMath::FRandRange(MinCm, MaxCm);
		const FVector Direction(FMath::Cos(Angle), FMath::Sin(Angle), 0.0f);
		// as primeiras tentativas só aceitam fora da vista; as últimas aceitam qualquer lugar
		if (bPreferHidden && Try < 32 && FVector::DotProduct(Direction, ViewForward) > 0.3f)
		{
			continue;
		}
		const FVector Candidate = Center + Direction * Distance;
		int32 CellX = 0;
		int32 CellY = 0;
		if (!Builder->WorldToCell(Candidate, CellX, CellY) || !Plan.IsWalkable(CellX, CellY))
		{
			continue;
		}
		OutLocation = FVector(Candidate.X, Candidate.Y, Builder->GroundHeight());
		return true;
	}
	return false;
}

const ZR::FZombieType* AZMBHordeDirector::PickType(bool bNightNow) const
{
	const UZMBGameData* Data = UZMBGameData::Get(this);
	if (!Data || Data->Zombies().empty())
	{
		return nullptr;
	}
	float Total = 0.0f;
	for (const ZR::FZombieType& Type : Data->Zombies())
	{
		Total += bNightNow ? Type.NightSpawnWeight : Type.SpawnWeight;
	}
	float Roll = FMath::FRandRange(0.0f, FMath::Max(Total, 0.0001f));
	for (const ZR::FZombieType& Type : Data->Zombies())
	{
		Roll -= bNightNow ? Type.NightSpawnWeight : Type.SpawnWeight;
		if (Roll <= 0.0f)
		{
			return &Type;
		}
	}
	return &Data->Zombies().back();
}

AZMBZombie* AZMBHordeDirector::SpawnZombie(const ZR::FZombieType& Type, const FVector& Location, const FString& Viewer, bool bHordeSpawn)
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return nullptr;
	}
	const float Scale = Type.Scale;
	const FTransform Transform(FRotator(0.0f, FMath::FRandRange(0.0f, 360.0f), 0.0f), Location + FVector(0.0f, 0.0f, 95.0f * Scale), FVector(Scale));
	AZMBZombie* Zombie = World->SpawnActorDeferred<AZMBZombie>(AZMBZombie::StaticClass(), Transform, nullptr, nullptr,
		ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn);
	if (!Zombie)
	{
		return nullptr;
	}
	Zombie->InitZombie(Type, Viewer, bHordeSpawn);
	Zombie->FinishSpawning(Transform);
	Alive.Add(Zombie);
	return Zombie;
}

bool AZMBHordeDirector::SpawnNamedZombie(const FString& Viewer)
{
	const UZMBGameData* Data = UZMBGameData::Get(this);
	if (!Data || Viewer.IsEmpty() || NamedAliveCount() >= Data->Tuning().MaxNamedZombies)
	{
		return false;
	}
	FVector Location;
	const ZR::FZombieType* Type = PickType(bNight);
	// perto o bastante para o espectador ver o "seu" zumbi chegando
	if (!Type || !PickSpawnLocation(Location, 1500.0f, 3500.0f, false))
	{
		return false;
	}
	return SpawnZombie(*Type, Location, Viewer, false) != nullptr;
}

void AZMBHordeDirector::SpawnBombSquad(int32 Count)
{
	const UZMBGameData* Data = UZMBGameData::Get(this);
	const ZR::FZombieType* Bomb = Data ? Data->FindZombie("bujao") : nullptr;
	FVector Location;
	if (!Bomb || !PickSpawnLocation(Location, 2000.0f, 4000.0f, false))
	{
		return;
	}
	for (int32 i = 0; i < Count; ++i)
	{
		const FVector Offset(FMath::FRandRange(-300.0f, 300.0f), FMath::FRandRange(-300.0f, 300.0f), 0.0f);
		SpawnZombie(*Bomb, Location + Offset, FString(), true);
	}
}

int32 AZMBHordeDirector::CallHorde(const FVector& Where, int32 Count)
{
	const UZMBGameData* Data = UZMBGameData::Get(this);
	if (!Data || !Builder)
	{
		return 0;
	}
	int32 Spawned = 0;
	PendingNoise += 100.0f;
	const int32 Cap = bNight ? Data->Tuning().Director.MaxAliveNight : Data->Tuning().Director.MaxAlive;
	for (int32 i = 0; i < Count && AliveCount() < Cap; ++i)
	{
		// em volta do gritador, numa rua (6 a 14 m); sem lugar andável, desiste deste
		for (int32 Try = 0; Try < 8; ++Try)
		{
			const float Angle = FMath::FRandRange(0.0f, 2.0f * UE_PI);
			const FVector Spot = Where + FVector(FMath::Cos(Angle), FMath::Sin(Angle), 0.0f) * FMath::FRandRange(600.0f, 1400.0f);
			int32 CellX = 0;
			int32 CellY = 0;
			const ZR::FZombieType* Type = PickType(bNight);
			if (Type && Type->Id != "gritador" && Builder->WorldToCell(Spot, CellX, CellY) && Builder->GetPlan().IsWalkable(CellX, CellY))
			{
				Spawned += SpawnZombie(*Type, FVector(Spot.X, Spot.Y, Builder->GroundHeight()), FString(), true) ? 1 : 0;
				break;
			}
		}
	}
	return Spawned;
}

void AZMBHordeDirector::DropSupplyCrate()
{
	FVector Location;
	if (!PickSpawnLocation(Location, 900.0f, 2200.0f, false))
	{
		return;
	}
	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	GetWorld()->SpawnActor<AZMBPickup>(AZMBPickup::StaticClass(), Location + FVector(0.0f, 0.0f, 3000.0f), FRotator::ZeroRotator, Params);
}

void AZMBHordeDirector::ConsumeChatActions()
{
	UZMBStreamSubsystem* Stream = UZMBStreamSubsystem::Get(this);
	if (!Stream)
	{
		return;
	}
	TArray<FZMBChatAction> Actions;
	Stream->ConsumeChatActions(Actions);
	for (const FZMBChatAction& Action : Actions)
	{
		switch (Action.Kind)
		{
		case EZMBChatActionKind::NamedZombie:
			SpawnNamedZombie(Action.ViewerName);
			break;
		case EZMBChatActionKind::Horde:
			StartHorde(45.0f);
			Stream->Announce(LOCTEXT("ChatHorde", "O CHAT MANDOU UMA HORDA"));
			Stream->React("horda");
			break;
		case EZMBChatActionKind::BombSquad:
			SpawnBombSquad(5);
			Stream->Announce(LOCTEXT("ChatBombs", "O CHAT MANDOU O ESQUADRÃO DO BUJÃO"));
			break;
		case EZMBChatActionKind::Airdrop:
			DropSupplyCrate();
			Stream->Announce(LOCTEXT("Airdrop", "ENTREGA POR DRONE A CAMINHO"));
			break;
		}
	}
}

#undef LOCTEXT_NAMESPACE
