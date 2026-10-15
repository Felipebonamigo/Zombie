#include "Combat/ZMBPickup.h"
#include "Zombie.h"
#include "Audio/ZMBAudioSubsystem.h"
#include "Game/ZMBGameData.h"
#include "Player/ZMBCharacter.h"
#include "Components/PointLightComponent.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "UObject/ConstructorHelpers.h"

AZMBPickup::AZMBPickup()
{
	PrimaryActorTick.bCanEverTick = true;

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMesh(TEXT("/Engine/BasicShapes/Cube.Cube"));
	Crate = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Crate"));
	Crate->SetStaticMesh(CubeMesh.Object);
	Crate->SetRelativeScale3D(FVector(0.9f, 0.9f, 0.7f));
	Crate->SetCollisionProfileName(TEXT("PhysicsActor"));
	Crate->SetCollisionResponseToChannel(ECC_Camera, ECR_Ignore);
	Crate->SetSimulatePhysics(true);
	Crate->SetLinearDamping(1.2f);
	SetRootComponent(Crate);

	// a esfera não escala com a caixa: o raio é absoluto
	Trigger = CreateDefaultSubobject<USphereComponent>(TEXT("Trigger"));
	Trigger->SetupAttachment(Crate);
	Trigger->SetAbsolute(false, false, true);
	Trigger->InitSphereRadius(170.0f);
	Trigger->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	Trigger->SetCollisionResponseToAllChannels(ECR_Ignore);
	Trigger->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	Trigger->SetGenerateOverlapEvents(true);

	Flare = CreateDefaultSubobject<UPointLightComponent>(TEXT("Flare"));
	Flare->SetupAttachment(Crate);
	Flare->SetRelativeLocation(FVector(0.0f, 0.0f, 90.0f));
	Flare->SetIntensityUnits(ELightUnits::Candelas);
	Flare->SetIntensity(3000.0f);
	Flare->SetAttenuationRadius(1800.0f);
	Flare->SetLightColor(FLinearColor(1.0f, 0.1f, 0.05f));
	Flare->SetCastShadows(false);
}

void AZMBPickup::BeginPlay()
{
	Super::BeginPlay();
	if (UZMBGameData* Data = UZMBGameData::Get(this))
	{
		Crate->SetMaterial(0, Data->ColorMaterial(FLinearColor(0.12f, 0.2f, 0.08f), 0.8f));
	}
	Trigger->OnComponentBeginOverlap.AddDynamic(this, &AZMBPickup::HandleOverlap);
	SetLifeSpan(180.0f);
}

void AZMBPickup::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	Age += DeltaSeconds;
	// o sinalizador tremula como fogo
	Flare->SetIntensity(3000.0f * (0.75f + 0.25f * FMath::PerlinNoise1D(Age * 9.0f)));
}

void AZMBPickup::HandleOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp,
	int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	AZMBCharacter* Player = Cast<AZMBCharacter>(OtherActor);
	if (bTaken || !Player || Player->IsDead())
	{
		return;
	}
	bTaken = true;
	UZMBAudioSubsystem::Play(this, "caixa", GetActorLocation());
	Player->Resupply(2, 2, 2, 35.0f);
	if (const UZMBGameData* Data = UZMBGameData::Get(this))
	{
		Player->AddScrap(Data->Tuning().Base.ScrapPerCrate);
	}
	Destroy();
}
