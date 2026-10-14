// Dados do jogo: lê Data/*.csv (pelas regras puras de Rules/ZRData.h) uma vez por GameInstance e guarda os materiais de
// cor compartilhados. Tabelas e colunas: docs/dados.md. Erro de dado não derruba o jogo: vale o padrão do código e o
// erro vai para o log (LogZombie) e para o HUD.
//
// Materiais: se o projeto tiver /Game/Zombie/Materials/M_ZMBSurface e M_ZMBEmissive (criados por
// Tools/Unreal/criar_materiais.py), usa os dois; senão cai no BasicShapeMaterial da engine (sem brilho próprio).

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Rules/ZRData.h"
#include "ZMBGameData.generated.h"

class UMaterialInterface;
class UMaterialInstanceDynamic;
class UStaticMesh;

enum class EZMBShape : uint8
{
	Cube,
	Sphere,
	Cylinder,
	Cone,
};

UCLASS()
class ZOMBIE_API UZMBGameData : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;

	/** Nulo fora de um mundo de jogo. */
	static UZMBGameData* Get(const UObject* WorldContext);

	/** Lê de novo as tabelas (console: zmb.dados.recarregar). Vale para o que nascer depois. */
	void Reload();

	const ZR::FTuning& Tuning() const { return LoadedTuning; }
	const std::vector<ZR::FZombieType>& Zombies() const { return LoadedZombies; }
	const std::vector<ZR::FWeaponType>& Weapons() const { return LoadedWeapons; }
	const std::vector<ZR::FExplosiveType>& Explosives() const { return LoadedExplosives; }
	const std::vector<ZR::FHypeEventDef>& HypeEvents() const { return LoadedHypeEvents; }
	const std::vector<ZR::FChatLine>& ChatLines() const { return LoadedChatLines; }
	const std::vector<ZR::FChallengeDef>& Challenges() const { return LoadedChallenges; }
	/** Peças do modo construção, na ordem da tabela (Data/Construcoes.csv). */
	const std::vector<ZR::FBuildPieceType>& BuildPieces() const { return LoadedBuildPieces; }

	const ZR::FZombieType* FindZombie(const char* Id) const;
	const ZR::FExplosiveType* FindExplosive(const char* Id) const;

	/** Erros da última leitura (vazio = tudo certo). */
	const TArray<FString>& DataErrors() const { return Errors; }

	/** Material de cor lisa, compartilhado (um por combinação de cor e aspereza). */
	UMaterialInterface* ColorMaterial(const FLinearColor& Color, float Roughness = 0.7f, float Metallic = 0.0f);

	/** Material que brilha (bola de fogo, rastro de tiro, janela acesa). Intensity é o multiplicador da cor emitida. */
	UMaterialInstanceDynamic* GlowMaterial(const FLinearColor& Color, float Intensity);

	/** Formas básicas da engine (/Engine/BasicShapes): 100 cm de lado ou diâmetro, pivô no centro. */
	static UStaticMesh* Shape(EZMBShape Kind);

	static FLinearColor ToLinear(std::uint32_t Rgb);
	static FString ToFString(const std::string& Utf8);

	/** Português (cultura do jogo começa com "pt"): escolhe a coluna _pt ou _en das tabelas. */
	static bool IsPortuguese();

private:
	bool LoadText(const TCHAR* FileName, std::string& Out);

	ZR::FTuning LoadedTuning;
	std::vector<ZR::FZombieType> LoadedZombies;
	std::vector<ZR::FWeaponType> LoadedWeapons;
	std::vector<ZR::FExplosiveType> LoadedExplosives;
	std::vector<ZR::FHypeEventDef> LoadedHypeEvents;
	std::vector<ZR::FChatLine> LoadedChatLines;
	std::vector<ZR::FChallengeDef> LoadedChallenges;
	std::vector<ZR::FBuildPieceType> LoadedBuildPieces;
	TArray<FString> Errors;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInterface> SurfaceParent;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInterface> GlowParent;

	UPROPERTY(Transient)
	TMap<FString, TObjectPtr<UMaterialInstanceDynamic>> Materials;
};
