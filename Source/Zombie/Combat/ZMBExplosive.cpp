#include "Combat/ZMBExplosive.h"
#include "Zombie.h"
#include "Audio/ZMBAudioSubsystem.h"
#include "Combat/ZMBExplosion.h"
#include "Game/ZMBGameData.h"
#include "Components/AudioComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/World.h"
#include "GameFramework/Controller.h"

AZMBExplosive::AZMBExplosive()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = false;

	Body = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Body"));
	Body->SetCollisionProfileName(TEXT("PhysicsActor"));
	Body->SetCollisionResponseToChannel(ECC_Camera, ECR_Ignore);
	SetRootComponent(Body);

	Detail = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Detail"));
	Detail->SetupAttachment(Body);
	Detail->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}

void AZMBExplosive::Setup(const ZR::FExplosiveType& InType)
{
	Type = InType;
	Health = Type.Health;
	UZMBGameData* Data = UZMBGameData::Get(this);
	if (!Data)
	{
		return;
	}
	const FLinearColor Color = UZMBGameData::ToLinear(Type.ColorRgb);
	const FVector Base = GetActorScale3D();
	float MassKg = 20.0f;
	if (Type.Id == "carro")
	{
		// carro abandonado: carroceria 4,4 x 1,9 x 0,8 m e cabine em cima (as medidas valem para a escala 1 do ator)
		bIsCar = true;
		Body->SetStaticMesh(UZMBGameData::Shape(EZMBShape::Cube));
		Body->SetWorldScale3D(Base * FVector(4.4f, 1.9f, 0.8f));
		Body->SetMaterial(0, Data->ColorMaterial(Color, 0.35f, 0.4f));
		Detail->SetStaticMesh(UZMBGameData::Shape(EZMBShape::Cube));
		Detail->SetRelativeLocation(FVector(-0.08f, 0.0f, 0.85f) * 100.0f);
		Detail->SetRelativeScale3D(FVector(0.55f, 0.9f, 0.85f));
		Detail->SetMaterial(0, Data->ColorMaterial(FLinearColor(0.02f, 0.025f, 0.03f), 0.08f));
		MassKg = 1200.0f;
	}
	else if (Type.Id == "barril")
	{
		Body->SetStaticMesh(UZMBGameData::Shape(EZMBShape::Cylinder));
		Body->SetWorldScale3D(Base * FVector(0.58f, 0.58f, 0.88f));
		Body->SetMaterial(0, Data->ColorMaterial(Color, 0.5f, 0.6f));
		Detail->SetVisibility(false);
		MassKg = 60.0f;
	}
	else if (Type.Id == "granada")
	{
		Body->SetStaticMesh(UZMBGameData::Shape(EZMBShape::Sphere));
		Body->SetWorldScale3D(Base * FVector(0.1f, 0.1f, 0.13f));
		Body->SetMaterial(0, Data->ColorMaterial(Color, 0.6f, 0.3f));
		Detail->SetVisibility(false);
		MassKg = 0.6f;
	}
	else
	{
		// bujão de 13 kg: corpo vermelho e a válvula em cima (o detalhe que todo brasileiro reconhece de longe)
		Body->SetStaticMesh(UZMBGameData::Shape(EZMBShape::Cylinder));
		Body->SetWorldScale3D(Base * FVector(0.36f, 0.36f, 0.5f));
		Body->SetMaterial(0, Data->ColorMaterial(Color, 0.3f, 0.2f));
		Detail->SetStaticMesh(UZMBGameData::Shape(EZMBShape::Cylinder));
		Detail->SetRelativeLocation(FVector(0.0f, 0.0f, 60.0f));
		Detail->SetRelativeScale3D(FVector(0.3f, 0.3f, 0.25f));
		Detail->SetMaterial(0, Data->ColorMaterial(FLinearColor(0.3f, 0.3f, 0.32f), 0.4f, 0.8f));
		MassKg = 28.0f;
	}
	Body->SetSimulatePhysics(true);
	Body->SetMassOverrideInKg(NAME_None, MassKg, true);
	Body->SetLinearDamping(bIsCar ? 0.2f : 0.05f);
	Body->SetAngularDamping(bIsCar ? 0.6f : 0.2f);
}

void AZMBExplosive::Throw(const FVector& Velocity)
{
	Body->SetPhysicsLinearVelocity(Velocity);
	Body->SetPhysicsAngularVelocityInDegrees(FMath::VRand() * 360.0f);
}

void AZMBExplosive::TakeExplosiveDamage(float Damage, AController* DamageInstigator, int32 ChainDepth)
{
	if (bExploded || bWreck || Damage <= 0.0f)
	{
		return;
	}
	if (DamageInstigator)
	{
		LastInstigator = DamageInstigator;
	}
	Health -= Damage;
	if (Health <= 0.0f && FuseLeft < 0.0f)
	{
		Ignite(ChainDepth);
	}
}

void AZMBExplosive::Arm(AController* DamageInstigator)
{
	LastInstigator = DamageInstigator;
	Ignite(0);
}

void AZMBExplosive::Ignite(int32 ChainDepth)
{
	PendingChainDepth = ChainDepth;
	// em cadeia o pavio fica mais curto e um pouco aleatório: a sequência "pipoca" em vez de estourar tudo junto
	const float Scale = ChainDepth > 0 ? FMath::FRandRange(0.25f, 0.6f) : 1.0f;
	FuseLeft = Type.FuseSeconds * Scale;
	FuseTime = 0.0f;
	if (FuseLeft <= 0.0f)
	{
		Explode();
		return;
	}
	FuseSound = UZMBAudioSubsystem::PlayAttached(this, "chiado", Body);
	FuseLight = NewObject<UPointLightComponent>(this, TEXT("FuseLight"));
	FuseLight->SetupAttachment(Body);
	FuseLight->SetMobility(EComponentMobility::Movable);
	FuseLight->SetRelativeLocation(FVector(0.0f, 0.0f, 60.0f));
	FuseLight->SetIntensityUnits(ELightUnits::Candelas);
	FuseLight->SetIntensity(800.0f);
	FuseLight->SetAttenuationRadius(500.0f);
	FuseLight->SetLightColor(FLinearColor(1.0f, 0.25f, 0.05f));
	FuseLight->SetCastShadows(false);
	FuseLight->RegisterComponent();
	SetActorTickEnabled(true);
}

void AZMBExplosive::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (FuseLeft < 0.0f || bExploded)
	{
		return;
	}
	FuseLeft -= DeltaSeconds;
	FuseTime += DeltaSeconds;
	if (FuseLight)
	{
		// pisca cada vez mais rápido
		const float Rate = 6.0f + FuseTime * 18.0f;
		FuseLight->SetVisibility(FMath::Sin(FuseTime * Rate) > 0.0f);
	}
	if (FuseLeft <= 0.0f)
	{
		Explode();
	}
}

void AZMBExplosive::Explode()
{
	if (bExploded)
	{
		return;
	}
	bExploded = true;
	FuseLeft = -1.0f;
	if (UAudioComponent* Hiss = FuseSound.Get())
	{
		Hiss->Stop();
	}
	if (FuseLight)
	{
		FuseLight->DestroyComponent();
		FuseLight = nullptr;
	}
	const FVector Origin = GetActorLocation();
	ZMBExplosion::Detonate(GetWorld(), Origin, Type.Blast, LastInstigator.Get(), this, PendingChainDepth, Type.Noise);

	if (bIsCar)
	{
		// carcaça: queimada, pula com a explosão e fica
		bWreck = true;
		SetActorTickEnabled(false);
		if (UZMBGameData* Data = UZMBGameData::Get(this))
		{
			Body->SetMaterial(0, Data->ColorMaterial(FLinearColor(0.012f, 0.011f, 0.01f), 0.95f));
			Detail->SetMaterial(0, Data->ColorMaterial(FLinearColor(0.005f, 0.005f, 0.005f), 0.9f));
		}
		Body->AddImpulse(FVector(FMath::FRandRange(-150.0f, 150.0f), FMath::FRandRange(-150.0f, 150.0f), 650.0f), NAME_None, true);
		Body->AddAngularImpulseInDegrees(FMath::VRand() * 200.0f, NAME_None, true);
		return;
	}
	Destroy();
}
