#include "Game/ZMBWorldBuilder.h"
#include "Zombie.h"
#include "Combat/ZMBExplosive.h"
#include "Loot/ZMBLootSpot.h"
#include "Game/ZMBGameData.h"
#include "Vehicles/ZMBCar.h"
#include "Components/DirectionalLightComponent.h"
#include "Components/ExponentialHeightFogComponent.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/PostProcessComponent.h"
#include "Components/SkyAtmosphereComponent.h"
#include "Components/SkyLightComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/CollisionProfile.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Math/RandomStream.h"

namespace
{
	constexpr float SidewalkHeight = 15.0f;
	constexpr float FloorHeight = 320.0f;

	const FLinearColor FacadeColors[] = {
		FLinearColor(0.30f, 0.29f, 0.27f),  // concreto
		FLinearColor(0.50f, 0.42f, 0.30f),  // bege
		FLinearColor(0.45f, 0.18f, 0.10f),  // terracota
		FLinearColor(0.62f, 0.52f, 0.22f),  // amarelo pastel
		FLinearColor(0.25f, 0.38f, 0.48f),  // azul claro
		FLinearColor(0.62f, 0.60f, 0.56f),  // branco encardido
		FLinearColor(0.58f, 0.30f, 0.32f),  // rosa colonial
	};

	const FLinearColor CarColors[] = {
		FLinearColor(0.6f, 0.6f, 0.62f), FLinearColor(0.02f, 0.02f, 0.025f), FLinearColor(0.5f, 0.04f, 0.03f),
		FLinearColor(0.05f, 0.12f, 0.35f), FLinearColor(0.8f, 0.78f, 0.75f), FLinearColor(0.15f, 0.25f, 0.12f),
	};
}

AZMBWorldBuilder::AZMBWorldBuilder()
{
	PrimaryActorTick.bCanEverTick = true;

	USceneComponent* Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	Root->SetMobility(EComponentMobility::Static);
	SetRootComponent(Root);

	Sun = CreateDefaultSubobject<UDirectionalLightComponent>(TEXT("Sun"));
	Sun->SetupAttachment(Root);
	Sun->SetMobility(EComponentMobility::Movable);
	Sun->SetIntensity(10.0f);
	Sun->SetLightColor(FLinearColor(1.0f, 0.95f, 0.88f));
	Sun->SetAtmosphereSunLight(true);
	Sun->SetAtmosphereSunLightIndex(0);
	Sun->ForwardShadingPriority = 1;

	Moon = CreateDefaultSubobject<UDirectionalLightComponent>(TEXT("Moon"));
	Moon->SetupAttachment(Root);
	Moon->SetMobility(EComponentMobility::Movable);
	Moon->SetIntensity(0.0f);
	Moon->SetLightColor(FLinearColor(0.55f, 0.65f, 1.0f));
	Moon->SetAtmosphereSunLight(true);
	Moon->SetAtmosphereSunLightIndex(1);
	Moon->SetCastShadows(false);

	Atmosphere = CreateDefaultSubobject<USkyAtmosphereComponent>(TEXT("Atmosphere"));
	Atmosphere->SetupAttachment(Root);

	SkyLight = CreateDefaultSubobject<USkyLightComponent>(TEXT("SkyLight"));
	SkyLight->SetupAttachment(Root);
	SkyLight->SetMobility(EComponentMobility::Movable);
	SkyLight->bRealTimeCapture = true;
	SkyLight->SetIntensity(1.0f);

	Fog = CreateDefaultSubobject<UExponentialHeightFogComponent>(TEXT("Fog"));
	Fog->SetupAttachment(Root);
	Fog->SetFogDensity(0.012f);
	Fog->SetFogHeightFalloff(0.12f);
	Fog->SetVolumetricFog(true);

	// "cinema do fim do mundo": pouco mais de contraste, sombras puxando para o azul, altas para o laranja, grão e vinheta
	PostProcess = CreateDefaultSubobject<UPostProcessComponent>(TEXT("PostProcess"));
	PostProcess->SetupAttachment(Root);
	PostProcess->bUnbound = true;
	FPostProcessSettings& Post = PostProcess->Settings;
	Post.bOverride_AutoExposureMinBrightness = true;
	Post.AutoExposureMinBrightness = -1.5f;
	Post.bOverride_AutoExposureMaxBrightness = true;
	Post.AutoExposureMaxBrightness = 15.0f;
	Post.bOverride_BloomIntensity = true;
	Post.BloomIntensity = 0.55f;
	Post.bOverride_VignetteIntensity = true;
	Post.VignetteIntensity = 0.35f;
	Post.bOverride_FilmGrainIntensity = true;
	Post.FilmGrainIntensity = 0.12f;
	Post.bOverride_SceneFringeIntensity = true;
	Post.SceneFringeIntensity = 0.2f;
	Post.bOverride_ColorSaturation = true;
	Post.ColorSaturation = FVector4(0.9, 0.92, 0.95, 1.0);
	Post.bOverride_ColorContrast = true;
	Post.ColorContrast = FVector4(1.08, 1.08, 1.08, 1.0);
	Post.bOverride_ColorGainShadows = true;
	Post.ColorGainShadows = FVector4(0.96, 1.0, 1.05, 1.0);
	Post.bOverride_ColorGainHighlights = true;
	Post.ColorGainHighlights = FVector4(1.04, 1.0, 0.95, 1.0);
}

FVector AZMBWorldBuilder::CellCenter(int32 X, int32 Y) const
{
	return Origin + FVector((X + 0.5f) * CellSize, (Y + 0.5f) * CellSize, 0.0f);
}

bool AZMBWorldBuilder::WorldToCell(const FVector& Location, int32& OutX, int32& OutY) const
{
	if (CellSize <= 0.0f)
	{
		return false;
	}
	OutX = FMath::FloorToInt32((Location.X - Origin.X) / CellSize);
	OutY = FMath::FloorToInt32((Location.Y - Origin.Y) / CellSize);
	return Plan.InBounds(OutX, OutY);
}

UInstancedStaticMeshComponent* AZMBWorldBuilder::NewInstances(const TCHAR* Name, UStaticMesh* Mesh, UMaterialInterface* Material, bool bCollision)
{
	UInstancedStaticMeshComponent* Instances = NewObject<UInstancedStaticMeshComponent>(this, Name);
	Instances->SetupAttachment(GetRootComponent());
	Instances->SetMobility(EComponentMobility::Static);
	Instances->SetStaticMesh(Mesh);
	Instances->SetMaterial(0, Material);
	if (bCollision)
	{
		Instances->SetCollisionProfileName(UCollisionProfile::BlockAll_ProfileName);
	}
	else
	{
		Instances->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Instances->SetCastShadow(false);
	}
	Instances->SetCanEverAffectNavigation(false);
	Instances->RegisterComponent();
	AddInstanceComponent(Instances);
	return Instances;
}

void AZMBWorldBuilder::Build(std::uint64_t Seed)
{
	UZMBGameData* Data = UZMBGameData::Get(this);
	if (!Data)
	{
		UE_LOG(LogZombie, Error, TEXT("AZMBWorldBuilder: sem UZMBGameData; nada a montar"));
		return;
	}
	const ZR::FTuning& Tuning = Data->Tuning();
	ZR::FCityConfig City = Tuning.City;
	City.Seed = Seed;
	CitySeed = Seed;
	Plan = ZR::GenerateCity(City);
	Day = Tuning.Day;
	CellSize = City.CellSizeCm;
	Origin = FVector(-Plan.Width * CellSize * 0.5f, -Plan.Height * CellSize * 0.5f, 0.0f);

	// começo: na avenida da orla (última linha de terra), numa rua do meio, olhando para a cidade (norte, -Y)
	const int32 Period = FMath::Max(1, City.BlockCells) + 1;
	const int32 StartX = FMath::Clamp(FMath::RoundToInt(Plan.Width * 0.5f / Period) * Period, 0, Plan.Width - 1);
	const int32 StartY = Plan.LandHeight - 1;
	StartLocation = CellCenter(StartX, StartY) + FVector(0.0f, 0.0f, 120.0f);
	StartRotation = FRotator(0.0f, -90.0f, 0.0f);

	BuildGround();
	BuildBlocks();
	BuildStreetFurniture();
	SpawnProps(Seed);
	SpawnLoot(Seed);
	Clock = 0.0;
	UpdateSky();
	UE_LOG(LogZombie, Display, TEXT("Porto Brasa: %d x %d células de %.0f m, %d prédios, %d praças, semente %llu"), Plan.Width, Plan.Height,
		CellSize / 100.0f, Plan.CountOf(ZR::ECell::Building), Plan.CountOf(ZR::ECell::Plaza), static_cast<unsigned long long>(Seed));
}

void AZMBWorldBuilder::BuildGround()
{
	UZMBGameData* Data = UZMBGameData::Get(this);
	UStaticMesh* Cube = UZMBGameData::Shape(EZMBShape::Cube);
	const float Width = Plan.Width * CellSize;
	const float LandDepth = Plan.LandHeight * CellSize;
	const int32 BeachRows = Data->Tuning().City.BeachRows;
	const float BeachDepth = BeachRows * CellSize;
	const float SeaStartY = Origin.Y + LandDepth + BeachDepth;

	auto Block = [this, Cube](const TCHAR* Name, const FVector& Center, const FVector& SizeCm, UMaterialInterface* Material, bool bCollision, bool bHiddenInGame)
	{
		UStaticMeshComponent* Part = NewObject<UStaticMeshComponent>(this, Name);
		Part->SetupAttachment(GetRootComponent());
		Part->SetMobility(EComponentMobility::Static);
		Part->SetStaticMesh(Cube);
		Part->SetMaterial(0, Material);
		Part->SetRelativeLocation(Center);
		Part->SetRelativeScale3D(SizeCm / 100.0f);
		if (bCollision)
		{
			Part->SetCollisionProfileName(UCollisionProfile::BlockAll_ProfileName);
		}
		else
		{
			Part->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		}
		Part->SetHiddenInGame(bHiddenInGame);
		Part->SetCanEverAffectNavigation(false);
		Part->RegisterComponent();
		AddInstanceComponent(Part);
		return Part;
	};

	const float CenterX = Origin.X + Width * 0.5f;
	// asfalto (a terra toda; calçadas e prédios ficam por cima)
	Block(TEXT("Asphalt"), FVector(CenterX, Origin.Y + LandDepth * 0.5f, -50.0f), FVector(Width, LandDepth, 100.0f),
		Data->ColorMaterial(FLinearColor(0.035f, 0.035f, 0.038f), 0.85f), true, false);
	// arredores (vistos da câmera alta): terra escura além da cidade
	Block(TEXT("Outskirts"), FVector(CenterX, Origin.Y + LandDepth * 0.5f, -55.0f), FVector(Width * 3.0f, LandDepth * 3.0f, 100.0f),
		Data->ColorMaterial(FLinearColor(0.07f, 0.065f, 0.05f), 0.95f), false, false);
	if (BeachRows > 0)
	{
		Block(TEXT("Beach"), FVector(CenterX, Origin.Y + LandDepth + BeachDepth * 0.5f, -48.0f), FVector(Width * 3.0f, BeachDepth, 100.0f),
			Data->ColorMaterial(FLinearColor(0.55f, 0.47f, 0.33f), 0.95f), true, false);
	}
	// mar até o horizonte: fundo de areia escura e a lâmina d'água lisa (reflexo do Lumen)
	Block(TEXT("SeaFloor"), FVector(CenterX, SeaStartY + 40000.0f, -400.0f), FVector(Width * 6.0f, 80000.0f, 100.0f),
		Data->ColorMaterial(FLinearColor(0.12f, 0.1f, 0.07f), 0.95f), false, false);
	Block(TEXT("Water"), FVector(CenterX, SeaStartY + 40000.0f, -25.0f), FVector(Width * 6.0f, 80000.0f, 10.0f),
		Data->ColorMaterial(FLinearColor(0.008f, 0.035f, 0.05f), 0.04f), false, false);

	// paredes invisíveis: o jogador não sai da cidade nem entra no mar
	const float WallHeight = 3000.0f;
	Block(TEXT("WallSea"), FVector(CenterX, SeaStartY + 50.0f, WallHeight * 0.5f), FVector(Width * 3.0f, 100.0f, WallHeight), nullptr, true, true);
	Block(TEXT("WallNorth"), FVector(CenterX, Origin.Y - 50.0f, WallHeight * 0.5f), FVector(Width, 100.0f, WallHeight), nullptr, true, true);
	const float SideDepth = LandDepth + BeachDepth;
	Block(TEXT("WallWest"), FVector(Origin.X - 50.0f, Origin.Y + SideDepth * 0.5f, WallHeight * 0.5f), FVector(100.0f, SideDepth, WallHeight), nullptr, true, true);
	Block(TEXT("WallEast"), FVector(Origin.X + Width + 50.0f, Origin.Y + SideDepth * 0.5f, WallHeight * 0.5f), FVector(100.0f, SideDepth, WallHeight), nullptr, true, true);
}

void AZMBWorldBuilder::BuildBlocks()
{
	UZMBGameData* Data = UZMBGameData::Get(this);
	UStaticMesh* Cube = UZMBGameData::Shape(EZMBShape::Cube);
	UStaticMesh* Cylinder = UZMBGameData::Shape(EZMBShape::Cylinder);
	UStaticMesh* Sphere = UZMBGameData::Shape(EZMBShape::Sphere);
	FRandomStream Rng(static_cast<int32>(CitySeed & 0x7FFFFFFF) ^ 0x5EED);

	UInstancedStaticMeshComponent* Sidewalk = NewInstances(TEXT("Sidewalks"), Cube, Data->ColorMaterial(FLinearColor(0.30f, 0.29f, 0.27f), 0.9f), true);
	UInstancedStaticMeshComponent* Grass = NewInstances(TEXT("Grass"), Cube, Data->ColorMaterial(FLinearColor(0.05f, 0.11f, 0.03f), 0.95f), false);
	UInstancedStaticMeshComponent* Trunks = NewInstances(TEXT("Trunks"), Cylinder, Data->ColorMaterial(FLinearColor(0.08f, 0.05f, 0.03f), 0.9f), true);
	UInstancedStaticMeshComponent* Canopies = NewInstances(TEXT("Canopies"), Sphere, Data->ColorMaterial(FLinearColor(0.03f, 0.08f, 0.02f), 0.9f), false);
	Canopies->SetCastShadow(true);
	UInstancedStaticMeshComponent* Glass = NewInstances(TEXT("Windows"), Cube, Data->ColorMaterial(FLinearColor(0.02f, 0.025f, 0.03f), 0.06f, 0.2f), false);
	LitWindows = Data->GlowMaterial(FLinearColor(1.0f, 0.75f, 0.42f), 1.0f);
	UInstancedStaticMeshComponent* Lit = NewInstances(TEXT("LitWindows"), Cube, LitWindows, false);
	UInstancedStaticMeshComponent* Tanks = NewInstances(TEXT("WaterTanks"), Cylinder, Data->ColorMaterial(FLinearColor(0.03f, 0.14f, 0.38f), 0.5f), true);
	UInstancedStaticMeshComponent* Units = NewInstances(TEXT("RoofUnits"), Cube, Data->ColorMaterial(FLinearColor(0.35f, 0.35f, 0.34f), 0.6f, 0.5f), true);
	UInstancedStaticMeshComponent* Lines = NewInstances(TEXT("LaneLines"), Cube, Data->ColorMaterial(FLinearColor(0.7f, 0.6f, 0.2f), 0.6f), false);

	TArray<UInstancedStaticMeshComponent*> Facades;
	for (int32 i = 0; i < static_cast<int32>(UE_ARRAY_COUNT(FacadeColors)); ++i)
	{
		Facades.Add(NewInstances(*FString::Printf(TEXT("Facade%d"), i), Cube, Data->ColorMaterial(FacadeColors[i], 0.85f), true));
	}

	TArray<FTransform> SidewalkT, GrassT, TrunkT, CanopyT, GlassT, LitT, TankT, UnitT, LineT;
	TArray<TArray<FTransform>> FacadeT;
	FacadeT.SetNum(Facades.Num());

	for (int32 Y = 0; Y < Plan.Height; ++Y)
	{
		for (int32 X = 0; X < Plan.Width; ++X)
		{
			const ZR::ECell Cell = Plan.At(X, Y);
			const FVector Center = CellCenter(X, Y);
			if (Cell == ZR::ECell::Road)
			{
				// faixa amarela tracejada no meio da rua reta
				const bool bEastWest = Plan.At(X - 1, Y) == ZR::ECell::Road && Plan.At(X + 1, Y) == ZR::ECell::Road
					&& Plan.At(X, Y - 1) != ZR::ECell::Road && Plan.At(X, Y + 1) != ZR::ECell::Road;
				const bool bNorthSouth = Plan.At(X, Y - 1) == ZR::ECell::Road && Plan.At(X, Y + 1) == ZR::ECell::Road
					&& Plan.At(X - 1, Y) != ZR::ECell::Road && Plan.At(X + 1, Y) != ZR::ECell::Road;
				for (int32 Dash = 0; Dash < 3 && (bEastWest || bNorthSouth); ++Dash)
				{
					const float Along = (Dash - 1) * CellSize / 3.0f;
					const FVector Where = Center + (bEastWest ? FVector(Along, 0.0f, 0.6f) : FVector(0.0f, Along, 0.6f));
					const FVector Size = bEastWest ? FVector(2.6f, 0.14f, 0.01f) : FVector(0.14f, 2.6f, 0.01f);
					LineT.Add(FTransform(FRotator::ZeroRotator, Where, Size));
				}
				continue;
			}
			if (Cell != ZR::ECell::Building && Cell != ZR::ECell::Plaza)
			{
				continue;
			}
			SidewalkT.Add(FTransform(FRotator::ZeroRotator, Center + FVector(0.0f, 0.0f, SidewalkHeight * 0.5f),
				FVector(CellSize / 100.0f, CellSize / 100.0f, SidewalkHeight / 100.0f)));

			if (Cell == ZR::ECell::Plaza)
			{
				const float Lawn = CellSize - 400.0f;
				GrassT.Add(FTransform(FRotator::ZeroRotator, Center + FVector(0.0f, 0.0f, SidewalkHeight + 2.0f), FVector(Lawn / 100.0f, Lawn / 100.0f, 0.04f)));
				const int32 Trees = Rng.RandRange(2, 5);
				for (int32 t = 0; t < Trees; ++t)
				{
					const FVector Spot = Center + FVector(Rng.FRandRange(-0.35f, 0.35f) * CellSize, Rng.FRandRange(-0.35f, 0.35f) * CellSize, SidewalkHeight);
					const float Height = Rng.FRandRange(450.0f, 750.0f);
					TrunkT.Add(FTransform(FRotator::ZeroRotator, Spot + FVector(0.0f, 0.0f, Height * 0.5f), FVector(0.35f, 0.35f, Height / 100.0f)));
					const float Crown = Rng.FRandRange(4.0f, 6.5f);
					CanopyT.Add(FTransform(FRotator(0.0f, Rng.FRandRange(0.0f, 360.0f), 0.0f), Spot + FVector(0.0f, 0.0f, Height + Crown * 25.0f),
						FVector(Crown, Crown * Rng.FRandRange(0.8f, 1.0f), Crown * 0.75f)));
				}
				continue;
			}

			// prédio: recuo da calçada vindo da planta (as regras usam o mesmo: dá para andar e construir na calçada e no beco),
			// altura pelos andares da planta
			const int32 Floors = Plan.FloorsAt(X, Y);
			const float Inset = Plan.InsetAt(X, Y);
			const float Side = CellSize - 2.0f * Inset;
			const float Height = Floors * FloorHeight;
			const float Base = SidewalkHeight;
			const int32 Facade = Rng.RandRange(0, Facades.Num() - 1);
			FacadeT[Facade].Add(FTransform(FRotator::ZeroRotator, Center + FVector(0.0f, 0.0f, Base + Height * 0.5f),
				FVector(Side / 100.0f, Side / 100.0f, Height / 100.0f)));

			// janelas: uma faixa por andar em cada fachada; algumas acesas (à noite brilham, de dia quase não)
			const float BandWidth = Side * 0.84f;
			for (int32 Floor = 0; Floor < Floors; ++Floor)
			{
				const float BandHeight = Floor == 0 ? FloorHeight * 0.6f : FloorHeight * 0.42f;
				const float Z = Base + Floor * FloorHeight + (Floor == 0 ? FloorHeight * 0.35f : FloorHeight * 0.55f);
				for (int32 Face = 0; Face < 4; ++Face)
				{
					const bool bAlongX = Face < 2;
					const float Sign = (Face % 2 == 0) ? 1.0f : -1.0f;
					const FVector Offset = bAlongX ? FVector(0.0f, Sign * (Side * 0.5f + 3.0f), Z) : FVector(Sign * (Side * 0.5f + 3.0f), 0.0f, Z);
					const FVector Size = bAlongX ? FVector(BandWidth / 100.0f, 0.06f, BandHeight / 100.0f) : FVector(0.06f, BandWidth / 100.0f, BandHeight / 100.0f);
					const bool bLit = Floor > 0 && Rng.FRand() < 0.14f;
					(bLit ? LitT : GlassT).Add(FTransform(FRotator::ZeroRotator, FVector(Center.X, Center.Y, 0.0f) + Offset, Size));
				}
			}

			// telhado: caixa d'água azul (muito brasileiro) e aparelhos de ar
			const float Roof = Base + Height;
			if (Rng.FRand() < 0.45f)
			{
				const FVector Spot = Center + FVector(Rng.FRandRange(-0.25f, 0.25f) * Side, Rng.FRandRange(-0.25f, 0.25f) * Side, Roof + 70.0f);
				TankT.Add(FTransform(FRotator::ZeroRotator, Spot, FVector(1.6f, 1.6f, 1.4f)));
			}
			const int32 UnitCount = Rng.RandRange(0, 3);
			for (int32 u = 0; u < UnitCount; ++u)
			{
				const FVector Spot = Center + FVector(Rng.FRandRange(-0.35f, 0.35f) * Side, Rng.FRandRange(-0.35f, 0.35f) * Side, Roof + 40.0f);
				UnitT.Add(FTransform(FRotator(0.0f, Rng.FRandRange(0.0f, 90.0f), 0.0f), Spot, FVector(1.2f, 0.9f, 0.8f)));
			}
		}
	}

	Sidewalk->AddInstances(SidewalkT, false);
	Grass->AddInstances(GrassT, false);
	Trunks->AddInstances(TrunkT, false);
	Canopies->AddInstances(CanopyT, false);
	Glass->AddInstances(GlassT, false);
	Lit->AddInstances(LitT, false);
	Tanks->AddInstances(TankT, false);
	Units->AddInstances(UnitT, false);
	Lines->AddInstances(LineT, false);
	for (int32 i = 0; i < Facades.Num(); ++i)
	{
		Facades[i]->AddInstances(FacadeT[i], false);
	}
	UE_LOG(LogZombie, Display, TEXT("Porto Brasa: %d janelas (%d acesas), %d árvores"), GlassT.Num() + LitT.Num(), LitT.Num(), TrunkT.Num());
}

void AZMBWorldBuilder::BuildStreetFurniture()
{
	UZMBGameData* Data = UZMBGameData::Get(this);
	UStaticMesh* Cube = UZMBGameData::Shape(EZMBShape::Cube);
	UStaticMesh* Cylinder = UZMBGameData::Shape(EZMBShape::Cylinder);
	UInstancedStaticMeshComponent* Poles = NewInstances(TEXT("LampPoles"), Cylinder, Data->ColorMaterial(FLinearColor(0.08f, 0.08f, 0.085f), 0.5f, 0.7f), true);
	UInstancedStaticMeshComponent* Heads = NewInstances(TEXT("LampHeads"), Cube, Data->GlowMaterial(FLinearColor(1.0f, 0.62f, 0.3f), 4.0f), false);
	const int32 Period = FMath::Max(1, Data->Tuning().City.BlockCells) + 1;
	TArray<FTransform> PoleT, HeadT;
	int32 LightCount = 0;
	constexpr int32 MaxLights = 64;
	for (int32 Y = 0; Y < Plan.LandHeight; Y += Period)
	{
		for (int32 X = 0; X < Plan.Width; X += Period)
		{
			// poste na quina da calçada do quarteirão a sudeste do cruzamento
			const FVector Corner = CellCenter(X, Y) + FVector(CellSize * 0.5f + 150.0f, CellSize * 0.5f + 150.0f, 0.0f);
			if (!Plan.InBounds(X + 1, Y + 1) || Plan.At(X + 1, Y + 1) == ZR::ECell::Road)
			{
				continue;
			}
			const float Height = 850.0f;
			PoleT.Add(FTransform(FRotator::ZeroRotator, Corner + FVector(0.0f, 0.0f, SidewalkHeight + Height * 0.5f), FVector(0.18f, 0.18f, Height / 100.0f)));
			const FVector Lamp = Corner + FVector(-120.0f, -120.0f, SidewalkHeight + Height);
			HeadT.Add(FTransform(FRotator(0.0f, 45.0f, 0.0f), Lamp, FVector(1.0f, 0.35f, 0.15f)));
			if (LightCount < MaxLights)
			{
				UPointLightComponent* Light = NewObject<UPointLightComponent>(this, *FString::Printf(TEXT("StreetLight%d"), LightCount));
				Light->SetupAttachment(GetRootComponent());
				Light->SetMobility(EComponentMobility::Movable);
				Light->SetRelativeLocation(Lamp - FVector(0.0f, 0.0f, 30.0f));
				Light->SetIntensityUnits(ELightUnits::Candelas);
				Light->SetIntensity(1500.0f);
				Light->SetAttenuationRadius(2600.0f);
				Light->SetSourceRadius(20.0f);
				Light->SetLightColor(FLinearColor(1.0f, 0.62f, 0.32f));
				Light->SetCastShadows(false);
				Light->SetVisibility(false);
				Light->RegisterComponent();
				AddInstanceComponent(Light);
				StreetLights.Add(Light);
				++LightCount;
			}
		}
	}
	Poles->AddInstances(PoleT, false);
	Heads->AddInstances(HeadT, false);
}

void AZMBWorldBuilder::SpawnProps(std::uint64_t Seed)
{
	UZMBGameData* Data = UZMBGameData::Get(this);
	UWorld* World = GetWorld();
	const ZR::FTuning& Tuning = Data->Tuning();
	const ZR::FExplosiveType* Bomb = Data->FindExplosive("bujao");
	const ZR::FExplosiveType* Barrel = Data->FindExplosive("barril");
	const ZR::FExplosiveType* Wreck = Data->FindExplosive("carro");
	FRandomStream Rng(static_cast<int32>(Seed & 0x7FFFFFFF) ^ 0xB0B0);
	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

	auto SpawnExplosive = [World, &Params](const ZR::FExplosiveType* Type, const FVector& Where, const FRotator& Rotation) -> AZMBExplosive*
	{
		if (!Type)
		{
			return nullptr;
		}
		AZMBExplosive* Explosive = World->SpawnActor<AZMBExplosive>(AZMBExplosive::StaticClass(), Where, Rotation, Params);
		if (Explosive)
		{
			Explosive->Setup(*Type);
		}
		return Explosive;
	};

	int32 StartX = 0;
	int32 StartY = 0;
	WorldToCell(StartLocation, StartX, StartY);

	// o primeiro minuto já tem a sua cena de clipe: um caminhão de gás descarregado no meio da rua, 25 m à frente
	const FVector Depot = StartLocation + StartRotation.Vector() * 2500.0f;
	for (int32 i = 0; i < 10; ++i)
	{
		const FVector Where(Depot.X + (i % 5 - 2) * 70.0f, Depot.Y + (i / 5) * 70.0f, 40.0f);
		SpawnExplosive(Bomb, Where, FRotator::ZeroRotator);
	}

	const std::vector<ZR::FPropSpot> Spots = ZR::PickRoadSpots(Plan, Tuning.StreetExplosives, Seed + 1, StartX, StartY, 1);
	for (const ZR::FPropSpot& Spot : Spots)
	{
		const FVector Where = CellCenter(Spot.X, Spot.Y) + FVector(Spot.OffsetX * CellSize, Spot.OffsetY * CellSize, 60.0f);
		SpawnExplosive(Rng.FRand() < 0.6f ? Bomb : Barrel, Where, FRotator(0.0f, Rng.FRandRange(0.0f, 360.0f), 0.0f));
	}

	const std::vector<ZR::FPropSpot> CarSpots = ZR::PickRoadSpots(Plan, Tuning.StreetCars, Seed + 2, StartX, StartY, 1);
	for (const ZR::FPropSpot& Spot : CarSpots)
	{
		const bool bEastWest = Plan.At(Spot.X - 1, Spot.Y) == ZR::ECell::Road;
		const float Yaw = (bEastWest ? 0.0f : 90.0f) + Rng.FRandRange(-20.0f, 20.0f) + (Rng.FRand() < 0.5f ? 180.0f : 0.0f);
		const FVector Where = CellCenter(Spot.X, Spot.Y) + FVector(Spot.OffsetX * CellSize, Spot.OffsetY * CellSize, 70.0f);
		ZR::FExplosiveType Painted = Wreck ? *Wreck : ZR::FExplosiveType();
		const FColor Color = CarColors[Rng.RandRange(0, static_cast<int32>(UE_ARRAY_COUNT(CarColors)) - 1)].ToFColor(true);
		Painted.ColorRgb = (static_cast<std::uint32_t>(Color.R) << 16) | (static_cast<std::uint32_t>(Color.G) << 8) | Color.B;
		Painted.Id = "carro";
		SpawnExplosive(&Painted, Where, FRotator(0.0f, Yaw, 0.0f));
	}

	// carros de verdade para dirigir, na orla perto do começo
	const FVector Along = FRotator(0.0f, StartRotation.Yaw + 90.0f, 0.0f).Vector();
	for (int32 i = 0; i < 3; ++i)
	{
		const FVector Where = StartLocation + Along * (900.0f + i * 700.0f) * (i % 2 == 0 ? 1.0f : -1.0f) + FVector(0.0f, 0.0f, 40.0f);
		World->SpawnActor<AZMBCar>(AZMBCar::StaticClass(), Where, FRotator(0.0f, StartRotation.Yaw + 90.0f, 0.0f), Params);
	}
}

void AZMBWorldBuilder::SpawnLoot(std::uint64_t Seed)
{
	const UZMBGameData* Data = UZMBGameData::Get(this);
	UWorld* World = GetWorld();
	if (!Data || !World)
	{
		return;
	}
	const std::vector<ZR::FLootSpotType>& Types = Data->LootSpots();
	const FVector StartOnPlan = StartLocation - Origin;
	const std::vector<ZR::FLootPlace> Places = ZR::PlaceLoot(Plan, CellSize, Types, Data->Tuning().Loot, Seed + 3,
		static_cast<float>(StartOnPlan.X), static_cast<float>(StartOnPlan.Y));
	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	const FCollisionObjectQueryParams Movables(FCollisionObjectQueryParams::AllDynamicObjects);
	int32 Spawned = 0;
	int32 Skipped = 0;
	for (const ZR::FLootPlace& Place : Places)
	{
		if (Place.Type < 0 || Place.Type >= static_cast<int32>(Types.size()))
		{
			continue;
		}
		const ZR::FLootSpotType& Type = Types[static_cast<size_t>(Place.Type)];
		// na calçada fica em cima do meio-fio; na rua, no asfalto
		const float Floor = Type.Where == ZR::ELootWhere::Sidewalk ? SidewalkHeight : 0.0f;
		const FVector Where = Origin + FVector(Place.X, Place.Y, Floor);
		const FRotator Facing(0.0f, Place.Yaw, 0.0f);
		// a planta não sabe dos carros, dos bujões e do caminhão de gás: lugar que cairia embaixo de um deles fica de fora.
		// Uma coluna de 3 m: eles ainda estão no ar quando o saque nasce (bujão a 60 cm, carro a 1,6 m) e vão cair em cima
		const FVector Half(Type.WidthCm * 0.5f, Type.DepthCm * 0.5f, FMath::Max(Type.HeightCm * 0.5f, 150.0f));
		if (World->OverlapAnyTestByObjectType(Where + FVector(0.0f, 0.0f, Half.Z + 2.0f), Facing.Quaternion(), Movables, FCollisionShape::MakeBox(Half)))
		{
			++Skipped;
			continue;
		}
		if (AZMBLootSpot* Spot = World->SpawnActor<AZMBLootSpot>(AZMBLootSpot::StaticClass(), Where, Facing, Params))
		{
			Spot->Setup(Type);
			++Spawned;
		}
	}
	UE_LOG(LogZombie, Display, TEXT("Porto Brasa: %d lugares de saque (%d pulados por cair em cima de carro ou bujão)"), Spawned, Skipped);
}

float AZMBWorldBuilder::GetHour() const
{
	return static_cast<float>(ZR::HourAt(Day, Clock));
}

bool AZMBWorldBuilder::IsNightNow() const
{
	return ZR::IsNight(Day, ZR::HourAt(Day, Clock));
}

int32 AZMBWorldBuilder::GetDayIndex() const
{
	return ZR::DayIndexAt(Day, Clock);
}

void AZMBWorldBuilder::SkipHours(float Hours)
{
	Clock += static_cast<double>(Hours) / 24.0 * Day.DayLengthSeconds;
	UpdateSky();
}

void AZMBWorldBuilder::SetHour(float Hour)
{
	const double Delta = FMath::Fmod(static_cast<double>(Hour) - Day.StartHour + 24.0, 24.0);
	Clock = Delta / 24.0 * Day.DayLengthSeconds;
	UpdateSky();
}

void AZMBWorldBuilder::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (bFrozen)
	{
		return;
	}
	Clock += DeltaSeconds;
	UpdateSky();
}

void AZMBWorldBuilder::UpdateSky()
{
	const double Hour = ZR::HourAt(Day, Clock);
	const float Elevation = static_cast<float>(ZR::SunElevationDegrees(Hour, Day.MaxSunElevationDeg));
	// o sol nasce no leste (+X) e se põe no oeste; a lua anda do lado oposto
	const float Azimuth = static_cast<float>(Hour / 24.0 * 360.0) + 90.0f;
	Sun->SetWorldRotation(FRotator(-Elevation, Azimuth, 0.0f));
	Moon->SetWorldRotation(FRotator(Elevation, Azimuth + 180.0f, 0.0f));
	Sun->SetIntensity(10.0f * FMath::Clamp(Elevation / 8.0f + 0.15f, 0.0f, 1.0f));
	Moon->SetIntensity(0.3f * FMath::Clamp(-Elevation / 8.0f, 0.0f, 1.0f));

	const bool bNight = ZR::IsNight(Day, Hour);
	if (bNight != bLightsOn)
	{
		bLightsOn = bNight;
		for (UPointLightComponent* Light : StreetLights)
		{
			if (Light)
			{
				Light->SetVisibility(bNight);
			}
		}
		if (LitWindows)
		{
			LitWindows->SetScalarParameterValue(TEXT("Intensity"), bNight ? 6.0f : 0.4f);
		}
	}
}
