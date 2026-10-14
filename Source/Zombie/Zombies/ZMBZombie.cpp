#include "Zombies/ZMBZombie.h"
#include "Zombie.h"
#include "Base/ZMBBuildPiece.h"
#include "Combat/ZMBExplosion.h"
#include "Combat/ZMBFx.h"
#include "Game/ZMBGameData.h"
#include "Stream/ZMBStreamSubsystem.h"
#include "Zombies/ZMBHordeDirector.h"
#include "Camera/PlayerCameraManager.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/Controller.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "UObject/ConstructorHelpers.h"

namespace
{
	constexpr int32 HeadIndex = static_cast<int32>(ZR::ELimb::Head);
	constexpr int32 TorsoIndex = static_cast<int32>(ZR::ELimb::Torso);
	constexpr int32 LegLIndex = static_cast<int32>(ZR::ELimb::LegL);
	constexpr int32 LegRIndex = static_cast<int32>(ZR::ELimb::LegR);
	constexpr int32 ArmLIndex = static_cast<int32>(ZR::ELimb::ArmL);
	constexpr int32 ArmRIndex = static_cast<int32>(ZR::ELimb::ArmR);

	/** Peça do corpo: só o tiro (Visibility) acerta; não empurra nada nem atrapalha o movimento. */
	void SetupBodyPart(UStaticMeshComponent* Part)
	{
		Part->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
		Part->SetCollisionObjectType(ECC_WorldDynamic);
		Part->SetCollisionResponseToAllChannels(ECR_Ignore);
		Part->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
		Part->SetGenerateOverlapEvents(false);
		Part->SetCanEverAffectNavigation(false);
	}

	const FLinearColor CarnivalColors[] = {
		FLinearColor(0.75f, 0.05f, 0.35f), FLinearColor(0.95f, 0.75f, 0.02f), FLinearColor(0.02f, 0.55f, 0.65f),
		FLinearColor(0.1f, 0.6f, 0.15f), FLinearColor(0.9f, 0.3f, 0.02f), FLinearColor(0.45f, 0.1f, 0.7f),
	};
}

AZMBZombie::AZMBZombie()
{
	PrimaryActorTick.bCanEverTick = true;
	AutoPossessAI = EAutoPossessAI::Disabled;
	AIControllerClass = nullptr;
	bUseControllerRotationYaw = false;

	UCapsuleComponent* Capsule = GetCapsuleComponent();
	Capsule->InitCapsuleSize(34.0f, 88.0f);
	Capsule->SetCollisionObjectType(ECC_ZMBZombie);
	Capsule->SetCollisionResponseToChannel(ECC_Visibility, ECR_Ignore);
	Capsule->SetCollisionResponseToChannel(ECC_Camera, ECR_Ignore);

	UCharacterMovementComponent* Move = GetCharacterMovement();
	Move->bRunPhysicsWithNoController = true;
	Move->bOrientRotationToMovement = true;
	Move->RotationRate = FRotator(0.0f, 300.0f, 0.0f);
	Move->bUseRVOAvoidance = true;
	Move->AvoidanceConsiderationRadius = 220.0f;
	Move->MaxWalkSpeed = 150.0f;

	GetMesh()->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMesh(TEXT("/Engine/BasicShapes/Cube.Cube"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> SphereMesh(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> CylinderMesh(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> ConeMesh(TEXT("/Engine/BasicShapes/Cone.Cone"));

	struct FPartSpec
	{
		const TCHAR* Name;
		UStaticMesh* Mesh;
		FVector Location;
		FRotator Rotation;
		FVector Scale;
	};
	// medidas para a escala 1 (cápsula de 1,76 m): a cabeça no alto, braços esticados para a frente (a silhueta do zumbi)
	const FPartSpec Specs[] = {
		{TEXT("Head"), SphereMesh.Object, FVector(4.0f, 0.0f, 70.0f), FRotator::ZeroRotator, FVector(0.28f)},
		{TEXT("Torso"), CubeMesh.Object, FVector(0.0f, 0.0f, 24.0f), FRotator::ZeroRotator, FVector(0.26f, 0.42f, 0.6f)},
		{TEXT("ArmL"), CylinderMesh.Object, FVector(24.0f, -27.0f, 40.0f), FRotator(-80.0f, 0.0f, 0.0f), FVector(0.11f, 0.11f, 0.6f)},
		{TEXT("ArmR"), CylinderMesh.Object, FVector(24.0f, 27.0f, 40.0f), FRotator(-80.0f, 0.0f, 0.0f), FVector(0.11f, 0.11f, 0.6f)},
		{TEXT("LegL"), CylinderMesh.Object, FVector(0.0f, -11.0f, -46.0f), FRotator::ZeroRotator, FVector(0.15f, 0.15f, 0.84f)},
		{TEXT("LegR"), CylinderMesh.Object, FVector(0.0f, 11.0f, -46.0f), FRotator::ZeroRotator, FVector(0.15f, 0.15f, 0.84f)},
	};
	for (const FPartSpec& Spec : Specs)
	{
		UStaticMeshComponent* Part = CreateDefaultSubobject<UStaticMeshComponent>(Spec.Name);
		Part->SetupAttachment(Capsule);
		Part->SetStaticMesh(Spec.Mesh);
		Part->SetRelativeLocation(Spec.Location);
		Part->SetRelativeRotation(Spec.Rotation);
		Part->SetRelativeScale3D(Spec.Scale);
		SetupBodyPart(Part);
		Limbs.Add(Part);
	}

	Tank = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Tank"));
	Tank->SetupAttachment(Capsule);
	Tank->SetStaticMesh(CylinderMesh.Object);
	Tank->SetRelativeLocation(FVector(-24.0f, 0.0f, 26.0f));
	Tank->SetRelativeScale3D(FVector(0.32f, 0.32f, 0.5f));
	SetupBodyPart(Tank);
	Tank->SetVisibility(false);
	Tank->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	Hat = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Hat"));
	Hat->SetupAttachment(Limbs[HeadIndex]);
	Hat->SetStaticMesh(ConeMesh.Object);
	Hat->SetRelativeLocation(FVector(0.0f, 0.0f, 60.0f));
	Hat->SetRelativeScale3D(FVector(0.9f, 0.9f, 1.0f));
	Hat->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Hat->SetVisibility(false);
}

void AZMBZombie::InitZombie(const ZR::FZombieType& InType, const FString& InViewerName, bool bInHorde)
{
	Type = InType;
	Body.Reset(Type.Body);
	ViewerName = InViewerName;
	bHorde = bInHorde;
}

void AZMBZombie::BeginPlay()
{
	Super::BeginPlay();
	if (Type.Id.empty())
	{
		// nasceu sem InitZombie (colocado à mão no mapa): vira um comum com os padrões
		Type.Id = "comum";
		Body.Reset(Type.Body);
	}
	WobblePhase = FMath::FRandRange(0.0f, 10.0f);
	WalkPhase = FMath::FRandRange(0.0f, 6.0f);
	GetCharacterMovement()->MaxWalkSpeed = Type.WalkSpeed;

	UZMBGameData* Data = UZMBGameData::Get(this);
	if (Data)
	{
		const float Tint = FMath::FRandRange(0.85f, 1.15f);
		const FLinearColor Skin = FLinearColor(0.30f, 0.34f, 0.26f) * Tint;
		FLinearColor Clothes = UZMBGameData::ToLinear(Type.ColorRgb) * FMath::FRandRange(0.8f, 1.1f);
		if (Type.Id == "foliao")
		{
			const int32 Last = static_cast<int32>(UE_ARRAY_COUNT(CarnivalColors)) - 1;
			Clothes = CarnivalColors[FMath::RandRange(0, Last)];
			Hat->SetVisibility(true);
			Hat->SetMaterial(0, Data->ColorMaterial(CarnivalColors[FMath::RandRange(0, Last)], 0.4f));
		}
		const FLinearColor Pants = FLinearColor(0.04f, 0.04f, 0.05f) * Tint;
		Limbs[HeadIndex]->SetMaterial(0, Data->ColorMaterial(Skin, 0.6f));
		Limbs[ArmLIndex]->SetMaterial(0, Data->ColorMaterial(Skin, 0.6f));
		Limbs[ArmRIndex]->SetMaterial(0, Data->ColorMaterial(Skin, 0.6f));
		Limbs[TorsoIndex]->SetMaterial(0, Data->ColorMaterial(Clothes, 0.8f));
		Limbs[LegLIndex]->SetMaterial(0, Data->ColorMaterial(Pants, 0.85f));
		Limbs[LegRIndex]->SetMaterial(0, Data->ColorMaterial(Pants, 0.85f));

		if (Type.Id == "bujao")
		{
			Tank->SetVisibility(true);
			Tank->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
			Tank->SetMaterial(0, Data->ColorMaterial(FLinearColor(0.55f, 0.03f, 0.02f), 0.3f, 0.2f));
		}
		else if (Type.Id == "inchado")
		{
			Limbs[TorsoIndex]->SetRelativeScale3D(FVector(0.5f, 0.6f, 0.7f));
			Limbs[TorsoIndex]->SetMaterial(0, Data->ColorMaterial(Clothes, 0.35f));
		}
		else if (Type.Id == "brutamontes")
		{
			// capacete e colete do choque, por cima da cabeça e do tronco
			Hat->SetStaticMesh(UZMBGameData::Shape(EZMBShape::Sphere));
			Hat->SetRelativeLocation(FVector(0.0f, 0.0f, 12.0f));
			Hat->SetRelativeScale3D(FVector(1.18f));
			Hat->SetVisibility(true);
			Hat->SetMaterial(0, Data->ColorMaterial(FLinearColor(0.02f, 0.025f, 0.035f), 0.25f, 0.5f));
		}
	}

	for (UStaticMeshComponent* Part : Limbs)
	{
		RestTransforms.Add(Part->GetRelativeTransform());
	}

	if (IsNamed())
	{
		Label = NewObject<UTextRenderComponent>(this, TEXT("ViewerLabel"));
		Label->SetupAttachment(GetCapsuleComponent());
		Label->SetRelativeLocation(FVector(0.0f, 0.0f, 125.0f));
		Label->SetHorizontalAlignment(EHTA_Center);
		Label->SetWorldSize(32.0f);
		Label->SetTextRenderColor(FColor(255, 214, 64));
		Label->SetText(FText::FromString(ViewerName));
		Label->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Label->RegisterComponent();
	}
}

int32 AZMBZombie::LimbIndexOf(const UPrimitiveComponent* Component) const
{
	for (int32 i = 0; i < Limbs.Num(); ++i)
	{
		if (Limbs[i] == Component)
		{
			return i;
		}
	}
	return INDEX_NONE;
}

void AZMBZombie::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (bExplodePending)
	{
		ExplodeTimer -= DeltaSeconds;
		if (Tank->IsVisible())
		{
			if (UZMBGameData* Data = UZMBGameData::Get(this))
			{
				const bool bBright = FMath::Fmod(ExplodeTimer, 0.16f) < 0.08f;
				Tank->SetMaterial(0, bBright ? Data->GlowMaterial(FLinearColor(1.0f, 0.3f, 0.05f), 25.0f)
					: Data->ColorMaterial(FLinearColor(0.55f, 0.03f, 0.02f), 0.3f, 0.2f));
			}
		}
		if (ExplodeTimer <= 0.0f)
		{
			ExplodeSelf();
		}
	}
	if (Body.bDead)
	{
		return;
	}

	APawn* Target = UGameplayStatics::GetPlayerPawn(this, 0);
	if (!Target)
	{
		return;
	}
	const FVector Me = GetActorLocation();
	FVector ToTarget = Target->GetActorLocation() - Me;
	ToTarget.Z = 0.0f;
	const float Distance = ToTarget.Size();

	AZMBHordeDirector* Director = AZMBHordeDirector::Find(this);
	const bool bNight = Director && Director->IsNight();
	const bool bHordeNow = bHorde || (Director && Director->IsHordeActive());
	const bool bRun = Distance < 1800.0f || bNight || bHordeNow;
	float Speed = bRun ? Type.RunSpeed : Type.WalkSpeed;
	if (bCrawler)
	{
		Speed *= 0.3f;
	}
	TrapSlowLeft -= DeltaSeconds;
	if (TrapSlowLeft > 0.0f)
	{
		Speed *= TrapSlowFactor;
	}
	GetCharacterMovement()->MaxWalkSpeed = Speed;

	// longe: segue a rua pelo campo de fluxo; perto (ou fora da grade, num beco): direto no jogador
	FVector Direction = FVector::ZeroVector;
	if (Distance < 1500.0f || !Director || !Director->FlowDirection(Me, Direction))
	{
		Direction = ToTarget.GetSafeNormal();
	}
	const FVector Heading = Direction;
	BlockCheckTimer -= DeltaSeconds;
	if (BlockCheckTimer <= 0.0f)
	{
		BlockCheckTimer = 0.25f;
		BlockingPiece = Distance < 4000.0f ? FindBlockingBarricade(Heading) : nullptr;
	}
	WobblePhase += DeltaSeconds;
	Direction = Direction.RotateAngleAxis(FMath::Sin(WobblePhase * 1.7f) * 14.0f, FVector::UpVector);

	// gritador: ao chegar a 25 m do jogador, para e grita uma vez, chamando a horda
	if (Type.Id == "gritador" && !bScreamed && Distance < 2500.0f)
	{
		Scream();
	}
	ScreamLeft = FMath::Max(0.0f, ScreamLeft - DeltaSeconds);
	if (ScreamLeft > 0.0f)
	{
		SetActorRotation(FRotator(0.0f, ToTarget.Rotation().Yaw, 0.0f));
		// 0,15 s é o meio da estocada (seno = 1): braços para cima o grito inteiro, e descem suave no fim
		LungeTimer = 0.15f;
		Animate(DeltaSeconds, 0.0f);
		UpdateLabel();
		return;
	}
	AttackCooldown -= DeltaSeconds;
	LungeTimer -= DeltaSeconds;
	const float HeightGap = FMath::Abs(Target->GetActorLocation().Z - Me.Z);
	const bool bPlayerInReach = Distance < Type.AttackRangeCm && HeightGap < 220.0f;

	// barricada na frente e o jogador fora do alcance: para e bate nela até quebrar. Só começa travado (parado contra
	// ela); quem vem de lado escorrega pela parede e dá a volta. Depois de começar, continua até ela cair ou sair da frente.
	AZMBBuildPiece* Barricade = BlockingPiece.Get();
	bBashing = Barricade && !Barricade->IsGone() && !bPlayerInReach && (bBashing || GetVelocity().Size2D() < 40.0f);
	if (bBashing)
	{
		SetActorRotation(FRotator(0.0f, Heading.Rotation().Yaw, 0.0f));
		if (AttackCooldown <= 0.0f)
		{
			AttackCooldown = Type.AttackIntervalSeconds;
			LungeTimer = 0.3f;
			Barricade->TakeZombieHit(Type.AttackDamage * (Body.ArmsLeft() > 0 ? 1.0f : 0.5f), Me);
		}
		Animate(DeltaSeconds, 0.0f);
		UpdateLabel();
		return;
	}

	if (Distance > Type.AttackRangeCm * 0.6f)
	{
		AddMovementInput(Direction, 1.0f);
	}
	if (bPlayerInReach && AttackCooldown <= 0.0f)
	{
		AttackCooldown = Type.AttackIntervalSeconds;
		LungeTimer = 0.3f;
		// sem braços morde mais fraco
		const float Damage = Type.AttackDamage * (Body.ArmsLeft() > 0 ? 1.0f : 0.5f);
		UGameplayStatics::ApplyDamage(Target, Damage, nullptr, this, nullptr);
	}

	Animate(DeltaSeconds, GetVelocity().Size2D());
	UpdateLabel();
}

AZMBBuildPiece* AZMBZombie::FindBlockingBarricade(const FVector& Heading) const
{
	UWorld* World = GetWorld();
	const FVector Flat = Heading.GetSafeNormal2D();
	if (!World || Flat.IsNearlyZero())
	{
		return nullptr;
	}
	const FVector Start = GetActorLocation();
	const FVector End = Start + Flat * (GetCapsuleComponent()->GetScaledCapsuleRadius() + 70.0f);
	FCollisionQueryParams Params(SCENE_QUERY_STAT(ZMBBarricade), false, this);
	FHitResult Hit;
	if (!World->SweepSingleByObjectType(Hit, Start, End, FQuat::Identity, FCollisionObjectQueryParams(ECC_WorldDynamic),
			FCollisionShape::MakeSphere(20.0f), Params))
	{
		return nullptr;
	}
	AZMBBuildPiece* Piece = Cast<AZMBBuildPiece>(Hit.GetActor());
	return Piece && Piece->IsBarricade() && !Piece->IsGone() ? Piece : nullptr;
}

void AZMBZombie::ApplyTrapSlow(float Factor)
{
	TrapSlowFactor = TrapSlowLeft > 0.0f ? FMath::Min(TrapSlowFactor, Factor) : Factor;
	TrapSlowLeft = 0.25f;
}

ZR::FHitOutcome AZMBZombie::TakeTrapHit(float Damage, bool bLegs, const FZMBHitContext& Context)
{
	ZR::FHitOutcome None;
	if (Body.bGibbed || Body.bDead || Damage <= 0.0f)
	{
		return None;
	}
	int32 Limb = TorsoIndex;
	if (bLegs)
	{
		// sempre a mesma perna (pelo id do ator) até ela sair: a estaca "come" uma perna e o zumbi vira rastejante
		const int32 First = (GetUniqueID() % 2) == 0 ? LegLIndex : LegRIndex;
		const int32 Second = First == LegLIndex ? LegRIndex : LegLIndex;
		Limb = !Body.Severed[First] ? First : (!Body.Severed[Second] ? Second : TorsoIndex);
	}
	LastHit = Context;
	const ZR::FHitOutcome Outcome = ZR::ApplyHit(Type.Body, Body, static_cast<ZR::ELimb>(Limb), Damage);
	const FVector Where = Limbs.IsValidIndex(Limb) && Limbs[Limb] ? Limbs[Limb]->GetComponentLocation() : GetActorLocation();
	if (FMath::FRand() < 0.3f)
	{
		AZMBFx::Blood(GetWorld(), Where, FVector(0.0f, 0.0f, 120.0f), 1);
	}
	ApplyOutcome(Outcome, Where, FVector(0.0f, 0.0f, 150.0f), false, Context);
	return Outcome;
}

void AZMBZombie::Animate(float DeltaSeconds, float Speed)
{
	if (RestTransforms.Num() != Limbs.Num())
	{
		return;
	}
	WalkPhase += DeltaSeconds * Speed / 35.0f;
	const float Swing = FMath::Sin(WalkPhase) * FMath::Clamp(Speed / 200.0f, 0.0f, 1.0f) * (bCrawler ? 10.0f : 24.0f);
	const float Lunge = LungeTimer > 0.0f ? -25.0f * FMath::Sin(LungeTimer / 0.3f * UE_PI) : 0.0f;
	for (int32 i = 0; i < Limbs.Num(); ++i)
	{
		UStaticMeshComponent* Part = Limbs[i];
		if (!Part || Body.Severed[i] || Part->IsSimulatingPhysics())
		{
			continue;
		}
		FRotator Delta = FRotator::ZeroRotator;
		if (i == LegLIndex)
		{
			Delta.Pitch = Swing;
		}
		else if (i == LegRIndex)
		{
			Delta.Pitch = -Swing;
		}
		else if (i == ArmLIndex || i == ArmRIndex)
		{
			Delta.Pitch = FMath::Sin(WalkPhase * 0.5f + (i == ArmLIndex ? 0.0f : 1.3f)) * 6.0f + Lunge;
		}
		else if (i == HeadIndex)
		{
			Delta.Roll = FMath::Sin(WobblePhase * 1.1f) * 12.0f;
		}
		Part->SetRelativeRotation(RestTransforms[i].Rotator() + Delta);
	}
}

void AZMBZombie::Scream()
{
	bScreamed = true;
	ScreamLeft = 1.6f;
	AZMBHordeDirector* Director = AZMBHordeDirector::Find(this);
	const int32 Called = Director ? Director->CallHorde(GetActorLocation(), 5) : 0;
	// sem ninguém para chamar (teto de vivos cheio), o grito não vira anúncio
	UZMBStreamSubsystem* Stream = Called > 0 ? UZMBStreamSubsystem::Get(this) : nullptr;
	if (Stream)
	{
		Stream->Announce(NSLOCTEXT("ZombieZombie", "Scream", "UM GRITADOR CHAMOU A HORDA"));
		Stream->React("horda");
	}
}

void AZMBZombie::UpdateLabel()
{
	if (!Label)
	{
		return;
	}
	if (const APlayerCameraManager* Camera = UGameplayStatics::GetPlayerCameraManager(this, 0))
	{
		const FVector ToCamera = Camera->GetCameraLocation() - Label->GetComponentLocation();
		Label->SetWorldRotation(ToCamera.Rotation());
	}
}

ZR::FHitOutcome AZMBZombie::TakeLimbHit(UPrimitiveComponent* HitComponent, float Damage, const FVector& HitLocation, const FVector& Impulse,
	const FZMBHitContext& Context)
{
	ZR::FHitOutcome None;
	if (Body.bGibbed)
	{
		return None;
	}
	if (HitComponent && HitComponent == Tank && Tank->IsVisible() && !Tank->IsSimulatingPhysics())
	{
		ArmTank(Context);
		AZMBFx::ImpactDust(GetWorld(), HitLocation, -Impulse.GetSafeNormal());
		return None;
	}
	int32 Limb = LimbIndexOf(HitComponent);
	if (Limb == INDEX_NONE)
	{
		Limb = TorsoIndex;
	}
	if (Body.Severed[Limb] && Limbs[Limb]->IsSimulatingPhysics())
	{
		// pedaço solto: o tiro só empurra (dá para "fazer embaixadinha" com uma cabeça)
		Limbs[Limb]->AddImpulse(Impulse * 1.5f, NAME_None, true);
		return None;
	}
	LastHit = Context;
	const ZR::FHitOutcome Outcome = ZR::ApplyHit(Type.Body, Body, static_cast<ZR::ELimb>(Limb), Damage);
	AZMBFx::Blood(GetWorld(), HitLocation, Impulse, Limb == HeadIndex ? 5 : 2);
	ApplyOutcome(Outcome, HitLocation, Impulse, false, Context);
	return Outcome;
}

ZR::FHitOutcome AZMBZombie::TakeBlast(float Damage, const FVector& Origin, float ImpulseStrength, const FZMBHitContext& Context)
{
	ZR::FHitOutcome None;
	if (Body.bGibbed || Damage <= 0.0f)
	{
		return None;
	}
	LastHit = Context;
	// o bujão nas costas pega fogo com a explosão do vizinho: é assim que a fila de bujões vira cadeia
	if (Tank->IsVisible() && !bTankArmed && Damage > 30.0f)
	{
		ArmTank(Context);
	}
	const ZR::FHitOutcome Outcome = ZR::ApplyBlast(Type.Body, Body, Damage);
	if (!Body.bDead)
	{
		const FVector Away = (GetActorLocation() - Origin).GetSafeNormal2D();
		LaunchCharacter(Away * ImpulseStrength * 0.4f + FVector(0.0f, 0.0f, ImpulseStrength * 0.25f), true, true);
	}
	ApplyOutcome(Outcome, Origin, FVector(ImpulseStrength, 0.0f, 0.0f), true, Context);
	if (Outcome.bGibbed)
	{
		AZMBFx::Blood(GetWorld(), GetActorLocation(), FVector::UpVector, 10);
	}
	return Outcome;
}

ZR::FHitOutcome AZMBZombie::TakeVehicleHit(float Damage, const FVector& Velocity, const FZMBHitContext& Context)
{
	ZR::FHitOutcome None;
	if (Body.bGibbed || Body.bDead)
	{
		return None;
	}
	LastHit = Context;
	const ZR::FHitOutcome Outcome = ZR::ApplyBlast(Type.Body, Body, Damage);
	if (!Body.bDead)
	{
		LaunchCharacter(Velocity * 0.6f + FVector(0.0f, 0.0f, 450.0f), true, true);
	}
	// "origem" atrás do zumbi, na direção de onde o carro veio: os pedaços voam para a frente do carro
	const FVector Origin = GetActorLocation() - Velocity.GetSafeNormal() * 150.0f;
	ApplyOutcome(Outcome, Origin, FVector(Velocity.Size() * 0.9f, 0.0f, 0.0f), true, Context);
	AZMBFx::Blood(GetWorld(), GetActorLocation(), Velocity, Outcome.bGibbed ? 10 : 4);
	return Outcome;
}

void AZMBZombie::ApplyOutcome(const ZR::FHitOutcome& Outcome, const FVector& Origin, const FVector& Impulse, bool bFromBlast,
	const FZMBHitContext& Context)
{
	int32 SeveredCount = 0;
	for (int32 i = 0; i < ZR::LimbCount && i < Limbs.Num(); ++i)
	{
		if ((Outcome.SeveredMask & (1 << i)) == 0)
		{
			continue;
		}
		++SeveredCount;
		FVector PartImpulse = Impulse;
		if (bFromBlast)
		{
			// Impulse.X carrega a força; a direção sai do centro da explosão para o membro
			const FVector Away = (Limbs[i]->GetComponentLocation() - Origin).GetSafeNormal();
			PartImpulse = (Away + FVector(0.0f, 0.0f, 0.5f)) * Impulse.X * FMath::FRandRange(0.6f, 1.0f);
		}
		SeverLimb(i, PartImpulse, !bFromBlast && i == HeadIndex);
	}
	if (Outcome.bBecameCrawler && !Outcome.bKilled)
	{
		BecomeCrawler();
	}
	if (Outcome.bKilled)
	{
		Die(Outcome, Context);
	}
	else if (SeveredCount > 0 && Context.Instigator.IsValid() && Context.Instigator->IsPlayerController())
	{
		if (UZMBStreamSubsystem* Stream = UZMBStreamSubsystem::Get(this))
		{
			Stream->ReportHype(ZR::EHypeEvent::Dismember, SeveredCount, GetActorLocation());
		}
	}
}

void AZMBZombie::SeverLimb(int32 Limb, const FVector& Impulse, bool bPop)
{
	if (!Limbs.IsValidIndex(Limb))
	{
		return;
	}
	UStaticMeshComponent* Part = Limbs[Limb];
	if (!Part || Part->IsSimulatingPhysics() || !Part->IsVisible())
	{
		return;
	}
	if (bPop)
	{
		// cabeça estourada pelo tiro: some numa nuvem de sangue
		Part->SetVisibility(false, true);
		Part->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		AZMBFx::Blood(GetWorld(), Part->GetComponentLocation(), Impulse, 8);
		return;
	}
	Part->DetachFromComponent(FDetachmentTransformRules::KeepWorldTransform);
	Part->SetCollisionProfileName(TEXT("PhysicsActor"));
	Part->SetCollisionResponseToChannel(ECC_Pawn, ECR_Ignore);
	Part->SetCollisionResponseToChannel(ECC_ZMBZombie, ECR_Ignore);
	Part->SetCollisionResponseToChannel(ECC_Camera, ECR_Ignore);
	Part->SetSimulatePhysics(true);
	Part->AddImpulse(Impulse, NAME_None, true);
	Part->AddAngularImpulseInDegrees(FMath::VRand() * 600.0f, NAME_None, true);
	AZMBFx::Blood(GetWorld(), Part->GetComponentLocation(), Impulse, 2);
}

void AZMBZombie::BecomeCrawler()
{
	bCrawler = true;
	if (RestTransforms.Num() != Limbs.Num())
	{
		return;
	}
	// deita: tronco na horizontal rente ao chão, cabeça na frente, braços puxando
	const FTransform Lying[] = {
		FTransform(FRotator::ZeroRotator, FVector(64.0f, 0.0f, -66.0f), FVector(0.28f)),
		FTransform(FRotator(-88.0f, 0.0f, 0.0f), FVector(22.0f, 0.0f, -70.0f), FVector(0.26f, 0.42f, 0.6f)),
		FTransform(FRotator(-95.0f, 0.0f, 0.0f), FVector(70.0f, -24.0f, -78.0f), FVector(0.11f, 0.11f, 0.6f)),
		FTransform(FRotator(-95.0f, 0.0f, 0.0f), FVector(70.0f, 24.0f, -78.0f), FVector(0.11f, 0.11f, 0.6f)),
		FTransform(FRotator(88.0f, 0.0f, 0.0f), FVector(-45.0f, -11.0f, -78.0f), FVector(0.15f, 0.15f, 0.84f)),
		FTransform(FRotator(88.0f, 0.0f, 0.0f), FVector(-45.0f, 11.0f, -78.0f), FVector(0.15f, 0.15f, 0.84f)),
	};
	for (int32 i = 0; i < Limbs.Num() && i < static_cast<int32>(UE_ARRAY_COUNT(Lying)); ++i)
	{
		RestTransforms[i] = Lying[i];
		if (!Body.Severed[i] && !Limbs[i]->IsSimulatingPhysics())
		{
			Limbs[i]->SetRelativeTransform(Lying[i]);
		}
	}
	Tank->SetRelativeLocation(FVector(22.0f, 0.0f, -40.0f));
}

void AZMBZombie::Die(const ZR::FHitOutcome& Outcome, const FZMBHitContext& Context)
{
	UCharacterMovementComponent* Move = GetCharacterMovement();
	const FVector LastVelocity = GetVelocity();
	Move->StopMovementImmediately();
	Move->DisableMovement();
	GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	if (Label)
	{
		Label->SetVisibility(false);
	}

	// o que ainda estava preso cai com física: o corpo desmorona em pedaços
	for (int32 i = 0; i < Limbs.Num(); ++i)
	{
		if (!Body.Severed[i] || i == TorsoIndex)
		{
			SeverLimb(i, LastVelocity * 0.5f + FMath::VRand() * 120.0f + FVector(0.0f, 0.0f, 80.0f), false);
		}
	}

	const UZMBGameData* Data = UZMBGameData::Get(this);
	SetLifeSpan(Data ? Data->Tuning().CorpseSeconds : 12.0f);

	if (Type.bExplodes && !bExplodePending && !bSelfExploded)
	{
		bExplodePending = true;
		ExplodeTimer = 0.25f;
	}

	const bool bPlayerKill = Context.Instigator.IsValid() && Context.Instigator->IsPlayerController();
	if (AZMBHordeDirector* Director = AZMBHordeDirector::Find(this))
	{
		Director->ReportZombieDied(this, bPlayerKill);
	}
	if (!bPlayerKill)
	{
		return;
	}
	if (UZMBStreamSubsystem* Stream = UZMBStreamSubsystem::Get(this))
	{
		FZMBKillInfo Info;
		Info.Cause = Context.Cause;
		Info.bHeadshot = Outcome.bHeadshotKill;
		Info.bGibbed = Outcome.bGibbed;
		Info.bSlowMo = Context.bSlowMo;
		Info.bNamed = IsNamed();
		Info.ViewerName = ViewerName;
		Info.DistanceCm = Context.DistanceCm;
		Info.Location = GetActorLocation();
		for (int32 i = 0; i < ZR::LimbCount; ++i)
		{
			Info.SeveredLimbs += (i != HeadIndex && i != TorsoIndex && (Outcome.SeveredMask & (1 << i)) != 0) ? 1 : 0;
		}
		Stream->ReportKill(Info);
		if (Type.Id == "gritador" && !bScreamed)
		{
			Stream->ReportHype(ZR::EHypeEvent::Silenced, 1, Info.Location);
		}
	}
}

void AZMBZombie::ArmTank(const FZMBHitContext& Context)
{
	if (bTankArmed)
	{
		return;
	}
	bTankArmed = true;
	LastHit = Context;
	bExplodePending = true;
	// baleado chia um pouquinho (dá tempo de ver e correr); aceso por outra explosão estoura quase junto
	ExplodeTimer = Context.Cause == EZMBKillCause::Blast ? FMath::FRandRange(0.15f, 0.35f) : 0.6f;
}

void AZMBZombie::ExplodeSelf()
{
	bExplodePending = false;
	bSelfExploded = true;
	if (!Body.bGibbed)
	{
		FZMBHitContext Context = LastHit;
		Context.Cause = EZMBKillCause::Blast;
		const bool bWasAlive = !Body.bDead;
		for (int32 i = 0; i < ZR::LimbCount; ++i)
		{
			Body.Severed[i] = true;
		}
		Body.bGibbed = true;
		Body.bDead = true;
		const FVector Center = GetActorLocation();
		for (int32 i = 0; i < Limbs.Num(); ++i)
		{
			const FVector Away = (Limbs[i]->GetComponentLocation() - Center).GetSafeNormal();
			SeverLimb(i, (Away + FVector(0.0f, 0.0f, 0.8f)) * 1600.0f, false);
		}
		if (bWasAlive)
		{
			ZR::FHitOutcome Outcome;
			Outcome.bKilled = true;
			Outcome.bGibbed = true;
			Outcome.SeveredMask = ZR::AllLimbsMask;
			Die(Outcome, Context);
		}
	}
	Tank->SetVisibility(false);
	Tank->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	AZMBFx::Blood(GetWorld(), GetActorLocation(), FVector::UpVector, 12);
	ZMBExplosion::Detonate(GetWorld(), GetActorLocation(), Type.Blast, LastHit.Instigator.Get(), this, LastHit.ChainDepth, 45.0f);
}
