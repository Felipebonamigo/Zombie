#include "Game/ZMBGameData.h"
#include "Zombie.h"
#include "Engine/GameInstance.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "HAL/IConsoleManager.h"
#include "Internationalization/Culture.h"
#include "Internationalization/Internationalization.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"

namespace
{
	const TCHAR* BasicShapeMaterialPath = TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial");
	const TCHAR* SurfaceMaterialPath = TEXT("/Game/Zombie/Materials/M_ZMBSurface.M_ZMBSurface");
	const TCHAR* GlowMaterialPath = TEXT("/Game/Zombie/Materials/M_ZMBEmissive.M_ZMBEmissive");

	void ReloadDataCommand(const TArray<FString>& Args, UWorld* World)
	{
		if (UZMBGameData* Data = UZMBGameData::Get(World))
		{
			Data->Reload();
		}
	}

	FAutoConsoleCommandWithWorldAndArgs GReloadDataCommand(
		TEXT("zmb.dados.recarregar"),
		TEXT("Lê de novo Data/*.csv (vale para o que nascer depois)."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&ReloadDataCommand));
}

void UZMBGameData::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	SurfaceParent = LoadObject<UMaterialInterface>(nullptr, SurfaceMaterialPath, nullptr, LOAD_NoWarn | LOAD_Quiet);
	GlowParent = LoadObject<UMaterialInterface>(nullptr, GlowMaterialPath, nullptr, LOAD_NoWarn | LOAD_Quiet);
	if (!SurfaceParent)
	{
		SurfaceParent = LoadObject<UMaterialInterface>(nullptr, BasicShapeMaterialPath);
		UE_LOG(LogZombie, Display, TEXT("UZMBGameData: sem M_ZMBSurface; usando o BasicShapeMaterial (rode Tools/Unreal/criar_materiais.ps1)"));
	}
	Reload();
}

UZMBGameData* UZMBGameData::Get(const UObject* WorldContext)
{
	const UWorld* World = WorldContext ? WorldContext->GetWorld() : nullptr;
	UGameInstance* GameInstance = World ? World->GetGameInstance() : nullptr;
	return GameInstance ? GameInstance->GetSubsystem<UZMBGameData>() : nullptr;
}

bool UZMBGameData::LoadText(const TCHAR* FileName, std::string& Out)
{
	const FString Path = FPaths::ConvertRelativePathToFull(FPaths::Combine(FPaths::ProjectDir(), TEXT("Data"), FileName));
	TArray<uint8> Bytes;
	if (!FFileHelper::LoadFileToArray(Bytes, *Path))
	{
		Errors.Add(FString::Printf(TEXT("não abri %s (valem os padrões do código)"), *Path));
		return false;
	}
	Out.assign(reinterpret_cast<const char*>(Bytes.GetData()), static_cast<size_t>(Bytes.Num()));
	return true;
}

void UZMBGameData::Reload()
{
	Errors.Reset();
	std::vector<std::string> RuleErrors;
	std::vector<std::string> Missing;
	std::string Text;

	LoadedTuning = ZR::FTuning();
	if (LoadText(TEXT("Ajustes.csv"), Text))
	{
		ZR::LoadTuning(Text, LoadedTuning, RuleErrors, Missing);
	}
	if (LoadText(TEXT("Zumbis.csv"), Text))
	{
		ZR::LoadZombies(Text, LoadedZombies, RuleErrors);
	}
	if (LoadText(TEXT("Armas.csv"), Text))
	{
		ZR::LoadWeapons(Text, LoadedWeapons, RuleErrors);
	}
	if (LoadText(TEXT("Explosivos.csv"), Text))
	{
		ZR::LoadExplosives(Text, LoadedExplosives, RuleErrors);
	}
	if (LoadText(TEXT("Hype.csv"), Text))
	{
		ZR::LoadHypeEvents(Text, LoadedHypeEvents, RuleErrors);
	}
	if (LoadText(TEXT("ChatFalso.csv"), Text))
	{
		ZR::LoadChatLines(Text, LoadedChatLines, RuleErrors);
	}
	if (LoadText(TEXT("Pautas.csv"), Text))
	{
		ZR::LoadChallenges(Text, LoadedChallenges, RuleErrors);
	}

	// sem tabela, o jogo ainda roda: um tipo de zumbi, uma arma, os explosivos e os eventos com os padrões do código
	if (LoadedZombies.empty())
	{
		ZR::FZombieType Fallback;
		Fallback.Id = "comum";
		Fallback.NamePt = "Comum";
		LoadedZombies.push_back(Fallback);
	}
	if (LoadedWeapons.empty())
	{
		ZR::FWeaponType Fallback;
		Fallback.Id = "pistola";
		Fallback.NamePt = "Pistola";
		LoadedWeapons.push_back(Fallback);
	}
	for (const char* Id : {"bujao", "barril", "granada", "carro"})
	{
		if (!FindExplosive(Id))
		{
			ZR::FExplosiveType Fallback;
			Fallback.Id = Id;
			Fallback.NamePt = Id;
			LoadedExplosives.push_back(Fallback);
		}
	}
	// as peças da base dependem dos explosivos (o bujão armado): lê depois deles, já com os padrões
	if (LoadText(TEXT("Construcoes.csv"), Text))
	{
		ZR::LoadBuildPieces(Text, LoadedExplosives, LoadedBuildPieces, RuleErrors);
	}
	if (LoadedBuildPieces.empty())
	{
		ZR::FBuildPieceType Fallback;
		Fallback.Id = "barricada";
		Fallback.NamePt = "Barricada";
		Fallback.NameEn = "Barricade";
		LoadedBuildPieces.push_back(Fallback);
	}
	if (LoadedHypeEvents.size() != static_cast<size_t>(ZR::EHypeEvent::Count))
	{
		LoadedHypeEvents.assign(static_cast<size_t>(ZR::EHypeEvent::Count), ZR::FHypeEventDef());
	}

	for (const std::string& Error : RuleErrors)
	{
		Errors.Add(ToFString(Error));
	}
	for (const FString& Error : Errors)
	{
		UE_LOG(LogZombie, Error, TEXT("Dados: %s"), *Error);
	}
	for (const std::string& Key : Missing)
	{
		UE_LOG(LogZombie, Warning, TEXT("Dados: Ajustes.csv sem '%s' (vale o padrão do código)"), *ToFString(Key));
	}
	UE_LOG(LogZombie, Display, TEXT("Dados: %d tipos de zumbi, %d armas, %d explosivos, %d falas de chat, %d pautas, %d peças da base, %d erros"),
		static_cast<int32>(LoadedZombies.size()), static_cast<int32>(LoadedWeapons.size()), static_cast<int32>(LoadedExplosives.size()),
		static_cast<int32>(LoadedChatLines.size()), static_cast<int32>(LoadedChallenges.size()), static_cast<int32>(LoadedBuildPieces.size()),
		Errors.Num());
}

const ZR::FZombieType* UZMBGameData::FindZombie(const char* Id) const
{
	return ZR::FindById(LoadedZombies, std::string(Id));
}

const ZR::FExplosiveType* UZMBGameData::FindExplosive(const char* Id) const
{
	return ZR::FindById(LoadedExplosives, std::string(Id));
}

UMaterialInterface* UZMBGameData::ColorMaterial(const FLinearColor& Color, float Roughness, float Metallic)
{
	const FString Key = FString::Printf(TEXT("S:%.3f,%.3f,%.3f:%.2f:%.2f"), Color.R, Color.G, Color.B, Roughness, Metallic);
	if (const TObjectPtr<UMaterialInstanceDynamic>* Found = Materials.Find(Key))
	{
		return Found->Get();
	}
	UMaterialInstanceDynamic* Material = UMaterialInstanceDynamic::Create(SurfaceParent, this);
	if (!Material)
	{
		return SurfaceParent;
	}
	Material->SetVectorParameterValue(TEXT("Color"), Color);
	Material->SetScalarParameterValue(TEXT("Roughness"), Roughness);
	Material->SetScalarParameterValue(TEXT("Metallic"), Metallic);
	Materials.Add(Key, Material);
	return Material;
}

UMaterialInstanceDynamic* UZMBGameData::GlowMaterial(const FLinearColor& Color, float Intensity)
{
	const FString Key = FString::Printf(TEXT("G:%.3f,%.3f,%.3f:%.1f"), Color.R, Color.G, Color.B, Intensity);
	if (const TObjectPtr<UMaterialInstanceDynamic>* Found = Materials.Find(Key))
	{
		return Found->Get();
	}
	// sem o material emissivo do projeto, o brilho vira só cor (a luz pontual de cada efeito faz o resto)
	UMaterialInterface* Parent = GlowParent ? GlowParent.Get() : SurfaceParent.Get();
	UMaterialInstanceDynamic* Material = UMaterialInstanceDynamic::Create(Parent, this);
	if (!Material)
	{
		return nullptr;
	}
	Material->SetVectorParameterValue(TEXT("Color"), Color);
	Material->SetScalarParameterValue(TEXT("Intensity"), Intensity);
	Materials.Add(Key, Material);
	return Material;
}

UStaticMesh* UZMBGameData::Shape(EZMBShape Kind)
{
	switch (Kind)
	{
	case EZMBShape::Sphere: return LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Sphere.Sphere"));
	case EZMBShape::Cylinder: return LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	case EZMBShape::Cone: return LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cone.Cone"));
	case EZMBShape::Cube:
	default: return LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube"));
	}
}

FLinearColor UZMBGameData::ToLinear(std::uint32_t Rgb)
{
	const FColor Srgb(static_cast<uint8>((Rgb >> 16) & 0xFF), static_cast<uint8>((Rgb >> 8) & 0xFF), static_cast<uint8>(Rgb & 0xFF));
	return FLinearColor(Srgb);
}

FString UZMBGameData::ToFString(const std::string& Utf8)
{
	return FString(UTF8_TO_TCHAR(Utf8.c_str()));
}

bool UZMBGameData::IsPortuguese()
{
	return FInternationalization::Get().GetCurrentCulture()->GetTwoLetterISOLanguageName() == TEXT("pt");
}
