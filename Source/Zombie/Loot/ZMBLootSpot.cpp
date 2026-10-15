#include "Loot/ZMBLootSpot.h"
#include "Zombie.h"
#include "Game/ZMBGameData.h"
#include "Game/ZMBGameMode.h"
#include "Game/ZMBWorldBuilder.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "HAL/PlatformTime.h"

AZMBLootSpot::AZMBLootSpot()
{
	// só pensa enquanto espera voltar a render (renova_dias), a cada 5 s
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = false;
	PrimaryActorTick.TickInterval = 5.0f;

	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(Root);

	Body = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Body"));
	Body->SetupAttachment(Root);
	Body->SetCollisionProfileName(TEXT("BlockAllDynamic"));
	Body->SetCollisionResponseToChannel(ECC_Camera, ECR_Ignore);
	Body->SetCanEverAffectNavigation(false);
}

void AZMBLootSpot::Setup(const ZR::FLootSpotType& InType)
{
	Type = InType;
	State = ZR::FLootSpotState();
	Body->SetStaticMesh(UZMBGameData::Shape(EZMBShape::Cube));
	// largura ao longo de +X (para onde ele olha), profundidade de lado: o mesmo que a planta usou para caber no chão
	Body->SetRelativeScale3D(FVector(Type.WidthCm, Type.DepthCm, Type.HeightCm) / 100.0f);
	Body->SetRelativeLocation(FVector(0.0f, 0.0f, Type.HeightCm * 0.5f));
	if (Type.HeightCm < 60.0f)
	{
		// mochila e isopor: o jogador, os zumbis e o carro atravessam (ninguém tropeça nem bate numa caixa de 45 cm);
		// o E e a mira continuam achando
		Body->SetCollisionResponseToChannel(ECC_Pawn, ECR_Ignore);
		Body->SetCollisionResponseToChannel(ECC_ZMBZombie, ECR_Ignore);
		Body->SetCollisionResponseToChannel(ECC_Vehicle, ECR_Ignore);
	}
	Paint(false);
}

bool AZMBLootSpot::CanSearch(int32 Day) const
{
	return ZR::CanSearch(Type, State, Day);
}

ZR::FLootResult AZMBLootSpot::Search(int32 Day)
{
	if (!CanSearch(Day))
	{
		return ZR::FLootResult();
	}
	const UZMBGameData* Data = UZMBGameData::Get(this);
	ZR::FRng Rng(FPlatformTime::Cycles64() ^ static_cast<std::uint64_t>(GetUniqueID()) * 0x9E3779B97F4A7C15ull);
	const ZR::FLootResult Result = Data ? ZR::RollLoot(Data->LootItems(), Type, Rng) : ZR::FLootResult();
	State.SearchedDay = Day;
	Paint(true);
	if (Type.RefillDays > 0)
	{
		SetActorTickEnabled(true);
	}
	return Result;
}

void AZMBLootSpot::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	// voltou a render: acende de novo, para o olho achar
	if (CanSearch(CurrentDay()))
	{
		Paint(false);
		SetActorTickEnabled(false);
	}
}

int32 AZMBLootSpot::CurrentDay() const
{
	const AZMBGameMode* GameMode = AZMBGameMode::Get(this);
	const AZMBWorldBuilder* Builder = GameMode ? GameMode->GetBuilder() : nullptr;
	return Builder ? Builder->GetDayIndex() : 0;
}

float AZMBLootSpot::DistanceTo(const FVector& Where) const
{
	const FVector Local = GetActorTransform().InverseTransformPosition(Where);
	const float Dx = FMath::Max(FMath::Abs(static_cast<float>(Local.X)) - Type.WidthCm * 0.5f, 0.0f);
	const float Dy = FMath::Max(FMath::Abs(static_cast<float>(Local.Y)) - Type.DepthCm * 0.5f, 0.0f);
	return FMath::Sqrt(Dx * Dx + Dy * Dy);
}

FString AZMBLootSpot::GetDisplayName() const
{
	return UZMBGameData::ToFString(UZMBGameData::IsPortuguese() || Type.NameEn.empty() ? Type.NamePt : Type.NameEn);
}

void AZMBLootSpot::Paint(bool bEmpty)
{
	UZMBGameData* Data = UZMBGameData::Get(this);
	if (!Data)
	{
		return;
	}
	// revirado: apagado, para o olho achar de longe o que ainda rende
	const FLinearColor Color = UZMBGameData::ToLinear(Type.ColorRgb) * (bEmpty ? 0.3f : 1.0f);
	Body->SetMaterial(0, Data->ColorMaterial(FLinearColor(Color.R, Color.G, Color.B, 1.0f), 0.6f, Type.Where == ZR::ELootWhere::Road ? 0.3f : 0.0f));
}
