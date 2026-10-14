#include "Combat/ZMBFx.h"
#include "Zombie.h"
#include "Game/ZMBGameData.h"
#include "Components/PointLightComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/World.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"

int32 AZMBFx::PhysicsPieces = 0;

namespace
{
	const FLinearColor FireColor(1.0f, 0.42f, 0.08f);
	const FLinearColor BloodColor(0.16f, 0.005f, 0.004f);
	const FLinearColor SmokeColor(0.035f, 0.033f, 0.03f);
}

AZMBFx::AZMBFx()
{
	PrimaryActorTick.bCanEverTick = true;
	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Mesh->SetCastShadow(false);
	Mesh->SetGenerateOverlapEvents(false);
	SetRootComponent(Mesh);
}

AZMBFx* AZMBFx::SpawnFx(UWorld* World, const FVector& Location, const FRotator& Rotation, float InLifetime)
{
	if (!World)
	{
		return nullptr;
	}
	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	AZMBFx* Fx = World->SpawnActor<AZMBFx>(AZMBFx::StaticClass(), Location, Rotation, Params);
	if (Fx)
	{
		Fx->Lifetime = FMath::Max(0.02f, InLifetime);
		Fx->SetLifeSpan(Fx->Lifetime);
	}
	return Fx;
}

bool AZMBFx::CanSpawnPiece(UWorld* World)
{
	const UZMBGameData* Data = UZMBGameData::Get(World);
	const int32 Budget = Data ? Data->Tuning().MaxGibs : 220;
	return PhysicsPieces < Budget;
}

void AZMBFx::SetShape(int32 ShapeIndex, UMaterialInterface* Material, const FVector& InStartScale, const FVector& InEndScale)
{
	Mesh->SetStaticMesh(UZMBGameData::Shape(static_cast<EZMBShape>(ShapeIndex)));
	if (Material)
	{
		Mesh->SetMaterial(0, Material);
	}
	StartScale = InStartScale;
	EndScale = InEndScale;
	SetActorScale3D(StartScale);
}

void AZMBFx::AddFlash(const FLinearColor& Color, float Intensity, float Radius)
{
	Light = NewObject<UPointLightComponent>(this, TEXT("Flash"));
	Light->SetupAttachment(Mesh);
	Light->SetMobility(EComponentMobility::Movable);
	Light->SetIntensityUnits(ELightUnits::Candelas);
	Light->SetIntensity(Intensity);
	Light->SetAttenuationRadius(Radius);
	Light->SetLightColor(Color);
	Light->SetCastShadows(false);
	Light->SetAbsolute(false, false, true);
	Light->RegisterComponent();
	LightStart = Intensity;
}

void AZMBFx::MakePhysicsPiece(const FVector& Velocity)
{
	bPhysicsPiece = true;
	++PhysicsPieces;
	Mesh->SetCollisionProfileName(TEXT("PhysicsActor"));
	Mesh->SetCollisionResponseToChannel(ECC_Pawn, ECR_Ignore);
	Mesh->SetCollisionResponseToChannel(ECC_ZMBZombie, ECR_Ignore);
	Mesh->SetCollisionResponseToChannel(ECC_Camera, ECR_Ignore);
	Mesh->SetCollisionResponseToChannel(ECC_Visibility, ECR_Ignore);
	Mesh->SetCastShadow(true);
	Mesh->SetSimulatePhysics(true);
	Mesh->SetPhysicsLinearVelocity(Velocity);
	Mesh->SetPhysicsAngularVelocityInDegrees(FMath::VRand() * FMath::FRandRange(180.0f, 720.0f));
}

void AZMBFx::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (bPhysicsPiece)
	{
		bPhysicsPiece = false;
		PhysicsPieces = FMath::Max(0, PhysicsPieces - 1);
	}
	Super::EndPlay(EndPlayReason);
}

void AZMBFx::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	Age += DeltaSeconds;
	const float T = FMath::Clamp(Age / Lifetime, 0.0f, 1.0f);
	if (!bPhysicsPiece)
	{
		const float Grow = FMath::Clamp(T / FMath::Max(GrowFraction, 0.01f), 0.0f, 1.0f);
		FVector Scale = FMath::Lerp(StartScale, EndScale, FMath::InterpEaseOut(0.0f, 1.0f, Grow, 2.0f));
		if (bShrinkAtEnd && T > GrowFraction)
		{
			const float Fade = (T - GrowFraction) / FMath::Max(1.0f - GrowFraction, 0.01f);
			Scale *= FMath::Max(0.0f, 1.0f - Fade);
		}
		SetActorScale3D(Scale.ComponentMax(FVector(0.001f)));
		if (!Drift.IsNearlyZero())
		{
			AddActorWorldOffset(Drift * DeltaSeconds);
		}
	}
	if (Light)
	{
		Light->SetIntensity(LightStart * FMath::Square(1.0f - T));
	}
}

void AZMBFx::Explosion(UWorld* World, const FVector& Origin, float RadiusCm)
{
	UZMBGameData* Data = UZMBGameData::Get(World);
	if (!Data)
	{
		return;
	}
	// bola de fogo: cresce rápido até ~40% do raio e encolhe
	if (AZMBFx* Fire = SpawnFx(World, Origin + FVector(0, 0, 60), FRotator::ZeroRotator, 0.7f))
	{
		const float Size = RadiusCm * 0.8f / 50.0f;
		Fire->SetShape(static_cast<int32>(EZMBShape::Sphere), Data->GlowMaterial(FireColor, 40.0f), FVector(0.4f), FVector(Size));
		Fire->GrowFraction = 0.3f;
		Fire->bShrinkAtEnd = true;
		Fire->AddFlash(FireColor, 120000.0f, RadiusCm * 3.0f);
	}
	// fumaça subindo
	for (int32 i = 0; i < 6; ++i)
	{
		const FVector Offset = FMath::VRand() * FVector(RadiusCm * 0.25f, RadiusCm * 0.25f, 40.0f);
		if (AZMBFx* Smoke = SpawnFx(World, Origin + Offset + FVector(0, 0, 80), FRotator::ZeroRotator, FMath::FRandRange(3.0f, 5.5f)))
		{
			const float Size = RadiusCm / 50.0f * FMath::FRandRange(0.35f, 0.6f);
			Smoke->SetShape(static_cast<int32>(EZMBShape::Sphere), Data->ColorMaterial(SmokeColor, 1.0f), FVector(Size * 0.3f), FVector(Size));
			Smoke->GrowFraction = 0.6f;
			Smoke->bShrinkAtEnd = true;
			Smoke->Drift = FVector(FMath::FRandRange(-40.0f, 40.0f), FMath::FRandRange(-40.0f, 40.0f), FMath::FRandRange(150.0f, 320.0f));
		}
	}
	Debris(World, Origin + FVector(0, 0, 40), 8, FLinearColor(0.05f, 0.045f, 0.04f), 1400.0f);
	Splat(World, Origin, RadiusCm * 0.45f, FLinearColor(0.012f, 0.011f, 0.01f));
}

void AZMBFx::MuzzleFlash(UWorld* World, const FVector& Location, const FRotator& Direction)
{
	UZMBGameData* Data = UZMBGameData::Get(World);
	if (!Data)
	{
		return;
	}
	if (AZMBFx* Flash = SpawnFx(World, Location, Direction, 0.06f))
	{
		Flash->SetShape(static_cast<int32>(EZMBShape::Cone), Data->GlowMaterial(FLinearColor(1.0f, 0.7f, 0.3f), 30.0f),
			FVector(0.12f, 0.12f, 0.25f), FVector(0.05f, 0.05f, 0.1f));
		Flash->SetActorRotation(Direction + FRotator(-90.0f, 0.0f, 0.0f));
		Flash->AddFlash(FLinearColor(1.0f, 0.75f, 0.4f), 6000.0f, 600.0f);
	}
}

void AZMBFx::Tracer(UWorld* World, const FVector& Start, const FVector& End)
{
	UZMBGameData* Data = UZMBGameData::Get(World);
	const FVector Delta = End - Start;
	const float Length = Delta.Size();
	if (!Data || Length < 1.0f)
	{
		return;
	}
	if (AZMBFx* Line = SpawnFx(World, Start + Delta * 0.5f, Delta.Rotation(), 0.05f))
	{
		const FVector Scale(Length / 100.0f, 0.012f, 0.012f);
		Line->SetShape(static_cast<int32>(EZMBShape::Cube), Data->GlowMaterial(FLinearColor(1.0f, 0.85f, 0.5f), 20.0f), Scale, Scale);
	}
}

void AZMBFx::Blood(UWorld* World, const FVector& Location, const FVector& Direction, int32 Chunks)
{
	UZMBGameData* Data = UZMBGameData::Get(World);
	if (!Data)
	{
		return;
	}
	UMaterialInterface* Material = Data->ColorMaterial(BloodColor, 0.25f);
	for (int32 i = 0; i < Chunks && CanSpawnPiece(World); ++i)
	{
		if (AZMBFx* Chunk = SpawnFx(World, Location + FMath::VRand() * 8.0f, FRotator::ZeroRotator, FMath::FRandRange(4.0f, 7.0f)))
		{
			const float Size = FMath::FRandRange(0.04f, 0.09f);
			Chunk->SetShape(static_cast<int32>(EZMBShape::Cube), Material, FVector(Size), FVector(Size));
			const FVector Velocity = (Direction.GetSafeNormal() + FMath::VRand() * 0.8f + FVector(0, 0, 0.6f)) * FMath::FRandRange(300.0f, 900.0f);
			Chunk->MakePhysicsPiece(Velocity);
		}
	}
	// nem todo tiro deixa mancha: um tiro de escopeta acerta até 9 vezes
	if (FMath::FRand() < 0.3f)
	{
		Splat(World, Location, FMath::FRandRange(40.0f, 90.0f), BloodColor);
	}
}

void AZMBFx::Splat(UWorld* World, const FVector& Location, float RadiusCm, const FLinearColor& Color)
{
	UZMBGameData* Data = UZMBGameData::Get(World);
	if (!Data)
	{
		return;
	}
	// mancha no chão: procura o piso logo abaixo
	FHitResult Hit;
	FCollisionQueryParams Params(SCENE_QUERY_STAT(ZMBSplat), false);
	FVector Ground = Location;
	if (World->LineTraceSingleByObjectType(Hit, Location + FVector(0, 0, 50), Location - FVector(0, 0, 400),
		FCollisionObjectQueryParams(ECC_WorldStatic), Params))
	{
		Ground = Hit.ImpactPoint;
	}
	if (AZMBFx* Mark = SpawnFx(World, Ground + FVector(0, 0, 1.5f), FRotator(0.0f, FMath::FRandRange(0.0f, 360.0f), 0.0f), 25.0f))
	{
		const FVector Scale(RadiusCm / 50.0f, RadiusCm / 50.0f * FMath::FRandRange(0.6f, 1.0f), 0.01f);
		Mark->SetShape(static_cast<int32>(EZMBShape::Cylinder), Data->ColorMaterial(Color, 0.35f), Scale, Scale);
		// mancha parada: sem Tick (podem ser centenas numa horda)
		Mark->SetActorTickEnabled(false);
	}
}

void AZMBFx::Debris(UWorld* World, const FVector& Origin, int32 Count, const FLinearColor& Color, float Speed)
{
	UZMBGameData* Data = UZMBGameData::Get(World);
	if (!Data)
	{
		return;
	}
	UMaterialInterface* Material = Data->ColorMaterial(Color, 0.9f);
	for (int32 i = 0; i < Count && CanSpawnPiece(World); ++i)
	{
		if (AZMBFx* Piece = SpawnFx(World, Origin + FMath::VRand() * 20.0f, FRotator::ZeroRotator, FMath::FRandRange(4.0f, 8.0f)))
		{
			const FVector Scale = FVector(FMath::FRandRange(0.06f, 0.18f), FMath::FRandRange(0.06f, 0.18f), FMath::FRandRange(0.03f, 0.1f));
			Piece->SetShape(static_cast<int32>(EZMBShape::Cube), Material, Scale, Scale);
			FVector Direction = FMath::VRand();
			Direction.Z = FMath::Abs(Direction.Z) + 0.4f;
			Piece->MakePhysicsPiece(Direction.GetSafeNormal() * Speed * FMath::FRandRange(0.4f, 1.0f));
		}
	}
}

void AZMBFx::ImpactDust(UWorld* World, const FVector& Location, const FVector& Normal)
{
	UZMBGameData* Data = UZMBGameData::Get(World);
	if (!Data)
	{
		return;
	}
	if (AZMBFx* Puff = SpawnFx(World, Location + Normal * 5.0f, FRotator::ZeroRotator, 0.35f))
	{
		Puff->SetShape(static_cast<int32>(EZMBShape::Sphere), Data->ColorMaterial(FLinearColor(0.25f, 0.23f, 0.2f), 1.0f),
			FVector(0.04f), FVector(0.22f));
		Puff->GrowFraction = 0.5f;
		Puff->bShrinkAtEnd = true;
		Puff->Drift = Normal * 60.0f;
	}
}
