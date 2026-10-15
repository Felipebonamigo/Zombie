#include "Base/ZMBBuildPiece.h"
#include "Zombie.h"
#include "Combat/ZMBExplosion.h"
#include "Combat/ZMBFx.h"
#include "Combat/ZMBTypes.h"
#include "Stream/ZMBStreamSubsystem.h"
#include "Zombies/ZMBHordeDirector.h"
#include "Zombies/ZMBZombie.h"
#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/World.h"
#include "GameFramework/Controller.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/MaterialInstanceDynamic.h"

namespace
{
	/** Barricada nova, quebrada ou desmontada: a horda recalcula por onde contornar a base. */
	void NotifyBaseChanged(const UObject* WorldContext)
	{
		if (AZMBHordeDirector* Director = AZMBHordeDirector::Find(WorldContext))
		{
			Director->MarkBaseDirty();
		}
	}

	/** Quanto a cor escurece em cada estágio de dano (inteira, avariada, quase caindo). */
	constexpr float StageShade[] = {1.0f, 0.72f, 0.5f};
	/** Quanto cada parte da barricada entorta por estágio [graus]. */
	constexpr float StageTiltDeg = 3.5f;
}

AZMBBuildPiece::AZMBBuildPiece()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = false;

	Box = CreateDefaultSubobject<UBoxComponent>(TEXT("Box"));
	Box->SetCollisionObjectType(ECC_WorldDynamic);
	Box->SetCanEverAffectNavigation(false);
	SetRootComponent(Box);
}

void AZMBBuildPiece::Setup(const ZR::FBuildPieceType& InType, int32 InRot90)
{
	Type = InType;
	Rot90 = InRot90;
	State.Reset(Type);
	Box->SetBoxExtent(FVector(Type.WidthCm, Type.DepthCm, Type.HeightCm) * 0.5f);
	if (IsBarricade())
	{
		Box->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
		Box->SetCollisionResponseToAllChannels(ECR_Block);
		// a câmera de longe passa por dentro: não pula para perto quando a parede fica entre ela e o jogador
		Box->SetCollisionResponseToChannel(ECC_Camera, ECR_Ignore);
		Box->SetGenerateOverlapEvents(false);
	}
	else
	{
		// armadilha: só sente zumbi; jogador, carro e bala passam por cima
		Box->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
		Box->SetCollisionResponseToAllChannels(ECR_Ignore);
		Box->SetCollisionResponseToChannel(ECC_ZMBZombie, ECR_Overlap);
		Box->SetGenerateOverlapEvents(true);
		SetActorTickEnabled(true);
	}
	BuildLook();
	LastStage = -1;
	UpdateLook();
	if (IsBarricade())
	{
		NotifyBaseChanged(this);
	}
}

UStaticMeshComponent* AZMBBuildPiece::AddPart(EZMBShape Shape, const FVector& LocationCm, const FRotator& Rotation, const FVector& SizeCm,
	const FLinearColor& Color, float Roughness, float Metallic)
{
	UStaticMeshComponent* Part = NewObject<UStaticMeshComponent>(this);
	Part->SetupAttachment(Box);
	Part->SetStaticMesh(UZMBGameData::Shape(Shape));
	Part->SetRelativeLocationAndRotation(LocationCm, Rotation);
	Part->SetRelativeScale3D(SizeCm / 100.0f);
	Part->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Part->SetCanEverAffectNavigation(false);
	Part->RegisterComponent();
	Parts.Add(Part);
	FPartLook Look;
	Look.Color = Color;
	Look.Roughness = Roughness;
	Look.Metallic = Metallic;
	Look.Rest = Rotation;
	Looks.Add(Look);
	return Part;
}

void AZMBBuildPiece::BuildLook()
{
	const FLinearColor Color = UZMBGameData::ToLinear(Type.ColorRgb);
	const FLinearColor Wood(0.16f, 0.09f, 0.04f);
	const FLinearColor Steel(0.18f, 0.18f, 0.19f);
	const float W = Type.WidthCm;
	const float D = Type.DepthCm;
	const float H = Type.HeightCm;
	// o ator tem o centro na metade da altura: o chão fica em -H/2
	const float Floor = -H * 0.5f;

	if (Type.Id == "barricada")
	{
		// dois cavaletes e três tábuas tortas: feita às pressas
		for (int32 Side = -1; Side <= 1; Side += 2)
		{
			const float X = Side * (W * 0.5f - 35.0f);
			AddPart(EZMBShape::Cube, FVector(X, -D * 0.35f, 0.0f), FRotator(0.0f, 0.0f, 16.0f), FVector(8.0f, 8.0f, H), Color * 0.7f, 0.9f, 0.0f);
			AddPart(EZMBShape::Cube, FVector(X, D * 0.35f, 0.0f), FRotator(0.0f, 0.0f, -16.0f), FVector(8.0f, 8.0f, H), Color * 0.7f, 0.9f, 0.0f);
		}
		const float Heights[] = {0.3f, 0.6f, 0.88f};
		const float Tilts[] = {3.0f, -2.0f, 4.0f};
		for (int32 i = 0; i < 3; ++i)
		{
			AddPart(EZMBShape::Cube, FVector(0.0f, 0.0f, Floor + H * Heights[i]), FRotator(Tilts[i], 0.0f, 0.0f), FVector(W, 5.0f, 20.0f),
				Color, 0.85f, 0.0f);
		}
	}
	else if (Type.Id == "muro")
	{
		// chapa de lata entre dois postes, com um remendo enferrujado
		AddPart(EZMBShape::Cube, FVector::ZeroVector, FRotator::ZeroRotator, FVector(W - 12.0f, D * 0.4f, H), Color, 0.55f, 0.6f);
		AddPart(EZMBShape::Cube, FVector(-W * 0.18f, D * 0.22f, Floor + H * 0.55f), FRotator(0.0f, 0.0f, 0.0f), FVector(W * 0.35f, 3.0f, H * 0.45f),
			FLinearColor(0.32f, 0.14f, 0.05f), 0.7f, 0.4f);
		for (int32 Side = -1; Side <= 1; Side += 2)
		{
			AddPart(EZMBShape::Cube, FVector(Side * (W * 0.5f - 6.0f), 0.0f, 4.0f), FRotator::ZeroRotator, FVector(12.0f, D, H + 8.0f), Steel, 0.6f, 0.5f);
		}
	}
	else if (Type.Id == "estacas")
	{
		// prancha no chão com duas fileiras de pontas abertas para fora
		AddPart(EZMBShape::Cube, FVector(0.0f, 0.0f, Floor + 4.0f), FRotator::ZeroRotator, FVector(W, D, 8.0f), Color * 0.8f, 0.9f, 0.0f);
		const int32 PerRow = FMath::Max(3, FMath::FloorToInt32(W / 40.0f));
		for (int32 Row = -1; Row <= 1; Row += 2)
		{
			for (int32 i = 0; i < PerRow; ++i)
			{
				const float X = -W * 0.5f + (i + 0.5f) * W / PerRow;
				AddPart(EZMBShape::Cone, FVector(X, Row * D * 0.22f, Floor + 8.0f + (H - 8.0f) * 0.5f), FRotator(0.0f, 0.0f, Row * 22.0f),
					FVector(9.0f, 9.0f, H - 8.0f), Color, 0.8f, 0.0f);
			}
		}
	}
	else if (Type.Id == "arame")
	{
		// três piquetes com fios esticados e dois em X: de longe lê como cerca de arame farpado
		for (int32 i = -1; i <= 1; ++i)
		{
			AddPart(EZMBShape::Cube, FVector(i * W * 0.5f, 0.0f, 0.0f), FRotator::ZeroRotator, FVector(7.0f, 7.0f, H), Wood, 0.9f, 0.0f);
		}
		const float Diagonal = FMath::RadiansToDegrees(FMath::Atan2(H * 0.7f, W * 0.5f));
		for (int32 Half = -1; Half <= 1; Half += 2)
		{
			const float X = Half * W * 0.25f;
			for (const float Height : {0.25f, 0.55f, 0.85f})
			{
				AddPart(EZMBShape::Cube, FVector(X, 0.0f, Floor + H * Height), FRotator::ZeroRotator, FVector(W * 0.5f, 3.5f, 3.5f), Color, 0.4f, 0.9f);
			}
			const float Length = FMath::Sqrt(FMath::Square(W * 0.5f) + FMath::Square(H * 0.7f));
			AddPart(EZMBShape::Cube, FVector(X, 0.0f, Floor + H * 0.55f), FRotator(Diagonal, 0.0f, 0.0f), FVector(Length, 3.5f, 3.5f), Color, 0.4f, 0.9f);
			AddPart(EZMBShape::Cube, FVector(X, 0.0f, Floor + H * 0.55f), FRotator(-Diagonal, 0.0f, 0.0f), FVector(Length, 3.5f, 3.5f), Color, 0.4f, 0.9f);
		}
	}
	else if (Type.IsExplosive())
	{
		// bujão com a válvula e o arame de tropeço de ponta a ponta
		UStaticMeshComponent* Tank = AddPart(EZMBShape::Cylinder, FVector(0.0f, 0.0f, Floor + 25.0f), FRotator::ZeroRotator, FVector(36.0f, 36.0f, 50.0f),
			Color, 0.3f, 0.2f);
		// só o bujão leva tiro (o resto da armadilha deixa a bala passar)
		Tank->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
		Tank->SetCollisionObjectType(ECC_WorldDynamic);
		Tank->SetCollisionResponseToAllChannels(ECR_Ignore);
		Tank->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
		AddPart(EZMBShape::Cylinder, FVector(0.0f, 0.0f, Floor + 56.0f), FRotator::ZeroRotator, FVector(11.0f, 11.0f, 12.0f), Steel, 0.4f, 0.8f);
		AddPart(EZMBShape::Cube, FVector(0.0f, 0.0f, Floor + 12.0f), FRotator::ZeroRotator, FVector(W, 3.0f, 3.0f), FLinearColor(0.6f, 0.6f, 0.6f), 0.4f, 0.9f);
		for (int32 Side = -1; Side <= 1; Side += 2)
		{
			AddPart(EZMBShape::Cube, FVector(Side * W * 0.5f, 0.0f, Floor + 8.0f), FRotator::ZeroRotator, FVector(4.0f, 4.0f, 16.0f), Wood, 0.9f, 0.0f);
		}
	}
	else
	{
		// id sem visual próprio (peça nova na tabela): um bloco com a cor dela
		AddPart(EZMBShape::Cube, FVector::ZeroVector, FRotator::ZeroRotator, FVector(W, D, H), Color, 0.8f, 0.0f);
	}
}

void AZMBBuildPiece::UpdateLook()
{
	const int32 Stage = FMath::Clamp(ZR::DamageStage(Type, State), 0, 2);
	if (Stage == LastStage)
	{
		return;
	}
	LastStage = Stage;
	UZMBGameData* Data = UZMBGameData::Get(this);
	for (int32 i = 0; i < Parts.Num() && i < Looks.Num(); ++i)
	{
		const FPartLook& Look = Looks[i];
		if (Data)
		{
			Parts[i]->SetMaterial(0, Data->ColorMaterial(Look.Color * StageShade[Stage], Look.Roughness, Look.Metallic));
		}
		// a barricada avariada entorta (cada parte para um lado, sempre igual: dá para ler o estado de longe)
		const float Tilt = IsBarricade() ? ((i % 2) == 0 ? 1.0f : -1.0f) * StageTiltDeg * Stage : 0.0f;
		Parts[i]->SetRelativeRotation(Look.Rest + FRotator(0.0f, 0.0f, Tilt));
	}
}

void AZMBBuildPiece::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (bGone)
	{
		return;
	}
	if (FuseLeft >= 0.0f)
	{
		FuseLeft -= DeltaSeconds;
		if (FuseLeft <= 0.0f)
		{
			Explode();
		}
		return;
	}
	// armadilha olha quem está em cima dez vezes por segundo (o tempo que passou entra no dano)
	TrapClock += DeltaSeconds;
	if (TrapClock < 0.1f)
	{
		return;
	}
	const float Dt = TrapClock;
	TrapClock = 0.0f;
	TickTrap(Dt);
}

void AZMBBuildPiece::TickTrap(float Dt)
{
	TArray<AActor*> Touching;
	Box->GetOverlappingActors(Touching, AZMBZombie::StaticClass());
	if (Touching.Num() == 0)
	{
		return;
	}
	// a armadilha é do jogador: o abate conta para a live
	FZMBHitContext Context;
	Context.Instigator = UGameplayStatics::GetPlayerController(this, 0);
	Context.Cause = EZMBKillCause::Trap;
	for (AActor* Actor : Touching)
	{
		AZMBZombie* Zombie = Cast<AZMBZombie>(Actor);
		if (!Zombie || Zombie->IsDead())
		{
			continue;
		}
		const ZR::FTrapContact Contact = ZR::TouchTrap(Type, State, Dt);
		if (Contact.bDetonate)
		{
			Arm(Context.Instigator.Get(), 0);
			return;
		}
		Zombie->ApplyTrapSlow(Contact.SpeedFactor);
		if (Contact.Damage > 0.0f)
		{
			Zombie->TakeTrapHit(Contact.Damage, Contact.Target == ZR::ETrapTarget::Legs, Context);
		}
		if (Contact.bBrokeNow)
		{
			Break();
			return;
		}
	}
}

void AZMBBuildPiece::TakeZombieHit(float Damage, const FVector& From)
{
	if (bGone)
	{
		return;
	}
	const UZMBGameData* Data = UZMBGameData::Get(this);
	const float Multiplier = Data ? Data->Tuning().Base.ZombieDamageMultiplier : 1.0f;
	FVector Toward = From - GetActorLocation();
	Toward.Z = 0.0f;
	AZMBFx::ImpactDust(GetWorld(), GetActorLocation() + Toward.GetClampedToMaxSize(Type.DepthCm * 0.5f + 10.0f), Toward.GetSafeNormal());
	if (ZR::DamagePiece(State, Damage * Multiplier))
	{
		Break();
		return;
	}
	UpdateLook();
}

void AZMBBuildPiece::TakeBlast(float Damage, AController* DamageInstigator, int32 ChainDepth)
{
	if (bGone || FuseLeft >= 0.0f || Damage <= 0.0f)
	{
		return;
	}
	const UZMBGameData* Data = UZMBGameData::Get(this);
	const float Multiplier = Data ? Data->Tuning().Base.BlastDamageMultiplier : 0.5f;
	if (!ZR::DamagePiece(State, Damage * Multiplier))
	{
		UpdateLook();
		return;
	}
	if (Type.IsExplosive())
	{
		Arm(DamageInstigator, ChainDepth);
		return;
	}
	Break();
}

bool AZMBBuildPiece::TakeVehicleHit(float Damage, const FVector& ImpactPoint)
{
	if (bGone || FuseLeft >= 0.0f || !IsBarricade())
	{
		return false;
	}
	AZMBFx::ImpactDust(GetWorld(), ImpactPoint, (ImpactPoint - GetActorLocation()).GetSafeNormal2D());
	if (ZR::DamagePiece(State, Damage))
	{
		Break();
		return true;
	}
	UpdateLook();
	return false;
}

void AZMBBuildPiece::TakeBullet(AController* DamageInstigator)
{
	if (bGone || FuseLeft >= 0.0f || !Type.IsExplosive())
	{
		return;
	}
	State.bBroken = true;
	Arm(DamageInstigator, 0);
}

void AZMBBuildPiece::Arm(AController* DamageInstigator, int32 ChainDepth)
{
	const UZMBGameData* Data = UZMBGameData::Get(this);
	const ZR::FExplosiveType* Explosive = Data ? Data->FindExplosive(Type.ExplosiveId.c_str()) : nullptr;
	PendingInstigator = DamageInstigator;
	PendingChainDepth = ChainDepth;
	// pavio curto: o clique do arame e já era (em cadeia "pipoca" como os bujões da rua)
	FuseLeft = FMath::Max(0.05f, (Explosive ? Explosive->FuseSeconds : 0.9f) * (ChainDepth > 0 ? FMath::FRandRange(0.25f, 0.6f) : 0.4f));
	SetActorTickEnabled(true);
	if (UZMBGameData* Glow = UZMBGameData::Get(this); Glow && Parts.Num() > 0)
	{
		Parts[0]->SetMaterial(0, Glow->GlowMaterial(FLinearColor(1.0f, 0.3f, 0.05f), 25.0f));
	}
}

void AZMBBuildPiece::Explode()
{
	if (bGone)
	{
		return;
	}
	bGone = true;
	FuseLeft = -1.0f;
	const UZMBGameData* Data = UZMBGameData::Get(this);
	const ZR::FExplosiveType* Explosive = Data ? Data->FindExplosive(Type.ExplosiveId.c_str()) : nullptr;
	// a própria explosão não acha esta peça de novo
	SetActorEnableCollision(false);
	if (Explosive)
	{
		ZMBExplosion::Detonate(GetWorld(), GetActorLocation(), Explosive->Blast, PendingInstigator.Get(), this, PendingChainDepth, Explosive->Noise);
	}
	Destroy();
}

void AZMBBuildPiece::Break()
{
	if (bGone)
	{
		return;
	}
	bGone = true;
	const FLinearColor Color = Looks.Num() > 0 ? Looks.Last().Color : UZMBGameData::ToLinear(Type.ColorRgb);
	AZMBFx::Debris(GetWorld(), GetActorLocation(), IsBarricade() ? 10 : 5, Color, 500.0f);
	if (IsBarricade())
	{
		if (UZMBStreamSubsystem* Stream = UZMBStreamSubsystem::Get(this))
		{
			Stream->React("barricada");
		}
	}
	UE_LOG(LogZombie, Verbose, TEXT("Peça %s quebrou"), *UZMBGameData::ToFString(Type.Id));
	if (IsBarricade())
	{
		NotifyBaseChanged(this);
	}
	Destroy();
}

int32 AZMBBuildPiece::Repair(int32 Scrap)
{
	const UZMBGameData* Data = UZMBGameData::Get(this);
	if (bGone || FuseLeft >= 0.0f || !Data)
	{
		return 0;
	}
	const ZR::FRepairResult Result = ZR::RepairPiece(Type, State, Scrap, Data->Tuning().Base);
	State.Health = Result.Health;
	UpdateLook();
	return Result.ScrapSpent;
}

int32 AZMBBuildPiece::Dismantle()
{
	if (bGone || FuseLeft >= 0.0f)
	{
		return 0;
	}
	const int32 Refund = RefundNow();
	bGone = true;
	AZMBFx::Debris(GetWorld(), GetActorLocation(), 4, Looks.Num() > 0 ? Looks[0].Color : FLinearColor::Gray, 250.0f);
	if (IsBarricade())
	{
		NotifyBaseChanged(this);
	}
	Destroy();
	return Refund;
}

float AZMBBuildPiece::GetHealth01() const
{
	// armadilha gasta conta como avariada: o HUD mostra o pior entre a vida e o desgaste
	return ZR::PieceCondition(Type, State);
}

int32 AZMBBuildPiece::FullRepairCost() const
{
	const UZMBGameData* Data = UZMBGameData::Get(this);
	return Data ? ZR::RepairPiece(Type, State, 1 << 30, Data->Tuning().Base).ScrapSpent : 0;
}

int32 AZMBBuildPiece::RefundNow() const
{
	const UZMBGameData* Data = UZMBGameData::Get(this);
	return Data ? ZR::DismantleRefund(Type, State, Data->Tuning().Base) : 0;
}

ZR::FFootprint AZMBBuildPiece::Footprint(const FVector& WorldOrigin) const
{
	const FVector Local = GetActorLocation() - WorldOrigin;
	return ZR::MakeFootprint(Type, static_cast<float>(Local.X), static_cast<float>(Local.Y), Rot90);
}
